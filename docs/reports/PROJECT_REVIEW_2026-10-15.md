# Repository Review — 2026-10-15

## Scope and confidence

Read-only architectural and static review of the hand-maintained RTL, firmware, build wiring, CI, and primary architecture/protocol documentation. Generated Vivado/vendor output and binary artifacts were excluded from source-level review. No tests, builds, synthesis, or hardware runs were performed. This is an engineering review, not clinical validation.

The worktree was already substantially modified before this review. The reviewed state includes both tracked modifications and untracked additions; preserve those changes when starting follow-up work.

## Project map

- `hardware/common/`: vendor-agnostic 8-tap moving average and PPG peak/IBI detector, shared by both FPGA targets.
- `hardware/zynq/`: AXI4-Lite Zynq wrapper, self-checking testbench, constraints and Vivado scripts. The documented `xc7z020clg400-1`/Vivado 2022.2 baseline is historical and should be cited only with its stated evidence caveat.
- `hardware/shrikefi/`: ForgeFPGA SPI top and target. `gen_flat_source.py` builds the Renesas single-file source from the maintained top, `spi_target.v`, and `hardware/common/`; edit maintained inputs and regenerate the vendor copy.
- `firmware/core/`: portable clinical/hazard engines, HRV, SpO2, SQI, respiratory extraction, pressure trend and INT8 models.
- `firmware/shrikefi/`: ESP-IDF ESP32-S3 application, sensor drivers, SPI FPGA programming/link, SOS, GNSS, IMU/fall detection, local web UI and optional MQTT.
- `firmware/zynq/`: host-side harnesses, sensor abstractions and Zynq-oriented comparison/build support.
- `data/mimic/`: benchmark fixtures; README correctly states that RMSSD and labels are synthetic/derived and results do not constitute independent clinical validation.
- `.github/workflows/`: Verilator lint, RTL simulation, host C builds/tests, data/model gates and secret scanning. Synthesis is currently disabled unless a self-hosted Vivado runner is enabled.

## Architecture contract to preserve

- Zynq uses AXI4-Lite; ShrikeFi uses 8-bit SPI mode 0. Their register and timing capabilities differ. On ShrikeFi, the MCU timestamps returned beat flags; the FPGA's 20 ns `ibi_cycles` is not carried over SPI.
- The MCU/FPGA pins are 3.3 V maximum. SPI pin assignments and boot-time FPGA configuration sequence are described in `docs/SHRIKEFI_LINK_PROTOCOL.md` and `firmware/shrikefi/shrikefi_pinmap.h`.
- Keep `hardware/common/` vendor-neutral, use nonblocking sequential RTL assignments, and synchronize asynchronous SPI inputs into the 50 MHz FPGA clock domain.
- ESP32-S3 target builds are configured at 160 MHz in the checked-in `sdkconfig.defaults`; 240 MHz is the silicon maximum, not the current configured clock. Avoid software `double` in target hot paths and preserve `s_data_mutex` protection of shared telemetry.
- MQTT is optional and disabled in defaults. The SoftAP is offline and currently open; do not assume its health data or control endpoints are access-controlled.

## Findings

### High — MPU6050 setup can report success with a partially configured sensor

In `firmware/shrikefi/mpu6050.c` (around lines 149–163), writes for sample-rate divider, DLPF, gyro/accelerometer range and interrupt configuration discard their return values. The function then marks the device initialized. A transient I2C failure can therefore leave fall detection running with sensor settings different from the scale factors in software. Check each required write and leave `initialized` false on failure.

### High — Open SoftAP exposes live health data and emergency controls

`firmware/shrikefi/web_status.c` sets `WIFI_AUTH_OPEN` (around line 660) and registers status/history, location update, SOS trigger and SOS cancel routes (around lines 680–685). Anyone in radio range can join and read telemetry or invoke those controls. Same-origin checks mitigate browser-origin requests but do not authenticate another client already connected to the AP. This may be an intentional rescue-access tradeoff; it needs to be treated as a deliberate threat-model decision, not described as private from nearby users.

### Medium — GPS fix expiry does not invalidate the parser's fix state

`task_gps_uart()` clears `s_has_gps_fix` after five seconds without a fresh fix, but it does not clear `s_gnss.data.fix_valid` (around lines 2004–2008). The five-second diagnostic later checks the parser flag and can continue logging old coordinates as `[3D FIX LOCKED]`. Downstream paths using the parser directly can also observe stale coordinates. Expire validity in one synchronized parser API or use the timestamp consistently at every consumer.

### Medium — target signal-processing code uses software-emulated double precision

The ESP32-S3 application compiles `firmware/core/ppg_sqi.c`, `spo2_engine.c`, and `pressure_trend.c`. These use `double` accumulators/arithmetic (for example SQI lines 43–83, SpO2 lines 43–58, pressure regression lines 84–108). This conflicts with the stated single-precision target guardrail and adds avoidable cost to Core 1. Convert to carefully bounded float or integer/fixed-point math where accuracy permits, then compare numerical behavior on representative inputs.

### Medium — host firmware build cannot demonstrate multicore synchronization

The non-ESP branch in `main_shrikefi.c` supplies no-op semaphore stubs (around lines 59–60). Host tests can exercise the algorithm and integration paths but cannot prove that cross-core state is protected or that lock timing is safe. Keep concurrency claims tied to source review or add target-side instrumentation/tests when needed.

### Low — documentation contains competing claims about clock and displayed validation status

`firmware/shrikefi/sdkconfig.defaults` sets the CPU to 160 MHz, while some project copy calls the device “240 MHz” without always distinguishing maximum from configured frequency. The README is comparatively careful about synthetic RMSSD, derived labels and x86-only inference timing; preserve those caveats wherever figures are repeated. A few docs also state test counts/verification as current facts; they should be rechecked against CI after the current worktree changes are finalized.

## Working tree and process notes

- The repository had many modified tracked files and untracked feature/docs/assets, including a new GNSS/MPU6050/fall-detection feature set and Zynq generated block-design files. These were not modified by this review.
- `firmware/shrikefi/main/CMakeLists.txt` currently includes the GNSS and fall-detection sources and the respiratory/SQI/pressure modules. Update this explicit ESP-IDF source list whenever adding `.c` files.
- Current CI is the best repeatable baseline for changes, but this review did not run it. Ask before treating any pre-existing CI claims as freshly verified.
- The local `.env` is ignored. Its contents were not read or copied into this report.

## Suggested first fixes when you want to work on findings

1. Make MPU6050 initialization fail closed on every required register-write error.
2. Define and document the intended SoftAP threat model; if open access is required, minimize exposed controls/data and make that tradeoff explicit in user-facing docs.
3. Make GNSS fix expiry atomic and authoritative for every coordinate consumer/log.
4. Replace double precision in ESP32 hot paths with numerically checked float/fixed-point implementations.
5. Treat host tests as functional tests, not proof of FreeRTOS multicore behavior.

## Follow-up repair and verification status (2026-10-15)

The findings above describe the static review before the repair pass. Current status:

- **MPU6050 setup:** fixed; required reset/configuration writes now fail initialization if they fail.
- **GNSS stale fix:** fixed; expiry now clears the parser's synchronized fix state, with rollover and recovery covered by tests.
- **ESP32 `double` math:** fixed in SpO₂, SQI, and pressure-trend target paths using float arithmetic; software and target builds passed.
- **SoftAP:** remains open by design for offline/rescue access and is documented as a privacy/control risk. It exposes health/status and emergency routes to associated clients.
- **Host concurrency limit:** unchanged; host tests do not prove FreeRTOS multicore behavior.
- **Clinical metric:** local evaluator rerun on 16,387 records: 94.11% accuracy but 58.18% sensitivity. This is not adequate evidence for clinical screening and must remain a prominent limitation.

Final software regression, ESP-IDF build, flash, and boot evidence are in the root `VERIFICATION_REPORT.md`. Physical boot confirmed MAX30102 FIFO reads and an I²C scan of OLED, MAX30102, MPU6050, and BME280. No finger contact or FPGA beat was captured, so actual vital measurement and FPGA peak processing remain unverified.
