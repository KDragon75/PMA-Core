# Open questions

Open questions should not be silently resolved during implementation. Link the task or plan that answers each question and add a decision record when the answer affects architecture.

## Knowledge model

1. **Predicate registry growth** — Which predicates ship initially, and what threshold allows creation of a new canonical predicate?
2. **Temporal representation** — Are `valid_from` and `valid_until` sufficient for the first release, or is a richer event-time model required?
3. **Generalization threshold** — How many independent observations are required before automatic generalization?
4. **Entity merge policy** — Which exact-match cases may merge automatically, and which require an unresolved alias or review candidate?
5. **Goal lifecycle** — When does an unresolved goal become inactive, abandoned, or completed?

## Retrieval

6. **Score fusion** — Which deterministic baseline formula should combine FTS5, vectors, graph proximity, authority, strength, and utility?
7. **Graph expansion** — Is two hops the correct default, and should hop limits vary by edge class?
8. **ContextSlice labels** — Which authority or conflict labels are worth their token cost?
9. **Evaluation confidence** — What evidence is sufficient to mark a recalled memory useful versus merely present?

## Providers

10. **Initial embedding model** — Select through benchmark and compatibility testing; do not hard-code based only on popularity.
11. **Initial extraction model** — Select a Transformers.js-compatible model that reliably produces bounded JSON under local resource constraints.
12. **GPU policy** — Whether WebGPU is enabled by default or opt-in after cross-platform reliability testing.
13. **Remote provider parity** — Which OpenAI-compatible structured-output variations need dedicated adapters?

## Operations and UX

14. **User review interface** — Initial release may expose CLI and MCP inspection; a dedicated UI is deferred.
15. **Deletion semantics** — Exact behavior for deleting evidence that supports derived claims requires a formal policy.
16. **Background scheduling** — Whether PMA-Core owns a persistent maintenance scheduler or only runs queued work while a host is connected.
17. **Bundle encryption** — Deferred until threat model and key-management requirements are clear.

## Testing and release

18. **Performance gates** — Replace provisional targets with measured release thresholds after the first end-to-end prototype.
19. **Stress-test model diversity** — Define the minimum independent model families needed to reduce circular evaluation.
20. **Supported platform matrix** — Windows x64 and Linux x64 are initial; ARM64 support remains a later decision.
