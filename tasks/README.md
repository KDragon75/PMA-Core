# PMA-Core implementation tasks

Work through these tasks in order unless a task explicitly states it can run in parallel. For work spanning several commits, create an execution plan using [PLANS.md](../PLANS.md).

| Task | Outcome |
|---|---|
| [001](001-BOOTSTRAP.md) | Cross-platform repository and test skeleton. |
| [002](002-SQLITE-FOUNDATION.md) | Authoritative SQLite database, migrations, backup, verification. |
| [003](003-SERVICE-PROTOCOL.md) | JSON-RPC stdio service with stable system methods. |
| [004](004-EVIDENCE-AND-GRAPH.md) | Evidence, entities, claims, procedures, goals, and graph primitives. |
| [005](005-LIFECYCLE-PROCESSING.md) | Durable jobs and deterministic Observe–Consolidate processing. |
| [006](006-VECTOR-PROJECTIONS.md) | Separate vector SQLite stores, replay, rebuild, validation. |
| [007](007-NODE-PROVIDER.md) | Transformers.js and OpenAI-compatible external provider. |
| [008](008-RETRIEVAL-AND-CONTEXT.md) | Hybrid retrieval, ContextPacket, and minimal ContextSlice. |
| [009](009-PI-EXTENSION.md) | Automatic Pi lifecycle integration. |
| [010](010-MCP-ADAPTER.md) | MCP tools, resources, and prompt guidance. |
| [011](011-TEST-AND-STRESS-HARNESS.md) | Performance, portability, fault, and LLM simulation harnesses. |
| [012](012-HARDENING-AND-RELEASE.md) | Packaging, migration matrix, security review, release gate. |

## Task execution rules

1. Read the task and every linked governing document.
2. Verify repository state before planning changes.
3. Do not implement later-layer abstractions early “for flexibility.”
4. Add tests in the same task as behavior.
5. Record deviations in the task's completion notes and add a decision record when architectural.
6. Stop at the acceptance criteria; avoid opportunistic unrelated features.

Use [TASK_TEMPLATE.md](TASK_TEMPLATE.md) when new work packages are added.
