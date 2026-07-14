import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import readline from "node:readline";

export interface RpcClient {
  start(): Promise<void>;
  stop(): Promise<void>;
  request<T = unknown>(method: string, params?: Record<string, unknown>): Promise<T>;
  notify(method: string, params?: Record<string, unknown>): void;
}

interface Pending {
  resolve(value: unknown): void;
  reject(error: Error): void;
}

export class PmaClient implements RpcClient {
  private child: ChildProcessWithoutNullStreams | undefined;
  private nextId = 1;
  private readonly pending = new Map<number, Pending>();
  private stopping = false;

  constructor(private readonly executable: string, private readonly args: string[] = []) {}

  async start(): Promise<void> {
    if (this.child && this.child.exitCode === null) return;
    this.stopping = false;
    const child = spawn(this.executable, this.args, { stdio: ["pipe", "pipe", "pipe"], windowsHide: true });
    this.child = child;
    child.stdin.setDefaultEncoding("utf8");
    const lines = readline.createInterface({ input: child.stdout, crlfDelay: Infinity });
    lines.on("line", line => this.handleLine(line));
    child.on("exit", () => {
      if (this.child === child) this.child = undefined;
      for (const pending of this.pending.values()) pending.reject(new Error("PMA process exited"));
      this.pending.clear();
    });
    await new Promise<void>((resolve, reject) => {
      child.once("spawn", resolve);
      child.once("error", reject);
    });
  }

  async stop(): Promise<void> {
    const child = this.child;
    if (!child) return;
    this.stopping = true;
    try { await this.request("pma.shutdown"); } catch { /* process may already be gone */ }
    child.stdin.end();
    if (child.exitCode === null) {
      await Promise.race([
        new Promise<void>(resolve => child.once("exit", () => resolve())),
        new Promise<void>(resolve => setTimeout(resolve, 1000))
      ]);
    }
    if (child.exitCode === null) {
      child.kill();
      await Promise.race([
        new Promise<void>(resolve => child.once("exit", () => resolve())),
        new Promise<void>(resolve => setTimeout(resolve, 1000))
      ]);
    }
    this.child = undefined;
  }

  async request<T = unknown>(method: string, params: Record<string, unknown> = {}): Promise<T> {
    await this.ensureRunning();
    const id = this.nextId++;
    const promise = new Promise<T>((resolve, reject) => {
      this.pending.set(id, { resolve: value => resolve(value as T), reject });
    });
    this.child!.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", id, method, params })}\n`);
    return promise;
  }

  notify(method: string, params: Record<string, unknown> = {}): void {
    if (!this.child || this.child.exitCode !== null) return;
    this.child.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", method, params })}\n`);
  }

  private async ensureRunning(): Promise<void> {
    if (!this.child || this.child.exitCode !== null) {
      if (this.stopping) throw new Error("PMA client is stopping");
      await this.start();
    }
  }

  private handleLine(line: string): void {
    let message: { id?: number; result?: unknown; error?: { message?: string; data?: unknown } };
    try { message = JSON.parse(line) as typeof message; }
    catch { return; }
    if (typeof message.id !== "number") return;
    const pending = this.pending.get(message.id);
    if (!pending) return;
    this.pending.delete(message.id);
    if (message.error) pending.reject(new Error(message.error.message ?? "PMA request failed"));
    else pending.resolve(message.result);
  }
}
