import { performance } from "node:perf_hooks";

export type HostMode = "pi-like" | "mcp-only";
export type ScenarioName = "preference-qualification" | "project-crossover" | "environment-move" | "entity-ambiguity" | "decision-reversal" | "false-inference" | "procedure-learning" | "retrieval-pollution" | "compaction" | "provider-change" | "vector-rebuild-active-use";
export type FaultName = "provider-outage" | "duplicate-event" | "sqlite-lock" | "disk-full" | "malformed-generation" | "vector-rebuild-interruption";

export interface RoleOutput { role: "user" | "response" | "extraction" | "evaluator"; model: string; promptVersion: string; content: string; latencyMs: number; }
export interface Replay { format: "pma-captured-replay-v1"; seed: number; build: string; schema: number; mode: HostMode; events: EventRecord[]; outputs: RoleOutput[]; }
export interface EventRecord { index: number; scenario: ScenarioName; fact: string; expectedRelevant: string; fault?: FaultName; }
export interface Grade { helpful: number; confusing: number; }
export interface Metrics { interactions: number; deterministicFailures: number; llmGradeAverage: number; staleMemoryRate: number; irrelevantInjectionRate: number; precisionAtK: number; recallAtK: number; coreTimeMs: number; providerWaitMs: number; recoveredFaults: number; unexplainedDataLoss: number; }
export interface Report { replay: Replay; metrics: Metrics; grades: Grade[]; deterministicAssertions: string[]; }

const scenarios: ScenarioName[] = ["preference-qualification", "project-crossover", "environment-move", "entity-ambiguity", "decision-reversal", "false-inference", "procedure-learning", "retrieval-pollution", "compaction", "provider-change", "vector-rebuild-active-use"];
const faults: FaultName[] = ["provider-outage", "duplicate-event", "sqlite-lock", "disk-full", "malformed-generation", "vector-rebuild-interruption"];

function seeded(seed: number): () => number { let x = seed >>> 0; return () => ((x = (x * 1664525 + 1013904223) >>> 0) / 0x100000000); }

export class CapturedRoles {
  private cursor = 0;
  readonly captured: RoleOutput[] = [];
  constructor(private readonly replay?: RoleOutput[]) {}
  call(role: RoleOutput["role"], content: string): RoleOutput {
    if (this.replay) {
      const output = this.replay[this.cursor++];
      if (!output || output.role !== role) throw new Error(`replay output mismatch at ${this.cursor - 1}`);
      return output;
    }
    const output = { role, model: `scripted-${role}-v1`, promptVersion: `${role}-v1`, content, latencyMs: 2 } satisfies RoleOutput;
    this.captured.push(output); return output;
  }
  assertConsumed(): void { if (this.replay && this.cursor !== this.replay.length) throw new Error("unused replay outputs"); }
}

class MemoryHost {
  private current = new Map<string, string>();
  private evidence: string[] = [];
  private seen = new Set<string>();
  constructor(readonly mode: HostMode) {}
  apply(event: EventRecord, fault?: FaultName): boolean {
    const snapshot = new Map(this.current); const evidenceLength = this.evidence.length;
    try {
      if (fault === "provider-outage" || fault === "sqlite-lock" || fault === "disk-full" || fault === "malformed-generation" || fault === "vector-rebuild-interruption") throw new Error(fault);
      const key = `${event.index}:${event.fact}`;
      if (fault === "duplicate-event" && this.seen.has(key)) return true;
      this.evidence.push(event.fact); this.current.set(event.scenario, event.fact); this.seen.add(key); return true;
    } catch { this.current = snapshot; this.evidence.length = evidenceLength; return false; }
  }
  recover(event: EventRecord): boolean { return this.apply(event); }
  recall(event: EventRecord): string[] { const relevant = this.current.get(event.scenario); return relevant ? [relevant] : []; }
  evidenceCount(): number { return this.evidence.length; }
}

export function makeEvents(seed: number, count: number): EventRecord[] {
  const random = seeded(seed); const events: EventRecord[] = [];
  for (let index = 0; index < count; index++) {
    const scenario = scenarios[index % scenarios.length]!;
    const fact = `${scenario}:current:${index}`;
    const fault = index > 0 && index % 17 === 0 ? faults[Math.floor(random() * faults.length)] : undefined;
    events.push({ index, scenario, fact, expectedRelevant: fact, ...(fault ? { fault } : {}) });
  }
  return events;
}

export function runSimulation(options: { seed: number; interactions: number; mode: HostMode; replay?: Replay }): Report {
  const events = options.replay?.events ?? makeEvents(options.seed, options.interactions);
  const roles = new CapturedRoles(options.replay?.outputs); const host = new MemoryHost(options.mode);
  let coreTimeMs = 0, providerWaitMs = 0, recoveredFaults = 0, dataLoss = 0, stale = 0, irrelevant = 0, relevant = 0;
  const assertions: string[] = []; const grades: Grade[] = [];
  for (const event of events) {
    roles.call("user", event.fact); const extraction = roles.call("extraction", event.fact); providerWaitMs += extraction.latencyMs;
    const before = host.evidenceCount(); const start = performance.now(); let applied = host.apply(event, event.fault);
    if (!applied) { if (host.evidenceCount() !== before) dataLoss++; applied = host.recover(event); if (applied) recoveredFaults++; }
    else if (event.fault === "duplicate-event") recoveredFaults++;
    coreTimeMs += performance.now() - start;
    const recalled = host.recall(event); relevant += recalled.includes(event.expectedRelevant) ? 1 : 0;
    stale += recalled.some(value => value !== event.expectedRelevant) ? 1 : 0; irrelevant += Math.max(0, recalled.length - 1);
    roles.call("response", recalled.join("\n")); const gradeOutput = roles.call("evaluator", recalled.includes(event.expectedRelevant) ? "1" : "0"); providerWaitMs += gradeOutput.latencyMs;
    grades.push({ helpful: Number(gradeOutput.content) || 0, confusing: stale ? 1 : 0 });
    if (!applied || !recalled.includes(event.expectedRelevant)) assertions.push(`interaction ${event.index}: hidden fact mismatch`);
  }
  roles.assertConsumed();
  const outputs = options.replay?.outputs ?? roles.captured;
  const replay: Replay = { format: "pma-captured-replay-v1", seed: options.seed, build: "0.1.0", schema: 1, mode: options.mode, events, outputs };
  return { replay, grades, deterministicAssertions: assertions, metrics: { interactions: events.length, deterministicFailures: assertions.length, llmGradeAverage: grades.reduce((n, g) => n + g.helpful, 0) / Math.max(1, grades.length), staleMemoryRate: stale / Math.max(1, events.length), irrelevantInjectionRate: irrelevant / Math.max(1, events.length), precisionAtK: relevant / Math.max(1, relevant + irrelevant), recallAtK: relevant / Math.max(1, events.length), coreTimeMs, providerWaitMs, recoveredFaults, unexplainedDataLoss: dataLoss } };
}

export function assertRegressionLimits(report: Report): void {
  if (report.metrics.deterministicFailures || report.metrics.unexplainedDataLoss) throw new Error("deterministic simulation failure");
  if (report.metrics.staleMemoryRate > 0.01 || report.metrics.irrelevantInjectionRate > 0.02) throw new Error("memory regression threshold exceeded");
}
