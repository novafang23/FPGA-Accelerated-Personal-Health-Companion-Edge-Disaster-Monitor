# Verification Report — 2026-10-15

## Scope and worktree

The worktree was already dirty, with unrelated firmware, RTL, dashboard, documentation, and generated Vivado changes. Those changes were preserved. This report records checks run against that worktree and the targeted repairs made during this session; it does not claim a clean checkout or a GitHub CI run.

## Defects repaired

- MPU6050 initialization now checks reset and required configuration writes and only marks the sensor initialized after they succeed. Previously, failed writes could leave it marked ready with mismatched scale/configuration.
- GNSS fix expiry now updates the parser's shared validity and quality under its existing critical section. The application no longer reads the parser timestamp outside that synchronization. Tests cover timeout boundary, tick-counter wrap, fix loss, and recovery.
- ESP32 signal-processing paths no longer use software `double` for SpO₂, PPG SQI, or pressure regression. SpO₂ uses float Welford moments to avoid cancellation at large DC/small AC; pressure regression uses relative timestamps and wrap-safe ordering.
- The FPGA MISO pull test was described as proof of FPGA execution and could trigger repeated automatic bitstream reflashes. The test is now named and logged as a line-level heuristic, and it cannot trigger a reflash. A changing beat response after sensor contact is still required to verify FPGA processing.
- The I²C inventory ran before sensor initialization and reported no devices even though drivers later read them. It now runs after the sensor drivers probe their devices.

## Software baseline and final regression

The targeted baseline checks passed before these repairs. After the repairs, the following checks passed:

| Check | Result |
|---|---|
| Zynq Icarus RTL bench | 6 passed, 0 failed |
| Common peak-detector Icarus bench | 3/3 passed |
| ShrikeFi Icarus RTL bench | 10 passed, 0 failed |
| Verilator Zynq accelerator lint | Passed, no diagnostics |
| Verilator ShrikeFi ForgeFPGA lint | Passed, no diagnostics |
| Generated ForgeFPGA source check | Both generated files up to date |
| Hardware-connection HTML regeneration | Passed; Python emitted two `codecs.open()` deprecation warnings |
| Dashboard absent/healthy data check | Passed; absent data stays absent |
| Firmware engine unit suite | All tests passed, including SpO₂, pressure rollover, and RR checks |
| Fall-detection suite | 16/16 passed |
| GNSS parser suite | 12/12 passed, including fix expiry and rollover |
| Full ShrikeFi host application profiles | Profiles 1–12 passed |
| FP32/INT8 comparison harness | Completed expected normal, heat, pollution, and flood scenarios |
| MIMIC-III evaluator | 16,387 records; accuracy 94.11%, agreement 99.95%, sensitivity 58.18%, specificity 98.19% |
| ESP-IDF target build | Passed; ESP32-S3 app image 980,032 bytes, 87% of the 7 MiB app partition remains free |

The evaluator's 58.18% sensitivity is a material limitation for a triage or screening use case. Overall accuracy does not make this model clinically validated; labels are derived and the evaluation is retrospective.

Commands used for the final repeatable checks:

```powershell
iverilog -o $env:TEMP\valor_zynq.vvp hardware/zynq/tb_ppg_system.v hardware/zynq/axi_ppg_accelerator.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v
vvp $env:TEMP\valor_zynq.vvp
iverilog -o $env:TEMP\valor_common.vvp hardware/common/tb_ppg_peak_detector.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v
vvp $env:TEMP\valor_common.vvp
iverilog -o $env:TEMP\valor_shrikefi.vvp hardware/shrikefi/tb_forgefpga_system.v hardware/shrikefi/forgefpga_ppg_top.v hardware/shrikefi/spi_target.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v
vvp $env:TEMP\valor_shrikefi.vvp
python hardware/shrikefi/gen_flat_source.py --check
python firmware/shrikefi/dashboard/verify_dashboard.py
python docs/build_pdf.py
```

The two Verilator commands and all GCC suite commands are recorded in `.github/workflows/ci.yml`; the ESP-IDF command is:

```powershell
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py build"
```

The targeted GCC invocations used for the final engine, fall, GNSS, and host-app runs were:

```powershell
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/zynq/test_disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/hrv_analysis.c firmware/core/pm25_calibration_int8.c firmware/core/clinical_vitals_engine.c firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/disaster_risk_engine.c firmware/core/pressure_trend.c firmware/core/ppg_respiratory_rate.c firmware/shrikefi/sos.c firmware/shrikefi/location.c firmware/shrikefi/web_status.c -lm -o $env:TEMP\valor_test_engine.exe
& $env:TEMP\valor_test_engine.exe
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi firmware/shrikefi/test_fall_detect.c firmware/shrikefi/fall_detect.c -lm -o $env:TEMP\valor_test_fall.exe
& $env:TEMP\valor_test_fall.exe
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi firmware/shrikefi/test_gps_l89hs.c firmware/shrikefi/gnss_nmea.c firmware/shrikefi/gps_l89hs.c -lm -o $env:TEMP\valor_test_gnss.exe
& $env:TEMP\valor_test_gnss.exe
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi -Ifirmware/core firmware/shrikefi/main_shrikefi.c firmware/shrikefi/shrikefi_link_driver.c firmware/shrikefi/esp32_i2c_hal.c firmware/shrikefi/max30102.c firmware/shrikefi/bme280.c firmware/shrikefi/pms5003.c firmware/shrikefi/gnss_nmea.c firmware/shrikefi/gps_l89hs.c firmware/shrikefi/mpu6050.c firmware/shrikefi/fall_detect.c firmware/shrikefi/ssd1306.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/pm25_calibration_int8.c firmware/core/clinical_vitals_engine.c firmware/core/ppg_sqi.c firmware/core/ppg_respiratory_rate.c firmware/core/pressure_trend.c firmware/shrikefi/location.c firmware/shrikefi/sos.c firmware/shrikefi/web_status.c -lm -o $env:TEMP\valor_shrikefi_host.exe
& $env:TEMP\valor_shrikefi_host.exe
```

All four executions returned 0. Comparison and evaluator compile/run commands are the corresponding steps in `.github/workflows/ci.yml`.

## HRV sequence continuity follow-up (2026-10-15)

- Found a concrete RMSSD bookkeeping defect: the live IBI filter rejects bad intervals, but the HRV accumulator then treated the valid intervals on either side of a rejection as adjacent. That artificial pair can add a false successive difference to RMSSD.
- Added per-IBI adjacency markers and `hrv_break_sequence()`. Rejected/priming intervals, including software-fallback rejections before the shared pipeline, now break RMSSD adjacency; SDNN still uses accepted intervals. HRV readiness also requires the minimum number of valid adjacent pairs. Added regression coverage for rejected gaps and ring-buffer wrap.
- Focused engine tests compiled with GCC `-Wall -Wextra -Werror -pedantic -std=c11` and passed, including `test_hrv_rmssd_does_not_bridge_rejected_interval` and all existing engine tests.
- The complete ShrikeFi host simulation compiled with the same warning-as-error flags and passed its profiles, including the IBI pipeline profile.
- An initial `idf.py build` from the repository root failed because that directory has no `CMakeLists.txt`; rerunning from `firmware/shrikefi` succeeded with ESP-IDF 5.5.5. The initial HRV build was 0xEFDD0 bytes; after the software-fallback rejection fix, the final image was 0xEFDE0 bytes (87% free in the app partition).
- The initial HRV build was flashed to COM4; esptool verified the bootloader, app, and partition-table hashes. Its no-reset serial capture showed the app booted, but stayed in ACQUIRING with SQI about 0.39–0.64 and no usable IBI window; HR, SpO2, and RMSSD remained unavailable. This confirms boot only, not stable HRV with good finger contact.
- After the software-fallback rejection fix, the focused engine tests, complete ShrikeFi host simulation, and ESP-IDF build were rerun successfully. The final image was flashed to COM4; esptool verified hashes for the bootloader, app, and partition table and hard-reset the ESP32-S3. No full-chip erase or fuse operation was run. The final image itself was not captured over serial after this second flash. Monitor processes were stopped and COM4 released.
- Hardware follow-up: place a finger firmly and still on MAX30102, then capture `IBI ... verdict ... rmssd=...`, SQI, and displayed RMSSD after the signal is stable. If SQI stays low, correct contact/motion first; do not judge HRV from a low-quality capture.

### Live RMSSD follow-up (2026-10-15)

- A live fingertip capture exposed two additional problems. First, SQI stayed 0.36–0.46 while accepted intervals still accumulated into RMSSD. Added an SQI validity gate: HRV is cleared/withheld below 0.70, only reset at quality transitions, and the OLED asks the user to improve the PPG signal; HR and SpO2 keep their independent paths. Resetting on every low-SQI update would have prevented the interval-based SQI estimator from acquiring, so the implementation deliberately keeps collecting intervals for SQI while withholding them from published HRV.
- A second live capture had SQI 0.88–0.89 but accepted a 669 ms / 1020 ms split-peak pair against an 810 ms reference; RMSSD jumped to 136.8 ms. Tightened the local IBI acceptance window from 0.70–1.30 to 0.85–1.15 of the rolling median. Added a host regression reproducing that measured pair; both intervals are rejected, the HRV sample count remains unchanged, and RMSSD stays 0.0 ms in the stable baseline sequence.
- Rebuilt the full ShrikeFi host simulation with `gcc -Wall -Wextra -Werror -pedantic -std=c11`; all profiles passed, including the new split-pair regression. The focused engine suite also passed, including rejected-gap RMSSD and ring rollover coverage.
- ESP-IDF 5.5.5 `idf.py build` passed; final app image `0xEFEF0` bytes, 87% free in the app partition. The final image was flashed to COM4; esptool verified bootloader, app, and partition-table hashes, and the board hard-reset. No full-chip erase or fuse operation was run.
- Intermediate live capture on the prior image had raw IR about 180,000, HR around 72–94 BPM, SpO2 about 99.4–99.7%, SQI 0.84–0.89, and FPGA IBI events. It reproduced the 136.8 ms RMSSD spike, which motivated the tighter interval filter.
- The post-final-flash capture did not reproduce good finger contact: raw IR stayed about 3,800–4,400, SQI 0.30–0.63, HR remained 0, SpO2 was unavailable, and RMSSD stayed 0.0. FPGA events were logged but produced gaps; this run does not prove the final interval filter on hardware. OLED framebuffer writes initially failed, then runtime retry initialized `0x3C` and transferred a frame successfully over I2C. Visible panel output was not directly observed in this final capture.
- Remaining hardware step: place a fingertip firmly on MAX30102 and capture at least 20–30 seconds with raw IR in the tissue-contact range, valid SpO2, SQI >=0.70, and IBI verdicts. Verify that 669/1020-like intervals are rejected and RMSSD no longer spikes. Until then, software regression and build are verified; final physical HRV behavior is not.
- A further no-reset capture after the final contact-threshold build showed an intermittent shared-I2C failure: MAX30102 Part ID reads returned `0xFFFFFFFF` on repeated retries, OLED initialization remained unavailable, and telemetry stayed `NO_FINGER`. The final firmware is built and flashed, but this boot does not prove sensor/OLED operation or the contact threshold on hardware. The UART capture was finite and closed cleanly. Check the shared 3.3 V/GND supply and SDA/SCL connections (GPIO 1/2) before the next live run.
- **Final user-assisted live capture (finger held on MAX30102, same flashed image):** the sensor recovered without a reset. Over the 32-second UART capture, raw IR stayed about 181,000–185,000, status remained `LOCKED`, HR was about 72–82 BPM, SpO2 was valid at about 97.5–100.0%, and SQI rose from 0.81 to 0.91. The RMSSD telemetry stayed about 40.8–43.2 ms; accepted-window diagnostics stayed near 40–43 ms. Short/long intervals were marked `low`/`high` (for example 359 ms and 1049 ms) without changing RMSSD, while regular intervals were accepted. This supports that the final contact threshold and HRV filtering ran with real finger data and that the earlier large RMSSD spike did not recur in this capture. The UART capture closed cleanly. This is a short demonstration, not clinical validation; OLED visibility was not separately observed during this particular capture (the user had previously confirmed the live OLED output).
- Capture method: PowerShell/.NET `SerialPort` on `COM4` at 115200 baud with DTR/RTS disabled; read for 32 seconds and filter for I2C/OLED/MAX30102/IBI/telemetry/FPGA logs. The script exited normally and closed COM4. Reproduce with the 32-second serial capture procedure used during this session; do not use a monitor option that resets the board when checking the current running state.

## Physical hardware validation

- Board on `COM4` enumerated as USB-Enhanced-SERIAL CH9102. A non-erasing chip query identified ESP32-S3 revision v0.2 with 8 MB XMC flash.
- `idf.py -p COM4 flash` completed, verified image hashes, and reset the board. No full-chip erase command, eFuse operation, or security-fuse change was run.
- Serial boot showed the app starting at 160 MHz, the 46,408-byte ForgeFPGA image streamed, and the runtime SPI2 link configured for mode 0 at 1 MHz on GPIO 10/11/12/13.
- The `0x55` SPI probe returned `0x00`; the RTL protocol makes this expected and it is not a handshake. The weak-pull MISO result was stable, which does not prove FPGA execution.
- The post-initialization I²C scan found `0x3C` OLED, `0x57` MAX30102, `0x68` MPU6050, and `0x76` BME280. MAX30102 FIFO reads produced changing raw values on serial.
- During the captured session there was no finger contact: HR and SpO₂ remained unavailable, zero FPGA beat events were observed, and the FPGA peak-detection path was not proven end to end. No actual GNSS fix or end-user web-page request was verified. A successful flash and boot are not a successful clinical measurement.

## Remaining limits and risks

- Put a finger on MAX30102 and capture a real beat event, stable HR/SpO₂, and corresponding web-status response before claiming product-level PPG verification.
- Validate GNSS with an antenna and clear sky view; the earlier boot log showed no UART data / zero satellites.
- Confirm the sensor values and fall-detection behavior with known stimuli and attached peripherals. An I²C ACK scan alone does not prove sensor calibration or measurement correctness.
- The SoftAP is open and exposes health/status and emergency routes to clients that join; this is documented as a rescue-access tradeoff, but remains a privacy/control risk.
- ESP-IDF reported that the project uses the legacy I²C driver API. Target build and sensor access worked, but a migration is future maintenance work.
- Host tests use no-op synchronization shims and do not prove FreeRTOS multicore race freedom. No Vivado synthesis was run; the Zynq baseline figures remain the repository's documented historical baseline. No ForgeFPGA vendor fitter/timing run was performed.

## Reproduction

From the repository root, run the RTL/lint, generated-source, dashboard, firmware unit, comparison, and accuracy steps in `.github/workflows/ci.yml`. To rebuild and flash the attached ESP32-S3 (expected port in this session: `COM4`):

```powershell
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py build"
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py -p COM4 flash"
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py -p COM4 monitor"
```

Flashing writes the selected firmware and normal flash sectors it occupies; do not substitute a full-flash erase command unless separately intended.

## OLED/I2C recovery follow-up (2026-10-15)

- After attaching the serial monitor to the running board, the ESP32 restarted. The subsequent boot reported no I2C acknowledgements from any sensor. I later found that my first monitor process had remained open; it was stopped. Subsequent serial captures used `--no-reset`.
- Added startup I2C line inspection and a guarded nine-clock bus-clear attempt when SDA is low and SCL is released. If SCL is low, firmware logs the condition and does not drive against it. Added low-rate OLED and MAX30102 initialization retries; OLED initialization now fails if the initial framebuffer transfer fails.
- `idf.py build` passed. `idf.py -p COM4 flash` wrote and hash-verified the normal bootloader, app, and partition-table ranges; no full-chip erase or eFuse operation was run.
- The post-flash boot reported SDA=1/SCL=0 on GPIO 1/2 before I2C controller initialization, then no devices at the initial scan. This is direct evidence the bus was not usable at startup; powered sensor LEDs alone do not confirm I2C communication.
- Runtime retries later produced real-looking MAX30102 FIFO values (for example IR=720, Red=924) and `ACQUIRING` status. Those values are below the firmware's contact threshold, so no HR/SpO2 was verified. Repeated OLED retry warnings stopped by about 102 seconds uptime, which suggests initialization may have recovered, but no direct visual confirmation of the panel was captured.
- At this capture point, physical OLED confirmation and a finger-on-sensor HR/SpO2 run remained outstanding; the later live run is recorded below.

### Latest deployment and runtime capture (2026-10-15)

- Rebuilt the current recovery changes with ESP-IDF 5.5.5: `idf.py build` passed; app image size was `0xEFCE0` bytes (87% free in the 7 MiB app partition).
- `idf.py -p COM4 flash` completed and verified hashes for the bootloader, app, and partition table. It wrote normal firmware sectors only; no full-chip erase or eFuse operation was run.
- Captured startup using `idf.py -p COM4 monitor --no-reset`, then closed it with Ctrl+]. The current boot reported I2C SDA=1/SCL=1 before controller init (the earlier SCL-low observation was from a previous boot and is not the current line state).
- Runtime OLED retry initialized address `0x3C`; the dashboard framebuffer was then transferred successfully over I2C. This proves the MCU received I2C acknowledgements for initialization and framebuffer writes, but it does not prove visible light from the physical panel.
- During the capture the PPG raw values were mostly about 700–1,200, below the firmware's 1,500 contact threshold. Serial status remained `NO FINGER`, HR `0`, and SpO2 unavailable. Sensor LEDs/power and these raw readings do not establish finger contact or live vitals.
- The monitor process was closed and a process query found no remaining `idf.py`/Python monitor or flash process using COM4.
- During a later no-reset capture, a transient I2C loss was visible: OLED retries failed and MAX30102 returned Part ID `0xFF` for several retries. MAX30102 subsequently reinitialized without a reset and produced continuous samples. This was intermittent, not a stable sensor-path failure.
- With the user's fingertip placed on MAX30102, the live capture reached `LOCKED`: HR stabilized around 74–83 BPM, SpO2 was valid around 99.5–99.9%, SQI around 0.87–0.89, and IBI count reached 80+. Telemetry and FPGA systolic-crest/IBI events were present. The user visually confirmed the OLED changed from its waiting prompt to live vitals. This verifies the live display and sensor path on the attached hardware; it is not clinical validation.
- The no-reset monitor was closed with Ctrl+]; a process query found no remaining monitor/flash process holding COM4.

## Dashboard audit and disconnected-board follow-up (2026-10-15)

The project has two separate dashboard clients. `firmware/shrikefi/dashboard/valor_dashboard.html` is the richer standalone/USB Web Serial page opened with `launch_web_dashboard.bat`; its live mode reads telemetry from a browser serial connection. The phone page at `http://192.168.4.1` after joining `VALOR-xxxx` is a separate compact document embedded in `firmware/shrikefi/web_status.c`, served by the ESP32-S3 SoftAP and polling `/status.json`. Neither page needs external assets or a CDN.

Fixed defects in this follow-up:

- The standalone page parsed MAX30102 `SKINTEMP` telemetry but omitted it from the UI. It now displays a separately labeled optical finger-temperature estimate and clears it on `NO_FINGER`; firmware includes `skin_temp` in status JSON when contact makes a reading available. The label states that it is not a clinical thermometer.
- Standalone live mode measured freshness against any serial line, so console output could hide a lost telemetry stream. It now expires live readings after three seconds without telemetry, clears stale values/risk/trace, and recovers when telemetry resumes.
- SoftAP poll failures previously left the last vitals visible. Failed HTTP responses or fetches now clear current readings and show `NO CONNECTION`.
- Removed wildcard CORS from history responses; hardened same-origin validation against lookalike hosts; added script-hash CSP and response security headers; removed inline event handlers from the embedded page. The open SoftAP remains unauthenticated by design, so origin checks limit browser drive-by actions and do not protect against a client that joins the AP.
- Updated the offline dashboard audit, expanded browser-state checks, and corrected dashboard documentation to describe both clients accurately.

Software checks actually run for this follow-up:

| Check | Result |
|---|---|
| `python -m py_compile firmware/shrikefi/dashboard/verify_dashboard.py firmware/shrikefi/dashboard/verify_softap_page.py` | Passed |
| `python firmware/shrikefi/dashboard/verify_dashboard.py` | Passed in headless Chrome: empty state, live values, `NO_FINGER` temperature clearing, and stale telemetry expiration |
| `python firmware/shrikefi/dashboard/verify_softap_page.py` | Passed in headless Chrome using the extracted embedded HTML and a local HTTP fixture: live values including negative/zero environment readings, SOS click feedback, no-finger clearing, and fetch-loss clearing |
| Strict GCC firmware engine suite (`-Wall -Wextra -Werror -pedantic -std=c11`) | Passed, including status JSON temperature and origin-boundary regressions plus existing engine tests |
| ESP-IDF 5.5.5 `idf.py build` from `firmware/shrikefi` | Passed; image `0xF0810` bytes, 87% free in the 7 MiB application partition |
| `git diff --check` | Passed; Git printed existing working-copy LF/CRLF normalization notices |

The dashboard verifier commands are:

```powershell
python -m py_compile firmware/shrikefi/dashboard/verify_dashboard.py firmware/shrikefi/dashboard/verify_softap_page.py
python firmware/shrikefi/dashboard/verify_dashboard.py
python firmware/shrikefi/dashboard/verify_softap_page.py
```

The strict engine suite used this warning-as-error GCC command and then ran the resulting executable:

```powershell
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/zynq/test_disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/hrv_analysis.c firmware/core/pm25_calibration_int8.c firmware/core/clinical_vitals_engine.c firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/disaster_risk_engine.c firmware/core/pressure_trend.c firmware/core/ppg_respiratory_rate.c firmware/shrikefi/sos.c firmware/shrikefi/location.c firmware/shrikefi/web_status.c -lm -o $env:TEMP\valor_dashboard_engine.exe
& $env:TEMP\valor_dashboard_engine.exe
```

The final target build command was run from `firmware/shrikefi`:

```powershell
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py build"
```

The board was disconnected for this audit. Therefore this follow-up verifies the extracted SoftAP page and server-side formatter/origin logic in software fixtures and compiles the firmware, but does **not** verify that the physical AP starts, that a phone joins, or that live `/status.json` and SOS POST requests work over WiFi. No flash or hardware test was performed as part of this dashboard follow-up. The prior 2026-09-28 hardware page test recorded above is historical evidence for that earlier firmware state, not proof of these newer changes. Next physical check: reconnect the S3, flash the current build, join `VALOR-xxxx` from a phone, verify live `/status.json` and stale-link behavior, then exercise SOS trigger/cancel and confirm the displayed state.

## I2C startup and sensor dropout follow-up (2026-10-15)

Two serial captures after the previous 100 kHz bus change showed real I2C failures. At 400 kHz, OLED initialization failed, MAX30102 Part ID reads repeatedly returned an I2C error, and BME280/MPU6050 initialization failed; only ACKs from `0x57` and `0x76` appeared in a later address scan. At 100 kHz, the boot scan initially found no responding devices and MAX30102 register reads reported `ESP_ERR_TIMEOUT`; MAX30102 recovered at about 22 seconds and OLED at about 27 seconds. The no-finger capture kept LED current at its starting value, so it does not support adaptive LED current as the cause of the observed dim red emitter. These results point to an intermittent shared bus/power/contact problem, but do not identify which physical connection is faulty.

Software changes made:

- Set the shared I2C bus to 100 kHz, added ESP-IDF error names for failed register reads/writes, and made MAX30102 register-read failure explicit in its log.
- Added periodic BME280 and MPU6050 initialization retries. Repeated BME280/MPU6050 read failures now invalidate their initialized state and retry instead of leaving failed sensors permanently offline.
- Removed fabricated BME280 startup values (25 C / 50% RH). Missing ambient data is now shown as unavailable on the OLED, standalone dashboard, and SoftAP JSON; it is excluded from humidity calibration and TinyML ambient assessment. Heat risk becomes UNKNOWN when ambient data is invalid, and a normal overall risk is changed to UNKNOWN while ambient assessment is unavailable.
- Added browser and host regressions for missing ambient readings and JSON `null` output.

Checks run for these changes:

| Check | Result |
|---|---|
| Strict ShrikeFi host build and run (`gcc -Wall -Wextra -Werror -pedantic -std=c11`, all firmware/core sources) | Passed; `ShrikeFi Host Test Completed Successfully` |
| `python firmware/shrikefi/dashboard/verify_dashboard.py` | Passed; empty/live/no-finger/missing-sensor/stale browser cases |
| `python firmware/shrikefi/dashboard/verify_softap_page.py` | Passed; live values, SOS feedback, no-finger, and link-loss cases |
| Strict risk/status engine build and run (`-Wall -Wextra -Werror -pedantic -std=c11`) | Passed; `ALL TESTS PASSED` |
| ESP-IDF 5.5.5 `idf.py build` from `firmware/shrikefi` | Passed; image `0xF0F10` bytes, 87% free in the 7 MiB application partition |
| `git diff --check` | Passed; Git reported existing LF/CRLF normalization notices |

The ShrikeFi host check used this strict GCC command and then ran the executable from the repository root:

```powershell
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi -Ifirmware/core firmware/shrikefi/main_shrikefi.c firmware/shrikefi/shrikefi_link_driver.c firmware/shrikefi/esp32_i2c_hal.c firmware/shrikefi/max30102.c firmware/shrikefi/bme280.c firmware/shrikefi/pms5003.c firmware/shrikefi/gnss_nmea.c firmware/shrikefi/gps_l89hs.c firmware/shrikefi/mpu6050.c firmware/shrikefi/fall_detect.c firmware/shrikefi/ssd1306.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/pm25_calibration_int8.c firmware/core/clinical_vitals_engine.c firmware/core/ppg_sqi.c firmware/core/ppg_respiratory_rate.c firmware/core/pressure_trend.c firmware/shrikefi/location.c firmware/shrikefi/sos.c firmware/shrikefi/web_status.c -lm -o $env:TEMP\valor_i2c_host.exe
& $env:TEMP\valor_i2c_host.exe
```

At this point the final `0xF0F10` image had not yet been flashed; the subsequent reconnection and flash results are recorded below.

### Reconnection and final image on hardware

After the user reseated the shared connections, the board appeared on COM4. A 30 s 115200-baud capture with DTR/RTS disabled showed all four I2C addresses in the boot scan. The OLED initialized after a runtime retry at about 1.2 s and its framebuffer transfer was acknowledged. BME280 initialized and continued reporting plausible ambient values near 29.2 C / 66% RH. MAX30102 initialized after retry at about 1 s, but then repeatedly NACKed register/FIFO reads (`ESP_FAIL`); that pre-flash capture counted 116 failed reads at address `0x57`, 35 at `0x68`, and one at `0x3C`. MPU6050 was initialized at boot in this capture. The OLED was responsive quickly, but MAX30102 and MPU6050 bus failures remained. HR/SpO2 were not assessed because the sensor reported `NO FINGER` throughout.

Flashed the built `0xF0F10` application to COM4 using the normal ESP-IDF flash target. Bootloader, app, and partition-table hashes verified; esptool hard-reset the ESP32-S3. No erase-all or fuse operation was used. A 30 s post-flash serial capture began after early boot output; it counted 65 NACKs at `0x57` and 9 at `0x68`, showed MAX30102 retry initialization several times, and showed MPU6050 recovery after a five-read failure streak. It confirmed LED current register values `led_r=79 led_ir=79` during no-finger diagnostics, but does not establish how the LEDs behave under fingertip contact. The post-flash capture had no finger contact, so live HR/SpO2 and visible OLED pixels were not verified in this session.

Updated the hardware connection table and project guide from 400 kHz to the actual configured 100 kHz I2C bus rate. The next hardware action is to inspect MAX30102 and MPU6050 SDA/SCL/VCC/GND connections and the 3.3 V rail under load. If failures persist, a close-up wiring photo and a 3.3 V/GND measurement at the sensor breakout will help distinguish a loose/shared-rail issue from a sensor/module fault. Keep all bus pins at 3.3 V maximum.
