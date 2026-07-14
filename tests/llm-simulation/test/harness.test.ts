import assert from "node:assert/strict";
import test from "node:test";
import { assertRegressionLimits, makeEvents, runSimulation } from "../src/harness.js";

test("all required hidden-world scenarios are generated", () => {
  assert.equal(new Set(makeEvents(1, 11).map(event => event.scenario)).size, 11);
});

test("captured provider output reproduces deterministic results", () => {
  const live = runSimulation({ seed: 42, interactions: 80, mode: "pi-like" });
  const replay = runSimulation({ seed: 999, interactions: 1, mode: "pi-like", replay: live.replay });
  assert.deepEqual({ ...replay.metrics, coreTimeMs: 0 }, { ...live.metrics, coreTimeMs: 0 });
  assert.deepEqual(replay.deterministicAssertions, live.deterministicAssertions);
});

test("host modes preserve hidden facts independently from qualitative grades", () => {
  for (const mode of ["pi-like", "mcp-only"] as const) {
    const report = runSimulation({ seed: 7, interactions: 100, mode });
    assertRegressionLimits(report);
    assert.equal(report.metrics.deterministicFailures, 0);
    assert.equal(report.metrics.recallAtK, 1);
    assert.ok(report.metrics.providerWaitMs > report.metrics.coreTimeMs);
  }
});

test("controlled faults recover without unexplained data loss", () => {
  const report = runSimulation({ seed: 9, interactions: 200, mode: "pi-like" });
  assert.ok(report.metrics.recoveredFaults >= 10);
  assert.equal(report.metrics.unexplainedDataLoss, 0);
  assert.equal(report.metrics.staleMemoryRate, 0);
  assert.equal(report.metrics.irrelevantInjectionRate, 0);
});
