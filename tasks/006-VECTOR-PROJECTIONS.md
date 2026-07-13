# Task 006 — Vector projections

## Objective

Implement rebuildable separate vector SQLite files before real model integration.

## Governing documents

- [Vector projections](../docs/03-storage/VECTOR_PROJECTIONS.md)
- [ADR-0002](../docs/decisions/ADR-0002-VECTOR-PROJECTIONS.md)
- [Portability and resilience](../docs/06-testing/PORTABILITY_AND_RESILIENCE.md)

## Deliverables

- Embedding-document renderer and versioning.
- Embedding event stream.
- Profile and runtime-manifest tables.
- Vector-store registry and vector SQLite schema.
- Raw packed vector format and flat cosine scan.
- Fake embedding provider.
- Catch-up, build, validate, activate, retire, and inspect jobs.
- Conformance cases and environment reconstruction plan output.

## Tests

- Profile differences for model hash, dimensions, prefix, pooling, renderer, and tokenizer.
- Replay upsert/delete and interruption.
- Multiple generations and atomic activation.
- Missing, corrupt, NaN, wrong-size, orphan, and stale vectors.
- Core remains usable without vector files.

## Acceptance criteria

- A stale compatible store catches up idempotently.
- Full rebuild never destroys the active generation.
- Incompatible profiles are never mixed.
- A vector file can describe how it was produced without consulting filenames.
