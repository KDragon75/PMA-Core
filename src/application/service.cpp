#include "pma-internal/service.hpp"

#include "pma-internal/graph.hpp"
#include "pma-internal/lifecycle.hpp"
#include "pma-internal/provider_process.hpp"
#include "pma-internal/retrieval.hpp"
#include "pma-internal/storage.hpp"
#include "pma-internal/vectors.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
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

class ProcessEmbeddingProvider final : public vectors::EmbeddingProvider,
                                       public lifecycle::ProposalProvider {
public:
  ProcessEmbeddingProvider(std::vector<std::string> command, Json config)
      : process_(std::move(command)), config_(std::move(config)) {
    process_.start();
    const auto response = call("provider.initialize", {{"config", config_}});
    runtime_ = response.at("runtime");
  }

  std::vector<float> embed(std::string_view text, const vectors::Profile &) override {
    return vectors_from(call("embedding.embed_documents", {{"texts", Json::array({text})}})
                            .at("vectors").at(0));
  }

  std::vector<float> embed_query(std::string_view text) {
    return vectors_from(call("embedding.embed_query", {{"text", text}}).at("vector"));
  }

  std::vector<lifecycle::Proposal>
  propose(const std::vector<graph::EvidenceSegment> &evidence) override {
    try {
    std::string task = "Extract only durable, reusable facts, preferences, decisions, procedures, or corrections "
                       "explicitly supported by this evidence. Omit transient conversation and speculation. Evidence:\n";
    for (const auto &item : evidence) task += "[" + item.id + "] " + item.text + "\n";
    const Json schema = {{"type", "object"},
                         {"additionalProperties", false},
                         {"required", Json::array({"memories"})},
                         {"properties", {{"memories", {{"type", "array"}, {"items", {
                           {"type", "object"}, {"additionalProperties", false},
                           {"required", Json::array({"statement", "evidence_ids", "confidence"})},
                           {"properties", {
                             {"statement", {{"type", "string"}, {"minLength", 1}}},
                             {"evidence_ids", {{"type", "array"}, {"items", {{"type", "string"}}}, {"minItems", 1}}},
                             {"confidence", {{"type", "number"}, {"minimum", 0}, {"maximum", 1}}}
                           }}
                         }}}}}}};
    const auto result = call("generation.generate_structured", {{"task", task}, {"schema", schema}});
    std::set<std::string> allowed;
    for (const auto &item : evidence) allowed.insert(item.id);
    std::vector<lifecycle::Proposal> proposals;
    for (const auto &memory : result.at("value").at("memories")) {
      std::vector<std::string> ids;
      for (const auto &id : memory.at("evidence_ids")) {
        const auto value = id.get<std::string>();
        if (allowed.contains(value)) ids.push_back(value);
      }
      if (ids.empty()) continue;
      proposals.push_back({lifecycle::OperationType::create, "entity:learned-memory",
                           "pred:has_property", std::nullopt,
                           memory.at("statement").get<std::string>(), std::nullopt,
                           std::nullopt, std::move(ids), "observed",
                           memory.at("confidence").get<double>()});
    }
    return proposals;
    } catch (const std::exception &error) {
      throw lifecycle::ProviderError(error.what());
    }
  }

  const Json &runtime() const { return runtime_; }

private:
  Json call(std::string_view method, Json params) {
    const auto id = next_id_++;
    const auto response = Json::parse(process_.request(
        Json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}}.dump()));
    if (response.contains("error")) throw std::runtime_error("provider request failed");
    return response.at("result");
  }

  static std::vector<float> vectors_from(const Json &values) {
    std::vector<float> result;
    result.reserve(values.size());
    for (const auto &value : values) result.push_back(value.get<float>());
    return result;
  }

  providers::Process process_;
  Json config_;
  Json runtime_;
  std::int64_t next_id_{1};
};

Json capabilities() {
  return {{"methods",
           Json::array({"pma.initialize", "pma.health", "pma.get_capabilities",
                        "pma.get_status", "pma.vectors.sync", "pma.shutdown", "$/cancelRequest"})},
          {"notifications", Json::array({"$/cancelRequest"})},
          {"transport", "stdio-ndjson"}};
}

} // namespace

struct Dispatcher::Impl {
  struct PendingInteraction {
    std::string session_id;
    std::string prompt;
    std::vector<lifecycle::ObservationEvent> events;
  };

  bool initialized{false};
  bool shutdown{false};
  std::set<std::string> completed_ids;
  std::set<std::string> cancelled_ids;
  std::unique_ptr<storage::Database> database;
  std::unique_ptr<graph::Store> graph_store;
  std::unique_ptr<lifecycle::Processor> lifecycle_processor;
  std::unique_ptr<vectors::Manager> vector_manager;
  std::unique_ptr<retrieval::Engine> retrieval_engine;
  std::unique_ptr<ProcessEmbeddingProvider> provider;
  std::optional<std::string> active_vector_store;
  std::string provider_status{"not_configured"};
  std::string provider_error;
  std::unordered_map<std::string, PendingInteraction> interactions;

  void initialize_runtime(const Json &params) {
    if (!params.contains("database_path") || !params["database_path"].is_string()) {
      return;
    }
    const std::filesystem::path database_path = params["database_path"].get<std::string>();
    const std::filesystem::path bundle_root = params.value("bundle_root", database_path.parent_path().string());
    std::filesystem::create_directories(database_path.parent_path());
    database = std::make_unique<storage::Database>(database_path);
    graph_store = std::make_unique<graph::Store>(*database);
    lifecycle_processor = std::make_unique<lifecycle::Processor>(*database, *graph_store);
    lifecycle_processor->initialize(core_version);
    vector_manager = std::make_unique<vectors::Manager>(*database, bundle_root);
    vector_manager->initialize(core_version);
    retrieval_engine = std::make_unique<retrieval::Engine>(*database, vector_manager.get());
    retrieval_engine->initialize(core_version);

    auto learned = database->prepare("SELECT 1 FROM entities WHERE id='entity:learned-memory'");
    if (!learned.step()) graph_store->create_entity("entity:learned-memory", "Learned memory", "concept");

    provider.reset();
    active_vector_store.reset();
    provider_status = "not_configured";
    provider_error.clear();
    if (params.contains("provider") && params["provider"].is_object()) {
      try {
        const auto &configuration = params["provider"];
        const auto command = configuration.at("command").get<std::vector<std::string>>();
        provider = std::make_unique<ProcessEmbeddingProvider>(command, configuration.at("config"));
        const auto &runtime = provider->runtime();
        const auto fingerprint_source = runtime.dump();
        const std::string suffix = storage::sha256(fingerprint_source).substr(0, 24);
        vectors::RuntimeManifest manifest{
            "runtime:" + suffix,
            runtime.value("providerPackage", "unknown"), runtime.value("providerVersion", "unknown"),
            runtime.value("nodeVersion", "unknown"), runtime.value("os", "unknown"),
            runtime.value("architecture", "unknown"), runtime.value("lockfileHash", "unknown"),
            runtime.value("backend", "unknown"), runtime.value("optionsHash", "unknown")};
        const auto &embedding = runtime.at("embedding");
        const auto &model = runtime.at("model");
        vectors::Profile profile{
            "profile:" + suffix, runtime.value("providerPackage", "unknown"),
            runtime.value("runtimeType", "unknown"), model.value("artifactHash", "unknown"),
            model.value("revision", "unknown"), model.value("tokenizerHash", "unknown"),
            embedding.at("dimensions").get<std::int64_t>(), "float32-le",
            embedding.value("pooling", "mean"), embedding.value("normalize", true),
            embedding.value("queryPrefix", ""), embedding.value("documentPrefix", ""),
            embedding.value("maxTokens", 512), embedding.value("truncation", "end"),
            "claim-v1", runtime.value("optionsHash", "unknown")};
        vector_manager->register_profile(profile, manifest);
        provider_status = "ready";
      } catch (const std::exception &error) {
        provider.reset();
        provider_status = "unavailable";
        provider_error = error.what();
      }
    }

    auto active = database->prepare(
        "SELECT id FROM vector_store_registry WHERE state='active' ORDER BY generation DESC LIMIT 1");
    if (active.step()) active_vector_store = active.column_text(0);
  }

  Json sync_vectors() {
    if (!provider || !vector_manager || !database) {
      return {{"status", provider_status}, {"synchronized", false}};
    }
    try {
      if (lifecycle_processor) {
        auto queued = database->prepare(
            "SELECT id FROM jobs WHERE state='queued' ORDER BY created_at,id LIMIT 10");
        std::vector<std::string> job_ids;
        while (queued.step()) job_ids.push_back(queued.column_text(0));
        for (const auto &job_id : job_ids) lifecycle_processor->run_job(job_id, *provider);
      }
      const std::string profile_id = "profile:" + storage::sha256(provider->runtime().dump()).substr(0, 24);
      auto active = database->prepare(
          "SELECT id FROM vector_store_registry WHERE profile_id=?1 AND state='active' "
          "ORDER BY generation DESC LIMIT 1");
      active.bind(1, profile_id);
      if (active.step()) {
        active_vector_store = active.column_text(0);
        vector_manager->catch_up(*active_vector_store, *provider);
      } else {
        auto generation = database->prepare(
            "SELECT COALESCE(max(generation),0)+1 FROM vector_store_registry WHERE profile_id=?1");
        generation.bind(1, profile_id);
        (void)generation.step();
        const auto number = generation.column_int64(0);
        const std::string store_id = profile_id + ":generation:" + std::to_string(number);
        const std::string runtime_id = "runtime:" + storage::sha256(provider->runtime().dump()).substr(0, 24);
        vector_manager->build(store_id, profile_id, number, runtime_id, *provider);
        vector_manager->activate(store_id);
        active_vector_store = store_id;
      }
      provider_status = "ready";
      const auto info = vector_manager->inspect(*active_vector_store);
      return {{"status", "ready"}, {"synchronized", true}, {"store_id", info.id},
              {"watermark", info.watermark}, {"vector_count", info.vector_count}};
    } catch (const std::exception &error) {
      provider_status = "degraded";
      provider_error = error.what();
      return {{"status", "degraded"}, {"synchronized", false}};
    }
  }

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
      initialize_runtime(params);
      initialized = true;
      return {{"protocol_version", protocol_version},
              {"core_version", core_version},
              {"core_schema_version", 400},
              {"vector_schema_version", 1},
              {"capabilities", capabilities()},
              {"provider_availability", Json::array({provider_status})},
              {"vector_store", {{"status", active_vector_store ? "active" : "not_configured"}}},
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
    if (method == "pma.session.open" || method == "pma.session.close") {
      return {{"accepted", true}};
    }
    if (method == "pma.interaction.begin") {
      if (!lifecycle_processor || !params.contains("interaction_id")) {
        return {{"accepted", false}, {"degraded", true}};
      }
      interactions[params["interaction_id"].get<std::string>()] = {
          params.value("session_id", "pi:unknown"), params.value("prompt", ""), {}};
      return {{"accepted", true}};
    }
    if (method == "pma.observe.event") {
      if (!lifecycle_processor) {
        return {{"accepted", false}, {"degraded", true}};
      }
      const std::string source_id = params.value("source_id", "event:unknown");
      const std::string interaction_id = params.value("interaction_id", "");
      lifecycle::ObservationEvent event{source_id,
                                        params.value("ordinal", 0),
                                        params.value("text", ""),
                                        params.value("idempotency_key", source_id),
                                        std::nullopt,
                                        std::nullopt};
      if (!interaction_id.empty() && interactions.contains(interaction_id)) {
        interactions[interaction_id].events.push_back(std::move(event));
      } else {
        lifecycle::Observation observation{"episode:" + source_id, "pi", source_id,
                                            "job:" + source_id, {std::move(event)}};
        lifecycle_processor->observe(observation);
      }
      return {{"accepted", true}};
    }
    if (method == "pma.interaction.complete") {
      const std::string interaction_id = params.value("interaction_id", "");
      auto found = interactions.find(interaction_id);
      if (!lifecycle_processor || found == interactions.end()) {
        return {{"accepted", false}, {"degraded", true}};
      }
      if (found->second.events.empty() && !found->second.prompt.empty()) {
        found->second.events.push_back({interaction_id + ":prompt", 0, found->second.prompt,
                                        interaction_id + ":prompt", std::nullopt, std::nullopt});
      }
      lifecycle_processor->observe({"episode:" + interaction_id, "pi", interaction_id,
                                    "job:" + interaction_id, found->second.events});
      interactions.erase(found);
      return {{"accepted", true}, {"job_id", "job:" + interaction_id}};
    }
    if (method == "pma.learning.process_interaction") {
      if (!lifecycle_processor || !provider) {
        return {{"queued", true}, {"processed", false}, {"provider_status", provider_status}};
      }
      const std::string interaction_id = params.value("interaction_id", "");
      if (interaction_id.empty()) throw invalid_params("interaction_id is required");
      try {
        lifecycle_processor->run_job("job:" + interaction_id, *provider);
        return {{"queued", false}, {"processed", true}};
      } catch (const std::exception &) {
        provider_status = "degraded";
        return {{"queued", true}, {"processed", false}, {"provider_status", provider_status}};
      }
    }
    if (method == "pma.learning.consolidate") return {{"queued", true}};
    if (method == "pma.vectors.sync") return sync_vectors();
    if (method == "pma.learning.propose") {
      if (!database || !graph_store || !lifecycle_processor ||
          !params.contains("statement") || !params["statement"].is_string()) {
        throw invalid_params("statement is required");
      }
      const std::string statement = params["statement"].get<std::string>();
      const std::string key = storage::sha256(statement).substr(0, 24);
      const std::string claim_id = "explicit:" + key + ":0:claim";
      auto existing = database->prepare("SELECT 1 FROM claims WHERE id=?1");
      existing.bind(1, claim_id);
      if (!existing.step()) {
        auto entity = database->prepare("SELECT 1 FROM entities WHERE id='entity:explicit-memory'");
        if (!entity.step()) {
          graph_store->create_entity("entity:explicit-memory", "Explicit memory", "concept");
        }
        lifecycle::Observation observation{
            "episode:explicit:" + key, "pi", "explicit:" + key, "explicit:" + key,
            {{"evidence:explicit:" + key, 0, statement, "explicit:" + key,
              std::nullopt, std::nullopt}}};
        lifecycle_processor->observe(observation);
        lifecycle::Proposal proposal{lifecycle::OperationType::create,
                                     "entity:explicit-memory",
                                     "pred:has_property",
                                     std::nullopt,
                                     statement,
                                     std::nullopt,
                                     std::nullopt,
                                     {"evidence:explicit:" + key},
                                     params.value("authority", "explicit_user"),
                                     1.0};
        lifecycle_processor->apply("explicit:" + key, {proposal});
      }
      return {{"claim_id", claim_id}, {"status", "active"}};
    }
    if (method == "pma.memory.propose_correction") {
      if (!lifecycle_processor || !params.contains("target") ||
          !params.contains("correction")) {
        throw invalid_params("target and correction are required");
      }
      const std::string target = params["target"].get<std::string>();
      const std::string correction = params["correction"].get<std::string>();
      const std::string key = storage::sha256(target + correction).substr(0, 24);
      lifecycle::Observation observation{
          "episode:correction:" + key, "pi", "correction:" + key,
          "correction-observe:" + key,
          {{"evidence:correction:" + key, 0, correction, "correction:" + key,
            std::nullopt, std::nullopt}}};
      lifecycle_processor->observe(observation);
      const auto claim_id = lifecycle_processor->correct(
          "correction:" + key, target, "evidence:correction:" + key, correction, 1.0);
      return {{"claim_id", claim_id}, {"supersedes", target}, {"status", "active"}};
    }
    if (method == "pma.recall" || method == "pma.memory.search") {
      if (!retrieval_engine) {
        return {{"packet_id", params.value("packet_id", "")}, {"slice", ""}, {"degraded", true}};
      }
      retrieval_engine->rebuild_fts();
      retrieval::RecallRequest request{params.value("packet_id", "packet:rpc"),
                                       params.value("interaction_id", "interaction:rpc"),
                                       params.value("query", params.value("text", ""))};
      if (params.contains("selected_limit") && params["selected_limit"].is_number_integer()) {
        request.selected_limit = params["selected_limit"].get<std::size_t>();
      }
      if (provider && active_vector_store) {
        try {
          request.vector_store_id = *active_vector_store;
          request.query_vector = provider->embed_query(request.query);
        } catch (const std::exception &) {
          request.vector_store_id.reset();
          request.query_vector.reset();
          provider_status = "degraded";
        }
      }
      const auto packet = retrieval_engine->recall(request);
      const auto slice = retrieval_engine->render_slice(
          packet, params.value("token_budget", static_cast<std::size_t>(600)));
      return {{"packet_id", packet.id}, {"slice", slice.text},
              {"selected_count", packet.selected.size()}};
    }
    if (method == "pma.memory.get") {
      if (!graph_store || !params.contains("id")) {
        throw invalid_params("memory id is required");
      }
      const auto claim = graph_store->get_claim(params["id"].get<std::string>());
      return {{"id", claim.id}, {"subject_id", claim.subject_id},
              {"predicate_id", claim.predicate_id}, {"status", claim.status},
              {"authority", claim.authority}, {"confidence", claim.confidence}};
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
      Json vector = {{"status", active_vector_store ? "active" : "not_configured"}};
      if (active_vector_store && vector_manager) {
        const auto info = vector_manager->inspect(*active_vector_store);
        vector = {{"status", info.state}, {"store_id", info.id}, {"profile_id", info.profile_id},
                  {"watermark", info.watermark}, {"vector_count", info.vector_count}};
      }
      std::int64_t pending_count = 0;
      if (database) {
        auto pending = database->prepare("SELECT count(*) FROM jobs WHERE state IN ('queued','failed')");
        (void)pending.step();
        pending_count = pending.column_int64(0);
      }
      return {{"state", provider_status == "degraded" ? "degraded" : "ready"},
              {"protocol_version", protocol_version},
              {"core_version", core_version},
              {"core_schema_version", 400}, {"vector_schema_version", 1},
              {"provider", {{"status", provider_status}}}, {"vector_store", vector},
              {"pending_learning_jobs", pending_count},
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
