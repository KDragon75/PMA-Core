# Release and versioning

## Independent versions

Track separately:

- Product release version.
- PMA service protocol.
- Provider protocol.
- Core SQLite schema.
- Vector SQLite schema.
- Knowledge-object schema.
- Pi adapter.
- MCP adapter.
- Prompt and extraction schema assets.

One product version must not be used as a substitute for compatibility checks.

## Semantic versioning

Use semantic versioning for product and protocol packages after the first public release. Before 1.0, breaking changes are allowed but require explicit migration and compatibility notes.

## Protocol compatibility

- Initialization negotiates supported protocol range.
- Additive optional fields are backward compatible.
- New required fields or changed semantics require a protocol version change.
- Unknown methods return stable method-not-found errors.
- Clients must not infer support from product version alone.

## Database compatibility

- Newer core schemas may be opened read-only by older versions only when explicitly supported.
- Unsupported older binaries must refuse writes.
- Vector schema upgrades may rebuild projection files rather than mutate in place.
- Core migrations preserve authoritative history.

## Provider compatibility

Provider initialization negotiates capability and protocol versions. Vector compatibility uses runtime/profile fingerprints and conformance, not adapter version alone.

## Release artifacts

Initial release artifacts:

- Windows x64 PMA-Core executable and required runtime files.
- Linux x64 PMA-Core executable.
- Node provider package with lockfile.
- Pi extension package.
- MCP adapter mode included with PMA-Core or packaged alongside it.
- Schema and migration documentation.
- Third-party notices.

Models are not necessarily redistributed. The runtime reconstruction manifest identifies required artifacts and acquisition rules.

## Support policy

Define at release time:

- Supported upgrade paths.
- Number of prior schema versions covered by tests.
- Supported Node and compiler versions.
- Provider and model compatibility expectations.
- Deprecation period for protocol methods.
