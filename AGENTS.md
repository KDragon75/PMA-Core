# PMA-Core repository instructions

## Start here

- Read [PROJECT.md](PROJECT.md) and [docs/README.md](docs/README.md).
- For implementation work, select the next unblocked item from [tasks/README.md](tasks/README.md).
- For work spanning more than one focused change, create or update an execution plan using [PLANS.md](PLANS.md).
- Read the design document linked from the selected task before editing code.

## Engineering rules

- Favor readability, explicit control flow, and ordinary data structures over clever abstraction.
- Target C++20 on Windows/MSVC and Linux/GCC or Clang.
- Keep the PMA core independent of model runtimes, Node internals, Pi types, MCP types, and provider-specific HTTP formats.
- SQLite is authoritative for structured state. JSON is for protocol boundaries and provider-specific opaque options.
- Never treat vector indexes as authoritative knowledge.
- Never rewrite source evidence during consolidation or reconsolidation.
- A model may propose memory operations; deterministic PMA code validates and commits them.
- Do not add a production dependency without documenting why the standard library or current dependencies are insufficient.
- Do not add an ORM, graph database, Boost, gRPC, protobuf, or an in-process model runtime without an accepted decision record.
- Do not expose a public native C++ API. The supported boundary is the PMA service protocol.
- Keep injected ContextSlice output minimal; diagnostics belong in the internal ContextPacket.

## Work practices

- Make one architectural change at a time.
- Preserve backward compatibility for persisted data unless the task explicitly includes a migration.
- Add tests with every behavior change.
- Run the narrowest relevant tests first, then the full affected suite.
- Update documentation and decision records in the same change when behavior or contracts change.
- Do not silently resolve ambiguities in memory semantics. Record the question in [OPEN_QUESTIONS.md](docs/01-foundation/OPEN_QUESTIONS.md) when a decision is not established.

## Expected repository commands

These commands become authoritative once the bootstrap task creates them:

```text
cmake --preset <preset>
cmake --build --preset <preset>
ctest --preset <preset>
npm --prefix providers test
npm --prefix adapters/pi test
npm --prefix adapters/mcp test
npm --prefix tests/llm-simulation test
```

Do not invent alternate build commands when the documented preset exists.

## Definition of done

A task is complete only when:

- Its acceptance criteria pass.
- Required tests are implemented and passing.
- New persisted data has migration and integrity coverage.
- Protocol changes include schemas and compatibility notes.
- Relevant documentation links remain valid.
- The task file records any deliberate deviation from the original plan.

See [DEFINITION_OF_DONE.md](docs/07-delivery/DEFINITION_OF_DONE.md).
