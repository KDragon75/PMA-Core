# Memory processing

This document defines the operational pipeline implementing the [memory lifecycle](MEMORY_LIFECYCLE.md) over the [knowledge graph](KNOWLEDGE_GRAPH.md).

## Processing stages

### Evidence capture

1. Assign an idempotency key from the host source identity.
2. Store the original event and ordering metadata.
3. Link messages, tool calls, and results.
4. Create evidence segments using deterministic boundaries where practical.
5. Append an extraction job.

Capture commits before any provider call.

### Candidate extraction

The structured-generation provider receives:

- New evidence segments.
- Nearby resolved entities.
- Nearby active claims.
- Allowed object types and predicates.
- Allowed memory operations.
- A strict output schema.

The provider returns proposals only. It does not choose database IDs, timestamps, transaction boundaries, or final status.

### Entity resolution

For each proposed entity mention:

1. Query exact canonical names and aliases.
2. Search contextually related entities.
3. Optionally use embedding similarity.
4. Score candidate identity matches.
5. Auto-link only above the conservative threshold.
6. Otherwise create a distinct entity or unresolved-reference candidate.

### Claim comparison

Compare proposed claims by subject, predicate, object/value, temporal range, and related context. Classify as:

- New claim.
- Supporting evidence.
- Duplicate wording.
- Qualification.
- Contradiction.
- Supersession.
- Possible generalization.

### Operation validation

Allowed initial operations:

```text
create
support
qualify
contradict
supersede
merge
split
generalize
specialize
contextualize
reinforce
weaken
deactivate
restore
```

Every operation has required evidence, allowed source states, reversibility behavior, and an automatic-application policy.

### Commit

One transaction writes:

- New or revised knowledge objects.
- Evidence links.
- Graph edges.
- Operation audit record.
- Embedding-document updates and embedding events.
- Follow-up jobs.

Provider calls never occur while a write transaction is held.

## Automatic versus proposed operations

Safe initial automatic operations:

- Attach new evidence to an exact existing claim.
- Create an explicit claim from direct current evidence.
- Record an explicit correction and supersession when the target is unambiguous.
- Add provenance and contextual relationships.

Initially review or defer:

- Broad generalization.
- Entity merge without exact durable identity.
- Procedure abstraction across sessions.
- Claim split.
- Automatic new predicate creation.
- Permanent deletion.

## Generalization policy

Generalization creates a new lower-confidence claim linked through `generalized_from`. It must preserve source claims and avoid widening across unrelated contexts. The first implementation should require multiple supporting observations and no material contradiction.

## Reconsolidation policy

Retrieval alone does not strengthen memory. Reinforcement requires evidence such as explicit confirmation, successful outcome attribution, or repeated independent support. Repeated irrelevant recall may lower utility without lowering truth confidence.

## Queue behavior

Jobs are durable and resumable. Provider failure leaves the source episode safe and the job retryable. Poisoned jobs enter a failed state with the provider response and validation error retained for diagnosis.

See [Errors, jobs, and events](../05-implementation/ERRORS_JOBS_EVENTS.md).
