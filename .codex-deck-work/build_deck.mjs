import fs from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";

const ROOT = "C:/Users/abhin/OneDrive/Desktop/verilog";
const WORK = path.join(ROOT, ".codex-deck-work");
const BUILD = path.join(WORK, "build");
const PREVIEW = path.join(WORK, "draft-render");
const SKILL = "C:/Users/abhin/.codex/plugins/cache/openai-primary-runtime/presentations/26.1024.11018/skills/presentations";
const RUNTIME_NODE_MODULES = "C:/Users/abhin/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules";
const FONT = "Segoe UI";

const C = {
  paper: "#F4F2EC",
  white: "#FFFFFF",
  ink: "#142231",
  dark: "#0E1A28",
  dark2: "#192B3D",
  text: "#22313B",
  muted: "#65747A",
  line: "#D7DDD8",
  teal: "#1A7472",
  tealLight: "#DDECE8",
  coral: "#D36D4D",
  coralLight: "#F2E2D9",
  gold: "#D5A33E",
  pale: "#E8EBE6",
};

const { Presentation, PresentationFile } = await (async () => {
  const req = await import(pathToFileURL(path.join(RUNTIME_NODE_MODULES, "@oai/artifact-tool", "dist", "index.js")).href).catch(async () => {
    const { createRequire } = await import("node:module");
    const require = createRequire(path.join(RUNTIME_NODE_MODULES, "__runtime__.cjs"));
    const resolved = require.resolve("@oai/artifact-tool");
    return import(pathToFileURL(resolved).href);
  });
  return req;
})();

const presentation = Presentation.create({ slideSize: { width: 1280, height: 720 } });

function addBox(slide, name, x, y, w, h, fill, stroke = C.line, strokeWidth = 1) {
  return slide.shapes.add({
    geometry: "rect",
    name,
    position: { left: x, top: y, width: w, height: h },
    fill,
    line: { style: "solid", fill: stroke, width: strokeWidth },
  });
}

function addText(slide, name, value, x, y, w, h, options = {}) {
  const shape = slide.shapes.add({
    geometry: "textbox",
    name,
    position: { left: x, top: y, width: w, height: h },
    fill: "none",
    line: { style: "solid", fill: "none", width: 0 },
  });
  shape.text = value;
  shape.text.style = {
    typeface: FONT,
    fontSize: options.size ?? 24,
    color: options.color ?? C.text,
    bold: options.bold ?? false,
    alignment: options.align ?? "left",
    verticalAlignment: options.valign ?? "middle",
    autoFit: "shrinkText",
    wrap: "square",
    insets: options.insets ?? 0,
  };
  return shape;
}

function addRule(slide, x1, y1, x2, y2, color = C.line, width = 1) {
  return slide.shapes.add({
    geometry: "line",
    position: { left: x1, top: y1, width: x2 - x1, height: y2 - y1 },
    fill: "none",
    line: { style: "solid", fill: color, width },
  });
}

function connect(slide, from, to, options = {}) {
  return slide.shapes.connect(from, to, {
    kind: options.kind ?? "elbow",
    fromSide: options.fromSide ?? "right",
    toSide: options.toSide ?? "left",
    line: { style: "solid", fill: options.color ?? C.teal, width: options.width ?? 2.5 },
    ...(options.head ? { head: options.head } : {}),
    ...(options.tail ? { tail: options.tail } : {}),
  });
}

async function addImage(slide, name, relPath, x, y, w, h, options = {}) {
  const full = path.join(ROOT, relPath);
  const bytes = await fs.readFile(full);
  const ext = path.extname(full).toLowerCase();
  const contentType = ext === ".jpg" || ext === ".jpeg" ? "image/jpeg" : "image/png";
  return slide.images.add({
    blob: bytes,
    contentType,
    alt: options.alt ?? name,
    fit: options.fit ?? "cover",
    position: { left: x, top: y, width: w, height: h },
    ...(options.crop ? { crop: options.crop } : {}),
  });
}

function makeSlide(title, options = {}) {
  const slide = presentation.slides.add();
  slide.background.fill = options.background ?? C.paper;
  const dark = options.dark ?? false;
  addText(slide, `title-${title}`, title, 72, 46, 1136, 76, {
    size: 56,
    bold: true,
    color: dark ? C.white : C.ink,
  });
  addRule(slide, 72, 137, 146, 137, dark ? C.coral : C.teal, 4);
  return slide;
}

function addImageFrame(slide, name, x, y, w, h, fill = C.white) {
  return addBox(slide, `${name}-frame`, x, y, w, h, fill, C.line, 1.2);
}

// 1. Cover
{
  const slide = presentation.slides.add();
  slide.background.fill = C.dark;
  addRule(slide, 82, 111, 150, 111, C.coral, 4);
  addText(slide, "cover-team", "SIH26181   TEAM CHIPSTERS   MEDTECH TRACK", 82, 70, 510, 34, {
    size: 18,
    bold: true,
    color: C.gold,
  });
  addText(slide, "cover-valor", "VALOR", 78, 169, 500, 112, {
    size: 86,
    bold: true,
    color: C.white,
  });
  addText(slide, "cover-expanded", "Vital and Atmospheric Logic\nfor Offline Rescue", 82, 281, 500, 120, {
    size: 35,
    bold: false,
    color: C.white,
  });
  addText(slide, "cover-descriptor", "A local health and exposure monitor\nfor disaster response", 84, 424, 475, 74, {
    size: 25,
    color: "#C3CDD1",
  });
  addText(slide, "cover-round", "FIRST-ROUND HACKATHON SUBMISSION", 84, 628, 480, 31, {
    size: 18,
    bold: true,
    color: C.coral,
  });
  addImageFrame(slide, "cover-prototype", 614, 146, 602, 430, C.dark2);
  await addImage(slide, "Current ShrikeFi bench board", "docs/images/hardware/valor_zeropcb.jpeg", 626, 158, 578, 326, {
    alt: "ShrikeFi board and sensor modules assembled on a prototype board",
    fit: "cover",
  });
  addText(slide, "cover-photo-caption", "ShrikeFi prototype board", 630, 501, 560, 36, {
    size: 20,
    color: "#D5DFE0",
  });
  addText(slide, "cover-photo-stage", "Current bench build", 630, 538, 560, 29, {
    size: 18,
    color: "#9CADB1",
  });
  slide.speakerNotes.text = "Project name, team, track and identifier follow the provided VALOR Explainer Deck. Prototype photo: docs/images/hardware/valor_zeropcb.jpeg. The image shows the current bench board, not a finished wearable enclosure.";
}

// 2. Why context matters
{
  const slide = makeSlide("Vitals need exposure context");
  addText(slide, "context-lead", "A person's pulse sits inside a changing environment. VALOR combines both views on the device for a local first-pass assessment.", 72, 151, 1118, 64, {
    size: 24,
    color: C.muted,
  });

  const person = addBox(slide, "context-person-box", 72, 246, 315, 144, C.white, C.line, 1.2);
  addBox(slide, "context-person-accent", 72, 246, 7, 144, C.coral, C.coral, 0);
  addText(slide, "context-person-heading", "PERSON", 101, 265, 250, 35, { size: 20, bold: true, color: C.coral });
  addText(slide, "context-person-copy", "Heart rate, SpO₂, beat intervals, breathing estimate", 101, 307, 252, 62, { size: 22, color: C.ink });

  const exposure = addBox(slide, "context-exposure-box", 72, 430, 315, 144, C.white, C.line, 1.2);
  addBox(slide, "context-exposure-accent", 72, 430, 7, 144, C.teal, C.teal, 0);
  addText(slide, "context-exposure-heading", "SURROUNDINGS", 101, 449, 250, 35, { size: 20, bold: true, color: C.teal });
  addText(slide, "context-exposure-copy", "Ambient heat and humidity, PM₂.₅, pressure trend, motion", 101, 491, 252, 65, { size: 22, color: C.ink });

  const local = addBox(slide, "context-triage-box", 492, 328, 282, 205, C.dark, C.dark, 1);
  addText(slide, "context-triage-heading", "LOCAL TRIAGE", 518, 361, 230, 40, { size: 25, bold: true, color: C.white });
  addText(slide, "context-triage-copy", "Signal checks\nClinical rules\nHazard rules", 518, 409, 230, 94, { size: 22, color: "#D4DDDF" });

  const output = addBox(slide, "context-output-box", 872, 328, 336, 205, C.tealLight, C.teal, 1.5);
  addText(slide, "context-output-heading", "LOCAL OUTPUTS", 901, 360, 280, 40, { size: 24, bold: true, color: C.teal });
  addText(slide, "context-output-copy", "OLED, LED, buzzer\nand a nearby phone page", 901, 412, 280, 73, { size: 23, color: C.ink });

  connect(slide, person, local, { color: C.coral, fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  connect(slide, exposure, local, { color: C.teal, fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  connect(slide, local, output, { color: C.ink, kind: "straight", fromSide: "right", toSide: "left", tail: { type: "arrow", width: "med", length: "med" } });
  addText(slide, "context-footnote", "The first-pass sensing and decision path does not depend on an internet connection.", 72, 622, 1100, 34, {
    size: 21,
    color: C.muted,
  });
  slide.speakerNotes.text = "System intent and local processing are described in docs/MASTER_PROJECT_GUIDE.md and docs/SHRIKEFI_LINK_PROTOCOL.md. The diagram is an editable summary of local sensor fusion and outputs.";
}

// 3. System architecture
{
  const slide = makeSlide("System architecture");

  const sensors = addBox(slide, "architecture-sensors", 62, 185, 302, 424, C.white, C.line, 1.2);
  addText(slide, "architecture-sensors-heading", "SENSORS", 88, 202, 248, 35, { size: 20, bold: true, color: C.teal });
  const sensorRows = [
    ["MAX30102", "Red and IR optical PPG"],
    ["BME280", "Ambient temperature, RH, pressure"],
    ["PMS5003", "Particulate matter, UART"],
    ["MPU6050", "Motion and fall cues"],
    ["GNSS", "Outdoor position"],
  ];
  let sy = 248;
  for (let i = 0; i < sensorRows.length; i += 1) {
    addText(slide, `architecture-sensor-${i}-name`, sensorRows[i][0], 88, sy, 242, 29, { size: 22, bold: true, color: C.ink });
    addText(slide, `architecture-sensor-${i}-detail`, sensorRows[i][1], 88, sy + 29, 248, 34, { size: 19, color: C.muted });
    if (i < sensorRows.length - 1) addRule(slide, 88, sy + 68, 338, sy + 68, C.pale, 1);
    sy += 72;
  }

  const mcu = addBox(slide, "architecture-esp32", 430, 267, 375, 260, C.dark, C.dark, 1);
  addText(slide, "architecture-esp32-title", "ESP32-S3", 458, 286, 320, 42, { size: 31, bold: true, color: C.white });
  addText(slide, "architecture-esp32-copy", "Core 0  PPG capture and pulse analytics\nCore 1  clinical and hazard assessment\nFreeRTOS tasks share local device state", 458, 344, 320, 142, { size: 22, color: "#D2DDE0" });

  const fpga = addBox(slide, "architecture-forgefpga", 476, 558, 285, 112, C.white, C.teal, 1.6);
  addText(slide, "architecture-forgefpga-title", "ForgeFPGA SLG47910", 498, 568, 244, 32, { size: 22, bold: true, color: C.ink });
  addText(slide, "architecture-forgefpga-copy", "50 MHz filter and beat timer\n8-bit mode-0 SPI", 498, 605, 244, 53, { size: 19, color: C.teal });

  const outputs = addBox(slide, "architecture-outputs", 894, 185, 322, 424, C.white, C.line, 1.2);
  addText(slide, "architecture-outputs-heading", "LOCAL OUTPUTS", 921, 202, 268, 35, { size: 20, bold: true, color: C.teal });
  const outputRows = [
    ["128 × 64 OLED", "Vitals and alert state"],
    ["LED and buzzer", "At-device alert cues"],
    ["ESP32 SoftAP", "Phone status page and history"],
    ["USB serial", "Detailed desktop dashboard"],
  ];
  let oy = 256;
  for (let i = 0; i < outputRows.length; i += 1) {
    addText(slide, `architecture-output-${i}-name`, outputRows[i][0], 921, oy, 264, 31, { size: 22, bold: true, color: C.ink });
    addText(slide, `architecture-output-${i}-detail`, outputRows[i][1], 921, oy + 33, 264, 38, { size: 19, color: C.muted });
    if (i < outputRows.length - 1) addRule(slide, 921, oy + 76, 1185, oy + 76, C.pale, 1);
    oy += 83;
  }

  connect(slide, sensors, mcu, { color: C.teal, kind: "straight", fromSide: "right", toSide: "left", tail: { type: "arrow", width: "med", length: "med" } });
  connect(slide, mcu, outputs, { color: C.teal, kind: "straight", fromSide: "right", toSide: "left", tail: { type: "arrow", width: "med", length: "med" } });
  connect(slide, mcu, fpga, { color: C.coral, kind: "straight", fromSide: "bottom", toSide: "top", head: { type: "arrow", width: "sm", length: "sm" }, tail: { type: "arrow", width: "sm", length: "sm" } });
  addText(slide, "architecture-bus-caption", "Sensor buses: I²C / UART. FPGA link: 8-bit mode-0 SPI.", 72, 680, 1136, 24, { size: 18, color: C.muted, align: "center" });
  slide.speakerNotes.text = "Source: docs/SHRIKEFI_LINK_PROTOCOL.md, docs/SHRIKEFI_HARDWARE_CONNECTIONS.md, and firmware/shrikefi/main_shrikefi.c. The MCU samples MAX30102 data, sends the IR path to ForgeFPGA over mode-0 SPI, and receives beat/IBI data. Red/IR ratio processing for SpO₂ stays on the MCU. The dashboard interfaces are distinct: a compact local SoftAP page and a richer USB serial desktop page.";
}

// 4. FPGA beat timing
{
  const slide = makeSlide("FPGA beat timing", { background: C.dark, dark: true });
  const stages = [
    ["IR sample", "MAX30102 via ESP32"],
    ["8-tap average", "Running-sum filter"],
    ["Peak detector", "Crest rule, 250 ms refractory"],
    ["IBI capture", "32-bit count at 50 MHz"],
    ["SPI return", "0xC3 plus 32-bit interval"],
  ];
  const stageShapes = [];
  const startX = 58;
  const stageW = 218;
  const gap = 15;
  for (let i = 0; i < stages.length; i += 1) {
    const x = startX + i * (stageW + gap);
    const accent = i === 0 ? C.coral : C.teal;
    const s = addBox(slide, `ppg-stage-${i}`, x, 182, stageW, 128, C.dark2, "#385062", 1.2);
    stageShapes.push(s);
    addBox(slide, `ppg-stage-${i}-accent`, x, 182, 6, 128, accent, accent, 0);
    addText(slide, `ppg-stage-${i}-title`, stages[i][0], x + 22, 199, stageW - 40, 37, { size: 23, bold: true, color: C.white });
    addText(slide, `ppg-stage-${i}-body`, stages[i][1], x + 22, 244, stageW - 40, 53, { size: 19, color: "#CFD9DD" });
  }
  for (let i = 0; i < stageShapes.length - 1; i += 1) {
    connect(slide, stageShapes[i], stageShapes[i + 1], { color: C.gold, kind: "straight", fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  }

  addText(slide, "ppg-wave-title", "RTL simulation trace", 72, 347, 740, 30, { size: 19, bold: true, color: C.gold });
  addImageFrame(slide, "ppg-waveform", 62, 380, 750, 250, C.white);
  await addImage(slide, "ForgeFPGA RTL waveform", "docs/images/forgefpga_waveform.png", 74, 392, 726, 226, {
    alt: "ForgeFPGA simulation waveform showing filtered sample, detector state and IBI counter",
    fit: "contain",
  });
  addText(slide, "ppg-tick-title", "20 ns", 866, 386, 294, 78, { size: 62, bold: true, color: C.coral });
  addText(slide, "ppg-tick-copy", "Counter tick at 50 MHz. This is timer resolution, not beat accuracy.", 868, 468, 328, 74, { size: 21, color: "#D2DDE0" });
  addRule(slide, 868, 557, 1188, 557, "#405261", 1);
  addText(slide, "ppg-red-copy", "Red optical samples stay on the ESP32-S3 for SpO₂ ratio processing. Firmware checks IBI plausibility before publishing pulse metrics.", 868, 573, 328, 74, { size: 20, color: "#D2DDE0" });
  slide.speakerNotes.text = "Source: hardware/shrikefi/forgefpga_ppg_top.v, hardware/common/moving_average_8tap.v, hardware/common/ppg_peak_detector.v, and docs/SHRIKEFI_LINK_PROTOCOL.md. The waveform is an RTL simulation image (docs/images/forgefpga_waveform.png), not a human measurement. The 50 MHz counter has a 20 ns tick; firmware validates intervals and signal quality before publication.";
}

// 5. Rules and TinyML
{
  const slide = makeSlide("Rules and TinyML in the risk path");
  addText(slide, "risk-intro", "The model is an advisory path. Deterministic rules retain the hard safety thresholds.", 72, 151, 1100, 43, { size: 23, color: C.muted });

  const inputs = addBox(slide, "risk-inputs", 64, 226, 296, 340, C.white, C.line, 1.2);
  addText(slide, "risk-inputs-title", "ON-DEVICE SIGNALS", 88, 248, 248, 36, { size: 20, bold: true, color: C.teal });
  addText(slide, "risk-inputs-person-title", "Person", 88, 302, 248, 30, { size: 22, bold: true, color: C.ink });
  addText(slide, "risk-inputs-person", "HR, SpO₂, RR, RMSSD", 88, 335, 248, 38, { size: 21, color: C.muted });
  addRule(slide, 88, 390, 336, 390, C.pale, 1);
  addText(slide, "risk-inputs-exposure-title", "Exposure", 88, 411, 248, 30, { size: 22, bold: true, color: C.ink });
  addText(slide, "risk-inputs-exposure", "Ambient temperature and RH, PM₂.₅, pressure trend, motion", 88, 445, 248, 96, { size: 21, color: C.muted });

  const rules = addBox(slide, "risk-rules", 446, 205, 334, 151, C.dark, C.dark, 1);
  addText(slide, "risk-rules-title", "DETERMINISTIC RULES", 472, 222, 282, 34, { size: 20, bold: true, color: C.gold });
  addText(slide, "risk-rules-copy", "mNEWS2 subset for HR, SpO₂ and RR\nEnvironmental thresholds and fall logic\nHard overrides", 472, 265, 282, 79, { size: 20, color: C.white });

  const model = addBox(slide, "risk-model", 446, 397, 334, 169, C.tealLight, C.teal, 1.5);
  addText(slide, "risk-model-title", "TINYML ADVISORY", 472, 413, 282, 34, { size: 20, bold: true, color: C.teal });
  addText(slide, "risk-model-copy", "Layer sizes 6-24-16-3\n619 bytes of INT8 weights\n88.39% synthetic-teacher agreement", 472, 455, 282, 93, { size: 21, color: C.ink });

  const fusion = addBox(slide, "risk-fusion", 865, 287, 345, 205, C.white, C.coral, 1.5);
  addText(slide, "risk-fusion-title", "FUSED TRIAGE", 895, 309, 285, 40, { size: 25, bold: true, color: C.coral });
  addText(slide, "risk-fusion-copy", "Clinical and hazard outputs\ncombined locally. The model may\nraise an environmental tier.", 895, 362, 285, 92, { size: 22, color: C.ink });

  connect(slide, inputs, rules, { color: C.teal, fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  connect(slide, inputs, model, { color: C.gold, fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  connect(slide, rules, fusion, { color: C.teal, fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  connect(slide, model, fusion, { color: C.coral, fromSide: "right", toSide: "left", tail: { type: "arrow", width: "sm", length: "sm" } });
  addText(slide, "risk-caveat", "BME280 temperature is ambient, not body temperature. Teacher agreement on synthetic data is not clinical accuracy.", 72, 614, 1120, 43, { size: 19, color: C.muted });
  slide.speakerNotes.text = "Source: firmware/core/clinical_vitals_engine.c, firmware/core/disaster_risk_engine.c, firmware/core/nn_risk_model_int8.h, and docs/reports/AI_MODEL_AUDIT_2026-10-20.md. The deployed INT8 model has 619 bytes of parameter storage and 88.39% agreement with its synthetic rule teacher on a held-out synthetic validation set. This is not clinical performance. The device does not have a fitted body-temperature sensor; BME280 measures ambient air.";
}

// 6. Offline access
{
  const slide = makeSlide("Offline access on the ESP32-S3", { background: C.dark, dark: true });
  addImageFrame(slide, "dashboard-preview", 64, 165, 734, 466, C.dark2);
  await addImage(slide, "VALOR dashboard simulation screenshot", "docs/images/valor_dashboard_normal.png", 76, 177, 710, 442, {
    alt: "VALOR dark dashboard with vital signs and a visible simulation profile label",
    fit: "cover",
    crop: { left: 0, top: 0, right: 0, bottom: 0.42 },
  });
  addText(slide, "dashboard-preview-caption", "Desktop dashboard preview. This screenshot uses a simulation profile.", 76, 640, 720, 30, { size: 17, color: "#AEBBC0" });

  const blocks = [
    ["ON THE DEVICE", "OLED, LED and buzzer provide local alert cues."],
    ["PHONE NEARBY", "ESP32 SoftAP serves a compact status page and history."],
    ["DESKTOP VIEW", "A richer dashboard can connect over USB serial."],
    ["NETWORK PATH", "No cloud or external CDN is required for local status."],
  ];
  let y = 178;
  for (let i = 0; i < blocks.length; i += 1) {
    addText(slide, `offline-label-${i}`, blocks[i][0], 850, y, 350, 29, { size: 19, bold: true, color: i === 0 ? C.gold : C.teal });
    addText(slide, `offline-copy-${i}`, blocks[i][1], 850, y + 34, 350, 58, { size: 22, color: C.white });
    if (i < blocks.length - 1) addRule(slide, 850, y + 103, 1194, y + 103, "#405261", 1);
    y += 116;
  }
  slide.speakerNotes.text = "Source: firmware/shrikefi/web_status.c, firmware/shrikefi/dashboard/README.md, and docs/DEMO_RUNBOOK.md. The phone SoftAP page is embedded in firmware and served locally; the richer dashboard screenshot is the separate USB Web Serial page and is explicitly in simulation mode. The SoftAP is open and lacks authentication; the final slide records that field-use boundary.";
}

// 7. Measured implementation evidence
{
  const slide = makeSlide("Measured implementation evidence");

  addText(slide, "evidence-forge-label", "ACTIVE BUILD · RENESAS FORGEFPGA", 72, 160, 612, 32, { size: 19, bold: true, color: C.teal });
  addText(slide, "evidence-forge-luts", "515 / 1,120", 72, 191, 612, 89, { size: 72, bold: true, color: C.ink });
  addText(slide, "evidence-forge-lut-caption", "LUT5 resources in the current fitter report", 75, 278, 610, 33, { size: 22, color: C.muted });
  addText(slide, "evidence-forge-ff", "303 flip-flops; 105 of 140 CLBs used", 75, 320, 610, 35, { size: 22, color: C.ink });
  addText(slide, "evidence-forge-timing", "WNS +7.316 ns at 50 MHz; TNS = 0", 75, 361, 610, 35, { size: 22, color: C.ink });
  addText(slide, "evidence-forge-frequency", "Reported achievable frequency: 78.85 MHz", 75, 402, 610, 35, { size: 22, color: C.ink });

  addImageFrame(slide, "floorplan-view", 742, 160, 470, 370, C.dark);
  await addImage(slide, "ForgeFPGA floorplan", "docs/images/forgefpga_floorplan.png", 754, 172, 446, 346, {
    alt: "Renesas ForgeFPGA floorplan for the current PPG design",
    fit: "contain",
  });
  addText(slide, "floorplan-caption", "ForgeFPGA placement view", 756, 537, 446, 27, { size: 18, color: C.muted, align: "center" });

  addRule(slide, 72, 458, 690, 458, C.line, 1.2);
  addText(slide, "evidence-zynq-label", "PRESERVED REFERENCE · XILINX ZYNQ-7000", 72, 477, 1100, 31, { size: 19, bold: true, color: C.coral });
  addText(slide, "evidence-zynq-line1", "185 LUT, 16 LUTRAM, 266 FF, 0 DSP48, 0 BRAM", 72, 512, 1115, 35, { size: 22, color: C.ink });
  addText(slide, "evidence-zynq-line2", "WNS +5.603 ns · WHS +0.184 ns · Fmax 69.45 MHz · 6/6 tests", 72, 552, 1115, 35, { size: 22, color: C.ink });
  addText(slide, "evidence-caveat", "Separate devices and tool flows; utilization figures are not directly comparable.", 72, 607, 1120, 29, { size: 18, color: C.muted });
  slide.speakerNotes.text = "Current ForgeFPGA values: hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log and timing_summary_spi_link_50mhz.log (post-route, 50 MHz constraint). Zynq numbers and 6/6 tests: repository AGENTS.md verified baseline and hardware/zynq/tb_ppg_system.v. The ForgeFPGA and Zynq figures describe separate targets and must not be combined or compared as if they used the same LUT architecture.";
}

// 8. Prototype status and validation boundary
{
  const slide = makeSlide("Prototype status and validation gaps");
  addImageFrame(slide, "prototype-photo", 64, 160, 455, 480, C.white);
  await addImage(slide, "VALOR breadboard prototype", "docs/images/hardware/valor_breadboard.jpeg", 76, 172, 431, 432, {
    alt: "Bench prototype with ESP32, sensors and wiring during integration",
    fit: "cover",
    crop: { left: 0.03, top: 0.22, right: 0.03, bottom: 0.05 },
  });
  addText(slide, "prototype-photo-caption", "Bench prototype shown", 78, 610, 425, 28, { size: 18, color: C.muted, align: "center" });

  const x = 568;
  addText(slide, "prototype-build-label", "CURRENT BUILD", x, 172, 620, 29, { size: 19, bold: true, color: C.teal });
  addText(slide, "prototype-build-copy", "ESP32-S3 and ForgeFPGA with optical PPG, environmental sensors, motion sensing, GNSS and OLED.", x, 207, 620, 66, { size: 22, color: C.ink });
  addRule(slide, x, 291, 1192, 291, C.line, 1);
  addText(slide, "prototype-limits-label", "VALIDATION BOUNDARIES", x, 311, 620, 29, { size: 19, bold: true, color: C.coral });
  addText(slide, "prototype-limits-copy", "This is an engineering prototype. The BME280 measures ambient air; body temperature is not instrumented. The model was compared with a synthetic teacher, not clinical outcomes.", x, 346, 620, 103, { size: 21, color: C.ink });
  addRule(slide, x, 465, 1192, 465, C.line, 1);
  addText(slide, "prototype-next-label", "NEXT ENGINEERING WORK", x, 485, 620, 29, { size: 19, bold: true, color: C.teal });
  addText(slide, "prototype-next-copy", "Build and test an enclosure, validate sensors and power under field conditions, and add access control before exposing sensitive data over the open SoftAP.", x, 520, 620, 97, { size: 21, color: C.ink });
  slide.speakerNotes.text = "Source: docs/DEMO_RUNBOOK.md, firmware/shrikefi/dashboard/README.md, firmware/core/skin_estimate.c, and the current sensor path. The prototype photo is docs/images/hardware/valor_breadboard.jpeg. The embedded SoftAP is open and unauthenticated. Body temperature is not instrumented by the current clinical scoring path; the BME280 reports ambient air. No clinical efficacy or human-subject validation claim is made.";
}

await fs.mkdir(BUILD, { recursive: true });
await fs.mkdir(PREVIEW, { recursive: true });
const draftPath = path.join(BUILD, "candidate.pptx");
await (await PresentationFile.exportPptx(presentation)).save(draftPath);

for (let i = 0; i < presentation.slides.items.length; i += 1) {
  const slide = presentation.slides.items[i];
  const preview = await presentation.export({ slide, format: "png", scale: 1 });
  await fs.writeFile(path.join(PREVIEW, `slide-${String(i + 1).padStart(2, "0")}.png`), new Uint8Array(await preview.arrayBuffer()));
}
const montage = await presentation.export({ format: "png", montage: true, scale: 0.55 });
await fs.writeFile(path.join(PREVIEW, "montage.png"), new Uint8Array(await montage.arrayBuffer()));
console.log(JSON.stringify({ draftPath, slideCount: presentation.slides.items.length, previewDir: PREVIEW }, null, 2));
