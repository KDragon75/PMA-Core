# Errors, jobs, and events

## Error model

Every application error includes:

- Stable code.
- Category.
- Human-readable message.
- Retryable flag.
- Operation, interaction, or job ID where available.
- Safe structured details.
- Optional internal diagnostic cause excluded from untrusted clients.

Initial categories:

```text
validation
not_found
conflict
incompatible_profile
provider_unavailable
provider_invalid_output
storage
corruption
migration
cancelled
permission
internal
```

Do not use exception text as a protocol contract. Exceptions may be used internally for unrecoverable local control flow only if consistently translated at the application boundary.

## Durable jobs

Long-running or provider-dependent work is represented as durable jobs:

- Extraction.
- Consolidation.
- Vector catch-up.
- Vector rebuild.
- Vector verification.
- Environment reconstruction.
- Import/export.
- Large integrity verification.

Job states:

```text
queued
running
waiting_provider
waiting_dependency
succeeded
failed
cancelled
```

Jobs record attempts, next retry time, progress, phase, checkpoints, and final result/error.

## Cancellation

Cancellation is cooperative:

- Service clients may request job cancellation.
- Provider requests carry cancellation IDs.
- Vector batches stop between documents or safe batch boundaries.
- Database transactions complete or roll back; they are not abandoned midway.

## Internal events

Publish application events after successful commits:

```text
memory.created
memory.updated
memory.superseded
operation.applied
operation.rejected
recall.completed
recall.outcome_recorded
vector_store.created
vector_store.ready
vector_store.activated
vector_store.failed
provider.profile_changed
session.consolidated
```

Events contain sequence, timestamp, affected IDs, and causal operation/job ID. They are not raw SQLite trigger events.

## Notifications

The PMA service may emit JSON-RPC notifications for job progress and important status changes. Clients must tolerate reconnecting and querying current state rather than depending on every notification being delivered.

## Retry policy

Retry only errors marked retryable. Use bounded exponential delay with jitter for remote providers. Local provider process crashes may trigger one controlled restart before jobs remain queued or failed.

Provider-invalid output is not retried indefinitely. Retain the raw invalid response and validation diagnostics for testing and diagnosis.
