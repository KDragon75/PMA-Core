# Task 002 SQLite foundation

## Objective
Implement a typed, authoritative SQLite kernel that creates, migrates, verifies, and backs up PMA databases without exposing SQLite handles outside storage.

## Governing documents
- [Task 002](../tasks/002-SQLITE-FOUNDATION.md)
- [SQLite core](../docs/03-storage/SQLITE_CORE.md)
- [Migrations, backup, and recovery](../docs/03-storage/MIGRATIONS_BACKUP_RECOVERY.md)
- [ADR-0001](../docs/decisions/ADR-0001-SQLITE-AUTHORITATIVE.md)

## Constraints
- SQLite remains authoritative and uses STRICT PMA-owned tables.
- Every connection enables foreign keys, WAL, NORMAL synchronous mode, and a 5000 ms busy timeout.
- Migrations are ordered, transactional, immutable, and SHA-256 checksummed.
- No raw SQLite handle may escape `src/storage`.
- No semantic memory schema or provider behavior is included.

## Current state
- Task 001 builds a minimal executable and Catch2 smoke test.
- SQLite 3.50.4 and the SHA-256 implementation are vendored but not compiled.
- No storage interfaces, schema, migrations, backup, or verification behavior exists.

## Work sequence
1. Compile vendored SQLite/SHA-256 behind an internal storage library.
2. Add typed errors and RAII database, statement, and transaction wrappers.
3. Add bootstrap metadata, identity, and transactional checksum-enforced migrations.
4. Add online backup and integrity/foreign-key verification.
5. Add temporary database fixtures and acceptance tests.
6. Build and run all available checks; record evidence and stop before Task 003.

## Tests
- Statement binding and UTF-8 round-trip.
- Transaction commit and automatic rollback.
- Foreign-key enforcement and typed error mapping.
- Migration application, idempotence, checksum mismatch, and failed migration rollback.
- Identity persistence after reopen.
- Online backup from a live WAL database and backup verification.
- Integrity and foreign-key violation reporting.
- Existing smoke and Node suites.

## Risks and rollback
SQLite compile flags or platform filesystem behavior may differ by compiler. The storage library is isolated and can be removed without changing protocol or domain contracts.

## Progress
- [x] Governing documents and repository state reviewed.
- [x] Storage wrappers implemented.
- [x] Migrations and identity implemented.
- [x] Backup and verification implemented.
- [x] Tests passing on available Windows presets; Linux presets remain CI-validated.

## Decisions made during execution
None beyond governing documents.

## Completion evidence
- Windows MSVC Debug configured and built successfully; `ctest --preset windows-msvc-debug` passed 9/9 tests.
- Windows MSVC Release configured and built successfully; `ctest --preset windows-msvc-release` passed 9/9 tests.
- Storage coverage includes UTF-8 binding, transaction commit/rollback, foreign keys, typed constraints, migration idempotence/checksums/failure rollback, identity reopen, live-WAL backup, and verification failures.
- `npm --prefix providers test`, `npm --prefix adapters/pi test`, and `npm --prefix tests/llm-simulation test` each passed 1/1 tests.
- `git diff --check` passed.
- A source scan confirmed no raw `sqlite3*` declarations outside `src/storage` and vendored implementation code.
- Linux GCC/Clang execution remains delegated to the checked-in CI matrix because this host is Windows.
