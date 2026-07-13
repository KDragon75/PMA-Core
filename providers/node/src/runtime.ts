import { createHash } from "node:crypto";
import { readdir, readFile, stat } from "node:fs/promises";
import path from "node:path";

export async function hashFile(file: string): Promise<string> {
  return createHash("sha256").update(await readFile(file)).digest("hex");
}

export async function hashArtifacts(root?: string): Promise<string> {
  if (!root) return "unavailable";
  const files: string[] = [];
  async function walk(directory: string): Promise<void> {
    for (const entry of await readdir(directory, { withFileTypes: true })) {
      const full = path.join(directory, entry.name);
      if (entry.isDirectory()) await walk(full);
      else if (entry.isFile()) files.push(full);
    }
  }
  try {
    if (!(await stat(root)).isDirectory()) return hashFile(root);
    await walk(root);
  } catch {
    return "unavailable";
  }
  files.sort();
  const hash = createHash("sha256");
  for (const file of files) {
    hash.update(path.relative(root, file).replaceAll("\\", "/"));
    hash.update("\0");
    hash.update(await readFile(file));
  }
  return hash.digest("hex");
}

export function hashOptions(value: unknown): string {
  return createHash("sha256").update(JSON.stringify(value)).digest("hex");
}

export async function packageLockHash(): Promise<string> {
  const candidates = [new URL("../../package-lock.json", import.meta.url), new URL("../../../package-lock.json", import.meta.url)];
  for (const candidate of candidates) {
    try {
      return createHash("sha256").update(await readFile(candidate)).digest("hex");
    } catch { /* try parent workspace */ }
  }
  return "unavailable";
}
