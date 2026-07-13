# Migrations, backup, and recovery

## Schema migrations

Migrations are ordered, immutable, and checksummed. Each migration records:

- Version.
- Name.
- Content hash.
- Applied timestamp.
- PMA build version.

Never edit a migration after release. Add a new migration that corrects the prior one.

## Migration rules

- Back up before destructive or long-running migrations.
- Run migrations inside transactions where SQLite permits.
- Separate core schema and vector schema versions.
- Keep old application versions from opening unsupported newer schemas in write mode.
- Include upgrade tests from every supported release boundary.
- Provide verification after migration.

## Backup

Use SQLite's online backup API for a consistent core copy. The bundle backup procedure must also capture:

- Core database.
- Active and optionally retired vector projections.
- Referenced attachments.
- Bundle manifest.

Vector files may be excluded when a compact backup is requested because they are rebuildable. The manifest must state what was omitted.

## Restore

Restore validates:

1. Bundle manifest and file hashes.
2. Core SQLite integrity.
3. Schema compatibility.
4. Vector-store registry against available files.
5. Provider and path configuration requiring local remapping.

Missing vectors do not make the core restore fail.

## Crash recovery

- WAL recovery is delegated to SQLite.
- Durable jobs resume from committed checkpoints.
- Incomplete vector builds remain in `building` or `failed` state and never become active automatically.
- An operation transaction either commits all graph changes or none.
- Provider responses are retained only after a successful parse; invalid partial output cannot mutate knowledge.

## Corruption handling

PMA-Core distinguishes:

- Core database corruption: critical, restore or repair from backup.
- Rebuildable FTS corruption: recreate projection.
- Vector file corruption: retire and rebuild generation.
- Missing attachment: preserve metadata and mark unresolved.
- Audit discontinuity: fail verification and prevent unsafe maintenance operations.

## Export

SQLite bundle copy is the primary portable format. JSONL export is optional interoperability functionality and is never a second canonical representation.

## Retention

Retired vectors, old backups, and provider caches may be pruned by policy. Original evidence and operation history require explicit retention or deletion policy and must not be removed merely because retrieval strength decays.
