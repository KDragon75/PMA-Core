# Transformers.js provider

The initial first-party provider is a long-lived Node.js/TypeScript process using Transformers.js v4. It supplies local embeddings and structured generation while preserving the external-provider boundary.

## Process layout

```text
providers/node/
├── shared protocol and process host
├── transformers-js embedding capability
├── transformers-js generation capability
└── OpenAI-compatible remote capability
```

One Node package may host both local and remote adapters, but each configured provider instance reports independent runtime metadata.

## Local embedding

Use the `feature-extraction` pipeline with explicit:

- Model ID and revision.
- Local cache/model directory.
- Device and dtype.
- Pooling strategy.
- Normalization.
- Query/document prefix.
- Truncation and maximum tokens.

Do not rely on pipeline defaults without recording them in the runtime description.

## Structured generation

Transformers.js text generation does not guarantee universal schema-constrained decoding. The initial flow is:

1. Render a task-specific JSON-only prompt.
2. Generate with deterministic or low-variance settings.
3. Extract one JSON value.
4. Validate against the supplied schema.
5. Retry once with concise validation feedback.
6. Return failure without mutating PMA state if still invalid.
7. PMA-Core independently validates the proposal.

Prompts and schemas are versioned artifacts.

## Model lifecycle

- Load models lazily.
- Keep one reusable pipeline instance per effective configuration.
- Serialize or bound concurrent access based on runtime safety.
- Report load state and memory use where available.
- Support clean cancellation and shutdown.

## Local-only and reproducibility mode

Configuration must support:

- Disabling remote model downloads.
- Resolving models from a local path.
- Dedicated PMA model cache.
- Hashing required model and tokenizer files.
- Capturing package-lock and Transformers.js versions.
- Conformance embedding cases.

## Remote OpenAI-compatible adapter

Use Node's built-in `fetch` to call configured endpoints. Support initial endpoints:

```text
/v1/embeddings
/v1/chat/completions
```

Structured-output support is capability-probed. Provider-specific differences remain inside the Node adapter rather than leaking into PMA-Core.

## Node dependencies

Runtime:

- `@huggingface/transformers`
- A focused JSON Schema validator if the native validation approach is insufficient.

Development:

- TypeScript.
- Node type definitions.
- Test runner selected during bootstrap.

Avoid bundlers and web frameworks unless distribution testing proves they are necessary.

## Model selection

The project plan does not hard-code the first embedding or extraction model. Selection requires:

- Transformers.js compatibility.
- Local resource fit.
- Reproducible artifacts.
- Embedding quality and retrieval benchmarks.
- Reliable structured extraction on the PMA schema suite.
- Windows and Linux execution.

Selection is an explicit work item in [Work packages](../07-delivery/WORK_PACKAGES.md).
