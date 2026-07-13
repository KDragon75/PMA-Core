# Task 002 — SQLite foundation

## Objective

Implement the authoritative database kernel before semantic memory behavior.

## Governing documents

- [SQLite core](../docs/03-storage/SQLITE_CORE.md)
- [Migrations, backup, and recovery](../docs/03-storage/MIGRATIONS_BACKUP_RECOVERY.md)
- [ADR-0001](../docs/decisions/ADR-0001-SQLITE-AUTHORITATIVE.md)

## Deliverables

- RAII connection, statement, transaction, and result wrappers.
- Database bootstrap metadata and migration runner.
- Migration checksum enforcement.
- Required connection PRAGMAs.
- Core database identity.
- Online backup wrapper.
- `verify` service callable internally.
- Temporary bundle fixture for tests.

## Tests

- Transaction commit/rollback.
- Statement binding and UTF-8 text.
- Foreign-key enforcement.
- Migration application, repeat execution, checksum mismatch, and failure rollback.
- Online backup of a live WAL database.
- Integrity verification and deliberate corruption fixtures where feasible.

## Acceptance criteria

- Database can be created, reopened, backed up, and verified on Windows and Linux.
- Failed migration does not leave a partially advanced schema version.
- SQLite errors map to typed PMA errors.
- No raw `sqlite3*` escapes the storage layer.
