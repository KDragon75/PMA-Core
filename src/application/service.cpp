#include "pma-internal/service.hpp"

#include <nlohmann/json.hpp>

#include <set>
#include <string>
#include <utility>

namespace pma::service {
namespace {

using Json = nlohmann::json;
constexpr int protocol_version = 1;
constexpr std::string_view core_version = "0.1.0";

struct PublicError {
  int rpc_code;
  std::string code;
  std::string category;
  std::string message;
  bool retryable{false};
  Json details = Json::object();
};

Json error_response(const Json &id, const PublicError &error) {
  return {{"jsonrpc", "2.0"},
          {"id", id},
          {"error",
           {{"code", error.rpc_code},
            {"message", error.message},
            {"data",
             {{"code", error.code},
              {"category", error.category},
              {"retryable", error.retryable},
              {"details", error.details}}}}}};
}

PublicError invalid_request(std::string message) {
  return {-32600, "PMA_INVALID_REQUEST", "validation", std::move(message)};
}

PublicError invalid_params(std::string message) {
  return {-32602, "PMA_INVALID_PARAMS", "validation", std::move(message)};
}

Json capabilities() {
  return {{"methods",
           Json::array({"pma.initialize", "pma.health", "pma.get_capabilities",
                        "pma.get_status", "pma.shutdown", "$/cancelRequest"})},
          {"notifications", Json::array({"$/cancelRequest"})},
          {"transport", "stdio-ndjson"}};
}

} // namespace

struct Dispatcher::Impl {
  bool initialized{false};
  bool shutdown{false};
  std::set<std::string> completed_ids;
  std::set<std::string> cancelled_ids;

  Json dispatch(std::string_view method, const Json &params) {
    if (method == "pma.initialize") {
      if (!params.is_object() || !params.contains("protocol_version") ||
          !params["protocol_version"].is_object()) {
        throw invalid_params("protocol_version range is required");
      }
      const auto &range = params["protocol_version"];
      if (!range.contains("min") || !range.contains("max") || !range["min"].is_number_integer() ||
          !range["max"].is_number_integer()) {
        throw invalid_params("protocol_version.min and protocol_version.max must be integers");
      }
      const int minimum = range["min"].get<int>();
      const int maximum = range["max"].get<int>();
      if (minimum > maximum || minimum > protocol_version || maximum < protocol_version) {
        throw PublicError{-32001,
                          "PMA_PROTOCOL_VERSION_MISMATCH",
                          "validation",
                          "no compatible PMA protocol version",
                          false,
                          {{"server_min", protocol_version}, {"server_max", protocol_version}}};
      }
      initialized = true;
      return {{"protocol_version", protocol_version},
              {"core_version", core_version},
              {"core_schema_version", 0},
              {"vector_schema_version", 0},
              {"capabilities", capabilities()},
              {"provider_availability", Json::array()},
              {"vector_store", {{"status", "not_configured"}}},
              {"feature_flags", Json::object()}};
    }
    if (method == "pma.health") {
      return {{"status", "ok"}, {"initialized", initialized}};
    }
    if (method == "$/cancelRequest") {
      if (!params.is_object() || !params.contains("id") ||
          !(params["id"].is_string() || params["id"].is_number_integer())) {
        throw invalid_params("cancellation id must be a string or integer");
      }
      cancelled_ids.insert(params["id"].dump());
      return Json::object();
    }
    if (method == "pma.shutdown") {
      shutdown = true;
      return {{"accepted", true}};
    }
    if (method != "pma.get_capabilities" && method != "pma.get_status") {
      throw PublicError{-32601, "PMA_METHOD_NOT_FOUND", "not_found", "method is not supported"};
    }
    if (!initialized) {
      throw PublicError{-32002, "PMA_NOT_INITIALIZED", "conflict",
                        "pma.initialize must complete before this method"};
    }
    if (method == "pma.get_capabilities") {
      return capabilities();
    }
    if (method == "pma.get_status") {
      return {{"state", "ready"},
              {"protocol_version", protocol_version},
              {"core_version", core_version},
              {"cancelled_request_count", cancelled_ids.size()}};
    }
    throw PublicError{-32603, "PMA_INTERNAL", "internal", "internal PMA error"};
  }
};

Dispatcher::Dispatcher() : impl_(std::make_unique<Impl>()) {}
Dispatcher::Dispatcher(Dispatcher &&) noexcept = default;
Dispatcher &Dispatcher::operator=(Dispatcher &&) noexcept = default;
Dispatcher::~Dispatcher() = default;

std::optional<std::string> Dispatcher::handle_line(std::string_view line) {
  Json request;
  try {
    request = Json::parse(line);
  } catch (const Json::parse_error &) {
    return error_response(nullptr,
                          {-32700, "PMA_PARSE_ERROR", "validation", "invalid JSON input"})
        .dump();
  }

  Json id = request.is_object() && request.contains("id") ? request["id"] : Json(nullptr);
  const bool notification = request.is_object() && !request.contains("id");
  try {
    if (!request.is_object() || request.value("jsonrpc", "") != "2.0" ||
        !request.contains("method") || !request["method"].is_string()) {
      throw invalid_request("a JSON-RPC 2.0 object with a string method is required");
    }
    if (request.contains("id") &&
        !(request["id"].is_string() || request["id"].is_number_integer())) {
      throw invalid_request("request id must be a string or integer");
    }
    if (request.contains("params") && !request["params"].is_object() &&
        !request["params"].is_array()) {
      throw invalid_params("params must be an object or array");
    }
    if (!notification) {
      const std::string key = id.dump();
      if (impl_->completed_ids.contains(key)) {
        throw PublicError{-32003, "PMA_DUPLICATE_REQUEST", "conflict",
                          "request id has already completed"};
      }
      impl_->completed_ids.insert(key);
    }
    const Json params = request.value("params", Json::object());
    Json result = impl_->dispatch(request["method"].get<std::string>(), params);
    if (notification) {
      return std::nullopt;
    }
    return Json{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}}.dump();
  } catch (const PublicError &error) {
    if (notification) {
      return std::nullopt;
    }
    return error_response(id, error).dump();
  } catch (...) {
    if (notification) {
      return std::nullopt;
    }
    return error_response(id, {-32603, "PMA_INTERNAL", "internal", "internal PMA error"}).dump();
  }
}

bool Dispatcher::shutdown_requested() const noexcept { return impl_->shutdown; }

} // namespace pma::service
