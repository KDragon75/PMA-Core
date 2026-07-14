# PMA service protocol

PMA-Core exposes a process-level JSON-RPC 2.0 application protocol. It does not expose a public native C++ API. Pi, MCP, CLI, tests, and future hosts call the same application services.

## MVP transport

Newline-delimited JSON-RPC over stdio:

```text
stdin  -> requests and notifications
stdout -> protocol responses and notifications only
stderr -> diagnostics only
```

Every JSON-RPC message occupies one line. Payload text fields may contain escaped newlines. Protocol logging must never be written to stdout.

## Initialization

The client calls `pma.initialize` with:

- Protocol version range.
- Client and adapter identity.
- Requested capabilities.
- Active environment observations.
- Optional session source identity.
- Optional external provider configuration containing a command argument array and provider-specific configuration object. Secrets are symbolic environment references, not literal persisted values.

When configured, initialization starts the external provider, records its effective runtime identity, and selects an exact embedding profile. Provider failure does not prevent authoritative initialization.

The server returns:

- Selected protocol version.
- Core and schema versions.
- Capabilities.
- Provider availability.
- Vector-store status.
- Feature flags.

## Service groups

### System

```text
pma.initialize
pma.health
pma.get_capabilities
pma.get_status
pma.shutdown
```

### Observation

```text
pma.session.open
pma.session.close
pma.interaction.begin
pma.observe.event
pma.interaction.complete
```

### Recall and context

```text
pma.recall
pma.context.get_packet
pma.context.render_slice
pma.recall.record_outcome
```

### Knowledge inspection and correction

```text
pma.memory.get
pma.memory.search
pma.memory.history
pma.memory.confirm
pma.memory.propose_correction
pma.memory.suppress
pma.memory.restore
```

### Learning

```text
pma.learning.propose
pma.learning.process_interaction
pma.learning.consolidate
pma.learning.get_operation
```

### Vector administration

```text
pma.vector.list
pma.vector.inspect
pma.vector.plan
pma.vector.build
pma.vector.catch_up
pma.vector.verify
pma.vector.activate
pma.vector.retire
pma.vector.reconstruct_environment
pma.vectors.sync
```

`pma.vectors.sync` resumes queued learning jobs, catches up an exact compatible active projection, or builds, validates, and activates the first generation. It is safe to invoke at session startup and after settled learning. Failure returns degraded status while authoritative evidence and queued jobs remain intact.

### Jobs

```text
pma.job.get
pma.job.cancel
pma.job.list
```

## Request rules

- All write requests accept an idempotency key.
- External IDs are namespaced by adapter and environment.
- Long-running methods return a job ID.
- Responses use typed result schemas rather than arbitrary JSON blobs.
- Unknown fields are tolerated according to protocol version policy; invalid required fields fail before domain processing.

## Errors

Stable error objects contain:

- Machine-readable code.
- Category.
- Human-readable message.
- Retryable flag.
- Operation or job ID where available.
- Structured details safe for the calling adapter.

See [Errors, jobs, and events](../05-implementation/ERRORS_JOBS_EVENTS.md).

## Versioning

Version separately:

- PMA service protocol.
- Core SQLite schema.
- Vector SQLite schema.
- Knowledge object schema.
- Provider protocol.
- Pi adapter.
- MCP adapter.

Protocol compatibility rules are in [Release and versioning](../07-delivery/RELEASE_AND_VERSIONING.md).
