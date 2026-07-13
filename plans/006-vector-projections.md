# Task 006 vector projections

## Objective
Implement rebuildable, self-describing vector SQLite generations with exact profile isolation, event replay, validation, activation, and flat cosine search.

## Governing documents
- [Task 006](../tasks/006-VECTOR-PROJECTIONS.md)
- [Vector projections](../docs/03-storage/VECTOR_PROJECTIONS.md)
- [ADR-0002](../docs/decisions/ADR-0002-VECTOR-PROJECTIONS.md)
- [Portability and resilience](../docs/06-testing/PORTABILITY_AND_RESILIENCE.md)

## Constraints
- Core SQLite remains authoritative; vector files are disposable projections.
- Store paths are bundle-relative and each file is self-describing.
- Every profile-affecting field participates in the fingerprint.
- Replay watermark advances atomically with vector changes.
- Rebuild uses a new generation and never destroys the active store.

## Current state
- Lifecycle migration 200 has preliminary embedding documents/events.
- No renderer, profile registry, vector file, packed format, provider, scan, or generation lifecycle exists.

## Work sequence
1. Extend core projection metadata and canonical embedding-document rendering.
2. Implement profiles, runtime manifests, self-describing vector schema, and packed vectors.
3. Implement deterministic fake embeddings, event catch-up, generation rebuild, and validation.
4. Implement atomic activation, retirement, inspection, cosine scan, and reconstruction plan.
5. Test corruption/interruption/profile isolation, run suites, commit, and push before Task 007.

## Tests
- Fingerprint changes for every compatibility field.
- Upsert/delete replay, idempotence, and interrupted batches.
- Multiple generations and atomic activation.
- Missing/corrupt/NaN/wrong-size/orphan/stale vector detection.
- Core operation without vector files and reconstruction metadata.

## Risks and rollback
Vector files are rebuildable. Failed generations remain non-active and can be removed; core registry activation is the only authoritative pointer change.

## Progress
- [x] Governing documents and state reviewed.
- [x] Core projection metadata and renderer implemented.
- [x] Vector files and provider implemented.
- [x] Build/catch-up/validation lifecycle implemented.
- [x] Acceptance suites passing on Windows; Linux execution remains CI-validated.

## Decisions made during execution
- MVP vectors use packed little-endian IEEE-754 float32 and deterministic flat cosine scanning.

## Completion evidence
- Migration 300 adds renderer metadata, exact embedding profiles, runtime manifests, and a bundle-relative vector-store registry.
- Vector files include self-describing metadata, packed float32 rows, source-core identity, runtime identity, profile fingerprint, generation, and replay watermark.
- Deterministic fake embeddings, flat cosine scan, idempotent batched replay, build, validate, activate, retire, inspect, and reconstruction-plan behavior are implemented.
- Tests cover every profile field, upsert/delete replay, interruption rollback, incompatible fingerprints, blue-green activation, failed rebuild isolation, missing files, wrong dimensions/size, hash mismatch, NaN, orphan rows, stale watermarks, and authoritative core access without vectors.
- Windows MSVC Debug and Release each passed 40/40 CTest cases.
- All three Node package suites and `git diff --check` passed.
- Linux execution remains delegated to CI.
