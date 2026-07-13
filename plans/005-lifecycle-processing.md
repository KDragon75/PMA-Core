# Task 005 lifecycle processing

## Objective
Implement deterministic Observe, proposal ingestion, validation, immediate consolidation, durable jobs, and auditable corrections.

## Governing documents
- [Task 005](../tasks/005-LIFECYCLE-PROCESSING.md)
- [Memory lifecycle](../docs/02-architecture/MEMORY_LIFECYCLE.md)
- [Memory processing](../docs/02-architecture/MEMORY_PROCESSING.md)
- [Functional tests](../docs/06-testing/FUNCTIONAL_TESTS.md)

## Constraints
- Evidence commits before provider execution and is never rewritten by consolidation.
- Provider output is proposal data only and cannot choose IDs or mutate storage.
- Entire proposal batches validate before any semantic mutation.
- Every semantic mutation writes an operation audit and embedding-event placeholder atomically.
- Corrections preserve prior claims and evidence.

## Current state
- Evidence and graph schema/services exist through migration 101.
- No durable jobs, provider proposal boundary, operation audit, or lifecycle runner exists.

## Work sequence
1. Add durable job, operation audit, and embedding-event migrations.
2. Add atomic observation and extraction-job creation.
3. Define proposal/provider boundary and deterministic validation.
4. Apply initial operations atomically with audit and embedding placeholders.
5. Add scripted provider, retry/checkpoint/poison behavior, and direct correction.
6. Run all suites, update evidence, commit, and push before Task 006.

## Tests
- Evidence-to-claim deterministic processing.
- Invalid evidence and unsupported operation rejection.
- Support, contradiction, supersession, and direct correction.
- Provider failure after evidence commit.
- Restart, retry, and poison-job behavior.
- Atomic batch rollback and audit/embedding event creation.

## Risks and rollback
Operation semantics are durable. Migration 200 and processor behavior are tested as one unit before release; no provider call is made under a write transaction.

## Progress
- [x] Governing documents and state reviewed.
- [x] Lifecycle migration and observation implemented.
- [x] Validation and operations implemented.
- [x] Durable runner and fake provider implemented.
- [x] Acceptance suites passing on Windows; Linux execution remains CI-validated.

## Decisions made during execution
- Deterministic IDs derive from job ID and proposal ordinal; provider proposals contain no database ID fields.

## Completion evidence
- Migration 200 adds durable jobs, attempts, checkpoints, operation inputs/audit, embedding documents, and ordered embedding-event placeholders.
- Observation atomically commits episodes, immutable evidence, and extraction jobs before provider execution.
- Scripted providers return ID-free proposals; complete batches are deterministically validated before one atomic operation transaction.
- Implemented and tested create, support, qualify, contradict, supersede, contextualize, deactivate, and restore, including direct correction.
- Provider retry/restart and poison-job tests preserve source evidence; invalid mixed batches create no claims, audit rows, or embedding events.
- Windows MSVC Debug and Release each passed 31/31 CTest cases.
- All three Node package suites passed, `git diff --check` passed, and no new VS Code diagnostics were reported.
- Linux execution remains delegated to CI.
