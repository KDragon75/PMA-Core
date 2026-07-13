# Task 012 — Hardening and first release

## Objective

Turn the end-to-end prototype into a supported cross-platform release.

## Governing documents

- [Definition of done](../docs/07-delivery/DEFINITION_OF_DONE.md)
- [Release and versioning](../docs/07-delivery/RELEASE_AND_VERSIONING.md)
- [Security and privacy](../docs/05-implementation/SECURITY_AND_PRIVACY.md)

## Deliverables

- Supported migration matrix.
- Backup, restore, verify, and compact CLI commands.
- Windows and Linux packaging.
- Node provider and Pi extension packages.
- License and third-party notices.
- Security and prompt-injection review.
- Performance release thresholds.
- Compatibility table and known limitations.
- Upgrade and recovery documentation.

## Release tests

- Fresh install.
- Upgrade from every supported schema fixture.
- Cross-platform bundle move.
- Missing/corrupt vector recovery.
- Provider outage and profile change.
- Pi and MCP end-to-end flows.
- Long-run stress release scenario.

## Acceptance criteria

Every item in [Definition of done](../docs/07-delivery/DEFINITION_OF_DONE.md) passes, release artifacts are reproducible, and known limitations are explicit rather than hidden behind fallback behavior.
