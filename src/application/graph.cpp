#include "pma-internal/graph.hpp"

#include <utility>

namespace pma::graph {
namespace {

using storage::Database;
using storage::Error;
using storage::ErrorCode;
using storage::Migration;
using storage::Statement;
using storage::Transaction;

const std::vector<Migration> migrations{
    {100,
     "evidence_and_graph",
     R"sql(
CREATE TABLE episodes(
  id TEXT PRIMARY KEY, source_adapter TEXT NOT NULL, external_key TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  UNIQUE(source_adapter, external_key)
) STRICT;
CREATE TABLE graph_nodes(
  id TEXT PRIMARY KEY,
  node_type TEXT NOT NULL CHECK(node_type IN ('evidence_segment','entity','claim','procedure','procedure_step','goal','resource','context','memory_operation','retrieval_event')),
  status TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','superseded','suppressed','completed','abandoned')),
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
) STRICT;
CREATE TABLE evidence_segments(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), episode_id TEXT NOT NULL REFERENCES episodes(id),
  ordinal INTEGER NOT NULL CHECK(ordinal>=0), text TEXT NOT NULL, idempotency_key TEXT NOT NULL,
  observed_agent TEXT, model_runtime TEXT,
  UNIQUE(episode_id, ordinal), UNIQUE(episode_id, idempotency_key)
) STRICT;
CREATE TABLE entities(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), canonical_name TEXT NOT NULL, category TEXT NOT NULL
) STRICT;
CREATE TABLE entity_aliases(
  id TEXT PRIMARY KEY, entity_id TEXT NOT NULL REFERENCES entities(id), alias TEXT NOT NULL,
  UNIQUE(entity_id, alias)
) STRICT;
CREATE TABLE contexts(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), context_type TEXT NOT NULL, external_key TEXT NOT NULL,
  UNIQUE(context_type, external_key)
) STRICT;
CREATE TABLE resources(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), logical_name TEXT NOT NULL,
  repository_identity TEXT, repository_relative_path TEXT
) STRICT;
CREATE TABLE resource_locations(
  id TEXT PRIMARY KEY, resource_id TEXT NOT NULL REFERENCES resources(id),
  location_kind TEXT NOT NULL CHECK(location_kind IN ('absolute_path','repository_relative','uri','artifact','unresolved')),
  environment_id TEXT REFERENCES contexts(id), locator TEXT NOT NULL,
  observed_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  CHECK(location_kind!='absolute_path' OR environment_id IS NOT NULL)
) STRICT;
CREATE TABLE predicates(
  id TEXT PRIMARY KEY, name TEXT NOT NULL UNIQUE, description TEXT NOT NULL,
  subject_category TEXT NOT NULL, object_category TEXT NOT NULL,
  inverse_id TEXT REFERENCES predicates(id), symmetric INTEGER NOT NULL DEFAULT 0 CHECK(symmetric IN (0,1)),
  schema_version INTEGER NOT NULL, status TEXT NOT NULL CHECK(status IN ('active','review','retired'))
) STRICT;
CREATE TABLE typed_values(
  id TEXT PRIMARY KEY, value_type TEXT NOT NULL CHECK(value_type IN ('text','integer','real','boolean','timestamp','uri')),
  text_value TEXT, integer_value INTEGER, real_value REAL,
  CHECK((text_value IS NOT NULL) + (integer_value IS NOT NULL) + (real_value IS NOT NULL) = 1)
) STRICT;
CREATE TABLE claims(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), subject_id TEXT NOT NULL REFERENCES entities(id),
  predicate_id TEXT NOT NULL REFERENCES predicates(id), object_entity_id TEXT REFERENCES entities(id),
  typed_value_id TEXT REFERENCES typed_values(id),
  polarity TEXT NOT NULL CHECK(polarity IN ('positive','negative')),
  modality TEXT NOT NULL CHECK(modality IN ('asserted','possible','required','preferred')),
  authority TEXT NOT NULL, confidence REAL NOT NULL CHECK(confidence>=0.0 AND confidence<=1.0),
  strength REAL NOT NULL DEFAULT 1.0 CHECK(strength>=0.0),
  status TEXT NOT NULL CHECK(status IN ('active','superseded','contradicted','suppressed')),
  source_kind TEXT NOT NULL CHECK(source_kind IN ('evidence','imported','inferred')),
  valid_from TEXT, valid_to TEXT,
  CHECK((object_entity_id IS NOT NULL) + (typed_value_id IS NOT NULL) = 1)
) STRICT;
CREATE TABLE claim_evidence(
  claim_id TEXT NOT NULL REFERENCES claims(id), evidence_id TEXT NOT NULL REFERENCES evidence_segments(id),
  relation TEXT NOT NULL CHECK(relation IN ('supported_by','contradicted_by','derived_from')),
  PRIMARY KEY(claim_id,evidence_id,relation)
) STRICT;
CREATE TABLE procedures(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), name TEXT NOT NULL
) STRICT;
CREATE TABLE procedure_steps(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), procedure_id TEXT NOT NULL REFERENCES procedures(id),
  ordinal INTEGER NOT NULL CHECK(ordinal>=0), instruction TEXT NOT NULL,
  UNIQUE(procedure_id,ordinal)
) STRICT;
CREATE TABLE goals(
  id TEXT PRIMARY KEY REFERENCES graph_nodes(id), description TEXT NOT NULL,
  goal_status TEXT NOT NULL CHECK(goal_status IN ('open','active','blocked','completed','abandoned')),
  completion_evidence_id TEXT REFERENCES evidence_segments(id)
) STRICT;
CREATE TABLE graph_edges(
  id TEXT PRIMARY KEY, source_id TEXT NOT NULL REFERENCES graph_nodes(id),
  target_id TEXT NOT NULL REFERENCES graph_nodes(id),
  edge_class TEXT NOT NULL CHECK(edge_class IN ('provenance','structural','semantic','associative','contextual')),
  relation TEXT NOT NULL, weight REAL,
  CHECK(source_id<>target_id), CHECK(edge_class='associative' OR weight IS NULL)
) STRICT;
CREATE INDEX evidence_episode_order ON evidence_segments(episode_id,ordinal);
CREATE INDEX entities_name ON entities(canonical_name);
CREATE INDEX aliases_name ON entity_aliases(alias);
CREATE INDEX graph_edges_source ON graph_edges(source_id,edge_class);
)sql"},
    {101,
     "initial_predicates",
     R"sql(
INSERT INTO predicates(id,name,description,subject_category,object_category,schema_version,status) VALUES
 ('pred:is_a','is_a','Classifies an entity','entity','entity',1,'active'),
 ('pred:part_of','part_of','Relates a component to a whole','entity','entity',1,'active'),
 ('pred:uses','uses','Records use of an entity or resource','entity','any',1,'active'),
 ('pred:located_at','located_at','Relates an entity to a resource or context','entity','any',1,'active'),
 ('pred:has_property','has_property','Assigns a typed property','entity','typed_value',1,'active');
)sql"}};

void bind_optional(Statement &statement, std::size_t index,
                   std::optional<std::string_view> value) {
  if (value.has_value()) {
    statement.bind(index, *value);
  } else {
    statement.bind_null(index);
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

void insert_node(Database &database, std::string_view id, std::string_view type) {
  auto statement = database.prepare("INSERT INTO graph_nodes(id,node_type) VALUES(?1,?2)");
  statement.bind(1, id);
  statement.bind(2, type);
  statement.execute();
}

EvidenceSegment evidence_from(Statement &query) {
  return {query.column_text(0),
          query.column_text(1),
          query.column_int64(2),
          query.column_text(3),
          query.column_is_null(4) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(4)},
          query.column_is_null(5) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(5)}};
}

Entity entity_from(Statement &query) {
  return {query.column_text(0), query.column_text(1), query.column_text(2)};
}

Claim claim_from(Statement &query) {
  return {query.column_text(0),
          query.column_text(1),
          query.column_text(2),
          query.column_is_null(3) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(3)},
          query.column_is_null(4) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(4)},
          query.column_text(5),
          query.column_text(6),
          query.column_text(7),
          query.column_double(8),
          query.column_text(9),
          query.column_text(10)};
}

[[noreturn]] void not_found(std::string_view type, std::string_view id) {
  throw Error(ErrorCode::constraint, 0, std::string(type) + " not found: " + std::string(id));
}

} // namespace

Store::Store(Database &database) : database_(&database) {}

void Store::initialize(std::string_view build_version) {
  storage::migrate(*database_, migrations, build_version);
}

void Store::create_episode(std::string_view id, std::string_view source_adapter,
                           std::string_view external_key) {
  auto insert = database_->prepare(
      "INSERT INTO episodes(id,source_adapter,external_key) VALUES(?1,?2,?3)");
  insert.bind(1, id);
  insert.bind(2, source_adapter);
  insert.bind(3, external_key);
  insert.execute();
}

Episode Store::get_episode(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,source_adapter,external_key FROM episodes WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("episode", id);
  }
  return {query.column_text(0), query.column_text(1), query.column_text(2)};
}

EvidenceSegment Store::append_evidence(
    std::string_view id, std::string_view episode_id, std::int64_t ordinal,
    std::string_view text, std::string_view idempotency_key,
    std::optional<std::string_view> observed_agent,
    std::optional<std::string_view> model_runtime) {
  auto existing = database_->prepare(
      "SELECT id,episode_id,ordinal,text,observed_agent,model_runtime FROM evidence_segments "
      "WHERE episode_id=?1 AND idempotency_key=?2");
  existing.bind(1, episode_id);
  existing.bind(2, idempotency_key);
  if (existing.step()) {
    auto result = evidence_from(existing);
    if (result.id != id || result.ordinal != ordinal || result.text != text) {
      throw Error(ErrorCode::constraint, 0, "idempotency key reused with different evidence");
    }
    return result;
  }
  Transaction transaction(*database_);
  insert_node(*database_, id, "evidence_segment");
  auto insert = database_->prepare(
      "INSERT INTO evidence_segments(id,episode_id,ordinal,text,idempotency_key,observed_agent,"
      "model_runtime) VALUES(?1,?2,?3,?4,?5,?6,?7)");
  insert.bind(1, id);
  insert.bind(2, episode_id);
  insert.bind(3, ordinal);
  insert.bind(4, text);
  insert.bind(5, idempotency_key);
  bind_optional(insert, 6, observed_agent);
  bind_optional(insert, 7, model_runtime);
  insert.execute();
  transaction.commit();
  return get_evidence(id);
}

EvidenceSegment Store::get_evidence(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,episode_id,ordinal,text,observed_agent,model_runtime FROM evidence_segments "
      "WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("evidence", id);
  }
  return evidence_from(query);
}

std::vector<EvidenceSegment> Store::list_evidence(std::string_view episode_id) {
  auto query = database_->prepare(
      "SELECT id,episode_id,ordinal,text,observed_agent,model_runtime FROM evidence_segments "
      "WHERE episode_id=?1 ORDER BY ordinal");
  query.bind(1, episode_id);
  std::vector<EvidenceSegment> results;
  while (query.step()) {
    results.push_back(evidence_from(query));
  }
  return results;
}

std::vector<EvidenceSegment> Store::search_evidence(std::string_view text) {
  auto query = database_->prepare(
      "SELECT id,episode_id,ordinal,text,observed_agent,model_runtime FROM evidence_segments "
      "WHERE text LIKE ?1 ORDER BY episode_id,ordinal");
  query.bind(1, "%" + std::string(text) + "%");
  std::vector<EvidenceSegment> results;
  while (query.step()) {
    results.push_back(evidence_from(query));
  }
  return results;
}

void Store::create_entity(std::string_view id, std::string_view canonical_name,
                          std::string_view category) {
  Transaction transaction(*database_);
  insert_node(*database_, id, "entity");
  auto insert = database_->prepare(
      "INSERT INTO entities(id,canonical_name,category) VALUES(?1,?2,?3)");
  insert.bind(1, id);
  insert.bind(2, canonical_name);
  insert.bind(3, category);
  insert.execute();
  transaction.commit();
}

Entity Store::get_entity(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,canonical_name,category FROM entities WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("entity", id);
  }
  return entity_from(query);
}

std::vector<Entity> Store::search_entities(std::string_view name) {
  auto query = database_->prepare(
      "SELECT DISTINCT e.id,e.canonical_name,e.category FROM entities e LEFT JOIN entity_aliases a "
      "ON a.entity_id=e.id WHERE e.canonical_name LIKE ?1 OR a.alias LIKE ?1 ORDER BY e.id");
  query.bind(1, "%" + std::string(name) + "%");
  std::vector<Entity> results;
  while (query.step()) {
    results.push_back(entity_from(query));
  }
  return results;
}

void Store::add_alias(std::string_view id, std::string_view entity_id, std::string_view alias) {
  auto insert = database_->prepare(
      "INSERT INTO entity_aliases(id,entity_id,alias) VALUES(?1,?2,?3)");
  insert.bind(1, id);
  insert.bind(2, entity_id);
  insert.bind(3, alias);
  insert.execute();
}

void Store::create_context(std::string_view id, std::string_view type,
                           std::string_view external_key) {
  Transaction transaction(*database_);
  insert_node(*database_, id, "context");
  auto insert = database_->prepare(
      "INSERT INTO contexts(id,context_type,external_key) VALUES(?1,?2,?3)");
  insert.bind(1, id);
  insert.bind(2, type);
  insert.bind(3, external_key);
  insert.execute();
  transaction.commit();
}

Context Store::get_context(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,context_type,external_key FROM contexts WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("context", id);
  }
  return {query.column_text(0), query.column_text(1), query.column_text(2)};
}

void Store::create_resource(std::string_view id, std::string_view logical_name,
                            std::optional<std::string_view> repository_identity,
                            std::optional<std::string_view> repository_relative_path) {
  Transaction transaction(*database_);
  insert_node(*database_, id, "resource");
  auto insert = database_->prepare(
      "INSERT INTO resources(id,logical_name,repository_identity,repository_relative_path) "
      "VALUES(?1,?2,?3,?4)");
  insert.bind(1, id);
  insert.bind(2, logical_name);
  bind_optional(insert, 3, repository_identity);
  bind_optional(insert, 4, repository_relative_path);
  insert.execute();
  transaction.commit();
}

Resource Store::get_resource(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,logical_name,repository_identity,repository_relative_path FROM resources WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("resource", id);
  }
  return {query.column_text(0), query.column_text(1),
          query.column_is_null(2) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(2)},
          query.column_is_null(3) ? std::nullopt
                                  : std::optional<std::string>{query.column_text(3)}};
}

void Store::add_resource_location(std::string_view id, std::string_view resource_id,
                                  std::string_view kind, std::string_view locator,
                                  std::optional<std::string_view> environment_id) {
  auto insert = database_->prepare(
      "INSERT INTO resource_locations(id,resource_id,location_kind,environment_id,locator) "
      "VALUES(?1,?2,?3,?4,?5)");
  insert.bind(1, id);
  insert.bind(2, resource_id);
  insert.bind(3, kind);
  bind_optional(insert, 4, environment_id);
  insert.bind(5, locator);
  insert.execute();
}

std::vector<ResourceLocation> Store::resource_locations(std::string_view resource_id) {
  auto query = database_->prepare(
      "SELECT id,resource_id,location_kind,environment_id,locator FROM resource_locations "
      "WHERE resource_id=?1 ORDER BY id");
  query.bind(1, resource_id);
  std::vector<ResourceLocation> results;
  while (query.step()) {
    results.push_back(
        {query.column_text(0), query.column_text(1), query.column_text(2),
         query.column_is_null(3) ? std::nullopt
                                 : std::optional<std::string>{query.column_text(3)},
         query.column_text(4)});
  }
  return results;
}

Predicate Store::get_predicate(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,name,description,subject_category,object_category,status FROM predicates WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("predicate", id);
  }
  return {query.column_text(0), query.column_text(1), query.column_text(2),
          query.column_text(3), query.column_text(4), query.column_text(5)};
}

void Store::create_typed_text(std::string_view id, std::string_view value) {
  auto insert = database_->prepare(
      "INSERT INTO typed_values(id,value_type,text_value) VALUES(?1,'text',?2)");
  insert.bind(1, id);
  insert.bind(2, value);
  insert.execute();
}

void Store::create_claim(const Claim &claim) {
  Transaction transaction(*database_);
  insert_node(*database_, claim.id, "claim");
  auto insert = database_->prepare(
      "INSERT INTO claims(id,subject_id,predicate_id,object_entity_id,typed_value_id,polarity,"
      "modality,authority,confidence,status,source_kind) VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11)");
  insert.bind(1, claim.id);
  insert.bind(2, claim.subject_id);
  insert.bind(3, claim.predicate_id);
  bind_optional(insert, 4, claim.object_entity_id);
  bind_optional(insert, 5, claim.typed_value_id);
  insert.bind(6, claim.polarity);
  insert.bind(7, claim.modality);
  insert.bind(8, claim.authority);
  insert.bind(9, claim.confidence);
  insert.bind(10, claim.status);
  insert.bind(11, claim.source_kind);
  insert.execute();
  transaction.commit();
}

Claim Store::get_claim(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,subject_id,predicate_id,object_entity_id,typed_value_id,polarity,modality,"
      "authority,confidence,status,source_kind FROM claims WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("claim", id);
  }
  return claim_from(query);
}

void Store::link_claim_evidence(std::string_view claim_id, std::string_view evidence_id,
                                std::string_view relation) {
  auto insert = database_->prepare(
      "INSERT INTO claim_evidence(claim_id,evidence_id,relation) VALUES(?1,?2,?3)");
  insert.bind(1, claim_id);
  insert.bind(2, evidence_id);
  insert.bind(3, relation);
  insert.execute();
}

void Store::add_edge(std::string_view id, std::string_view source_id,
                     std::string_view target_id, std::string_view edge_class,
                     std::string_view relation) {
  auto insert = database_->prepare(
      "INSERT INTO graph_edges(id,source_id,target_id,edge_class,relation) VALUES(?1,?2,?3,?4,?5)");
  insert.bind(1, id);
  insert.bind(2, source_id);
  insert.bind(3, target_id);
  insert.bind(4, edge_class);
  insert.bind(5, relation);
  insert.execute();
}

Edge Store::get_edge(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,source_id,target_id,edge_class,relation FROM graph_edges WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("edge", id);
  }
  return {query.column_text(0), query.column_text(1), query.column_text(2),
          query.column_text(3), query.column_text(4)};
}

void Store::create_procedure(std::string_view id, std::string_view name) {
  Transaction transaction(*database_);
  insert_node(*database_, id, "procedure");
  auto insert = database_->prepare("INSERT INTO procedures(id,name) VALUES(?1,?2)");
  insert.bind(1, id);
  insert.bind(2, name);
  insert.execute();
  transaction.commit();
}

Procedure Store::get_procedure(std::string_view id) {
  auto query = database_->prepare("SELECT id,name FROM procedures WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("procedure", id);
  }
  return {query.column_text(0), query.column_text(1)};
}

void Store::add_procedure_step(std::string_view id, std::string_view procedure_id,
                               std::int64_t ordinal, std::string_view instruction) {
  Transaction transaction(*database_);
  insert_node(*database_, id, "procedure_step");
  auto insert = database_->prepare(
      "INSERT INTO procedure_steps(id,procedure_id,ordinal,instruction) VALUES(?1,?2,?3,?4)");
  insert.bind(1, id);
  insert.bind(2, procedure_id);
  insert.bind(3, ordinal);
  insert.bind(4, instruction);
  insert.execute();
  transaction.commit();
}

std::vector<ProcedureStep> Store::procedure_steps(std::string_view procedure_id) {
  auto query = database_->prepare(
      "SELECT id,procedure_id,ordinal,instruction FROM procedure_steps WHERE procedure_id=?1 "
      "ORDER BY ordinal");
  query.bind(1, procedure_id);
  std::vector<ProcedureStep> results;
  while (query.step()) {
    results.push_back({query.column_text(0), query.column_text(1), query.column_int64(2),
                       query.column_text(3)});
  }
  return results;
}

void Store::create_goal(std::string_view id, std::string_view description,
                        std::string_view status) {
  Transaction transaction(*database_);
  insert_node(*database_, id, "goal");
  auto insert = database_->prepare(
      "INSERT INTO goals(id,description,goal_status) VALUES(?1,?2,?3)");
  insert.bind(1, id);
  insert.bind(2, description);
  insert.bind(3, status);
  insert.execute();
  transaction.commit();
}

Goal Store::get_goal(std::string_view id) {
  auto query = database_->prepare(
      "SELECT id,description,goal_status FROM goals WHERE id=?1");
  query.bind(1, id);
  if (!query.step()) {
    not_found("goal", id);
  }
  return {query.column_text(0), query.column_text(1), query.column_text(2)};
}

std::vector<GraphIssue> Store::verify() const {
  std::vector<GraphIssue> issues;
  auto foreign_keys = database_->prepare("PRAGMA foreign_key_check");
  while (foreign_keys.step()) {
    issues.push_back({"orphan", foreign_keys.column_text(1),
                      "foreign key violation in " + foreign_keys.column_text(0)});
  }

  auto missing_evidence = database_->prepare(
      "SELECT c.id FROM claims c WHERE c.source_kind='evidence' AND NOT EXISTS("
      "SELECT 1 FROM claim_evidence ce WHERE ce.claim_id=c.id AND ce.relation='supported_by')");
  while (missing_evidence.step()) {
    issues.push_back({"claim_without_evidence", missing_evidence.column_text(0),
                      "evidence-sourced claim has no supporting evidence"});
  }

  auto node_mismatch = database_->prepare(
      "SELECT n.id FROM graph_nodes n WHERE "
      "(n.node_type='entity' AND NOT EXISTS(SELECT 1 FROM entities x WHERE x.id=n.id)) OR "
      "(n.node_type='claim' AND NOT EXISTS(SELECT 1 FROM claims x WHERE x.id=n.id)) OR "
      "(n.node_type='evidence_segment' AND NOT EXISTS(SELECT 1 FROM evidence_segments x WHERE x.id=n.id)) OR "
      "(n.node_type='procedure' AND NOT EXISTS(SELECT 1 FROM procedures x WHERE x.id=n.id)) OR "
      "(n.node_type='procedure_step' AND NOT EXISTS(SELECT 1 FROM procedure_steps x WHERE x.id=n.id)) OR "
      "(n.node_type='goal' AND NOT EXISTS(SELECT 1 FROM goals x WHERE x.id=n.id)) OR "
      "(n.node_type='resource' AND NOT EXISTS(SELECT 1 FROM resources x WHERE x.id=n.id)) OR "
      "(n.node_type='context' AND NOT EXISTS(SELECT 1 FROM contexts x WHERE x.id=n.id))");
  while (node_mismatch.step()) {
    issues.push_back({"node_shape", node_mismatch.column_text(0),
                      "graph node has no matching typed record"});
  }

  auto step_gaps = database_->prepare(
      "SELECT procedure_id FROM procedure_steps GROUP BY procedure_id "
      "HAVING min(ordinal)<>0 OR max(ordinal)<>count(*)-1");
  while (step_gaps.step()) {
    issues.push_back({"procedure_order", step_gaps.column_text(0),
                      "procedure step ordinals are not contiguous from zero"});
  }
  return issues;
}

} // namespace pma::graph
