#include "pma-internal/graph.hpp"
#include "pma-internal/lifecycle.hpp"
#include "pma-internal/retrieval.hpp"
#include "pma-internal/vectors.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace {

class Simulation {
public:
  Simulation() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    root_ = std::filesystem::temp_directory_path() /
            ("pma-learning-recall-" + std::to_string(nonce));
    std::filesystem::create_directories(root_);
    database_ = std::make_unique<pma::storage::Database>(root_ / "pma.sqlite");
    graph_ = std::make_unique<pma::graph::Store>(*database_);
    lifecycle_ = std::make_unique<pma::lifecycle::Processor>(*database_, *graph_);
    lifecycle_->initialize("simulation");
    vectors_ = std::make_unique<pma::vectors::Manager>(*database_, root_);
    vectors_->initialize("simulation");
    retrieval_ = std::make_unique<pma::retrieval::Engine>(*database_, vectors_.get());
    retrieval_->initialize("simulation");
  }

  ~Simulation() {
    retrieval_.reset();
    vectors_.reset();
    lifecycle_.reset();
    graph_.reset();
    database_.reset();
    std::filesystem::remove_all(root_);
  }

  pma::storage::Database &database() { return *database_; }
  pma::graph::Store &graph() { return *graph_; }
  pma::lifecycle::Processor &lifecycle() { return *lifecycle_; }
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

bool contains_claim(const pma::retrieval::ContextPacket &packet, std::string_view claim_id) {
  return std::any_of(packet.selected.begin(), packet.selected.end(), [&](const auto &item) {
    return item.claim_id == claim_id;
  });
}

} // namespace

TEST_CASE("full simulation learns recalls evaluates corrects and recalls again", "[simulation]") {
  Simulation simulation;
  simulation.graph().create_entity("entity:pma", "PMA-Core", "project");
  simulation.graph().create_context("context:pma", "project", "pma-core");

  const pma::lifecycle::Observation observation{
        "episode:learning",
        "simulation",
        "conversation:learning",
        "job:learning",
        {{"evidence:learning", 0,
          "The user decided that PMA-Core stores vectors in the core database.",
          "event:learning", std::string("simulated-agent"), std::nullopt}}};
    simulation.lifecycle().observe(observation);

    pma::lifecycle::Proposal learned{
        pma::lifecycle::OperationType::create,
        "entity:pma",
        "pred:has_property",
        std::nullopt,
        std::string("stores vectors in the core database"),
        std::nullopt,
        std::nullopt,
        {"evidence:learning"},
        "explicit_decision",
        1.0};
    pma::lifecycle::ScriptedProvider provider({learned});
    simulation.lifecycle().run_job("job:learning", provider);

    REQUIRE(simulation.lifecycle().get_job("job:learning").state ==
            pma::lifecycle::JobState::succeeded);
    REQUIRE(simulation.graph().get_claim("job:learning:0:claim").status == "active");
  REQUIRE(simulation.graph().get_evidence("evidence:learning").text ==
          "The user decided that PMA-Core stores vectors in the core database.");

  simulation.vectors().render_claim("job:learning:0:claim", "job:learning:0:operation");
    const pma::vectors::Profile profile{"profile:simulation", "fake", "deterministic",
                                        "model:simulation", "1", "tokenizer:simulation", 16};
    const pma::vectors::RuntimeManifest runtime{
        "runtime:simulation", "pma-fake", "1", "24", "test", "portable", "lock",
        "cpu", "deterministic"};
    simulation.vectors().register_profile(profile, runtime);
    pma::vectors::FakeEmbeddingProvider embeddings;
    simulation.vectors().build("store:simulation", profile.id, 1, runtime.id, embeddings);
    simulation.vectors().activate("store:simulation");
    simulation.retrieval().rebuild_fts();

    auto request = pma::retrieval::RecallRequest{
        "packet:first-recall", "interaction:first-recall",
        "Where does PMA-Core store vectors?", {"context:pma"}, "store:simulation",
        embeddings.embed("PMA-Core | has_property | stores vectors in the core database", profile)};
    const auto packet = simulation.retrieval().recall(request);
    REQUIRE(contains_claim(packet, "job:learning:0:claim"));
    REQUIRE(packet.selected.front().scores.final_score > 0.0);
    REQUIRE_FALSE(packet.selected.front().reasons.empty());

    const auto slice = simulation.retrieval().render_slice(packet, 80);
    REQUIRE(slice.estimated_tokens <= 80);
    REQUIRE(slice.text.find("stores vectors in the core database") != std::string::npos);
    REQUIRE(slice.text.find("final_score") == std::string::npos);
    REQUIRE(slice.text.find("job:learning") == std::string::npos);

    const double utility_before = simulation.retrieval().utility("job:learning:0:claim");
    REQUIRE(utility_before == 0.0);
    simulation.retrieval().record_outcome("packet:first-recall", "job:learning:0:claim",
                                          pma::retrieval::Outcome::used);
    REQUIRE(simulation.retrieval().utility("job:learning:0:claim") > utility_before);

  const pma::lifecycle::Observation correction{
          "episode:correction",
          "simulation",
          "conversation:correction",
          "job:correction-observation",
          {{"evidence:correction", 0,
            "Correction: vectors use separate rebuildable SQLite projection files.",
            "event:correction", std::string("simulated-agent"), std::nullopt}}};
      simulation.lifecycle().observe(correction);
      const auto current_claim = simulation.lifecycle().correct(
          "job:direct-correction", "job:learning:0:claim", "evidence:correction",
          "uses separate rebuildable SQLite projection files", 1.0);

      REQUIRE(simulation.graph().get_claim("job:learning:0:claim").status == "superseded");
      REQUIRE(simulation.graph().get_claim(current_claim).status == "active");
      REQUIRE(simulation.graph().get_evidence("evidence:learning").text.find("core database") !=
              std::string::npos);

      simulation.vectors().render_claim(current_claim, "job:direct-correction:0:operation");
      simulation.vectors().render_claim("job:learning:0:claim", "job:learning:0:operation");
      simulation.vectors().catch_up("store:simulation", embeddings, 1);
      simulation.retrieval().rebuild_fts();

      auto corrected_request = pma::retrieval::RecallRequest{
          "packet:corrected-recall", "interaction:corrected-recall",
          "What is the current PMA-Core vector storage decision?", {"context:pma"},
          "store:simulation",
          embeddings.embed(
              "PMA-Core | has_property | uses separate rebuildable SQLite projection files",
              profile)};
      const auto corrected_packet = simulation.retrieval().recall(corrected_request);
      REQUIRE(contains_claim(corrected_packet, current_claim));
      REQUIRE(corrected_packet.selected.front().claim_id == current_claim);
      REQUIRE(simulation.graph().get_claim("job:learning:0:claim").status == "superseded");

      const auto corrected_slice = simulation.retrieval().render_slice(corrected_packet, 100);
      REQUIRE(corrected_slice.text.find("separate rebuildable SQLite projection files") !=
              std::string::npos);
      REQUIRE(corrected_slice.estimated_tokens <= 100);

      auto audits = simulation.database().prepare("SELECT count(*) FROM memory_operations");
      REQUIRE(audits.step());
      REQUIRE(audits.column_int64(0) == 2);
      auto packets = simulation.database().prepare("SELECT count(*) FROM context_packets");
      REQUIRE(packets.step());
  REQUIRE(packets.column_int64(0) == 2);
}
