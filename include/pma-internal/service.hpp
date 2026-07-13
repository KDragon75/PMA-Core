#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace pma::service {

class Dispatcher {
public:
  Dispatcher();
  Dispatcher(Dispatcher &&) noexcept;
  Dispatcher &operator=(Dispatcher &&) noexcept;
  ~Dispatcher();
  Dispatcher(const Dispatcher &) = delete;
  Dispatcher &operator=(const Dispatcher &) = delete;

  // Returns no payload for valid JSON-RPC notifications.
  [[nodiscard]] std::optional<std::string> handle_line(std::string_view line);
  [[nodiscard]] bool shutdown_requested() const noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

int run_stdio();

} // namespace pma::service
