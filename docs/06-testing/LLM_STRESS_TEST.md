# LLM-driven stress test

The LLM stress harness simulates long, messy, realistic use while a deterministic controller maintains hidden ground truth.

## Architecture

```text
Scenario controller
├── hidden world state
├── simulated user LLM
├── simulated host/agent
├── PMA-Core
├── extraction/embedding providers
├── deterministic assertions
└── independent LLM evaluator
```

## Hidden world state

The controller owns authoritative scenario facts:

- Preferences and exceptions.
- Project decisions.
- Entities and aliases.
- Procedures.
- Active and abandoned goals.
- Temporary and superseded facts.
- Resource identities and environment mappings.

The user LLM receives only the narrative information needed for its role. It cannot directly mutate ground truth.

## User behavior

Generate interactions that:

- Mix multiple overlapping projects.
- State and later qualify preferences.
- Reverse decisions.
- Refer vaguely to prior work.
- Introduce similarly named entities.
- Move between Windows and Linux paths.
- Resume old tasks after long gaps.
- Provide transient details that should not become durable memory.
- Explicitly request remembering or forgetting.
- Correct false inferences.
- Trigger compaction and session forks.

## Host modes

Run the same scenarios under:

1. Full Pi-like lifecycle integration.
2. MCP-only integration with system-prompt guidance.
3. Provider outage and recovery.
4. Embedding-profile change during continued use.
5. Bundle migration to a new environment.

## Scenario tiers

```text
Smoke: 20–50 interactions
Daily: 500–1,000 interactions
Long run: 10,000+ interactions
Soak: continuous repeated worlds with faults and database growth
```

## Scoring

### Deterministic

- Expected claims and current decisions.
- Correct supersession and qualification.
- Evidence traceability.
- Entity resolution.
- Goal restoration.
- Wrong-context exclusion.
- Vector and migration invariants.

### Retrieval metrics

- Precision@k and Recall@k.
- Irrelevant-injection rate.
- Qualification-preservation rate.
- Context tokens per successful task.
- Harmful stale-memory rate.

### LLM evaluator

An independent model scores whether the ContextSlice and final response were useful, consistent, properly qualified, and free of memory-induced confusion. The evaluator does not see PMA's internal confidence or utility scores before judging.

## Avoiding circular evaluation

Prefer separate roles or model families for:

- Simulated user.
- PMA extraction.
- Response agent.
- Evaluator.

At minimum, isolate prompts and contexts. Critical scenarios use deterministic expected results even when qualitative grading is also present.

## Fault injection

Random controlled events include:

- Kill PMA-Core.
- Kill provider process.
- Duplicate or delay host events.
- Interrupt vector rebuild.
- Remove a vector file.
- Change model behind an endpoint.
- Temporarily lock SQLite.
- Simulate disk-full failure.
- Corrupt a copied bundle.
- Return malformed structured generation.

Every fault has an expected recovery state.

## Replay

- **Live replay:** rerun models with recorded seed and configuration.
- **Captured replay:** use saved model outputs to reproduce PMA behavior deterministically.

Captured replay is required for regression debugging.
