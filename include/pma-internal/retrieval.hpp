#pragma once

#include "pma-internal/storage.hpp"
#include "pma-internal/vectors.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace pma::retrieval {

struct ScoreComponents {
  double lexical{};
  double vector{};
  double entity_predicate{};
  double graph{};
  double context{};
  double temporal{};
  double authority{};
  double strength{};
  double utility{};
  double final_score{};
};

struct PacketItem {
  std::string claim_id;
  std::string text;
  std::string status;
  std::string authority;
  ScoreComponents scores;
  std::vector<std::string> reasons;
  bool conflict{};
  bool qualification{};
  std::size_t estimated_tokens{};
};

struct ContextPacket {
  std::string id;
  std::string interaction_id;
  std::string query_fingerprint;
  std::string policy_version;
  std::vector<PacketItem> candidates;
  std::vector<PacketItem> selected;
  std::optional<std::string> vector_profile_id;
  std::int64_t vector_watermark{};
};

struct RecallRequest {
  std::string packet_id;
  std::string interaction_id;
  std::string query;
  std::vector<std::string> active_context_ids;
  std::optional<std::string> vector_store_id;
  std::optional<std::vector<float>> query_vector;
  std::size_t candidate_limit{50};
  std::size_t selected_limit{10};
};

struct ContextSlice {
  std::string text;
  std::size_t estimated_tokens{};
  std::vector<std::string> included_claim_ids;
};

enum class Outcome { used, irrelevant, corrected, harmful, unknown };

class Engine {
public:
  Engine(storage::Database &database, vectors::Manager *vector_manager = nullptr);
  void initialize(std::string_view build_version = "0.1.0");
  void rebuild_fts();
  [[nodiscard]] ContextPacket recall(const RecallRequest &request);
  [[nodiscard]] ContextSlice render_slice(const ContextPacket &packet,
                                          std::size_t token_budget) const;
  void record_outcome(std::string_view packet_id, std::string_view claim_id,
                      Outcome outcome);
  [[nodiscard]] double utility(std::string_view claim_id);
  [[nodiscard]] static std::size_t estimate_tokens(std::string_view text);

private:
  storage::Database *database_;
  vectors::Manager *vector_manager_;
};

} // namespace pma::retrieval
