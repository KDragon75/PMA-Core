import readline from "node:readline";

const input = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
const pending = new Map();
input.on("line", line => {
  const request = JSON.parse(line);
  if (request.method === "crash") process.exit(23);
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
