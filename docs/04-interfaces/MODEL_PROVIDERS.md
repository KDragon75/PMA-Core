# Model providers

PMA-Core never loads model weights or inference runtimes. All model work occurs in external provider processes.

## Provider process boundary

```text
PMA-Core
    -> provider JSON-RPC over stdio
Node provider
    -> Transformers.js local inference
    -> or OpenAI-compatible remote endpoint
```

Using one provider-process protocol keeps local and remote model access outside the C++ core and avoids a direct HTTP dependency in the MVP.

## Initial capabilities

### Embeddings

- Describe effective runtime.
- Embed documents in batches.
- Embed one query.
- Report dimensions, pooling, normalization, prefixes, truncation, and artifacts.

### Structured generation

- Describe runtime.
- Generate JSON for a named PMA task.
- Validate output against the requested schema.
- Return raw output and validation diagnostics on failure.

### Later capability

Reranking remains optional and is added only after deterministic hybrid retrieval is measured.

## Required provider methods

```text
provider.initialize
provider.describe_runtime
provider.get_capabilities
provider.health
provider.cancel
provider.shutdown
embedding.embed_documents
embedding.embed_query
generation.generate_structured
```

## Runtime identity

The provider returns enough information to create a reconstructable manifest:

- Provider package and version.
- Runtime and Node versions.
- OS, architecture, device, and backend.
- Model repository, revision, and artifact hashes.
- Tokenizer hashes.
- Lockfile or package manifest hash.
- Inference options affecting output.

PMA-Core computes the compatibility fingerprint and owns the vector-store registry.

## Failure behavior

- Provider unavailable: queue work and continue deterministic features.
- Invalid structured output: retry according to policy, then fail the job without knowledge mutation.
- Profile change: refuse incompatible vector queries and plan a new projection.
- Partial embedding batch: commit no watermark beyond successfully stored vectors.

## Provider configuration

Non-secret configuration is stored in SQLite. Secrets remain external and are referenced symbolically, for example `env:PMA_OPENAI_API_KEY`.

## Deferred provider options

- Additional provider executables in other languages.
- Direct llama-server or Ollama adapters.
- Inbound HTTP provider protocol.
- Host-supplied sampling.
- In-process inference.

The provider protocol should remain small enough that these can be added without changing memory semantics.
