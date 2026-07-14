#pragma once

#include <filesystem>
#include <string>

namespace pma::maintenance {

struct Result {
  bool ok;
  std::string message;
};

[[nodiscard]] Result backup(const std::filesystem::path &database,
                            const std::filesystem::path &destination);
[[nodiscard]] Result restore(const std::filesystem::path &backup,
                             const std::filesystem::path &database);
[[nodiscard]] Result verify(const std::filesystem::path &database);
[[nodiscard]] Result compact(const std::filesystem::path &database);
int run(int argc, char **argv);

} // namespace pma::maintenance
