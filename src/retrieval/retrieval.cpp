#include "pma-internal/retrieval.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace pma::retrieval {
namespace {

using storage::Database;
using storage::Error;
using storage::ErrorCode;
using storage::Migration;
using storage::Statement;
using storage::Transaction;

const std::vector<Migration> migrations{{
    400,
    "retrieval_context_and_evaluation",
    R"sql(
CREATE VIRTUAL TABLE claim_fts USING fts5(claim_id UNINDEXED, text, tokenize='unicode61');
CREATE VIRTUAL TABLE evidence_fts USING fts5(evidence_id UNINDEXED, text, tokenize='unicode61');
CREATE VIRTUAL TABLE entity_fts USING fts5(entity_id UNINDEXED, text, tokenize='unicode61');
CREATE TABLE claim_utility(
  claim_id TEXT PRIMARY KEY REFERENCES claims(id), utility REAL NOT NULL DEFAULT 0.0 CHECK(utility>=-1.0 AND utility<=1.0),
  used_count INTEGER NOT NULL DEFAULT 0, irrelevant_count INTEGER NOT NULL DEFAULT 0,
  harmful_count INTEGER NOT NULL DEFAULT 0, corrected_count INTEGER NOT NULL DEFAULT 0
) STRICT;
CREATE TABLE context_packets(
  id TEXT PRIMARY KEY, interaction_id TEXT NOT NULL, query_fingerprint TEXT NOT NULL,
  query_text TEXT NOT NULL, policy_version TEXT NOT NULL, vector_profile_id TEXT,
  vector_watermark INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
) STRICT;
CREATE TABLE retrieval_candidates(
  packet_id TEXT NOT NULL REFERENCES context_packets(id), claim_id TEXT NOT NULL REFERENCES claims(id),
  selected INTEGER NOT NULL CHECK(selected IN (0,1)), rank INTEGER,
  lexical REAL NOT NULL, vector REAL NOT NULL, entity_predicate REAL NOT NULL,
  graph REAL NOT NULL, context REAL NOT NULL, temporal REAL NOT NULL,
  authority REAL NOT NULL, strength REAL NOT NULL, utility REAL NOT NULL, final_score REAL NOT NULL,
  reasons TEXT NOT NULL, estimated_tokens INTEGER NOT NULL,
  PRIMARY KEY(packet_id,claim_id)
) STRICT;
CREATE TABLE retrieval_outcomes(
  packet_id TEXT NOT NULL REFERENCES context_packets(id), claim_id TEXT NOT NULL REFERENCES claims(id),
  outcome TEXT NOT NULL CHECK(outcome IN ('used','irrelevant','corrected','harmful','unknown')),
  recorded_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  PRIMARY KEY(packet_id,claim_id)
) STRICT;
CREATE TABLE associations(
  left_claim_id TEXT NOT NULL REFERENCES claims(id), right_claim_id TEXT NOT NULL REFERENCES claims(id),
  weight REAL NOT NULL CHECK(weight>=-1.0 AND weight<=1.0), evidence_count INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY(left_claim_id,right_claim_id), CHECK(left_claim_id<right_claim_id)
) STRICT;
)sql"}};

struct Candidate {
  PacketItem item;
};

std::string fts_query(std::string_view query) {
  std::vector<std::string> terms;
  std::string term;
  for (const unsigned char character : query) {
    if (std::isalnum(character) != 0 || character >= 128) {
      term.push_back(static_cast<char>(character));
    } else if (!term.empty()) {
      terms.push_back(std::move(term));
      term.clear();
    }
  }
  if (!term.empty()) {
    terms.push_back(std::move(term));
  }
  std::ostringstream result;
  for (std::size_t index = 0; index < terms.size(); ++index) {
    if (index > 0) {
      result << " OR ";
    }
    result << '"' << terms[index] << '"';
  }
  return result.str();
}

bool exists(Database &database, std::string_view sql, std::string_view id) {
  auto query = database.prepare(sql);
  query.bind(1, id);
  return query.step();
}

std::string render_claim(Statement &query) {
  return query.column_text(1) + " " + query.column_text(2) + " " + query.column_text(3);
}

PacketItem load_item(Database &database, std::string_view claim_id) {
  auto query = database.prepare(
      "SELECT c.id,s.canonical_name,p.name,COALESCE(o.canonical_name,v.text_value),c.status,"
      "c.authority,c.strength,COALESCE(u.utility,0.0) FROM claims c "
      "JOIN entities s ON s.id=c.subject_id JOIN predicates p ON p.id=c.predicate_id "
      "LEFT JOIN entities o ON o.id=c.object_entity_id LEFT JOIN typed_values v ON v.id=c.typed_value_id "
      "LEFT JOIN claim_utility u ON u.claim_id=c.id WHERE c.id=?1");
  query.bind(1, claim_id);
  if (!query.step()) {
    throw Error(ErrorCode::constraint, 0, "retrieval claim not found");
  }
  PacketItem item;
  item.claim_id = query.column_text(0);
  item.text = render_claim(query);
  item.status = query.column_text(4);
  item.authority = query.column_text(5);
  item.scores.strength = std::min(1.0, query.column_double(6));
  item.scores.utility = query.column_double(7);
  item.scores.temporal = item.status == "active" ? 1.0 : (item.status == "superseded" ? 0.1 : 0.25);
  item.scores.authority =
      (item.authority.find("decision") != std::string::npos ||
       item.authority.find("explicit") != std::string::npos)
          ? 1.0
          : (item.authority == "inferred" ? 0.25 : 0.6);
  item.estimated_tokens = Engine::estimate_tokens(item.text);
  auto relationships = database.prepare(
      "SELECT relation FROM graph_edges WHERE source_id=?1 OR target_id=?1");
  relationships.bind(1, claim_id);
  while (relationships.step()) {
    const auto relation = relationships.column_text(0);
    item.conflict = item.conflict || relation == "contradicts";
    item.qualification = item.qualification || relation == "qualifies";
  }
  auto contradiction = database.prepare(
      "SELECT 1 FROM claim_evidence WHERE claim_id=?1 AND relation='contradicted_by' LIMIT 1");
  contradiction.bind(1, claim_id);
  item.conflict = item.conflict || contradiction.step() || item.status == "contradicted";
  return item;
}

void reason(PacketItem &item, std::string value) {
  if (std::find(item.reasons.begin(), item.reasons.end(), value) == item.reasons.end()) {
    item.reasons.push_back(std::move(value));
  }
}

std::string join_reasons(const std::vector<std::string> &reasons) {
  std::ostringstream output;
  for (std::size_t index = 0; index < reasons.size(); ++index) {
    if (index > 0) {
      output << ',';
    }
    output << reasons[index];
  }
  return output.str();
}

std::string outcome_text(Outcome outcome) {
  switch (outcome) {
  case Outcome::used:
    return "used";
  case Outcome::irrelevant:
    return "irrelevant";
  case Outcome::corrected:
    return "corrected";
  case Outcome::harmful:
    return "harmful";
  case Outcome::unknown:
    return "unknown";
  }
  return "unknown";
}

} // namespace

Engine::Engine(Database &database, vectors::Manager *vector_manager)
    : database_(&database), vector_manager_(vector_manager) {}

void Engine::initialize(std::string_view build_version) {
  storage::migrate(*database_, migrations, build_version);
  rebuild_fts();
}

void Engine::rebuild_fts() {
  Transaction transaction(*database_);
  database_->execute("DELETE FROM claim_fts; DELETE FROM evidence_fts; DELETE FROM entity_fts;");
  database_->execute(
      "INSERT INTO claim_fts(claim_id,text) SELECT c.id,s.canonical_name||' '||p.name||' '||"
      "COALESCE(o.canonical_name,v.text_value) FROM claims c JOIN entities s ON s.id=c.subject_id "
      "JOIN predicates p ON p.id=c.predicate_id LEFT JOIN entities o ON o.id=c.object_entity_id "
      "LEFT JOIN typed_values v ON v.id=c.typed_value_id;"
      "INSERT INTO evidence_fts(evidence_id,text) SELECT id,text FROM evidence_segments;"
      "INSERT INTO entity_fts(entity_id,text) SELECT e.id,e.canonical_name||' '||COALESCE(group_concat(a.alias,' '),'') "
      "FROM entities e LEFT JOIN entity_aliases a ON a.entity_id=e.id GROUP BY e.id;");
  transaction.commit();
}

ContextPacket Engine::recall(const RecallRequest &request) {
  if (request.packet_id.empty() || request.query.empty()) {
    throw Error(ErrorCode::constraint, 0, "recall requires packet id and query");
  }
  std::unordered_map<std::string, Candidate> candidates;
  auto ensure = [&](const std::string &id) -> PacketItem & {
    auto found = candidates.find(id);
    if (found == candidates.end()) {
      found = candidates.emplace(id, Candidate{load_item(*database_, id)}).first;
    }
    return found->second.item;
  };

  const std::string match = fts_query(request.query);
  if (!match.empty()) {
    auto lexical = database_->prepare(
        "SELECT claim_id,bm25(claim_fts) FROM claim_fts WHERE claim_fts MATCH ?1 ORDER BY bm25(claim_fts) LIMIT ?2");
    lexical.bind(1, match);
    lexical.bind(2, static_cast<std::int64_t>(request.candidate_limit));
    while (lexical.step()) {
      auto &item = ensure(lexical.column_text(0));
      item.scores.lexical = std::max(item.scores.lexical,
                                     1.0 / (1.0 + std::abs(lexical.column_double(1))));
      reason(item, "fts");
    }

    auto entity = database_->prepare(
        "SELECT DISTINCT c.id FROM entity_fts f JOIN claims c ON c.subject_id=f.entity_id "
        "WHERE entity_fts MATCH ?1 UNION SELECT DISTINCT c.id FROM claim_fts f JOIN claims c "
        "ON c.id=f.claim_id JOIN predicates p ON p.id=c.predicate_id WHERE p.name LIKE ?2 LIMIT ?3");
    entity.bind(1, match);
    entity.bind(2, "%" + request.query + "%");
    entity.bind(3, static_cast<std::int64_t>(request.candidate_limit));
    while (entity.step()) {
      auto &item = ensure(entity.column_text(0));
      item.scores.entity_predicate = 1.0;
      reason(item, "entity_or_predicate");
    }
  }

  ContextPacket packet{request.packet_id,
                       request.interaction_id,
                       storage::sha256(request.query),
                       "hybrid-v1"};
  if (vector_manager_ != nullptr && request.vector_store_id.has_value() &&
      request.query_vector.has_value()) {
    const auto info = vector_manager_->inspect(*request.vector_store_id);
    packet.vector_profile_id = info.profile_id;
    packet.vector_watermark = info.watermark;
    for (const auto &result : vector_manager_->search(*request.vector_store_id,
                                                      *request.query_vector,
                                                      request.candidate_limit)) {
      auto document = database_->prepare(
          "SELECT object_id FROM embedding_documents WHERE id=?1 AND object_type='claim'");
      document.bind(1, result.document_id);
      if (document.step()) {
        auto &item = ensure(document.column_text(0));
        item.scores.vector = std::max(0.0, static_cast<double>((result.score + 1.0F) / 2.0F));
        reason(item, "vector");
      }
    }
  }

  std::set<std::string> frontier;
  for (const auto &[id, candidate] : candidates) {
    if (candidate.item.scores.lexical > 0.0 || candidate.item.scores.entity_predicate > 0.0) {
      frontier.insert(id);
    }
  }
  std::set<std::string> visited = frontier;
  for (int hop = 1; hop <= 2 && !frontier.empty(); ++hop) {
    std::set<std::string> next;
    for (const auto &seed : frontier) {
      auto edges = database_->prepare(
          "SELECT source_id,target_id,edge_class FROM graph_edges WHERE source_id=?1 OR target_id=?1");
      edges.bind(1, seed);
      while (edges.step()) {
        const auto edge_class = edges.column_text(2);
        if (edge_class == "associative") {
          continue;
        }
        const std::string other = edges.column_text(0) == seed ? edges.column_text(1)
                                                               : edges.column_text(0);
        if (exists(*database_, "SELECT 1 FROM claims WHERE id=?1", other)) {
          auto &item = ensure(other);
          item.scores.graph = std::max(item.scores.graph, 1.0 / static_cast<double>(hop + 1));
          reason(item, "graph");
          if (visited.insert(other).second) {
            next.insert(other);
          }
        }
      }
    }
    frontier = std::move(next);
  }

  for (const auto &context_id : request.active_context_ids) {
    auto context = database_->prepare(
        "SELECT CASE WHEN source_id=?1 THEN target_id ELSE source_id END FROM graph_edges "
        "WHERE edge_class='contextual' AND (source_id=?1 OR target_id=?1)");
    context.bind(1, context_id);
    while (context.step()) {
      const auto claim_id = context.column_text(0);
      if (exists(*database_, "SELECT 1 FROM claims WHERE id=?1", claim_id)) {
        auto &item = ensure(claim_id);
        item.scores.context = 1.0;
        reason(item, "active_context");
      }
    }
  }

  for (auto &[id, candidate] : candidates) {
    auto &scores = candidate.item.scores;
    scores.final_score = 0.30 * scores.lexical + 0.20 * scores.vector +
                         0.15 * scores.entity_predicate + 0.10 * scores.graph +
                         0.10 * scores.context + 0.05 * scores.temporal +
                         0.10 * scores.authority + 0.05 * scores.strength +
                         0.10 * scores.utility;
    if (candidate.item.conflict) {
      reason(candidate.item, "material_conflict");
    }
    if (candidate.item.qualification) {
      reason(candidate.item, "qualification");
    }
    packet.candidates.push_back(std::move(candidate.item));
  }
  std::sort(packet.candidates.begin(), packet.candidates.end(),
            [](const auto &left, const auto &right) {
              if (left.scores.final_score != right.scores.final_score) {
                return left.scores.final_score > right.scores.final_score;
              }
              return left.claim_id < right.claim_id;
            });
  for (const auto &item : packet.candidates) {
    if (packet.selected.size() >= request.selected_limit) {
      break;
    }
    if (item.scores.utility <= -0.75 || item.scores.final_score <= 0.05) {
      continue;
    }
    packet.selected.push_back(item);
  }

  Transaction transaction(*database_);
  auto packet_insert = database_->prepare(
      "INSERT INTO context_packets(id,interaction_id,query_fingerprint,query_text,policy_version,"
      "vector_profile_id,vector_watermark) VALUES(?1,?2,?3,?4,?5,?6,?7)");
  packet_insert.bind(1, packet.id);
  packet_insert.bind(2, packet.interaction_id);
  packet_insert.bind(3, packet.query_fingerprint);
  packet_insert.bind(4, request.query);
  packet_insert.bind(5, packet.policy_version);
  if (packet.vector_profile_id.has_value()) {
    packet_insert.bind(6, *packet.vector_profile_id);
  } else {
    packet_insert.bind_null(6);
  }
  packet_insert.bind(7, packet.vector_watermark);
  packet_insert.execute();
  std::set<std::string> selected_ids;
  for (const auto &item : packet.selected) {
    selected_ids.insert(item.claim_id);
  }
  for (std::size_t rank = 0; rank < packet.candidates.size(); ++rank) {
    const auto &item = packet.candidates[rank];
    const auto &s = item.scores;
    auto insert = database_->prepare(
        "INSERT INTO retrieval_candidates VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16)");
    insert.bind(1, packet.id);
    insert.bind(2, item.claim_id);
    insert.bind(3, static_cast<std::int64_t>(selected_ids.contains(item.claim_id)));
    insert.bind(4, static_cast<std::int64_t>(rank));
    insert.bind(5, s.lexical);
    insert.bind(6, s.vector);
    insert.bind(7, s.entity_predicate);
    insert.bind(8, s.graph);
    insert.bind(9, s.context);
    insert.bind(10, s.temporal);
    insert.bind(11, s.authority);
    insert.bind(12, s.strength);
    insert.bind(13, s.utility);
    insert.bind(14, s.final_score);
    insert.bind(15, join_reasons(item.reasons));
    insert.bind(16, static_cast<std::int64_t>(item.estimated_tokens));
    insert.execute();
  }
  transaction.commit();
  return packet;
}

ContextSlice Engine::render_slice(const ContextPacket &packet, std::size_t token_budget) const {
  ContextSlice result;
  const std::string opening = "<PMA-memory>\nRelevant memory:\n";
  const std::string closing = "</PMA-memory>";
  if (estimate_tokens(opening + closing) > token_budget) {
    return result;
  }
  std::string rendered = opening;
  std::set<std::string> seen;
  for (const auto &item : packet.selected) {
    std::string normalized = item.text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (!seen.insert(normalized).second) {
      continue;
    }
    std::string label;
    if (item.conflict) {
      label = "[conflict] ";
    } else if (item.qualification) {
      label = "[qualification] ";
    } else if (item.authority.find("decision") != std::string::npos) {
      label = "[decision] ";
    } else if (item.authority == "inferred") {
      label = "[inference, uncertain] ";
    }
    const std::string line = "- " + label + item.text + "\n";
    if (estimate_tokens(rendered + line + closing) > token_budget) {
      continue;
    }
    rendered += line;
    result.included_claim_ids.push_back(item.claim_id);
  }
  if (result.included_claim_ids.empty()) {
    return result;
  }
  rendered += closing;
  result.text = std::move(rendered);
  result.estimated_tokens = estimate_tokens(result.text);
  return result;
}

void Engine::record_outcome(std::string_view packet_id, std::string_view claim_id,
                            Outcome outcome) {
  auto candidate = database_->prepare(
      "SELECT 1 FROM retrieval_candidates WHERE packet_id=?1 AND claim_id=?2");
  candidate.bind(1, packet_id);
  candidate.bind(2, claim_id);
  if (!candidate.step()) {
    throw Error(ErrorCode::constraint, 0, "claim was not in retrieval packet");
  }
  const std::string value = outcome_text(outcome);
  Transaction transaction(*database_);
  auto insert = database_->prepare(
      "INSERT INTO retrieval_outcomes(packet_id,claim_id,outcome) VALUES(?1,?2,?3)");
  insert.bind(1, packet_id);
  insert.bind(2, claim_id);
  insert.bind(3, value);
  insert.execute();
  double delta = 0.0;
  std::string counter;
  if (outcome == Outcome::used) {
    delta = 0.15;
    counter = "used_count";
  } else if (outcome == Outcome::irrelevant) {
    delta = -0.20;
    counter = "irrelevant_count";
  } else if (outcome == Outcome::corrected) {
    delta = -0.35;
    counter = "corrected_count";
  } else if (outcome == Outcome::harmful) {
    delta = -0.50;
    counter = "harmful_count";
  }
  auto ensure_utility = database_->prepare(
      "INSERT OR IGNORE INTO claim_utility(claim_id) VALUES(?1)");
  ensure_utility.bind(1, claim_id);
  ensure_utility.execute();
  if (!counter.empty()) {
    auto update = database_->prepare(
        "UPDATE claim_utility SET utility=max(-1.0,min(1.0,utility+?2))," + counter +
        "=" + counter + "+1 WHERE claim_id=?1");
    update.bind(1, claim_id);
    update.bind(2, delta);
    update.execute();
  }
  if (outcome == Outcome::used) {
    auto peers = database_->prepare(
        "SELECT claim_id FROM retrieval_candidates WHERE packet_id=?1 AND selected=1 AND claim_id<>?2");
    peers.bind(1, packet_id);
    peers.bind(2, claim_id);
    while (peers.step()) {
      const std::string peer = peers.column_text(0);
      const std::string left = std::min(std::string(claim_id), peer);
      const std::string right = std::max(std::string(claim_id), peer);
      auto association = database_->prepare(
          "INSERT INTO associations(left_claim_id,right_claim_id,weight,evidence_count) "
          "VALUES(?1,?2,0.1,1) ON CONFLICT(left_claim_id,right_claim_id) DO UPDATE SET "
          "weight=min(1.0,weight+0.1),evidence_count=evidence_count+1");
      association.bind(1, left);
      association.bind(2, right);
      association.execute();
    }
  }
  transaction.commit();
}

double Engine::utility(std::string_view claim_id) {
  auto query = database_->prepare("SELECT utility FROM claim_utility WHERE claim_id=?1");
  query.bind(1, claim_id);
  return query.step() ? query.column_double(0) : 0.0;
}

std::size_t Engine::estimate_tokens(std::string_view text) {
  return text.empty() ? 0 : (text.size() + 3) / 4;
}

} // namespace pma::retrieval
