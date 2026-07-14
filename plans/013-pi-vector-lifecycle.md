# Pi-triggered vector lifecycle

## Objective
Pi lifecycle completion automatically drives configured extraction and embedding work so durable claims and a compatible vector generation are created, activated, caught up, and used for recall without explicit memory-tool calls.

## Governing documents
- [Memory lifecycle](../docs/02-architecture/MEMORY_LIFECYCLE.md)
- [Vector projections](../docs/03-storage/VECTOR_PROJECTIONS.md)
- [Pi extension](../docs/04-interfaces/PI_EXTENSION.md)
- [Model providers](../docs/04-interfaces/MODEL_PROVIDERS.md)
- [Task 006](../tasks/006-VECTOR-PROJECTIONS.md)
- [Task 007](../tasks/007-NODE-PROVIDER.md)
- [Task 009](../tasks/009-PI-EXTENSION.md)

## Constraints
- SQLite remains authoritative; vector files remain disposable projections.
- PMA-Core must not load a model runtime or add HTTP dependencies.
- Provider configuration is explicit and secrets remain external.
- Provider failure must degrade to FTS/graph retrieval without losing observations.
- Response-model selection must not alter the embedding profile.

## Current state
- Lifecycle integration creates authoritative claims and embedding events.
- The vector manager can register profiles, build, validate, activate, catch up, and search stores.
- The provider process supports runtime description and embeddings.
- The Pi extension does not configure a provider or request vector synchronization, so `vectors/` remains empty.

## Work sequence
1. Add a provider-backed embedding adapter and deterministic vector synchronization coordinator in PMA-Core.
2. Extend initialization/status and add a lifecycle-safe synchronization service method with explicit provider configuration.
3. Pass provider configuration from the Pi extension and trigger synchronization after settled learning, without making provider availability authoritative.
4. Add native and Pi integration tests for first build, catch-up, failure degradation, and no duplicate lifecycle work.
5. Update packaging, compatibility, and deployment documentation so the provider and model/runtime configuration travel with the bundle.

## Tests
- Native coordinator tests with a protocol fixture provider.
- Service integration test verifies build/activation and status.
- Pi lifecycle test verifies one post-settled synchronization request and graceful failure.
- Existing vector, provider, service, retrieval, and Pi suites.
- Full affected CTest and npm suites.

## Risks and rollback
Provider startup or inference may be slow or unavailable. Synchronization is explicitly triggered after settled work and failures leave embedding events queued while lexical/graph recall remains available. Removing provider configuration restores the existing provider-free behavior.

## Progress
- [x] Governing contracts and current gap verified.
- [x] Core provider/vector coordinator implemented.
- [x] Service and Pi lifecycle integration implemented.
- [x] Packaging and documentation updated.
- [x] Debug and Release acceptance tests pass.

## Decisions made during execution
- Provider configuration remains explicit; no model download or remote endpoint is silently selected.
- Configured work runs synchronously at Pi session startup and after `agent_settled`; there is no process outside an active Pi session.
- Provider-generated durable statements are mapped deterministically to the learned-memory concept and validated against exact evidence IDs before commit.

## Completion evidence
- Configured service integration test performs observation, structured extraction, claim/document creation, vector build, validation, activation, and status reporting through a real fixture provider process.
- Lifecycle claim creation now creates canonical embedding documents and events atomically; vector initialization backfills hashes for pre-vector rows.
- Recall requests use the active compatible store and provider query embedding, with automatic lexical/graph degradation on failure.
- Pi requests resume/synchronization at session startup and once after settled learning; lifecycle tests verify the calls remain deterministic.
- Windows Debug and Release each passed 56/56 CTest cases; provider 4/4, Pi 4/4, MCP 5/5, and simulation 5/5 passed.
- `scripts/create-pi-bundle.mjs` produced a smoke-tested bundle containing core, Pi extension, provider, notices, and an explicit provider configuration template.
