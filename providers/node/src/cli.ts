#!/usr/bin/env node
import readline from "node:readline";
import { ProviderServer } from "./server.js";

const server = new ProviderServer();
const input = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
let pending = 0;
input.on("line", line => {
  pending++;
  void server.handleLine(line).then(response => {
    if (response !== undefined) process.stdout.write(`${response}\n`);
    if (server.shutdownRequested) input.close();
  }).catch(error => {
    console.error("provider host failure", error);
    process.exitCode = 1;
    input.close();
  }).finally(() => { pending--; });
});
input.on("close", () => {
  if (pending === 0 && !server.shutdownRequested) process.exitCode = process.exitCode ?? 0;
});
