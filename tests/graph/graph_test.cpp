#include "pma-internal/graph.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <string>

namespace {

class GraphFixture {
public:
  GraphFixture() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    directory_ = std::filesystem::temp_directory_path() /
                 ("pma-graph-test-" + std::to_string(nonce));
    std::filesystem::create_directories(directory_);
    database_ = std::make_unique<pma::storage::Database>(directory_ / "pma.sqlite");
    store_ = std::make_unique<pma::graph::Store>(*database_);
    store_->initialize("test");
  }
  ~GraphFixture() {
    store_.reset();
    database_.reset();
    std::filesystem::remove_all(directory_);
  }

  pma::storage::Database &database() { return *database_; }
  pma::graph::Store &store() { return *store_; }

private:
  std::filesystem::path directory_;
  std::unique_ptr<pma::storage::Database> database_;
  std::unique_ptr<pma::graph::Store> store_;
};

pma::graph::Claim literal_claim(std::string id, std::string source_kind = "imported") {
  return {std::move(id), "entity:subject", "pred:has_property", std::nullopt,
          "value:text", "positive", "asserted", "explicit", 0.9, "active",
          std::move(source_kind)};
}

} // namespace

TEST_CASE("evidence is ordered and idempotent", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_episode("episode:1", "pi", "session:1");
  REQUIRE(store.get_episode("episode:1").source_adapter == "pi");
  store.append_evidence("evidence:2", "episode:1", 1, "second", "event:2");
  const auto first =
      store.append_evidence("evidence:1", "episode:1", 0, "first 🐉", "event:1", "agent:x");
  const auto repeated =
      store.append_evidence("evidence:1", "episode:1", 0, "first 🐉", "event:1", "agent:x");
  REQUIRE(repeated.id == first.id);
  const auto ordered = store.list_evidence("episode:1");
  REQUIRE(ordered.size() == 2);
  REQUIRE(ordered[0].text == "first 🐉");
  REQUIRE(ordered[1].text == "second");
  REQUIRE(store.search_evidence("🐉").size() == 1);
  REQUIRE_THROWS(store.append_evidence("evidence:other", "episode:1", 0, "changed", "event:1"));
}

TEST_CASE("aliases remain ambiguous and do not merge entities", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_entity("entity:one", "Mercury planet", "concept");
  store.create_entity("entity:two", "Mercury element", "concept");
  store.add_alias("alias:one", "entity:one", "Mercury");
  store.add_alias("alias:two", "entity:two", "Mercury");
  const auto results = store.search_entities("Mercury");
  REQUIRE(results.size() == 2);
  REQUIRE(store.get_entity("entity:one").id != store.get_entity("entity:two").id);
}

TEST_CASE("claims enforce exactly one object representation", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_entity("entity:subject", "PMA-Core", "project");
  store.create_entity("entity:object", "SQLite", "technology");
  store.create_typed_text("value:text", "portable");

  auto valid = literal_claim("claim:valid");
  store.create_claim(valid);
  REQUIRE(store.get_claim("claim:valid").typed_value_id == "value:text");

  auto invalid = literal_claim("claim:invalid");
  invalid.object_entity_id = "entity:object";
  REQUIRE_THROWS(store.create_claim(invalid));
}

TEST_CASE("evidence claims and contradiction history are retained", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_episode("episode:1", "pi", "session:1");
  store.append_evidence("evidence:1", "episode:1", 0, "original", "event:1");
  store.append_evidence("evidence:2", "episode:1", 1, "correction", "event:2");
  store.create_entity("entity:subject", "System", "concept");
  store.create_typed_text("value:text", "state");
  store.create_claim(literal_claim("claim:old", "evidence"));
  store.create_claim(literal_claim("claim:new", "evidence"));
  store.link_claim_evidence("claim:old", "evidence:1", "supported_by");
  store.link_claim_evidence("claim:new", "evidence:2", "supported_by");
  store.link_claim_evidence("claim:old", "evidence:2", "contradicted_by");
  store.add_edge("edge:supersedes", "claim:new", "claim:old", "structural", "supersedes");
  REQUIRE(store.get_edge("edge:supersedes").relation == "supersedes");
  REQUIRE(store.verify().empty());

  auto query = fixture.database().prepare(
      "SELECT count(*) FROM graph_edges WHERE relation='supersedes'");
  REQUIRE(query.step());
  REQUIRE(query.column_int64(0) == 1);
}

TEST_CASE("procedure steps have stable order", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_procedure("procedure:1", "Build");
  REQUIRE(store.get_procedure("procedure:1").name == "Build");
  store.create_goal("goal:1", "Ship", "active");
  REQUIRE(store.get_goal("goal:1").status == "active");
  store.add_procedure_step("step:2", "procedure:1", 1, "Test");
  store.add_procedure_step("step:1", "procedure:1", 0, "Compile");
  const auto steps = store.procedure_steps("procedure:1");
  REQUIRE(steps.size() == 2);
  REQUIRE(steps[0].instruction == "Compile");
  REQUIRE(steps[1].instruction == "Test");
  REQUIRE(store.verify().empty());
}

TEST_CASE("resource paths preserve environment scope and portable identity", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_context("environment:windows", "environment", "host:workstation-a");
  REQUIRE(store.get_context("environment:windows").type == "environment");
  store.create_resource("resource:source", "storage.cpp", "git:pma-core", "src/storage/storage.cpp");
  REQUIRE(store.get_resource("resource:source").repository_relative_path ==
          "src/storage/storage.cpp");
  REQUIRE(store.get_predicate("pred:uses").status == "active");
  store.add_resource_location("location:absolute", "resource:source", "absolute_path",
                              R"(C:\Users\Curtis\src\PMA-Core\src\storage\storage.cpp)",
                              "environment:windows");
  store.add_resource_location("location:relative", "resource:source", "repository_relative",
                              "src/storage/storage.cpp");
  const auto locations = store.resource_locations("resource:source");
  REQUIRE(locations.size() == 2);
  REQUIRE(locations[0].environment_id == "environment:windows");
  REQUIRE_FALSE(locations[1].environment_id.has_value());
  REQUIRE_THROWS(store.add_resource_location("location:invalid", "resource:source",
                                             "absolute_path", "/tmp/source"));
}

TEST_CASE("verification detects impossible and orphaned graph state", "[graph]") {
  GraphFixture fixture;
  auto &store = fixture.store();
  store.create_entity("entity:subject", "Subject", "concept");
  store.create_typed_text("value:text", "unsupported");
  store.create_claim(literal_claim("claim:unsupported", "evidence"));
  fixture.database().execute(
      "INSERT INTO graph_nodes(id,node_type) VALUES('entity:missing-shape','entity')");
  fixture.database().execute("PRAGMA foreign_keys=OFF");
  fixture.database().execute(
      "INSERT INTO graph_edges(id,source_id,target_id,edge_class,relation) "
      "VALUES('edge:orphan','missing:source','entity:subject','semantic','related')");
  fixture.database().execute("PRAGMA foreign_keys=ON");

  const auto issues = store.verify();
  REQUIRE(issues.size() >= 3);
  bool saw_orphan = false;
  bool saw_shape = false;
  bool saw_provenance = false;
  for (const auto &issue : issues) {
    saw_orphan = saw_orphan || issue.code == "orphan";
    saw_shape = saw_shape || issue.code == "node_shape";
    saw_provenance = saw_provenance || issue.code == "claim_without_evidence";
  }
  REQUIRE(saw_orphan);
  REQUIRE(saw_shape);
  REQUIRE(saw_provenance);
}
