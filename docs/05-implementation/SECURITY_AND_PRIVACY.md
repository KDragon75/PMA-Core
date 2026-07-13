# Security and privacy

PMA-Core stores long-lived conversational and tool-derived evidence. Security and deletion behavior must be explicit even for a local-first tool.

## Trust boundaries

- Host adapter input is untrusted evidence.
- Provider output is untrusted proposals.
- MCP tool arguments are model-controlled input.
- Imported bundles are untrusted until verified.
- Retrieved memory text may contain old prompt injection attempts.

## Prompt-injection handling

- Evidence is never automatically promoted to system instructions.
- ContextSlice rendering distinguishes knowledge from instructions and preserves authority.
- Imported or inferred content cannot override current explicit user direction.
- Tool output and document text remain source evidence until validated.
- PMA-generated system-prompt fragments are static versioned assets, not retrieved content.

## Secrets

- Never store API keys in `pma.sqlite` or exported bundles.
- Store symbolic secret references such as `env:PMA_OPENAI_API_KEY`.
- Redact authorization headers from logs and captured provider requests.
- Environment-specific secret mappings are local configuration.

## Filesystem

- Bundle-relative paths are validated against traversal.
- Provider executables and model paths are explicit configuration.
- The core does not execute commands proposed by a model.
- Pi and MCP adapters do not expose administrative vector or deletion tools by default.

## Data deletion

Required operations distinguish:

- Suppress from retrieval.
- Mark derived memory inactive.
- Delete a derived memory while retaining evidence.
- Redact source content.
- Delete source evidence and invalidate dependent knowledge.
- Delete an entire bundle.

Deletion must produce an audit record unless the audit itself is covered by the deletion request. Exact dependency behavior remains an open design item in [Open questions](../01-foundation/OPEN_QUESTIONS.md).

## Backup privacy

Backups may omit environment mappings and vector files. Manifests state omissions. Future bundle encryption is deferred until key management is designed.

## MCP safety

MCP tools capable of deletion, import, provider change, or vector activation require explicit enablement and host confirmation. Read tools still need careful output limits to avoid exposing unrelated memory.

## Supply chain

- Pin C++ and npm dependencies.
- Record checksums for vendored SQLite and SHA-256 sources.
- Verify model artifacts used for reconstructable embedding profiles.
- Review third-party Pi packages before enabling them in the project.
