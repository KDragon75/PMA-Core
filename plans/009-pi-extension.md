# Task 009 Pi extension

## Objective
Provide automatic PMA capture, one-recall-per-interaction context injection, settled learning, compaction durability, and explicit memory controls during normal Pi usage.

## Governing documents
- [Task 009](../tasks/009-PI-EXTENSION.md)
- [Pi extension](../docs/04-interfaces/PI_EXTENSION.md)
- [Service protocol](../docs/04-interfaces/SERVICE_PROTOCOL.md)
- [ContextPacket and ContextSlice](../docs/04-interfaces/CONTEXT_PACKET_AND_SLICE.md)
- Pi extension, compaction, and session-format documentation from the installed Pi package.

## Constraints
- Launch session-scoped processes only from `session_start`; shutdown is idempotent.
- Recall occurs once per user interaction and the same minimal slice is reused across tool loops.
- Capture final ordered messages; learning starts only at `agent_settled`.
- Compaction first flushes observed evidence; summaries are evidence, not authority.
- Response-model selection records provenance and never changes PMA providers.

## Current state
- `adapters/pi` is a TypeScript smoke package only.
- PMA-Core has a stdio system protocol, while lifecycle protocol expansion remains incremental.
- Pi lifecycle types and extension APIs are available from the installed/public coding-agent package.

## Work sequence
1. Implement restartable NDJSON PMA child client.
2. Implement a testable lifecycle controller with context mapping, ordering, recall caching, and flush/reconcile behavior.
3. Bind controller to Pi lifecycle events and context injection.
4. Add recall/learn/correct/inspect tools and commands plus concise prompt guidance.
5. Simulate retry, parallel results, fork/resume, compaction, model changes, and shutdown.
6. Run suites, commit, and push before Task 010.

## Tests
- One recall and one injected slice across repeated context/tool loops.
- Ordered parallel tool results and restart reconciliation.
- Learning only after settled state.
- Flush before compaction and shutdown.
- Fork/resume source identity and model provenance independence.
- Child crash/restart and stable protocol errors.

## Risks and rollback
Pi event APIs evolve. Runtime-specific code is kept in `index.ts`; the lifecycle controller and process client are independently testable.

## Progress
- [x] Governing and Pi documentation reviewed.
- [x] PMA client and lifecycle controller implemented.
- [x] Pi event/tool bindings implemented.
- [x] Lifecycle simulations passing on Windows; Linux execution remains CI-validated.

## Decisions made during execution
- Controller-generated source keys combine Pi session ID, interaction sequence, and ordered message ordinal; Pi files remain external evidence.

## Completion evidence
- Added a restartable NDJSON child client, source-ordered lifecycle controller, one-recall interaction cache, compaction flush, branch reconciliation, environment/repository mapping, and idempotent shutdown.
- Added project auto-discovery at `.pi/extensions/pma-core/index.ts` and Pi bindings for session/input/before-agent/agent/context/message/turn/settled/compaction/model/shutdown events.
- Added automatic hidden ContextSlice injection plus `pma_recall`, `pma_learn`, `pma_correct`, and `pma_inspect` tools and matching status/recall/learn/correct/inspect commands.
- Expanded the core stdio host to open project databases, persist Pi evidence, queue settled learning, perform provider-free FTS recall, and support explicit learning, correction, and inspection.
- Pi lifecycle tests passed 4/4, covering one recall across tool loops, no duplicate slices, settled-only learning, parallel source order, compaction flush, resume reconciliation, model provenance isolation, crash restart, and shutdown.
- Windows MSVC Debug and Release each passed 52/52 CTest cases, including real service lifecycle persistence and explicit learn/correct/recall.
- Node provider passed 4/4 and simulation passed 1/1; `git diff --check` passed.
- Automatic settled learning is durably queued; continuous background execution of queued provider jobs remains later integration/hardening work. Linux execution remains delegated to CI.
