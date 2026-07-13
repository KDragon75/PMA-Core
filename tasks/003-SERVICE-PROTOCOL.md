# Task 003 — PMA service protocol

## Objective

Create the transport-neutral application boundary and stdio JSON-RPC host.

## Governing documents

- [PMA service protocol](../docs/04-interfaces/SERVICE_PROTOCOL.md)
- [Errors, jobs, and events](../docs/05-implementation/ERRORS_JOBS_EVENTS.md)
- [ADR-0003](../docs/decisions/ADR-0003-PROCESS-API.md)

## Deliverables

- Newline-delimited JSON-RPC reader/writer.
- Typed request parsing and result serialization.
- Dispatcher and cancellation plumbing.
- `pma.initialize`, `health`, `get_capabilities`, `get_status`, and `shutdown`.
- Version negotiation.
- Stable error schema and golden protocol fixtures.
- Strict stdout/stderr separation.

## Tests

- Valid requests and notifications.
- Malformed JSON and invalid JSON-RPC envelopes.
- Unknown methods.
- Duplicate IDs and idempotency behavior where applicable.
- Protocol version overlap and mismatch.
- Child-process integration from Node on Windows and Linux.

## Acceptance criteria

- A Node test client launches PMA-Core, initializes it, reads status, and shuts it down.
- Protocol output never contains diagnostic text.
- Internal exceptions or SQLite messages do not become unstable public error contracts.
