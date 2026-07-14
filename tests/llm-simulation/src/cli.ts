import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname } from "node:path";
import { assertRegressionLimits, runSimulation, type HostMode, type Replay } from "./harness.js";

const args = new Map<string, string>();
for (let i = 2; i + 1 < process.argv.length; i += 2) args.set(process.argv[i]!, process.argv[i + 1]!);
const replayPath = args.get("--replay");
const captured = replayPath ? JSON.parse(await readFile(replayPath, "utf8")) as Replay | { replay: Replay } : undefined;
const replay = captured && "replay" in captured ? captured.replay : captured;
const report = runSimulation({ seed: Number(args.get("--seed") ?? replay?.seed ?? 11011), interactions: Number(args.get("--interactions") ?? replay?.events.length ?? 50), mode: (args.get("--mode") ?? replay?.mode ?? "pi-like") as HostMode, ...(replay ? { replay } : {}) });
assertRegressionLimits(report);
const output = args.get("--output");
if (output) {
  await mkdir(dirname(output), { recursive: true });
  await writeFile(output, `${JSON.stringify(report, null, 2)}\n`);
}
console.log(JSON.stringify(report.metrics));
