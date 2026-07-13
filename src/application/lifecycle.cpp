#include "pma-internal/lifecycle.hpp"

#include <utility>

namespace pma::lifecycle {
namespace {

using storage::Database;
using storage::Error;
using storage::ErrorCode;
using storage::Migration;
using storage::Statement;
using storage::Transaction;

const std::vector<Migration> migrations{{
    200,
    "lifecycle_jobs_and_operations",
    R"sql(
CREATE TABLE jobs(
  id TEXT PRIMARY KEY, job_type TEXT NOT NULL, episode_id TEXT REFERENCES episodes(id),
  state TEXT NOT NULL CHECK(state IN ('queued','running','succeeded','failed','cancelled')),
  attempts INTEGER NOT NULL DEFAULT 0 CHECK(attempts>=0), checkpoint INTEGER NOT NULL DEFAULT 0,
  error_code TEXT, error_detail TEXT,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
) STRICT;
CREATE TABLE job_attempts(
  job_id TEXT NOT NULL REFERENCES jobs(id), attempt INTEGER NOT NULL,
  state TEXT NOT NULL CHECK(state IN ('running','succeeded','failed')),
  error_code TEXT, started_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  finished_at TEXT, PRIMARY KEY(job_id,attempt)
) STRICT;
CREATE TABLE job_checkpoints(
  job_id TEXT NOT NULL REFERENCES jobs(id), checkpoint INTEGER NOT NULL, phase TEXT NOT NULL,
  recorded_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  PRIMARY KEY(job_id,checkpoint)
) STRICT;
CREATE TABLE memory_operations(
  id TEXT PRIMARY KEY, job_id TEXT REFERENCES jobs(id), operation_type TEXT NOT NULL,
  target_id TEXT, result_id TEXT, status TEXT NOT NULL CHECK(status IN ('applied','rejected')),
  reason_code TEXT, applied_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
) STRICT;
CREATE TABLE operation_inputs(
  operation_id TEXT NOT NULL REFERENCES memory_operations(id), evidence_id TEXT REFERENCES evidence_segments(id),
  input_role TEXT NOT NULL, PRIMARY KEY(operation_id,evidence_id,input_role)
) STRICT;
CREATE TABLE embedding_documents(
  id TEXT PRIMARY KEY, object_type TEXT NOT NULL, object_id TEXT NOT NULL,
  text TEXT NOT NULL, revision INTEGER NOT NULL DEFAULT 1
) STRICT;
CREATE TABLE embedding_events(
  event_seq INTEGER PRIMARY KEY AUTOINCREMENT, operation_id TEXT NOT NULL REFERENCES memory_operations(id),
  object_id TEXT NOT NULL, action TEXT NOT NULL CHECK(action IN ('upsert','delete')),
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
) STRICT;
)sql"}};

JobState parse_state(std::string_view state) {
  if (state == "queued") {
    return JobState::queued;
  }
  if (state == "running") {
    return JobState::running;
  }
  if (state == "succeeded") {
    return JobState::succeeded;
  }
  return JobState::failed;
}

std::string operation_text(OperationType operation) {
  switch (operation) {
  case OperationType::create:
    return "create";
  case OperationType::support:
    return "support";
  case OperationType::qualify:
    return "qualify";
  case OperationType::contradict:
    return "contradict";
  case OperationType::supersede:
    return "supersede";
  case OperationType::contextualize:
    return "contextualize";
  case OperationType::deactivate:
    return "deactivate";
  case OperationType::restore:
    return "restore";
  }
  return "unsupported";
}

bool exists(Database &database, std::string_view sql, std::string_view id) {
  auto query = database.prepare(sql);
  query.bind(1, id);
  return query.step();
}

void require_exists(Database &database, std::string_view sql, std::string_view id,
                    std::string_view type) {
  if (!exists(database, sql, id)) {
    throw Error(ErrorCode::constraint, 0,
                std::string("invalid ") + std::string(type) + " reference");
  }
}

void bind_optional(Statement &statement, std::size_t index,
                   const std::optional<std::string> &value) {
  if (value.has_value()) {
    statement.bind(index, *value);
  } else {
    statement.bind_null(index);
  }
}

void validate(Database &database, const Proposal &proposal) {
  if (proposal.confidence < 0.0 || proposal.confidence > 1.0) {
    throw Error(ErrorCode::constraint, 0, "proposal confidence is outside [0,1]");
  }
  for (const auto &evidence : proposal.evidence_ids) {
    require_exists(database, "SELECT 1 FROM evidence_segments WHERE id=?1", evidence, "evidence");
  }
  const bool creates = proposal.operation == OperationType::create ||
                       proposal.operation == OperationType::qualify ||
                       proposal.operation == OperationType::supersede;
  if (creates) {
    require_exists(database, "SELECT 1 FROM entities WHERE id=?1", proposal.subject_id, "subject");
    require_exists(database, "SELECT 1 FROM predicates WHERE id=?1", proposal.predicate_id,
                   "predicate");
    if (proposal.object_entity_id.has_value() == proposal.literal_text.has_value()) {
      throw Error(ErrorCode::constraint, 0, "proposal must have exactly one object form");
    }
    if (proposal.object_entity_id.has_value()) {
      require_exists(database, "SELECT 1 FROM entities WHERE id=?1", *proposal.object_entity_id,
                     "object entity");
    }
    if (proposal.evidence_ids.empty()) {
      throw Error(ErrorCode::constraint, 0, "claim proposal requires evidence");
    }
  }
  const bool targets = proposal.operation != OperationType::create;
  if (targets) {
    if (!proposal.target_claim_id.has_value()) {
      throw Error(ErrorCode::constraint, 0, "operation requires a target claim");
    }
    require_exists(database, "SELECT 1 FROM claims WHERE id=?1", *proposal.target_claim_id,
                   "target claim");
  }
  if ((proposal.operation == OperationType::support ||
       proposal.operation == OperationType::contradict) &&
      proposal.evidence_ids.empty()) {
    throw Error(ErrorCode::constraint, 0, "operation requires evidence");
  }
  if (proposal.operation == OperationType::contextualize) {
    if (!proposal.context_id.has_value()) {
      throw Error(ErrorCode::constraint, 0, "contextualize requires context");
    }
    require_exists(database, "SELECT 1 FROM contexts WHERE id=?1", *proposal.context_id,
                   "context");
  }
}

void insert_node(Database &database, std::string_view id, std::string_view type) {
  auto node = database.prepare("INSERT INTO graph_nodes(id,node_type) VALUES(?1,?2)");
  node.bind(1, id);
  node.bind(2, type);
  node.execute();
}

} // namespace

ScriptedProvider::ScriptedProvider(std::vector<Proposal> proposals)
    : proposals_(std::move(proposals)) {}

void ScriptedProvider::fail_next(std::string message) { failure_ = std::move(message); }

std::vector<Proposal>
ScriptedProvider::propose(const std::vector<graph::EvidenceSegment> & /*evidence*/) {
  if (failure_.has_value()) {
    const auto message = std::move(*failure_);
    failure_.reset();
    throw ProviderError(message);
  }
  return proposals_;
}

Processor::Processor(Database &database, graph::Store &graph_store)
    : database_(&database), graph_store_(&graph_store) {}

void Processor::initialize(std::string_view build_version) {
  graph_store_->initialize(build_version);
  storage::migrate(*database_, migrations, build_version);
}

void Processor::observe(const Observation &observation) {
  Transaction transaction(*database_);
  auto episode_lookup = database_->prepare(
      "SELECT id FROM episodes WHERE source_adapter=?1 AND external_key=?2");
  episode_lookup.bind(1, observation.source_adapter);
  episode_lookup.bind(2, observation.external_key);
  if (episode_lookup.step()) {
    if (episode_lookup.column_text(0) != observation.episode_id) {
      throw Error(ErrorCode::constraint, 0, "observation identity conflicts with existing episode");
    }
  } else {
    auto episode = database_->prepare(
        "INSERT INTO episodes(id,source_adapter,external_key) VALUES(?1,?2,?3)");
    episode.bind(1, observation.episode_id);
    episode.bind(2, observation.source_adapter);
    episode.bind(3, observation.external_key);
    episode.execute();
  }

  for (const auto &event : observation.events) {
    auto lookup = database_->prepare(
        "SELECT id,ordinal,text FROM evidence_segments WHERE episode_id=?1 AND idempotency_key=?2");
    lookup.bind(1, observation.episode_id);
    lookup.bind(2, event.idempotency_key);
    if (lookup.step()) {
      if (lookup.column_text(0) != event.id || lookup.column_int64(1) != event.ordinal ||
          lookup.column_text(2) != event.text) {
        throw Error(ErrorCode::constraint, 0, "observation idempotency conflict");
      }
      continue;
    }
    insert_node(*database_, event.id, "evidence_segment");
    auto evidence = database_->prepare(
        "INSERT INTO evidence_segments(id,episode_id,ordinal,text,idempotency_key,observed_agent,"
        "model_runtime) VALUES(?1,?2,?3,?4,?5,?6,?7)");
    evidence.bind(1, event.id);
    evidence.bind(2, observation.episode_id);
    evidence.bind(3, event.ordinal);
    evidence.bind(4, event.text);
    evidence.bind(5, event.idempotency_key);
    bind_optional(evidence, 6, event.observed_agent);
    bind_optional(evidence, 7, event.model_runtime);
    evidence.execute();
  }
  auto job = database_->prepare(
      "INSERT OR IGNORE INTO jobs(id,job_type,episode_id,state) VALUES(?1,'extract',?2,'queued')");
  job.bind(1, observation.job_id);
  job.bind(2, observation.episode_id);
  job.execute();
  transaction.commit();
}

Job Processor::get_job(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,episode_id,state,attempts,checkpoint,error_code FROM jobs WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    throw Error(ErrorCode::constraint, 0, "job not found");
  }
  return {query.column_text(0), query.column_text(1), parse_state(query.column_text(2)),
          query.column_int64(3), query.column_int64(4),
          query.column_is_null(5) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(5)}};
}

void Processor::run_job(std::string_view id, ProposalProvider &provider,
                        std::int64_t max_attempts) {
  auto job = get_job(id);
  if (job.state == JobState::succeeded || job.state == JobState::failed) {
    return;
  }
  const std::int64_t attempt = job.attempts + 1;
  {
    Transaction transaction(*database_);
    auto update = database_->prepare(
        "UPDATE jobs SET state='running',attempts=?2,error_code=NULL,error_detail=NULL,"
        "updated_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE id=?1");
    update.bind(1, id);
    update.bind(2, attempt);
    update.execute();
    auto record = database_->prepare(
        "INSERT INTO job_attempts(job_id,attempt,state) VALUES(?1,?2,'running')");
    record.bind(1, id);
    record.bind(2, attempt);
    record.execute();
    transaction.commit();
  }

  try {
    const auto evidence = graph_store_->list_evidence(job.episode_id);
    const auto proposals = provider.propose(evidence);
    apply(id, proposals);
    Transaction transaction(*database_);
    auto update = database_->prepare(
        "UPDATE jobs SET state='succeeded',checkpoint=?2,updated_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE id=?1");
    update.bind(1, id);
    update.bind(2, static_cast<std::int64_t>(proposals.size()));
    update.execute();
    auto attempt_update = database_->prepare(
        "UPDATE job_attempts SET state='succeeded',finished_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE job_id=?1 AND attempt=?2");
    attempt_update.bind(1, id);
    attempt_update.bind(2, attempt);
    attempt_update.execute();
    auto checkpoint = database_->prepare(
        "INSERT INTO job_checkpoints(job_id,checkpoint,phase) VALUES(?1,?2,'consolidated')");
    checkpoint.bind(1, id);
    checkpoint.bind(2, static_cast<std::int64_t>(proposals.size()));
    checkpoint.execute();
    transaction.commit();
  } catch (const ProviderError &error) {
    const bool poison = attempt >= max_attempts;
    Transaction transaction(*database_);
    auto update = database_->prepare(
        "UPDATE jobs SET state=?2,error_code='PROVIDER_UNAVAILABLE',error_detail=?3,"
        "updated_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE id=?1");
    update.bind(1, id);
    update.bind(2, poison ? "failed" : "queued");
    update.bind(3, error.what());
    update.execute();
    auto attempt_update = database_->prepare(
        "UPDATE job_attempts SET state='failed',error_code='PROVIDER_UNAVAILABLE',"
        "finished_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE job_id=?1 AND attempt=?2");
    attempt_update.bind(1, id);
    attempt_update.bind(2, attempt);
    attempt_update.execute();
    transaction.commit();
  } catch (const Error &) {
    Transaction transaction(*database_);
    auto update = database_->prepare(
        "UPDATE jobs SET state='failed',error_code='INVALID_PROPOSAL',"
        "updated_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE id=?1");
    update.bind(1, id);
    update.execute();
    auto attempt_update = database_->prepare(
        "UPDATE job_attempts SET state='failed',error_code='INVALID_PROPOSAL',"
        "finished_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE job_id=?1 AND attempt=?2");
    attempt_update.bind(1, id);
    attempt_update.bind(2, attempt);
    attempt_update.execute();
    transaction.commit();
  }
}

void Processor::apply(std::string_view job_id, const std::vector<Proposal> &proposals) {
  require_exists(*database_, "SELECT 1 FROM jobs WHERE id=?1", job_id, "job");
  for (const auto &proposal : proposals) {
    validate(*database_, proposal);
  }

  Transaction transaction(*database_);
  for (std::size_t index = 0; index < proposals.size(); ++index) {
    const auto &proposal = proposals[index];
    const std::string prefix = std::string(job_id) + ":" + std::to_string(index);
    const std::string operation_id = prefix + ":operation";
    std::optional<std::string> result_id;
    std::optional<std::string> target = proposal.target_claim_id;

    const bool creates = proposal.operation == OperationType::create ||
                         proposal.operation == OperationType::qualify ||
                         proposal.operation == OperationType::supersede;
    if (creates) {
      const std::string claim_id = prefix + ":claim";
      result_id = claim_id;
      std::optional<std::string> value_id;
      if (proposal.literal_text.has_value()) {
        value_id = prefix + ":value";
        auto value = database_->prepare(
            "INSERT INTO typed_values(id,value_type,text_value) VALUES(?1,'text',?2)");
        value.bind(1, *value_id);
        value.bind(2, *proposal.literal_text);
        value.execute();
      }
      insert_node(*database_, claim_id, "claim");
      auto claim = database_->prepare(
          "INSERT INTO claims(id,subject_id,predicate_id,object_entity_id,typed_value_id,polarity,"
          "modality,authority,confidence,status,source_kind) "
          "VALUES(?1,?2,?3,?4,?5,'positive','asserted',?6,?7,'active','evidence')");
      claim.bind(1, claim_id);
      claim.bind(2, proposal.subject_id);
      claim.bind(3, proposal.predicate_id);
      bind_optional(claim, 4, proposal.object_entity_id);
      bind_optional(claim, 5, value_id);
      claim.bind(6, proposal.authority);
      claim.bind(7, proposal.confidence);
      claim.execute();
      for (const auto &evidence_id : proposal.evidence_ids) {
        auto link = database_->prepare(
            "INSERT INTO claim_evidence(claim_id,evidence_id,relation) VALUES(?1,?2,'supported_by')");
        link.bind(1, claim_id);
        link.bind(2, evidence_id);
        link.execute();
      }
      if (proposal.operation == OperationType::qualify ||
          proposal.operation == OperationType::supersede) {
        const std::string edge_id = prefix + ":edge";
        auto edge = database_->prepare(
            "INSERT INTO graph_edges(id,source_id,target_id,edge_class,relation) "
            "VALUES(?1,?2,?3,'structural',?4)");
        edge.bind(1, edge_id);
        edge.bind(2, claim_id);
        edge.bind(3, *proposal.target_claim_id);
        edge.bind(4, proposal.operation == OperationType::qualify ? "qualifies" : "supersedes");
        edge.execute();
      }
      if (proposal.operation == OperationType::supersede) {
        auto old_claim = database_->prepare("UPDATE claims SET status='superseded' WHERE id=?1");
        old_claim.bind(1, *proposal.target_claim_id);
        old_claim.execute();
        auto old_node = database_->prepare("UPDATE graph_nodes SET status='superseded' WHERE id=?1");
        old_node.bind(1, *proposal.target_claim_id);
        old_node.execute();
      }
    } else if (proposal.operation == OperationType::support ||
               proposal.operation == OperationType::contradict) {
      const std::string relation = proposal.operation == OperationType::support
                                       ? "supported_by"
                                       : "contradicted_by";
      for (const auto &evidence_id : proposal.evidence_ids) {
        auto link = database_->prepare(
            "INSERT OR IGNORE INTO claim_evidence(claim_id,evidence_id,relation) VALUES(?1,?2,?3)");
        link.bind(1, *proposal.target_claim_id);
        link.bind(2, evidence_id);
        link.bind(3, relation);
        link.execute();
      }
      result_id = proposal.target_claim_id;
      if (proposal.operation == OperationType::contradict) {
        auto update = database_->prepare("UPDATE claims SET status='contradicted' WHERE id=?1");
        update.bind(1, *proposal.target_claim_id);
        update.execute();
      }
    } else if (proposal.operation == OperationType::contextualize) {
      result_id = proposal.target_claim_id;
      auto edge = database_->prepare(
          "INSERT INTO graph_edges(id,source_id,target_id,edge_class,relation) "
          "VALUES(?1,?2,?3,'contextual','relevant_to')");
      edge.bind(1, prefix + ":edge");
      edge.bind(2, *proposal.target_claim_id);
      edge.bind(3, *proposal.context_id);
      edge.execute();
    } else {
      result_id = proposal.target_claim_id;
      const bool restore = proposal.operation == OperationType::restore;
      auto claim = database_->prepare("UPDATE claims SET status=?2 WHERE id=?1");
      claim.bind(1, *proposal.target_claim_id);
      claim.bind(2, restore ? "active" : "suppressed");
      claim.execute();
      auto node = database_->prepare("UPDATE graph_nodes SET status=?2 WHERE id=?1");
      node.bind(1, *proposal.target_claim_id);
      node.bind(2, restore ? "active" : "suppressed");
      node.execute();
    }

    auto audit = database_->prepare(
        "INSERT INTO memory_operations(id,job_id,operation_type,target_id,result_id,status) "
        "VALUES(?1,?2,?3,?4,?5,'applied')");
    audit.bind(1, operation_id);
    audit.bind(2, job_id);
    audit.bind(3, operation_text(proposal.operation));
    bind_optional(audit, 4, target);
    bind_optional(audit, 5, result_id);
    audit.execute();
    for (const auto &evidence_id : proposal.evidence_ids) {
      auto input = database_->prepare(
          "INSERT INTO operation_inputs(operation_id,evidence_id,input_role) VALUES(?1,?2,'evidence')");
      input.bind(1, operation_id);
      input.bind(2, evidence_id);
      input.execute();
    }
    auto event = database_->prepare(
        "INSERT INTO embedding_events(operation_id,object_id,action) VALUES(?1,?2,'upsert')");
    event.bind(1, operation_id);
    event.bind(2, *result_id);
    event.execute();
  }
  transaction.commit();
}

std::string Processor::correct(std::string_view operation_id,
                               std::string_view target_claim_id,
                               std::string_view evidence_id,
                               std::string_view replacement_text, double confidence) {
  const auto target = graph_store_->get_claim(target_claim_id);
  auto episode = database_->prepare(
      "SELECT episode_id FROM evidence_segments WHERE id=?1");
  episode.bind(1, evidence_id);
  if (!episode.step()) {
    throw Error(ErrorCode::constraint, 0, "invalid correction evidence");
  }
  auto job = database_->prepare(
      "INSERT INTO jobs(id,job_type,episode_id,state) VALUES(?1,'direct_correction',?2,'running')");
  job.bind(1, operation_id);
  job.bind(2, episode.column_text(0));
  job.execute();
  Proposal proposal{OperationType::supersede,
                    target.subject_id,
                    target.predicate_id,
                    std::nullopt,
                    std::string(replacement_text),
                    std::string(target_claim_id),
                    std::nullopt,
                    {std::string(evidence_id)},
                    "explicit_correction",
                    confidence};
  apply(operation_id, {proposal});
  auto finish = database_->prepare("UPDATE jobs SET state='succeeded',checkpoint=1 WHERE id=?1");
  finish.bind(1, operation_id);
  finish.execute();
  return std::string(operation_id) + ":0:claim";
}

} // namespace pma::lifecycle
