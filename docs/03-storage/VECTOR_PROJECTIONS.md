# Vector projections

Vectors are materialized projections of authoritative embedding documents. Each embedding profile and rebuild generation may have a separate SQLite file.

## Layout

```text
bundle/
├── pma.sqlite
└── vectors/
    ├── <profile>-generation-1.sqlite
    ├── <profile>-generation-2.sqlite
    └── <other-profile>-generation-1.sqlite
```

Paths stored in `pma.sqlite` are relative to the bundle root.

## Embedding documents

PMA-Core renders canonical embedding text before calling a provider. Store:

- Source object and revision.
- Document type.
- Exact rendered text.
- Content hash.
- Renderer version.
- Active status.

Changing the renderer creates new embedding work even if the source claim did not change.

## Embedding event stream

Every create, update, deactivate, or delete of an embedding document appends an event with a monotonic integer sequence. Vector projections track the highest applied sequence.

Use operations:

```text
upsert
delete
```

Timestamps are diagnostic only; replay correctness depends on sequence numbers.

## Embedding profile

The compatibility fingerprint includes every setting that can affect vectors:

- Provider and runtime type.
- Model artifact hash and revision.
- Tokenizer hash.
- Dimensions and data type.
- Pooling and normalization.
- Query and document prefixes.
- Maximum input and truncation.
- Embedding-document renderer version.
- Provider options affecting inference.

Same model name or dimensions do not establish compatibility.

## Vector file metadata

Every vector SQLite file is self-describing and repeats enough registry data to detect copied, renamed, orphaned, or mismatched files:

- Store ID.
- Profile ID and fingerprint.
- Generation.
- Source core database ID.
- Vector schema version.
- Runtime manifest ID.
- Dimensions, metric, normalization, and data type.
- Replay watermark.
- Build and completion timestamps.

## Vector rows

The canonical table stores raw packed vectors and content hashes. An optional future vector index may create additional rebuildable tables, but the raw table remains readable without the extension.

The MVP implements a clear flat similarity scan in C++ for correctness and baseline benchmarks. Optional acceleration is added only after performance data justifies it.

## Catch-up

For a compatible stale projection:

1. Read events after the stored watermark.
2. Fetch current embedding documents for upserts.
3. Batch provider calls.
4. Commit vector changes and the new watermark in one vector-database transaction.
5. Repeat until current.

Replay is idempotent. A failed batch does not advance the watermark.

## Rebuild and activation

Full rebuilds always create a new generation:

1. Open a consistent core snapshot.
2. Record `snapshot_seq`.
3. Embed all active documents.
4. Set the new store watermark to `snapshot_seq`.
5. Replay events after the snapshot.
6. Validate counts, hashes, dimensions, values, and profile.
7. Atomically activate the new generation in the core registry.
8. Retire but do not immediately delete the old generation.

The current active projection remains usable while the replacement builds.

## Runtime reconstruction

A vector store retains:

- Provider package and runtime versions.
- Node version, OS, and architecture.
- Model and tokenizer artifact hashes.
- Lockfile or environment recipe hash.
- Device/backend and deterministic settings.
- Conformance inputs and expected vector hashes or tolerances.

Compatibility levels:

```text
configuration-compatible
numerically reproducible
byte-identical
incompatible
```

Byte identity is not assumed across hardware or math backends. Conformance testing is required before reusing a reconstructed runtime.

## Startup policy

Default behavior:

- Reuse and catch up an exact compatible store.
- Build a new generation for the same profile when repair is unsafe.
- Build a new profile when the runtime fingerprint changes.
- Use FTS5 and graph retrieval while no compatible vectors are ready.
- Never query an old vector space with a new query embedding.
