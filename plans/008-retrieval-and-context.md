# Task 008 retrieval and context

## Objective
Implement explainable hybrid recall, persisted ContextPackets, compact budgeted ContextSlices, and conservative outcome-driven utility updates.

## Governing documents
- [Task 008](../tasks/008-RETRIEVAL-AND-CONTEXT.md)
- [ContextPacket and ContextSlice](../docs/04-interfaces/CONTEXT_PACKET_AND_SLICE.md)
- [Knowledge graph](../docs/02-architecture/KNOWLEDGE_GRAPH.md)
- [Memory lifecycle](../docs/02-architecture/MEMORY_LIFECYCLE.md)

## Constraints
- Every selected item retains component-level score diagnostics in the packet.
- Injected slices contain no IDs, scores, evidence, provider, or vector diagnostics.
- Context influences relevance but never filters by ownership.
- Superseded/history records remain inspectable but are penalized for current recall.
- Retrieval alone never changes strength, utility, or associations.

## Current state
- Claims, evidence, graph edges, embedding documents, and flat vector scan exist.
- No FTS projections, score fusion, packet persistence, slice renderer, or recall outcomes exist.

## Work sequence
1. Add FTS5 and retrieval/evaluation schema with rebuildable indexing.
2. Implement lexical, vector, entity/predicate, context, and bounded graph channels.
3. Add deterministic score components, fusion, temporal/authority/utility policy, and packet persistence.
4. Add deduplicating conflict-aware ContextSlice rendering with a strict token estimator.
5. Add outcome recording and conservative association/utility updates.
6. Run suites, commit, and push before Task 009.

## Tests
- Each retrieval channel independently and provider-free fallback.
- Explicit decisions outrank weak vector similarity.
- Context relevance without ownership filtering.
- Current versus superseded claims and low-utility exclusion.
- Conflict/qualification rendering, deduplication, and token limits.
- Packet diagnostics persistence and no reinforcement before outcomes.

## Risks and rollback
FTS tables and packets are projections/audit data and can be rebuilt. Scoring policy is versioned so future calibration does not reinterpret persisted packet diagnostics.

## Progress
- [x] Governing documents and state reviewed.
- [x] Retrieval schema and channels implemented.
- [x] Fusion and packet persistence implemented.
- [x] Slice and outcome rules implemented.
- [x] Acceptance suites passing on Windows; Linux execution remains CI-validated.

## Decisions made during execution
- Policy `hybrid-v1` uses deterministic weighted normalized channels and a conservative four-characters-per-token estimate.

## Completion evidence
- Migration 400 adds rebuildable FTS5 projections, persisted packets/candidate diagnostics, outcomes, claim utility, and associations.
- Hybrid recall combines FTS, vectors, entity/predicate seeds, two-hop non-associative graph expansion, and active context with deterministic `hybrid-v1` components.
- Authority, current/superseded status, strength, and explicit outcome utility affect ranking; sufficiently irrelevant memories are excluded despite strength.
- ContextSlice rendering removes duplicate text, preserves conflict/qualification labels, omits diagnostics, and enforces the rendered token estimate.
- Retrieval itself leaves utility/strength unchanged; explicit used/irrelevant/corrected/harmful outcomes update bounded utility, and useful co-selection creates cautious associations.
- Windows MSVC Debug and Release each passed 50/50 CTest cases. Node provider passed 4/4, and Pi/simulation suites passed 1/1 each.
- The local Qwen3 Q4 model smoke passed with 1024 normalized dimensions.
- `git diff --check` and VS Code diagnostics passed. Linux execution remains delegated to CI.
