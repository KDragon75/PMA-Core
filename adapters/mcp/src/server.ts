import { administrativeTools, promptFragment, protocolVersion, safeTools } from "./catalog.js";
import type { PmaService } from "./pma-client.js";

type Id = string | number | null;
type Params = Record<string, unknown>;
interface Request { jsonrpc: "2.0"; id?: string | number; method: string; params?: Params }

export class McpServer {
  private initialized = false;
  constructor(private readonly pma: PmaService, private readonly allowAdministrative = false) {}

  async handleLine(line: string): Promise<string | undefined> {
    let request: Request;
    try { request = JSON.parse(line) as Request; }
    catch { return error(null, -32700, "Parse error"); }
    const notification = request?.id === undefined;
    const id = request?.id ?? null;
    if (!request || request.jsonrpc !== "2.0" || typeof request.method !== "string")
      return notification ? undefined : error(id, -32600, "Invalid Request");
    try {
      const result = await this.dispatch(request.method, request.params ?? {});
      return notification ? undefined : JSON.stringify({ jsonrpc: "2.0", id, result });
    } catch (caught) {
      if (notification) return undefined;
      const issue = caught instanceof McpError ? caught : new McpError(-32603, "Internal error");
      return error(id, issue.code, issue.message, issue.data);
    }
  }

  private async dispatch(method: string, params: Params): Promise<unknown> {
    if (method === "initialize") {
      const requested = params.protocolVersion;
      if (typeof requested !== "string") throw new McpError(-32602, "protocolVersion is required");
      this.initialized = true;
      return {
        protocolVersion,
        capabilities: { tools: { listChanged: false }, resources: { subscribe: false, listChanged: false }, prompts: { listChanged: false } },
        serverInfo: { name: "@pma/mcp-adapter", version: "0.1.0" },
        instructions: promptFragment
      };
    }
    if (method === "notifications/initialized" || method === "notifications/cancelled") return {};
    if (!this.initialized) throw new McpError(-32002, "MCP server is not initialized");
    if (method === "ping") return {};
    if (method === "tools/list") return { tools: this.allowAdministrative ? [...safeTools, ...administrativeTools] : safeTools };
    if (method === "tools/call") return this.callTool(params);
    if (method === "resources/list") return { resources: [
      { uri: "pma://status", name: "PMA status", mimeType: "application/json", description: "Current PMA service status" }
    ] };
    if (method === "resources/templates/list") return { resourceTemplates: [
      { uriTemplate: "pma://memory/{id}", name: "PMA memory", mimeType: "application/json", description: "Inspectable durable memory by ID" }
    ] };
    if (method === "resources/read") return this.readResource(params);
    if (method === "prompts/list") return { prompts: [
      { name: "pma-memory-guidance", description: "Safe guidance for explicit persistent-memory use", arguments: [] }
    ] };
    if (method === "prompts/get") {
      if (params.name !== "pma-memory-guidance") throw new McpError(-32602, "Unknown prompt");
      return { description: "PMA persistent-memory guidance", messages: [
        { role: "user", content: { type: "text", text: promptFragment } }
      ] };
    }
    throw new McpError(-32601, "Method not found");
  }

  private async callTool(params: Params): Promise<unknown> {
    if (typeof params.name !== "string") throw new McpError(-32602, "Tool name is required");
    const args = isObject(params.arguments) ? params.arguments : {};
    const safe = safeTools.find(tool => tool.name === params.name);
    const administrative = administrativeTools.find(tool => tool.name === params.name);
    if (administrative && !this.allowAdministrative)
      throw new McpError(-32003, "Administrative PMA tool is disabled", { code: "PMA_PERMISSION_DENIED" });
    if (!safe && !administrative) throw new McpError(-32602, "Unknown tool");
    const schema = (safe ?? administrative)!.inputSchema;
    validate(schema as Schema, args);
    let result: unknown;
    switch (params.name) {
      case "pma_get_context":
        result = await this.pma.request("pma.recall", { query: args.query, token_budget: args.token_budget ?? 600 });
        break;
      case "pma_recall":
      case "pma_search_memories":
        result = await this.pma.request("pma.memory.search", { query: args.query, selected_limit: args.limit ?? 10 });
        break;
      case "pma_get_memory":
        result = await this.pma.request("pma.memory.get", { id: args.id });
        break;
      case "pma_remember":
      case "pma_learn":
        result = await this.pma.request("pma.learning.propose", {
          statement: args.statement, authority: args.authority ?? "explicit_mcp"
        });
        break;
      case "pma_propose_correction":
        result = await this.pma.request("pma.memory.propose_correction", { target: args.target, correction: args.correction });
        break;
      case "pma_prepare_compaction":
        result = await this.pma.request("pma.learning.consolidate", { reason: "mcp_compaction", summary: args.summary });
        break;
      case "pma_restore_after_compaction":
        result = await this.pma.request("pma.recall", { query: args.query ?? "active goals decisions unresolved work", token_budget: 1000 });
        break;
      default:
        throw new McpError(-32601, "Administrative operation is not implemented by this adapter");
    }
    const text = bounded(JSON.stringify(result, null, 2));
    return { content: [{ type: "text", text }], structuredContent: result, isError: false };
  }

  private async readResource(params: Params): Promise<unknown> {
    if (typeof params.uri !== "string") throw new McpError(-32602, "Resource URI is required");
    let value: unknown;
    if (params.uri === "pma://status") value = await this.pma.request("pma.get_status");
    else {
      const match = /^pma:\/\/memory\/([^/]+)$/.exec(params.uri);
      if (!match) throw new McpError(-32002, "Resource not found");
      value = await this.pma.request("pma.memory.get", { id: decodeURIComponent(match[1] ?? "") });
    }
    return { contents: [{ uri: params.uri, mimeType: "application/json", text: bounded(JSON.stringify(value, null, 2)) }] };
  }
}

class McpError extends Error {
  constructor(readonly code: number, message: string, readonly data?: unknown) { super(message); }
}

interface Schema { properties: Record<string, { type?: string }>; required?: readonly string[]; additionalProperties?: boolean }
function validate(schema: Schema, args: Params): void {
  for (const field of schema.required ?? []) {
    if (!(field in args) || args[field] === "") throw new McpError(-32602, `Missing required field: ${field}`);
  }
  if (schema.additionalProperties === false) {
    for (const field of Object.keys(args)) if (!(field in schema.properties)) throw new McpError(-32602, `Unknown field: ${field}`);
  }
  for (const [field, value] of Object.entries(args)) {
    const expected = schema.properties[field]?.type;
    if (expected === "string" && typeof value !== "string") throw new McpError(-32602, `Invalid field type: ${field}`);
    if (expected === "integer" && !Number.isInteger(value)) throw new McpError(-32602, `Invalid field type: ${field}`);
  }
}
function isObject(value: unknown): value is Params { return Boolean(value) && typeof value === "object" && !Array.isArray(value); }
function bounded(text: string): string { return text.length <= 50_000 ? text : `${text.slice(0, 50_000)}\n[truncated]`; }
function error(id: Id, code: number, message: string, data?: unknown): string {
  return JSON.stringify({ jsonrpc: "2.0", id, error: { code, message, ...(data === undefined ? {} : { data }) } });
}
