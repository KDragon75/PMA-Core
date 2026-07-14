# PMA-Core Learning and Recall Simulation Report

**Run date:** 2026-07-14  
**Build:** Windows MSVC Release  
**Result:** PASS

## Executive summary

The automated scenario successfully observed immutable evidence, accepted a structured learning proposal, created an auditable durable claim, built and activated a vector projection, recalled the claim through hybrid retrieval, rendered a compact ContextSlice, recorded a useful outcome, applied an explicit correction, and recalled the corrected claim as the current answer while retaining the prior claim and source evidence.

- Test cases: 1/1 passed
- Assertions: 24/24 passed
- Errors/failures/skips: 0/0/0
- Learning/recall scenario time: 0.112 seconds (JUnit)
- Qwen3 embedding smoke: 1/1 passed in 3.084 seconds

## Scenario

### 1. Observe

Input evidence stated that PMA-Core stores vectors in the core database. The episode and exact source text were committed before processing.

**Verified:** evidence remained byte-for-byte inspectable and the durable processing job succeeded.

### 2. Encode, validate, and consolidate

A deterministic simulated provider proposed an explicit-decision claim. PMA validation accepted it and committed the claim, evidence link, operation audit, and embedding event.

**Verified:** the learned claim became active and remained traceable to its source evidence.

### 3. Build retrieval projections

The simulation rendered the claim into an embedding document, built a separate vector SQLite generation, validated it, activated it, and rebuilt FTS.

**Verified:** the core remained authoritative while the vector generation acted as a projection.

### 4. Initial recall

Query: `Where does PMA-Core store vectors?`

The learned claim was selected by hybrid retrieval.

- Final fused score: 0.849999
- Selection reasons: present and inspectable in the ContextPacket
- Rendered ContextSlice estimate: 29 tokens out of an 80-token budget
- Diagnostics and internal IDs in injected slice: none

### 5. Evaluation

Before explicit evaluation, claim utility was 0.0. Recording a `used` outcome increased utility to 0.15.

**Verified:** retrieval by itself did not reinforce memory; explicit outcome evidence did.

### 6. Correction and reconsolidation input

New evidence corrected the decision: vectors use separate rebuildable SQLite projection files.

**Verified:**

- Original claim status became `superseded`.
- Corrected claim status became `active`.
- Original evidence remained unchanged and inspectable.
- Vector and FTS projections caught up after the correction.

### 7. Corrected recall

Query: `What is the current PMA-Core vector storage decision?`

The corrected claim ranked first and appeared in the rendered context.

- Corrected slice estimate: 47 tokens out of a 100-token budget
- Persisted operation audits: 2
- Persisted ContextPackets: 2
- Prior superseded claim: retained historically

## Qwen3 model verification

The local `onnx-community/Qwen3-Embedding-0.6B-ONNX` Q4 model loaded through Transformers.js and produced a finite, normalized 1024-dimensional query embedding.

**Result:** PASS

## Important scope note

The lifecycle scenario uses a deterministic scripted proposal provider and deterministic fake embeddings so semantic behavior and database mutations are reproducible. The Qwen3 runtime is tested separately as a real local embedding smoke test. A future provider-integrated simulation should route the lifecycle scenario's embedding calls through the live Node/Qwen process and add a real structured-generation model before claiming fully model-driven end-to-end learning.

## Artifacts

- `learning-recall-results.xml` — JUnit result
- `learning-recall-console.txt` — assertion-level output
- `qwen-model-smoke.txt` — real local model smoke output
