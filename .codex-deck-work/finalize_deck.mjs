import fs from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";

const ROOT = "C:/Users/abhin/OneDrive/Desktop/verilog";
const WORK = path.join(ROOT, ".codex-deck-work");
const SKILL = "C:/Users/abhin/.codex/plugins/cache/openai-primary-runtime/presentations/26.1024.11018/skills/presentations";
const candidatePath = path.join(WORK, "build", "candidate.pptx");
const finalPath = path.join(ROOT, "submission", "VALOR_Hackathon_Submission.pptx");
const stagingDir = path.join(WORK, "finalizer");
await fs.mkdir(stagingDir, { recursive: true });
await fs.mkdir(path.dirname(finalPath), { recursive: true });

const { finalizePresentation } = await import(pathToFileURL(path.join(SKILL, "container_tools/artifact_tool_utils.mjs")).href);
const result = await finalizePresentation({
  explicitTotalSlideCount: 8,
  requiredNativeTableOwnerSlides: [],
  requiredNativeChartOwnerSlides: [],
  workspaceDir: ROOT,
  candidatePath,
  finalPath,
  pythonExecutable: "C:/Users/abhin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe",
  integrityValidatorPath: path.join(SKILL, "container_tools/inspect_presentation_package_integrity.py"),
  layoutValidatorPath: path.join(SKILL, "container_tools/inspect_presentation_layout_geometry.py"),
  layoutArgs: [
    "--expected-slide-size-emu", "12192000,6858000",
    "--validate-bullet-geometry",
    "--validate-heading-fit",
  ],
  fontPolicy: { basis: "design", families: ["Segoe UI"] },
  verifyArtifactToolImport: true,
  receiptPath: path.join(stagingDir, "VALOR_Hackathon_Submission.validation.json"),
});
console.log(JSON.stringify({ finalPath, result }, null, 2));
