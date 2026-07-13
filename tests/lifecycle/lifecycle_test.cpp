#include "pma-internal/lifecycle.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace {

class Fixture {
public:
  Fixture() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    directory_ = std::filesystem::temp_directory_path() /
                 ("pma-lifecycle-test-" + std::to_string(nonce));
    std::filesystem::create_directories(directory_);
    database_ = std::make_unique<pma::storage::Database>(directory_ / "pma.sqlite");
    graph_ = std::make_unique<pma::graph::Store>(*database_);
    processor_ = std::make_unique<pma::lifecycle::Processor>(*database_, *graph_);
    processor_->initialize("test");
    graph_->create_entity("entity:subject", "PMA-Core", "project");
    graph_->create_entity("entity:object", "SQLite", "technology");
    graph_->create_context("context:project", "project", "pma-core");
  }
  ~Fixture() {
    processor_.reset();
    graph_.reset();
    database_.reset();
    std::filesystem::remove_all(directory_);
  }

  pma::lifecycle::Observation observe(std::string suffix, std::string text = "observed fact") {
    pma::lifecycle::Observation observation{
        "episode:" + suffix,
        "test",
        "source:" + suffix,
        "job:" + suffix,
        {{"evidence:" + suffix, 0, std::move(text), "event:" + suffix, std::nullopt,
          std::nullopt}}};
    processor_->observe(observation);
    return observation;
  }

  pma::lifecycle::Proposal create(std::string evidence) {
    return {pma::lifecycle::OperationType::create,
            "entity:subject",
            "pred:uses",
            std::string("entity:object"),
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {std::move(evidence)},
            "direct_observation",
            0.95};
  }

  pma::storage::Database &database() { return *database_; }
  pma::graph::Store &graph() { return *graph_; }
  pma::lifecycle::Processor &processor() { return *processor_; }

private:
  std::filesystem::path directory_;
  std::unique_ptr<pma::storage::Database> database_;
  std::unique_ptr<pma::graph::Store> graph_;
  std::unique_ptr<pma::lifecycle::Processor> processor_;
};

std::int64_t count(pma::storage::Database &database, std::string_view table) {
  auto query = database.prepare("SELECT count(*) FROM " + std::string(table));
  REQUIRE(query.step());
  return query.column_int64(0);
}

} // namespace

TEST_CASE("observation and provider proposal produce an audited claim", "[lifecycle]") {
  Fixture fixture;
  const auto observation = fixture.observe("one", "PMA-Core uses SQLite");
  pma::lifecycle::ScriptedProvider provider({fixture.create("evidence:one")});
  fixture.processor().run_job(observation.job_id, provider);

  const auto job = fixture.processor().get_job(observation.job_id);
  REQUIRE(job.state == pma::lifecycle::JobState::succeeded);
  REQUIRE(job.checkpoint == 1);
  const auto claim = fixture.graph().get_claim("job:one:0:claim");
  REQUIRE(claim.object_entity_id == "entity:object");
  REQUIRE(count(fixture.database(), "memory_operations") == 1);
  REQUIRE(count(fixture.database(), "embedding_events") == 1);
  REQUIRE(fixture.graph().get_evidence("evidence:one").text == "PMA-Core uses SQLite");
}

TEST_CASE("provider failure leaves committed evidence and can resume", "[lifecycle]") {
  Fixture fixture;
  const auto observation = fixture.observe("retry");
  pma::lifecycle::ScriptedProvider provider({fixture.create("evidence:retry")});
  provider.fail_next("offline");
  fixture.processor().run_job(observation.job_id, provider);
  REQUIRE(fixture.processor().get_job(observation.job_id).state ==
          pma::lifecycle::JobState::queued);
  REQUIRE(fixture.graph().get_evidence("evidence:retry").text == "observed fact");

  pma::lifecycle::Processor restarted(fixture.database(), fixture.graph());
  restarted.run_job(observation.job_id, provider);
  REQUIRE(restarted.get_job(observation.job_id).state == pma::lifecycle::JobState::succeeded);
  REQUIRE(restarted.get_job(observation.job_id).attempts == 2);
}

TEST_CASE("repeated provider failure poisons a job", "[lifecycle]") {
  Fixture fixture;
  const auto observation = fixture.observe("poison");
  pma::lifecycle::ScriptedProvider provider({});
  for (int attempt = 0; attempt < 3; ++attempt) {
    provider.fail_next("still offline");
    fixture.processor().run_job(observation.job_id, provider, 3);
  }
  const auto job = fixture.processor().get_job(observation.job_id);
  REQUIRE(job.state == pma::lifecycle::JobState::failed);
  REQUIRE(job.attempts == 3);
  REQUIRE(job.error_code == "PROVIDER_UNAVAILABLE");
  REQUIRE(count(fixture.database(), "job_attempts") == 3);
}

TEST_CASE("invalid proposal batches are atomic and become poison jobs", "[lifecycle]") {
  Fixture fixture;
  const auto observation = fixture.observe("invalid");
  auto valid = fixture.create("evidence:invalid");
  auto invalid = fixture.create("evidence:invented");
  pma::lifecycle::ScriptedProvider provider({valid, invalid});
  fixture.processor().run_job(observation.job_id, provider);
  REQUIRE(fixture.processor().get_job(observation.job_id).state ==
          pma::lifecycle::JobState::failed);
  REQUIRE(fixture.processor().get_job(observation.job_id).error_code == "INVALID_PROPOSAL");
  REQUIRE(count(fixture.database(), "claims") == 0);
  REQUIRE(count(fixture.database(), "memory_operations") == 0);
  REQUIRE(fixture.graph().get_evidence("evidence:invalid").text == "observed fact");
}

TEST_CASE("support contradiction deactivate restore and context are audited", "[lifecycle]") {
  Fixture fixture;
  auto initial = fixture.observe("initial");
  pma::lifecycle::ScriptedProvider create_provider({fixture.create("evidence:initial")});
  fixture.processor().run_job(initial.job_id, create_provider);
  const std::string claim_id = "job:initial:0:claim";

  auto changes = fixture.observe("changes", "new evidence");
  const auto operation = [&](pma::lifecycle::OperationType type) {
    return pma::lifecycle::Proposal{type,
                                    {},
                                    {},
                                    std::nullopt,
                                    std::nullopt,
                                    claim_id,
                                    type == pma::lifecycle::OperationType::contextualize
                                        ? std::optional<std::string>{"context:project"}
                                        : std::nullopt,
                                    (type == pma::lifecycle::OperationType::support ||
                                     type == pma::lifecycle::OperationType::contradict)
                                        ? std::vector<std::string>{"evidence:changes"}
                                        : std::vector<std::string>{}};
  };
  fixture.processor().apply(changes.job_id,
                            {operation(pma::lifecycle::OperationType::support),
                             operation(pma::lifecycle::OperationType::contradict),
                             operation(pma::lifecycle::OperationType::contextualize),
                             operation(pma::lifecycle::OperationType::deactivate),
                             operation(pma::lifecycle::OperationType::restore)});
  REQUIRE(fixture.graph().get_claim(claim_id).status == "active");
  REQUIRE(count(fixture.database(), "memory_operations") == 6);
  REQUIRE(count(fixture.database(), "embedding_events") == 6);
}

TEST_CASE("qualification creates a specific claim and preserves the broad claim", "[lifecycle]") {
  Fixture fixture;
  auto original = fixture.observe("broad");
  pma::lifecycle::ScriptedProvider provider({fixture.create("evidence:broad")});
  fixture.processor().run_job(original.job_id, provider);
  auto detail = fixture.observe("qualified", "only in this environment");
  auto qualification = fixture.create("evidence:qualified");
  qualification.operation = pma::lifecycle::OperationType::qualify;
  qualification.target_claim_id = "job:broad:0:claim";
  qualification.object_entity_id.reset();
  qualification.literal_text = "environment-specific";
  qualification.predicate_id = "pred:has_property";
  fixture.processor().apply(detail.job_id, {qualification});
  REQUIRE(fixture.graph().get_claim("job:broad:0:claim").status == "active");
  REQUIRE(fixture.graph().get_claim("job:qualified:0:claim").status == "active");
  auto edge = fixture.database().prepare(
      "SELECT relation FROM graph_edges WHERE source_id='job:qualified:0:claim'");
  REQUIRE(edge.step());
  REQUIRE(edge.column_text(0) == "qualifies");
}

TEST_CASE("explicit correction supersedes without deleting history", "[lifecycle]") {
  Fixture fixture;
  auto original = fixture.observe("original", "old decision");
  auto proposal = fixture.create("evidence:original");
  proposal.object_entity_id.reset();
  proposal.literal_text = "old value";
  proposal.predicate_id = "pred:has_property";
  pma::lifecycle::ScriptedProvider provider({proposal});
  fixture.processor().run_job(original.job_id, provider);

  fixture.observe("correction", "explicit correction");
  const auto replacement = fixture.processor().correct(
      "correction:operation", "job:original:0:claim", "evidence:correction", "new value");
  REQUIRE(fixture.graph().get_claim("job:original:0:claim").status == "superseded");
  REQUIRE(fixture.graph().get_claim(replacement).status == "active");
  REQUIRE(fixture.graph().get_evidence("evidence:original").text == "old decision");
  REQUIRE(fixture.graph().get_evidence("evidence:correction").text == "explicit correction");
  REQUIRE(count(fixture.database(), "claims") == 2);
}
