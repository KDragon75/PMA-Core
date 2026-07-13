# ADR-0004: Context is provenance, not ownership

**Status:** Accepted

## Context

PMA-Core is a general portable knowledge platform. Memories should not be partitioned as belonging to a user, project, agent, session, task, or model. Those objects still provide meaning and relevance.

## Decision

Represent people, projects, environments, sessions, tasks, agents, and models as contexts and provenance relationships. Do not introduce a cognitive identity or “mind” into the knowledge path.

## Consequences

- Knowledge can remain useful across tools and environments.
- Project/session context influences activation without acting as ownership.
- Model identity is optional audit metadata.
- Retrieval and access control remain separate concepts.

See [Context and provenance](../02-architecture/CONTEXT_AND_PROVENANCE.md).
