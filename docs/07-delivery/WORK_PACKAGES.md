# Work packages

Each package should be implementable and reviewable independently. Detailed task briefs are under [tasks/](../../tasks/README.md).

## WP1 — Build and repository skeleton

- CMake and presets.
- vcpkg manifest.
- SQLite vendoring.
- Node workspaces.
- CI matrix.
- Formatting and warnings.

## WP2 — Storage kernel

- SQLite connection, statement, transaction, and migration wrappers.
- Core metadata and verification.
- Online backup wrapper.
- Temporary bundle test fixture.

## WP3 — Protocol kernel

- JSON-RPC parser and dispatcher.
- Initialize, health, capabilities, status, and shutdown.
- Stable error model.
- Protocol schema fixtures.

## WP4 — Evidence and graph

- Episode and segment ingestion.
- Entity, alias, context, resource, and location storage.
- Predicate registry, typed values, claims, and evidence links.
- Procedures, goals, edges, and graph verification.

## WP5 — Memory operations and jobs

- Operation vocabulary and validators.
- Durable jobs and checkpoints.
- Support, contradiction, qualification, supersession, and correction.
- Scripted provider fixtures.

## WP6 — Vector projections

- Embedding documents and renderer.
- Embedding event stream.
- Profile fingerprints and store registry.
- Separate vector SQLite schema.
- Catch-up, rebuild, verify, activate, retire.
- Environment reconstruction plan and conformance cases.

## WP7 — Provider process

- Provider JSON-RPC.
- Process manager and cancellation.
- Transformers.js embeddings.
- Structured generation and schema validation.
- OpenAI-compatible remote adapter.
- Runtime manifest and model artifact hashing.
- Benchmark-driven initial model selection.

## WP8 — Retrieval and application

- FTS5 projections.
- Flat vector search.
- Graph seed and bounded expansion.
- Score fusion.
- ContextPacket persistence.
- Minimal ContextSlice renderer.
- Retrieval outcomes and reconsolidation inputs.

## WP9 — Pi extension

- Lifecycle event mapping.
- Source identity and session reconciliation.
- Context injection.
- Compaction support.
- Commands and explicit tools.
- Adapter tests.

## WP10 — MCP adapter

- Tool/resource definitions.
- Prompt guidance.
- Permission defaults.
- Mapping to application services.
- Model-behavior tests.

## WP11 — Evaluation and stress

- Performance datasets and benchmarks.
- Hidden-world scenario controller.
- Multi-role LLM simulation.
- Captured replay.
- Fault injection.
- Metrics reports.

## WP12 — Hardening and release

- Migration matrix.
- Backup/restore CLI.
- Packaging.
- License and third-party notices.
- Security review.
- Release docs and compatibility table.
