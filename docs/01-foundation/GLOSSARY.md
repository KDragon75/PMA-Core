# Glossary

This vocabulary is normative. Code, schemas, tools, and documentation should use these terms consistently.

## Evidence and episodes

**Episode** — An immutable record of an observed interaction or source event.

**Evidence segment** — The smallest source span used to support or contradict a memory.

**Source evidence** — Original message, document, tool result, or imported record. It is not silently rewritten.

## Knowledge objects

**Entity** — A person, organization, project, system, concept, resource, or other identifiable thing.

**Claim** — A first-class semantic assertion with a subject, predicate, object or typed value, confidence, authority, status, and evidence.

**Decision** — A claim representing an explicit chosen direction.

**Preference** — A claim describing a favored option, potentially qualified by context.

**Procedure** — An ordered reusable method with explicit steps.

**Goal** — An intended future state, including unresolved work.

**Association** — A weighted retrieval relationship. It helps recall but is not itself a factual claim.

## Processing

**Candidate** — A proposed knowledge object or memory operation that has not passed validation.

**Memory operation** — A finite action such as create, support, qualify, contradict, supersede, generalize, split, or weaken.

**Consolidation** — Integrating validated observations and deriving broader semantic knowledge.

**Reconsolidation** — Revising accessibility or semantic structure after retrieval, correction, or new evidence.

**Supersession** — Marking a newer claim as the current replacement while retaining the older claim.

**Qualification** — Narrowing a claim by adding an exception or applicability condition.

## Retrieval

**Confidence** — Estimated correctness of a claim.

**Strength** — Long-term durability or importance of a memory.

**Activation** — Current accessibility of a memory for a specific request.

**Utility** — Historical evidence that recalling a memory helped or harmed later work.

**Authority** — How a memory was established, such as explicit decision, direct observation, tool result, or inference.

**ContextPacket** — Full internal retrieval result containing selected memories, scores, provenance, diagnostics, and retrieval metadata.

**ContextSlice** — Minimal rendered knowledge injected into a model context.

## Storage

**Core database** — `pma.sqlite`, the authoritative structured store.

**Embedding document** — Canonical text prepared for embedding from one retrievable knowledge object.

**Embedding profile** — Complete compatibility description for an embedding configuration.

**Vector projection** — Separate SQLite file containing vectors materialized from embedding documents under one profile and generation.

**Replay watermark** — Highest core embedding-event sequence applied to a vector projection.

## Integration

**Host** — Agent harness or application controlling lifecycle integration, such as Pi.

**Provider** — External process that performs embeddings, structured generation, or later reranking.

**PMA service protocol** — Transport-neutral JSON-RPC application interface used by host adapters and administrative clients.

**MCP adapter** — Translation layer exposing selected PMA operations as MCP tools and resources.

## Context and provenance

**Context** — A related person, project, environment, session, task, agent, document, or concept. Contexts do not own memories.

**Provenance** — Where knowledge came from and how it was processed.

**Observed location** — Environment-specific path or URI recorded as historical evidence.

**Portable reference** — Logical identity that can be resolved in another environment through repository-relative paths, hashes, revisions, aliases, or mappings.
