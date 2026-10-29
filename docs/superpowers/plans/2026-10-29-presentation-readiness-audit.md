# Presentation Readiness A-to-Z Audit Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Before tomorrow's (2026-10-30) SIH26181 presentation, prove that every test passes, every number on slides and in docs matches the evidence, the live hardware demo and the fallback both work, and every weak point a judge could attack has an honest, rehearsed answer.

**Architecture:** This is an audit, not a feature. Every task runs checks, writes one evidence artifact, and fixes only **documentation, slide text, and runbook** problems. Firmware and RTL stay **frozen**: a code change is allowed only for a P0 bug (the demo cannot run), and any such change forces Task 7 to be re-run from the start. All findings collect in one register that the presenter reads tonight.

**Tech Stack:** PowerShell, MSYS2 GCC (`C:\msys64\ucrt64\bin\gcc.exe`), Icarus Verilog (`C:\iverilog\bin`), Verilator (MSYS2), Python `.venv` (python-pptx, pyserial), ESP-IDF v5.5.5 (`C:\Espressif\frameworks\esp-idf-v5.5.5`), headless Chrome (dashboard verifiers). No `make` and no `gh` on PATH, so CI is replayed with the exact `gcc`/`iverilog` lines from `.github/workflows/ci.yml`.

**Spec:** User request (2026-10-29): "tomorrow is my presentation, analyse everything from A to Z that is perfect and that can bite me, and also check and verify." Source-of-truth documents: `AGENTS.md`, `README.md` Â§"Reproducible claims" + Â§"Provenance caveats" (lines 31-53), `docs/DEMO_RUNBOOK.md`, `docs/JUDGE_QA.md`, `.github/workflows/ci.yml`.

## Global Constraints

- Firmware/RTL freeze: do not edit `firmware/**/*.c|h` or `hardware/**/*.v` unless a P0 is logged in the register with its reproduction steps.
- Never force-push or rewrite history. Add only new commits and one annotated tag.
- Leave untracked user files alone: `Assignment2_Solutions.pdf`, `generate_assignment_pdf.py`, `hardware/enclosure/stl/PRINT_BOTH_ON_ONE_BED_v2.3mf`.
- Canonical numbers (quote exactly; every slide and doc must agree with these):
  - Zynq `xc7z020clg400-1`, Vivado ML 2022.2 OOC: WNS +5.603 ns, WHS +0.184 ns, Fmax 69.45 MHz @ 50 MHz target; 185 LUT, 16 LUTRAM, 266 FF, 0 DSP48, 0 BRAM. Evidence is **PNG only** (README caveat 5).
  - ForgeFPGA SLG47910C: 515 / 1120 LUT5 (45.98%), 303 FF.
  - Testbenches: Zynq `6 passed, 0 failed`; common `ALL 4 TESTS PASSED`; ShrikeFi `13 passed, 0 failed`.
  - TinyML: 6-24-16-3, 619 INT8 params, 576 MACs; 88.47% FP32 / 88.39% INT8 **agreement with the rule-engine teacher** (not clinical accuracy).
  - MIMIC-III demo subset: 16,387 records, 98 subjects; accuracy 94.11%, sensitivity 58.18%, specificity 98.19%, TP 971 / FP 267 / TN 14451 / FN 698. RMSSD column is synthetic; labels derived in-tree.
  - Inference latency 0.44 Âµs is **x86-64 host only**; ESP32-S3 latency not measured.
  - Firmware on device: `v1.0-zynq-SIH-316-g8b4cab5`. SoftAP `VALOR-0081` at `http://192.168.4.1`.
- Forbidden phrasings anywhere in slides or docs: "clinically validated", "medical-grade", "FDA", "88.39% accuracy" without "teacher"/"agreement" in the same sentence, "0.44 Âµs" next to "ESP32", any claim of HRV/RMSSD accuracy (ECG comparison T4.1 still open, `docs/HARDWARE_DEBUG_HANDOVER.md:146`).
- All I/O is 3.3 V max; demo wiring must never put 5 V on a signal pin.
- Dashboards must work with Wi-Fi off (no CDN, no external fetch).

## Review Focus

1. **A judge asks "what's your false-negative rate?"** The 94.11% headline hides 58.18% sensitivity (698 missed crises). Expect a straight number and a reason, not a dodge. â†’ Task 4 risk R1 needs a written 20-second answer.
2. **Venue lighting or a cold finger means no contact.** HRV never settles inside the 20 s / 30-beat window and the demo shows `settling` the whole time. Expect a tested recovery: cup the sensor, warm the finger, or switch to the fallback. â†’ Task 7 Step 4 times settle under room light and Task 8 rehearses the switch.
3. **Cold boot away from the laptop** (power bank, different cable). The device must boot and serve SoftAP without a serial monitor attached. â†’ Task 7 Step 2.
4. **Phone joins `VALOR-0081` and then drops it** because there is no internet (mobile-data fallback). The page should still load once mobile data is off. â†’ Task 7 Step 6.
5. **Slide numbers disagree with README/docs** (stale LUT counts, old accuracy, a missing guide path). Every number should match the canonical list. â†’ Task 3 checker scans docs **and** extracted slide text.

---

### Task 1: Baseline snapshot and one-command verifier

**Files:**
- Create: `scripts/predemo_verify.ps1`
- Create: `docs/presentation/PREDEMO_VERIFY_LOG.txt` (generated output, committed as evidence)

**Interfaces:**
- Produces: `scripts/predemo_verify.ps1` exits 0 only when every gate passes and prints one `PASS|FAIL <gate-name>` line per gate plus `SUMMARY: N/N PASS`. Tasks 9 and 10 re-run it.

- [ ] **Step 1: Record baseline.** Run `git status --short; git describe --tags; git log -1 --format=%H`. Expected: only the 3 untracked user files above; describe = `v1.0-zynq-SIH-316-g8b4cab5`. Any other modified file is a finding (log it in Task 4's register).
- [ ] **Step 2: Write `scripts/predemo_verify.ps1`** with one gate per CI step, each a command copied verbatim from `.github/workflows/ci.yml` (adjusted only for `.exe` suffix and Windows paths), checking output for the pinned string:
  - `verilator-zynq`, `verilator-shrikefi` (ci.yml lint job): exit 0.
  - `flat-source`: `python hardware/shrikefi/gen_flat_source.py --check` exit 0.
  - `tb-zynq`: output contains `6 passed, 0 failed`.
  - `tb-common`: contains `ALL 4 TESTS PASSED`.
  - `tb-shrikefi`: contains `13 passed, 0 failed`.
  - `fp32-hygiene`: the `-Werror -Wdouble-promotion -fsyntax-only` line exits 0.
  - `unit-engine`: `test_engine` contains `ALL TESTS PASSED`.
  - `unit-fall`, `unit-gnss`, plus every remaining gcc build/run in the simulate job (`shrikefi_host`, `compare_harness`, `test_pm25_correction`): exit 0.
  - `mimic-gate`: `acc_eval data/mimic/mimic_eval_feed.csv` `METRICS_JSON` has `total==16387`, `accuracy>=93.0`, `agreement_pct>=99.0`.
  - `nn-train`: `python firmware/core/train_nn_risk_model.py --min-accuracy 88.0` exit 0, log contains `"arch":"6-24-16-3"` and `"params":619`; then `git diff --quiet -- firmware/core/nn_risk_model_int8.c.inc firmware/core/nn_risk_model_trained.c.inc` (if it differs, **restore with `git checkout --`** and report WARN, not FAIL).
  - `dash-softap`: `python firmware/shrikefi/dashboard/verify_softap_page.py` all PASS.
  - `dash-desktop`: `python firmware/shrikefi/dashboard/verify_dashboard.py` all OK; afterwards delete `firmware/shrikefi/dashboard/tmp*.html`.
  - `secrets`: `git ls-files firmware/shrikefi/wifi_credentials.h` is empty and `git grep -nE 'SHRIKEFI_WIFI_PASS[[:space:]]+"[^"]+"' -- . ':!*.example' ':!*.md'` finds nothing.
  - `fw-build`: in an ESP-IDF shell, `idf.py -C firmware/shrikefi build` exits 0 (no flash).
  Build outputs go to a temp directory under `$env:TEMP\predemo`, not into the repo.
- [ ] **Step 3: Run it.** `powershell -ExecutionPolicy Bypass -File scripts/predemo_verify.ps1 | Tee-Object docs/presentation/PREDEMO_VERIFY_LOG.txt`. Expected: `SUMMARY: 19/19 PASS` (WARN allowed only for nn weight drift). Any FAIL â†’ stop and log as P0/P1 in Task 4's register before going further.
- [ ] **Step 4: Confirm the tree is clean.** `git status --short` shows only the new script, the log, and the 3 user files.
- [ ] **Step 5: Commit.** `git add scripts/predemo_verify.ps1 docs/presentation/PREDEMO_VERIFY_LOG.txt; git commit -m "test(predemo): add one-command local CI replay and baseline log"`

### Task 2: Offline, CSP and safety invariants

**Files:**
- Test only: `firmware/shrikefi/dashboard/valor_dashboard.html`, `docs/archive/valor_dashboard_2.html`, `firmware/shrikefi/web_status.c`, `firmware/shrikefi/main_shrikefi.c`, `hardware/common/*.v`
- Append findings to: `docs/presentation/PRESENTATION_RISK_REGISTER.md` (created in Task 4; create the file with only the header here if it does not exist yet)

**Interfaces:**
- Consumes: Task 1 `dash-*` gates (CSP digests already verified there).

- [ ] **Step 1: No external fetch.** `Select-String -Path firmware/shrikefi/dashboard/valor_dashboard.html,docs/archive/valor_dashboard_2.html -Pattern 'https?://' | Where-Object { $_.Line -notmatch '192\.168\.4\.1|www\.w3\.org/2000/svg' }` returns nothing. Same check on the `PAGE_HTML` string in `web_status.c`. Any hit is a P1 (dashboard breaks offline).
- [ ] **Step 2: Byte-identical dashboards.** `(Get-FileHash firmware/shrikefi/dashboard/valor_dashboard.html).Hash -eq (Get-FileHash docs/archive/valor_dashboard_2.html).Hash` â†’ `True`.
- [ ] **Step 3: Firmware safety greps (no fixes, register only).** In `firmware/`, excluding `managed_components` and `build`: `\bstrcpy\(|\bsprintf\(|\bstrcat\(|\bgets\(` â†’ expect 0 hits; `\bdouble\b|\bsqrt\(|\bfabs\(|\bpow\(` in `firmware/core` and `firmware/shrikefi` non-test sources â†’ each hit noted with file:line and whether it runs on the target.
- [ ] **Step 4: RTL guardrails.** Run `verilator --lint-only -Wall` on `hardware/common/*.v` alone (no `-Wno-*`), plus `Select-String hardware/common/*.v -Pattern 'AXI|axi_|\(\*\s*(keep|mark_debug)|BUFG|IBUF|MMCM|PLLE'` â†’ expect 0 hits (vendor-agnostic rule). Log every warning with a one-line "harmless because â€¦" or "risk".
- [ ] **Step 5: Commit** (only if the register changed). `git add docs/presentation/PRESENTATION_RISK_REGISTER.md; git commit -m "docs(predemo): record offline/CSP/safety invariant results"`

### Task 3: Number and claim consistency checker (docs + slides)

**Files:**
- Create: `scripts/check_presentation_claims.py`
- Create: `docs/presentation/FACT_SHEET.md`
- Modify: whichever `docs/*.md` / `README.md` lines the checker flags (text only)

**Interfaces:**
- Produces: `python scripts/check_presentation_claims.py [--pptx PATH ...]` prints `file:line: RULE-ID: excerpt` per violation and exits 1 if any are found. Task 6 calls it with `--pptx`.

- [ ] **Step 1: Write the checker** with these rules (scan `README.md`, `docs/*.md`, `docs/presentation/*.md`; skip `docs/archive/` and `docs/superpowers/`; with `--pptx` also scan text from every shape, table cell and speaker note via python-pptx, reporting `deck.pptx:slideN`):
  - `STALE-PATH`: `docs/PROJECT_GUIDE.md` (file does not exist; the correct file is `docs/MASTER_PROJECT_GUIDE.md`). Also any `docs/...md` link whose target does not exist.
  - `TEACHER`: `88\.(39|47)\s*%` in a sentence without `teacher|agreement|distil`.
  - `HOST-LATENCY`: `0\.44\s*[Âµu]s` in a sentence containing `ESP32` without `host|x86`.
  - `OVERCLAIM`: `clinically validated|medical[- ]grade|FDA|diagnos(e|tic) (device|grade)` (case-insensitive).
  - `HRV-ACCURACY`: `(HRV|RMSSD)[^.]{0,40}(accura|validated|precise)` unless the same sentence contains `not|open|pending`.
  - `NUM-MISMATCH`: a number near a known keyword that differs from the Global Constraints value: `LUT5?s?` next to `/\s*1120` must be 515; Zynq `LUT` must be 185 when it appears with `Zynq|xc7z020`; `Fmax` must be 69.45; `WNS` must be +5.603; `params|parameters` with `INT8` must be 619; MIMIC `records` must be 16,387 / 16387; `sensitivity` 58.18; `specificity` 98.19; triage `accuracy` 94.11.
  - `SENSITIVITY-HIDDEN`: any line quoting `94.11` where sensitivity `58.18` does not appear within Â±5 lines (warning-level; print but exit 0 for this rule alone).
- [ ] **Step 2: Run it on the docs.** `python scripts/check_presentation_claims.py`. Expected first run: FAIL with at least `docs/DEMO_RUNBOOK.md: STALE-PATH: docs/PROJECT_GUIDE.md` (already confirmed missing). Save the full output into the register.
- [ ] **Step 3: Fix every doc violation** by editing text only (correct number, add "teacher agreement", add "host-measured", point the path at `docs/MASTER_PROJECT_GUIDE.md`). Never change a canonical number to match a doc; the doc changes to match.
- [ ] **Step 4: Re-run.** Expected: exit 0 (SENSITIVITY-HIDDEN warnings allowed only where Step 5's fact sheet covers them).
- [ ] **Step 5: Write `docs/presentation/FACT_SHEET.md`**: one table, columns `Claim | Exact value | Evidence file | Command to reproduce | Caveat to say out loud`, one row per canonical number in Global Constraints. This is the presenter's printable cheat sheet.
- [ ] **Step 6: Commit.** `git add scripts/check_presentation_claims.py docs/presentation/FACT_SHEET.md README.md docs/*.md; git commit -m "docs(predemo): add claim checker, fact sheet, and fix stale numbers/paths"`

### Task 4: "What can bite me" risk register and judge-answer cross-check

**Files:**
- Create/extend: `docs/presentation/PRESENTATION_RISK_REGISTER.md`
- Modify: `docs/JUDGE_QA.md` (only answers that contradict code or the fact sheet)

**Interfaces:**
- Consumes: Task 1 log, Task 2 findings, Task 3 checker output.
- Produces: register rows `ID | Severity (P0 demo-breaking / P1 wrong claim / P2 weak spot) | Risk | Evidence (file:line) | Status | 20-second honest answer`.

- [ ] **Step 1: Seed these known risks**, each with evidence path and a drafted answer:
  - R1 Sensitivity 58.18% / 698 FN on MIMIC: README:39 and :193.
  - R2 MIMIC RMSSD synthetic, labels in-tree, demo subset only (98 subjects): README:48-50.
  - R3 88.39% is teacher agreement, not clinical: README:51.
  - R4 Zynq 185 LUT / +5.603 ns evidence is PNG only (`.runs/` overwritten): README:52, `hardware/zynq/synthesis_evidence/README.md`.
  - R5 ESP32-S3 inference latency not measured: README:53.
  - R6 HRV/RMSSD accuracy not validated against ECG (T4.1 open): `docs/HARDWARE_DEBUG_HANDOVER.md:146,303,319`.
  - R7 MAX30102 reflective SpOâ‚‚ is not a calibrated clinical oximeter. Skin temperature is peripheral, not core (dashboard shows core temp sub-score `n/a`).
  - R8 Low-cost PM2.5 sensor: point to the correction model and its test `firmware/core/test_pm25_correction.c`, plus `docs/JUDGE_QA.md` Q7.5.
  - R9 ForgeFPGA bitstream: NVRAM vs runtime load (JUDGE_QA Q4.2). Confirm the answer matches `hardware/shrikefi/` docs and `convert_bitstream.py`.
  - R10 The Zynq part ships with the project, but the live demo runs on ShrikeFi. Have a one-line explanation of why two FPGA targets exist.
- [ ] **Step 2: Cross-check every `docs/JUDGE_QA.md` answer** against code and the fact sheet. For each Q, open the cited source file and confirm the claim (numbers, pin map GPIO10/11/12/13 SPI mode 0, mutex `s_data_mutex`, HRV 30-beat / â‰¥15 NN-pair settle in `hrv_is_settled()`, NEWS2 bands 0 / 1-3 / 4-6 / â‰¥7). Each contradiction becomes a P1 row and a text fix in JUDGE_QA.md.
- [ ] **Step 3: Run the claim checker on JUDGE_QA.md specifically**: `python scripts/check_presentation_claims.py` exits 0.
- [ ] **Step 4: Commit.** `git add docs/presentation/PRESENTATION_RISK_REGISTER.md docs/JUDGE_QA.md; git commit -m "docs(predemo): add risk register and reconcile judge Q&A with code"`

### Task 5: Runbook correctness

**Files:**
- Modify: `docs/DEMO_RUNBOOK.md`

**Interfaces:**
- Consumes: Task 3 checker (STALE-PATH), Task 1 toolchain facts.

- [ ] **Step 1: Execute every command in runbook Â§3 Stage 1-2 verbatim**, without the flash and monitor steps (those run in Task 7). Each command must run as written on this laptop. Fix any wrong path or version; the IDF path must be `esp-idf-v5.5.5`.
- [ ] **Step 2: Fix the Â§7 checklist.** Tab 3 â†’ `docs/MASTER_PROJECT_GUIDE.md`. Add Tab 4 â†’ `docs/presentation/FACT_SHEET.md`. Add a line "Run `scripts/predemo_verify.ps1` â†’ `SUMMARY: 19/19 PASS`". The "Â§6 Permission denied: build_host_tests.exe" row must name an executable that actually exists in the current flow (`test_engine.exe`), or be removed.
- [ ] **Step 3: Check that the Â§4 Act timings add up to the slot length** and that every Act names a feature the current firmware really shows: compare Act text against `[TELEMETRY]` fields in `main_shrikefi.c` and the SoftAP JSON keys in `web_status_json()`. Fix wording that promises something the UI doesn't show.
- [ ] **Step 4: Verify.** `python scripts/check_presentation_claims.py` exits 0.
- [ ] **Step 5: Commit.** `git add docs/DEMO_RUNBOOK.md; git commit -m "docs(runbook): fix guide path, IDF version, and checklist for demo day"`

### Task 6: Slide deck audit

**Files:**
- Read: `submission/VALOR_Hackathon_Submission.pptx`, `docs/presentation/VALOR_Round1.pptx`, `docs/presentation/VALOR Explainer Deck.pptx`
- Create: `docs/presentation/SLIDE_FIXES.md`
- Fix: `scripts/inspect_presentation.py` hard-codes the missing `docs/presentation/FINAL_FINAL_v6.pptx`; make it take the deck path as `sys.argv[1]`.

**Interfaces:**
- Consumes: `scripts/check_presentation_claims.py --pptx PATH`.

- [ ] **Step 1: Ask the human which deck they are presenting.** Do not guess. The submission deck and `.codex-deck-work/build/candidate.pptx` share a timestamp. Audit that deck first, then the others.
- [ ] **Step 2: Run** `python scripts/check_presentation_claims.py --pptx "<deck>"`. Record every violation by slide number.
- [ ] **Step 3: Manual pass per slide**: architecture diagram matches the real data path (MAX30102 â†’ ForgeFPGA peak/IBI â†’ SPI â†’ ESP32-S3 â†’ engines â†’ OLED/SoftAP/serial); pin numbers; no 5 V on any signal; part numbers (`SLG47910C`, `xc7z020clg400-1`, ESP32-S3); team/problem statement ID SIH26181; no broken images (python-pptx: every picture shape has an image blob).
- [ ] **Step 4: Write `docs/presentation/SLIDE_FIXES.md`**: `Slide # | Current text | Corrected text | Reason (rule ID or evidence path)`. **Do not edit the .pptx automatically.** The human applies fixes in PowerPoint so layout stays intact.
- [ ] **Step 5: Commit.** `git add docs/presentation/SLIDE_FIXES.md scripts/inspect_presentation.py; git commit -m "docs(predemo): slide-by-slide correction list"`

### Task 7: Live hardware rehearsal on COM4 (human-in-the-loop)

**Files:**
- Create: `docs/presentation/HW_REHEARSAL_2026-10-29.md`
- Create: `docs/presentation/serial_capture_2026-10-29.log`

**Interfaces:**
- Consumes: Task 1 `fw-build` (same commit that is flashed).
- Every step below needs the human at the bench. The agent prompts, captures, and records PASS/FAIL with the timestamp and the log line.

- [ ] **Step 1: Firmware identity.** Capture 60 s of serial from COM4 at the configured baud (pyserial from `.venv`, or `idf.py -p COM4 monitor`; close it afterwards). The boot banner contains `v1.0-zynq-SIH-316-g8b4cab5`, or the hash of whatever commit Task 10 will tag. If it differs, re-flash with `idf.py -p COM4 flash` and repeat.
- [ ] **Step 2: Cold boot without the laptop (Review Focus 3).** Power from a power bank or wall adapter, no serial attached, three power cycles. Each time the OLED shows the boot screen and `VALOR-0081` shows up on the phone within 15 s.
- [ ] **Step 3: FPGA link health.** In the capture, the SPI link frames are valid. No link-error/CRC/timeout lines over 60 s with a finger on the sensor. Confirm with `python hardware/shrikefi/tools/analyse_capture.py docs/presentation/serial_capture_2026-10-29.log` (or `replay_fpga_link_log.py`) and record its summary.
- [ ] **Step 4: Finger-on settle under room light (Review Focus 2).** Time from finger-on to HR/SpOâ‚‚ display and to HRV leaving `settling` (expect about 20 s / 30 beats, shown as `n/30` on SoftAP). Run 3 trials: plain room light, sensor cupped, and a cold finger. Record the times. Any trial over 60 s â†’ P0 row with the mitigation script.
- [ ] **Step 5: Finger-off.** Within about 3 s the dashboard and SoftAP show no-contact and clear vitals, skin temp, NEWS2 sub-scores and `hrvN`. No stale values stay on screen.
- [ ] **Step 6: SoftAP on phone (Review Focus 4).** Mobile data OFF, join `VALOR-0081`, open `http://192.168.4.1`: values refresh and the grid lays out cleanly. Then turn mobile data ON and record whether the page still loads. Write the exact phone steps into the runbook Â§6 row.
- [ ] **Step 7: Fall â†’ SOS â†’ cancel, twice in a row** without reloading the page. The second 3-second hold CANCEL must also work (regression check for `5df4c11`).
- [ ] **Step 8: Web Serial desktop dashboard.** Open `firmware/shrikefi/dashboard/valor_dashboard.html` with Wi-Fi off, connect COM4, then confirm: header switches from SYNTHETIC to WAITING to live, NEWS2 band label, and Storm badge render. Unplug USB for 10 s and plug back in: the 24h history stays and the page reconnects.
- [ ] **Step 9: 15-minute soak.** Finger on and off at intervals. Expect no watchdog reset, no `Guru Meditation`, and no boot banner reappearing in the log. If the firmware prints free heap, it must not decline steadily.
- [ ] **Step 10: Record and commit.** Write a PASS/FAIL table with measured times into `HW_REHEARSAL_2026-10-29.md`. Then `git add docs/presentation/HW_REHEARSAL_2026-10-29.md docs/presentation/serial_capture_2026-10-29.log; git commit -m "test(predemo): live hardware rehearsal results"`

### Task 8: Fallback demo rehearsal (no hardware, no internet)

**Files:**
- Modify: `docs/DEMO_RUNBOOK.md` Â§5 "Executing the Verified Fallback Demonstration" (only if a step fails as written)

- [ ] **Step 1: Disable laptop Wi-Fi.** Follow runbook Â§5 fallback steps exactly as written: dashboard simulation/profile mode, host unit tests, RTL testbenches. Each step produces the output the runbook promises.
- [ ] **Step 2: Time the switch.** Measure live-demo-fails â†’ fallback-running on screen. Target under 30 s. Write the trigger phrase and the exact keystrokes into runbook Â§5.
- [ ] **Step 3: Open every evidence file named in the runbook tabs** (`hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log`, Zynq PNGs in `hardware/zynq/synthesis_evidence/`). Each must open offline and show the canonical numbers.
- [ ] **Step 4: Commit** (only if the runbook changed). `git add docs/DEMO_RUNBOOK.md; git commit -m "docs(runbook): rehearsed fallback timings"`

### Task 9: Whole-branch review and go/no-go

**Files:**
- Modify: `docs/presentation/PRESENTATION_RISK_REGISTER.md` (final status column + top-of-file GO/NO-GO line)

- [ ] **Step 1: Re-run** `scripts/predemo_verify.ps1` â†’ `SUMMARY: 19/19 PASS`, and `python scripts/check_presentation_claims.py` â†’ exit 0.
- [ ] **Step 2: Fresh reviewer pass** over every commit since `8b4cab5`. Confirm: no `.c/.h/.v` changes unless tied to a P0 row; every register row has a status and an answer; FACT_SHEET numbers equal Global Constraints.
- [ ] **Step 3: Write the GO/NO-GO line.** GO only if: zero open P0, every P1 fixed or listed in SLIDE_FIXES.md, Task 7 Steps 1-7 PASS, Task 8 PASS.
- [ ] **Step 4: Commit.** `git commit -am "docs(predemo): go/no-go decision"`

### Task 10: Freeze, push, tag

- [ ] **Step 1: Push.** `git push origin main` (no `--force`).
- [ ] **Step 2: Tag the exact demo commit.** `git tag -a presentation-2026-10-30 -m "SIH26181 presentation freeze"`, then `git push origin presentation-2026-10-30`.
- [ ] **Step 3: Device matches the tag.** If any firmware changed after `8b4cab5`, re-flash from the tagged commit and repeat Task 7 Steps 1, 4, 6 and 7. Otherwise confirm the Task 7 Step 1 banner hash equals `git rev-parse --short presentation-2026-10-30` or one of its doc-only ancestors.
- [ ] **Step 4: Offline copy.** Copy the presenting deck, FACT_SHEET.md, PRESENTATION_RISK_REGISTER.md, SLIDE_FIXES.md and the dashboard HTML to a USB stick or local folder that works without OneDrive sync.
