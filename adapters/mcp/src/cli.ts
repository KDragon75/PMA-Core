#!/usr/bin/env node
import { existsSync } from "node:fs";
import { mkdir } from "node:fs/promises";
import path from "node:path";
import readline from "node:readline";
import { ChildPmaService } from "./pma-client.js";
import { McpServer } from "./server.js";

const cwd = process.cwd();
const localCore = process.platform === "win32"
  ? path.join(cwd, "build", "windows-msvc-release", "Release", "pma-core.exe")
  : path.join(cwd, "build", "linux-gcc-release", "pma-core");
const executable = process.env.PMA_CORE_PATH ?? (existsSync(localCore) ? localCore : "pma-core");
const bundleRoot = process.env.PMA_BUNDLE_ROOT ?? path.join(cwd, ".pma");
await mkdir(bundleRoot, { recursive: true });
const pma = new ChildPmaService(executable);
await pma.start({
  protocol_version: { min: 1, max: 1 }, client: { name: "@pma/mcp-adapter", version: "0.1.0" },
  requested_capabilities: ["recall", "learning", "inspection"],
  database_path: process.env.PMA_DATABASE_PATH ?? path.join(bundleRoot, "pma.sqlite"),
  bundle_root: bundleRoot
});
const server = new McpServer(pma, process.env.PMA_MCP_ALLOW_ADMIN === "1");
const lines = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
const pending = new Set<Promise<void>>();
lines.on("line", line => {
  const operation = server.handleLine(line).then(response => {
    if (response !== undefined) process.stdout.write(`${response}\n`);
  }).catch(error => {
    console.error("MCP adapter internal failure", error);
    process.exitCode = 1;
  }).finally(() => pending.delete(operation));
  pending.add(operation);
});
lines.on("close", () => { void Promise.allSettled([...pending]).then(() => pma.close()); });
