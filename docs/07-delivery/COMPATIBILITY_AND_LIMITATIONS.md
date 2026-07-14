# 0.1.0 compatibility and known limitations

## Supported matrix

| Component | Supported in 0.1.0 |
|---|---|
| OS/architecture | Windows x64; Linux x64 |
| C++ build | MSVC 2022; GCC and Clang with C++20 |
| Node | 24.x |
| PMA service protocol | v1 only |
| Provider protocol | v1 only |
| Core schema | bootstrap metadata plus migrations 100, 101, 200, 300, and 400 |
| Vector schema | v1, rebuildable rather than migrated in place |
| Pi adapter | 0.1.0 with `@earendil-works/pi-coding-agent` 0.80.6 |
| MCP | 2025-11-25 stdio subset documented by the adapter |

The first release supports fresh databases and the single pre-release schema lineage represented by each checked-in migration boundary. There is no promise that an older binary can write a newer database. Make and verify a backup before upgrading.

## Measured release thresholds

On deterministic release stress runs:

- unexplained data loss: exactly 0;
- deterministic hidden-world failures: exactly 0;
- harmful stale-memory rate: at most 1%;
- irrelevant-injection rate: at most 2%;
- precision@k and recall@k: at least 95% for scripted release worlds;
- evidence capture and local recall must remain within the broad targets in the performance report, with provider wait reported separately.

Google Benchmark output is compared by platform/compiler/build type. A single noisy runner does not fail release; a sustained 20% p95 regression on controlled weekly runners requires review.

## Known limitations

- Automatic settled learning is queued by the Pi adapter; a continuously owned background provider-job scheduler is not included.
- The hidden-world harness uses scripted role outputs by default. Real multi-model diversity is an opt-in evaluation, not deterministic authority.
- Flat vector search is intended for single-user scale; large-scale approximate indexing is not included.
- Models and model caches are not distributed. Runtime manifests identify required artifacts.
- Bundle encryption and a dedicated review UI are deferred.
- Evidence deletion semantics remain unresolved; destructive memory tools are disabled by default.
- Maintenance `backup` copies authoritative SQLite only. Vector projections and environment mappings must be reconstructed or copied separately.
- Linux packages are produced and exercised by CI; Windows is the locally verified development host for 0.1.0 preparation.
- A public source-code license requires an explicit owner decision; dependency licenses and notices are complete, but no rights beyond applicable law are implied until that decision is recorded.
