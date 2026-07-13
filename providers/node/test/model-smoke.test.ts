import assert from "node:assert/strict";
import test from "node:test";
import { TransformersAdapter } from "../src/index.js";

const enabled = process.env.PMA_RUN_MODEL_TEST === "1";
test("small Transformers.js embedding model smoke test", { skip: !enabled }, async () => {
  const adapter = new TransformersAdapter({
    kind: "transformers-js",
    model: process.env.PMA_TEST_EMBEDDING_MODEL ?? "hf-internal-testing/tiny-random-BertModel",
    revision: "main",
    dimensions: 32,
    pooling: "mean",
    normalize: true,
    localOnly: false,
    device: "cpu",
    dtype: "fp32"
  });
  const vector = await adapter.embedQuery("persistent memory", new AbortController().signal);
  assert.ok(vector.length > 0);
  assert.ok(vector.every(Number.isFinite));
});
