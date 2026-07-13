# SQLite core storage

`pma.sqlite` is the authoritative database. It contains evidence, knowledge, graph structure, operations, jobs, retrieval history, configuration, migrations, and the event stream used to build vector projections.

## Storage rules

- Use SQLite `STRICT` tables for PMA-owned structures.
- Enable foreign keys on every connection.
- Use WAL mode for normal operation.
- Use explicit prepared SQL rather than an ORM.
- Store important structured fields in columns and normalized tables.
- Use JSON only for provider-specific opaque options or boundary payload archives.
- Avoid database triggers for domain behavior; application services must make operations explicit and auditable.

## Proposed table groups

### Schema and system

```text
schema_migrations
system_metadata
configuration
```

### Evidence

```text
episodes
episode_events
evidence_segments
source_attachments
```

### Graph identity

```text
graph_nodes
entities
entity_aliases
contexts
resources
resource_locations
```

### Knowledge

```text
predicates
typed_values
claims
claim_evidence
procedures
procedure_steps
goals
graph_edges
```

### Processing and audit

```text
memory_operations
operation_inputs
model_runs
provider_runtime_manifests
consolidation_runs
```

### Retrieval and evaluation

```text
retrieval_events
retrieval_candidates
retrieval_outcomes
associations
context_packets
```

### Embedding projection source

```text
embedding_documents
embedding_events
embedding_profiles
vector_store_registry
```

### Durable jobs

```text
jobs
job_attempts
job_checkpoints
```

## Stable IDs

Use application-generated sortable IDs. The exact implementation is selected during bootstrap, but IDs must:

- Be unique without database round trips.
- Serialize as text across protocols.
- Avoid embedding environment or model identity.
- Remain stable through export and migration.

SQLite integer sequence numbers are reserved for ordered local streams such as `embedding_events.event_seq` and audit sequence fields.

## Transactions

- Serialize core writes through one application writer.
- Keep provider calls outside transactions.
- Commit evidence capture separately from model-assisted processing.
- Apply one validated memory operation atomically with all graph and embedding-event changes.
- Update job checkpoint and associated durable results in the same transaction where possible.

## FTS5

Maintain FTS5 projections for:

- Evidence text.
- Canonical entity names and aliases.
- Claim renderings.
- Procedure names and steps.
- Embedding-document text.

FTS tables are rebuildable from authoritative rows, but their definitions and rebuild commands are migration-controlled.

## Connection policy

Provisional startup settings:

```sql
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA busy_timeout = 5000;
```

Use a small connection manager rather than a general-purpose pool. The MVP expects one writer and a bounded number of readers.

## Integrity

Provide a `pma-core verify` command that checks:

- SQLite integrity and foreign keys.
- Graph invariants.
- Evidence reachability.
- FTS row consistency.
- Operation audit continuity.
- Embedding-event sequence continuity.
- Vector registry metadata against files.

Detailed vector storage is in [Vector projections](VECTOR_PROJECTIONS.md). Backup and schema evolution are in [Migrations, backup, and recovery](MIGRATIONS_BACKUP_RECOVERY.md).
