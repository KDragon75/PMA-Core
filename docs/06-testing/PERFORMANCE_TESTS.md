# Performance tests

Performance testing separates PMA processing time from provider latency.

## Metrics

Record distributions and resource use:

```text
p50
p95
p99
maximum
throughput
CPU time
wall time
peak memory
database size
vector size
provider wait time
```

## Data tiers

Provisional synthetic tiers:

| Tier | Episodes | Claims | Vectors |
|---|---:|---:|---:|
| Small | 10,000 | 5,000 | 10,000 |
| Medium | 100,000 | 50,000 | 100,000 |
| Large | 1,000,000 | 500,000 | 1,000,000 |

These are stress tiers, not product promises.

## Benchmarks

### Storage

- Episode insert with varying message/tool sizes.
- Evidence segmentation.
- Claim and edge transaction.
- Job dequeue/checkpoint.
- FTS update.
- Startup and migration.
- Backup and verify.

### Retrieval

- FTS only.
- Flat vector only.
- Graph seed and one/two-hop traversal.
- Hybrid score fusion.
- ContextPacket construction.
- ContextSlice rendering under different budgets.

### Vector projection

- Batch embedding ingestion excluding provider time.
- Replay throughput.
- Delete replay.
- Full validation.
- Blue-green activation.
- Flat cosine scan by vector count and dimensions.

### Long-term SQLite behavior

- WAL growth and checkpoint time.
- Page count and fragmentation.
- Query-plan stability.
- Vacuum and backup time.
- FTS rebuild.

## Provisional budgets

Before real measurements, use broad engineering targets:

- Evidence capture should be unnoticeable relative to agent response latency.
- Local FTS/graph recall should normally remain below a few hundred milliseconds at expected single-user scale.
- ContextSlice rendering should be negligible compared with model inference.
- Provider time is reported separately and must not hide core regressions.

Replace these with percentile release gates after milestone 7 produces representative data.

## Regression policy

Store benchmark baselines by compiler, platform, build type, and dataset. Fail CI only on sustained meaningful regressions, not normal machine noise. Weekly controlled runners provide the authoritative trend.
