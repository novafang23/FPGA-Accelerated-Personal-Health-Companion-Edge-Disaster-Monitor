# Codex Handover

## Project map

- `hardware/common/`: vendor-neutral moving-average and PPG peak-detection RTL shared by both FPGA paths.
- `hardware/zynq/`: AXI4-Lite accelerator and Zynq testbench; documented target is `xc7z020clg400-1` at 50 MHz.
- `hardware/shrikefi/`: SPI target and ForgeFPGA SLG47910 PPG design. `gen_flat_source.py` produces the flat vendor source from the maintained top and shared RTL.
- `firmware/core/`: platform-independent signal processing, clinical triage, risk models, pressure trend, and respiration.
- `firmware/shrikefi/`: ESP32-S3 application, sensor/UART/SPI drivers, dashboard service, GNSS, and fall detection. ESP-IDF component source lists are explicit in `main/CMakeLists.txt`.

## Target details

ESP32-S3 target: ESP-IDF 5.5.5, current defaults build at 160 MHz, 8 MB XMC flash. ShrikeFi SPI is mode 0; GPIO 12 SCK, 10 SS, 11 MOSI, 13 MISO. I²C uses GPIO 1 SDA and GPIO 2 SCL. Keep all board I/O at 3.3 V maximum. Current serial port observed: `COM4` (CH9102 USB serial).

## Repairs and current state

This session made MPU6050 setup fail closed, synchronized GNSS fix expiry, converted ESP32 signal paths to float, moved I²C inventory after sensor startup, and removed the false FPGA-liveness auto-reflash trigger. Existing user changes in the worktree were preserved. See [VERIFICATION_REPORT.md](VERIFICATION_REPORT.md) for commands, results, and hardware limits.

Software RTL, lint, host, evaluator, dashboard, generated-source, and ESP-IDF build checks passed. The current firmware was flashed and booted on the connected S3. A later physical run confirmed the OLED displays live vitals: serial reported stable HR/SpO2 with valid signal quality and FPGA crest/IBI events, and the user confirmed those values appeared on the OLED. This is a functional hardware demonstration, not clinical validation. Do not treat the MISO weak-pull check or the expected `0x00` probe response as proof of FPGA execution.

## OLED recovery follow-up (2026-10-15)

Added guarded I2C bus recovery, runtime OLED/MAX30102 retries, failure checking for the OLED's initial framebuffer transfer, and recovery after repeated MAX30102 I2C read failures. Latest ESP-IDF build and normal flash passed. Latest boot had SDA/SCL high; OLED retry initialized `0x3C` and the dashboard frame was acknowledged over I2C. The user later confirmed the visible OLED changed from its waiting prompt to live vitals; the serial capture showed HR around 74–83 BPM and valid SpO2 around 99.5–99.9%. A transient I2C dropout occurred during the capture and recovered; if it recurs, inspect shared bus power/wiring and capture driver error codes. See [VERIFICATION_REPORT.md](VERIFICATION_REPORT.md).

Next: if the OLED/sensor dropout recurs, capture the I2C driver error type and inspect the shared I2C connection and supply for intermittent contact.

## HRV follow-up

RMSSD excludes differences that bridge rejected/missing intervals. Live testing found HRV being published below SQI 0.70, plus a 669/1020 ms split-peak pair passing the old ±30% IBI gate and spiking RMSSD to 136.8 ms at SQI 0.89. HRV is now withheld below 0.70, and the IBI gate is ±15%; the host regression rejects that captured pair. The 1.5k optical-contact threshold also admitted ambient IR/red readings of 3.8–5.4k, so it is now 10k with a host regression. Strict host tests, focused engine tests, ESP-IDF build, and final COM4 flash passed. After one transient MAX30102/OLED I2C failure, the user's next live run recovered without reset: 32 seconds of finger data had SQI 0.81–0.91, valid SpO2, and telemetry RMSSD 40.8–43.2 ms; 359/1049 ms intervals were rejected without an RMSSD jump. This demonstrates stable filtering in this run, not clinical validity. See `VERIFICATION_REPORT.md`.

Next: if the shared I2C dropout recurs, inspect 3.3 V/GND and GPIO 1/2 connections; otherwise the highest-value remaining physical check is GNSS outdoors with an antenna and clear sky view.

## I2C startup follow-up (2026-10-15)

Recent serial logs show intermittent shared I2C failures. The final `0xF0F10` application was flashed and hash-verified on COM4. After reseating, OLED initialized in about 1.2 s and BME280 readings remained plausible, but MAX30102 and MPU6050 still produced repeated NACKs and recovered only after retries. The capture had no finger contact, so HR/SpO2 are unverified. Next: inspect shared SDA/SCL and sensor power/ground connections, then repeat the finger test. See the latest section in `VERIFICATION_REPORT.md`.

## Browser dashboard follow-up (2026-10-15)

There are two distinct dashboard clients: the standalone [USB/Web Serial page](firmware/shrikefi/dashboard/valor_dashboard.html), launched by `launch_web_dashboard.bat`, and the compact phone page embedded in [web_status.c](firmware/shrikefi/web_status.c), served at `http://192.168.4.1` after joining `VALOR-xxxx`. Fixed missing display of the optical finger-temperature estimate, stale telemetry/status handling, SoftAP stale-value display on request loss, and origin/CSP/CORS issues. Browser fixtures, strict host engine tests, and the ESP-IDF build passed. The board was disconnected, so these latest changes have not been flashed or exercised over live WiFi. See `VERIFICATION_REPORT.md`; reconnect the S3 and verify phone join, `/status.json`, and SOS trigger/cancel before calling the current SoftAP path physically verified.

## Useful commands

```powershell
python hardware/shrikefi/gen_flat_source.py --check
python firmware/shrikefi/dashboard/verify_dashboard.py
python firmware/shrikefi/dashboard/verify_softap_page.py
verilator --lint-only -Wall -Wno-UNUSED -Wno-UNOPTFLAT -Wno-SYNCASYNCNET --top-module axi_ppg_accelerator -Ihardware/common hardware/zynq/axi_ppg_accelerator.v hardware/common/ppg_peak_detector.v hardware/common/moving_average_8tap.v
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py build"
```

Highest-priority remaining physical check: test GNSS outdoors with an antenna and clear sky view.
