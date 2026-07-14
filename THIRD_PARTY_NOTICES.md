# Third-party notices

Dependency identity is pinned by `vcpkg.json` (baseline `f87344cac03158cbf1467264565f1fd36b382a24`) and npm lockfiles. Model artifacts are not redistributed.

## Distributed native material

### SQLite 3.50.4

Public domain, <https://www.sqlite.org/copyright.html>. Provenance is in `third_party/sqlite/README.md`.

- `sqlite3.c` SHA-256 `e3f5d6901e7492af4a1fc8c4d745cae84c264942524c3fbfc02b82a5ca8818c8`
- `sqlite3.h` SHA-256 `abd1514e0351f79393d1be882830afdb40a8099e8257f311f0bfdf8486f11bea`

### amosnier/sha-2

Revision `565f65009bdd98267361b17d50cddd7c9beb3e6c`, MIT License. Full text: `third_party/sha256/LICENSE.md`.

- `sha-256.c` SHA-256 `7a74437adc78576b8faff060f9573ba88a6da798914f45d263c75b149add3d27`
- `sha-256.h` SHA-256 `c2173d83813a0c29fcc3345ce489766efeadee6a52ca927ecd0f917a120df9fb`

## Native build dependencies

Resolved from the pinned vcpkg baseline:

- Catch2 3.15.2 — Boost Software License 1.0 (test artifact only).
- Google Benchmark 1.9.5 — Apache License 2.0 (benchmark artifact only).
- nlohmann/json 3.12.0 — MIT License (compiled into PMA-Core).

The release executable does not package Catch2 or Google Benchmark. vcpkg stores the corresponding copyright files in the resolved installation and release CI archives them with build provenance.

## Node packages

The Node provider, Pi extension, and MCP adapter are distributed as separate npm tarballs with their dependency metadata and lockfiles. Their dependency license texts remain in the installed npm packages. Principal direct dependencies are:

- `@huggingface/transformers` 4.2.0 — Apache-2.0.
- Ajv 8.20.0 — MIT.
- `@earendil-works/pi-coding-agent` 0.80.6 — package license recorded by its npm metadata.
- TypeBox 1.3.6 — MIT.
- TypeScript 5.8.3 and `@types/node` 24.0.0 — development only.

Lockfile SHA-256 values:

- `providers/package-lock.json`: `3921605df01c1f8e619dd39fa03184fc18e9e5c6d878339cbabc06fdffd7f6c7`
- `adapters/pi/package-lock.json`: `659a886f8becd7609e2e1dacd6ebd16688c6491e2f103be38a7c9fb25e8ca049`
- `adapters/mcp/package-lock.json`: `f3b6e835024ab1b1b61a223d38257b99f5defe9d314f22a3d02a713f63b715f2`

Transitive package names, versions, integrity hashes, and resolved sources are authoritative in those lockfiles. Run `npm query .license --all` on each release installation to review the complete resolved license inventory.
