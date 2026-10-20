# VALOR / SIH26181 — A-to-Z Project Analysis (2026-10-17)

> **Update 2026-10-20:** the M5 finding below about the PM2.5 calibration network has
> been acted on — that network was deleted and replaced by the published EPA/AirGradient
> correction (`firmware/core/pm25_correction.c`, see
> `docs/reports/PM25_CALIBRATION_2026-10-20.md`). This document is otherwise the
> record of the 2026-10-17 analysis as it was written.

**Scope:** the entire repository at `C:\Users\abhin\OneDrive\Desktop\verilog` — hardware RTL, both
firmware targets, clinical/TinyML core, dashboards, docs, CI and scripts.
**Method:** full source read + re-execution of the project's own gates + a from-scratch export and
build of the *committed* tree + a real ESP-IDF target build.
**Changes made:** **none.** This file is the only artifact created. No source, config, or hardware
was modified. The board (COM4) was left untouched.

> Reader's note: findings are marked **[verified]** where I reproduced them by running code or reading
> the exact lines; **[reported]** where a deeper sub-audit raised them and I confirmed by reading but
> did not execute them.

---

## 1. Verdict

The project is **healthy and builds today**, on both the committed tree and the working tree, and
every headline number reproduces. It is not in danger of "stopping working" from a build standpoint.
The real risks are, in order: **(a) a latent use-after-scope crash in the status-page path**,
**(b) a large body of uncommitted work that is not protected by git**, and **(c) documentation that
contradicts itself on safety-relevant numbers**.

---

## 2. Evidence — what was actually executed

| Check | Command (abridged) | Result |
|---|---|---|
| Zynq RTL bench | `iverilog … tb_ppg_system.v … && vvp` | **6 passed, 0 failed** |
| Common peak-detector bench | `iverilog … tb_ppg_peak_detector.v …` | **ALL 3 TESTS PASSED** |
| ShrikeFi SPI bench | `iverilog … tb_forgefpga_system.v …` | **10 passed, 0 failed** |
| Generated flat source | `python hardware/shrikefi/gen_flat_source.py --check` | **both files up to date** |
| Firmware engine suite | `gcc -Wall -Wextra -Werror -pedantic -std=c11 …` | **ALL TESTS PASSED** |
| Fall detection | `firmware/shrikefi/test_fall_detect.c` | **16/16 PASSED** |
| GNSS NMEA | `firmware/shrikefi/test_gps_l89hs.c` | **12/12 PASSED** |
| ShrikeFi host app | full source list | **Profiles 1–13 passed** |
| MIMIC-III triage gate | `accuracy_evaluator.c data/mimic/mimic_eval_feed.csv` | **acc 94.11%, sens 58.18%, spec 98.19%, INT8 agreement 99.95%, total 16387** |
| TinyML retrain (isolated copy) | `train_nn_risk_model.py --min-accuracy 86.0` | **88.47% val, params 619, arch 6-24-16-3, INT8 max err 0.09136, tier agreement 97.32%** |
| Dashboard verifier | `verify_dashboard.py` | **PASSED** |
| SoftAP page verifier | `verify_softap_page.py` | **PASSED** |
| **ESP-IDF target build** | `idf.py build` (ESP-IDF 5.5.5) | **Project build complete; app 0xf12c0 B, 87% free** |
| **Committed tree build** (`git archive HEAD`) | RTL + flat-check + engine | **all pass — committed tree is self-consistent** |

Not executed: Renesas ForgeFPGA vendor fitter/timing, Vivado synthesis, live hardware tests.

---

## 3. Repository / worktree state

- Working tree is **heavily dirty**: 45 modified tracked files (+3,287 / −989), plus many untracked files.
- **Untracked (i.e. not versioned)** — and this is the biggest practical risk:
  - Firmware: `firmware/shrikefi/fall_detect.{c,h}`, `gnss_nmea.{c,h}`, `gps_l89hs.{c,h}`,
    `mpu6050.{c,h}`, `test_fall_detect.c`, `test_gps_l89hs.c`, `dashboard/verify_softap_page.py`
  - Docs: `docs/PROJECT_GUIDE.md`, `docs/DEMO_RUNBOOK.md`, `docs/JUDGE_QA.md`, `docs/reports/`,
    `VERIFICATION_REPORT.md`, `CODEX_HANDOVER.md`, `docs/build_guide_pdf.py`
- **Good news:** the *committed* `main_shrikefi.c` and `main/CMakeLists.txt` and `ci.yml` do **not**
  reference those files, so a fresh clone of `main` still builds and passes its committed CI (verified).
  The danger is **partial commits** (e.g. committing the updated `ci.yml`/`CMakeLists.txt` without the
  drivers → red CI) and **loss of uncommitted work**.
- Branches: `main`, `fix/honesty-pass`, `port/shrikefi`; origin also has `novafang23-audit-fixes`,
  `novafang23-codebase-analysis`.
- **Tracked size ~30.9 MB**, dominated by a committed Vivado `.gen/` tree (128 files, ~16.5 MB) that
  includes Windows DLLs and multi-MB netlists — regenerable output, `.gitignore` misses `*.gen/`.

---

## 4. Findings register

### 🔴 Critical

**C1 [verified] — Use-after-scope pointer → undefined behaviour / possible panic.**
- `firmware/shrikefi/main_shrikefi.c:1972` `char live_gps_buf[GNSS_COORD_STR_LEN];` (block-scope local)
- `:1980-1982` `strncpy(live_gps_buf, s_last_gps_coord, …); … loc_str = live_gps_buf;`
- `:1986-1987` `st.loc = loc_str ? loc_str : location_get(); web_status_publish(&st);`
- `firmware/shrikefi/web_status.c:57` `s_snap = *s;` — shallow copy of the struct; only the pointer is copied.
- Block closes at `main_shrikefi.c:2005`; the HTTP task later reads `s_snap.loc` via
  `web_status.c:193` (`json_append_string` → `strlen`).
- **Reachability:** normal factory config — no stored location **and** a GNSS fix — while the page
  polls `/status.json` at 1 Hz. Contract violated: `web_status.h:73-74` ("strings must outlive the call").
- **Impact:** reads a dead stack frame → garbage location on the emergency card/page, possible load fault.
- **Fix:** copy the text into a static / `s_snap`-owned buffer (or embed `char loc[N]` in `valor_status_t`).

**C2 [verified] — Significant uncommitted work is not versioned** (see §3). Loss risk + partial-commit
CI-break risk. No code fix; commit it.

### 🟠 High

**H1 [verified] — SpO₂ seating gate is dead code.**
`firmware/core/spo2_engine.c:81` sets `state->last_ir_dc = ir_dc;` *before* `:103` computes
`dc_step = fabsf(ir_dc - state->last_ir_dc) / state->last_ir_dc` → always `0`, so the `> 0.15f`
rejection at `:104` can never fire. No test covers it (all SpO₂ tests hold DC constant).

**H2 [verified] — `clinical_fuse_triage()` converts `RISK_UNKNOWN` to `RISK_NORMAL`.**
`firmware/core/clinical_vitals_engine.c:297`: `if (clin_level > disaster_level)`, with
`RISK_UNKNOWN = -1` and `clin_level = RISK_NORMAL = 0` → `0 > -1` is true, so
`fused->overall_risk = RISK_NORMAL` and the "unknown" advisory is dropped. An unassessed hazard is
reported as an all-clear. Untested.

**H3 [verified] — SOS state mutated under two different locking regimes (race).**
`firmware/shrikefi/web_status.c:569-590` calls `sos_manual_trigger()` / `sos_cancel_now()` /
`sos_get_state()` / `sos_get_trigger()` (manual trigger/cancel with **no** lock at all, the getters
only under `WS_LOCK`); the monitor task mutates the same statics under `s_data_mutex`
(`main_shrikefi.c:1923-1933`). `sos.c:15-26` has **no internal lock**. Same class:
`location_get()` (`location.c:229`) is read cross-core while `location_set()` (`location.c:207`)
writes `s_location` with no lock. Impact: spurious latch / failed cancel / torn location string.

**H4 [verified] — `sdkconfig` Wi-Fi-core drift.**
Committed `firmware/shrikefi/sdkconfig` has `CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_0=y`
(`…_1 is not set`); `sdkconfig.defaults` sets `CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1=y`. Wi-Fi runs
on Core 0, the same core as the 100 Hz PPG acquisition task the design is built around.

**H5 [reported, verify vs TRM] — Zynq PS I²C slave address written unshifted.**
`firmware/zynq/i2c_hal.c:90,123,158` write the raw 7-bit address
(`Xil_Out32(base + I2C_ADDR, (uint32_t)slave_addr)`), while the Zynq-7000 (Cadence) ADDR register
places the slave address in bits **[7:1]** (headers supply 7-bit values: `0x57`, `0x76`, `0x3C`).
This path is inside `#ifdef ZYNQ_HW`, so it is **not** exercised by host tests and does **not** affect
the active ShrikeFi target — but it would fail on real Zynq hardware. Confirm against the Zynq TRM
before changing (suggested: `slave_addr << 1`).

**H6 [reported] — `convert_bitstream.py --check` cannot pass.**
`hardware/shrikefi/convert_bitstream.py:101-106` fails when `FPGA_bitstream_MCU.bin` is absent, and
that `.bin` is gitignored; yet `hardware/shrikefi/synthesis_evidence/README.md:117-118`,
`firmware/shrikefi/README.md:114`, `docs/FPGA_PPG_FIX_HANDOFF.md:569` all claim it passes. The
committed `forgefpga_bitstream.h` (46,408 B) is therefore unverifiable in a clean checkout.

**H7 [verified] — Unauthenticated remote SOS trigger/cancel on an open AP.**
The AP is `WIFI_AUTH_OPEN` by design; `h_sos_trigger`/`h_sos_cancel` (`web_status.c:565-595`) gate
only on an Origin/Referer check that any non-browser client can forge. Anyone in range can cancel an
emergency. (The open-AP *vitals disclosure* is a documented tradeoff; the *control* endpoint is not.)

### 🟡 Medium

| ID | Location | Issue |
|---|---|---|
| M1 | `firmware/shrikefi/wifi_credentials.h` | Real SSID + password in plaintext on disk (gitignored, so not in history). Rotate. |
| M2 | `firmware/shrikefi/max30102.c` `delay_ms` | `ms=29` → `vTaskDelay(pdMS_TO_TICKS(29))` = **2 ticks = 20 ms** at the audit snapshot's `FREERTOS_HZ=100` (now 200, i.e. 5 ticks = 25 ms); the 29 ms MAX30102 temp conversion was under-waited. |
| M3 | `firmware/shrikefi/pms5003.c:120` (and `firmware/zynq/pms5003.c:120`) | `if (!dev->has_valid_data || !data)` dereferences `dev` before the NULL check. |
| M4 | `hardware/shrikefi/forgefpga_ppg_top.v:35` | `spi_miso_oe` tied `1'b1`; the gated OE from `spi_target` is discarded → MISO never tri-stated; OE logic is dead. |
| M5 | `firmware/core/pm25_calibration_int8.c`; `disaster_risk_engine.h:219` | "3→8→4→1 ReLU network" is really a fixed affine map (8 zero weight rows, no training script). Moran-PSI/AHA helpers labelled "peer-reviewed clinical models" are hand-tuned heuristics and are **untested**. |
| M6 | `hardware/zynq/vivado_project/**/*.gen/` | 128 tracked files / ~16.5 MB of regenerable build output incl. 2 Windows DLLs. `.gitignore` misses `*.gen/`. |
| M7 | docs | Contradictory numbers (latency, Fmax, liveness, integer-only, SOS window…) — see §5 worksheet. |
| M8 | `firmware/zynq/Makefile:97-100` + `max30102.c:8-12` | `make embedded` (arm-none-eabi, `-nostdlib`) cannot compile: `max30102.c` unconditionally includes `<windows.h>`/`<unistd.h>` with no `ZYNQ_HW` guard. `make test` (host) is the documented target. |
| M9 | `firmware/zynq/i2c_hal.c` | (same as H5; listed once) |

### ⚪ Low

- `spi_target.v:25` — `BIT_MAX = WIDTH[2:0] - 3'd1` is only correct for `WIDTH=8` (3-bit underflow); latent for other widths.
- `axi_ppg_accelerator.v:159-163,171-186` — write path ignores `s_axi_wstrb` (full-word writes only; not triggered by `driver_ppg.c`).
- `moving_average_8tap.v:14-16` — comment claims `rst_n` is tied high in the ForgeFPGA top; the top actually drives it from the POR counter.
- `spo2_engine.c` — the "trimmed-mean" branch (`if n>=5`) is unreachable because `n` is capped at 3.
- `pm25_calibration_int8.c:101,127-129` — `(double)` casts around `isfinite` (host-only perf nit; not a guardrail violation).
- `README.md:40,568` references `data/mimic/mimic_vital_feed.csv` (absent); `reports/` (absent), `docs/theory/`, `docs/media/`, `docs/presentation/FINAL_FINAL_v6.*`, `CONTRIBUTING.md`, `ROADMAP.md` referenced but missing.
- `scripts/deepseek_copilot.py:123` disables TLS verification while sending an API key; `scripts/generate_sih_presentation.py:28` hardcodes an absolute user path; `verify_workspace_clean.py` treats gitignored paths as "protected" (fails on fresh clone).
- CI: `synthesize` job is permanently `if: false`; no secret-scan step despite README/review claiming one; NN gate floor is `--min-accuracy 86.0` while docs advertise 88.47%; no gate on `convert_bitstream.py --check` or the dashboard verifiers.

---

## 5. Document contradiction worksheet (for your per-number decision)

| Topic | Value A (source) | Value B (source) | Code/evidence says |
|---|---|---|---|
| ESP32-S3 inference latency | 0.44 µs, host-measured, target **not** measured (`README.md:53,62,190,371`, `MASTER_PROJECT_GUIDE.md:667`) | 0.38 µs; "~18 µs on ESP32-S3" (`PROJECT_GUIDE.md:191,288,596`, `JUDGE_QA.md:140,188`, `DEMO_RUNBOOK.md:198`) | Target not measured; `0.44 µs` is x86-64 `-O2` |
| ForgeFPGA Fmax | "timing closure Fmax 82.73 MHz" (`JUDGE_QA.md:74`, `PROJECT_GUIDE.md:100,537,812`) | "no clock constraint exists; do not quote as closure" (`synthesis_evidence/README.md:104-108`, `MIGRATION.md:61-64`) | No XDC/PCF clock constraint; fitter auto-constrained 500 MHz |
| FPGA liveness proof | "0x55 probe is real proof a configured design is running" (`README.md:21`); "DONE asserted" (`DEMO_RUNBOOK.md:101`, `JUDGE_QA.md:83-86`) | "probe returns 0x00, not a liveness signature" (`SHRIKEFI_LINK_PROTOCOL.md:89-96`, `VERIFICATION_REPORT.md:103`) | Probe returns 0x00; only a beat flag after real contact is evidence |
| INT8 "integer-only" | "integer-only silicon; no floating-point expf()" (`JUDGE_QA.md:51,140,143`) | "dequantises to float for the MAC" (`MASTER_PROJECT_GUIDE.md:721,849`) | `nn_risk_model_int8.c:117` calls `expf()` |
| SOS critical confirm window | 3 s (`sos.h:59`, `REMAINING_WORK.md:33`, T1.2) | 15 s (`PROJECT_GUIDE.md:73,290`); 10 s (`DEMO_RUNBOOK.md:153`) | `sos.c:30` = **15000 ms** |
| SpO₂ acquisition windows | 8 windows @ 2.5% (`spo2_engine.h` prose; `MASTER_PROJECT_GUIDE.md:484-485`; `SHRIKEFI_HARDWARE_CONNECTIONS.md:306`) | 3 windows (`spo2_engine.h:56-58` constants) | constants = 3; prose stale |
| Perfusion-index range | 0.15–15% (`MASTER_PROJECT_GUIDE.md:482`) | 0.1–10% (`PROJECT_GUIDE.md:370`) | 0.02–15.0 (`spo2_engine.h:26-27`) |
| HR critical override | 140 (`PROJECT_GUIDE.md:101,359`) | 150 (`MASTER_PROJECT_GUIDE.md:609,630`) | — |
| Bradycardia BPM | 35 (`PROJECT_GUIDE.md:360`) | 40 (`MASTER_PROJECT_GUIDE.md:560`) | — |
| Pressure-trend window | 30 min (`REMAINING_WORK.md:300`) | 180 min / 3 h (`PROJECT_GUIDE.md:119,227`, `DEMO_RUNBOOK.md:144`) | — |
| AGENTS.md CPU freq | 240 MHz (`AGENTS.md:15`) | 160 MHz (`sdkconfig.defaults`) | 160 MHz |
| Qualcomm LPI power | < 1 mW (`QUALCOMM_PLATFORM_STRATEGY.md:10`) | < 5 mW (`README.md:67`) | — |
| ForgeFPGA LUTs | 363/1120 (`README.md:326`, `MIGRATION.md:41`) | 443 (older decks/theory) | 363 in current log; superseded copy in evidence dir |

---

## 6. What is solid (do not "fix" these)

- **Build discipline:** firmware is warning-clean under `-Wall -Wextra -Werror -pedantic -std=c11`;
  RTL lints clean (Verilator `-Wall`).
- **No AGENTS guardrail violations:** no software `double` in target arithmetic (the `double` grep
  hits are host-only tools and comment false-positives), no `strcpy`/`sprintf`/`gets`/`system`.
- **RTL:** single-sourced vendor-agnostic filter/detector, CI-gated generated flat file, 2-FF CDC in
  `spi_target`, no latches, non-blocking sequential style, register map matches `driver_ppg.c` exactly.
- **Buffer hygiene:** `esp32_i2c_hal.c`, `ssd1306.c`, `bme280.c`, `max30102.c`, `location.c`,
  `web_status.c` JSON — bounded, NUL-terminated, NULL-checked (except M3).
- **Safety-by-design:** unknown inputs propagate to `RISK_UNKNOWN` in the engine (except via H2);
  the status page derives nothing; 100% offline with no CDN; XSS-safe rendering.
- **All name helpers return statics**, so C1 is the only dangling-pointer instance.

---

## 7. Live-verification backlog (board is connected — run when you choose)

These were never proven on hardware and cannot be proven statically. Each needs your physical input:

1. Finger firmly on MAX30102 for 30 s → confirm HR ~60–90, valid SpO₂, SQI ≥ 0.70, and that
   split-peak intervals (e.g. ~669/1020 ms) are rejected without an RMSSD spike.
2. OLED visible output (SOS card + live vitals).
3. GNSS fix with antenna and clear sky → confirm a coordinate string and that C1's fix would
   matter here.
4. Join `VALOR-xxxx` from a phone → `/status.json` live, SOS trigger **and cancel**, stale-link clearing.

---

## 8. Suggested order of work (for later; not done here)

1. **Commit the untracked work** (protective, no code change) — before anything else.
2. Fix **C1** (UAF) — copy GPS text into an owned buffer; add a regression.
3. Fix **H2** (`fuse_triage` UNKNOWN) and **H1** (SpO₂ gate) with tests.
4. Fix **H3** (single lock for SOS/location) and re-run the host suite + target build.
5. Reconcile docs to your chosen canonical values using the §5 worksheet.
6. Optional: purge `*.gen/` from git; add a real secret-scan job; rotate the Wi-Fi password;
   confirm H5 against the Zynq TRM.

---

*Generated as a read-only analysis. No repository files or hardware state were modified.*
