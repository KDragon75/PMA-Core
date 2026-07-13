# Task 009 — Pi extension

## Objective

Provide automatic PMA behavior in normal Pi usage.

## Governing documents

- [Pi extension](../docs/04-interfaces/PI_EXTENSION.md)
- [PMA service protocol](../docs/04-interfaces/SERVICE_PROTOCOL.md)
- [ContextPacket and ContextSlice](../docs/04-interfaces/CONTEXT_PACKET_AND_SLICE.md)

## Deliverables

- TypeScript extension package.
- PMA child-process client and recovery.
- Session/environment/repository observation mapping.
- Event handling for session, input, agent start, context, messages, turns, settled state, compaction, model selection, and shutdown.
- One-recall-per-interaction cache.
- ContextSlice injection.
- Explicit recall/learn/correct/inspect tools and commands.
- Concise system prompt fragment.

## Tests

- Simulated Pi lifecycle including retry, tools, parallel results, fork, resume, compaction, and shutdown.
- No duplicate ContextSlice across model calls.
- Response-model changes do not change PMA providers.
- Source ordering and reconciliation after restart.

## Acceptance criteria

- User does not need to manually invoke memory tools for ordinary capture and recall.
- Learning queues only after the interaction settles.
- Compaction cannot lose already observed evidence.
- Pi session data remains external evidence, not PMA authority.
