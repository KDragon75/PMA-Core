import assert from "node:assert/strict";
import test from "node:test";
import { providerPackageName } from "../src/index.js";

test("provider package loads without a model download", () => {
  assert.equal(providerPackageName, "@pma/node-provider");
});
