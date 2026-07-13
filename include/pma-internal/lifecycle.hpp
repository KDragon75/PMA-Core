#pragma once

#include "pma-internal/graph.hpp"
#include "pma-internal/storage.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pma::lifecycle {

struct ObservationEvent {
  std::string id;
  std::int64_t ordinal;
  std::string text;
  std::string idempotency_key;
  std::optional<std::string> observed_agent;
  std::optional<std::string> model_runtime;
};

struct Observation {
  std::string episode_id;
  std::string source_adapter;
  std::string external_key;
  std::string job_id;
  std::vector<ObservationEvent> events;
};

enum class OperationType {
  create,
  support,
  qualify,
  contradict,
  supersede,
  contextualize,
  deactivate,
  restore
};

struct Proposal {
  OperationType operation;
  std::string subject_id;
  std::string predicate_id;
  std::optional<std::string> object_entity_id;
  std::optional<std::string> literal_text;
  std::optional<std::string> target_claim_id;
  std::optional<std::string> context_id;
  std::vector<std::string> evidence_ids;
  std::string authority{"observed"};
  double confidence{0.8};
};

class ProviderError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class ProposalProvider {
public:
  virtual ~ProposalProvider() = default;
  virtual std::vector<Proposal> propose(const std::vector<graph::EvidenceSegment> &evidence) = 0;
};

class ScriptedProvider final : public ProposalProvider {
public:
  explicit ScriptedProvider(std::vector<Proposal> proposals);
  void fail_next(std::string message);
  std::vector<Proposal> propose(const std::vector<graph::EvidenceSegment> &evidence) override;

private:
  std::vector<Proposal> proposals_;
  std::optional<std::string> failure_;
};

enum class JobState { queued, running, succeeded, failed };

struct Job {
  std::string id;
  std::string episode_id;
  JobState state;
  std::int64_t attempts;
  std::int64_t checkpoint;
  std::optional<std::string> error_code;
};

class Processor {
public:
  Processor(storage::Database &database, graph::Store &graph_store);
  void initialize(std::string_view build_version = "0.1.0");
  void observe(const Observation &observation);
  [[nodiscard]] Job get_job(std::string_view id);
  void run_job(std::string_view id, ProposalProvider &provider, std::int64_t max_attempts = 3);
  void apply(std::string_view job_id, const std::vector<Proposal> &proposals);
  [[nodiscard]] std::string correct(std::string_view operation_id,
                                    std::string_view target_claim_id,
                                    std::string_view evidence_id,
                                    std::string_view replacement_text,
                                    double confidence = 1.0);

private:
  storage::Database *database_;
  graph::Store *graph_store_;
};

} // namespace pma::lifecycle
