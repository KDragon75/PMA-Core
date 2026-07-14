# Task 007 Node provider

## Objective
Implement an external Node JSON-RPC provider for lazy Transformers.js inference and OpenAI-compatible endpoints, plus a restartable PMA-Core child-process client.

## Governing documents
- [Task 007](../tasks/007-NODE-PROVIDER.md)
- [Model providers](../docs/04-interfaces/MODEL_PROVIDERS.md)
- [Transformers.js provider](../docs/04-interfaces/TRANSFORMERS_JS_PROVIDER.md)
- [ADR-0005](../docs/decisions/ADR-0005-EXTERNAL-PROVIDERS.md)

## Constraints
- No model runtime or HTTP client enters the C++ core.
- Provider stdout is JSON-RPC only; diagnostics use stderr.
- Local models load lazily and local-only mode disables downloads.
- Structured generation validates, retries once, and never mutates PMA state.
- Runtime descriptions contain all vector compatibility inputs and artifact hashes.

## Current state
- `providers/node` is a TypeScript smoke package only.
- The core has no provider process manager.
- No provider protocol fixtures, model adapters, runtime hashing, or benchmark report exists.

## Work sequence
1. Define provider protocol types, runtime hashing, and JSON-RPC host.
2. Add lazy Transformers.js embedding/generation and local/cache controls.
3. Add OpenAI-compatible fetch adapter and structured JSON validation/retry.
4. Add cross-platform core process manager, cancellation, and restart tests.
5. Add protocol/remote/runtime fixtures and optional real-model smoke test.
6. Run suites, document model-selection evidence, commit, and push before Task 008.

## Tests
- Protocol initialize/health/capabilities/runtime/shutdown without models.
- Structured extraction success, invalid output, and bounded retry.
- Mock OpenAI embeddings/chat and cancellation.
- Runtime artifact/lockfile fingerprint changes.
- Child crash/restart and stdout purity.
- Small local embedding model smoke test in explicit model-test mode.

## Risks and rollback
Transformers.js packages and model artifacts are external to core. Lazy imports keep deterministic/provider-free operation available; failed child processes are isolated and restartable.

## Progress
- [x] Governing documents and state reviewed.
- [x] Provider host and adapters implemented.
- [x] Runtime hashing and structured validation implemented.
- [x] Core process manager implemented.
- [x] Deterministic acceptance suites and local Qwen model smoke passing.

## Decisions made during execution
- Pin `@huggingface/transformers` 4.2.0 and Ajv 8.20.0 in the provider lockfile.

## Completion evidence
- Added a protocol-only Node JSON-RPC host, lazy Transformers.js v4 embedding/generation pipelines, local-only/cache controls, OpenAI-compatible fetch adapter, runtime/artifact/lockfile hashing, and environment-referenced API keys.
- Structured generation extracts JSON, validates with Ajv, retries exactly once, and returns bounded diagnostics/raw output on failure.
- Added a cross-platform C++ child-process manager with stdio framing, cooperative cancellation, crash detection, and restart coverage; the core gained no HTTP or model-runtime dependency.
- Node provider tests pass 4/4, covering structured retry, mock remote embeddings/chat, artifact hash changes, protocol stdout purity, crash, and restart.
- The explicit `test:model` smoke test lazily runs the local Qwen3-Embedding-0.6B Q4 model when `PMA_RUN_MODEL_TEST=1`; it passed with 1024 finite normalized dimensions. Model artifacts remain ignored and are not downloaded in normal CI.
- Windows MSVC Debug and Release each passed 42/42 CTest cases, including process restart and cancellation.
- Existing Pi and simulation Node suites and `git diff --check` passed.
- The model-selection report recommends a benchmark process and smoke baseline without hard-coding a production model. Linux execution remains delegated to CI.
