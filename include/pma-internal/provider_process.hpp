#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pma::providers {

class Process {
public:
  explicit Process(std::vector<std::string> command);
  Process(Process &&) noexcept;
  Process &operator=(Process &&) noexcept;
  ~Process();
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;

  void start();
  void stop();
  void restart();
  [[nodiscard]] bool running() const;
  [[nodiscard]] std::string request(std::string_view json_line);
  void cancel(std::string_view request_id);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace pma::providers
