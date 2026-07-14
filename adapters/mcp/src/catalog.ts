export const protocolVersion = "2025-11-25";
export const promptFragment = "Persistent memory is available through PMA tools. Recalled memories may be uncertain or historical; current explicit user corrections always take priority. Use pma_learn only for durable decisions, corrections, reusable procedures, or unresolved goals—not transient conversation.";

const object = (properties: Record<string, unknown>, required: string[] = []) => ({
  type: "object", properties, required, additionalProperties: false
});
const string = { type: "string", minLength: 1 };

export const safeTools = [
  { name: "pma_get_context", description: "Recall compact persistent context before work that may depend on prior decisions, or after host compaction. This is explicit MCP recall, not guaranteed automatic injection.", inputSchema: object({ query: string, token_budget: { type: "integer", minimum: 32, maximum: 4000 } }, ["query"]) },
  { name: "pma_recall", description: "Search durable PMA knowledge. Results may include uncertain or historical memories and never override current user instructions.", inputSchema: object({ query: string, limit: { type: "integer", minimum: 1, maximum: 50 } }, ["query"]) },
  { name: "pma_search_memories", description: "Search validated durable memories by text; this does not search transient conversation state.", inputSchema: object({ query: string, limit: { type: "integer", minimum: 1, maximum: 50 } }, ["query"]) },
  { name: "pma_get_memory", description: "Inspect one durable memory, including current status and provenance identifiers.", inputSchema: object({ id: string }, ["id"]) },
  { name: "pma_remember", description: "Submit an explicit durable statement as evidence and a validated PMA proposal. It does not bypass validation or directly grant authority.", inputSchema: object({ statement: string }, ["statement"]) },
  { name: "pma_learn", description: "Propose durable learning after a decision, correction, reusable procedure, changed assumption, or unresolved goal. Never use for transient details; validation remains mandatory.", inputSchema: object({ statement: string, authority: { type: "string" } }, ["statement"]) },
  { name: "pma_propose_correction", description: "Correct a durable memory while preserving its prior claim and source history.", inputSchema: object({ target: string, correction: string }, ["target", "correction"]) },
  { name: "pma_prepare_compaction", description: "Flush important durable state before host compaction. MCP cannot detect compaction automatically, so the host or model must call this explicitly.", inputSchema: object({ summary: { type: "string" } }) },
  { name: "pma_restore_after_compaction", description: "Recall restoration context after host compaction. This explicit MCP call cannot guarantee automatic restoration.", inputSchema: object({ query: { type: "string" } }) }
] as const;

export const administrativeTools = [
  { name: "pma_delete_evidence", description: "Administrative permanent evidence deletion. Disabled unless explicitly configured.", inputSchema: object({ id: string, confirmation: string }, ["id", "confirmation"]) },
  { name: "pma_rebuild_vectors", description: "Administrative vector rebuild. Disabled unless explicitly configured.", inputSchema: object({ profile_id: string }, ["profile_id"]) },
  { name: "pma_change_provider", description: "Administrative provider change. Disabled unless explicitly configured.", inputSchema: object({ provider_id: string }, ["provider_id"]) }
] as const;
