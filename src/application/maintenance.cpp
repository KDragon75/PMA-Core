#include "pma-internal/maintenance.hpp"

#include "pma-internal/storage.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace pma::maintenance {
namespace {
Result failure(std::string message) { return {false, std::move(message)}; }
bool same_path(const std::filesystem::path &left, const std::filesystem::path &right) {
  return std::filesystem::absolute(left).lexically_normal() ==
         std::filesystem::absolute(right).lexically_normal();
}

Result verify_open(storage::Database &database) {
  const auto checked = storage::verify(database);
  if (checked.ok) return {true, "verification passed"};
  std::string message = "verification failed";
  for (const auto &issue : checked.issues) message += "; " + issue;
  return failure(std::move(message));
}
} // namespace

Result backup(const std::filesystem::path &database,
              const std::filesystem::path &destination) {
  try {
    if (!std::filesystem::exists(database)) return failure("source database does not exist");
    if (same_path(database, destination)) return failure("source and destination must differ");
    std::filesystem::create_directories(destination.parent_path());
    storage::Database source(database);
    const auto checked = verify_open(source);
    if (!checked.ok) return checked;
    source.backup_to(destination);
    storage::Database copied(destination);
    return verify_open(copied);
  } catch (const std::exception &error) { return failure(error.what()); }
}

Result restore(const std::filesystem::path &backup_path,
               const std::filesystem::path &database) {
  try {
    if (!std::filesystem::exists(backup_path)) return failure("backup database does not exist");
    if (same_path(backup_path, database)) return failure("backup and destination must differ");
    storage::Database source(backup_path);
    const auto checked = verify_open(source);
    if (!checked.ok) return checked;
    std::filesystem::create_directories(database.parent_path());
    const auto staged = database.string() + ".restore-staged";
    std::filesystem::remove(staged);
    source.backup_to(staged);
    {
      storage::Database candidate(staged);
      const auto staged_check = verify_open(candidate);
      if (!staged_check.ok) { std::filesystem::remove(staged); return staged_check; }
    }
    const auto prior = database.string() + ".restore-prior";
    const auto wal = database.string() + "-wal";
    const auto shm = database.string() + "-shm";
    const auto prior_wal = prior + "-wal";
    const auto prior_shm = prior + "-shm";
    std::filesystem::remove(prior);
    std::filesystem::remove(prior_wal);
    std::filesystem::remove(prior_shm);
    if (std::filesystem::exists(database)) std::filesystem::rename(database, prior);
    if (std::filesystem::exists(wal)) std::filesystem::rename(wal, prior_wal);
    if (std::filesystem::exists(shm)) std::filesystem::rename(shm, prior_shm);
    try {
      std::filesystem::rename(staged, database);
      std::filesystem::remove(prior);
      std::filesystem::remove(prior_wal);
      std::filesystem::remove(prior_shm);
    } catch (...) {
      if (std::filesystem::exists(prior) && !std::filesystem::exists(database))
        std::filesystem::rename(prior, database);
      if (std::filesystem::exists(prior_wal)) std::filesystem::rename(prior_wal, wal);
      if (std::filesystem::exists(prior_shm)) std::filesystem::rename(prior_shm, shm);
      throw;
    }
    return {true, "restore verified and completed"};
  } catch (const std::exception &error) { return failure(error.what()); }
}

Result verify(const std::filesystem::path &database) {
  try {
    if (!std::filesystem::exists(database)) return failure("database does not exist");
    storage::Database opened(database);
    return verify_open(opened);
  } catch (const std::exception &error) { return failure(error.what()); }
}

Result compact(const std::filesystem::path &database) {
  try {
    if (!std::filesystem::exists(database)) return failure("database does not exist");
    storage::Database opened(database);
    const auto checked = verify_open(opened);
    if (!checked.ok) return checked;
    opened.execute("PRAGMA wal_checkpoint(TRUNCATE)");
    opened.execute("VACUUM");
    return verify_open(opened);
  } catch (const std::exception &error) { return failure(error.what()); }
}

int run(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "usage: pma-core <verify|compact> <database> | <backup|restore> <source> <destination>\n";
    return 2;
  }
  const std::string command = argv[1];
  Result result{false, "unknown maintenance command"};
  if (command == "verify" && argc == 3) result = verify(argv[2]);
  else if (command == "compact" && argc == 3) result = compact(argv[2]);
  else if (command == "backup" && argc == 4) result = backup(argv[2], argv[3]);
  else if (command == "restore" && argc == 4) result = restore(argv[2], argv[3]);
  else { std::cerr << "invalid maintenance arguments\n"; return 2; }
  (result.ok ? std::cout : std::cerr) << result.message << '\n';
  return result.ok ? 0 : 1;
}
} // namespace pma::maintenance
