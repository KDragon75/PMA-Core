# Task 004 — Evidence and knowledge graph

## Objective

Implement deterministic storage and inspection for evidence and graph primitives.

## Governing documents

- [Knowledge graph](../docs/02-architecture/KNOWLEDGE_GRAPH.md)
- [Context and provenance](../docs/02-architecture/CONTEXT_AND_PROVENANCE.md)
- [Portable references](../docs/02-architecture/PORTABLE_REFERENCES.md)
- [ADR-0006](../docs/decisions/ADR-0006-CLAIMS-FIRST-GRAPH.md)

## Deliverables

- Migrations for episodes, evidence segments, nodes, entities, aliases, contexts, resources, locations, predicates, typed values, claims, evidence links, procedures, steps, goals, and edges.
- Deterministic create/get/search application services.
- Initial controlled predicate registry fixture.
- Graph verification routines.
- Portable resource-location representation.

## Tests

- Evidence ordering and idempotency.
- Claim object/value exclusivity.
- Alias without automatic identity merge.
- Procedure order.
- Supersession and contradiction link storage.
- Environment-specific absolute paths and repository-relative locations.
- Graph invariant failures.

## Acceptance criteria

- All objects are inspectable through typed application results.
- No memory ownership fields exist.
- Model/agent identity is optional provenance only.
- Graph verification catches orphan, invalid, and structurally impossible records.
