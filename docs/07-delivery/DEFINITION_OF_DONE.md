# Definition of done

## Per change

A behavior change is done when:

- Code is readable and scoped to one responsibility.
- Relevant unit and integration tests pass.
- Persisted changes include migration and rollback/recovery consideration.
- Protocol changes include schemas, examples, and version impact.
- Error handling is explicit.
- New provider behavior records runtime metadata.
- Documentation and linked task acceptance criteria are updated.

## Per milestone

A milestone is done when:

- All linked tasks are complete.
- Windows and Linux CI pass.
- No graph or database invariant failures remain.
- Representative restart/recovery tests pass.
- Performance is measured, not assumed.
- Known limitations are recorded.

## First release gate

### Functional

- Full memory lifecycle works end to end.
- Every accepted memory traces to evidence or explicit imported/inferred status.
- Corrections supersede without deleting history.
- ContextSlice respects budgets and preserves qualifications.
- Provider outages do not lose evidence.

### Vector

- Compatible profile detection is strict.
- Catch-up is resumable and idempotent.
- Blue-green rebuild keeps the active store available.
- Reconstructed runtime passes conformance or is reported incompatible.

### Integration

- Pi automatically captures, recalls, injects, and queues learning.
- MCP tools and resources pass conformance and safety tests.
- Response-model changes do not alter PMA provider profiles.

### Portability

- Bundle moves between supported Windows and Linux environments.
- Old absolute paths remain historical rather than assumed valid.
- Missing vector files degrade gracefully.
- Supported migrations preserve semantic state.

### Quality

- No critical deterministic test failures.
- Long-run simulation shows no unexplained data loss.
- Retrieval quality and irrelevant-injection metrics meet release thresholds established after benchmarking.
- Dependency licenses and hashes are documented.
