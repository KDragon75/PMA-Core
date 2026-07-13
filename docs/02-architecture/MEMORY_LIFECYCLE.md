# Memory lifecycle

The lifecycle is the governing behavioral contract. Storage, graph structure, providers, Pi hooks, MCP tools, and tests must implement these stages rather than invent parallel flows.

## 1. Observe

Store the original episode and source metadata immediately. Observation requires no model call.

Inputs include user messages, assistant messages, tool calls, tool results, document segments, and imported sources. Evidence is immutable except for explicit redaction or deletion.

Output: episode and evidence records ready for encoding.

## 2. Encode

Extract candidate entities, claims, preferences, decisions, events, procedures, goals, corrections, relationships, and resource references.

Encoding is model-assisted through the external provider described in [Model providers](../04-interfaces/MODEL_PROVIDERS.md). Output is a proposal, never a direct database mutation.

## 3. Validate

Deterministic PMA code verifies:

- Schema correctness.
- Evidence existence.
- Predicate validity.
- Entity resolution.
- Duplicate or near-duplicate knowledge.
- Contradictions and temporal conflicts.
- Confidence and authority.
- Whether the operation is allowed automatically.

Invalid or unsupported proposals are rejected and audited.

## 4. Consolidate

Validated observations are integrated into durable knowledge:

- New evidence supports an existing claim.
- Explicit changes supersede prior claims.
- Exceptions qualify broad claims.
- Repeated observations form cautious generalizations.
- Procedures and goals are linked to their source evidence.

Immediate integration and long-term abstraction are separate. A single episode may create a direct claim but should rarely create a broad generalization.

## 5. Retrieve

Retrieval combines:

- FTS5 lexical search.
- Vector similarity.
- Entity and predicate matching.
- Bounded graph traversal.
- Temporal relevance.
- Current interaction context.
- Authority, confidence, strength, and historical utility.

The result is a full [ContextPacket](../04-interfaces/CONTEXT_PACKET_AND_SLICE.md).

## 6. Apply

The packet is reduced to a minimal ContextSlice:

1. Remove diagnostic metadata.
2. Collapse duplicate or overlapping statements.
3. Preserve material qualifications and conflicts.
4. Fit the configured token budget.
5. Inject through the host adapter.

## 7. Evaluate

After the interaction settles, record evidence about the outcome:

- Used or apparently useful.
- Irrelevant or ignored.
- Corrected or contradicted.
- Confusing or harmful.
- Unknown.

Evaluation is conservative. Absence from the response is not proof that a memory was unused.

## 8. Reconsolidate

New evidence and retrieval outcomes may cause:

- Reinforcement or weakening.
- Qualification or specialization.
- Supersession.
- Splitting an overgeneralized memory.
- Association adjustments.
- Reduced retrieval priority.

Reconsolidation never rewrites original evidence.

## Timing

| Trigger | Work |
|---|---|
| Immediately | Evidence capture, idempotency, basic segmentation, queue creation. |
| Agent settled | Candidate extraction, entity resolution, direct corrections, cheap deduplication. |
| Compaction/session transition | Active goals, decisions, unresolved work, restoration context. |
| Threshold/maintenance | Generalization, concept merging, contradiction review, association updates. |
| Explicit tool call | High-salience remember, correction, outcome, or pre-compaction capture. |

The test mapping is defined in [Functional tests](../06-testing/FUNCTIONAL_TESTS.md).
