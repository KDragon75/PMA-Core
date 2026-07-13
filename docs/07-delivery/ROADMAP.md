# Roadmap

The roadmap advances from deterministic storage to model-assisted learning and finally host integration. Each milestone leaves a testable working system.

## Milestone 0 — Repository bootstrap

Deliver CMake presets, dependency manifests, formatting, test runners, Node workspaces, protocol schema directories, and CI skeleton. No memory behavior yet.

Exit: clean Windows and Linux builds plus C++ and Node smoke tests.

## Milestone 1 — SQLite and service foundation

Deliver core database creation, migrations, RAII SQLite wrappers, JSON-RPC stdio server, typed requests, stable errors, and health/status methods.

Exit: process can initialize a bundle, restart, migrate, verify, and answer protocol health calls.

## Milestone 2 — Evidence and graph primitives

Deliver episodes, segments, entities, aliases, contexts, claims, typed values, predicates, graph edges, procedures, goals, and audit operations.

Exit: deterministic APIs create and inspect graph objects with invariant tests.

## Milestone 3 — Lifecycle processing

Deliver observation ingestion, durable jobs, scripted provider fixtures, proposal validation, direct corrections, support, contradiction, and supersession.

Exit: deterministic scenarios pass Observe through Consolidate without a real model.

## Milestone 4 — Embedding documents and vector projections

Deliver renderer, event stream, profiles, separate vector databases, flat similarity scan, catch-up, rebuild, validation, activation, and reconstruction manifests.

Exit: multiple profiles and generations can coexist and replay safely.

## Milestone 5 — Node provider and local models

Deliver provider protocol, Transformers.js local embedding and generation, schema validation/retry, runtime manifests, conformance tests, and OpenAI-compatible remote adapter.

Exit: real local provider processes known fixtures on Windows and Linux.

## Milestone 6 — Hybrid retrieval and context rendering

Deliver FTS5, vectors, graph expansion, deterministic score fusion, ContextPacket, ContextSlice, token budgeting, conflicts, and outcome recording.

Exit: end-to-end recall improves deterministic task fixtures without exceeding budgets.

## Milestone 7 — Pi integration

Deliver Pi extension lifecycle mapping, process management, session/source reconciliation, automatic recall/injection, post-settle learning, compaction handling, and explicit tools.

Exit: normal Pi use records, recalls, and learns without manual memory tool calls.

## Milestone 8 — MCP integration

Deliver tools, resources, schemas, prompt fragment, permission defaults, and conformance tests.

Exit: MCP-only agents can explicitly recall, learn, correct, and prepare for compaction.

## Milestone 9 — Testing and hardening

Deliver performance baselines, cross-platform movement, failure injection, provider outage behavior, migration matrix, LLM stress harness, and long-run metrics.

Exit: release gates in [Definition of done](DEFINITION_OF_DONE.md) pass.

## Milestone 10 — First release

Freeze protocol/schema versions, create distributions, publish migration/support policy, and document known limitations.

The agent-sized execution order is in [tasks/README.md](../../tasks/README.md).
