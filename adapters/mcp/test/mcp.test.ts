import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { once } from "node:events";
import { existsSync } from "node:fs";
import { mkdtemp, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import readline from "node:readline";
import test from "node:test";
import { McpServer } from "../src/server.js";
import type { PmaService } from "../src/pma-client.js";

class MockPma implements PmaService {
  calls: Array<{ method: string; params?: Record<string, unknown> }> = [];
  async request<T>(method: string, params?: Record<string, unknown>): Promise<T> {
    this.calls.push(params ? { method, params } : { method });
    return { method, ...params } as T;
  }
  async close(): Promise<void> {}
}

async function send(server: McpServer, id: number, method: string, params: Record<string, unknown> = {}) {
  const line = await server.handleLine(JSON.stringify({ jsonrpc: "2.0", id, method, params }));
  assert.ok(line);
  return JSON.parse(line) as Record<string, any>;
}
async function initialize(server: McpServer) {
  return send(server, 1, "initialize", { protocolVersion: "2025-11-25", capabilities: {},
    clientInfo: { name: "test", version: "1" } });
}

test("MCP initialization exposes versioned safe capabilities", async () => {
  const pma = new MockPma();
  const server = new McpServer(pma);
  const response = await initialize(server);
  assert.equal(response.result.protocolVersion, "2025-11-25");
  assert.match(response.result.instructions, /current explicit user corrections/);
  const tools = await send(server, 2, "tools/list");
  assert.ok(tools.result.tools.some((tool: { name: string }) => tool.name === "pma_learn"));
  assert.ok(!tools.result.tools.some((tool: { name: string }) => tool.name === "pma_delete_evidence"));
  const resources = await send(server, 3, "resources/templates/list");
  assert.equal(resources.result.resourceTemplates[0].uriTemplate, "pma://memory/{id}");
});

test("safe tools validate and translate into PMA services", async () => {
  const pma = new MockPma();
  const server = new McpServer(pma);
  await initialize(server);
  const learned = await send(server, 2, "tools/call", { name: "pma_learn",
    arguments: { statement: "SQLite is authoritative", authority: "explicit_user" } });
  assert.equal(learned.result.isError, false);
  assert.deepEqual(pma.calls.at(-1), { method: "pma.learning.propose",
    params: { statement: "SQLite is authoritative", authority: "explicit_user" } });
  const invalid = await send(server, 3, "tools/call", { name: "pma_learn", arguments: {} });
  assert.equal(invalid.error.code, -32602);
  const unknown = await send(server, 4, "tools/call", { name: "pma_recall",
    arguments: { query: "x", surprise: true } });
  assert.equal(unknown.error.code, -32602);
});

test("administrative tools are denied by default", async () => {
  const server = new McpServer(new MockPma());
  await initialize(server);
  const response = await send(server, 2, "tools/call", { name: "pma_delete_evidence",
    arguments: { id: "evidence:1", confirmation: "yes" } });
  assert.equal(response.error.code, -32003);
  assert.equal(response.error.data.code, "PMA_PERMISSION_DENIED");
});

test("resources and compaction tools preserve explicit MCP semantics", async () => {
  const pma = new MockPma();
  const server = new McpServer(pma);
  await initialize(server);
  const resource = await send(server, 2, "resources/read", { uri: "pma://memory/claim%3A1" });
  assert.equal(resource.result.contents[0].mimeType, "application/json");
  assert.deepEqual(pma.calls.at(-1), { method: "pma.memory.get", params: { id: "claim:1" } });
  await send(server, 3, "tools/call", { name: "pma_prepare_compaction", arguments: { summary: "state" } });
  assert.equal(pma.calls.at(-1)?.method, "pma.learning.consolidate");
  await send(server, 4, "tools/call", { name: "pma_restore_after_compaction", arguments: {} });
  assert.equal(pma.calls.at(-1)?.method, "pma.recall");
});

const builtCore = process.platform === "win32"
  ? path.resolve(process.cwd(), "..", "..", "build", "windows-msvc-release", "Release", "pma-core.exe")
  : path.resolve(process.cwd(), "..", "..", "build", "linux-gcc-release", "pma-core");
test("stdio MCP host performs real learn recall and resource inspection", { skip: !existsSync(builtCore) }, async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "pma-mcp-e2e-"));
  const core = builtCore;
  const cli = path.resolve(process.cwd(), "dist", "src", "cli.js");
  const child = spawn(process.execPath, [cli], { stdio: ["pipe", "pipe", "pipe"],
    env: { ...process.env, PMA_CORE_PATH: core, PMA_BUNDLE_ROOT: root } });
  child.stdin.setDefaultEncoding("utf8");
  let stderr = "";
  child.stderr.setEncoding("utf8");
  child.stderr.on("data", chunk => { stderr += chunk; });
  const lines = readline.createInterface({ input: child.stdout, crlfDelay: Infinity });
  const waiters: Array<(value: any) => void> = [];
  lines.on("line", line => waiters.shift()?.(JSON.parse(line)));
  const request = (id: number, method: string, params: Record<string, unknown> = {}) =>
    new Promise<any>(resolve => { waiters.push(resolve); child.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", id, method, params })}\n`); });
  assert.equal((await request(1, "initialize", { protocolVersion: "2025-11-25",
    clientInfo: { name: "e2e", version: "1" }, capabilities: {} })).result.protocolVersion, "2025-11-25");
  const learned = await request(2, "tools/call", { name: "pma_learn",
    arguments: { statement: "MCP explicit memory survives" } });
  assert.equal(learned.result.isError, false);
  const claimId = learned.result.structuredContent.claim_id as string;
  const memory = await request(3, "resources/read", { uri: `pma://memory/${encodeURIComponent(claimId)}` });
  assert.match(memory.result.contents[0].text, /active/);
  const recalled = await request(4, "tools/call", { name: "pma_recall",
    arguments: { query: "MCP explicit memory" } });
  assert.match(recalled.result.content[0].text, /selected_count/);
  child.stdin.end();
  await once(child, "exit");
  assert.equal(stderr, "");
  await rm(root, { recursive: true });
});
