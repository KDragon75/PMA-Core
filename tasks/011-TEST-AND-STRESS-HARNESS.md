# Task 011 — Test and stress harness

## Objective

Establish measurable performance and realistic long-run behavior before release.

## Governing documents

- [Test strategy](../docs/06-testing/TEST_STRATEGY.md)
- [Performance tests](../docs/06-testing/PERFORMANCE_TESTS.md)
- [LLM stress test](../docs/06-testing/LLM_STRESS_TEST.md)
- [ADR-0007](../docs/decisions/ADR-0007-DETERMINISTIC-TESTS.md)

## Deliverables

- Synthetic data generators.
- Google Benchmark executables.
- Hidden-world TypeScript scenario controller.
- Simulated user, response, extraction, and evaluator roles.
- Captured replay format.
- Pi-like and MCP-only host modes.
- Fault injection library.
- Metrics and report output.
- Nightly and weekly CI workflows.

## Scenarios

Include preference qualification, project crossover, environment move, entity ambiguity, decision reversal, false inference, procedure learning, retrieval pollution, compaction, provider change, and vector rebuild during active use.

## Acceptance criteria

- A failed run is replayable with captured provider/model output.
- Deterministic hidden-state assertions are separate from LLM grades.
- Performance reports isolate core and provider time.
- Long-run simulation detects stale-memory and irrelevant-injection regressions.
- Fault tests demonstrate expected recovery without unexplained data loss.
