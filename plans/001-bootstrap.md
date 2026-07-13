# Task 001 repository bootstrap

## Objective
Create the smallest cross-platform C++20 and Node repository that configures, builds, and runs smoke tests on Windows and Linux.

## Governing documents
- [Task 001](../tasks/001-BOOTSTRAP.md)
- [Dependencies and build](../docs/05-implementation/DEPENDENCIES_AND_BUILD.md)
- [Repository layout](../docs/05-implementation/REPOSITORY_LAYOUT.md)

## Constraints
- Do not introduce memory schemas or provider behavior.
- Keep the core independent of model runtimes and adapters.
- Use vendored SQLite and SHA-256; use vcpkg manifest mode for Catch2, nlohmann-json, and Google Benchmark.
- No machine-specific paths.

## Current state
- The remote repository has no commits.
- The supplied project-plan documents were the only working-tree content.
- CMake 4.3.2 and Node 24.15.0 are installed locally; no C++ compiler is visible in the current shell.

## Work sequence
1. Promote the supplied planning files to the repository root and create the documented directory skeleton.
2. Add the CMake project, presets, vcpkg manifest, minimal executable, and Catch2 smoke test.
3. Vendor SQLite and SHA-256 with provenance and licensing notes.
4. Add Node/TypeScript workspaces and smoke tests.
5. Add formatting, warning, sanitizer, notice, and CI skeletons.
6. Run all locally available acceptance checks and record evidence.

## Tests
- `cmake --list-presets`
- Configure/build/test applicable host presets.
- `npm --prefix providers test`
- `npm --prefix adapters/pi test`
- `npm --prefix tests/llm-simulation test`
- Inspect tracked content for machine-specific absolute paths.

## Risks and rollback
Dependency restoration requires network access and a configured vcpkg installation. Platform presets can only be executed on their matching OS with an installed compiler. All changes are additive and can be rolled back by removing the bootstrap files.

## Progress
- [x] Repository state and governing documents verified.
- [x] Repository directory skeleton created.
- [x] C++ build and smoke test implemented.
- [x] Vendored dependencies documented.
- [x] Node workspaces and smoke tests implemented.
- [x] CI and tooling skeleton implemented.
- [x] Locally available acceptance checks completed; Linux execution is delegated to CI.

## Decisions made during execution
- Use SQLite 3.50.4, the current pinned amalgamation selected for bootstrap.
- Pin Node to the active 24.x LTS line and use ESM.

## Completion evidence
- `cmake --list-presets=all` listed all five configure/build/test workflows (host filtering shows the two Windows configure presets locally).
- `cmake --preset windows-msvc-debug`, `cmake --build --preset windows-msvc-debug`, and `ctest --preset windows-msvc-debug` passed; 1/1 test passed.
- `cmake --preset windows-msvc-release`, `cmake --build --preset windows-msvc-release`, and `ctest --preset windows-msvc-release` passed; 1/1 test passed.
- `npm --prefix providers test` passed; 1/1 test passed without a model download.
- `npm --prefix adapters/pi test` passed; 1/1 test passed.
- `npm --prefix tests/llm-simulation test` passed; 1/1 test passed offline.
- `git diff --check` passed.
- Repository scan found no machine-specific absolute paths outside upstream vendored SQLite examples.
- Linux GCC/Clang presets could not execute on this Windows host. The CI matrix executes all three exact Linux preset workflows on `ubuntu-latest`; final Linux acceptance therefore remains dependent on the first CI run.
