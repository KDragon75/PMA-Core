# Functional tests

## Lifecycle mapping

### Observe

Verify:

- Original evidence is preserved byte-for-byte where applicable.
- Event order and tool-call relationships are correct.
- Duplicate source events are idempotent.
- Interrupted interactions remain valid episodes.
- Redaction and deletion follow policy without broken references.

### Encode

Use fixtures covering facts, preferences, decisions, temporal events, procedures, goals, corrections, ambiguous entities, and transient details. Verify the extractor does not turn every sentence into durable knowledge.

### Validate

Reject:

- Missing or invented evidence IDs.
- Invalid predicate/object combinations.
- Unsupported operations.
- Nonexistent supersession targets.
- Confidence outside range.
- Provider-created database IDs.
- Ad hoc predicates when registry review is required.

### Consolidate

Verify exact support, duplicate merge, qualification, contradiction, supersession, generalization, specialization, procedure extraction, and unresolved-goal retention. Source evidence and specific claims must remain available.

### Retrieve

Test FTS, vector, entity, predicate, graph, temporal, context, authority, strength, and utility signals independently and in fusion. Explicit decisions should outrank weak semantic similarity.

### Apply

Verify ContextSlice deduplication, qualification preservation, conflict inclusion, token limits, and exclusion of diagnostic metadata.

### Evaluate

Record useful, irrelevant, corrected, harmful, and unknown outcomes. Verify conservative handling of unobserved usage.

### Reconsolidate

Test reinforcement after confirmation, weakening after repeated irrelevant retrieval, correction supersession, qualification, split, and association adjustment. Retrieval alone must not strengthen a memory.

## Graph invariants

- Every edge references existing nodes.
- Every accepted claim has evidence or explicit imported/inferred status.
- A claim has one subject and one object representation.
- Superseded claims remain queryable historically.
- Contradiction is represented consistently.
- Associative edges do not satisfy factual queries by themselves.
- Procedure steps retain order.
- Alias records do not imply entity merge.

## Temporal cases

Test current, historical, future, temporary, expired, superseded, and unknown validity. Queries about “now” must not retrieve obsolete claims as current without qualification.

## Entity cases

Required ambiguity fixture:

```text
Pi Agent
Raspberry Pi
mathematical pi
PMA
PMA-Core
Portable Memory Architecture Core
```

Test correct aliasing and prevention of false merges.

## Provider-output adversaries

- Malformed JSON.
- Valid JSON with wrong schema.
- References to nonexistent evidence.
- Unsupported broad generalization.
- Prompt-injected request to bypass validation.
- Correct proposal mixed with invalid operations.

PMA-Core applies only fully validated transaction sets according to the defined partial-failure policy.

## Protocol tests

Every method receives:

- Golden valid request/response fixture.
- Missing-field case.
- Unknown-field compatibility case.
- Invalid type case.
- Idempotency replay.
- Stable error-code assertion.
