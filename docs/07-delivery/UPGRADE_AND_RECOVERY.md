# Upgrade and recovery guide

## Before upgrade

1. Stop Pi, MCP, provider, and PMA processes using the bundle.
2. Run `pma-core verify <bundle>/pma.sqlite`.
3. Run `pma-core backup <bundle>/pma.sqlite <backup>/pma.sqlite`.
4. Preserve provider manifests and local environment mappings separately.
5. Record the PMA, protocol, Node, provider, and vector profile versions.

Opening a database through the normal service applies the ordered checksummed migrations. Never edit a recorded migration. After upgrade, run `verify` and exercise recall before deleting the backup.

## Maintenance commands

```text
pma-core verify  <database>
pma-core backup  <database> <backup-database>
pma-core restore <backup-database> <database>
pma-core compact <database>
```

`backup` uses SQLite online backup and verifies both source and copy. `restore` verifies the backup, writes and verifies a staged database, then replaces the destination while retaining the old file until replacement succeeds. `compact` first verifies, checkpoints WAL, vacuums, and verifies again. Commands return nonzero on refusal or failure.

## Moving a bundle

Copy the bundle while all writers are stopped, or copy from an online backup. Repository-relative resource locations remain portable. Old absolute locations remain historical and must be remapped for the destination environment. Verify the core database after the move. Missing vectors do not invalidate core knowledge: remove/retire the unavailable projection and rebuild it under a compatible provider profile.

## Recovery cases

- **Provider outage:** evidence already committed remains authoritative. Restart the provider and resume durable queued jobs.
- **Missing/corrupt vector:** do not repair the vector file as knowledge. Retire it and blue-green rebuild from embedding events.
- **Interrupted rebuild:** keep the active generation; resume or discard the non-active generation.
- **Core verification failure:** stop writes and restore the most recent verified backup. Do not silently create a fresh database at the same location.
- **Corrupt copied bundle:** retain the source bundle and reject the copy. Re-copy from a stopped source or verified backup.
- **Profile change:** build a new generation with the exact new fingerprint, validate it, then activate atomically.

## Supported migration fixtures

Release tests cover fresh initialization and idempotent reopen of current schema 400. Intermediate migration numbers were never public release boundaries and are not supported starting fixtures. Version 0.1.0 is the first public release boundary; future releases must retain an explicit fixture for every supported prior public schema.
