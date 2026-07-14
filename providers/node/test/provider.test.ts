import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { once } from "node:events";
import { mkdtemp, rm, writeFile } from "node:fs/promises";
import { createServer } from "node:http";
import os from "node:os";
import path from "node:path";
import readline from "node:readline";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { generateStructured, hashArtifacts, OpenAIAdapter } from "../src/index.js";

const remoteConfig = { kind: "openai-compatible" as const, model: "mock", dimensions: 3,
  baseUrl: "http://127.0.0.1" };

test("structured generation extracts, validates, and retries once", async () => {
  let calls = 0;
  const result = await generateStructured("fixture", {
    type: "object", required: ["claim"], properties: { claim: { type: "string" } }, additionalProperties: false
  }, async () => ++calls === 1 ? "not json" : "prefix {\"claim\":\"valid\"} suffix");
  assert.deepEqual(result.value, { claim: "valid" });
  assert.equal(result.attempts, 2);
  assert.equal(calls, 2);
  await assert.rejects(generateStructured("fixture", { type: "integer" }, async () => "{}"),
    /invalid after retry/);
});

test("OpenAI adapter uses embeddings and chat completion endpoints", async () => {
  const requests: string[] = [];
  const server = createServer((request, response) => {
    requests.push(request.url ?? "");
    response.setHeader("content-type", "application/json");
    response.end(request.url === "/v1/embeddings"
      ? JSON.stringify({ data: [{ index: 0, embedding: [1, 0, 0] }] })
      : JSON.stringify({ choices: [{ message: { content: "{\"ok\":true}" } }] }));
  });
  server.listen(0, "127.0.0.1");
  await once(server, "listening");
  const address = server.address();
  assert.ok(address && typeof address === "object");
  const adapter = new OpenAIAdapter({ ...remoteConfig, baseUrl: `http://127.0.0.1:${address.port}/v1/` });
  const signal = new AbortController().signal;
  assert.deepEqual(await adapter.embedQuery("query", signal), [1, 0, 0]);
  assert.equal(await adapter.generate("prompt", signal), "{\"ok\":true}");
  assert.deepEqual(requests, ["/v1/embeddings", "/v1/chat/completions"]);
  server.close();
  await once(server, "close");
});

test("runtime artifact hashing detects changes", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pma-provider-hash-"));
  await writeFile(path.join(directory, "model.bin"), "one");
  const first = await hashArtifacts(directory);
  await writeFile(path.join(directory, "model.bin"), "two");
  assert.notEqual(await hashArtifacts(directory), first);
  await rm(directory, { recursive: true });
});

test("provider process protocol is clean and restartable without loading models", async () => {
  async function run(crash: boolean): Promise<void> {
    const child = spawn(process.execPath, [fileURLToPath(new URL("../src/cli.js", import.meta.url))],
      { stdio: ["pipe", "pipe", "pipe"] });
    child.stdin.on("error", () => { /* expected when crash test closes the pipe */ });
    const lines = readline.createInterface({ input: child.stdout, crlfDelay: Infinity });
    const responses: unknown[] = [];
    let stderr = "";
    child.stderr.setEncoding("utf8");
    child.stderr.on("data", chunk => { stderr += chunk; });
    lines.on("line", line => responses.push(JSON.parse(line)));
    child.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", id: 1, method: "provider.initialize",
      params: { config: remoteConfig } })}\n`);
    child.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", id: 2, method: "provider.get_capabilities" })}\n`);
    if (crash) child.kill();
    else child.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", id: 3, method: "provider.shutdown" })}\n`);
    const [code] = await once(child, "exit") as [number | null];
    if (!crash) {
      assert.equal(code, 0);
      assert.equal(stderr, "");
      assert.equal(responses.length, 3);
    }
  }
  await run(true);
  await run(false);
});
