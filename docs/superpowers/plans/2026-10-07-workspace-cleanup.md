# Workspace Cleanup & Hygiene Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clean out all non-essential temporary build outputs, simulation waveforms, compiler binaries, EDA tool caches, and system garbage from the repository while strictly preserving all source code, dependencies configs, and environment configurations (`.env`).

**Architecture:** Automated multi-stage hygiene and validation pipeline. A deterministic verification script (`scripts/verify_workspace_clean.py`) enforces strict whitelisting of core assets (protecting `.env`, `.venv/`, `.vscode/`, `.agents/`, source RTL, C firmware) while safely deleting intermediate compilation artifacts (`*.exe`, `*.o`), simulation waveforms (`*.vcd`, `*.vvp`), Vivado/ForgeFPGA build artifacts, and Python cache directories.

**Tech Stack:** Python 3.11+ / PowerShell, Git, Icarus Verilog, GCC / MinGW-w64.

**Spec:** User request: "Find and completely delete all non-essential, temporary, build cache, and system files from this workspace. Keep only my core source code files, dependencies configs, and required configuration files. Ensure you completely clean out any local `__pycache__/`, `.pytest_cache/`, `.DS_Store`, and untracked temporary garbage, but DO NOT delete any `.env` configurations or my primary code"

---

## Global Constraints

- **Preserve Environment Files:** Under NO circumstances delete `.env`, `.env.example`, or any `.env*` configuration file.
- **Preserve Core Source Code:** All files in `firmware/` (`*.c`, `*.h`, `CMakeLists.txt`, `sdkconfig`, `Kconfig*`), `hardware/` (`*.v`, `*.xdc`, `*.xpr`, `*.wcfg`), `scripts/`, `docs/`, `specs/`, and `data/` must remain untouched.
- **Preserve Tooling & Configs:** Do not delete `.venv/` core environment, `.vscode/`, `.clangd`, `AGENTS.md`, `README.md`, `LICENSE`, or `.agents/`.
- **Target Deletion Items:** Delete only transient build artifacts: `*.exe`, `*.o`, `*.vcd`, `*.vvp`, `firmware/shrikefi/build/`, `firmware/shrikefi/.cache/`, `hardware/shrikefi/forgefpga_project/ffpga/build/`, `hardware/shrikefi/pcb/.history/`, Vivado intermediate directories (`*.cache/`, `*.hw/`, `*.ip_user_files/`, `*.runs/`, `*.sim/`, `vivado.jou`, `vivado.log`), and all `__pycache__/`, `.pytest_cache/`, `.DS_Store`.
- **Rebuildability Guarantee:** The repository must cleanly re-synthesize / re-compile on demand using the pristine source code.

---

## Review Focus

1. **Accidental `.env` Deletion:** A broad glob or regex deletes `.env` or `.env.example`, destroying local configuration. *Pinned by test in Task 1 and Task 4.*
2. **Loss of Hardware / PCB Project Files:** Vivado project files (`*.xpr`, `*.wcfg`) or PCB layout files (`hardware/shrikefi/pcb/`) must not be deleted alongside intermediate build directories. *Pinned by test in Task 3.*
3. **Destruction of Python Virtual Environment (`.venv`):** Python bytecode caches (`__pycache__`) may be removed, but `.venv/` virtual environment binaries and site-packages must remain intact. *Pinned by test in Task 3.*
4. **Firmware Config Loss:** ESP-IDF `sdkconfig` and `wifi_credentials.h` must not be treated as temporary build artifacts. *Pinned by test in Task 3 and Task 4.*
5. **Dangling Process Locks:** Ensuring no processes hold open file locks on `firmware/shrikefi/build/` before directory deletion. *Pinned by test in Task 3.*

---

## Proposed Changes

```
┌────────────────────────────────────────────────────────────────────────┐
│ Phase 1: Verification Harness Scaffolding                              │
│   └── Task 1: Create Workspace Integrity & Cleanliness Checker         │
├────────────────────────────────────────────────────────────────────────┤
│ Phase 2: Binary & Simulation Artifact Purge                            │
│   └── Task 2: Purge Root & Subdirectory Executables & Waveforms        │
├────────────────────────────────────────────────────────────────────────┤
│ Phase 3: Build Directory & Cache Purge                                 │
│   └── Task 3: Purge ESP-IDF, Vivado, PCB History & Python Caches       │
├────────────────────────────────────────────────────────────────────────┤
│ Phase 4: Integrity & Rebuild Verification                              │
│   └── Task 4: Validate Clean State, Protected Files & Buildability     │
└────────────────────────────────────────────────────────────────────────┘
```

---

## Task Structure

### Task 1: Create Workspace Integrity & Cleanliness Checker

**Files:**
- Create: `scripts/verify_workspace_clean.py`
- Test: `scripts/verify_workspace_clean.py`

**Interfaces:**
- Consumes: Workspace file system structure and environment configuration `.env`.
- Produces: CLI script `scripts/verify_workspace_clean.py` returning exit code 0 when clean and protected files are intact, or non-zero when garbage is present or protected files are missing.

- [ ] **Step 1: Write the failing test / verification script**

Create `scripts/verify_workspace_clean.py` defining:
1. `PROTECTED_FILES`: `['.env', '.env.example', 'firmware/shrikefi/sdkconfig', 'firmware/shrikefi/wifi_credentials.h', 'hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.xpr', 'shrike_fpga/shrike_fpga.xpr']`
2. `PROHIBITED_EXTENSIONS`: `['.exe', '.o', '.vcd', '.vvp']`
3. `PROHIBITED_DIRS`: `['firmware/shrikefi/build', 'firmware/shrikefi/.cache', 'hardware/shrikefi/forgefpga_project/ffpga/build', 'hardware/shrikefi/pcb/.history']`
4. `PROHIBITED_CACHE_NAMES`: `['__pycache__', '.pytest_cache', '.DS_Store']` (outside `.venv`)

- [ ] **Step 2: Run test to verify it fails (detects current untracked build artifacts)**

Run: `python scripts/verify_workspace_clean.py`
Expected: FAIL / Non-zero exit code listing existing `.exe`, `.vcd`, `.vvp`, and `build/` directories that must be cleaned.

- [ ] **Step 3: Implement `--dry-run` and `--clean` modes in `scripts/verify_workspace_clean.py`**

Add CLI flags to allow running `--check-only` (default) or `--clean` (safely removes only prohibited items while verifying protected items remain untouched).

- [ ] **Step 4: Run dry-run to verify output**

Run: `python scripts/verify_workspace_clean.py --dry-run`
Expected: PASS listing exact deletion targets and confirming all `PROTECTED_FILES` are flagged as SAFE.

- [ ] **Step 5: Commit verification harness**

```bash
git add scripts/verify_workspace_clean.py
git commit -m "test: add workspace cleanliness and integrity verification script"
```

---

### Task 2: Purge Root & Subdirectory Executables & Waveforms

**Files:**
- Modify: File system (delete untracked root & nested binaries and waveform files)
- Test: `scripts/verify_workspace_clean.py`

**Interfaces:**
- Consumes: Target file list from Task 1.
- Produces: Clean workspace free of compiled binaries and simulation dumps.

- [ ] **Step 1: Write the failing test assertion for binaries and waveforms**

Target artifacts to eliminate:
- Root: `acc_eval.exe`, `compare_harness.exe`, `test_engine.exe`, `test_header.o`, `test_null.o`, `ppg_system.vcd`, `shrikefi_sim.vcd`, `sim_common.vvp`, `sim_ppg.vvp`, `sim_shrikefi.vvp`.
- Subdirectories: `firmware/core/acc_eval.exe`, `firmware/core/test_pm25.exe`, `firmware/shrikefi/shrikefi_host.exe`, `hardware/zynq/test_engine.exe`, `hardware/shrikefi/shrikefi_sim.vcd`, `hardware/shrikefi/sim_shrikefi.vvp`.

Verification command:
`powershell -Command "Get-ChildItem -Path . -Recurse -Include *.exe,*.o,*.vcd,*.vvp | Where-Object { $_.FullName -notmatch '\\\.venv\\' } | Select-Object FullName"`
Expected before step: Returns 17 binary / simulation files.

- [ ] **Step 2: Run test to verify files exist**

Run: `powershell -Command "Get-ChildItem -Path . -Recurse -Include *.exe,*.o,*.vcd,*.vvp | Where-Object { $_.FullName -notmatch '\\\.venv\\' } | Measure-Object | Select-Object -ExpandProperty Count"`
Expected: Count > 0 (17 files found).

- [ ] **Step 3: Delete all target binaries, object files, and simulation artifacts**

Remove the target files using PowerShell:
```powershell
Get-ChildItem -Path . -Recurse -Include *.exe,*.o,*.vcd,*.vvp | Where-Object { $_.FullName -notmatch '\\\.venv\\' } | Remove-Item -Force
```

- [ ] **Step 4: Run test to verify all target binaries and waveforms are deleted**

Run: `powershell -Command "Get-ChildItem -Path . -Recurse -Include *.exe,*.o,*.vcd,*.vvp | Where-Object { $_.FullName -notmatch '\\\.venv\\' } | Measure-Object | Select-Object -ExpandProperty Count"`
Expected: `0`

- [ ] **Step 5: Verify git status is unaffected for tracked files**

Run: `git status -s`
Expected: No tracked source files modified or deleted.

---

### Task 3: Purge Build Directories, EDA Tool Caches & Python Caches

**Files:**
- Modify: File system (delete build output trees and EDA intermediate cache directories)
- Test: `scripts/verify_workspace_clean.py`

**Interfaces:**
- Consumes: File system directory trees.
- Produces: Reclaimed disk space (> 250 MB) and pristine project root.

- [ ] **Step 1: Write verification check for target directories before deletion**

Target directories and files to eliminate:
- `firmware/shrikefi/build/`
- `firmware/shrikefi/.cache/`
- `hardware/shrikefi/forgefpga_project/ffpga/build/`
- `hardware/shrikefi/pcb/.history/`
- `hardware/zynq/vivado_project/vivado.jou`
- `hardware/zynq/vivado_project/vivado.log`
- `hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.cache/`
- `hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.hw/`
- `hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.ip_user_files/`
- `hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.runs/`
- `hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.sim/`
- `shrike_fpga/shrike_fpga.cache/`
- `shrike_fpga/shrike_fpga.hw/`
- `shrike_fpga/shrike_fpga.ip_user_files/`
- `shrike_fpga/shrike_fpga.sim/`
- All `__pycache__/`, `.pytest_cache/`, `.DS_Store` across workspace.

- [ ] **Step 2: Run verification test to confirm directory presence and absence of file locks**

Run:
```powershell
$dirs = @('firmware/shrikefi/build', 'hardware/shrikefi/forgefpga_project/ffpga/build', 'hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.runs')
$dirs | ForEach-Object { Test-Path $_ }
```
Expected: All return `True`.

- [ ] **Step 3: Safely purge build directories and intermediate tool caches**

Execute directory removal while preserving project definitions (`.xpr`, `.wcfg`, PCB sources):
```powershell
$targetDirs = @(
    "firmware/shrikefi/build",
    "firmware/shrikefi/.cache",
    "hardware/shrikefi/forgefpga_project/ffpga/build",
    "hardware/shrikefi/pcb/.history",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.cache",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.hw",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.ip_user_files",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.runs",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.sim",
    "shrike_fpga/shrike_fpga.cache",
    "shrike_fpga/shrike_fpga.hw",
    "shrike_fpga/shrike_fpga.ip_user_files",
    "shrike_fpga/shrike_fpga.sim"
)
foreach ($dir in $targetDirs) {
    if (Test-Path $dir) { Remove-Item -Path $dir -Recurse -Force }
}
$targetFiles = @(
    "hardware/zynq/vivado_project/vivado.jou",
    "hardware/zynq/vivado_project/vivado.log"
)
foreach ($f in $targetFiles) {
    if (Test-Path $f) { Remove-Item -Path $f -Force }
}
Get-ChildItem -Path . -Recurse -Force -Include "__pycache__", ".pytest_cache", ".DS_Store" | Remove-Item -Recurse -Force
```

- [ ] **Step 4: Run test to verify build directories and caches are eliminated**

Run:
```powershell
$dirs = @(
    "firmware/shrikefi/build",
    "firmware/shrikefi/.cache",
    "hardware/shrikefi/forgefpga_project/ffpga/build",
    "hardware/shrikefi/pcb/.history",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.runs"
)
$dirs | ForEach-Object { Test-Path $_ }
```
Expected: All return `False`.

- [ ] **Step 5: Verify protected project files still exist (Pins Review Focus 2, 3, 4)**

Run:
```powershell
$protected = @(
    ".env",
    ".env.example",
    "firmware/shrikefi/sdkconfig",
    "firmware/shrikefi/wifi_credentials.h",
    "hardware/zynq/vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.xpr",
    "shrike_fpga/shrike_fpga.xpr",
    ".venv/Scripts/python.exe"
)
$protected | ForEach-Object { [PSCustomObject]@{ File = $_; Exists = (Test-Path $_) } }
```
Expected: All show `Exists: True`.

---

### Task 4: Validate Clean State, Protected Files & Buildability

**Files:**
- Test: `scripts/verify_workspace_clean.py`
- Test: `firmware/core/test_clinical_vitals.c`
- Test: `hardware/common/ppg_system.v`

**Interfaces:**
- Consumes: Clean repository state.
- Produces: Formal validation report confirming pristine cleanliness, `.env` protection, and rebuildability.

- [ ] **Step 1: Run workspace cleanliness verification script**

Run: `python scripts/verify_workspace_clean.py`
Expected: PASS (exit code 0), reporting 0 forbidden files, 0 forbidden directories, and 100% protected files present.

- [ ] **Step 2: Verify `.env` integrity (Pins Review Focus 1)**

Run:
```powershell
Get-Item .env | Select-Object FullName, Length, LastWriteTime
```
Expected: `.env` exists with valid non-zero byte size.

- [ ] **Step 3: Verify clean rebuildability of C firmware vitals test**

Run:
```powershell
gcc -O2 -Wall -Wextra -I firmware/core firmware/core/clinical_vitals_engine.c firmware/core/test_clinical_vitals.c -o test_clinical_temp.exe
.\test_clinical_temp.exe
Remove-Item test_clinical_temp.exe
```
Expected: Compiles with 0 warnings, runs successfully with `PASS: All clinical vitals tests passed`, and temporary binary is immediately removed.

- [ ] **Step 4: Verify clean rebuildability of Verilog RTL simulation**

Run:
```powershell
iverilog -o sim_test_temp.vvp hardware/common/ppg_system.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v hardware/zynq/tb_ppg_system.v
vvp sim_test_temp.vvp
Remove-Item sim_test_temp.vvp
```
Expected: Compiles cleanly, runs 6/6 tests passing (`TEST 6 PASSED: PPG System Integration test completed successfully`), and temporary sim file is removed.

- [ ] **Step 5: Verify git working tree status**

Run: `git status -s`
Expected: Only `scripts/verify_workspace_clean.py` and `docs/superpowers/plans/2026-10-07-workspace-cleanup.md` appear in git status (no tracked files touched, no unintended files staged).

---
