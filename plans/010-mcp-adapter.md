# Task 010 MCP adapter

## Objective
Expose safe explicit PMA read and controlled-write operations to MCP-only clients without duplicating memory semantics.

## Governing documents
- [Task 010](../tasks/010-MCP-ADAPTER.md)
- [MCP adapter](../docs/04-interfaces/MCP_ADAPTER.md)
- [Security and privacy](../docs/05-implementation/SECURITY_AND_PRIVACY.md)
- [External references](../docs/references/EXTERNAL_REFERENCES.md)
- MCP specification 2025-11-25 initialization, stdio, tools, resources, and prompts contracts.

## Constraints
- MCP arguments are untrusted and all writes translate to validated PMA service methods.
- No deletion, provider change, import, vector activation, or operation-apply tool is enabled by default.
- Stdout contains MCP JSON-RPC only.
- Tool/resource output is bounded and unrelated memory is not returned.
- Documentation explicitly states MCP cannot guarantee automatic lifecycle behavior.

## Current state
- `adapters/mcp` contains only a placeholder.
- The PMA service supports safe recall, explicit learn/correct, inspect, status, and lifecycle methods.
- The Pi adapter has automatic hooks, but MCP clients are model-controlled and explicit only.

## Work sequence
1. Implement MCP stdio initialization, capability negotiation, and bounded JSON-RPC errors.
2. Define versioned safe read/write tools and static prompt fragment.
3. Translate tools to existing PMA service methods through a child client.
4. Add inspectable status and memory resource templates.
5. Add administrative permission configuration and denial-by-default behavior.
6. Add fixtures/conformance tests, run suites, commit, and push before Task 011.

## Tests
- Initialize/initialized handshake, tools/list, resources/list/templates/read, and prompts.
- Input and output validation plus stable errors.
- Explicit learning/correction translations and permission denial.
- Compaction preparation/restoration guidance.
- Stdout purity and child restart behavior.

## Risks and rollback
MCP evolves independently. Protocol framing and catalogs are isolated from PMA translation so version updates do not alter core memory semantics.

## Progress
- [x] Governing documents and state reviewed.
- [x] MCP host/catalog implemented.
- [x] PMA translation and permissions implemented.
- [x] Protocol fixtures and deterministic tests passing; multi-model calling evaluation remains Task 011 stress work.

## Decisions made during execution
- Implement the small 2025-11-25 stdio subset directly instead of adding an MCP SDK dependency.

## Completion evidence
- Implemented MCP 2025-11-25 stdio initialization, ping, tools, resources/templates/read, prompts, bounded output, stable errors, and stdout separation without adding an MCP SDK.
- Nine safe explicit tools translate to existing PMA service methods; learn/remember remain evidence-backed validated proposals, and correction preserves history.
- Administrative deletion, vector rebuild, and provider-change tools are absent by default and return `PMA_PERMISSION_DENIED` when called without explicit enablement.
- Added inspectable status and memory resources, versioned prompt guidance, schemas/fixtures, deployment documentation, and CI coverage.
- MCP tests passed 5/5, including real child-process learn/recall/resource inspection; Pi passed 4/4, provider 4/4, and simulation 1/1.
- Windows MSVC Debug and Release each passed 52/52 CTest cases; `git diff --check` passed.
- Documentation explicitly states MCP cannot guarantee automatic lifecycle integration. Actual multi-model tool-calling behavior grading belongs to the Task 011 hidden-world stress harness; deterministic tool intent/schema behavior is covered here. Linux execution remains delegated to CI.
