import assert from "node:assert/strict";
import test from "node:test";
import { deterministicFixture } from "../src/index.js";

test("simulation harness smoke test is offline", () => {
  assert.equal(deterministicFixture(), "offline");
});
