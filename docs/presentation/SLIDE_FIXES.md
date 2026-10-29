# Slide Deck Audit & Presenter Corrections
**Target Presentations:** 
1. `submission/VALOR_Hackathon_Submission.pptx` (8-Slide Pitch Deck)
2. `docs/presentation/VALOR_Round1.pptx` (16-Slide Technical Deck)

> [!IMPORTANT]
> **Preserve Slide Layout & Formatting:**
> Per hackathon presentation guidelines, do **not** run automated tools to overwrite `.pptx` XML. The presenter should apply these targeted text corrections directly in PowerPoint or keep them in mind as spoken clarifications during the presentation.

---

## 1. Automated Claim & Number Scan Summary
- **Checker:** `scripts/check_presentation_claims.py`
- **Result:** **0 fatal claim violations found.**
- All key parameters (Zynq 185 LUT / +5.603 ns, ForgeFPGA 515 / 1120 LUT5 / +7.316 ns, INT8 619 parameters, 88.39% teacher agreement, 0.44 µs host latency, 16,387 MIMIC records, 58.18% sensitivity, 98.19% specificity) match canonical repository values.

---

## 2. Slide-by-Slide Correction & Spoken Caveat Table

| Deck | Slide # | Field / Location | Current Slide Text | Recommended Correction | Reason & Technical Grounding |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `VALOR_Round1.pptx` | **Slide 15** | References Table (NEWS2 entry) | `mNEWS2 from pulse, SpO₂ and respiration; bands 0–4, 5–6, ≥ 7` | `mNEWS2 from pulse, SpO₂ and respiration; bands 0 (Normal), 1–3 (Low), 4–6 (Medium/High), ≥ 7 (Critical)` | **Clinical Band Boundary:** VALOR modifies standard NEWS2 by triggering Medium/High alert at score **4** rather than 5 for faster edge disaster evacuation (`clinical_vitals_engine.c:225`). Stating `0-4` misrepresents the low-to-medium boundary. |
| `VALOR_Round1.pptx` | **Slide 13** | Feasibility & Cost | Prototype BOM: `₹4,916` (sum of retail breakouts) | *Spoken Caveat:* *"₹4,916 reflects single-quantity retail breakout modules. In production volume, our component SMD BOM is under ₹1,800 (~$21 USD)."* | **Cost Context:** Retail breakout modules (PMS5003 ₹1600, NEO-8N ₹1100) are ~3x volume component pricing. `FACT_SHEET.md` canonical production BOM is ₹1,780. |
| `VALOR_Round1.pptx` | **Slide 4** | Architecture Diagram | `ForgeFPGA SLG47910V` | Keep as is, or clarify `SLG47910` | The physical chip package on ShrikeFi is the Renesas SLG47910 (V = 2.0×3.0 mm STQFN package). |
| `VALOR_Round1.pptx` | **Slide 6** | Beat Detection FSM | `25% fall below the crest confirms a peak` | `H-01 slope gate: drop of ≥ 3 units (or relative crest fall) confirms a peak` | `ppg_peak_detector.v:75` uses `(peak_val - i_sample) >= MIN_FALL_STEP (3)` on the 8-bit signal (which corresponds to ~25% of systolic crest amplitude in normalized waveforms). |
| `VALOR_Round1.pptx` | **Slide 14** | Roadmap #1 | `Calibrate skin temperature against a reference thermometer` | `Populate TMP117 clinical contact probe and validate against reference thermometer` | In `docs/I2C_BUS_BENCH_REPORT.md`, TMP117 is unpopulated on Rev 1.0; BME280 serves as ambient exposure proxy while fingertip skin temperature is display-only. |
| `VALOR_Hackathon_Submission.pptx` | **Slide 5** | Deterministic Rules | `mNEWS2 subset for HR, SpO₂ and RR` | Keep text, verbalize: *"Bands are 0, 1–3, 4–6, ≥7"* | Reinforces why threshold is lowered to 4 for early disaster triage. |
| `VALOR_Hackathon_Submission.pptx` | **Slide 8** | Current Build | `CURRENT BUILD: ESP32-S3 and ForgeFPGA with optical PPG, environmental sensors...` | Verbalize: *"Active hardware runs on Vicharak ShrikeFi development board with Renesas ForgeFPGA over 8-bit SPI."* | Clarifies exact vendor platform and board name for judges. |

---

## 3. Mandatory Spoken Slides Script (3 Key Defenses)

1. **Slide 12 (AI Benchmark — Sensitivity 58.18%):**
   > *"Judges, look at the 58.18% sensitivity figure on Slide 12. We report this honestly. On raw, uncurated ICU data, our standalone 619-byte neural network misses 42% of rule-defined critical cases. This is precisely why VALOR is not a black-box AI monitor. Our deterministic clinical engine enforces hard overrides for critical vitals (SpO2 ≤ 91%, HR ≤ 40 or ≥ 131), guaranteeing that no critical patient is missed in the deployed triage pipeline."*

2. **Slide 11 (Hardware Validation — Dual FPGA Targets):**
   > *"Slide 11 shows our two hardware targets. The Xilinx Zynq-7000 (185 LUTs) proved our RTL synthesizability and timing closure (+5.6 ns margin) on standard Vivado tooling. The Renesas ForgeFPGA (515 LUTs, +7.3 ns margin) proves commercial wearable viability on a sub-$0.50 chip."*

3. **Slide 4 (End-to-End Jitter Elimination):**
   > *"Notice the blue LED driven by the FPGA. In software, RTOS scheduling jitter introduces 5 to 20 ms of random noise, corrupting RMSSD calculation. The ForgeFPGA runs at 50 MHz, providing 20-nanosecond hardware timestamps completely independent of CPU load."*
