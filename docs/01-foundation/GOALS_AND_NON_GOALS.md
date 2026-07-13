# Goals and non-goals

## Primary goals

### Human-like functional learning

PMA-Core implements the useful functional pattern described in [MEMORY_LIFECYCLE.md](../02-architecture/MEMORY_LIFECYCLE.md): observe, encode, validate, consolidate, retrieve, apply, evaluate, and reconsolidate. It does not attempt neurological simulation.

### Transparency

Every derived memory must expose:

- Original supporting evidence.
- Whether it was stated, observed, imported, or inferred.
- Confidence and current status.
- Operations that created or changed it.
- Contradictions and superseded versions.
- Why it was retrieved.

Transparency data remains in the internal [ContextPacket](../04-interfaces/CONTEXT_PACKET_AND_SLICE.md); only task-relevant knowledge is injected.

### Portability

The authoritative state is a portable SQLite bundle. Knowledge must remain usable when:

- The response model changes.
- The embedding model changes.
- The provider changes.
- The agent harness changes.
- The operating environment changes.
- Old absolute paths no longer exist.

See [Portable references](../02-architecture/PORTABLE_REFERENCES.md) and [Vector projections](../03-storage/VECTOR_PROJECTIONS.md).

### Speed

PMA-Core must not make every interaction wait for expensive consolidation. Evidence capture is immediate; extraction and consolidation may be queued. Retrieval must degrade to FTS5 and graph search when providers are unavailable.

### Simplicity and maintainability

- Use ordinary C++20 and explicit SQL.
- Keep dependencies small.
- Prefer finite operation vocabularies over arbitrary model behavior.
- Keep model runtimes outside PMA-Core.
- Avoid parallel representations of authoritative knowledge.

## Non-goals

PMA-Core is not initially:

- A chatbot or response generator.
- A graph database product.
- A general document management system.
- A vector database server.
- A biological brain simulation.
- A multi-tenant authorization platform.
- A cloud service requiring central infrastructure.
- A model-training system.
- An autonomous authority allowed to rewrite evidence.
- A guarantee that MCP-only hosts will perform automatic recall or learning.

## Explicit exclusions for the first release

- In-process model runtimes.
- Public native C++ API or stable C ABI.
- Remote inbound HTTP service.
- Automatic unrestricted ontology growth.
- Unbounded graph traversal.
- Automatic hard deletion based on decay.
- Graph neural networks or learned retrieval policy.
- Distributed SQLite synchronization.
- Multiple simultaneous writers to one core database.
