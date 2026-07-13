# ADR-0006: Claims-first knowledge graph

**Status:** Accepted

## Context

Plain subject-predicate-object triples cannot adequately carry evidence, confidence, authority, time, contradiction, or revision history.

## Decision

Use first-class claims with subjects, canonical predicates, typed objects, authority, confidence, validity, and evidence. Use graph edges to connect knowledge, provenance, structure, and associations.

## Consequences

- Claims are more verbose than raw triples but auditable.
- Predicate management and entity resolution become explicit subsystems.
- Procedures and goals retain dedicated structured tables.
- Associative edges are prevented from masquerading as facts.

See [Knowledge graph](../02-architecture/KNOWLEDGE_GRAPH.md).
