# Task 005 — Lifecycle processing

## Objective

Implement Observe, Encode proposal ingestion, Validate, and immediate Consolidate using deterministic provider fixtures.

## Governing documents

- [Memory lifecycle](../docs/02-architecture/MEMORY_LIFECYCLE.md)
- [Memory processing](../docs/02-architecture/MEMORY_PROCESSING.md)
- [Functional tests](../docs/06-testing/FUNCTIONAL_TESTS.md)

## Deliverables

- Observation APIs and episode transaction.
- Durable job schema, runner, attempts, and checkpoints.
- Structured proposal types.
- Operation validators and audit records.
- Initial operations: create, support, qualify, contradict, supersede, contextualize, deactivate, restore.
- Scripted fake generation provider.
- Direct correction path.

## Tests

- Full deterministic scenarios from evidence to accepted claim.
- Invalid evidence references and operations.
- Exact support versus contradiction.
- Explicit decision reversal.
- Provider failure after evidence commit.
- Job restart and poison-job behavior.
- Atomic operation commit and embedding-event placeholder creation.

## Acceptance criteria

- Provider proposals cannot directly mutate tables.
- Source evidence survives provider or processing failure.
- Explicit correction creates a new current claim and preserves prior history.
- Operation audit explains every semantic mutation.
