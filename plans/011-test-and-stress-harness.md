# Task 011 test and stress harness

## Objective
Provide reproducible performance measurements and a deterministic hidden-world long-run harness with captured replay, host modes, fault recovery checks, metrics, reports, and scheduled CI tiers.

## Governing documents
- [Task 011](../tasks/011-TEST-AND-STRESS-HARNESS.md)
- [Test strategy](../docs/06-testing/TEST_STRATEGY.md)
- [Performance tests](../docs/06-testing/PERFORMANCE_TESTS.md)
- [LLM stress test](../docs/06-testing/LLM_STRESS_TEST.md)
- [ADR-0007](../docs/decisions/ADR-0007-DETERMINISTIC-TESTS.md)

## Constraints
- Deterministic hidden state, evidence, replay, and recovery assertions remain authoritative; LLM grades are supplemental.
- Core and provider timing must be reported separately.
- Normal tests require no network or model download.
- Implement Task 011 only; do not begin release packaging or Task 012.
- Preserve the existing dirty `.pi/sessions/pma-core-build-session.jsonl` file without editing or committing it.

## Current state
- Tasks 001–010 are committed on `main`; Windows Debug/Release suites previously passed 52 CTest cases and all four Node workspaces pass.
- Google Benchmark is already a pinned vcpkg dependency, but no benchmark target or synthetic generator exists.
- `tests/llm-simulation` is an offline one-test placeholder.
- CI runs pull-request C++ and Node suites, but has no nightly or weekly workflows.
- The working tree contains only the pre-existing modified Pi session transcript.

## Work sequence
1. Add deterministic C++ synthetic datasets and Google Benchmark coverage for storage and retrieval/reportable core operations.
2. Implement the TypeScript hidden-world controller, role boundary, two host modes, captured replay, scenarios, metrics, fault injection, and report serialization.
3. Add deterministic tests for replay, hidden-state/grade separation, stale and irrelevant regression metrics, provider/core timing, host parity, and expected fault recovery.
4. Add nightly and weekly scheduled workflows with bounded and long simulation tiers plus benchmark artifacts.
5. Run narrow suites, then all acceptance suites; update this plan, commit, and push.

## Tests
- `cmake --preset windows-msvc-debug`
- `cmake --build --preset windows-msvc-debug`
- `ctest --preset windows-msvc-debug`
- `cmake --preset windows-msvc-release`
- `cmake --build --preset windows-msvc-release`
- `ctest --preset windows-msvc-release`
- Benchmark smoke invocation with JSON report output.
- `npm --prefix providers test`
- `npm --prefix adapters/pi test`
- `npm --prefix adapters/mcp test`
- `npm --prefix tests/llm-simulation test`
- Simulation CLI live run followed by captured replay.
- `git diff --check`

## Risks and rollback
Long simulations can be expensive and benchmark runners are noisy. PR tests use deterministic small tiers; scheduled workflows retain reports without noisy hard failure thresholds. Harness additions are isolated and can be rolled back without changing persisted schemas or service contracts.

## Progress
- [x] Governing documents, plans 001–010, and repository state reviewed.
- [x] C++ synthetic generators and benchmarks implemented.
- [x] Hidden-world simulation and replay implemented.
- [x] Fault/metric acceptance tests implemented.
- [x] Nightly and weekly workflows implemented.
- [x] Full local acceptance suite passed and plan finalized for the Task 011 commit.

## Decisions made during execution
- Captured replay uses `pma-captured-replay-v1` and records role, model, prompt version, content, and provider latency for every simulated role output.
- Scheduled reports enforce deterministic correctness and broad stale/irrelevant limits; benchmark trends are retained as artifacts rather than failing on noisy single-run timing changes.

## Completion evidence
- Windows MSVC Debug and Release configured, built, and passed 52/52 CTest cases each.
- `pma-benchmark` ran storage inserts at two payload sizes and ContextSlice token estimation at two sizes; its JSON report separates core counters from a zero provider-wait counter.
- The simulation workspace passed 5/5 tests covering all eleven scenarios, captured replay, both host modes, hidden-state assertions separate from grades, regression metrics, and controlled fault recovery.
- A 100-interaction live run and captured replay both completed with zero deterministic failures, stale-memory injection, irrelevant injection, or unexplained data loss; provider wait and core time were reported separately.
- Provider tests passed 4/4, Pi tests 4/4, and MCP tests 5/5.
- Nightly runs 1,000 interactions; weekly runs 10,000 interactions in both host modes and publishes Google Benchmark JSON.
- `git diff --check` passed. Linux GCC/Clang and scheduled tier execution remain delegated to CI.

