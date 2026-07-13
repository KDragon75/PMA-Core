# Locked decisions

This file summarizes accepted decisions. The rationale and consequences are in [decision records](../decisions/README.md).

| Area | Decision |
|---|---|
| Core language | C++20 targeting Windows/MSVC and Linux/GCC or Clang. |
| Design priority | Readability and simplicity over abstraction or maximum theoretical performance. |
| Authoritative storage | SQLite stores structured evidence, knowledge, operations, jobs, audit data, and configuration. |
| JSON usage | JSON is limited to protocol boundaries, provider-specific options, manifests, and optional exports. |
| Knowledge representation | Graph-oriented relational model with first-class claims, entities, procedures, goals, and associations. |
| Memory ownership | Memories are not owned by users, projects, agents, sessions, tasks, or models. Those are related contexts and provenance. |
| Model identity | Model and agent identity may be recorded as provenance metadata but do not participate in the normal knowledge path. |
| File paths | Absolute paths are environment-specific observations, never portable canonical identities. |
| Vectors | Vectors live in separate SQLite projection files and are always rebuildable. |
| Vector compatibility | Compatibility is based on a full embedding profile fingerprint, not model name or dimensions alone. |
| Vector updates | Core embedding events use monotonic sequence numbers and replay watermarks. |
| Rebuild behavior | Full rebuilds create a new generation; active stores are replaced atomically after validation. |
| Runtime reconstruction | Vector stores carry enough metadata and conformance cases to reconstruct or verify the generating environment. |
| Public interface | No public native C++ API. PMA-Core exposes a JSON-RPC service protocol. |
| First transport | Newline-delimited JSON-RPC over stdio. |
| Pi integration | Full lifecycle integration through a TypeScript extension. |
| MCP | MCP is an adapter for model-callable operations and resources, not the internal PMA protocol. |
| Context injection | Full ContextPacket is retained internally; only a minimal ContextSlice is injected. |
| Providers | All model providers are external processes. PMA-Core never loads model weights or inference runtimes. |
| Initial provider | First-party Node provider using Transformers.js v4 for local inference. |
| Remote inference | OpenAI-compatible endpoints are supported through an external provider adapter using the same provider boundary. |
| Learning control | Models propose operations; deterministic PMA validation applies or rejects them. |
| Automatic learning | Capture immediately, extract after settled interaction, consolidate at compaction/session boundaries and by threshold. |
| Testing | Deterministic tests own invariants; LLMs generate realistic workloads and qualitative scoring but are not the sole oracle. |

Changes to these decisions require a new or superseding record under `docs/decisions/`.
