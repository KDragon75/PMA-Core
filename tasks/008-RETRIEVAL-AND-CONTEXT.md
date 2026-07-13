# Task 008 — Retrieval and context

## Objective

Implement hybrid recall, compact context application, evaluation evidence, and initial reconsolidation inputs.

## Governing documents

- [ContextPacket and ContextSlice](../docs/04-interfaces/CONTEXT_PACKET_AND_SLICE.md)
- [Knowledge graph](../docs/02-architecture/KNOWLEDGE_GRAPH.md)
- [Memory lifecycle](../docs/02-architecture/MEMORY_LIFECYCLE.md)

## Deliverables

- FTS5 indexing and query.
- Vector candidate search.
- Entity/predicate seeding and bounded graph traversal.
- Deterministic score fusion with component diagnostics.
- Authority, temporal, strength, and utility adjustments.
- ContextPacket persistence.
- Minimal ContextSlice renderer and token estimator.
- Recall outcome recording.
- Initial association and utility update rules.

## Tests

- Each retrieval channel independently.
- Exact decision outranks weak semantic match.
- Project/context relevance without ownership filtering.
- Current versus superseded temporal claims.
- Conflict and qualification rendering.
- Redundancy removal and strict token budgets.
- Irrelevant high-strength memory exclusion.

## Acceptance criteria

- Retrieval is explainable from packet score components.
- Injected slice omits diagnostics and fits configured budget.
- FTS/graph retrieval works without provider availability.
- Retrieval alone does not reinforce memory.
