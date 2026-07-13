import { Ajv, type ErrorObject } from "ajv/dist/ajv.js";

export interface StructuredResult {
  value: unknown;
  raw: string;
  attempts: number;
}

export class StructuredGenerationError extends Error {
  constructor(message: string, readonly raw: string, readonly diagnostics: ErrorObject[] | string[]) {
    super(message);
  }
}

export function extractJson(text: string): unknown {
  for (let start = 0; start < text.length; start++) {
    if (text[start] !== "{" && text[start] !== "[") continue;
    const stack: string[] = [];
    let quoted = false;
    let escaped = false;
    for (let index = start; index < text.length; index++) {
      const character = text[index];
      if (quoted) {
        if (escaped) escaped = false;
        else if (character === "\\") escaped = true;
        else if (character === '"') quoted = false;
        continue;
      }
      if (character === '"') quoted = true;
      else if (character === "{" || character === "[") stack.push(character);
      else if (character === "}" || character === "]") {
        const expected = character === "}" ? "{" : "[";
        if (stack.pop() !== expected) break;
        if (stack.length === 0) {
          try { return JSON.parse(text.slice(start, index + 1)); } catch { break; }
        }
      }
    }
  }
  throw new Error("no JSON value found");
}

export async function generateStructured(
  task: string,
  schema: object,
  generate: (prompt: string) => Promise<string>
): Promise<StructuredResult> {
  const validate = new Ajv({ allErrors: true, strict: false }).compile(schema);
  let feedback = "";
  let lastRaw = "";
  let diagnostics: ErrorObject[] | string[] = [];
  for (let attempt = 1; attempt <= 2; attempt++) {
    const prompt = `${task}\nReturn exactly one JSON value matching this schema:\n${JSON.stringify(schema)}${feedback}`;
    lastRaw = await generate(prompt);
    try {
      const value = extractJson(lastRaw);
      if (validate(value)) return { value, raw: lastRaw, attempts: attempt };
      diagnostics = validate.errors ?? [];
      feedback = `\nPrevious output failed validation: ${JSON.stringify(diagnostics)}. Correct it.`;
    } catch (error) {
      diagnostics = [error instanceof Error ? error.message : "invalid JSON"];
      feedback = `\nPrevious output was not valid JSON: ${diagnostics[0]}. Correct it.`;
    }
  }
  throw new StructuredGenerationError("structured output invalid after retry", lastRaw, diagnostics);
}
