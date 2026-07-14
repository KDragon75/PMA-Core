import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import readline from "node:readline";

export interface PmaService {
  request<T = unknown>(method: string, params?: Record<string, unknown>): Promise<T>;
  close(): Promise<void>;
}

export class ChildPmaService implements PmaService {
  private child: ChildProcessWithoutNullStreams | undefined;
  private nextId = 1;
  private readonly pending = new Map<number, { resolve(value: unknown): void; reject(error: Error): void }>();
  constructor(private readonly command: string, private readonly args: string[] = []) {}

  async start(initialize: Record<string, unknown>): Promise<void> {
    if (this.child?.exitCode === null) return;
    const child = spawn(this.command, this.args, { stdio: ["pipe", "pipe", "pipe"], windowsHide: true });
    this.child = child;
    child.stdin.setDefaultEncoding("utf8");
    readline.createInterface({ input: child.stdout, crlfDelay: Infinity }).on("line", line => this.onLine(line));
    child.on("exit", () => {
      if (this.child === child) this.child = undefined;
      for (const pending of this.pending.values()) pending.reject(new Error("PMA service exited"));
      this.pending.clear();
    });
    await new Promise<void>((resolve, reject) => { child.once("spawn", resolve); child.once("error", reject); });
    await this.request("pma.initialize", initialize);
  }

  async request<T = unknown>(method: string, params: Record<string, unknown> = {}): Promise<T> {
    if (!this.child || this.child.exitCode !== null) throw new Error("PMA service unavailable");
    const id = this.nextId++;
    const result = new Promise<T>((resolve, reject) => this.pending.set(id, {
      resolve: value => resolve(value as T), reject
    }));
    this.child.stdin.write(`${JSON.stringify({ jsonrpc: "2.0", id, method, params })}\n`);
    return result;
  }

  async close(): Promise<void> {
    const child = this.child;
    if (!child) return;
    try { await this.request("pma.shutdown"); } catch { /* already stopped */ }
    child.stdin.end();
    if (child.exitCode === null) await Promise.race([
      new Promise<void>(resolve => child.once("exit", () => resolve())),
      new Promise<void>(resolve => setTimeout(resolve, 1000))
    ]);
    if (child.exitCode === null) {
      child.kill();
      await Promise.race([
        new Promise<void>(resolve => child.once("exit", () => resolve())),
        new Promise<void>(resolve => setTimeout(resolve, 1000))
      ]);
    }
    this.child = undefined;
  }

  private onLine(line: string): void {
    let response: { id?: number; result?: unknown; error?: { message?: string; data?: unknown } };
    try { response = JSON.parse(line) as typeof response; } catch { return; }
    if (typeof response.id !== "number") return;
    const pending = this.pending.get(response.id);
    if (!pending) return;
    this.pending.delete(response.id);
    if (response.error) pending.reject(new Error(response.error.message ?? "PMA request failed"));
    else pending.resolve(response.result);
  }
}
