# PMA-Core

PMA-Core is a portable, transparent memory and knowledge platform for model-driven agents. It captures evidence, builds auditable semantic knowledge, retrieves only task-relevant context, evaluates whether recalled memory helped, and reconsolidates knowledge without rewriting the original evidence.

This repository currently contains the complete implementation plan rather than production code. Start with [PROJECT.md](PROJECT.md), then use the [documentation map](docs/README.md) and the ordered [work packages](tasks/README.md).

## Core direction

- C++20 core, built for Windows and Linux.
- SQLite is the authoritative store for structured knowledge and audit history.
- Vector indexes are separate, rebuildable SQLite projections.
- The knowledge model is graph-oriented but stored relationally.
- Models are external providers; PMA-Core never loads model runtimes.
- The first local provider uses Transformers.js in a long-lived Node process.
- OpenAI-compatible remote providers use the same provider-process boundary.
- Pi receives a full lifecycle integration through a TypeScript extension.
- MCP exposes model-callable tools and inspectable resources, but is not the internal PMA protocol.
- Injected memory context is aggressively minimized for token efficiency.

## Build order

The canonical build sequence is defined in [ROADMAP.md](docs/07-delivery/ROADMAP.md) and decomposed into agent-sized tasks in [tasks/](tasks/README.md). Each task states prerequisites, outputs, tests, and acceptance criteria.

## Important starting documents

1. [Goals and non-goals](docs/01-foundation/GOALS_AND_NON_GOALS.md)
2. [Memory lifecycle](docs/02-architecture/MEMORY_LIFECYCLE.md)
3. [Knowledge graph](docs/02-architecture/KNOWLEDGE_GRAPH.md)
4. [SQLite core storage](docs/03-storage/SQLITE_CORE.md)
5. [Vector projections](docs/03-storage/VECTOR_PROJECTIONS.md)
6. [Service protocol](docs/04-interfaces/SERVICE_PROTOCOL.md)
7. [Pi extension](docs/04-interfaces/PI_EXTENSION.md)
8. [MCP adapter](docs/04-interfaces/MCP_ADAPTER.md)
9. [Transformers.js provider](docs/04-interfaces/TRANSFORMERS_JS_PROVIDER.md)
10. [Test strategy](docs/06-testing/TEST_STRATEGY.md)

## Agent workflow

Pi and Codex both load repository guidance from [AGENTS.md](AGENTS.md). Longer implementation work must use [PLANS.md](PLANS.md), select one task from [tasks/](tasks/README.md), and update the relevant design document when behavior changes. The first agent prompt is provided in [START_BUILD.md](START_BUILD.md).
