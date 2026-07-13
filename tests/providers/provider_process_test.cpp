#include "pma-internal/provider_process.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <future>
#include <string>
#include <thread>

#ifndef PMA_NODE_EXECUTABLE
#error PMA_NODE_EXECUTABLE must be defined
#endif
#ifndef PMA_FAKE_PROVIDER_SCRIPT
#error PMA_FAKE_PROVIDER_SCRIPT must be defined
#endif

TEST_CASE("provider process exchanges protocol and restarts after crash", "[provider]") {
  pma::providers::Process process({PMA_NODE_EXECUTABLE, PMA_FAKE_PROVIDER_SCRIPT});
  process.start();
  const auto health = nlohmann::json::parse(
      process.request(R"({"jsonrpc":"2.0","id":1,"method":"provider.health"})"));
  REQUIRE(health["result"]["status"] == "ok");

  REQUIRE_THROWS(process.request(R"({"jsonrpc":"2.0","id":2,"method":"crash"})"));
  for (int attempt = 0; attempt < 20 && process.running(); ++attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  process.restart();
  const auto restarted = nlohmann::json::parse(
      process.request(R"({"jsonrpc":"2.0","id":3,"method":"provider.health"})"));
  REQUIRE(restarted["result"]["status"] == "ok");
  process.stop();
}

TEST_CASE("provider process forwards cooperative cancellation", "[provider]") {
  pma::providers::Process process({PMA_NODE_EXECUTABLE, PMA_FAKE_PROVIDER_SCRIPT});
  process.start();
  auto pending = std::async(std::launch::async, [&process] {
    return process.request(R"({"jsonrpc":"2.0","id":"slow-1","method":"slow"})");
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  process.cancel("slow-1");
  REQUIRE(pending.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
  const auto response = nlohmann::json::parse(pending.get());
  REQUIRE(response["error"]["code"] == -32800);
  process.stop();
}
