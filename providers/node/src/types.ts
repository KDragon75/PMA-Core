export type JsonRpcId = string | number;

export interface RuntimeDescription {
  providerPackage: string;
  providerVersion: string;
  runtimeType: "transformers-js" | "openai-compatible";
  nodeVersion: string;
  transformersVersion: string | null;
  os: string;
  architecture: string;
  device: string;
  backend: string;
  model: {
    id: string;
    revision: string;
    artifactHash: string;
    tokenizerHash: string;
  };
  embedding: {
    dimensions: number;
    dataType: "float32";
    pooling: string;
    normalize: boolean;
    queryPrefix: string;
    documentPrefix: string;
    maxTokens: number;
    truncation: string;
  };
  lockfileHash: string;
  optionsHash: string;
}

export interface ProviderConfig {
  kind: "transformers-js" | "openai-compatible";
  model: string;
  revision?: string;
  dimensions: number;
  pooling?: "mean" | "cls";
  normalize?: boolean;
  queryPrefix?: string;
  documentPrefix?: string;
  maxTokens?: number;
  truncation?: "end" | "start";
  device?: string;
  dtype?: string;
  cacheDir?: string;
  localModelPath?: string;
  localOnly?: boolean;
  baseUrl?: string;
  apiKey?: string;
  generationModel?: string;
  artifactRoot?: string;
}

export interface ProviderAdapter {
  describeRuntime(): Promise<RuntimeDescription>;
  embedDocuments(texts: string[], signal: AbortSignal): Promise<number[][]>;
  embedQuery(text: string, signal: AbortSignal): Promise<number[]>;
  generate(prompt: string, signal: AbortSignal): Promise<string>;
}
