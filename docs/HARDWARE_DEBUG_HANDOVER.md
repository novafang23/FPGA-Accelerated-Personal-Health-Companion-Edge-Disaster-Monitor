# Hardware Debug & Stability Handover Guide
**Project:** VALOR — FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor (SIH26181)  
**Target Platform:** Vicharak ShrikeFi (ESP32-S3 Dual-Core Xtensa LX7 + Renesas ForgeFPGA SLG47910)  
**Port:** COM4 @ 115200 baud (460800 baud for flashing); the port can change
between USB reconnects — confirm in Device Manager.
**Date:** October 15, 2026 (updated October 19, 2026)

---

## 1. Executive Summary & Current System State

* **Firmware Flashing**: Flashed on **2026-10-19** with the peak/4 crest bitstream
  (`d9d58daa…`, 46,408 bytes) and the HRV readiness firmware. Boot verified on
  COM4: `configuration COMPLETE! (46408 bytes loaded)`, then MAX30102, BME280,
  PMS5003, GNSS, MPU6050, OLED and SoftAP initialized. Live HRV validation is
  still pending (Section 6).
* **FPGA Link**: The current Renesas ForgeFPGA SLG47910 bitstream (46,408 bytes,
  SHA-256 prefix `d9d58daa`) loads cleanly on SPI2 on every boot
  (`SLG47910 configuration COMPLETE!`).
* **SoftAP**: `VALOR-0081` active on `192.168.4.1`.
* **UART Peripherals**: u-blox NEO-M8N GNSS (UART2: GPIO16/17) and PMS5003 PM2.5 (UART1: GPIO14) are active and streaming.
* **I2C Bus Status**:
  * **SSD1306 OLED (0x3C)**: Active and communicating; 1024-byte framebuffers transfer cleanly.
  * **MPU6050 IMU (0x68)**: Active and communicating at 40 Hz; fall detection running.
  * **MAX30102 (0x57) & BME280 (0x76)**: Active on the 2026-10-19 boot; both
    initialized OK. The earlier `ESP_FAIL` was a breadboard jumper seating
    problem, now resolved (see Section 4).
* **Fall/SOS Physical Test (2026-10-19)**: **1/1 drop trial passed.** The FSM
  confirmed the fall and the full SOS path was exercised: 10 s countdown,
  buzzer, and 5 s hold-to-cancel. This is a single bench trial; false positives
  and multi-trial/orientation testing have not been characterised, so it is not
  field-validated.

---

## 2. Issues Investigated & Firmware Fixes Applied

### A. Heartbeat Counter Resetting & Never Reaching 10 Beats
* **Problem**: The user placed their finger, heartbeats were detected, but OLED showed guidance messages and the counter never counted up to 10 heartbeats, resetting back to 0.
* **Root Causes**:
  1. **Detector Handover Wipe**: In `firmware/shrikefi/main_shrikefi.c`, `ibi_pipeline_submit()` contained `if (p->source != source) { hrv_init(hrv); ... }`. Whenever a beat was detected by ForgeFPGA and the next by software fallback (or vice versa), `hrv_init()` **wiped the accumulated beat count back to 0**.
  2. **FPGA Liveness Lockout**: `fpga_alive = (s_last_fpga_beat_ms != 0)` was permanently sticky. If the FPGA emitted an initial beat or noise spike at finger contact, `fpga_alive` remained `true` forever, permanently muting the software peak detector. If the FPGA stopped emitting beats, the system was completely gagged.
  3. **Strict Contact Threshold**: `PPG_CONTACT_THRESHOLD` was set to `10000u`. Gentle or moderate finger placement produces 5,000–8,000 counts, which tripped the 100-sample disconnect counter and wiped the session.
* **Fixes Applied**:
  1. In `ibi_pipeline_submit()`, removed `hrv_init(hrv)` on detector handover. Accumulated beats (`hrv->count`) are preserved so the progress bar smoothly advances (`Reading... 1/10` $\to$ `10/10`).
  2. Added a 2.5-second timeout to `fpga_alive`: `bool fpga_alive = (s_last_fpga_beat_ms != 0) && ((now_ms - s_last_fpga_beat_ms) < 2500);`. If ForgeFPGA does not emit a beat within 2.5s, software DSP peak detection takes over automatically.
  3. Lowered `PPG_CONTACT_THRESHOLD` to `7000u`. This stays well above ambient board optical leakage (~4,400 counts) while reliably capturing finger contact.

### B. OLED Screen Glitching, Blanking & Shared I2C Bus Freezing
* **Problem**: Reconnecting OLED or running multiple sensors caused broken pixels, repeated black screen clears/refreshes every 1-3 seconds, and cascading `ESP_FAIL` errors across all I2C devices.
* **Root Causes**:
  1. **Premature Timeout (50 ms)**: The hardware I2C timeout had been erroneously reduced from 1,000 ms to 50 ms. Under FreeRTOS dual-core bus contention, long 1024-byte OLED framebuffer bursts and multi-byte sensor reads exceeded 50 ms and timed out prematurely.
  2. **Driver Deletion Storm**: `esp32_i2c_hal.c` contained an automatic recovery loop that executed `i2c_driver_delete()` and GPIO reconfiguration every 5 errors. When an optional or floating sensor NACKed, Core 0 was tearing down and reinstalling the I2C driver 20 times per second while Core 1 was mid-frame.
  3. **Destructive Display Unmarking (`disp->initialized = 0`)**: On any dropped packet, `ssd1306_update()` set `disp->initialized = 0`, causing `main_shrikefi.c` to execute `ssd1306_init()` and wipe the screen to black every second.
* **Fixes Applied**:
  1. Restored the hardware I2C transaction timeout to **1,000 ms** (`1000 / portTICK_PERIOD_MS`) in `firmware/shrikefi/esp32_i2c_hal.c` across `read`, `write`, and `write_raw`.
  2. Removed live runtime `i2c_driver_delete()` storms from normal read/write paths. `i2c_recover_bus_locked()` now checks if bus lines are actually held low (`sda == 0 || scl == 0`) before attempting hardware clock pulses.
  3. Removed `disp->initialized = 0` from `ssd1306_update()`. On transient packet drops, `ssd1306_resync()` restores column/page registers without clearing the screen to black.
  4. Restored `MAX30102_FIFO_SMP_AVE_4` (4-sample digital filtering) in `firmware/shrikefi/max30102.c` to suppress optical noise and prevent FIFO overflows.

### C. MAX30102 LED Blinking Off and On Intermittently
* **Problem**: MAX30102 Red/IR LEDs turned off and on cyclically.
* **Root Cause**: Old error handling executed `max30102_reset()` on consecutive I2C read glitches. `max30102_reset()` writes `MODE_RESET` to register 0x09, which shuts down the internal LED drivers (`LED1_PA = 0x00`). When re-initialized 1 second later, the LEDs turned back on, only to be turned off again on the next bus collision.
* **Fix Applied**: Replaced hard resets with `max30102_recover_fifo()`, which resets FIFO read/write pointers while keeping `MODE_CONFIG` and LED currents (`0x4F` = 15.8 mA) continuously active.

---

## 3. Hardware Questions Answered

### Q1: Do I need to regenerate the ForgeFPGA bitstream?
**Not for the current peak/4 build — it is already done.** The 2026-10-19
bitstream (`d9d58daa…`, 46,408 bytes) is in `forgefpga_bitstream.h`, was flashed
on COM4, and loads into the SLG47910 without error over SPI2. The earlier
heartbeat-LED problem was the optical threshold rejecting the raw signal
(`< 10000 counts`), which streamed zeroes to the FPGA (`FG ... 0 0 0`), not a
bad bitstream. If you change the RTL again, **yes**: regenerate the flat source,
re-fit in the Renesas GUI, run `convert_bitstream.py`, and reflash.

### Q2: What happens if I accidentally press the BOOT button on ShrikeFi?
**Nothing harmful.** GPIO 0 (BOOT) is only sampled by the ESP32-S3 ROM bootloader during chip reset (when holding BOOT while pulsing RST or during power-up). Pressing BOOT during normal runtime does not alter flash memory or reset the MCU.

---

## 4. Current Hardware Breadboard Action Checklist

Current 2026-10-19 boot scan:
```
0x3C  SSD1306 OLED        OK
0x57  MAX30102 PPG        OK
0x68  MPU6050 IMU         OK
0x76  BME280 T/H/P        OK
```
All four devices are communicating on GPIO 1 (SDA) and GPIO 2 (SCL). The earlier
MAX30102/BME280 `ESP_FAIL` was a breadboard jumper-seating problem; the runtime
retry recovered both. If it recurs:

1. **Physical finger flexion**: pressing the MAX30102 can rock the breadboard
   header pins and lose contact.
2. **Re-seat 3.3 V and GND** to MAX30102 and BME280 in fresh tie-points.
3. **Re-seat SDA/SCL**: MAX30102 SDA -> GPIO 1, SCL -> GPIO 2.
4. The runtime retry probes every second; when contact is solid, expect:
   `I (...) SHRIKEFI_MAIN: MAX30102 initialized after a runtime I2C retry`
   followed by beat counting.

---

## 5. Verification Commands for the Next Session

### Run Host Simulation Tests (All 12 Profiles):
```cmd
cd /d C:\Users\abhin\OneDrive\Desktop\verilog\firmware\shrikefi
run_host_test.bat
```

### Re-flash Firmware to ShrikeFi (COM4):
*Note: Due to Windows terminal sandboxing intercepts on Xtensa tool DLL relocations, run flash commands with `BypassSandbox: true`.*
```cmd
cmd.exe /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py -p COM4 flash"
```

### Live Serial Monitor (COM4 @ 115200 baud):
```python
python -c "import serial, time; s = serial.Serial('COM4', 115200, timeout=1); [print(s.readline().decode('utf-8', 'replace').strip()) for _ in range(50)]; s.close()"
```

---

## 6. Pending HRV / RMSSD Readiness Verification (updated 2026-10-19)

**Status: FLASHED; one 5-minute steady capture assessed (see Capture 4).** The
current tree passes the host suite, the 34-test Zynq suite and the ESP-IDF build.
The HRV readiness changes and the 2026-10-19 peak/4 bitstream are flashed on
COM4, and boot initialization is verified.

A 302 s still-contact capture on 2026-10-19 confirmed steady HR/SpO₂/SQI, the
absence of the old split-pair signature, and the SQI gate withholding HRV during
motion (full numbers in
[`reports/HRV_STILL_CAPTURE_2026-10-19.md`](reports/HRV_STILL_CAPTURE_2026-10-19.md)).
It also showed RMSSD takes ~3 minutes to settle, so the first 1–2 minutes of an
RMSSD reading are window-fill, not measurement. **That artefact is now gated in
firmware (2026-10-20):** `hrv_is_settled()` withholds RMSSD until the window holds
240 of 300 beats *and* spans ≥ 120 s of contact, so the panel should read
`HRV: Stabilizing` for ~2–3 minutes and then hold a stable value. That withheld
period has **not** yet been confirmed by a capture, so the physical **OLED**
state progression (the table below) is still **not** visually confirmed, and
T4.1 (ECG/chest-strap comparison) is still open. Do not claim HRV accuracy or the
OLED HRV states as hardware-verified.

### What changed in the flashed firmware

- `hrv_is_settled()` no longer takes the snapshot shortcut when a real 300-interval
  ring has wrapped to `head == 0`.
- All three publication paths now require `hrv_is_settled()`. The old
  `count >= 10 && rmssd > 0` fallback no longer bypasses the requirement for
  nine contiguous valid pairs.
- `hrv_is_ready()` is back to its structural meaning (enough contiguous pairs to
  compute a value) because the risk engines and the sibling dashboard use it as
  a summary-validity check. The publication policy lives only in
  `hrv_is_settled()`, so a caller cannot accidentally inherit it.
- The OLED shows `HRV: Low Signal` for SQI 0.50–0.69. `HRV: Stabilizing` is now
  reserved for SQI >= 0.70 with RMSSD still withheld.
- `firmware/zynq/Makefile` links `ppg_baseline_rr.c` and
  `pm25_calibration_int8.c`; its three host targets compile and the test target
  passes.

### Flash and boot

1. Connect the board and confirm its current port (COM4 on 2026-10-19; it can
   change after a USB reconnect).
2. Flash and monitor:

   ```cmd
   cmd.exe /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py -p COM4 flash monitor"
   ```

   If the Windows sandbox intercepts the Xtensa tool DLLs, use the same command
   with sandbox bypass as noted in Section 5.
3. Confirm the boot log still reports:
   - `SLG47910 configuration COMPLETE!`
   - SSD1306 at `0x3C`, MPU6050 at `0x68`
   - MAX30102 at `0x57` and BME280 at `0x76` (re-seat jumpers if not)
4. Expect the storm card to show `CYCLONE=UNKNOWN` after a flash: the 30-minute
   pressure window is reset. This is not an HRV fault.

### Finger test

Run at least two contacts:

1. **Steady contact, 60–90 s:** rest the finger without pressing hard or moving.
2. **Deliberate mild movement or light contact:** force SQI below 0.70 so the
   guidance path is exercised.

Capture the full serial log for both. The useful lines are:

- `IBI <source> <ms> <verdict> ref=<median> n=<count> rmssd=<ms>`
- `[PPG] ...`
- `[TELEMETRY] ...`
- `[TRIAGE] ...`

### Expected OLED progression

| Condition | Expected line |
|---|---|
| Fewer than 10 accepted intervals | `Reading... (n/10)` |
| 10+ accepted, RMSSD still withheld, SQI 0.50–0.69 | `HRV: Low Signal` |
| SQI below 0.50 | `Hold Finger Still` |
| SQI >= 0.70, window still filling (`count` < 240) | `HRV settle n/240` |
| SQI >= 0.70, window >= 240 beats but span or pair continuity unmet | `HRV: Stabilizing` |
| Ready | `HRV RMSSD:xx.xms` |

`HRV settle n/240` is the publish gate made visible: RMSSD is withheld until the
window holds 240 of 300 beats **and** spans 120 s of contact, so the line reports
how far the window has filled. `HRV: Stabilizing` is then a UI state, not a
separate filter — with a steady finger and SQI >= 0.70 the window is full but the
120 s span or NN adjacency is not yet met; one rejected interval costs roughly one
extra accepted beat before RMSSD publishes.

### Acceptance criteria

- `n` reaches 10 and then continues past it; it must not freeze.
- RMSSD publishes a nonzero, plausible resting value with SQI >= 0.70; it must
  not jump to the old ~190 ms artefact value.
- `HRV: Stabilizing` must not appear while SQI is below 0.70.
- The HRV line passes through `HRV settle n/240` with `n` climbing before any
  RMSSD appears; `n` must advance steadily to 240 rather than freeze.
  **Checked 2026-10-20 on a 30 s contact:** `n` climbed `0 -> 27` while 28 `ok`
  IBI intervals were accepted (one 1 Hz sample of lag), RMSSD stayed withheld at
  SQI 0.85-0.86, and `HRVN` in `[TELEMETRY]` carried the same count.
- The OLED RMSSD matches the `[TELEMETRY]` RMSSD at the same time.
- No detector handover every few seconds during steady contact. If it happens,
  capture the timestamps — it is the 2.5 s fallback interacting with the
  measured 3.38 s FPGA silences, not the RMSSD arithmetic.

### Live capture results — 2026-10-19 (COM4)

Boot and sensor verification passed: ForgeFPGA bitstream 46,408 bytes; I²C
0x3C/0x57/0x68/0x76; MAX30102, BME280, PMS5003, GNSS, MPU6050 and OLED all
initialized; SoftAP `VALOR-0081`; `[PPG]` streaming; `PI=` present in telemetry.

**Capture 1 (before PI gate):**

- HR 81–85 BPM, SpO₂ 97.3–98.2 %, SQI 0.84–0.88
- PI ~4.5 % during contact, but false transients of **113 %, 88 %, 10 %** when
  HR=0 and SQI 0.30–0.43
- 93 accepted IBIs, all FPGA after priming; 9 prime, 2 low, 1 high, 1 gap
- RMSSD 50.5 ms, inflated by split pairs **888→645**, **781→641**, **666→797**
  (each about two cardiac cycles); removing those gives ~30 ms
- RR correctly 0 / RSA unresolved

**Capture 2 (after PI gate and RR-note fix):**

- HR 79.5–90.3 BPM, SpO₂ 97.8–99.2 %, SQI 0.88–0.91
- PI 1.1–3.1 %, no impossible transients
- 151 accepted IBIs, 158 FPGA / 1 software, 6 prime, 1 low, 1 high
- RMSSD **35.6 ms**; one residual >100 ms successive difference
- RR baseline path briefly published **7.2–8.0 br/min** with `RRDEPTH=0` and
  confidence 0.5–0.64, then returned to 0; the RSA path itself had `r=0.000`.

**Capture 3 (peak/4 crest bitstream `d9d58daa`, 2026-10-19 01:22):**

- 95 s, 115 IBI log rows: **113 ok, 1 gap, 1 low**, all FPGA (source 2)
- HR 69.8–78.6 BPM, SpO₂ 97.3–99.1 %, SQI 0.87–0.91, PI 0.6–1.8 %
- The old **888/645, 781/641, 666/797** split-pair signature is **absent**
- One **2380 ms gap** at t=126.1 s while status moved ACQUIRING→LOCKED and the
  reported AC jumped 847→4590: contact/motion, not a missing R peak
- Two short/long-looking pairs remain at the start (746/958 and 938/737); after
  beat 20 there are **zero pair-sum hits** by the 0.92/1.08 + 10 % rule
- Clean-tail RMSSD is **46.2 ms** over the last 53 ok intervals (49.2 ms from
  beat 20; 46.6 ms from beat 30); firmware showed 52.7 ms because its 300-beat
  ring still contained the startup gap and post-gap beats
- Max successive difference in the settled tail: 140 ms (856→716 ms)

**Interpretation:** the crest-threshold change removed the specific early-crest
pairs, but this capture is **not evidence that HRV is stable or accurate**. The
firmware's 300-beat RMSSD window holds startup motion for roughly 4 minutes at
75 BPM, and the settled RMSSD is still higher than capture 2. The next two
candidate fixes are: (a) timestamp the **true peak** rather than the threshold
crossing, which removes confirmation-latency jitter, and (b) a contact-settling
gate that excludes the first stable-lock transient from HRV. Neither is applied
yet. T4.1 ECG/chest-strap comparison remains required before quoting RMSSD.

**Capture 4 (5-minute still contact, 2026-10-19, COM4):**

- 302 s, 361 IBI rows: **318 ok**, 8 prime, 17 gap, 11 high, 7 low (all FPGA)
- HR 70.3–84.1 BPM (median 76.3); SpO₂ 94.0–99.0 % (median 96.6);
  SQI 0.42–0.92 (median 0.87), **94 % of frames ≥ 0.70**; PI 0.46–5.49 %
- **0 split-pair candidates** (consecutive ok IBIs at ratio 1.6–2.5): the old
  888/645 signature did not recur across 318 accepted intervals
- RMSSD published from t=20.7 s but **climbed 31.3 → 68.4 ms by t≈134 s → 77.5 ms
  at t≈202 s**, then settled 75.7–76.5 ms; last published **76.5 ms at t=293.8 s**
- From t≈294 s the contact degraded (PI 5–8 %, SpO₂ `CALC/--`, SQI 0.70→0.54,
  `FLAGS=0x40`); **RMSSD was withheld to 0.0 ms** — the 0.70 gate worked
- `[HISTORY] H0` printed **9×** with coherent running averages (HR 74.9–76.8,
  SpO₂ 96.4–98.1, RMSSD 35.9→67.0, NEWS2 0–1): first on-device confirmation that
  the hourly feed accumulates and publishes

Full numbers:
[`reports/HRV_STILL_CAPTURE_2026-10-19.md`](reports/HRV_STILL_CAPTURE_2026-10-19.md).

**Interpretation of Capture 4:** steady-state HR/SpO₂/SQI are internally
consistent and the split-peak artefact stayed gone, but the settled RMSSD
(≈76 ms) is **higher** than Capture 3's 46 ms clean tail and the value needs
~3 minutes to stabilise. This is **not** evidence of HRV accuracy. It strengthens
the case for a contact-settling gate (discard the first 1–2 minutes) and keeps
T4.1 (independent ECG/chest-strap comparison) as the blocking requirement before
quoting RMSSD. That gate is now implemented (2026-10-20, host-tested): RMSSD is
withheld until the window holds 240 of 300 beats *and* spans 120 s of contact. SpO₂ remains prototype-calibrated.

### Open decisions to record after the run

- **FPGA fallback window:** 2.5 s is current. The 02:49 capture had 3.38 s
  silences, so a longer 5 s window (or a true sticky detector) may keep HRV from
  re-priming repeatedly. Do not change it until the handover frequency is
  measured on the current build.
- **SQI gate:** keep `PPG_SQI_THRESHOLD_VALID` at 0.70. Do not lower it just to
  make the OLED publish RMSSD; a low-SQI RMSSD is not a measurement.
- **Clinical validation remains open:** this run verifies readiness and display
  logic only. T4.1 (independent ECG/chest-strap comparison) is still required
  before claiming RMSSD accuracy.
