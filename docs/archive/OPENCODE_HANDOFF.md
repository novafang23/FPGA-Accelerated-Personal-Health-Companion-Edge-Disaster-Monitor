# VALOR / ShrikeFi — OpenCode Handoff

This file is a read-first handoff for the next coding agent. It records the current problem, the debugging history, the verified hardware facts, the changes already made, the current source-tree state, and the important source/firmware mismatch. **Do not repeat the full repository search before reading this file.**

## Project and target

- ESP32-S3 ESP-IDF firmware, ESP-IDF 5.5.5.
- Active firmware project: `firmware/shrikefi`.
- Host-side algorithm tests: `firmware/zynq/test_disaster_risk_engine.c`.
- Shared I2C bus:
  - SDA GPIO1
  - SCL GPIO2
  - MAX30102 address `0x57`
  - OLED address `0x3C`
  - BME280 address `0x76`
  - MPU6050/unknown device address `0x68`
- FPGA link uses SPI pins documented in `firmware/shrikefi/shrikefi_pinmap.h`.

## Original user problem

The user reported that the MAX30102 visible optical output was flickering and the OLED was resetting/flickering after code changes. The user repeatedly stated that the wiring and finger placement were already fine and that the system worked before the changes. Do not restart the investigation by blaming wiring or asking them to rewire.

The main technical causes investigated were:

1. Runtime MAX30102 LED-current changes causing visible optical flicker and waveform steps.
2. MAX30102 saturation at the previous high LED setting.
3. Shared I2C transactions between MAX30102, OLED, BME280, and MPU6050.
4. A weak/over-sensitive PPG Signal Quality Index calculation.
5. SpO2 red/IR ratio instability during acquisition.

## Hardware facts already proven

A controlled reset and diagnostic capture proved that the bus and sensor can work:

```text
I2C idle levels after init: SDA=1 SCL=1
Found device at 0x57
Detected MAX30102 sensor (Part ID: 0x15)
MAX3010x initialized successfully
```

Normal full-bus scans have found:

```text
0x3C OLED
0x57 MAX30102
0x68 unknown/MPU6050
0x76 BME280
```

The MAX30102 has produced real HR readings, so the sensor path is not completely dead. The OLED has also successfully completed I2C framebuffer transfers.

The previous LED setting around `0x4F`/79 caused IR saturation at `262143`. Do **not** restore that setting.

## Last verified live behavior before the current source-tree changes

The last device images that were flashed during this session used the tested I2C/LED baseline:

```text
I2C: 400 kHz
MAX30102 LED: 48/48 = 0x30/0x30
```

A live capture with contact showed roughly:

```text
HR: 78–87 BPM
MAX30102 windows: 16
valid windows: 10
SpO2: --
robust SpO2 spread: about 19%
SQI: about 0.45 in the later run
```

Another capture after the SQI change showed:

```text
SQI improved from approximately 0.2–0.3 to approximately 0.72
Status: READY
HR: approximately 87 BPM
IBI samples: 33
```

That demonstrated that the original SQI `max-min` calculation was a real problem. SpO2 was still withheld because the red/IR ratio spread was too large.

Some captures contained no finger contact and explicitly reported `NO_FINGER`; those captures do not test the SpO2 or SQI path.

## Changes already made in the working tree

The working tree is intentionally dirty and contains broad earlier project changes. At the last audit there were approximately 45 tracked changed files and approximately 40 untracked files. Do not assume every change belongs to one feature.

### PPG SQI — `firmware/core/ppg_sqi.c/.h`

The SQI implementation was changed from raw absolute `max-min` AC estimation to:

- Mean/variance standard-deviation AC estimate.
- Equivalent sinusoidal peak-to-peak scaling using `2*sqrt(2)*sigma`.
- Clipped skewness calculation to reduce the influence of isolated optical spikes.
- Optional IMU motion-energy penalty and high-motion gating.
- Beat-interval regularity remains 40% of the composite score.
- Clinical SQI threshold remains `PPG_SQI_THRESHOLD_VALID = 0.70f`.
- A no-beat waveform is deliberately capped below the validity threshold.

Important inconsistency:

- `firmware/core/ppg_sqi.h` documents motion penalty above `0.10g`.
- `firmware/core/ppg_sqi.c` applies the penalty above `0.08f`.

Do not lower the clinical threshold merely to make the dashboard look READY.

### SpO2 — `firmware/core/spo2_engine.c/.h`

The engine now uses Welford mean/variance accumulators for red and IR instead of subtracting large squared sums. It includes:

- DC, AC, perfusion-index, and ratio-R gates.
- Moving history of SpO2 estimates.
- Sorted history with one low and one high estimate discarded when enough samples exist.
- Trimmed mean publication.
- Monotonic-drift rejection logic.
- Diagnostic fields for total windows, valid windows, reject reason, spread, raw spread, and settled state.
- A post-validity slew limiter.

The current source-tree values are important and require review before flashing:

```c
#define SPO2_WINDOW_SIZE             (PPG_SAMPLE_RATE_HZ / 4)
#define SPO2_MIN_PERFUSION_INDEX     0.02f
#define SPO2_MIN_RATIO_R             0.10f
#define SPO2_MAX_RATIO_R             2.20f
#define SPO2_REQUIRED_VALID_WINDOWS  3
#define SPO2_STABLE_MIN_WINDOWS      3
#define SPO2_STABLE_SPREAD_PCT       3.5f
```

The FIFO is configured for four-sample averaging, so `PPG_SAMPLE_RATE_HZ / 4` is approximately 25 FIFO samples and approximately one second. That part is internally plausible.

However, the current three-window gate and widened R range are much weaker than the earlier conservative eight-window gate. A ratio of `2.2` is outside the prior physiological range and maps below the 70% raw calibration floor before clamping. This can make bad optical data look like a valid 70% reading.

### MAX30102 — `firmware/shrikefi/max30102.c/.h`

Current source configuration includes:

- MAX30102 detection by part ID `0x15` and MAX30100 compatibility handling.
- FIFO averaging `MAX30102_FIFO_SMP_AVE_4`.
- Fixed initialization LED value currently defined as `0x36`.
- Runtime LED adjustment function still exists, but the active main path does not currently call it; verify this before changing that assumption.
- FIFO recovery that avoids rewriting mode/LED registers on ordinary FIFO errors.
- FIFO pointer burst reads and fallback individual reads.

The current source header comments and implementation comments disagree about the current represented by `0x36`. The last verified live image showed `48/48` (`0x30/0x30`), not `0x36`.

### I2C HAL — `firmware/shrikefi/esp32_i2c_hal.c/.h`

Current source includes:

- Recursive I2C mutex.
- Atomic HAL reads/writes and OLED lock usage.
- SDA/SCL idle-line logging.
- I2C scan diagnostics.
- A GPIO bit-bang recovery routine that clocks up to nine pulses and generates STOP.

Potential issue: `xSemaphoreCreateRecursiveMutex()` is not checked for allocation failure. If it returns NULL, the HAL silently runs unlocked.

Potential issue: the recovery routine changes GPIO ownership while the legacy I2C driver remains installed and only calls `i2c_param_config()` afterward. Treat recovery as high-risk and test it separately.

### Pin map — `firmware/shrikefi/shrikefi_pinmap.h`

The current source says:

```c
#define PIN_I2C_SDA      1
#define PIN_I2C_SCL      2
#define I2C_BUS_SPEED_HZ 100000
```

The last verified live baseline used 400 kHz. Earlier testing at 100 kHz did not improve the MAX30102 response and may increase shared-bus occupancy. Do not assume the current 100 kHz source setting is the same as the last flashed image.

### Main application — `firmware/shrikefi/main_shrikefi.c`

The file has a large integration diff, including:

- MAX30102 retry and diagnostics.
- I2C recovery retry path.
- FPGA/software beat handover.
- HRV quality gating.
- PPG SQI integration.
- Respiration-rate tracking.
- Fall/GPS/environment features.
- Contact and rail detection.

There is still a build warning:

```text
unused variable 'hrv_quality_changed'
```

The current main task uses `PPG_FIFO_RATE_HZ` for most timing calculations, which is correct if FIFO averaging remains enabled.

## Important current source bug

In `firmware/core/spo2_engine.c`, the intended DC baseline jump check is currently ordered incorrectly:

1. The current window assigns `state->last_ir_dc = ir_dc`.
2. Later code compares `ir_dc` against `state->last_ir_dc`.

That means the comparison is against the current value, not the previous window value, and the `>15%` seating-transient rejection cannot trigger as intended.

This must be fixed before trusting the new DC-stability gate.

## Current source vs flashed firmware warning

The current source tree should be treated as **not equal to the last verified board image**.

Evidence from the last live run:

- The board booted at 400 kHz, while current source says 100 kHz.
- The board reported LED current 48/48, while current source defines `0x36`.
- The timing of the last capture was consistent with the earlier flashed window configuration, not necessarily the current `/4` source setting.

Do not flash the current tree blindly. First review the SpO2 safety gate, the I2C speed, and the LED default.

## Tests already run

The host regression command used was equivalent to:

```powershell
gcc -Wall -Wextra -Werror -pedantic -std=c11 `
  -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi `
  firmware/zynq/test_disaster_risk_engine.c `
  firmware/core/nn_risk_model.c `
  firmware/core/nn_risk_model_int8.c `
  firmware/core/hrv_analysis.c `
  firmware/core/pm25_calibration_int8.c `
  firmware/core/clinical_vitals_engine.c `
  firmware/core/spo2_engine.c `
  firmware/core/ppg_sqi.c `
  firmware/core/disaster_risk_engine.c `
  firmware/core/pressure_trend.c `
  firmware/core/ppg_respiratory_rate.c `
  firmware/shrikefi/sos.c `
  firmware/shrikefi/location.c `
  firmware/shrikefi/web_status.c `
  -lm
```

The current source checkout passes:

```text
ALL TESTS PASSED
```

This only validates the host algorithms. It does not validate I2C timing, ESP-IDF behavior, LED current, or the current board image.

## Zynq/RTL changes

`hardware/zynq/axi_ppg_accelerator.v` has a small AXI read-handshake change:

```verilog
if (~s_axi_arready && s_axi_arvalid && ~s_axi_rvalid)
```

This appears intended to prevent accepting a new read address while a response is active, but it should be checked with the RTL testbench.

`hardware/zynq/vivado_project/zynq_ppg_system.xpr` contains mostly generated/project metadata changes. Do not claim new timing/resource results without rerunning Vivado. The documented baseline remains:

- `xc7z020clg400-1`
- WNS `+5.603 ns`
- WHS `+0.184 ns`
- Fmax `69.45 MHz`
- 185 LUT, 16 LUTRAM, 266 FF, 0 DSP48, 0 BRAM
- RTL testbench: 6/6 passing

## Other repository changes

There are also changes in:

- Clinical vitals, disaster-risk, HRV, pressure, and SOS logic.
- Dashboard/web-status HTML and Python verification scripts.
- BME280, SSD1306, SPI/FPGA link, GPS/GNSS, MPU6050, and fall detection.
- Documentation, PDFs, reports, images, CI, and build configuration.
- Generated Vivado block-design files under `hardware/zynq/vivado_project/*gen` and `*srcs`.

Several generated files and demo artifacts are untracked. They should not automatically be committed as production source.

`pmsa003.c/.h` are deleted, but the active firmware appears to use `pms5003.c`; search the build files before assuming the deletion is safe.

## Recommended order for OpenCode

1. Read this file first.
2. Confirm the current source values in `spo2_engine.h`, `shrikefi_pinmap.h`, and `max30102.h`.
3. Fix or explicitly justify the dead DC-stability comparison.
4. Decide whether the conservative SpO2 gate should remain 8 windows/full history instead of the current 3-window gate.
5. Keep the physiological ratio bounds conservative; do not silently accept `R=2.2` merely to make SpO2 appear.
6. Decide whether 100 kHz or the verified 400 kHz I2C baseline is wanted, and keep source, build, and board consistent.
7. Decide whether LED `0x36` or the verified `0x30` is wanted; do not restore the old saturating `0x4F` setting.
8. Add regression tests for:
   - A DC baseline jump.
   - A slowly changing but non-monotonic SpO2 ratio.
   - An R value above the accepted physiological range.
   - Intermittent valid windows that must not latch too early.
9. Run host tests and an ESP-IDF build.
10. Only then flash and capture a 30–60 second run with finger contact.

## Explicit constraints

- Do not blame wiring/finger placement without evidence; the bus and MAX30102 have already been proven functional.
- Do not use the old high LED setting that saturated IR.
- Do not lower the SQI clinical threshold just to show READY.
- Do not publish SpO2 when the red/IR ratio history is unstable.
- Do not claim a flashed-board result from source-only changes.
- This handoff is informational; no source files were changed while producing it.
