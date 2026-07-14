# Supported migration matrix

| Starting release | Starting schema | Expected result | Support |
|---|---|---|---|
| Fresh install | bootstrap metadata | Apply 100, 101, 200, 300, and 400 | Supported |
| 0.1.0 | current through 400 | Idempotent open with checksum verification | Supported |

Versions 100, 101, 200, 300, and 400 were internal migration steps before the first public boundary, not independently released schemas. Upgrading from a database stopped at one of those development-only steps is not supported. The first durable public support boundary is 0.1.0/current schema 400. Future releases must add immutable fixture databases or fixture builders for every retained public release and must refuse checksum mismatches.
