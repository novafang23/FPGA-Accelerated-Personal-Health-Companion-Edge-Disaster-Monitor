# VALOR Complete Guide Synthesis Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Synthesize the forensic RTL and TinyML truths from Revision 1 into `VALOR_COMPLETE_GUIDE.md` while resolving all `[VERIFY]` tags with verified repository ground truth.

**Architecture:** Update `VALOR_COMPLETE_GUIDE.md` section by section. Preserve all 24 required sections (0 through 23), maintain structural invariants ("> **In one line:**" and "::: tip Judge might ask"), and verify with Python regression tests and hardware/firmware testbenches.

**Tech Stack:** Verilog RTL, C11 ESP-IDF FreeRTOS, INT8 TinyML, Markdown, Python verification scripts.

**Spec:** The user prompt and ground truth audit matrix in `walkthrough.md`.

## Global Constraints
- Target file: `VALOR_COMPLETE_GUIDE.md`.
- No invented numbers: all timing (+7.316 ns ForgeFPGA, +5.603 ns Zynq), resources (515 LUT5s, 105 CLBs), and accuracy metrics (88.39% INT8, 94.11% MIMIC-III triage) must match tracked repository evidence.
- Zero unconfirmed `[VERIFY]` tags remaining in the final text.
- Preserve all 24 required sections (0 to 23).

---

### Task 1: Incorporate RTL Forensic Truths into Sections 5, 8, 9, and 10
**Files:**
- Modify: `VALOR_COMPLETE_GUIDE.md` (Sections 5, 8, 9, 10)
- Verify: Python structure and RTL check script

- [ ] **Step 1:** In Section 5, document the 7-bit MISO wrap (modulo 128 sawtooth) due to Bit 7 beat flag, and confirm only the IR channel streams to the FPGA (Red is cached locally in MCU software).
- [ ] **Step 2:** In Section 9, document `first_beat_seen` first-crest suppression and explain the proportional crest fall rule (`peak_val >> 2`) including why the `8'd6` floor is inactive when `dyn_threshold = 120`.
- [ ] **Step 3:** In Section 10, update the `spi_target.v` description to clarify it is a 3-bit bit counter and shift register rather than a classical state machine.

### Task 2: Incorporate AI Sparsity & INT8 Dequantization Truths into Section 13
**Files:**
- Modify: `VALOR_COMPLETE_GUIDE.md` (Section 13)
- Verify: Python script against `firmware/core/nn_risk_model_int8.c.inc`

- [ ] **Step 1:** Document the exact parameter sparsity: 619 total bytes, 391 zeros, 11 dead neurons in Layer 1, 6 dead neurons in Layer 2, explaining how L1 regularization yields an effective 6→13→10→3 architecture.
- [ ] **Step 2:** Reconcile quantization errors from `docs/reports/AI_MODEL_AUDIT_2026-10-20.md`: MAE = 0.00790, Max Error = 0.09136, Tier Agreement = 97.32%.
- [ ] **Step 3:** Clarify that `expf()` is an optimized C runtime software routine running alongside single-precision hardware FPU multiplications.

### Task 3: Resolve All Reviewer Doubts with Verified Facts in Sections 11, 12, 15, and 16
**Files:**
- Modify: `VALOR_COMPLETE_GUIDE.md` (Sections 11, 12, 15, 16, 17, 19)
- Verify: Python verification script

- [ ] **Step 1:** In Section 0, 1, and 12, state unequivocally that `CONFIG_FREERTOS_HZ = 200` (5.0 ms tick quantum) in `sdkconfig.defaults`, removing any doubt about 5 ms jitter.
- [ ] **Step 2:** In Section 12, confirm Wi-Fi task is pinned to Core 1 (`CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1=y`).
- [ ] **Step 3:** In Section 11, explain both Vivado Zynq timing runs (+5.603 ns out-of-context baseline vs +12.836 ns / +13.168 ns full block design runs).
- [ ] **Step 4:** In Section 15, confirm TMP117 bench measurement date is 21 October 2026.
- [ ] **Step 5:** In Section 16, confirm GNSS driver passes 12/12 unit tests and explain indoor NVS flash fallback.

### Task 4: Polish Section 23 ("Known Architectural Nuances Disclosed to the Jury")
**Files:**
- Modify: `VALOR_COMPLETE_GUIDE.md` (Section 23)
- Verify: Python verification script

- [ ] **Step 1:** Replace the "Items still unconfirmed" list with a proactive, confident "Architectural Disclosures and Edge Realities" section that arms the student with answers for every known design nuance.
- [ ] **Step 2:** Run full automated guide verification to ensure all 24 sections pass, all invariants hold, and zero `[VERIFY]` tags remain.
