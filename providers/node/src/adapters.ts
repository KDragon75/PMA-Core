import os from "node:os";
import { hashArtifacts, hashOptions, packageLockHash } from "./runtime.js";
import type { ProviderAdapter, ProviderConfig, RuntimeDescription } from "./types.js";

interface TensorLike { data: Float32Array | number[]; dims?: number[] }
type FeaturePipeline = (input: string | string[], options: Record<string, unknown>) => Promise<TensorLike>;
type GenerationPipeline = (input: string, options: Record<string, unknown>) => Promise<Array<{ generated_text: string }> | { generated_text: string }[]>;

function baseRuntime(config: ProviderConfig, runtimeType: RuntimeDescription["runtimeType"], artifactHash: string, tokenizerHash: string): Promise<RuntimeDescription> {
  return packageLockHash().then(lockfileHash => ({
    providerPackage: "@pma/node-provider",
    providerVersion: "0.1.0",
    runtimeType,
    nodeVersion: process.version,
    transformersVersion: runtimeType === "transformers-js" ? "4.2.0" : null,
    os: os.platform(), architecture: os.arch(), device: config.device ?? "cpu",
    backend: runtimeType === "transformers-js" ? "onnxruntime" : "remote-http",
    model: { id: config.model, revision: config.revision ?? "main", artifactHash, tokenizerHash },
    embedding: {
      dimensions: config.dimensions, dataType: "float32", pooling: config.pooling ?? "mean",
      normalize: config.normalize ?? true, queryPrefix: config.queryPrefix ?? "",
      documentPrefix: config.documentPrefix ?? "", maxTokens: config.maxTokens ?? 512,
      truncation: config.truncation ?? "end"
    },
    lockfileHash,
    optionsHash: hashOptions({ dtype: config.dtype ?? "fp32", localOnly: config.localOnly ?? false,
      baseUrl: config.baseUrl ?? null, generationModel: config.generationModel ?? config.model })
  }));
}

export class TransformersAdapter implements ProviderAdapter {
  private feature?: FeaturePipeline;
  private generator?: GenerationPipeline;
  constructor(private readonly config: ProviderConfig) {}

  private async loadFeature(): Promise<FeaturePipeline> {
    if (!this.feature) {
      const transformers = await import("@huggingface/transformers");
      transformers.env.allowRemoteModels = !(this.config.localOnly ?? false);
      if (this.config.cacheDir) transformers.env.cacheDir = this.config.cacheDir;
      const model = this.config.localModelPath ?? this.config.model;
      const options = {
        revision: this.config.revision ?? "main", device: this.config.device ?? "cpu",
        dtype: this.config.dtype ?? "fp32", local_files_only: this.config.localOnly ?? false
      } as Parameters<typeof transformers.pipeline>[2];
      this.feature = await transformers.pipeline("feature-extraction", model, options) as unknown as FeaturePipeline;
    }
    return this.feature;
  }

  private async loadGenerator(): Promise<GenerationPipeline> {
    if (!this.generator) {
      const transformers = await import("@huggingface/transformers");
      transformers.env.allowRemoteModels = !(this.config.localOnly ?? false);
      if (this.config.cacheDir) transformers.env.cacheDir = this.config.cacheDir;
      const model = this.config.localModelPath ?? this.config.generationModel ?? this.config.model;
      const options = {
        revision: this.config.revision ?? "main", device: this.config.device ?? "cpu",
        dtype: this.config.dtype ?? "fp32", local_files_only: this.config.localOnly ?? false
      } as Parameters<typeof transformers.pipeline>[2];
      this.generator = await transformers.pipeline("text-generation", model, options) as unknown as GenerationPipeline;
    }
    return this.generator;
  }

  async describeRuntime(): Promise<RuntimeDescription> {
    const artifact = await hashArtifacts(this.config.artifactRoot ?? this.config.localModelPath);
    return baseRuntime(this.config, "transformers-js", artifact, artifact);
  }

  private async embed(texts: string[], prefix: string, signal: AbortSignal): Promise<number[][]> {
    if (signal.aborted) throw new DOMException("cancelled", "AbortError");
    const pipeline = await this.loadFeature();
    const output: number[][] = [];
    for (const text of texts) {
      if (signal.aborted) throw new DOMException("cancelled", "AbortError");
      const tensor = await pipeline(prefix + text, { pooling: this.config.pooling ?? "mean",
        normalize: this.config.normalize ?? true });
      output.push(Array.from(tensor.data));
    }
    return output;
  }
  embedDocuments(texts: string[], signal: AbortSignal): Promise<number[][]> {
    return this.embed(texts, this.config.documentPrefix ?? "", signal);
  }
  async embedQuery(text: string, signal: AbortSignal): Promise<number[]> {
    return (await this.embed([text], this.config.queryPrefix ?? "", signal))[0] ?? [];
  }
  async generate(prompt: string, signal: AbortSignal): Promise<string> {
    if (signal.aborted) throw new DOMException("cancelled", "AbortError");
    const pipeline = await this.loadGenerator();
    const output = await pipeline(prompt, { max_new_tokens: 512, do_sample: false, return_full_text: false });
    return output[0]?.generated_text ?? "";
  }
}

export class OpenAIAdapter implements ProviderAdapter {
  constructor(private readonly config: ProviderConfig, private readonly fetchImpl: typeof fetch = fetch) {
    if (!config.baseUrl) throw new Error("baseUrl is required");
  }
  describeRuntime(): Promise<RuntimeDescription> {
    return baseRuntime(this.config, "openai-compatible", "remote-managed", "remote-managed");
  }
  private headers(): Record<string, string> {
    const configured = this.config.apiKey;
    const apiKey = configured?.startsWith("env:") ? process.env[configured.slice(4)] : configured;
    return { "content-type": "application/json", ...(apiKey ? { authorization: `Bearer ${apiKey}` } : {}) };
  }
  async embedDocuments(texts: string[], signal: AbortSignal): Promise<number[][]> {
    const response = await this.fetchImpl(`${this.config.baseUrl}/v1/embeddings`, {
      method: "POST", headers: this.headers(), signal,
      body: JSON.stringify({ model: this.config.model, input: texts, dimensions: this.config.dimensions })
    });
    if (!response.ok) throw new Error(`remote embeddings failed (${response.status})`);
    const body = await response.json() as { data?: Array<{ index: number; embedding: number[] }> };
    return (body.data ?? []).sort((a, b) => a.index - b.index).map(item => item.embedding);
  }
  async embedQuery(text: string, signal: AbortSignal): Promise<number[]> {
    return (await this.embedDocuments([text], signal))[0] ?? [];
  }
  async generate(prompt: string, signal: AbortSignal): Promise<string> {
    const response = await this.fetchImpl(`${this.config.baseUrl}/v1/chat/completions`, {
      method: "POST", headers: this.headers(), signal,
      body: JSON.stringify({ model: this.config.generationModel ?? this.config.model,
        messages: [{ role: "user", content: prompt }], temperature: 0 })
    });
    if (!response.ok) throw new Error(`remote generation failed (${response.status})`);
    const body = await response.json() as { choices?: Array<{ message?: { content?: string } }> };
    return body.choices?.[0]?.message?.content ?? "";
  }
}

export function createAdapter(config: ProviderConfig): ProviderAdapter {
  return config.kind === "transformers-js" ? new TransformersAdapter(config) : new OpenAIAdapter(config);
}
