#include "pma-internal/storage.hpp"

#include "sha-256.h"
#include "sqlite3.h"

#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <utility>

namespace pma::storage {
namespace {

ErrorCode classify(int code) {
  const int primary = code & 0xff;
  if (primary == SQLITE_CONSTRAINT) {
    return ErrorCode::constraint;
  }
  if (primary == SQLITE_CORRUPT || primary == SQLITE_NOTADB) {
    return ErrorCode::corruption;
  }
  return ErrorCode::sql;
}

[[noreturn]] void fail(sqlite3 *database, int code, std::string_view context,
                       ErrorCode fallback = ErrorCode::sql) {
  const auto classified = classify(code);
  const auto selected = classified == ErrorCode::sql ? fallback : classified;
  std::string message{context};
  message += ": ";
  message += database == nullptr ? sqlite3_errstr(code) : sqlite3_errmsg(database);
  throw Error(selected, code, std::move(message));
}

std::string checksum(std::string_view content) {
  std::array<std::uint8_t, SIZE_OF_SHA_256_HASH> digest{};
  calc_sha_256(digest.data(), content.data(), content.size());
  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (const auto byte : digest) {
    output << std::setw(2) << static_cast<unsigned>(byte);
  }
  return output.str();
}

std::string generate_identity() {
  std::random_device random;
  std::array<std::uint8_t, 16> bytes{};
  for (auto &byte : bytes) {
    byte = static_cast<std::uint8_t>(random());
  }
  bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0fU) | 0x40U);
  bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3fU) | 0x80U);
  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    if (index == 4 || index == 6 || index == 8 || index == 10) {
      output << '-';
    }
    output << std::setw(2) << static_cast<unsigned>(bytes[index]);
  }
  return output.str();
}

} // namespace

Error::Error(ErrorCode code, int native_code, std::string message)
    : std::runtime_error(std::move(message)), code_(code), native_code_(native_code) {}

ErrorCode Error::code() const noexcept { return code_; }
int Error::native_code() const noexcept { return native_code_; }

struct Statement::Impl {
  sqlite3_stmt *statement{};
};

Statement::Statement(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
Statement::Statement(Statement &&) noexcept = default;
Statement &Statement::operator=(Statement &&) noexcept = default;
Statement::~Statement() {
  if (impl_ != nullptr && impl_->statement != nullptr) {
    sqlite3_finalize(impl_->statement);
  }
}

void Statement::bind(std::size_t index, std::int64_t value) {
  const int result = sqlite3_bind_int64(impl_->statement, static_cast<int>(index), value);
  if (result != SQLITE_OK) {
    fail(sqlite3_db_handle(impl_->statement), result, "bind integer");
  }
}

void Statement::bind(std::size_t index, std::string_view value) {
  const int result = sqlite3_bind_text64(impl_->statement, static_cast<int>(index), value.data(),
                                         value.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
  if (result != SQLITE_OK) {
    fail(sqlite3_db_handle(impl_->statement), result, "bind text");
  }
}

void Statement::bind_null(std::size_t index) {
  const int result = sqlite3_bind_null(impl_->statement, static_cast<int>(index));
  if (result != SQLITE_OK) {
    fail(sqlite3_db_handle(impl_->statement), result, "bind null");
  }
}

bool Statement::step() {
  const int result = sqlite3_step(impl_->statement);
  if (result == SQLITE_ROW) {
    return true;
  }
  if (result == SQLITE_DONE) {
    return false;
  }
  fail(sqlite3_db_handle(impl_->statement), result, "execute statement");
}

void Statement::execute() {
  if (step()) {
    throw Error(ErrorCode::sql, SQLITE_MISUSE, "statement unexpectedly returned rows");
  }
}

void Statement::reset() {
  int result = sqlite3_reset(impl_->statement);
  if (result == SQLITE_OK) {
    result = sqlite3_clear_bindings(impl_->statement);
  }
  if (result != SQLITE_OK) {
    fail(sqlite3_db_handle(impl_->statement), result, "reset statement");
  }
}

std::int64_t Statement::column_int64(std::size_t index) const {
  return sqlite3_column_int64(impl_->statement, static_cast<int>(index));
}

std::string Statement::column_text(std::size_t index) const {
  const auto *text = sqlite3_column_text(impl_->statement, static_cast<int>(index));
  const auto length = sqlite3_column_bytes(impl_->statement, static_cast<int>(index));
  if (text == nullptr) {
    return {};
  }
  return {reinterpret_cast<const char *>(text), static_cast<std::size_t>(length)};
}

bool Statement::column_is_null(std::size_t index) const {
  return sqlite3_column_type(impl_->statement, static_cast<int>(index)) == SQLITE_NULL;
}

struct Database::Impl {
  sqlite3 *database{};
};

Database::Database(const std::filesystem::path &path) : impl_(std::make_unique<Impl>()) {
  const auto path_text = path.u8string();
  const int result = sqlite3_open_v2(reinterpret_cast<const char *>(path_text.c_str()),
                                     &impl_->database,
                                     SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                                     nullptr);
  if (result != SQLITE_OK) {
    sqlite3 *failed_database = impl_->database;
    impl_->database = nullptr;
    std::string message = failed_database == nullptr ? sqlite3_errstr(result)
                                                     : sqlite3_errmsg(failed_database);
    sqlite3_close(failed_database);
    throw Error(ErrorCode::open, result, "open database: " + message);
  }
  sqlite3_extended_result_codes(impl_->database, 1);
  try {
    execute("PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL; PRAGMA synchronous = NORMAL; "
            "PRAGMA busy_timeout = 5000;");
    execute("CREATE TABLE IF NOT EXISTS schema_migrations("
            "version INTEGER PRIMARY KEY, name TEXT NOT NULL, content_hash TEXT NOT NULL, "
            "applied_at TEXT NOT NULL, build_version TEXT NOT NULL) STRICT;"
            "CREATE TABLE IF NOT EXISTS system_metadata("
            "key TEXT PRIMARY KEY, value TEXT NOT NULL) STRICT;");
    auto insert = prepare("INSERT OR IGNORE INTO system_metadata(key,value) VALUES('database_id',?1)");
    insert.bind(1, generate_identity());
    insert.execute();
  } catch (...) {
    sqlite3_close(impl_->database);
    impl_->database = nullptr;
    throw;
  }
}

Database::Database(Database &&) noexcept = default;
Database &Database::operator=(Database &&) noexcept = default;
Database::~Database() {
  if (impl_ != nullptr && impl_->database != nullptr) {
    sqlite3_close_v2(impl_->database);
  }
}

Statement Database::prepare(std::string_view sql) {
  auto impl = std::make_unique<Statement::Impl>();
  const int result = sqlite3_prepare_v3(impl_->database, sql.data(), static_cast<int>(sql.size()),
                                        SQLITE_PREPARE_PERSISTENT, &impl->statement, nullptr);
  if (result != SQLITE_OK) {
    fail(impl_->database, result, "prepare statement");
  }
  return Statement(std::move(impl));
}

void Database::execute(std::string_view sql) {
  char *detail = nullptr;
  const std::string terminated{sql};
  const int result = sqlite3_exec(impl_->database, terminated.c_str(), nullptr, nullptr, &detail);
  if (result != SQLITE_OK) {
    std::string context = "execute SQL";
    if (detail != nullptr) {
      context += ": ";
      context += detail;
    }
    sqlite3_free(detail);
    fail(impl_->database, result, context);
  }
}

std::string Database::identity() {
  auto query = prepare("SELECT value FROM system_metadata WHERE key='database_id'");
  if (!query.step()) {
    throw Error(ErrorCode::corruption, SQLITE_CORRUPT, "database identity is missing");
  }
  return query.column_text(0);
}

void Database::backup_to(const std::filesystem::path &destination) {
  const auto path_text = destination.u8string();
  sqlite3 *target = nullptr;
  int result = sqlite3_open_v2(reinterpret_cast<const char *>(path_text.c_str()), &target,
                               SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (result != SQLITE_OK) {
    const std::string detail = target == nullptr ? sqlite3_errstr(result) : sqlite3_errmsg(target);
    sqlite3_close(target);
    throw Error(ErrorCode::backup, result, "open backup destination: " + detail);
  }
  sqlite3_backup *backup = sqlite3_backup_init(target, "main", impl_->database, "main");
  if (backup == nullptr) {
    result = sqlite3_errcode(target);
  } else {
    do {
      result = sqlite3_backup_step(backup, -1);
    } while (result == SQLITE_BUSY || result == SQLITE_LOCKED);
    const int finish_result = sqlite3_backup_finish(backup);
    if (result == SQLITE_DONE) {
      result = finish_result;
    }
  }
  if (result != SQLITE_OK) {
    const std::string detail = sqlite3_errmsg(target);
    sqlite3_close(target);
    throw Error(ErrorCode::backup, result, "online backup: " + detail);
  }
  sqlite3_close(target);
}

Transaction::Transaction(Database &database) : database_(&database) {
  database_->execute("BEGIN IMMEDIATE");
}

Transaction::~Transaction() {
  if (active_) {
    try {
      database_->execute("ROLLBACK");
    } catch (...) {
    }
  }
}

void Transaction::commit() {
  if (!active_) {
    throw Error(ErrorCode::sql, SQLITE_MISUSE, "transaction is not active");
  }
  database_->execute("COMMIT");
  active_ = false;
}

void Transaction::rollback() {
  if (!active_) {
    throw Error(ErrorCode::sql, SQLITE_MISUSE, "transaction is not active");
  }
  database_->execute("ROLLBACK");
  active_ = false;
}

void migrate(Database &database, const std::vector<Migration> &migrations,
             std::string_view build_version) {
  std::int64_t previous = 0;
  for (const auto &migration : migrations) {
    if (migration.version <= previous || migration.version <= 0) {
      throw Error(ErrorCode::migration, SQLITE_MISUSE,
                  "migrations must have strictly increasing positive versions");
    }
    previous = migration.version;
    const std::string hash = checksum(migration.sql);
    auto existing = database.prepare(
        "SELECT name,content_hash FROM schema_migrations WHERE version=?1");
    existing.bind(1, migration.version);
    if (existing.step()) {
      if (existing.column_text(0) != migration.name || existing.column_text(1) != hash) {
        throw Error(ErrorCode::migration, SQLITE_MISMATCH,
                    "migration checksum or name mismatch at version " +
                        std::to_string(migration.version));
      }
      continue;
    }

    Transaction transaction(database);
    try {
      database.execute(migration.sql);
      auto record = database.prepare(
          "INSERT INTO schema_migrations(version,name,content_hash,applied_at,build_version) "
          "VALUES(?1,?2,?3,strftime('%Y-%m-%dT%H:%M:%fZ','now'),?4)");
      record.bind(1, migration.version);
      record.bind(2, migration.name);
      record.bind(3, hash);
      record.bind(4, build_version);
      record.execute();
      transaction.commit();
    } catch (const Error &error) {
      throw Error(ErrorCode::migration, error.native_code(),
                  "migration " + std::to_string(migration.version) + " failed: " + error.what());
    }
  }
}

VerificationResult verify(Database &database) {
  VerificationResult result{true, {}};
  auto integrity = database.prepare("PRAGMA integrity_check");
  while (integrity.step()) {
    const auto detail = integrity.column_text(0);
    if (detail != "ok") {
      result.issues.push_back("integrity: " + detail);
    }
  }
  auto foreign_keys = database.prepare("PRAGMA foreign_key_check");
  while (foreign_keys.step()) {
    result.issues.push_back("foreign key violation in " + foreign_keys.column_text(0));
  }
  result.ok = result.issues.empty();
  return result;
}

} // namespace pma::storage
