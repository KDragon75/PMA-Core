# Portability and resilience tests

## Cross-platform bundle tests

Move a bundle between:

- Windows x64 and Linux x64.
- Different user profiles and root directories.
- Different repository locations.
- Environments with and without original vector files.

Verify:

- `pma.sqlite` opens and verifies.
- Relative bundle paths resolve.
- Historical absolute paths are not assumed current.
- Repository-relative resources remap.
- Unresolved resources remain inspectable.
- Knowledge remains searchable with FTS5 and graph search.
- A new provider can rebuild vector projections.

## Provider resilience

Test startup outage, mid-request crash, timeout, invalid output, partial batch, rate limiting, authentication failure, changed runtime profile, and provider restart during rebuild.

Expected behavior:

| Failure | Result |
|---|---|
| Embedding unavailable | Existing compatible vectors plus FTS/graph; new work queued. |
| Generation unavailable | Evidence captured; extraction jobs wait. |
| Invalid JSON | Job fails or retries; no knowledge mutation. |
| Runtime profile changes | Old vectors remain isolated; new projection required. |
| Provider crashes during build | Job resumes from checkpoint or restarts generation safely. |

## Core crash tests

Kill the process:

- Before evidence transaction.
- After evidence commit but before job creation.
- During operation transaction.
- During vector batch.
- After new vector store validation but before activation.

Verify no partial semantic state and correct recovery action.

## Vector corruption tests

Inject missing vectors, wrong dimensions, NaN/Infinity, content-hash mismatch, stale watermark, wrong store ID, orphan rows, and truncated files. PMA-Core must detect and retire or rebuild rather than silently use bad data.

## Migration tests

For every supported release:

- Create database with old binaries or fixtures.
- Populate representative evidence and graph state.
- Upgrade.
- Verify semantic equivalence, history, FTS rebuild, and vector registry behavior.
- Confirm old binary refuses unsafe write access to newer schema.

## Resource resolution tests

Use same-named files in separate repositories, moved paths, modified contents, deleted files, and matching hashes. Resolution should prefer explicit durable identity and report ambiguity rather than opening the wrong resource.
