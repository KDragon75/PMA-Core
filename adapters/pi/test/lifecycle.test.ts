import assert from "node:assert/strict";
import { mkdtemp, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { LifecycleController, PmaClient, type RpcClient } from "../src/index.js";

interface Call { method: string; params: Record<string, unknown> | undefined; notification?: boolean }
class MockClient implements RpcClient {
  calls: Call[] = [];
  started = false;
  stopped = false;
  async start(): Promise<void> { this.started = true; }
  async stop(): Promise<void> { this.stopped = true; }
  async request<T>(method: string, params?: Record<string, unknown>): Promise<T> {
    this.calls.push({ method, params });
    if (method === "pma.recall") return { packet_id: "packet:1", slice: "<PMA-memory>remembered</PMA-memory>" } as T;
    return { ok: true } as T;
  }
  notify(method: string, params?: Record<string, unknown>): void {
    this.calls.push({ method, params, notification: true });
  }
}

const session = { sessionId: "pi:session:1", sessionFile: "session.jsonl", cwd: "/workspace",
  repository: "git:repo", environment: "host:test" };

test("automatic lifecycle recalls once, injects once, and learns only after settled", async () => {
  const client = new MockClient();
  const controller = new LifecycleController(client);
  await controller.startSession(session);
  assert.equal(client.started, true);
  assert.equal(await controller.beforeAgentStart("continue the project"),
    "<PMA-memory>remembered</PMA-memory>");
  assert.equal(client.calls.filter(call => call.method === "pma.recall").length, 1);

  const firstContext = controller.injectContext([{ role: "user", content: "prompt" }]);
  const secondContext = controller.injectContext(firstContext);
  assert.equal(secondContext.filter(message => message.customType === "pma-context").length, 1);
  assert.equal(client.calls.filter(call => call.method === "pma.recall").length, 1);

  controller.observeMessage({ role: "assistant", content: [{ type: "text", text: "answer" }] }, "entry:a");
  assert.equal(client.calls.some(call => call.method === "pma.learning.process_interaction"), false);
  await controller.agentSettled();
  assert.equal(client.calls.some(call => call.method === "pma.interaction.complete"), true);
  assert.equal(client.calls.some(call => call.method === "pma.learning.process_interaction"), true);
  assert.equal(controller.currentSlice, "");
});

test("parallel results preserve source invocation order and compaction flushes", async () => {
  const client = new MockClient();
  const controller = new LifecycleController(client);
  await controller.startSession(session);
  await controller.beforeAgentStart("run tools");
  controller.observeMessage({ role: "toolResult", toolCallId: "second-finished", content: "source first" }, "entry:1");
  controller.observeMessage({ role: "toolResult", toolCallId: "first-finished", content: "source second" }, "entry:2");
  await controller.beforeCompaction("Pi compacted summary");
  const observations = client.calls.filter(call => call.method === "pma.observe.event");
  assert.deepEqual(observations.slice(-3).map(call => call.params?.text),
    ["source first", "source second", "Pi compacted summary"]);
  assert.equal(client.calls.some(call => call.method === "pma.learning.consolidate"), true);
});

test("resume reconciles branch entries and model selection is provenance only", async () => {
  const client = new MockClient();
  const controller = new LifecycleController(client);
  await controller.startSession({ ...session, sessionId: "pi:resumed" }, [
    { id: "entry:old-user", message: { role: "user", content: "old prompt" } },
    { id: "entry:old-tool", message: { role: "toolResult", content: "old result" } }
  ]);
  const reconciled = client.calls.filter(call => call.method === "pma.observe.event");
  assert.equal(reconciled.length, 2);
  assert.deepEqual(reconciled.map(call => call.params?.source_id), ["entry:old-user", "entry:old-tool"]);
  controller.modelSelected("anthropic", "response-model");
  const modelCall = client.calls.at(-1);
  assert.equal(modelCall?.notification, true);
  assert.equal(modelCall?.method, "pma.observe.event");
  assert.equal(client.calls.some(call => call.method.includes("provider.initialize")), false);
  await controller.shutdown();
  assert.equal(client.stopped, true);
});

test("child client restarts after a process crash", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pma-pi-client-"));
  const script = path.join(directory, "fake.mjs");
  await writeFile(script, `
    import readline from "node:readline";
    const lines=readline.createInterface({input:process.stdin,crlfDelay:Infinity});
    lines.on("line", line => { const r=JSON.parse(line); if(r.method==="crash") process.exit(9);
      if(r.method==="pma.shutdown"){process.stdout.write(JSON.stringify({jsonrpc:"2.0",id:r.id,result:{ok:true}})+"\\n");lines.close();return;}
      process.stdout.write(JSON.stringify({jsonrpc:"2.0",id:r.id,result:{status:"ok"}})+"\\n"); });
  `);
  const client = new PmaClient(process.execPath, [script]);
  await client.start();
  assert.deepEqual(await client.request("health"), { status: "ok" });
  await assert.rejects(client.request("crash"), /exited/);
  assert.deepEqual(await client.request("health"), { status: "ok" });
  await client.stop();
  await rm(directory, { recursive: true });
});
