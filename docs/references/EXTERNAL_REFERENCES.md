# External references

These links are implementation references, not substitutes for PMA-Core's internal contracts. Pin dependency versions during implementation and recheck current documentation before release.

## Pi

- [Pi documentation](https://pi.dev/docs/latest)
- [Extensions and lifecycle events](https://pi.dev/docs/latest/extensions)
- [Session file format](https://pi.dev/docs/latest/session-format)
- [Compaction and branch summarization](https://pi.dev/docs/latest/compaction)
- [Pi security and project trust](https://pi.dev/docs/latest/security)

Pi supports TypeScript extensions, event interception, context injection, custom tools, and persistent session extension state. Session files are JSONL trees and remain external evidence for PMA-Core.

## MCP

- [MCP specification, 2025-11-25](https://modelcontextprotocol.io/specification/2025-11-25)
- [MCP tools](https://modelcontextprotocol.io/specification/2025-11-25/server/tools)
- [MCP resources](https://modelcontextprotocol.io/specification/2025-11-25/server/resources)
- [MCP prompts](https://modelcontextprotocol.io/specification/2025-11-25/server/prompts)

MCP messages use JSON-RPC. Tools are model-controlled, which is why the MCP adapter cannot replace full host lifecycle integration.

## Transformers.js

- [Transformers.js documentation](https://huggingface.co/docs/transformers.js/index)
- [Server-side inference in Node.js](https://huggingface.co/docs/transformers.js/tutorials/node)
- [Pipeline API](https://huggingface.co/docs/transformers.js/api/pipelines)
- [Custom model and environment configuration](https://huggingface.co/docs/transformers.js/custom_usage)
- [Transformers.js v4 announcement](https://huggingface.co/blog/transformersjs-v4)

Transformers.js v4 supports server-side runtimes and feature-extraction/text-generation pipelines. PMA-Core still validates structured output independently.

## SQLite

- [SQLite documentation](https://sqlite.org/docs.html)
- [FTS5](https://sqlite.org/fts5.html)
- [STRICT tables](https://sqlite.org/stricttables.html)
- [Write-ahead logging](https://sqlite.org/wal.html)
- [Online backup API](https://sqlite.org/backup.html)

## Codex and agent repository guidance

- [Codex AGENTS.md guidance](https://developers.openai.com/codex/guides/agents-md)
- [Codex best practices](https://developers.openai.com/codex/learn/best-practices)

The root [AGENTS.md](../../AGENTS.md) is intentionally concise. Longer work uses [PLANS.md](../../PLANS.md) and focused task files.
