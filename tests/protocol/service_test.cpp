#include "pma-internal/service.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
using Json = nlohmann::json;

Json send(pma::service::Dispatcher &dispatcher, const Json &request) {
  const auto response = dispatcher.handle_line(request.dump());
  REQUIRE(response.has_value());
  return Json::parse(*response);
}

Json read_fixture(std::string_view name) {
  std::ifstream input(std::filesystem::path(PMA_PROTOCOL_FIXTURE_DIR) / name);
  REQUIRE(input.good());
  return Json::parse(input);
}

Json initialize(pma::service::Dispatcher &dispatcher, int minimum = 1, int maximum = 1) {
  return send(dispatcher,
              {{"jsonrpc", "2.0"},
               {"id", 1},
               {"method", "pma.initialize"},
               {"params", {{"protocol_version", {{"min", minimum}, {"max", maximum}}},
                           {"client", {{"name", "test"}, {"version", "1"}}}}}});
}
} // namespace

TEST_CASE("initialization matches the golden protocol fixture", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  const auto request = read_fixture("initialize.request.json");
  const auto response = send(dispatcher, request);
  REQUIRE(response == read_fixture("initialize.response.json"));
}

TEST_CASE("initialization negotiates protocol and system methods", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  const auto initialized = initialize(dispatcher);
  REQUIRE(initialized["result"]["protocol_version"] == 1);
  REQUIRE(initialized["result"]["core_version"] == "0.1.0");

  const auto health = send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 2}, {"method", "pma.health"}});
  REQUIRE(health["result"]["status"] == "ok");
  const auto status =
      send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 3}, {"method", "pma.get_status"}});
  REQUIRE(status["result"]["state"] == "ready");
  const auto capabilities =
      send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 4}, {"method", "pma.get_capabilities"}});
  REQUIRE(capabilities["result"]["transport"] == "stdio-ndjson");
}

TEST_CASE("notifications and cancellation do not produce responses", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  const auto notification = dispatcher.handle_line(
      R"({"jsonrpc":"2.0","method":"$/cancelRequest","params":{"id":"work-1"}})");
  REQUIRE_FALSE(notification.has_value());
}

TEST_CASE("malformed JSON and envelopes have stable errors", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  const auto malformed = Json::parse(*dispatcher.handle_line("{"));
  REQUIRE(malformed["error"]["data"]["code"] == "PMA_PARSE_ERROR");
  REQUIRE(malformed["id"].is_null());

  const auto invalid = send(dispatcher, {{"jsonrpc", "1.0"}, {"id", 8}, {"method", "x"}});
  REQUIRE(invalid["error"]["data"]["code"] == "PMA_INVALID_REQUEST");
}

TEST_CASE("unknown methods and duplicate IDs are rejected", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  initialize(dispatcher);
  const auto unknown =
      send(dispatcher, {{"jsonrpc", "2.0"}, {"id", "unknown"}, {"method", "pma.missing"}});
  REQUIRE(unknown["error"]["data"]["code"] == "PMA_METHOD_NOT_FOUND");

  const auto duplicate =
      send(dispatcher, {{"jsonrpc", "2.0"}, {"id", "unknown"}, {"method", "pma.health"}});
  REQUIRE(duplicate["error"]["data"]["code"] == "PMA_DUPLICATE_REQUEST");
}

TEST_CASE("protocol mismatch is reported without internal details", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  const auto response = initialize(dispatcher, 2, 3);
  REQUIRE(response["error"]["data"]["code"] == "PMA_PROTOCOL_VERSION_MISMATCH");
  REQUIRE(response["error"]["data"]["category"] == "validation");
  REQUIRE_FALSE(response["error"]["data"]["retryable"].get<bool>());
}

TEST_CASE("Pi lifecycle methods persist evidence and provide provider-free recall", "[protocol]") {
  const auto root = std::filesystem::temp_directory_path() /
                    ("pma-service-pi-" + std::to_string(
                        std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(root);
  {
    pma::service::Dispatcher dispatcher;
    const auto initialized = send(
        dispatcher,
        {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "pma.initialize"},
         {"params", {{"protocol_version", {{"min", 1}, {"max", 1}}},
                     {"database_path", (root / "pma.sqlite").string()},
                     {"bundle_root", root.string()}}}});
    REQUIRE(initialized.contains("result"));
    REQUIRE(send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 2},
                              {"method", "pma.interaction.begin"},
                              {"params", {{"interaction_id", "pi:i1"},
                                          {"session_id", "pi:s1"}, {"prompt", "remember"}}}})
                ["result"]["accepted"]);
    REQUIRE(send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 3},
                              {"method", "pma.observe.event"},
                              {"params", {{"interaction_id", "pi:i1"},
                                          {"source_id", "pi:event:1"}, {"ordinal", 0},
                                          {"text", "durable source evidence"},
                                          {"idempotency_key", "pi:event:1"}}}})
                ["result"]["accepted"]);
    REQUIRE(send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 4},
                              {"method", "pma.interaction.complete"},
                              {"params", {{"interaction_id", "pi:i1"}}}})
                ["result"]["accepted"]);
    const auto learned = send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 5},
                                           {"method", "pma.learning.propose"},
                                           {"params", {{"statement", "SQLite remains authoritative"},
                                                       {"authority", "explicit_user"}}}});
    REQUIRE(learned["result"]["status"] == "active");
    const auto recall = send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 6},
                                          {"method", "pma.recall"},
                                          {"params", {{"packet_id", "pi:packet:1"},
                                                      {"interaction_id", "pi:i2"},
                                                      {"query", "SQLite authoritative"}}}});
    REQUIRE(recall["result"]["packet_id"] == "pi:packet:1");
    REQUIRE(recall["result"]["selected_count"] == 1);
    const auto corrected = send(
        dispatcher, {{"jsonrpc", "2.0"}, {"id", 7},
                     {"method", "pma.memory.propose_correction"},
                     {"params", {{"target", learned["result"]["claim_id"]},
                                 {"correction", "SQLite is authoritative structured storage"}}}});
    REQUIRE(corrected["result"]["status"] == "active");
  }
  std::filesystem::remove_all(root);
}

TEST_CASE("shutdown changes dispatcher lifecycle", "[protocol]") {
  pma::service::Dispatcher dispatcher;
  const auto response =
      send(dispatcher, {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "pma.shutdown"}});
  REQUIRE(response["result"]["accepted"]);
  REQUIRE(dispatcher.shutdown_requested());
}
