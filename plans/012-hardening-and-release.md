# Task 012 hardening and first release

## Objective
Produce a documented, reproducible 0.1.0 cross-platform release with safe maintenance CLI operations, migration/recovery coverage, package artifacts, explicit compatibility/security limits, and enforced release gates.

## Governing documents
- [Task 012](../tasks/012-HARDENING-AND-RELEASE.md)
- [Definition of done](../docs/07-delivery/DEFINITION_OF_DONE.md)
- [Release and versioning](../docs/07-delivery/RELEASE_AND_VERSIONING.md)
- [Security and privacy](../docs/05-implementation/SECURITY_AND_PRIVACY.md)

## Constraints
- Preserve authoritative evidence/history and all released migration checksums.
- Refuse unsafe maintenance rather than hiding corruption or incompatibility.
- Models and secrets are excluded from release artifacts.
- Package only pinned/build-produced dependencies and include their notices.
- Preserve the unrelated dirty `.pi/sessions/pma-core-build-session.jsonl` file.

## Current state
- Tasks 001–011 are on `main` at `5684569`; local Windows Debug/Release suites pass 52 CTest cases and all Node suites pass.
- Product/package versions are 0.1.0 and protocols are v1; core migrations currently span bootstrap plus semantic versions 100, 101, 200, 300, and 400.
- The executable only starts stdio service mode; storage already exposes online backup and verification primitives.
- CI builds/tests supported compilers, but no install/CPack release workflow or packaged maintenance CLI exists.
- Third-party notices exist but explicitly defer resolved vcpkg notices; release compatibility, upgrade/recovery, security review, and known-limit documents are absent.

## Work sequence
1. Add safe `backup`, `restore`, `verify`, and `compact` executable commands and deterministic CLI tests.
2. Add supported schema fixtures/matrix and portability, vector-loss, provider/profile, Pi/MCP, and release-stress gate coverage.
3. Add install/CPack rules and independently packed Node provider, Pi, and MCP artifacts with reproducible release workflow checksums.
4. Complete licenses/notices, compatibility, upgrade/recovery, security review, performance thresholds, and known limitations.
5. Run all local acceptance suites, package smoke checks, update this plan, commit, push, and stop.

## Tests
- Windows Debug and Release configure/build/CTest presets.
- Maintenance CLI backup/restore/verify/compact integration.
- Fresh database and every supported migration fixture.
- Missing/corrupt vector recovery and moved-bundle checks.
- Provider outage/profile change and Pi/MCP end-to-end suites.
- 10,000-interaction release simulation.
- CPack archive creation and fresh extraction smoke.
- All four Node package tests and package dry-runs.
- `git diff --check` and diagnostics.

## Risks and rollback
Restore and compaction can damage authoritative state if implemented as in-place shortcuts. Restore therefore verifies a staged copy before replacement, and compact refuses failed verification. Packaging changes are additive and can be removed without schema changes.

## Progress
- [x] Governing documents and repository state reviewed.
- [x] Maintenance CLI and tests complete.
- [x] Release-gate fixtures and tests complete.
- [x] Reproducible packages/workflow complete.
- [x] Release/security/compatibility documentation complete.
- [x] Full local acceptance evidence recorded and release commit prepared.

## Decisions made during execution
- Version 0.1.0 supports fresh initialization and current schema 400. Intermediate migration numbers were development steps, not independently supported public starting releases.
- The unapproved project license is represented conservatively as all-rights-reserved/`UNLICENSED`; no distribution rights were inferred. Public license selection is recorded in `OPEN_QUESTIONS.md`.
- Native packages use CPack ZIP/TGZ and Node integrations use npm tarballs; the release workflow publishes lockfiles, notices, and SHA-256 manifests alongside them.

## Completion evidence
- Windows MSVC Debug and Release configured, built, and passed 55/55 CTest cases each.
- Maintenance tests verify online backup, staged restore, integrity checks, compaction, authoritative-row preservation, and refusal of missing/corrupt inputs.
- Release fixture tests verify fresh migration through 100/101/200/300/400 and idempotent current-schema reopen.
- Provider passed 4/4, Pi 4/4, MCP 5/5, and hidden-world simulation 5/5; Pi and MCP child shutdown leave no process behind.
- The 10,000-interaction release simulation reported zero deterministic failures, stale-memory injection, irrelevant injection, and unexplained data loss, with precision/recall 1.0.
- CPack produced Windows ZIP and TGZ archives; fresh ZIP extraction contained the executable, license, notices, migration matrix, security review, compatibility/limitations, and recovery guide.
- npm pack produced provider, Pi, and MCP 0.1.0 tarballs. The release workflow builds/tests Windows and Linux native artifacts, tests/packs Node artifacts, and emits SHA-256 manifests.
- `git diff --check` passed. Linux GCC execution and release artifact reproduction remain CI-validated after push.

