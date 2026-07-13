#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pma::storage {

enum class ErrorCode { open, sql, constraint, migration, corruption, backup };

class Error final : public std::runtime_error {
public:
  Error(ErrorCode code, int native_code, std::string message);
  [[nodiscard]] ErrorCode code() const noexcept;
  [[nodiscard]] int native_code() const noexcept;

private:
  ErrorCode code_;
  int native_code_;
};

class Statement {
public:
  Statement(Statement &&) noexcept;
  Statement &operator=(Statement &&) noexcept;
  ~Statement();
  Statement(const Statement &) = delete;
  Statement &operator=(const Statement &) = delete;

  void bind(std::size_t index, std::int64_t value);
  void bind(std::size_t index, double value);
  void bind(std::size_t index, std::string_view value);
  void bind_blob(std::size_t index, const std::vector<std::byte> &value);
  void bind_null(std::size_t index);
  [[nodiscard]] bool step();
  void execute();
  void reset();
  [[nodiscard]] std::int64_t column_int64(std::size_t index) const;
  [[nodiscard]] double column_double(std::size_t index) const;
  [[nodiscard]] std::string column_text(std::size_t index) const;
  [[nodiscard]] std::vector<std::byte> column_blob(std::size_t index) const;
  [[nodiscard]] bool column_is_null(std::size_t index) const;

private:
  struct Impl;
  explicit Statement(std::unique_ptr<Impl> impl);
  std::unique_ptr<Impl> impl_;
  friend class Database;
};

class Database {
public:
  explicit Database(const std::filesystem::path &path);
  Database(Database &&) noexcept;
  Database &operator=(Database &&) noexcept;
  ~Database();
  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;

  [[nodiscard]] Statement prepare(std::string_view sql);
  void execute(std::string_view sql);
  [[nodiscard]] std::string identity();
  void backup_to(const std::filesystem::path &destination);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  friend class Transaction;
};

class Transaction {
public:
  explicit Transaction(Database &database);
  ~Transaction();
  Transaction(const Transaction &) = delete;
  Transaction &operator=(const Transaction &) = delete;
  void commit();
  void rollback();

private:
  Database *database_;
  bool active_{true};
};

struct Migration {
  std::int64_t version;
  std::string name;
  std::string sql;
};

void migrate(Database &database, const std::vector<Migration> &migrations,
             std::string_view build_version);

struct VerificationResult {
  bool ok;
  std::vector<std::string> issues;
};

[[nodiscard]] VerificationResult verify(Database &database);
[[nodiscard]] std::string sha256(std::string_view content);

} // namespace pma::storage
