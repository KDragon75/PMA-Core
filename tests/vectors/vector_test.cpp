#include "pma-internal/lifecycle.hpp"
#include "pma-internal/vectors.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>

namespace {

class Fixture {
public:
  Fixture() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    root_ = std::filesystem::temp_directory_path() / ("pma-vector-test-" + std::to_string(nonce));
    std::filesystem::create_directories(root_);
    core_ = std::make_unique<pma::storage::Database>(root_ / "pma.sqlite");
    graph_ = std::make_unique<pma::graph::Store>(*core_);
    lifecycle_ = std::make_unique<pma::lifecycle::Processor>(*core_, *graph_);
    lifecycle_->initialize("test");
    vectors_ = std::make_unique<pma::vectors::Manager>(*core_, root_);
    vectors_->initialize("test");
    graph_->create_entity("entity:subject", "PMA-Core", "project");
    pma::lifecycle::Observation observation{
        "episode:1", "test", "source:1", "job:1",
        {{"evidence:1", 0, "PMA-Core is portable", "event:1", std::nullopt, std::nullopt}}};
    lifecycle_->observe(observation);
    pma::lifecycle::Proposal proposal{pma::lifecycle::OperationType::create,
                                      "entity:subject",
                                      "pred:has_property",
                                      std::nullopt,
                                      std::string("portable"),
                                      std::nullopt,
                                      std::nullopt,
                                      {"evidence:1"}};
    pma::lifecycle::ScriptedProvider provider({proposal});
    lifecycle_->run_job("job:1", provider);
    vectors_->render_claim("job:1:0:claim", "job:1:0:operation");
  }
  ~Fixture() {
    vectors_.reset();
    lifecycle_.reset();
    graph_.reset();
    core_.reset();
    std::filesystem::remove_all(root_);
  }

  pma::vectors::Profile profile(std::string id = "profile:1") {
    return {std::move(id), "fake", "native-test", "model-sha", "r1", "tokenizer-sha", 8};
  }
  pma::vectors::RuntimeManifest runtime(std::string id = "runtime:1") {
    return {std::move(id), "@pma/fake", "1.0", "24", "test-os", "x64", "lock-sha",
            "cpu", "deterministic"};
  }
  void register_default() { vectors_->register_profile(profile(), runtime()); }

  pma::vectors::Manager &vectors() { return *vectors_; }
  pma::storage::Database &core() { return *core_; }
  pma::graph::Store &graph() { return *graph_; }
  std::filesystem::path root() const { return root_; }

private:
  std::filesystem::path root_;
  std::unique_ptr<pma::storage::Database> core_;
  std::unique_ptr<pma::graph::Store> graph_;
  std::unique_ptr<pma::lifecycle::Processor> lifecycle_;
  std::unique_ptr<pma::vectors::Manager> vectors_;
};

} // namespace

TEST_CASE("every profile compatibility setting affects fingerprint", "[vectors]") {
  Fixture fixture;
  const auto base = fixture.profile();
  const auto original = base.fingerprint();
  auto changed = base;
#define REQUIRE_PROFILE_CHANGE(field, value)                                                          \
  changed = base;                                                                                     \
  changed.field = value;                                                                              \
  REQUIRE(changed.fingerprint() != original)
  REQUIRE_PROFILE_CHANGE(model_hash, "other-model");
  REQUIRE_PROFILE_CHANGE(model_revision, "r2");
  REQUIRE_PROFILE_CHANGE(tokenizer_hash, "other-tokenizer");
  REQUIRE_PROFILE_CHANGE(dimensions, 9);
  REQUIRE_PROFILE_CHANGE(document_prefix, "document: ");
  REQUIRE_PROFILE_CHANGE(query_prefix, "query: ");
  REQUIRE_PROFILE_CHANGE(pooling, "cls");
  REQUIRE_PROFILE_CHANGE(renderer_version, "claim-v2");
  REQUIRE_PROFILE_CHANGE(provider_options, "{\"precision\":\"high\"}");
#undef REQUIRE_PROFILE_CHANGE
}

TEST_CASE("build creates a self-describing searchable generation", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  const auto store = fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  REQUIRE(store.state == "ready");
  REQUIRE(store.vector_count == 1);
  REQUIRE(fixture.vectors().validate("store:1").empty());
  fixture.vectors().activate("store:1");
  REQUIRE(fixture.vectors().inspect("store:1").state == "active");

  const auto query = provider.embed("PMA-Core | has_property | portable", fixture.profile());
  const auto results = fixture.vectors().search("store:1", query, 5);
  REQUIRE(results.size() == 1);
  REQUIRE(results[0].document_id == "embedding:job:1:0:claim");
  REQUIRE(results[0].score > 0.99F);

  pma::storage::Database vector_file(fixture.root() / store.relative_path);
  auto metadata = vector_file.prepare("SELECT count(*) FROM vector_metadata WHERE key IN ("
                                      "'store_id','profile_fingerprint','source_core_database_id',"
                                      "'runtime_manifest_id','dimensions','watermark')");
  REQUIRE(metadata.step());
  REQUIRE(metadata.column_int64(0) == 6);
  REQUIRE(fixture.vectors().reconstruction_plan("store:1").find("model_hash=model-sha") !=
          std::string::npos);
}

TEST_CASE("catch-up replays delete idempotently", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  const auto before = fixture.vectors().inspect("store:1").watermark;
  fixture.vectors().deactivate_document("embedding:job:1:0:claim", "job:1:0:operation");
  fixture.vectors().catch_up("store:1", provider, 1);
  const auto after = fixture.vectors().inspect("store:1");
  REQUIRE(after.watermark > before);
  REQUIRE(after.vector_count == 0);
  fixture.vectors().catch_up("store:1", provider, 1);
  REQUIRE(fixture.vectors().inspect("store:1").watermark == after.watermark);
}

TEST_CASE("interrupted catch-up does not advance its batch watermark", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  fixture.vectors().render_claim("job:1:0:claim", "job:1:0:operation", "claim-v2");
  const auto before = fixture.vectors().inspect("store:1").watermark;
  provider.fail_after(0);
  REQUIRE_THROWS(fixture.vectors().catch_up("store:1", provider, 100));
  REQUIRE(fixture.vectors().inspect("store:1").watermark == before);
  provider.fail_after(std::nullopt);
  fixture.vectors().catch_up("store:1", provider);
  REQUIRE(fixture.vectors().inspect("store:1").watermark > before);
}

TEST_CASE("new generation activates atomically without deleting old generation", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  fixture.vectors().activate("store:1");
  fixture.vectors().build("store:2", "profile:1", 2, "runtime:1", provider);
  REQUIRE(fixture.vectors().inspect("store:1").state == "active");
  fixture.vectors().activate("store:2");
  REQUIRE(fixture.vectors().inspect("store:1").state == "retired");
  REQUIRE(fixture.vectors().inspect("store:2").state == "active");
  REQUIRE(std::filesystem::exists(fixture.root() /
                                  fixture.vectors().inspect("store:1").relative_path));
}

TEST_CASE("validation detects vector corruption and stale projections", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  const auto info = fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  pma::storage::Database file(fixture.root() / info.relative_path);
  std::vector<std::byte> bad(sizeof(float) * 7);
  float nan = std::numeric_limits<float>::quiet_NaN();
  std::memcpy(bad.data(), &nan, sizeof(float));
  auto corrupt = file.prepare(
      "UPDATE vectors SET dimensions=7,content_hash='wrong',vector=?1");
  corrupt.bind_blob(1, bad);
  corrupt.execute();
  auto issues = fixture.vectors().validate("store:1");
  REQUIRE(std::any_of(issues.begin(), issues.end(),
                      [](const auto &issue) { return issue.code == "wrong_dimensions"; }));
  REQUIRE(std::any_of(issues.begin(), issues.end(),
                      [](const auto &issue) { return issue.code == "content_hash_mismatch"; }));

  std::vector<std::byte> nan_vector(sizeof(float) * 8);
  std::memcpy(nan_vector.data(), &nan, sizeof(float));
  auto finite_shape = file.prepare("UPDATE vectors SET dimensions=8,vector=?1");
  finite_shape.bind_blob(1, nan_vector);
  finite_shape.execute();
  issues = fixture.vectors().validate("store:1");
  REQUIRE(std::any_of(issues.begin(), issues.end(),
                      [](const auto &issue) { return issue.code == "non_finite"; }));

  file.execute("UPDATE vector_metadata SET value='wrong-store' WHERE key='store_id';"
               "UPDATE vector_metadata SET value='0' WHERE key='watermark';"
               "INSERT INTO vectors(document_id,content_hash,dimensions,vector) "
               "VALUES('embedding:orphan','none',8,x'0000')");
  issues = fixture.vectors().validate("store:1");
  REQUIRE(std::any_of(issues.begin(), issues.end(),
                      [](const auto &issue) { return issue.code == "metadata_mismatch"; }));
  REQUIRE(std::any_of(issues.begin(), issues.end(),
                      [](const auto &issue) { return issue.code == "stale_watermark"; }));
  REQUIRE(std::any_of(issues.begin(), issues.end(),
                      [](const auto &issue) { return issue.code == "orphan_vector"; }));
}

TEST_CASE("failed rebuild preserves the active generation", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  fixture.vectors().activate("store:1");
  provider.fail_after(0);
  REQUIRE_THROWS(fixture.vectors().build("store:failed", "profile:1", 2, "runtime:1", provider));
  REQUIRE(fixture.vectors().inspect("store:1").state == "active");
  REQUIRE(fixture.vectors().inspect("store:failed").state == "building");
}

TEST_CASE("incompatible profile fingerprint is never replayed", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  fixture.core().execute(
      "UPDATE vector_store_registry SET profile_fingerprint='incompatible' WHERE id='store:1'");
  REQUIRE_THROWS(fixture.vectors().catch_up("store:1", provider));
}

TEST_CASE("missing vector files do not affect authoritative core", "[vectors]") {
  Fixture fixture;
  fixture.register_default();
  pma::vectors::FakeEmbeddingProvider provider;
  const auto info = fixture.vectors().build("store:1", "profile:1", 1, "runtime:1", provider);
  std::filesystem::remove(info.relative_path.is_absolute() ? info.relative_path
                                                           : fixture.root() / info.relative_path);
  REQUIRE(fixture.vectors().validate("store:1")[0].code == "missing_file");
  REQUIRE(fixture.graph().get_claim("job:1:0:claim").status == "active");
}
