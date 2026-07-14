import { createHash } from "node:crypto";
import { join } from "node:path";
import type { RpcClient } from "./client.js";

export interface SessionContext {
  sessionId: string;
  sessionFile?: string;
  cwd: string;
  repository?: string;
  environment: string;
}

export interface MessageLike {
  role: string;
  content?: unknown;
  toolCallId?: string;
  toolName?: string;
  timestamp?: number;
  customType?: string;
  [key: string]: unknown;
}

export interface SliceResult { packet_id?: string; slice?: string; text?: string }

export class LifecycleController {
  private session: SessionContext | undefined;
  private interactionId: string | undefined;
  private interactionSequence = 0;
  private messageOrdinal = 0;
  private cachedSlice = "";
  private cachedPacketId: string | undefined;
  private observationQueue: Promise<void> = Promise.resolve();
  private settled = true;
  private responseModel: string | undefined;

  constructor(private readonly client: RpcClient) {}

  async startSession(context: SessionContext, branchEntries: Array<{ id?: string; message?: MessageLike }> = []): Promise<void> {
    this.session = context;
    await this.client.start();
    await this.safeRequest("pma.initialize", {
      protocol_version: { min: 1, max: 1 }, client: { name: "@pma/pi-adapter", version: "0.1.0" },
      requested_capabilities: ["observation", "recall", "learning"],
      environment: { external_key: context.environment, cwd: context.cwd, repository: context.repository },
      session: { external_key: context.sessionId, source_file: context.sessionFile },
      database_path: join(context.cwd, ".pma", "pma.sqlite"),
      bundle_root: join(context.cwd, ".pma")
    });
    await this.safeRequest("pma.session.open", {
      session_id: context.sessionId, source_adapter: "pi", source_file: context.sessionFile,
      cwd: context.cwd, repository: context.repository, environment: context.environment
    });
    await this.reconcile(branchEntries);
  }

  async beforeAgentStart(prompt: string): Promise<string> {
    if (!this.session) return "";
    this.interactionSequence++;
    this.messageOrdinal = 0;
    this.interactionId = `${this.session.sessionId}:interaction:${this.interactionSequence}`;
    this.cachedSlice = "";
    this.cachedPacketId = undefined;
    this.settled = false;
    await this.safeRequest("pma.interaction.begin", {
      interaction_id: this.interactionId, session_id: this.session.sessionId,
      prompt, idempotency_key: `${this.interactionId}:begin`, response_model: this.responseModel
    });
    const recall = await this.safeRequest<SliceResult>("pma.recall", {
      packet_id: `${this.interactionId}:packet`, interaction_id: this.interactionId, query: prompt,
      active_contexts: this.activeContexts(), token_budget: 600
    });
    this.cachedSlice = recall?.slice ?? recall?.text ?? "";
    this.cachedPacketId = recall?.packet_id;
    return this.cachedSlice;
  }

  injectContext(messages: MessageLike[]): MessageLike[] {
    const filtered = messages.filter(message => message.customType !== "pma-context");
    if (!this.cachedSlice) return filtered;
    return [...filtered, { role: "custom", customType: "pma-context", content: this.cachedSlice,
      display: false, timestamp: Date.now() }];
  }

  observeMessage(message: MessageLike, sourceEntryId?: string): void {
    if (!this.session || !this.interactionId || message.customType === "pma-context") return;
    const ordinal = this.messageOrdinal++;
    const sourceId = sourceEntryId ?? `${this.interactionId}:message:${ordinal}`;
    const text = messageText(message);
    this.observationQueue = this.observationQueue.then(async () => {
      await this.safeRequest("pma.observe.event", {
        session_id: this.session!.sessionId, interaction_id: this.interactionId,
        source_id: sourceId, parent_source_id: message.toolCallId,
        ordinal, role: message.role, text, tool_name: message.toolName,
        timestamp: message.timestamp, idempotency_key: `pi:${this.session!.environment}:${sourceId}`
      });
    });
  }

  async agentSettled(): Promise<void> {
    if (!this.interactionId || this.settled) return;
    await this.flush();
    await this.safeRequest("pma.interaction.complete", {
      interaction_id: this.interactionId, idempotency_key: `${this.interactionId}:complete`,
      recall_packet_id: this.cachedPacketId
    });
    await this.safeRequest("pma.learning.process_interaction", {
      interaction_id: this.interactionId, idempotency_key: `${this.interactionId}:learn`
    });
    this.settled = true;
    this.cachedSlice = "";
    this.cachedPacketId = undefined;
  }

  async beforeCompaction(summaryEvidence?: string): Promise<void> {
    await this.flush();
    if (summaryEvidence && this.interactionId) {
      this.observeMessage({ role: "custom", content: summaryEvidence, customType: "pi-compaction-summary" });
      await this.flush();
    }
    await this.safeRequest("pma.learning.consolidate", {
      session_id: this.session?.sessionId, reason: "pi_compaction"
    });
  }

  modelSelected(provider: string, model: string): void {
    this.responseModel = `${provider}/${model}`;
    if (this.session) this.client.notify("pma.observe.event", {
      session_id: this.session.sessionId, role: "model_provenance", text: this.responseModel,
      source_id: `${this.session.sessionId}:model:${Date.now()}`
    });
  }

  async shutdown(): Promise<void> {
    await this.flush();
    await this.safeRequest("pma.session.close", { session_id: this.session?.sessionId });
    await this.client.stop();
    this.session = undefined;
  }

  async flush(): Promise<void> { await this.observationQueue; }
  get currentSlice(): string { return this.cachedSlice; }
  get currentInteractionId(): string | undefined { return this.interactionId; }

  private activeContexts(): Array<{ type: string; external_key: string }> {
    if (!this.session) return [];
    return [
      { type: "repository", external_key: this.session.repository ?? this.session.cwd },
      { type: "environment", external_key: this.session.environment },
      { type: "session", external_key: this.session.sessionId }
    ];
  }

  private async reconcile(entries: Array<{ id?: string; message?: MessageLike }>): Promise<void> {
    for (const entry of entries) {
      if (!entry.message || !entry.id) continue;
      await this.safeRequest("pma.observe.event", {
        session_id: this.session?.sessionId, source_id: entry.id, role: entry.message.role,
        text: messageText(entry.message), idempotency_key: `pi:reconcile:${entry.id}`
      });
    }
  }

  private async safeRequest<T = unknown>(method: string, params: Record<string, unknown>): Promise<T | undefined> {
    try { return await this.client.request<T>(method, compact(params)); }
    catch { return undefined; }
  }
}

function compact(value: Record<string, unknown>): Record<string, unknown> {
  return Object.fromEntries(Object.entries(value).filter(([, item]) => item !== undefined));
}

export function messageText(message: MessageLike): string {
  if (typeof message.content === "string") return message.content;
  if (!Array.isArray(message.content)) return "";
  return message.content.map(block => {
    if (!block || typeof block !== "object") return "";
    const value = block as Record<string, unknown>;
    if (value.type === "text") return String(value.text ?? "");
    if (value.type === "thinking") return String(value.thinking ?? "");
    if (value.type === "toolCall") return `[tool ${String(value.name ?? "unknown")}] ${JSON.stringify(value.arguments ?? {})}`;
    if (value.type === "image") return "[image]";
    return "";
  }).filter(Boolean).join("\n");
}

export function environmentKey(): string {
  return createHash("sha256").update(`${process.platform}:${process.arch}:${process.env.COMPUTERNAME ?? process.env.HOSTNAME ?? "host"}`)
    .digest("hex").slice(0, 24);
}
