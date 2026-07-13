#pragma once

#include "pma-internal/storage.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pma::vectors {

struct Profile {
  std::string id;
  std::string provider;
  std::string runtime_type;
  std::string model_hash;
  std::string model_revision;
  std::string tokenizer_hash;
  std::int64_t dimensions;
  std::string data_type{"float32-le"};
  std::string pooling{"mean"};
  bool normalized{true};
  std::string query_prefix;
  std::string document_prefix;
  std::int64_t max_input{512};
  std::string truncation{"end"};
  std::string renderer_version{"claim-v1"};
  std::string provider_options{"{}"};
  [[nodiscard]] std::string fingerprint() const;
};

struct RuntimeManifest {
  std::string id;
  std::string provider_package;
  std::string provider_version;
  std::string node_version;
  std::string os;
  std::string architecture;
  std::string lockfile_hash;
  std::string backend;
  std::string deterministic_settings;
};

class EmbeddingProvider {
public:
  virtual ~EmbeddingProvider() = default;
  virtual std::vector<float> embed(std::string_view text, const Profile &profile) = 0;
};

class FakeEmbeddingProvider final : public EmbeddingProvider {
public:
  std::vector<float> embed(std::string_view text, const Profile &profile) override;
  void fail_after(std::optional<std::size_t> calls);

private:
  std::optional<std::size_t> fail_after_;
  std::size_t calls_{};
};

struct StoreInfo {
  std::string id;
  std::string profile_id;
  std::string fingerprint;
  std::int64_t generation;
  std::string state;
  std::filesystem::path relative_path;
  std::int64_t watermark;
  std::int64_t vector_count;
};

struct ValidationIssue {
  std::string code;
  std::string object_id;
};

struct SearchResult {
  std::string document_id;
  float score;
};

class Manager {
public:
  Manager(storage::Database &core, std::filesystem::path bundle_root);
  void initialize(std::string_view build_version = "0.1.0");
  void register_profile(const Profile &profile, const RuntimeManifest &runtime);
  void render_claim(std::string_view claim_id, std::string_view operation_id,
                    std::string_view renderer_version = "claim-v1");
  void deactivate_document(std::string_view document_id, std::string_view operation_id);
  StoreInfo build(std::string_view store_id, std::string_view profile_id,
                  std::int64_t generation, std::string_view runtime_id,
                  EmbeddingProvider &provider);
  void catch_up(std::string_view store_id, EmbeddingProvider &provider,
                std::size_t batch_size = 100);
  [[nodiscard]] std::vector<ValidationIssue> validate(std::string_view store_id);
  void activate(std::string_view store_id);
  void retire(std::string_view store_id);
  [[nodiscard]] StoreInfo inspect(std::string_view store_id);
  [[nodiscard]] std::vector<SearchResult> search(std::string_view store_id,
                                                 const std::vector<float> &query,
                                                 std::size_t limit);
  [[nodiscard]] std::string reconstruction_plan(std::string_view store_id);

private:
  storage::Database *core_;
  std::filesystem::path bundle_root_;
};

} // namespace pma::vectors
