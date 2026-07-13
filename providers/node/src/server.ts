import { createAdapter } from "./adapters.js";
import { generateStructured, StructuredGenerationError } from "./structured.js";
import type { JsonRpcId, ProviderAdapter, ProviderConfig } from "./types.js";

type JsonObject = Record<string, unknown>;
interface Request { jsonrpc: "2.0"; id?: JsonRpcId; method: string; params?: JsonObject }

export class ProviderServer {
  private adapter?: ProviderAdapter;
  private shutdown = false;
  private readonly active = new Map<string, AbortController>();

  get shutdownRequested(): boolean { return this.shutdown; }

  async handleLine(line: string): Promise<string | undefined> {
    let request: Request;
    try { request = JSON.parse(line) as Request; }
    catch { return this.error(null, -32700, "PROVIDER_PARSE_ERROR", "invalid JSON"); }
    const id = request?.id ?? null;
    const notification = request?.id === undefined;
    if (!request || request.jsonrpc !== "2.0" || typeof request.method !== "string") {
      return notification ? undefined : this.error(id, -32600, "PROVIDER_INVALID_REQUEST", "invalid JSON-RPC envelope");
    }
    const key = String(id);
    const controller = new AbortController();
    if (!notification) this.active.set(key, controller);
    try {
      const result = await this.dispatch(request.method, request.params ?? {}, controller.signal);
      return notification ? undefined : JSON.stringify({ jsonrpc: "2.0", id, result });
    } catch (error) {
      if (notification) return undefined;
      if (error instanceof StructuredGenerationError) {
        return this.error(id, -32003, "PROVIDER_INVALID_OUTPUT", error.message, false,
          { raw: error.raw, diagnostics: error.diagnostics });
      }
      if (error instanceof DOMException && error.name === "AbortError") {
        return this.error(id, -32800, "PROVIDER_CANCELLED", "request cancelled");
      }
      return this.error(id, -32603, "PROVIDER_INTERNAL", "provider operation failed");
    } finally {
      if (!notification) this.active.delete(key);
    }
  }

  private async dispatch(method: string, params: JsonObject, signal: AbortSignal): Promise<unknown> {
    if (method === "provider.initialize") {
      const config = params.config as ProviderConfig | undefined;
      if (!config || typeof config.kind !== "string" || typeof config.model !== "string" ||
          !Number.isInteger(config.dimensions) || config.dimensions <= 0) throw new Error("invalid config");
      this.adapter = createAdapter(config);
      return { protocolVersion: 1, initialized: true, runtime: await this.adapter.describeRuntime() };
    }
    if (method === "provider.health") return { status: "ok", initialized: Boolean(this.adapter) };
    if (method === "provider.get_capabilities") return {
      methods: ["provider.initialize", "provider.describe_runtime", "provider.get_capabilities",
        "provider.health", "provider.cancel", "provider.shutdown", "embedding.embed_documents",
        "embedding.embed_query", "generation.generate_structured"],
      embeddings: true, structuredGeneration: true, cancellation: true
    };
    if (method === "provider.cancel") {
      const target = params.id;
      if (typeof target !== "string" && typeof target !== "number") throw new Error("invalid cancellation id");
      this.active.get(String(target))?.abort();
      return { cancelled: true };
    }
    if (method === "provider.shutdown") { this.shutdown = true; return { accepted: true }; }
    if (!this.adapter) throw new Error("not initialized");
    if (method === "provider.describe_runtime") return this.adapter.describeRuntime();
    if (method === "embedding.embed_documents") {
      const texts = params.texts;
      if (!Array.isArray(texts) || !texts.every(text => typeof text === "string")) throw new Error("invalid texts");
      return { vectors: await this.adapter.embedDocuments(texts, signal) };
    }
    if (method === "embedding.embed_query") {
      if (typeof params.text !== "string") throw new Error("invalid text");
      return { vector: await this.adapter.embedQuery(params.text, signal) };
    }
    if (method === "generation.generate_structured") {
      if (typeof params.task !== "string" || typeof params.schema !== "object" || params.schema === null)
        throw new Error("invalid structured request");
      return generateStructured(params.task, params.schema as object, prompt => this.adapter!.generate(prompt, signal));
    }
    throw new Error("unknown method");
  }

  private error(id: JsonRpcId | null, rpcCode: number, code: string, message: string,
                retryable = false, details: JsonObject = {}): string {
    return JSON.stringify({ jsonrpc: "2.0", id, error: { code: rpcCode, message,
      data: { code, category: code === "PROVIDER_CANCELLED" ? "cancelled" : "provider", retryable, details } } });
  }
}
