# Portable references

A portable memory system cannot treat an absolute path, session ID, task ID, or agent-local identifier as globally valid.

## Resource identity

Resources receive an internal stable ID and may have several identifying signals:

- Logical name.
- Repository identity.
- Repository-relative path.
- Version-control revision.
- Content hash and size.
- URI or document locator.
- Environment-specific observed locations.
- Aliases and prior names.

No single field is always available. Resolution uses the strongest available combination.

## Observed locations

An absolute path is stored with its environment:

```text
Environment: workstation-a
Absolute path: C:\Users\Curtis\src\PMA-Core\src\memory.cpp
Observed at: 2026-07-13T...
```

After migration, this path remains historical evidence. PMA-Core must not test or open it unless the active environment matches or a mapping explicitly resolves it.

## Resolution order

1. Explicit environment mapping.
2. Repository identity plus relative path.
3. Artifact identity or revision.
4. Content hash.
5. Known aliases.
6. Unresolved result.

An unresolved reference is valid. The system should state that the resource was observed previously but cannot currently be located.

## Environment mappings

Mappings are configuration, not semantic truth:

```text
repository pma-core
old root: C:\Users\Curtis\src\PMA-Core
new root: /home/curtis/src/pma-core
```

Mappings may be local-only and excluded from exported bundles when they reveal sensitive paths.

## External identifiers

Pi session IDs, MCP client IDs, task IDs, and provider IDs are namespaced by source adapter and environment. They are provenance keys, not portable primary keys.

## Testing

Portability tests must move a bundle between Windows and Linux, change repository roots, remove old vector files, and verify that knowledge remains searchable. See [Portability and resilience](../06-testing/PORTABILITY_AND_RESILIENCE.md).
