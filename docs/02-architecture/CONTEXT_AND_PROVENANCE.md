# Context and provenance

PMA-Core does not assign memory ownership to users, projects, models, agents, sessions, or tasks. These objects provide meaning, provenance, and retrieval signals.

## Context types

Initial types include:

- Person
- Organization
- Project
- Repository
- Workspace
- Environment
- Session
- Interaction
- Task
- Agent
- Model runtime
- Tool
- Document
- Artifact
- Concept

Model runtime and agent identity are optional provenance metadata and do not belong in the normal knowledge path.

## Relationships

A memory or evidence object may be:

- `about` a person, project, system, resource, or concept.
- `learned_in` an environment or workspace.
- `learned_during` a session, interaction, or task.
- `derived_from` evidence or prior knowledge.
- `relevant_to` a context based on explicit or learned association.
- `observed_by` an agent or model runtime for audit purposes.

`learned_during` never means “owned by.” A decision made while discussing one project may remain generally useful, but project context influences retrieval relevance.

## Request context frame

Every observation and recall request may include a compact context frame:

```json
{
  "active_contexts": [
    {"type": "project", "external_key": "pma-core"},
    {"type": "repository", "external_key": "git:..."},
    {"type": "session", "external_key": "pi:..."}
  ],
  "environment": {
    "external_key": "host:workstation-a"
  },
  "source_adapter": "pi"
}
```

The adapter supplies observations. PMA resolves them to internal context records.

## Retrieval use

Context contributes to activation through:

- Exact entity match.
- Active project or repository proximity.
- Recent interaction continuity.
- Related task or goal.
- Environment compatibility for resources.

Context is one signal, not an access-control boundary.

## Provenance requirements

Every accepted knowledge object records:

- Evidence IDs.
- Operation that created or changed it.
- Extraction or consolidation run where applicable.
- Provider runtime manifest and prompt/schema version where applicable.
- Observation contexts.

Provenance is inspectable but normally excluded from the injected [ContextSlice](../04-interfaces/CONTEXT_PACKET_AND_SLICE.md).
