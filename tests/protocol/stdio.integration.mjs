import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import readline from "node:readline";

const executable = process.argv[2];
assert.ok(executable, "built pma-core executable path is required");

const child = spawn(executable, [], { stdio: ["pipe", "pipe", "pipe"], windowsHide: true });
child.stdin.setDefaultEncoding("utf8");
const lines = readline.createInterface({ input: child.stdout, crlfDelay: Infinity });
const responses = [];
let stderr = "";
child.stderr.setEncoding("utf8");
child.stderr.on("data", chunk => { stderr += chunk; });
lines.on("line", line => responses.push(JSON.parse(line)));

const request = message => child.stdin.write(`${JSON.stringify(message)}\n`);
request({ jsonrpc: "2.0", id: 1, method: "pma.initialize", params: {
  protocol_version: { min: 1, max: 1 }, client: { name: "node-integration", version: "1" }
} });
request({ jsonrpc: "2.0", method: "$/cancelRequest", params: { id: "unused" } });
request({ jsonrpc: "2.0", id: 2, method: "pma.get_status" });
request({ jsonrpc: "2.0", id: 3, method: "pma.shutdown" });

const exitCode = await new Promise((resolve, reject) => {
  child.on("error", reject);
  child.on("exit", resolve);
});

assert.equal(exitCode, 0);
assert.equal(stderr, "", "healthy protocol run must not emit diagnostics");
assert.equal(responses.length, 3, "notification must not produce a response");
assert.equal(responses[0].result.protocol_version, 1);
assert.equal(responses[1].result.state, "ready");
assert.equal(responses[1].result.cancelled_request_count, 1);
assert.equal(responses[2].result.accepted, true);
for (const response of responses) {
  assert.equal(response.jsonrpc, "2.0");
}
