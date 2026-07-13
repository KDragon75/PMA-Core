# Dependencies and build

The build must remain understandable and repeatable on Windows and Linux. Dependencies are deliberately limited.

## Core runtime dependencies

### C++20 standard library

Use conservative C++20 features supported by current MSVC, GCC, and Clang:

- `std::filesystem`
- RAII
- `std::variant` and `std::optional`
- `std::span`
- `std::chrono`
- `std::jthread` and `std::stop_token` where they simplify cancellation

Avoid template-heavy frameworks and compiler-specific extensions.

### SQLite amalgamation

Vendor the official SQLite amalgamation to control features and version. Required compile features:

- FTS5
- Thread-safe serialized build
- Foreign keys enabled by application startup
- STRICT table support from the selected SQLite version

### nlohmann/json

Use only at protocol and provider boundaries. Parse into PMA-owned typed structures before entering application or domain layers.

### SHA-256

Vendor one small permissively licensed implementation for content and artifact fingerprints. It is not used for password storage or authentication.

## No direct HTTP dependency in the MVP

PMA-Core communicates only with external provider processes. The first-party Node provider uses built-in `fetch` for OpenAI-compatible endpoints. This removes libcurl and TLS packaging from the core executable.

A future direct HTTP client requires a decision record and measured need.

## Platform process handling

Implement one small abstraction:

- Windows: `CreateProcessW`, inheritable pipes, job object where useful.
- Linux: `posix_spawn` or `fork/exec` with pipes.

Keep platform code under `src/platform/`. Domain and application code must not contain `#ifdef _WIN32` branches.

## Build system

Use CMake and checked-in `CMakePresets.json`.

Initial presets:

```text
windows-msvc-debug
windows-msvc-release
linux-gcc-debug
linux-gcc-release
linux-clang-sanitized
```

Use vcpkg manifest mode for `nlohmann-json`, Catch2, and Google Benchmark. SQLite and SHA-256 remain vendored.

## Testing dependencies

- Catch2 for C++ tests.
- CTest as the single C++ test launcher.
- Google Benchmark for dedicated performance executables.
- TypeScript/Node tests for providers, Pi adapter, MCP conformance, and LLM simulation.

## Node requirements

Use a pinned supported Node LTS version compatible with Transformers.js v4. Commit package lockfiles. Use ESM unless a concrete Pi packaging constraint requires otherwise.

## Static analysis and formatting

- `clang-format` with a checked-in configuration.
- Compiler warnings treated as errors in CI for PMA-owned code.
- `clang-tidy` on Linux/Clang CI after the codebase is stable enough to avoid noisy churn.
- AddressSanitizer and UndefinedBehaviorSanitizer on Linux.
- Windows debug runtime checks where practical.

## Explicit non-dependencies

Do not add initially:

- Boost
- Qt
- gRPC or protobuf
- ORM or migration framework
- Graph database
- MCP SDK
- OpenAI SDK
- llama.cpp library
- ONNX Runtime in the core
- Python
- Docker as a build requirement
- Logging framework
- Command-line parsing framework

The rationale is simplicity, not ideology. Add a dependency only when it removes more complexity than it introduces.
