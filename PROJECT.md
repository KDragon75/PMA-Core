# PMA-Core project charter

## Purpose

PMA-Core provides persistent agent memory that survives model changes, tool changes, application changes, and environment moves. It aims to reproduce useful properties of human learning—observation, encoding, consolidation, associative recall, evaluation, and reconsolidation—without pretending to reproduce biological cognition.

The central contract is described in [MEMORY_LIFECYCLE.md](docs/02-architecture/MEMORY_LIFECYCLE.md). The graph representation supporting that contract is described in [KNOWLEDGE_GRAPH.md](docs/02-architecture/KNOWLEDGE_GRAPH.md).

## Product statement

PMA-Core stores original evidence, derives traceable knowledge, links knowledge into a graph, retrieves a compact set of relevant memories, and records how those memories affected later work. Every derived memory remains inspectable and traceable to its source.

## Primary users

- Agent harness developers integrating persistent memory.
- Users who want portable memory across models and tools.
- Developers running local models who require transparent storage.
- Researchers evaluating long-term memory behavior.

## Initial integrations

- Pi Agent extension with lifecycle hooks and context injection.
- MCP server exposing memory tools and resources.
- First-party Transformers.js provider for local embeddings and structured extraction.
- OpenAI-compatible provider adapter for external inference endpoints.

## Success criteria

PMA-Core is successful when it can:

1. Preserve the same knowledge when the response model changes.
2. Explain where every durable claim came from.
3. Apply corrections without deleting history.
4. Rebuild vector projections under a different embedding profile.
5. Copy the memory bundle to another environment without assuming old paths are valid.
6. Retrieve relevant knowledge through text, vectors, entities, graph relationships, and time.
7. Keep injected context small enough that memory improves rather than crowds out the active task.
8. Recover safely from provider outages, interrupted builds, and process restarts.
9. Pass deterministic functionality tests and long-running LLM-driven simulations.

## Planning authority

Locked decisions are summarized in [DECISIONS.md](docs/01-foundation/DECISIONS.md) and individually justified in [decision records](docs/decisions/README.md). Open questions are tracked in [OPEN_QUESTIONS.md](docs/01-foundation/OPEN_QUESTIONS.md).
