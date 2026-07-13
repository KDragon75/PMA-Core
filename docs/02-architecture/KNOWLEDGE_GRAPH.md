# Knowledge graph

PMA-Core uses a graph-oriented knowledge model stored in relational SQLite tables. The graph connects evidence, semantic knowledge, procedures, goals, resources, and retrieval associations without flattening every object into an untyped triple.

## Three connected layers

### Evidence layer

Immutable episodes, evidence segments, documents, and tool results answer: **Where did this come from?**

### Knowledge layer

Entities, first-class claims, decisions, preferences, procedures, goals, and semantic relationships answer: **What has been learned?**

### Activation layer

Associations, retrieval events, utility evidence, and current activation answer: **What matters now?**

## Claims are first-class

A semantic edge alone cannot represent evidence, confidence, time, authority, or contradiction. A claim therefore contains:

- Subject entity.
- Canonical predicate.
- Entity object or typed literal value.
- Polarity and modality.
- Authority and confidence.
- Strength and status.
- Validity period where known.
- Supporting and contradicting evidence.

Example:

```text
Claim: PMA-Core uses separate SQLite vector projections.
Subject: PMA-Core
Predicate: uses_vector_storage_strategy
Object: separate SQLite projection files
Authority: explicit_decision
Supported by: episode segment 17
Supersedes: prior claim that vectors belong in pma.sqlite
```

## Node types

Initial graph node types:

- Evidence segment
- Entity
- Claim
- Procedure
- Procedure step
- Goal
- Resource
- Context
- Memory operation
- Retrieval event

Each structured type receives a dedicated table. A small `graph_nodes` table supplies stable identity and common lifecycle status.

## Edge classes

- **Provenance:** `supported_by`, `contradicted_by`, `derived_from`, `learned_during`.
- **Structural:** `part_of`, `step_of`, `generalized_from`, `specialized_from`.
- **Semantic:** controlled predicates representing actual knowledge.
- **Associative:** weighted retrieval links that do not assert truth.
- **Contextual:** `about`, `relevant_to`, `observed_in`.

Traversal must preserve edge class so an association cannot be mistaken for a fact.

## Predicate registry

PMA-Core uses an extensible controlled registry:

- Canonical predicate ID.
- Human-readable name and description.
- Allowed subject and object categories.
- Inverse or symmetry behavior where applicable.
- Alias phrases used during extraction.
- Schema version and status.

Models may propose a registry match. They may not create unlimited ad hoc predicate strings. New predicates require deterministic validation and initially require explicit review.

## Entity resolution

Resolution uses exact names, aliases, nearby context, repository identity, content hash, semantic similarity, and known relationships. Ambiguous mentions remain unresolved rather than forcing a merge.

False merges are more damaging than duplicates. Automatic merge policy is therefore conservative and tested under [Functional tests](../06-testing/FUNCTIONAL_TESTS.md).

## Associations

Association edges improve recall and may strengthen through co-occurrence, repeated successful retrieval, shared evidence, or explicit semantic links. They weaken through repeated irrelevant retrieval or decay. They never become factual claims solely because they are strong.

Graph expansion is bounded by hop count, edge class, traversal cost, and candidate budget. Two hops is the provisional MVP default.

## Procedures and goals

Procedures remain ordered structures with explicit steps. Goals maintain status, temporal state, and completion evidence. Both participate in the graph but are not reduced to ordinary claims.

## Invariants

- Every accepted claim traces to evidence or is marked imported/inferred.
- Superseded claims remain available historically.
- Contradiction never silently deletes either side.
- A claim object has one subject and one object form.
- Associative edges cannot satisfy factual queries without supporting claims.
- Procedure step order is stable.
- Entity aliases do not imply identity until resolution succeeds.
