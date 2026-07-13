import assert from "node:assert/strict";
import test from "node:test";
import { adapterPackageName } from "../src/index.js";

test("Pi adapter package loads", () => {
  assert.equal(adapterPackageName, "@pma/pi-adapter");
});
