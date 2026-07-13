#include "pma-internal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <string>

namespace {

class TemporaryDatabase {
public:
  TemporaryDatabase() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    directory_ = std::filesystem::temp_directory_path() /
                 ("pma-storage-test-" + std::to_string(nonce));
    std::filesystem::create_directories(directory_);
  }
  ~TemporaryDatabase() { std::filesystem::remove_all(directory_); }

  [[nodiscard]] std::filesystem::path path(std::string_view name = "pma.sqlite") const {
    return directory_ / name;
  }

private:
  std::filesystem::path directory_;
};

} // namespace

using pma::storage::Database;
using pma::storage::Error;
using pma::storage::ErrorCode;
using pma::storage::Migration;
using pma::storage::Transaction;

TEST_CASE("statements bind and return UTF-8 text", "[storage]") {
  TemporaryDatabase fixture;
  Database database(fixture.path());
  database.execute("CREATE TABLE messages(id INTEGER PRIMARY KEY, body TEXT) STRICT");
  auto insert = database.prepare("INSERT INTO messages(body) VALUES(?1)");
  insert.bind(1, "Zażółć gęślą jaźń 🐉");
  insert.execute();
  auto query = database.prepare("SELECT body FROM messages");
  REQUIRE(query.step());
  REQUIRE(query.column_text(0) == "Zażółć gęślą jaźń 🐉");
}

TEST_CASE("transactions commit and roll back", "[storage]") {
  TemporaryDatabase fixture;
  Database database(fixture.path());
  database.execute("CREATE TABLE values_table(value INTEGER NOT NULL) STRICT");
  {
    Transaction transaction(database);
    database.execute("INSERT INTO values_table VALUES(1)");
    transaction.commit();
  }
  {
    Transaction transaction(database);
    database.execute("INSERT INTO values_table VALUES(2)");
  }
  auto count = database.prepare("SELECT count(*) FROM values_table");
  REQUIRE(count.step());
  REQUIRE(count.column_int64(0) == 1);
}

TEST_CASE("foreign keys are enabled and constraints are typed", "[storage]") {
  TemporaryDatabase fixture;
  Database database(fixture.path());
  database.execute("CREATE TABLE parent(id INTEGER PRIMARY KEY) STRICT;"
                   "CREATE TABLE child(parent_id INTEGER REFERENCES parent(id)) STRICT;");
  try {
    database.execute("INSERT INTO child VALUES(99)");
    FAIL("foreign key violation was accepted");
  } catch (const Error &error) {
    REQUIRE(error.code() == ErrorCode::constraint);
  }
}

TEST_CASE("migrations apply once and enforce checksums", "[storage]") {
  TemporaryDatabase fixture;
  Database database(fixture.path());
  const std::vector migrations{
      Migration{1, "create_items", "CREATE TABLE items(id INTEGER PRIMARY KEY) STRICT"},
      Migration{2, "add_item", "INSERT INTO items VALUES(1)"},
  };
  pma::storage::migrate(database, migrations, "test");
  pma::storage::migrate(database, migrations, "test");
  auto count = database.prepare("SELECT count(*) FROM schema_migrations");
  REQUIRE(count.step());
  REQUIRE(count.column_int64(0) == 2);

  const std::vector changed{
      Migration{1, "create_items", "CREATE TABLE items(id TEXT PRIMARY KEY) STRICT"}};
  try {
    pma::storage::migrate(database, changed, "test");
    FAIL("changed migration was accepted");
  } catch (const Error &error) {
    REQUIRE(error.code() == ErrorCode::migration);
  }
}

TEST_CASE("failed migration rolls back schema and version", "[storage]") {
  TemporaryDatabase fixture;
  Database database(fixture.path());
  const std::vector migrations{
      Migration{1, "broken", "CREATE TABLE partial(id INTEGER) STRICT; INVALID SQL"}};
  REQUIRE_THROWS_AS(pma::storage::migrate(database, migrations, "test"), Error);
  auto version_count = database.prepare("SELECT count(*) FROM schema_migrations");
  REQUIRE(version_count.step());
  REQUIRE(version_count.column_int64(0) == 0);
  auto table_count = database.prepare(
      "SELECT count(*) FROM sqlite_schema WHERE type='table' AND name='partial'");
  REQUIRE(table_count.step());
  REQUIRE(table_count.column_int64(0) == 0);
}

TEST_CASE("database identity survives reopen", "[storage]") {
  TemporaryDatabase fixture;
  std::string identity;
  {
    Database database(fixture.path());
    identity = database.identity();
    REQUIRE_FALSE(identity.empty());
  }
  Database reopened(fixture.path());
  REQUIRE(reopened.identity() == identity);
}

TEST_CASE("online backup copies a live WAL database", "[storage]") {
  TemporaryDatabase fixture;
  Database source(fixture.path());
  source.execute("CREATE TABLE live_data(value TEXT NOT NULL) STRICT;"
                 "INSERT INTO live_data VALUES('durable')");
  source.backup_to(fixture.path("backup.sqlite"));

  Database backup(fixture.path("backup.sqlite"));
  auto query = backup.prepare("SELECT value FROM live_data");
  REQUIRE(query.step());
  REQUIRE(query.column_text(0) == "durable");
  REQUIRE(pma::storage::verify(backup).ok);
}

TEST_CASE("verification reports foreign key violations", "[storage]") {
  TemporaryDatabase fixture;
  Database database(fixture.path());
  database.execute("CREATE TABLE parent(id INTEGER PRIMARY KEY) STRICT;"
                   "CREATE TABLE child(parent_id INTEGER REFERENCES parent(id)) STRICT;"
                   "PRAGMA foreign_keys=OFF; INSERT INTO child VALUES(42); PRAGMA foreign_keys=ON;");
  const auto result = pma::storage::verify(database);
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.issues.empty());
}
