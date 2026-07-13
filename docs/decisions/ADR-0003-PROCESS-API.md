# ADR-0003: Process-level API, no public C++ API

**Status:** Accepted

## Context

A public C++ API would introduce compiler and standard-library ABI concerns and would not serve TypeScript Pi or MCP integrations directly.

## Decision

Expose a versioned JSON-RPC application protocol, initially over stdio. C++ remains an implementation detail. MCP and Pi are adapters over the same application services.

## Consequences

- Language-neutral integration.
- No native ABI compatibility burden.
- Protocol schemas and conformance tests are required.
- Internal C++ types must not leak into transport contracts.

See [PMA service protocol](../04-interfaces/SERVICE_PROTOCOL.md).
