# PMA-Core documentation map

This directory is ordered from foundational intent to delivery. Documents are deliberately narrow so an agent can load only the material needed for the current task.

## 01 — Foundation

- [Goals and non-goals](01-foundation/GOALS_AND_NON_GOALS.md)
- [Glossary](01-foundation/GLOSSARY.md)
- [Locked decisions](01-foundation/DECISIONS.md)
- [Open questions](01-foundation/OPEN_QUESTIONS.md)

## 02 — Architecture

- [System architecture](02-architecture/SYSTEM_ARCHITECTURE.md)
- [Memory lifecycle](02-architecture/MEMORY_LIFECYCLE.md)
- [Knowledge graph](02-architecture/KNOWLEDGE_GRAPH.md)
- [Memory processing](02-architecture/MEMORY_PROCESSING.md)
- [Context and provenance](02-architecture/CONTEXT_AND_PROVENANCE.md)
- [Portable references](02-architecture/PORTABLE_REFERENCES.md)

## 03 — Storage

- [SQLite core](03-storage/SQLITE_CORE.md)
- [Vector projections](03-storage/VECTOR_PROJECTIONS.md)
- [Migrations, backup, and recovery](03-storage/MIGRATIONS_BACKUP_RECOVERY.md)

## 04 — Interfaces

- [PMA service protocol](04-interfaces/SERVICE_PROTOCOL.md)
- [ContextPacket and ContextSlice](04-interfaces/CONTEXT_PACKET_AND_SLICE.md)
- [Pi extension](04-interfaces/PI_EXTENSION.md)
- [MCP adapter](04-interfaces/MCP_ADAPTER.md)
- [Provider boundary](04-interfaces/MODEL_PROVIDERS.md)
- [Transformers.js provider](04-interfaces/TRANSFORMERS_JS_PROVIDER.md)

## 05 — Implementation

- [Dependencies and build](05-implementation/DEPENDENCIES_AND_BUILD.md)
- [Repository layout](05-implementation/REPOSITORY_LAYOUT.md)
- [Errors, jobs, and events](05-implementation/ERRORS_JOBS_EVENTS.md)
- [Security and privacy](05-implementation/SECURITY_AND_PRIVACY.md)
- [Observability](05-implementation/OBSERVABILITY.md)

## 06 — Testing

- [Overall test strategy](06-testing/TEST_STRATEGY.md)
- [Functional tests](06-testing/FUNCTIONAL_TESTS.md)
- [Performance tests](06-testing/PERFORMANCE_TESTS.md)
- [LLM-driven stress tests](06-testing/LLM_STRESS_TEST.md)
- [Portability and resilience](06-testing/PORTABILITY_AND_RESILIENCE.md)

## 07 — Delivery

- [Roadmap](07-delivery/ROADMAP.md)
- [Work packages](07-delivery/WORK_PACKAGES.md)
- [Definition of done](07-delivery/DEFINITION_OF_DONE.md)
- [Release and versioning](07-delivery/RELEASE_AND_VERSIONING.md)
- [Compatibility and known limitations](07-delivery/COMPATIBILITY_AND_LIMITATIONS.md)
- [Migration matrix](07-delivery/MIGRATION_MATRIX.md)
- [Upgrade and recovery](07-delivery/UPGRADE_AND_RECOVERY.md)
- [0.1.0 security review](07-delivery/SECURITY_REVIEW_0.1.0.md)

## Decision records and references

- [Decision records](decisions/README.md)
- [External references](references/EXTERNAL_REFERENCES.md)

## Agent navigation rule

Read the selected task first, then only the linked design documents. Read the full documentation set only when modifying a cross-cutting contract.
