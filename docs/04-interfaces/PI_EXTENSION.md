# Pi extension

The Pi extension provides the complete PMA integration because Pi exposes lifecycle events, context modification, custom tools, session persistence, and compaction hooks. Pi's current extension and session behavior is referenced in [External references](../references/EXTERNAL_REFERENCES.md).

## Responsibilities

- Launch or connect to PMA-Core.
- Map Pi sessions and interactions to PMA source identities.
- Capture ordered messages and tool results.
- Request recall once per user interaction.
- Inject one minimal ContextSlice into each model call for that interaction.
- Trigger post-interaction learning after the agent settles.
- Preserve state before compaction.
- Register explicit memory tools and inspection commands.

## Lifecycle mapping

### `session_start`

- Connect to PMA-Core.
- Resolve current environment, working directory, repository, and session observations.
- Open or resume a PMA session.
- Reconcile any source entries missed before the prior shutdown.

### `input`

- Capture raw input metadata if useful.
- Do not treat pre-expansion input as the final canonical user message.

### `before_agent_start`

- Begin the PMA interaction.
- Record the canonical submitted prompt.
- Build the active context frame.
- Request a ContextPacket and cache its ContextSlice.

### `context`

- Remove any previous synthetic PMA context from the copied message list.
- Insert the cached ContextSlice exactly once.
- Do not rerun full recall for every tool-loop model call.

The MVP does not refresh mid-turn. Later refresh triggers may include material tool-state changes, explicit corrections, or user steering.

### `message_end`

- Capture finalized user, assistant, and tool-result messages.
- Preserve source order and Pi entry identity.

### `turn_end`

- Record the coherent model response and its tool results for telemetry and evaluation.

### `agent_settled`

- Complete the interaction.
- Record recall outcome evidence.
- Queue extraction and immediate integration.
- Clear the active ContextSlice.

### `session_before_compact`

- Verify that all source episodes are durable.
- Trigger active-goal and unresolved-decision consolidation.
- Prepare restoration context.
- Treat Pi's compaction summary as additional evidence, not authoritative PMA knowledge.

### `model_select`

Record response-model provenance only. Do not change PMA embedding or extraction providers automatically.

### `session_shutdown`

Flush adapter state, close the PMA session, and terminate or release the child process according to launch policy.

## Registered tools

The extension may expose explicit tools equivalent to the MCP surface:

- Recall more context.
- Remember a durable statement.
- Report a correction.
- Explain recalled memory.
- Inspect a memory.

Automatic lifecycle behavior must not rely on the model calling these tools.

## Session files

Pi JSONL sessions are external evidence and may branch. The adapter records source IDs and parent relationships but never uses the Pi file as PMA's canonical knowledge store.

## Extension instructions

The extension appends a short system instruction explaining when explicit PMA tools should be used. Keep it concise because automatic recall and capture are already handled by lifecycle hooks.
