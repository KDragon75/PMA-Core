# Initial model selection benchmark status

No production model is hard-coded. The current recommendation process compares Transformers.js v4 compatibility, artifact reproducibility, Windows/Linux execution, local memory and latency, retrieval quality, and structured-output validity.

## Current smoke candidate

`Xenova/all-MiniLM-L6-v2` is a small embedding baseline suitable for smoke and latency measurements, not a final production selection. `test:model` accepts `PMA_TEST_EMBEDDING_MODEL` so candidates can be compared without source changes.

## Required report fields

For every candidate record revision and artifact hashes, tokenizer hash, dimensions, pooling/prefix settings, corpus and relevance labels, recall@k/MRR, cold/warm latency, peak memory, structured fixture pass rate where applicable, and Windows/Linux reproducibility. Select production defaults only after this evidence is checked in.
