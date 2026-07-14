#include "pma-internal/retrieval.hpp"
#include "pma-internal/storage.hpp"

#include <benchmark/benchmark.h>
#include <filesystem>
#include <string>

namespace {
std::filesystem::path temporary_path() {
  static std::size_t sequence = 0;
  return std::filesystem::temp_directory_path() / ("pma-benchmark-" + std::to_string(++sequence) + ".db");
}

static void BM_StorageEpisodeInsert(benchmark::State &state) {
  const auto path = temporary_path();
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + "-wal");
  std::filesystem::remove(path.string() + "-shm");
  pma::storage::Database database(path);
  database.execute("CREATE TABLE synthetic_episode(id INTEGER PRIMARY KEY, body TEXT NOT NULL) STRICT");
  std::int64_t id = 0;
  for (auto _ : state) {
    auto statement = database.prepare("INSERT INTO synthetic_episode(id, body) VALUES(?, ?)");
    statement.bind(1, ++id); statement.bind(2, std::string(static_cast<std::size_t>(state.range(0)), 'x'));
    statement.execute();
  }
  state.counters["core_items_per_second"] = benchmark::Counter(static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["provider_wait_ms"] = 0;
}
BENCHMARK(BM_StorageEpisodeInsert)->Arg(128)->Arg(4096);

static void BM_ContextTokenEstimate(benchmark::State &state) {
  const std::string text(static_cast<std::size_t>(state.range(0)), 'x');
  for (auto _ : state) benchmark::DoNotOptimize(pma::retrieval::Engine::estimate_tokens(text));
  state.counters["core_items_per_second"] = benchmark::Counter(static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["provider_wait_ms"] = 0;
}
BENCHMARK(BM_ContextTokenEstimate)->Arg(1024)->Arg(16384);
}
BENCHMARK_MAIN();
