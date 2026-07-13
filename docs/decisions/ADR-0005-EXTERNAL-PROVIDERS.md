# ADR-0005: External model providers

**Status:** Accepted

## Context

Linking inference runtimes into PMA-Core would increase dependencies, hardware coupling, crash scope, and rebuild complexity.

## Decision

All embedding and generation providers run outside PMA-Core. The initial first-party Node provider supports Transformers.js locally and OpenAI-compatible remote endpoints.

## Consequences

- Core remains independent of ONNX Runtime, CUDA, ROCm, and model files.
- Provider crashes are isolated.
- Provider protocol and runtime manifests are required.
- The core can operate in degraded mode without providers.
- Direct HTTP from C++ is deferred.

See [Model providers](../04-interfaces/MODEL_PROVIDERS.md).
