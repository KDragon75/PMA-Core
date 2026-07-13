# PMA-Core execution plans

Use an execution plan for changes that span multiple components, require migrations, alter a protocol, or cannot be completed and verified in one focused pass.

## Plan template

```markdown
# <Short plan title>

## Objective
What observable outcome will exist when this plan is complete?

## Governing documents
- [Relevant architecture document](...)
- [Relevant task](...)
- [Relevant decision record](...)

## Constraints
List locked decisions, compatibility requirements, and explicit exclusions.

## Current state
Describe only facts verified in the repository.

## Work sequence
1. Small independently testable step.
2. Next step.
3. Migration or integration step.

## Tests
List exact unit, integration, migration, and performance checks.

## Risks and rollback
State what can fail and how to restore the previous working state.

## Progress
- [ ] Step one
- [ ] Step two

## Decisions made during execution
Record decisions that were not already established. Promote durable decisions into `docs/decisions/`.

## Completion evidence
Commands run, tests passed, relevant output, and remaining limitations.
```

## Plan rules

- Keep plans in `plans/` once implementation begins.
- Link the active task and all affected specifications.
- Update progress as work is performed; do not reconstruct progress afterward.
- Do not use a plan to bypass an unresolved architectural decision.
- Split the plan when independent deliverables can be implemented and reviewed separately.
