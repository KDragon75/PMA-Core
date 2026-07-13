# ADR-0001: SQLite is authoritative

**Status:** Accepted

## Context

PMA-Core must remain fast, portable, inspectable, and independent of database servers. Maintaining SQLite and JSONL as parallel canonical stores would create synchronization and migration ambiguity.

## Decision

Use `pma.sqlite` as the authoritative structured store. Use JSON only at protocol boundaries, for provider-specific opaque configuration, manifests, and optional export.

## Consequences

- One portable source of truth.
- Explicit SQL and migrations are central project assets.
- JSONL export is compatibility output, not normal operation.
- Important provider metadata must be normalized when PMA behavior depends on it.

See [SQLite core](../03-storage/SQLITE_CORE.md).
