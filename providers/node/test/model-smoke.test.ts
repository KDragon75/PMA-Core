import assert from "node:assert/strict";
import test from "node:test";
import { TransformersAdapter } from "../src/index.js";

const enabled = process.env.PMA_RUN_MODEL_TEST === "1";
const localModelPath = process.env.PMA_TEST_MODEL_PATH;
test("Qwen3 embedding model smoke test", { skip: !enabled }, async () => {
  const adapter = new TransformersAdapter({
    kind: "transformers-js",
    model: process.env.PMA_TEST_EMBEDDING_MODEL ?? "onnx-community/Qwen3-Embedding-0.6B-ONNX",
    ...(localModelPath ? { localModelPath, artifactRoot: localModelPath } : {}),
    revision: "main",
    dimensions: 1024,
    pooling: "last_token",
    normalize: true,
    queryPrefix: "Instruct: Retrieve relevant persistent-memory knowledge for the query.\nQuery: ",
    documentPrefix: "",
    maxTokens: 32768,
    truncation: "end",
    localOnly: Boolean(localModelPath),
    device: "cpu",
    dtype: process.env.PMA_TEST_MODEL_DTYPE ?? "q4"
  });
  const vector = await adapter.embedQuery("How does PMA-Core store vectors?", new AbortController().signal);
  assert.equal(vector.length, 1024);
  assert.ok(vector.every(Number.isFinite));
  const norm = Math.sqrt(vector.reduce((sum, value) => sum + value * value, 0));
  assert.ok(Math.abs(norm - 1) < 0.001);
});
