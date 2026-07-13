# Task 003 service protocol

## Objective
Provide a transport-neutral JSON-RPC application boundary and newline-delimited stdio host that Node can initialize, inspect, and shut down.

## Governing documents
- [Task 003](../tasks/003-SERVICE-PROTOCOL.md)
- [Service protocol](../docs/04-interfaces/SERVICE_PROTOCOL.md)
- [Errors, jobs, and events](../docs/05-implementation/ERRORS_JOBS_EVENTS.md)
- [ADR-0003](../docs/decisions/ADR-0003-PROCESS-API.md)

## Constraints
- Stdout contains protocol messages only; diagnostics use stderr.
- Public errors are stable typed values and never exception or SQLite text.
- The process boundary is the supported API; C++ interfaces remain internal.
- Implement only Task 003 system methods, not semantic memory methods.

## Current state
- The executable prints a version line and exits.
- nlohmann/json is declared in vcpkg but unused.
- No transport, dispatcher, protocol schemas, fixtures, or child-process test exists.

## Work sequence
1. Define typed service requests/results and stable application errors.
2. Implement JSON-RPC envelope validation, dispatch, version negotiation, duplicate handling, and cancellation plumbing.
3. Implement the NDJSON stdio loop and strict output separation.
4. Add schemas, golden fixtures, unit/conformance tests, and a cross-platform Node child-process test.
5. Run all available acceptance suites and stop before Task 004.

## Tests
- Valid requests and notifications.
- Malformed JSON and invalid envelopes.
- Unknown methods and duplicate IDs.
- Compatible and incompatible protocol ranges.
- Initialize, health, capabilities, status, cancellation, and shutdown.
- Node launches the built executable and confirms stdout contains only JSON-RPC.

## Risks and rollback
Streaming framing and process shutdown differ across platforms. The transport is isolated from the dispatcher and can be replaced without changing application results.

## Progress
- [x] Governing documents and current state reviewed.
- [x] Typed dispatcher implemented.
- [x] Stdio host implemented.
- [x] Schemas and fixtures added.
- [x] Unit and child-process tests passing on Windows; Linux execution remains CI-validated.

## Decisions made during execution
- Protocol version 1 is the sole bootstrap-supported version; negotiation uses inclusive integer `min` and `max` fields.

## Completion evidence
- Windows MSVC Debug configured and built; `ctest --preset windows-msvc-debug` passed 17/17 tests.
- Windows MSVC Release configured and built; `ctest --preset windows-msvc-release` passed 17/17 tests.
- Dispatcher tests cover initialization, all Task 003 system methods, notifications, cancellation, malformed JSON, invalid envelopes, unknown methods, duplicate IDs, version mismatch, and shutdown.
- Golden initialization fixtures match serialized results.
- The Node child-process test launches `pma-core`, initializes, cancels via notification, reads status, shuts down, and asserts stderr is empty and every stdout line parses as JSON-RPC.
- All three existing Node package suites passed.
- `git diff --check` passed.
- Linux child-process execution remains delegated to the checked-in GCC/Clang CI jobs because this host is Windows.
