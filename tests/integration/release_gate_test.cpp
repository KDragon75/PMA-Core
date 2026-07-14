#include "pma-internal/graph.hpp"
#include "pma-internal/lifecycle.hpp"
#include "pma-internal/maintenance.hpp"
#include "pma-internal/retrieval.hpp"
#include "pma-internal/storage.hpp"
#include "pma-internal/vectors.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

namespace {
std::filesystem::path temporary(std::string_view name) {
  static std::size_t sequence = 0;
  return std::filesystem::temp_directory_path() /
         ("pma-release-" + std::string(name) + "-" + std::to_string(++sequence) + ".db");
}
void initialize_current(pma::storage::Database &database, const std::filesystem::path &root) {
  pma::graph::Store graph(database);
  pma::lifecycle::Processor lifecycle(database, graph);
  lifecycle.initialize("0.1.0");
  pma::vectors::Manager vectors(database, root);
  vectors.initialize("0.1.0");
  pma::retrieval::Engine retrieval(database, &vectors);
  retrieval.initialize("0.1.0");
}
}

TEST_CASE("fresh and current release schema fixtures open idempotently") {
  const auto root = temporary("bundle").replace_extension();
  std::filesystem::create_directories(root);
  const auto database_path = root / "pma.sqlite";
  {
    pma::storage::Database database(database_path);
    initialize_current(database, root);
    auto versions = database.prepare("SELECT group_concat(version, ',') FROM schema_migrations ORDER BY version");
    REQUIRE(versions.step()); CHECK(versions.column_text(0) == "100,101,200,300,400");
  }
  {
    pma::storage::Database database(database_path);
    initialize_current(database, root);
    auto count = database.prepare("SELECT count(*) FROM schema_migrations");
    REQUIRE(count.step()); CHECK(count.column_int64(0) == 5);
  }
  CHECK(pma::maintenance::verify(database_path).ok);
  std::filesystem::remove_all(root);
}
