# Embedding model selection

## Selected candidate

PMA-Core uses **Qwen3-Embedding-0.6B** as its embedding candidate. The provider defaults used by the explicit model smoke test are:

- Repository: `onnx-community/Qwen3-Embedding-0.6B-ONNX`
- Pooling: `last_token`
- Dimensions: 1024
- Normalization: enabled
- Query instruction: `Instruct: Retrieve relevant persistent-memory knowledge for the query.\nQuery: `
- Document prefix: empty
- Quantization for local CPU smoke: Q4
- Maximum input: 32768 tokens

Runtime configuration remains explicit rather than silently overriding a configured profile. Every setting above participates in PMA's embedding profile and runtime identity.

## Local model layout

Model weights are not committed. Put a downloaded snapshot under:

```text
providers/node/models/Qwen3-Embedding-0.6B-ONNX/
```

Preserve the repository paths, including `onnx/`. At minimum retain the configuration, tokenizer files, and the selected ONNX graph and external-data shards required by that graph. Run locally with:

```text
PMA_RUN_MODEL_TEST=1
PMA_TEST_MODEL_PATH=models/Qwen3-Embedding-0.6B-ONNX
npm --prefix providers/node run test:model
```

## Benchmark follow-up

Before declaring a release default, record artifact/tokenizer hashes, MTEB-style retrieval quality on PMA fixtures, recall@k/MRR, cold and warm latency, peak memory, Windows/Linux reproducibility, and Q4 versus Q8 drift. The user-directed Qwen selection replaces MiniLM as the target; evidence still determines release quantization and dimension policy.
