# Observability

Observability must support debugging memory behavior without turning internal reasoning or sensitive evidence into uncontrolled logs.

## Three distinct records

### Diagnostic logs

Human-readable or JSON-lines process diagnostics written to stderr or a configured file. Used for startup, provider health, retry, performance, and errors.

### Domain audit

Structured SQLite records for memory operations, provider runs, corrections, vector activation, and administrative actions. This is authoritative history, not a log file.

### Retrieval telemetry

Structured packet, candidate, score, and outcome records used to evaluate retrieval quality.

Do not mix these concerns.

## Log fields

- Timestamp.
- Level.
- Component.
- Event name.
- Correlation/interaction/job ID.
- Short message.
- Safe structured fields.

Never log secrets or entire conversations by default.

## Metrics

Track:

- Evidence ingestion latency and throughput.
- Queue depth and job age.
- Provider request latency and error rate.
- FTS, vector, graph, and fusion latency.
- ContextSlice tokens and selected item count.
- Vector replay lag.
- Database and vector-file sizes.
- Retrieval usefulness and irrelevant-injection rates.

Metrics may initially be available through `pma.get_status` and benchmark output rather than a dedicated metrics server.

## Explainability commands

Administrative inspection should answer:

- Why was this memory created?
- What evidence supports it?
- What changed it?
- Why was it retrieved?
- Which candidates were excluded and why?
- Which vector profile was used?
- Is the projection current?

## Trace capture

Tests may enable full captured traces. Production defaults retain hashes and metadata rather than raw provider prompts unless debug capture is explicitly enabled.
