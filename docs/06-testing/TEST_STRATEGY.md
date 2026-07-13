# Test strategy

PMA-Core uses deterministic tests for correctness and LLM-driven simulations for realistic complexity. An LLM is never the sole authority for evidence integrity, graph validity, migrations, replay, or known scenario truth.

## Test layers

1. **Unit tests** — Pure domain rules, parsers, renderers, scoring, and vector math.
2. **Component tests** — SQLite repositories, migrations, provider client, jobs, and protocol handlers.
3. **Lifecycle tests** — Observe through reconsolidate using deterministic provider fixtures.
4. **Integration tests** — Real Transformers.js provider, Pi event simulator, MCP conformance, and bundle movement.
5. **Performance tests** — Repeatable latency, throughput, memory, and storage benchmarks.
6. **LLM simulation** — Long-running realistic user/agent worlds with hidden ground truth and fault injection.

## Test doubles

Required deterministic doubles:

- Fake embedding provider returning stable configured vectors.
- Scripted structured-generation provider returning valid, invalid, and adversarial proposals.
- Captured provider-response replay.
- Fake clock.
- Deterministic ID generator.
- Temporary bundle environment.
- Pi lifecycle event simulator.

Core lifecycle tests must not require network access or model downloads.

## Invariant ownership

Deterministic assertions own:

- Evidence immutability.
- Foreign keys and graph invariants.
- Operation policy.
- Supersession history.
- Vector watermark correctness.
- Token budgets.
- Migration results.
- Job resume behavior.
- Expected hidden-world facts.

LLM evaluators own only qualitative judgments such as whether context was helpful, confusing, or appropriately qualified.

## Reproducibility

Every scenario records:

- Seed.
- PMA build and schemas.
- Provider runtime manifests.
- Model IDs and artifact hashes.
- Prompt/schema versions.
- Starting bundle snapshot.
- Events and provider responses.
- Final metrics.

Support live replay and captured replay.

## CI tiers

### Pull request

Unit, schema, graph invariant, protocol, migration smoke, and short lifecycle tests.

### Nightly

Provider integration, vector rebuild/replay, performance regression, fault subset, and 500–1,000 interaction simulation.

### Weekly

Large datasets, 10,000+ interaction simulation, cross-platform bundle movement, provider profile change, and soak testing.

### Release gate

No data-loss or graph-integrity failures, successful supported migrations, passing portability suite, valid vector reconstruction, and performance inside established regression limits.

Detailed suites are split into [Functional tests](FUNCTIONAL_TESTS.md), [Performance tests](PERFORMANCE_TESTS.md), [LLM stress tests](LLM_STRESS_TEST.md), and [Portability and resilience](PORTABILITY_AND_RESILIENCE.md).
