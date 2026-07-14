# Security and prompt-injection review — 0.1.0

## Reviewed boundaries

- Host, MCP, provider, imported bundle, filesystem, secret, and recalled-text boundaries were reviewed against the security specification.
- Provider output remains proposal data and deterministic validation owns commits.
- ContextSlice is data, not an executable instruction channel; current explicit user direction takes priority.
- MCP administrative deletion, provider change, import, and vector activation remain denied by default.
- Core code does not execute model-proposed commands.
- Provider credentials use environment references; artifacts and reports must not contain authorization headers.

## Release checks

- Malformed and adversarial proposal tests cannot partially mutate semantic state.
- Tool arguments and protocol envelopes are validated and bounded.
- Bundle-relative vector paths are constrained by the vector manager and missing files degrade without replacing authoritative state.
- Backup/restore verifies SQLite before maintenance and uses staged replacement.
- npm lockfiles, vcpkg baseline, vendored revisions, and model artifact hashes provide supply-chain identity.

## Residual risks

Retrieved evidence can contain hostile prose and an LLM can still follow it despite labeling; adapters minimize and qualify injected context but cannot guarantee model behavior. Local database and backup encryption is not implemented. Anyone with filesystem access can read memory. Exact evidence deletion/redaction dependency semantics are unresolved, so destructive tools remain unavailable by default. Operators must protect environment secrets, bundles, backups, and provider caches with OS permissions.
