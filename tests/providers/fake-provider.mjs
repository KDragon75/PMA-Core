import readline from "node:readline";

const input = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
const pending = new Map();
input.on("line", line => {
  const request = JSON.parse(line);
  if (request.method === "crash") process.exit(23);
  if (request.method === "provider.initialize") {
    const dimensions = request.params?.config?.dimensions ?? 3;
    process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: {
      protocolVersion: 1, initialized: true, runtime: {
        providerPackage: "@pma/fake-provider", providerVersion: "0.1.0",
        runtimeType: "test", nodeVersion: process.version, os: process.platform,
        architecture: process.arch, backend: "fake", lockfileHash: "fake-lock",
        optionsHash: "fake-options", model: { artifactHash: "fake-model", revision: "1", tokenizerHash: "fake-tokenizer" },
        embedding: { dimensions, pooling: "mean", normalize: true, queryPrefix: "", documentPrefix: "",
          maxTokens: 512, truncation: "end" }
      }
    } })}\n`);
    return;
  }
  if (request.method === "embedding.embed_documents") {
    const dimensions = 3;
    const vectors = request.params.texts.map((text) =>
      Array.from({ length: dimensions }, (_, index) => index === text.length % dimensions ? 1 : 0));
    process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: { vectors } })}\n`);
    return;
  }
  if (request.method === "embedding.embed_query") {
    process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: { vector: [1, 0, 0] } })}\n`);
    return;
  }
  if (request.method === "generation.generate_structured") {
    const evidenceMatch = String(request.params.task).match(/\[([^\]]+)\]/);
    const value = { memories: evidenceMatch ? [{ statement: "The user prefers durable vector recall.",
      evidence_ids: [evidenceMatch[1]], confidence: 0.95 }] : [] };
    process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: { value, raw: JSON.stringify(value), attempts: 1 } })}\n`);
    return;
  }
  if (request.method === "provider.shutdown") {
    process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: { accepted: true } })}\n`);
    input.close();
    return;
  }
  if (request.method === "slow") {
    const timer = setTimeout(() => {
      pending.delete(String(request.id));
      process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: { completed: true } })}\n`);
    }, 5000);
    pending.set(String(request.id), timer);
    return;
  }
  if (request.method === "provider.cancel") {
    const id = String(request.params.id);
    const timer = pending.get(id);
    if (timer) {
      clearTimeout(timer);
      pending.delete(id);
      process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id, error: { code: -32800, message: "cancelled" } })}\n`);
    }
    return;
  }
  process.stdout.write(`${JSON.stringify({ jsonrpc: "2.0", id: request.id, result: { status: "ok" } })}\n`);
});
