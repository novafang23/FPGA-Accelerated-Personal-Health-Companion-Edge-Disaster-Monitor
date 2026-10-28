# Repository Architecture Standardization & Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform the repository from a fast-iterated prototype workspace into a clean, professional, production-grade repository by safely auditing and removing junk, relocating loose assets into canonical directories, upgrading `.gitignore`, adding `.editorconfig` and `.prettierrc`, and preparing clean git commits for core files without disrupting active hardware/firmware baselines.

**Architecture:** Systematic multi-layer cleanup separating pure junk (local binary artifacts and stale duplicate files) from valuable assets (bench photos, 3D CAD models, audit reports, and presentation decks). All moves follow the repo's established directory taxonomy (`hardware/enclosure/`, `docs/images/hardware/`, `docs/reports/`, `docs/presentation/`). Standardized developer configuration files (`.editorconfig`, `.prettierrc`) and an expanded multi-tool `.gitignore` lock down the repository against future clutter.

**Tech Stack:** Verilog-2001 (Xilinx Vivado & Renesas ForgeFPGA), C11 / ESP-IDF (FreeRTOS Xtensa LX7), OpenSCAD / Slicer (3D CAD), Python 3.10+, PowerShell / Bash, Git.

**Spec:** User architecture cleanup request (2026-10-28) & [`AGENTS.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/AGENTS.md).

## Global Constraints

- **Strict User Permission Gate:** Present cleanup plan and obtain explicit user consent before executing any destructive deletions (`rm`, `git rm`, or `Remove-Item`).
- **No History Rewrites:** Never force-push (`--force`) or rewrite git history.
- **Hardware & Voltage Safety:** All documentation and pin definitions strictly observe 3.3 V maximum on ESP32-S3 and ForgeFPGA GPIOs.
- **Forbidden Topics:** Zero mentions of battery life, battery capacity (mAh or hours), power budgets, or bill-of-materials (BOM) cost.
- **Preserve Verified Baselines:** Do not alter Xilinx Zynq-7000 baseline (`xc7z020clg400-1`, 185 LUTs, 0 DSP, 0 BRAM) or ShrikeFi SPI link contracts.
- **Link Formatting:** All file links in markdown documents must use `file:///` URI scheme with forward slashes.

## Review Focus

1. **Destructive deletions vs relocations:** Verify that files selected for deletion (`test.exe`, `valor_dashboard_2.html`) are truly obsolete or build artifacts, and never delete user-created 3D models or bench photos.
2. **Dashboard integrity:** Ensure that deleting `valor_dashboard_2.html` in root does not impact the canonical dashboard at `firmware/shrikefi/dashboard/valor_dashboard.html` or the launcher `launch_web_dashboard.bat`.
3. **.gitignore exclusions vs whitelist:** Verify that `.gitignore` patterns do not accidentally ignore tracked evidence files in `hardware/shrikefi/synthesis_evidence/` or `hardware/zynq/synthesis_evidence/`.
4. **Git working tree cleanliness:** Ensure that staging `git add` does not pull in unreviewed binary caches or scratch files.
5. **Cross-platform line endings:** Ensure `.editorconfig` preserves CRLF for Windows `.bat` files and LF for C/Verilog/Python source code.

---

### Task 1: Audit & Delete Obsolete Junk (Requires Explicit Permission)

**Files:**
- Delete: `test.exe` (135 KB local test binary)
- Delete: `valor_dashboard_2.html` (120 KB stale dashboard copy)
- Git rm: `scratch/patch_tmp117_warning.py`
- Git rm: `scratch/tmp117_i2c_scanner.ino`
- Git rm: `scratch/tmp117_i2c_scanner/tmp117_i2c_scanner.ino`

- [ ] **Step 1: Verify `test.exe` provenance**
  Confirm `test.exe` is a transient local GCC binary created during previous test compilation and is covered by `.gitignore`.
- [ ] **Step 2: Verify `valor_dashboard_2.html` is an obsolete duplicate**
  Confirm `valor_dashboard_2.html` in root has stale 240-beat gates and missing history pollers compared to the canonical `firmware/shrikefi/dashboard/valor_dashboard.html`.
- [ ] **Step 3: Delete obsolete root files upon user confirmation**
  Execute deletion of `test.exe` and `valor_dashboard_2.html`.
- [ ] **Step 4: Clean git status of already deleted scratch files**
  Execute `git rm scratch/patch_tmp117_warning.py scratch/tmp117_i2c_scanner.ino scratch/tmp117_i2c_scanner/tmp117_i2c_scanner.ino`.
- [ ] **Step 5: Verify working tree status**
  Run `git status -s` to verify junk has been eliminated.

---

### Task 2: Relocate Loose Assets & Reports to Canonical Directories

**Files:**
- Move: `WhatsApp Image *.jpeg` (6 files) -> `docs/images/hardware/bench_wiring_2026-10-28/`
- Move: `PRINT_BOTH_ON_ONE_BED_v2.stl` -> `hardware/enclosure/stl/`
- Move: `valor_max30102_onboard_shroud.stl` -> `hardware/enclosure/stl/`
- Move: `valor_max30102_onboard_shroud.3mf` -> `hardware/enclosure/stl/`
- Move: `max30102_reflective_cradle_v2.scad` -> `hardware/enclosure/`
- Move: `valor_max30102_onboard_shroud.scad` -> `hardware/enclosure/`
- Move: `VERIFICATION_REPORT_2.md` -> `docs/reports/`
- Move: `VALOR Explainer Deck.pptx` -> `docs/presentation/`
- Move: `VALOR_Round1.pptx` -> `docs/presentation/`
- Move: `valor_rigorous_skin_temperature_calibration_fusion_master_spec.md` -> `docs/reference/`
- Move: `VALOR_COMPLETE_GUIDE_2.pdf` -> `docs/`

- [ ] **Step 1: Create target subdirectories if not present**
  Ensure `docs/images/hardware/bench_wiring_2026-10-28/`, `hardware/enclosure/stl/`, `docs/reports/`, and `docs/presentation/` exist.
- [ ] **Step 2: Relocate bench wiring photos**
  Move the 6 WhatsApp bench wiring photos to `docs/images/hardware/bench_wiring_2026-10-28/`.
- [ ] **Step 3: Relocate 3D CAD files**
  Move OpenSCAD models (`.scad`) to `hardware/enclosure/` and printable meshes (`.stl`, `.3mf`) to `hardware/enclosure/stl/`.
- [ ] **Step 4: Relocate audit reports and technical specifications**
  Move `VERIFICATION_REPORT_2.md` to `docs/reports/VERIFICATION_REPORT_2.md` and temperature calibration spec to `docs/reference/`.
- [ ] **Step 5: Relocate presentations and PDFs**
  Move presentation decks to `docs/presentation/` and guide PDF to `docs/`.
- [ ] **Step 6: Update internal document references**
  Ensure references or scripts pointing to these assets remain consistent.

---

### Task 3: Overhaul `.gitignore` for Comprehensive Multi-Tool Coverage

**Files:**
- Modify: `.gitignore`

- [ ] **Step 1: Review current `.gitignore` gaps**
  Identify missing rules: Node/NPM cache rules (`node_modules/`, `package-lock.json`), Office temporary lockfiles (`~$*.docx`, `~$*.pptx`), generic build artifacts (`*.tmp`, `*.swp`), and OpenSCAD slicer outputs (`*.gcode`).
- [ ] **Step 2: Update `.gitignore` with organized thematic sections**
  Add explicit sections for:
  - Language toolchains (C/C++, ESP-IDF, Python, Node/Web)
  - EDA & FPGA tools (Xilinx Vivado, Renesas ForgeFPGA, Icarus Verilog, GTKWave)
  - Mechanical CAD & 3D Printing (Slicer exports, OpenSCAD cache)
  - IDEs, Editors & OS metadata (VS Code, Clangd, Windows Thumbs/Desktop.ini, macOS .DS_Store)
  - AI agent runtimes & local scratch workspaces
  - Strict preservation of synthesis evidence whitelists (`!hardware/*/synthesis_evidence/**`)
- [ ] **Step 3: Test `.gitignore` rules against repository tree**
  Run `git status --ignored` to confirm unwanted files are properly ignored and no tracked core files are masked.

---

### Task 4: Add Professional Configuration Standards (`.editorconfig` & `.prettierrc`)

**Files:**
- Create: `.editorconfig`
- Create: `.prettierrc`

- [ ] **Step 1: Create `.editorconfig`**
  Configure root-level `.editorconfig` defining:
  - Default: `charset = utf-8`, `end_of_line = lf`, `trim_trailing_whitespace = true`, `insert_final_newline = true`
  - Verilog / SystemVerilog (`*.v`, `*.sv`): `indent_style = space`, `indent_size = 2`
  - C / C++ / Headers (`*.c`, `*.h`): `indent_style = space`, `indent_size = 4` (matching `.clang-format`)
  - Python (`*.py`): `indent_style = space`, `indent_size = 4`
  - Web & JSON (`*.html`, `*.css`, `*.js`, `*.json`): `indent_style = space`, `indent_size = 2`
  - Markdown (`*.md`): `indent_style = space`, `indent_size = 2`, `trim_trailing_whitespace = false`
  - Windows Batch (`*.bat`, `*.cmd`): `end_of_line = crlf`
- [ ] **Step 2: Create `.prettierrc`**
  Configure `.prettierrc` for formatting HTML dashboards, JSON specs, and Markdown documentation:
  `{ "printWidth": 100, "tabWidth": 2, "useTabs": false, "singleQuote": true, "trailingComma": "es5" }`.
- [ ] **Step 3: Verify existing configs**
  Verify `.clang-format`, `.flake8`, and `LICENSE` (MIT) remain valid and consistent.

---

### Task 5: Review Uncommitted Changes & Safe Push Protocol

**Files:**
- Review: `git status`
- Commit: Core changes & organized assets
- Push: Guidance for `origin/main`

- [ ] **Step 1: Segregate changes by category**
  - **Category A (Core Firmware/Hardware Updates):** HRV 30-beat settle gate, 120ms consecutive delta gate, dashboard sync (already committed locally as `fbfcc5f..e782175`).
  - **Category B (Documentation & Evidence):** Standalone hardware guide (`docs/VALOR_STANDALONE_HARDWARE_GUIDE.docx`), executed HRV plan (`docs/superpowers/plans/2026-10-28-hrv-stabilization-and-windowing.md`), relocated audit reports.
  - **Category C (Mechanical & CAD Assets):** Relocated OpenSCAD and STL files in `hardware/enclosure/`.
  - **Category D (Repo Configs):** Updated `.gitignore`, `.editorconfig`, `.prettierrc`.
- [ ] **Step 2: Stage and commit Category B, C, D in atomic conventional commits**
  - Commit 1: `chore(repo): organize root assets, 3D models, and audit reports into canonical folders`
  - Commit 2: `chore(repo): upgrade .gitignore and add .editorconfig and .prettierrc`
  - Commit 3: `docs(plans): record HRV stabilization and standalone hardware guide`
- [ ] **Step 3: Provide exact push verification commands**
  Document the exact command `git push origin main` and review the 40+ commits ahead of origin to ensure full confidence before pushing.
