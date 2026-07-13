# Task 007 — Node provider

## Objective

Implement the external provider process with local Transformers.js and remote OpenAI-compatible capabilities.

## Governing documents

- [Model providers](../docs/04-interfaces/MODEL_PROVIDERS.md)
- [Transformers.js provider](../docs/04-interfaces/TRANSFORMERS_JS_PROVIDER.md)
- [ADR-0005](../docs/decisions/ADR-0005-EXTERNAL-PROVIDERS.md)

## Deliverables

- Provider JSON-RPC process and initialization.
- Runtime/capability description.
- Transformers.js v4 feature-extraction pipeline.
- Transformers.js structured-generation pipeline.
- JSON extraction, schema validation, and one bounded retry.
- Local-only/cache configuration.
- Runtime artifact and lockfile hashing.
- OpenAI-compatible embeddings and chat-completions adapter using Node fetch.
- PMA-Core provider process manager and cancellation.

## Tests

- Protocol fixtures without models.
- One small local embedding model smoke test.
- Structured extraction fixture suite.
- Invalid generation and retry.
- Process crash/restart.
- Remote adapter against a mock HTTP server.
- Runtime fingerprint change detection.

## Acceptance criteria

- PMA-Core has no model-runtime or HTTP-client dependency.
- Provider stdout contains protocol only.
- Runtime description contains all embedding compatibility fields.
- Model selection benchmark report recommends, but does not hard-code without evidence, the initial production models.
