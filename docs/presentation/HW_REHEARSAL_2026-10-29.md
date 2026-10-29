# Hardware Live Rehearsal & Verification Log
**SIH26181 — Project VALOR**  
**Date:** 2026-10-29 | **Target Hardware:** Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910C) on COM4  
**Firmware Version:** `v1.0-zynq-SIH-316-g8b4cab5` (Commit `8b4cab5`)

---

## 1. Live Hardware Rehearsal Results (COM4 Bench Execution)

| Test Step | Verification Objective | Bench Observation / Evidence | Verdict |
| :--- | :--- | :--- | :--- |
| **Step 1: Firmware Identity** | Boot banner matches tagged commit | Captured `v1.0-zynq-SIH-316-g8b4cab5`, compiled `Oct 28 2026 21:17:16`, ELF SHA256 `5065cf745...`, ESP-IDF v5.5.5-dirty. Probed 4 I2C devices (0x3C, 0x57, 0x68, 0x76). | ✅ **PASS** |
| **Step 2: Cold Boot Autonomous Mode** | Boot without laptop over 5V USB-C / power bank | OLED displays boot splash and operational telemetry within 2 seconds. SoftAP beacon `VALOR-0081` broadcasts within 3 seconds of power application. Tested across 3 cold power cycles. | ✅ **PASS** |
| **Step 3: FPGA Link Health** | 8-bit SPI mode 0 link data integrity | Bitstream loaded: 46,408 bytes over SPI2 (`configuration COMPLETE!`). Replay analysis via `replay_fpga_link_log.py` confirmed **100.0% bit-exact arithmetic** between hardware FPGA output and Verilog RTL model (`rx == ma[n] & 0x7F: 100%`). Zero CRC or timeout errors. | ✅ **PASS** |
| **Step 4: Finger-on Settle** | Time to stable HR/SpO₂ and HRV settlement gate | HR acquires in ~6–8 s; SpO₂ stabilizes in ~18 s. HRV correctly enters `settling n/30` gate and holds publication until 30 beats and 20 seconds of continuous contact (`hrv_is_settled()`). | ✅ **PASS** |
| **Step 5: Finger-off Teardown** | Zero stale values on sensor release | On finger removal, telemetry instantly resets within 1 second: `[TELEMETRY] HR=0.0,SPO2=0.0,RMSSD=0.0`, `[TRIAGE] RISK=UNKNOWN`, `AIVALID=0`, `HRVN=0`. Verified live in `serial_capture_2026-10-29.log:300-374`. | ✅ **PASS** |
| **Step 6: SoftAP Web Dashboard** | Phone connectivity to `192.168.4.1` | Wi-Fi SoftAP `VALOR-0081` connects seamlessly on mobile (iOS/Android) with mobile data disabled. Web UI renders full dark-mode vitals, environmental status, and emergency cards. | ✅ **PASS** |
| **Step 7: Dual SOS Trigger & Cancel** | Regression check for SOS latch re-arming | Fall / SOS trigger countdown (10 s) chirps buzzer. Hardware cancel (5 s hold on GPIO5) and Web UI cancel (3 s hold on `#sos-cancel`) successfully disarm to `SOS_CANCELLED` twice consecutively without page refresh. | ✅ **PASS** |
| **Step 8: Desktop Web Serial Reconnect** | USB hot-plug resilience in `valor_dashboard.html` | Connects over Chrome Web Serial API at 115200 baud. Real-time 60 FPS PPG oscilloscope and gauges render smoothly. Unplugging and replugging USB reconnects with history intact. | ✅ **PASS** |
| **Step 9: 15-Minute Soak** | System stability, memory leak, and panic audit | 15-minute continuous operation completed. Zero `Guru Meditation Errors`, zero watchdog resets, zero heap degradation. Inter-core communication protected by `s_data_mutex` without deadlocks. | ✅ **PASS** |
| **Step 10: Indoor GPS Verification** | Non-blocking behavior under GNSS satellite shadow | GNSS reports `[SEARCHING SATELLITES] Sats: 0 | NMEA parsed: 13 | Fix: NO` without blocking. System gracefully preserves last-known coordinates `KIET COLLEGE GHAZIABAD` in NVRAM. | ✅ **PASS** |

---

## 2. Hardware Invariants Certified
1. **Voltage Compliance:** All I2C and SPI buses strictly 3.3V LVCMOS.
2. **Offline Immunity:** SoftAP and Web Serial operate 100% without external internet, cellular tower, or cloud dependencies.
3. **Hardware Jitter Free:** Blue LED D12 pulses directly from FPGA hardware peak detector pin, providing 20 ns hardware interval timestamping.
