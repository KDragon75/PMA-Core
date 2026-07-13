# MCP adapter

MCP exposes selected PMA capabilities to models and clients. MCP tools are model-controlled, so an MCP-only integration cannot guarantee automatic recall, context injection, or learning. Full automatic behavior requires a host integration such as the [Pi extension](PI_EXTENSION.md).

## Initial tools

### Read and recall

```text
pma_get_context
pma_recall
pma_search_memories
pma_get_memory
pma_explain_recall
pma_list_conflicts
pma_get_active_goals
pma_resolve_reference
```

### Controlled writes

```text
pma_remember
pma_learn
pma_propose_correction
pma_confirm_memory
pma_report_memory_outcome
pma_prepare_compaction
pma_restore_after_compaction
```

`pma_remember` and `pma_learn` create evidence and high-priority operation proposals. They do not bypass validation or directly create trusted semantic claims.

## Tools not enabled by default

```text
pma_delete_evidence
pma_forget_permanently
pma_change_context_relationship
pma_apply_operation
pma_rebuild_vectors
pma_activate_vector_store
pma_import_bundle
pma_change_provider
```

Administrative functionality belongs in the PMA service or CLI and may require explicit user confirmation.

## Resources

```text
pma://memory/{id}
pma://memory/{id}/history
pma://episode/{id}
pma://interaction/{id}
pma://context-packet/{id}
pma://conflicts
pma://vector-stores
pma://vector-stores/{id}
pma://providers
pma://status
```

Resources provide inspectable data. Mutation occurs through tools.

## Tool descriptions

Descriptions must teach the model when to call the tool. Example intent:

- `pma_get_context`: before tasks that may depend on prior work, after compaction, or when the user references earlier decisions.
- `pma_learn`: after explicit corrections, durable decisions, reusable procedures, changed assumptions, or unresolved goals.
- `pma_prepare_compaction`: before host summarization when important state may leave the model context.

Tool descriptions are guidance, not enforcement, and are tested with multiple models under [LLM-driven stress tests](../06-testing/LLM_STRESS_TEST.md).

## Recommended system prompt fragment

MCP hosts should add a compact instruction that says persistent memory is available, recalled memories may be uncertain, and current user corrections outrank recalled content. The exact prompt is versioned and evaluated rather than hard-coded into tool descriptions alone.

## Implementation

The adapter translates MCP JSON-RPC messages into the same application services used by the PMA protocol. It must not duplicate memory-processing logic.
