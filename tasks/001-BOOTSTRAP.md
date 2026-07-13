# Task 001 — Repository bootstrap

## Objective

Create the smallest cross-platform repository that builds and tests on Windows and Linux.

## Governing documents

- [Dependencies and build](../docs/05-implementation/DEPENDENCIES_AND_BUILD.md)
- [Repository layout](../docs/05-implementation/REPOSITORY_LAYOUT.md)
- [AGENTS.md](../AGENTS.md)

## Deliverables

- Root CMake project and checked-in presets.
- vcpkg manifest with pinned baseline.
- Vendored SQLite and SHA-256 directories with provenance notes.
- Minimal `pma-core` executable.
- Catch2/CTest smoke test.
- Node workspace for `providers/node`, `adapters/pi`, and `tests/llm-simulation`.
- TypeScript configuration and smoke tests.
- Formatting, warning, sanitizer, and CI skeleton.
- Third-party notice template.

## Constraints

- No memory schema or provider implementation yet.
- No Boost, ORM, web framework, or inbound HTTP server.
- Keep root `AGENTS.md` below agent instruction limits.

## Acceptance criteria

- MSVC release/debug presets configure and build.
- Linux GCC and Clang presets configure and build.
- `ctest` passes.
- Node tests pass without downloading models.
- CI executes the same documented commands.
- Clean clone contains no machine-specific paths.
