# Architecture decision records

Decision records protect the project from drifting back into a conventional opaque RAG implementation. A later record may supersede an earlier one but should not rewrite it.

- [ADR-0001: SQLite is authoritative](ADR-0001-SQLITE-AUTHORITATIVE.md)
- [ADR-0002: Vector stores are separate projections](ADR-0002-VECTOR-PROJECTIONS.md)
- [ADR-0003: Process-level API, no public C++ API](ADR-0003-PROCESS-API.md)
- [ADR-0004: Context is provenance, not ownership](ADR-0004-CONTEXT-NOT-OWNERSHIP.md)
- [ADR-0005: External model providers](ADR-0005-EXTERNAL-PROVIDERS.md)
- [ADR-0006: Claims-first knowledge graph](ADR-0006-CLAIMS-FIRST-GRAPH.md)
- [ADR-0007: Deterministic tests remain authoritative](ADR-0007-DETERMINISTIC-TESTS.md)
