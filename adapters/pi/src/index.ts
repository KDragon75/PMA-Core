import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import { Type } from "typebox";
import { existsSync } from "node:fs";
import { mkdir } from "node:fs/promises";
import { resolve } from "node:path";
import { LifecycleController, environmentKey, type MessageLike } from "./controller.js";
import { PmaClient } from "./client.js";

export const adapterPackageName = "@pma/pi-adapter";
const promptFragment = "PMA supplies recalled context automatically. Use PMA tools only for explicit recall, durable learning, correction, or inspection; current user instructions always take priority.";

export default function pmaExtension(pi: ExtensionAPI): void {
  const localCore = defaultCorePath(process.cwd());
  const executable = process.env.PMA_CORE_PATH ?? (existsSync(localCore) ? localCore : "pma-core");
  const client = new PmaClient(executable);
  const controller = new LifecycleController(client);

  pi.on("session_start", async (_event, ctx) => {
    let repository: string | undefined;
    await mkdir(resolve(ctx.cwd, ".pma"), { recursive: true });
    const remote = await pi.exec("git", ["config", "--get", "remote.origin.url"], { cwd: ctx.cwd });
    if (remote.code === 0) repository = remote.stdout.trim() || undefined;
    const sessionFile = ctx.sessionManager.getSessionFile();
    await controller.startSession({
      sessionId: ctx.sessionManager.getSessionId(),
      ...(sessionFile ? { sessionFile } : {}),
      cwd: ctx.cwd,
      ...(repository ? { repository } : {}),
      environment: environmentKey()
    }, ctx.sessionManager.getBranch().flatMap(entry =>
      entry.type === "message" ? [{ id: entry.id, message: entry.message as MessageLike }] : []
    ));
    if (ctx.hasUI) ctx.ui.setStatus("pma", "PMA ready");
  });

  pi.on("input", event => {
    if (event.source === "extension") return { action: "continue" as const };
    return { action: "continue" as const };
  });

  pi.on("before_agent_start", async (event) => {
    const slice = await controller.beforeAgentStart(event.prompt);
    return {
      ...(slice ? { message: { customType: "pma-context", content: slice, display: false } } : {}),
      systemPrompt: `${event.systemPrompt}\n\n${promptFragment}`
    };
  });

  pi.on("agent_start", async () => {
    // Interaction state is established in before_agent_start before the low-level run begins.
  });

  pi.on("context", event => ({
    messages: controller.injectContext(event.messages as MessageLike[]) as typeof event.messages
  }));

  pi.on("message_end", (event, ctx) => {
    controller.observeMessage(event.message as MessageLike, ctx.sessionManager.getLeafId() ?? undefined);
  });

  pi.on("turn_end", async () => {
    // Final messages and tool results are captured by message_end in Pi source order.
  });

  pi.on("agent_settled", async () => { await controller.agentSettled(); });

  pi.on("session_before_compact", async event => {
    await controller.beforeCompaction(event.preparation.previousSummary);
  });

  pi.on("model_select", event => {
    controller.modelSelected(event.model.provider, event.model.id);
  });

  pi.on("session_shutdown", async (_event, ctx) => {
    await controller.shutdown();
    if (ctx.hasUI) ctx.ui.setStatus("pma", undefined);
  });

  registerTools(pi, client);
  registerCommands(pi, client);
}

function registerTools(pi: ExtensionAPI, client: PmaClient): void {
  pi.registerTool({
    name: "pma_recall",
    label: "Recall Memory",
    description: "Explicitly search persistent memory when automatic context is insufficient.",
    promptSnippet: "Search PMA persistent memory",
    parameters: Type.Object({ query: Type.String(), limit: Type.Optional(Type.Number()) }),
    async execute(_id, params) {
      const result = await client.request("pma.recall", { query: params.query, selected_limit: params.limit ?? 10 });
      return textResult(result);
    }
  });
  pi.registerTool({
    name: "pma_learn",
    label: "Learn Memory",
    description: "Explicitly propose a durable statement backed by the current interaction.",
    parameters: Type.Object({ statement: Type.String(), authority: Type.Optional(Type.String()) }),
    async execute(_id, params) {
      const result = await client.request("pma.learning.propose", {
        statement: params.statement, authority: params.authority ?? "explicit_user"
      });
      return textResult(result);
    }
  });
  pi.registerTool({
    name: "pma_correct",
    label: "Correct Memory",
    description: "Record an explicit correction while preserving prior memory history.",
    parameters: Type.Object({ target: Type.String(), correction: Type.String() }),
    async execute(_id, params) {
      const result = await client.request("pma.memory.propose_correction", params);
      return textResult(result);
    }
  });
  pi.registerTool({
    name: "pma_inspect",
    label: "Inspect Memory",
    description: "Inspect one persistent memory and its provenance.",
    parameters: Type.Object({ id: Type.String() }),
    async execute(_id, params) {
      const result = await client.request("pma.memory.get", params);
      return textResult(result);
    }
  });
}

function registerCommands(pi: ExtensionAPI, client: PmaClient): void {
  pi.registerCommand("pma-status", {
    description: "Show PMA-Core status",
    handler: async (_args, ctx) => {
      try {
        const status = await client.request("pma.get_status");
        ctx.ui.notify(JSON.stringify(status), "info");
      } catch (error) {
        ctx.ui.notify(error instanceof Error ? error.message : "PMA unavailable", "error");
      }
    }
  });
  pi.registerCommand("pma-recall", {
    description: "Explicitly query PMA memory",
    handler: async (args, ctx) => notifyRequest(client, ctx, "pma.recall", { query: args.trim() })
  });
  pi.registerCommand("pma-learn", {
    description: "Store an explicit durable statement",
    handler: async (args, ctx) => notifyRequest(client, ctx, "pma.learning.propose",
      { statement: args.trim(), authority: "explicit_user" })
  });
  pi.registerCommand("pma-correct", {
    description: "Correct memory: /pma-correct <claim-id> | <correction>",
    handler: async (args, ctx) => {
      const [target, ...parts] = args.split("|");
      await notifyRequest(client, ctx, "pma.memory.propose_correction",
        { target: target?.trim() ?? "", correction: parts.join("|").trim() });
    }
  });
  pi.registerCommand("pma-inspect", {
    description: "Inspect a claim by ID",
    handler: async (args, ctx) => notifyRequest(client, ctx, "pma.memory.get", { id: args.trim() })
  });
}

async function notifyRequest(client: PmaClient, ctx: { ui: { notify(message: string, type: "info" | "error"): void } },
                             method: string, params: Record<string, unknown>): Promise<void> {
  if (Object.values(params).some(value => value === "")) return;
  try {
    const result = await client.request(method, params);
    ctx.ui.notify(JSON.stringify(result), "info");
  } catch (error) {
    ctx.ui.notify(error instanceof Error ? error.message : "PMA request failed", "error");
  }
}

function textResult(value: unknown): { content: Array<{ type: "text"; text: string }>; details: unknown } {
  const text = typeof value === "string" ? value : JSON.stringify(value, null, 2);
  return { content: [{ type: "text", text }], details: value };
}

export function defaultCorePath(cwd: string): string {
  return process.platform === "win32"
    ? resolve(cwd, "build", "windows-msvc-release", "Release", "pma-core.exe")
    : resolve(cwd, "build", "linux-gcc-release", "pma-core");
}

export * from "./client.js";
export * from "./controller.js";
