#include "pma-internal/maintenance.hpp"
#include "pma-internal/storage.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

namespace {
std::filesystem::path path(std::string_view name) {
  static std::size_t sequence = 0;
  return std::filesystem::temp_directory_path() /
         ("pma-maintenance-" + std::string(name) + "-" + std::to_string(++sequence) + ".db");
}
}

TEST_CASE("backup restore verify and compact preserve authoritative rows") {
  const auto source = path("source"), backup = path("backup"), restored = path("restored");
  {
    pma::storage::Database database(source);
    database.execute("CREATE TABLE facts(id INTEGER PRIMARY KEY, text TEXT NOT NULL) STRICT");
    database.execute("INSERT INTO facts VALUES(1, 'preserved')");
  }
  REQUIRE(pma::maintenance::backup(source, backup).ok);
  REQUIRE(pma::maintenance::restore(backup, restored).ok);
  REQUIRE(pma::maintenance::verify(restored).ok);
  REQUIRE(pma::maintenance::compact(restored).ok);
  {
    pma::storage::Database database(restored);
    auto row = database.prepare("SELECT text FROM facts WHERE id=1");
    REQUIRE(row.step()); CHECK(row.column_text(0) == "preserved");
  }
  std::filesystem::remove(source); std::filesystem::remove(backup); std::filesystem::remove(restored);
}

TEST_CASE("maintenance refuses missing and corrupt inputs") {
  const auto missing = path("missing"), corrupt = path("corrupt"), destination = path("destination");
  CHECK_FALSE(pma::maintenance::verify(missing).ok);
  { std::ofstream output(corrupt); output << "not sqlite"; }
  CHECK_FALSE(pma::maintenance::backup(corrupt, destination).ok);
  CHECK_FALSE(pma::maintenance::restore(corrupt, destination).ok);
  std::filesystem::remove(corrupt);
}
