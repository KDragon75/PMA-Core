# ContextPacket and ContextSlice

Retrieval diagnostics and injected model context have different requirements. PMA-Core therefore uses two representations.

## ContextPacket

The ContextPacket is the full internal retrieval result. It may contain:

- Packet and interaction IDs.
- Query fingerprint.
- Candidate and selected memory revisions.
- FTS, vector, graph, temporal, authority, strength, and utility scores.
- Selection reasons.
- Evidence references.
- Conflicts and supersession relationships.
- Vector profile and replay watermark.
- Token estimates.
- Retrieval policy version.

Packets support inspection, evaluation, replay, and debugging. They are not semantic memories.

## ContextSlice

The ContextSlice is compact text injected into the response model. It normally contains only:

- Active goals relevant to the request.
- Prior decisions that constrain the task.
- Relevant factual knowledge.
- Relevant preferences or procedures.
- Material conflicts or uncertainty.

Do not normally inject IDs, timestamps, scores, evidence IDs, vector metadata, provider details, or retrieval explanations.

## Rendering pipeline

```text
Selected packet items
    -> group by semantic purpose
    -> remove duplicates
    -> collapse repeated evidence into one statement
    -> preserve exceptions and conflicts
    -> estimate tokens
    -> drop lowest-value statements
    -> render ContextSlice
```

Budget the rendered result, not the raw candidate count.

## Authority labels

Labels cost tokens and are used only where they change interpretation:

```text
[decision]
[inference, uncertain]
[conflict]
[temporary]
```

Explicit facts need no label when the entire section is clearly identified.

## Example

```text
<PMA-memory>
Current goal:
- Define PMA-Core's memory-processing architecture.

Relevant decisions:
- SQLite is the authoritative structured store.
- Vector embeddings use separate rebuildable SQLite projections.
- Projects, agents, sessions, and environments are provenance, not memory owners.

Constraint:
- Absolute file paths are valid only in the environment where observed.
</PMA-memory>
```

## Injection safety

- Clearly delimit PMA context from user instructions.
- Do not store untrusted retrieved text as system-level instructions without policy.
- Preserve source authority so imported or inferred content cannot silently override current user direction.
- Current explicit user corrections outrank recalled content.

## Packet persistence

Store enough packet information to evaluate retrieval and reproduce selection. Large provider-independent diagnostics may be normalized into retrieval tables rather than retained as one JSON blob.
