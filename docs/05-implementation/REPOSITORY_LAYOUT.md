# Repository layout

The intended implementation layout keeps protocol, application, domain, storage, adapters, and tests visibly separate.

```text
PMA-Core/
├── AGENTS.md
├── PLANS.md
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── cmake/
├── docs/
├── tasks/
├── protocol/
│   ├── pma-service/
│   ├── provider/
│   └── schemas/
├── src/
│   ├── main/
│   ├── transport/
│   ├── application/
│   ├── domain/
│   ├── storage/
│   ├── retrieval/
│   ├── vectors/
│   ├── providers/
│   └── platform/
├── include/
│   └── pma-internal/
├── third_party/
│   ├── sqlite/
│   └── sha256/
├── adapters/
│   ├── pi/
│   └── mcp/
├── providers/
│   └── node/
├── tests/
│   ├── unit/
│   ├── component/
│   ├── lifecycle/
│   ├── protocol/
│   ├── integration/
│   ├── migrations/
│   ├── performance/
│   └── llm-simulation/
└── tools/
```

## Directory responsibilities

### `protocol/`

Versioned JSON schemas, method catalogs, examples, and conformance fixtures. These are the public process-level contracts.

### `src/domain/`

Plain types and deterministic rules for claims, evidence, operations, activation, and graph semantics. No SQLite, JSON-RPC, Pi, MCP, or provider types.

### `src/application/`

Use-case orchestration such as observe, recall, apply operation, consolidate, and build vectors. Owns transaction boundaries and policy checks.

### `src/storage/`

Explicit SQLite repositories, migrations, statements, and verification. No provider calls.

### `src/retrieval/`

FTS5, flat vector scan, graph expansion, score fusion, ContextPacket construction, and ContextSlice rendering.

### `src/providers/`

Provider process client and runtime comparison. Does not implement model inference.

### `src/platform/`

Child process, pipes, filesystem-specific behavior, and OS diagnostics.

### `adapters/`

TypeScript integration packages. Each may have a local `AGENTS.md` when implementation starts so Pi/Codex receives directory-specific rules.

### `providers/node/`

First-party external provider supporting Transformers.js and OpenAI-compatible endpoints.

## Header policy

There is no public native API. Headers under `include/pma-internal/` exist only to organize the executable and tests. Do not design for binary ABI stability.

## File size and cohesion

Prefer focused source files and ordinary classes/functions. Split by coherent responsibility, not by one class per file or generic “utils” buckets.
