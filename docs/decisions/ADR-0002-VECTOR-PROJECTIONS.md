# ADR-0002: Vector stores are separate projections

**Status:** Accepted

## Context

Vectors are incompatible across embedding profiles and must be rebuilt when model artifacts, pooling, prefixes, renderer, or related settings change. Rebuilding in the core database would enlarge backups and risk destroying the active working index.

## Decision

Store vectors in separate self-describing SQLite files. Treat them as materialized projections of authoritative embedding documents. Track them through profiles, generations, event-sequence watermarks, and blue-green activation.

## Consequences

- Multiple profiles and generations can coexist.
- A bundle remains useful without vectors.
- Rebuild and rollback are safe.
- Runtime reconstruction metadata and conformance tests are mandatory.

See [Vector projections](../03-storage/VECTOR_PROJECTIONS.md).
