# Task 004 evidence and knowledge graph

## Objective
Add deterministic, inspectable evidence and claims-first graph storage with portable resource locations and invariant verification.

## Governing documents
- [Task 004](../tasks/004-EVIDENCE-AND-GRAPH.md)
- [Knowledge graph](../docs/02-architecture/KNOWLEDGE_GRAPH.md)
- [Context and provenance](../docs/02-architecture/CONTEXT_AND_PROVENANCE.md)
- [Portable references](../docs/02-architecture/PORTABLE_REFERENCES.md)
- [ADR-0006](../docs/decisions/ADR-0006-CLAIMS-FIRST-GRAPH.md)

## Constraints
- Claims are first-class and have exactly one object form.
- Accepted knowledge traces to evidence or is explicitly imported/inferred.
- Aliases never merge identities automatically.
- Absolute paths are environment-scoped; repository-relative locations remain portable.
- No ownership columns or required model/agent identity.

## Current state
- Task 002 provides typed SQLite wrappers and checksummed migrations.
- Only bootstrap metadata tables exist.
- No evidence, graph, predicate, resource, procedure, or goal storage exists.

## Work sequence
1. Add the immutable core graph migration and controlled predicate fixture.
2. Add typed deterministic create/get/search services for evidence and graph primitives.
3. Add portable resource location and ordered procedure operations.
4. Extend verification with graph invariants.
5. Add acceptance tests, run all suites, commit, and push Task 004 before Task 005.

## Tests
- Evidence ordering and idempotency.
- Claim object/value exclusivity and provenance.
- Alias ambiguity without identity merge.
- Procedure step ordering.
- Supersession and contradiction edges.
- Environment-scoped absolute and repository-relative paths.
- Orphan and structurally invalid graph verification.

## Risks and rollback
The migration is immutable once released. Schema constraints and verification queries are tested before the task commit; rollback before release is removal of migration version 100 and graph services.

## Progress
- [x] Governing documents and state reviewed.
- [x] Graph migration implemented.
- [x] Typed services implemented.
- [x] Graph verification implemented.
- [x] Acceptance suites passing on Windows; Linux execution remains CI-validated.

## Decisions made during execution
- Callers provide stable text IDs; deterministic create methods use `INSERT OR IGNORE` only where an explicit idempotency contract exists.

## Completion evidence
- Added immutable migrations 100 and 101 for evidence, graph nodes, entities/aliases, contexts, resources/locations, predicates, typed values, claims/evidence, procedures/steps, goals, and classified edges.
- Added typed create/get/search services and a controlled five-predicate fixture.
- Graph verification reports foreign-key orphans, missing typed node shapes, evidence claims without support, and non-contiguous procedure ordering.
- Windows MSVC Debug and Release each passed 24/24 CTest cases.
- Tests cover evidence ordering/idempotency, object exclusivity, ambiguous aliases, procedure order, contradiction/supersession, portable locations, typed inspection, and deliberate invariant failures.
- All three Node package suites passed.
- `git diff --check` passed, source scan found no ownership fields, and VS Code reported no diagnostics.
- Linux execution remains delegated to the checked-in CI matrix.
