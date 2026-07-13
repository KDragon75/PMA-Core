#pragma once

#include "pma-internal/storage.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pma::graph {

struct Episode {
  std::string id;
  std::string source_adapter;
  std::string external_key;
};

struct EvidenceSegment {
  std::string id;
  std::string episode_id;
  std::int64_t ordinal;
  std::string text;
  std::optional<std::string> observed_agent;
  std::optional<std::string> model_runtime;
};

struct Entity {
  std::string id;
  std::string canonical_name;
  std::string category;
};

struct Claim {
  std::string id;
  std::string subject_id;
  std::string predicate_id;
  std::optional<std::string> object_entity_id;
  std::optional<std::string> typed_value_id;
  std::string polarity;
  std::string modality;
  std::string authority;
  double confidence;
  std::string status;
  std::string source_kind;
};

struct Context {
  std::string id;
  std::string type;
  std::string external_key;
};

struct Resource {
  std::string id;
  std::string logical_name;
  std::optional<std::string> repository_identity;
  std::optional<std::string> repository_relative_path;
};

struct Predicate {
  std::string id;
  std::string name;
  std::string description;
  std::string subject_category;
  std::string object_category;
  std::string status;
};

struct ResourceLocation {
  std::string id;
  std::string resource_id;
  std::string kind;
  std::optional<std::string> environment_id;
  std::string locator;
};

struct Procedure {
  std::string id;
  std::string name;
};

struct Goal {
  std::string id;
  std::string description;
  std::string status;
};

struct Edge {
  std::string id;
  std::string source_id;
  std::string target_id;
  std::string edge_class;
  std::string relation;
};

struct ProcedureStep {
  std::string id;
  std::string procedure_id;
  std::int64_t ordinal;
  std::string instruction;
};

struct GraphIssue {
  std::string code;
  std::string object_id;
  std::string message;
};

class Store {
public:
  explicit Store(storage::Database &database);
  void initialize(std::string_view build_version = "0.1.0");

  void create_episode(std::string_view id, std::string_view source_adapter,
                      std::string_view external_key);
  [[nodiscard]] Episode get_episode(std::string_view id);
  EvidenceSegment append_evidence(std::string_view id, std::string_view episode_id,
                                  std::int64_t ordinal, std::string_view text,
                                  std::string_view idempotency_key,
                                  std::optional<std::string_view> observed_agent = std::nullopt,
                                  std::optional<std::string_view> model_runtime = std::nullopt);
  [[nodiscard]] EvidenceSegment get_evidence(std::string_view id);
  [[nodiscard]] std::vector<EvidenceSegment> list_evidence(std::string_view episode_id);
  [[nodiscard]] std::vector<EvidenceSegment> search_evidence(std::string_view text);

  void create_entity(std::string_view id, std::string_view canonical_name,
                     std::string_view category);
  [[nodiscard]] Entity get_entity(std::string_view id);
  [[nodiscard]] std::vector<Entity> search_entities(std::string_view name);
  void add_alias(std::string_view id, std::string_view entity_id, std::string_view alias);

  void create_context(std::string_view id, std::string_view type,
                      std::string_view external_key);
  [[nodiscard]] Context get_context(std::string_view id);
  void create_resource(std::string_view id, std::string_view logical_name,
                       std::optional<std::string_view> repository_identity = std::nullopt,
                       std::optional<std::string_view> repository_relative_path = std::nullopt);
  [[nodiscard]] Resource get_resource(std::string_view id);
  void add_resource_location(std::string_view id, std::string_view resource_id,
                             std::string_view kind, std::string_view locator,
                             std::optional<std::string_view> environment_id = std::nullopt);
  [[nodiscard]] std::vector<ResourceLocation> resource_locations(std::string_view resource_id);
  [[nodiscard]] Predicate get_predicate(std::string_view id);

  void create_typed_text(std::string_view id, std::string_view value);
  void create_claim(const Claim &claim);
  [[nodiscard]] Claim get_claim(std::string_view id);
  void link_claim_evidence(std::string_view claim_id, std::string_view evidence_id,
                           std::string_view relation);
  void add_edge(std::string_view id, std::string_view source_id, std::string_view target_id,
                std::string_view edge_class, std::string_view relation);
  [[nodiscard]] Edge get_edge(std::string_view id);

  void create_procedure(std::string_view id, std::string_view name);
  [[nodiscard]] Procedure get_procedure(std::string_view id);
  void add_procedure_step(std::string_view id, std::string_view procedure_id,
                          std::int64_t ordinal, std::string_view instruction);
  [[nodiscard]] std::vector<ProcedureStep> procedure_steps(std::string_view procedure_id);
  void create_goal(std::string_view id, std::string_view description, std::string_view status);
  [[nodiscard]] Goal get_goal(std::string_view id);

  [[nodiscard]] std::vector<GraphIssue> verify() const;

private:
  storage::Database *database_;
};

} // namespace pma::graph
