#!/usr/bin/env node
import { cpSync, existsSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { basename, resolve } from "node:path";

const [coreArgument, outputArgument] = process.argv.slice(2);
if (!coreArgument || !outputArgument) {
  console.error("usage: node scripts/create-pi-bundle.mjs <pma-core executable> <output directory>");
  process.exit(2);
}
const root = resolve(import.meta.dirname, "..");
const core = resolve(coreArgument);
const output = resolve(outputArgument);
if (!existsSync(core)) throw new Error(`core executable not found: ${core}`);
rmSync(output, { recursive: true, force: true });
mkdirSync(`${output}/core`, { recursive: true });
cpSync(core, `${output}/core/${basename(core)}`);
for (const [source, destination] of [
  ["adapters/pi", "extension"],
  ["providers/node", "provider"],
]) {
  mkdirSync(`${output}/${destination}`, { recursive: true });
  for (const entry of ["package.json", "package-lock.json", "LICENSE", "src", "dist"]) {
    const from = `${root}/${source}/${entry}`;
    if (existsSync(from)) cpSync(from, `${output}/${destination}/${entry}`, { recursive: true });
  }
}
cpSync(`${root}/providers/node/MODEL_SELECTION.md`, `${output}/provider/MODEL_SELECTION.md`);
for (const entry of ["LICENSE", "THIRD_PARTY_NOTICES.md"]) cpSync(`${root}/${entry}`, `${output}/${entry}`);
writeFileSync(`${output}/provider-config.example.json`, JSON.stringify({
  kind: "transformers-js",
  model: "onnx-community/Qwen3-Embedding-0.6B-ONNX",
  dimensions: 1024,
  pooling: "last_token",
  normalize: true,
  queryPrefix: "Instruct: Retrieve relevant persistent-memory knowledge for the query.\nQuery: ",
  documentPrefix: "",
  maxTokens: 32768,
  localModelPath: "/absolute/path/to/embedding-model",
  generationModel: "/absolute/path/to/structured-generation-model"
}, null, 2) + "\n");
writeFileSync(`${output}/INSTALL.md`, `# PMA Pi bundle installation\n\nRequires Node 24 and a compatible Pi installation. Models are not included.\n\n1. Run \`npm install --omit=dev\` in both \`provider\` and \`extension\`.\n2. Run \`pi install /absolute/path/to/this/bundle/extension\`.\n3. Set \`PMA_CORE_PATH\` to \`core/${basename(core)}\`.\n4. Set \`PMA_PROVIDER_COMMAND\` to a JSON array such as \`[\"node\",\"/absolute/path/to/this/bundle/provider/dist/src/cli.js\"]\`.\n5. Copy \`provider-config.example.json\`, set the model path or remote endpoint, and place its compact JSON content in \`PMA_PROVIDER_CONFIG\`.\n6. Start Pi and run \`/pma-status\`. After configured learning, status reports an active vector generation.\n`);
console.log(output);
