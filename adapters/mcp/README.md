# PMA MCP adapter

This stdio MCP server exposes explicit, model-controlled PMA operations. It cannot guarantee automatic observation, recall, context injection, settled learning, or compaction handling; use the Pi extension for full lifecycle integration.

## Run

```text
npm --prefix adapters/mcp install
npm --prefix adapters/mcp run build
PMA_CORE_PATH=/path/to/pma-core node adapters/mcp/dist/src/cli.js
```

Optional environment:

- `PMA_BUNDLE_ROOT`: bundle directory; defaults to `<cwd>/.pma`.
- `PMA_DATABASE_PATH`: authoritative core database path.
- `PMA_MCP_ALLOW_ADMIN=1`: exposes administrative tool descriptions. Administrative operations still require service-side support and confirmation; they are disabled by default.

## Safe behavior

`pma_remember` and `pma_learn` submit source evidence and validated PMA proposals. They never bypass core validation. Current user corrections outrank recalled content. Read output is capped at 50KB.

The versioned prompt guidance is available through `prompts/get` with `pma-memory-guidance`.
