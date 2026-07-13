# Task 010 — MCP adapter

## Objective

Expose safe explicit memory operations for MCP-only clients.

## Governing documents

- [MCP adapter](../docs/04-interfaces/MCP_ADAPTER.md)
- [Security and privacy](../docs/05-implementation/SECURITY_AND_PRIVACY.md)
- [External references](../docs/references/EXTERNAL_REFERENCES.md)

## Deliverables

- MCP initialization and capability negotiation.
- Initial read and controlled-write tools.
- Inspectable resources.
- Versioned tool descriptions and system prompt fragment.
- Permission configuration for administrative tools.
- Translation into existing PMA application services.

## Tests

- MCP schema and protocol fixtures.
- Tool input/output validation.
- Permission denial.
- Multiple model behavior tests for when tools are called.
- Remember/learn never bypasses validation.
- Compaction preparation and restoration scenarios.

## Acceptance criteria

- MCP adapter contains no duplicate memory logic.
- Dangerous tools are disabled by default.
- Tool descriptions clearly state durable versus transient knowledge behavior.
- Documentation states that MCP cannot guarantee automatic lifecycle integration.
