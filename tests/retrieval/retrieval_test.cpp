#include "pma-internal/graph.hpp"
#include "pma-internal/lifecycle.hpp"
#include "pma-internal/retrieval.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>

namespace {

class Fixture {
public:
  Fixture() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    root_ = std::filesystem::temp_directory_path() /
            ("pma-retrieval-test-" + std::to_string(nonce));
    std::filesystem::create_directories(root_);
    database_ = std::make_unique<pma::storage::Database>(root_ / "pma.sqlite");
    graph_ = std::make_unique<pma::graph::Store>(*database_);
    lifecycle_ = std::make_unique<pma::lifecycle::Processor>(*database_, *graph_);
    lifecycle_->initialize("test");
    vectors_ = std::make_unique<pma::vectors::Manager>(*database_, root_);
    vectors_->initialize("test");
    graph_->create_entity("entity:pma", "PMA-Core", "project");
    graph_->create_entity("entity:other", "Other System", "project");
    graph_->create_context("context:pma", "project", "pma-core");
    retrieval_ = std::make_unique<pma::retrieval::Engine>(*database_, vectors_.get());
    retrieval_->initialize("test");
  }
  ~Fixture() {
    retrieval_.reset();
    vectors_.reset();
    lifecycle_.reset();
    graph_.reset();
    database_.reset();
    std::filesystem::remove_all(root_);
  }

  void claim(std::string id, std::string subject, std::string text,
             std::string authority = "observed", std::string status = "active") {
    const std::string value_id = "value:" + id;
    graph_->create_typed_text(value_id, text);
    graph_->create_claim({std::move(id), std::move(subject), "pred:has_property", std::nullopt,
                          value_id, "positive", "asserted", std::move(authority), 0.9,
                          std::move(status), "imported"});
  }

  void refresh() { retrieval_->rebuild_fts(); }
  pma::retrieval::RecallRequest request(std::string id, std::string query) {
    return {std::move(id), "interaction:1", std::move(query)};
  }
  pma::storage::Database &database() { return *database_; }
  pma::graph::Store &graph() { return *graph_; }
  pma::vectors::Manager &vectors() { return *vectors_; }
  pma::retrieval::Engine &retrieval() { return *retrieval_; }

private:
  std::filesystem::path root_;
  std::unique_ptr<pma::storage::Database> database_;
  std::unique_ptr<pma::graph::Store> graph_;
  std::unique_ptr<pma::lifecycle::Processor> lifecycle_;
  std::unique_ptr<pma::vectors::Manager> vectors_;
  std::unique_ptr<pma::retrieval::Engine> retrieval_;
};

const pma::retrieval::PacketItem &item(const pma::retrieval::ContextPacket &packet,
                                       std::string_view id) {
  const auto found = std::find_if(packet.candidates.begin(), packet.candidates.end(),
                                  [&](const auto &value) { return value.claim_id == id; });
  REQUIRE(found != packet.candidates.end());
  return *found;
}

} // namespace

TEST_CASE("FTS retrieval is explainable without a provider", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:sqlite", "entity:pma", "SQLite authoritative storage", "explicit_decision");
  fixture.refresh();
  const auto packet = fixture.retrieval().recall(fixture.request("packet:fts", "SQLite storage"));
  REQUIRE_FALSE(packet.selected.empty());
  REQUIRE(item(packet, "claim:sqlite").scores.lexical > 0.0);
  REQUIRE(item(packet, "claim:sqlite").scores.final_score > 0.0);
  REQUIRE(std::find(item(packet, "claim:sqlite").reasons.begin(),
                    item(packet, "claim:sqlite").reasons.end(), "fts") !=
          item(packet, "claim:sqlite").reasons.end());
  auto persisted = fixture.database().prepare(
      "SELECT lexical,final_score,reasons FROM retrieval_candidates WHERE packet_id='packet:fts'");
  REQUIRE(persisted.step());
  REQUIRE(persisted.column_double(0) > 0.0);
}

TEST_CASE("entity seeding and bounded graph traversal add independent channels", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:seed", "entity:pma", "build architecture");
  fixture.claim("claim:linked", "entity:other", "unrelated wording");
  fixture.graph().add_edge("edge:related", "claim:seed", "claim:linked", "semantic", "related_to");
  fixture.refresh();
  const auto packet = fixture.retrieval().recall(fixture.request("packet:graph", "PMA-Core"));
  REQUIRE(item(packet, "claim:seed").scores.entity_predicate > 0.0);
  REQUIRE(item(packet, "claim:linked").scores.graph > 0.0);
}

TEST_CASE("active project context boosts but does not own or filter claims", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:context", "entity:other", "environment constraint");
  fixture.graph().add_edge("edge:context", "claim:context", "context:pma", "contextual",
                           "relevant_to");
  fixture.refresh();
  auto request = fixture.request("packet:context", "no lexical match");
  request.active_context_ids = {"context:pma"};
  const auto packet = fixture.retrieval().recall(request);
  REQUIRE(item(packet, "claim:context").scores.context == 1.0);
  REQUIRE(item(packet, "claim:context").text.find("Other System") != std::string::npos);
}

TEST_CASE("current explicit decision outranks superseded semantic match", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:current", "entity:pma", "storage policy", "explicit_decision");
  fixture.claim("claim:old", "entity:pma", "storage policy", "inferred", "superseded");
  fixture.refresh();
  const auto packet = fixture.retrieval().recall(fixture.request("packet:temporal", "storage policy"));
  REQUIRE(packet.selected.front().claim_id == "claim:current");
  REQUIRE(item(packet, "claim:current").scores.authority > item(packet, "claim:old").scores.authority);
  REQUIRE(item(packet, "claim:current").scores.temporal > item(packet, "claim:old").scores.temporal);
}

TEST_CASE("vector candidates participate in deterministic fusion", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:vector", "entity:pma", "semantic vector target");
  fixture.database().execute(
      "INSERT INTO jobs(id,job_type,state) VALUES('job:render','render','succeeded');"
      "INSERT INTO memory_operations(id,job_id,operation_type,result_id,status) "
      "VALUES('operation:render','job:render','create','claim:vector','applied')");
  fixture.vectors().render_claim("claim:vector", "operation:render");
  pma::vectors::Profile profile{"profile:retrieval", "fake", "test", "model", "r1",
                                "tokenizer", 8};
  pma::vectors::RuntimeManifest runtime{"runtime:retrieval", "fake", "1", "24", "test",
                                        "x64", "lock", "cpu", "deterministic"};
  fixture.vectors().register_profile(profile, runtime);
  pma::vectors::FakeEmbeddingProvider provider;
  fixture.vectors().build("store:retrieval", profile.id, 1, runtime.id, provider);
  fixture.refresh();
  auto request = fixture.request("packet:vector", "words absent from index");
  request.vector_store_id = "store:retrieval";
  request.query_vector = provider.embed("PMA-Core | has_property | semantic vector target", profile);
  const auto packet = fixture.retrieval().recall(request);
  REQUIRE(item(packet, "claim:vector").scores.vector > 0.99);
  REQUIRE(packet.vector_profile_id == profile.id);
}

TEST_CASE("slice preserves conflict and qualification while removing duplicates", "[retrieval]") {
  pma::retrieval::ContextPacket packet;
  pma::retrieval::PacketItem conflict{"claim:conflict", "Storage is remote", "contradicted",
                                       "observed", {}, {}, true, false, 4};
  pma::retrieval::PacketItem qualification{"claim:qualification", "Paths require matching environment",
                                            "active", "observed", {}, {}, false, true, 6};
  auto duplicate = qualification;
  duplicate.claim_id = "claim:duplicate";
  packet.selected = {conflict, qualification, duplicate};
  pma::storage::Database database(":memory:");
  pma::retrieval::Engine engine(database);
  const auto slice = engine.render_slice(packet, 40);
  REQUIRE(slice.estimated_tokens <= 40);
  REQUIRE(slice.text.find("[conflict]") != std::string::npos);
  REQUIRE(slice.text.find("[qualification]") != std::string::npos);
  REQUIRE(slice.included_claim_ids.size() == 2);
  REQUIRE(slice.text.find("claim:") == std::string::npos);
  REQUIRE(slice.text.find("score") == std::string::npos);
}

TEST_CASE("explicit useful outcomes create cautious associations", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:left", "entity:pma", "shared retrieval term left");
  fixture.claim("claim:right", "entity:pma", "shared retrieval term right");
  fixture.refresh();
  const auto packet = fixture.retrieval().recall(
      fixture.request("packet:association", "shared retrieval term"));
  REQUIRE(packet.selected.size() == 2);
  REQUIRE(fixture.retrieval().utility("claim:left") == 0.0);
  fixture.retrieval().record_outcome("packet:association", "claim:left",
                                     pma::retrieval::Outcome::used);
  REQUIRE(fixture.retrieval().utility("claim:left") > 0.0);
  auto association = fixture.database().prepare(
      "SELECT weight,evidence_count FROM associations WHERE left_claim_id='claim:left' "
      "AND right_claim_id='claim:right'");
  REQUIRE(association.step());
  REQUIRE(association.column_double(0) == 0.1);
  REQUIRE(association.column_int64(1) == 1);
}

TEST_CASE("outcomes change utility but retrieval alone does not reinforce", "[retrieval]") {
  Fixture fixture;
  fixture.claim("claim:utility", "entity:pma", "high strength irrelevant detail");
  fixture.refresh();
  for (int index = 0; index < 4; ++index) {
    const std::string packet_id = "packet:utility:" + std::to_string(index);
    const auto packet = fixture.retrieval().recall(
        fixture.request(packet_id, "high strength irrelevant detail"));
    REQUIRE_FALSE(packet.selected.empty());
    if (index == 0) {
      REQUIRE(fixture.retrieval().utility("claim:utility") == 0.0);
    }
    fixture.retrieval().record_outcome(packet_id, "claim:utility",
                                       pma::retrieval::Outcome::irrelevant);
  }
  REQUIRE(fixture.retrieval().utility("claim:utility") <= -0.75);
  const auto excluded = fixture.retrieval().recall(
      fixture.request("packet:excluded", "high strength irrelevant detail"));
  REQUIRE(std::none_of(excluded.selected.begin(), excluded.selected.end(),
                       [](const auto &value) { return value.claim_id == "claim:utility"; }));
  auto strength = fixture.database().prepare("SELECT strength FROM claims WHERE id='claim:utility'");
  REQUIRE(strength.step());
  REQUIRE(strength.column_double(0) == 1.0);
}
