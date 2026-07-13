# ADR-0007: Deterministic tests remain authoritative

**Status:** Accepted

## Context

LLMs are useful for generating realistic conversations and evaluating nuanced usefulness, but they are nondeterministic and can agree with their own errors.

## Decision

Use deterministic code and hidden scenario state as the authority for evidence, graph, migration, replay, and known-fact correctness. Use independent LLM roles for workload generation and qualitative scoring.

## Consequences

- Fake providers and captured replay are mandatory.
- Stress runs remain reproducible enough to debug.
- LLM grades supplement rather than replace hard assertions.
- Multiple model roles or families are preferred.

See [Test strategy](../06-testing/TEST_STRATEGY.md).
