# VALOR — Complete Project Context (A→Z)

**Read this first. It is the single source of truth for the project.**
Everything an agent or engineer needs to work on this repo without reading the
whole history is here: hardware, pin maps, connector pinouts, firmware
architecture, algorithms, published numbers, build/flash/test procedures, API
contracts, and the honest list of what is *not* verified.

| | |
|---|---|
| Project | SIH26181 — **VALOR** (Vital and Atmospheric Logic for Offline Rescue) |
| Repo | `FPGA-Accelerated-Personal-Health-Companion-Edge-Disaster-Monitor` |
| Branch | `main` |
| What it is | Offline, wearable, multi-hazard health + environment monitor for first responders, evacuees, industrial workers |
| Status | Bench prototype working end-to-end. Wearable PCB is **design intent only** (not fabricated) |
| Regulatory | **Not a medical device.** Research / competition prototype |

---

## 1. Architecture in one picture

```
        MAX30102 (PPG, I2C 0x57)          TMP117 (skin, I2C 0x48)
        BME280   (env, I2C 0x76)          MPU6050 (IMU, I2C 0x68)
        PMS5003  (PM2.5, UART1)           GNSS    (NMEA, UART2)
                    │                            │
                    ▼                            ▼
   ┌──────────────────────────────┐   ┌───────────────────────────────┐
   │ ESP32-S3 Core 0              │   │ ESP32-S3 Core 1               │
   │  • PPG streaming @100 Hz     │   │  • env/IMU/GNSS/PM polling    │
   │  • SQI + motion gating       │   │  • mNEWS2 triage              │
   │  • IBI pipeline / HR / HRV   │   │  • INT8 TinyML risk model     │
   └───────────┬──────────────────┘   │  • OLED UI                    │
               │ SPI2 @1 MHz          │  • SoftAP HTTP server         │
               ▼                      └───────────────────────────────┘
   ┌──────────────────────────────┐                  ▲
   │ Renesas ForgeFPGA SLG47910C  │                  │ mutex-protected
   │  • 8-tap running-sum MAF     │                  │ shared state
   │  • 4-state systolic peak det │──────────────────┘
   │  • crest timestamp latch     │
   │  1120 LUT5, 50 MHz internal  │
   └──────────────────────────────┘
```

**Why the FPGA exists:** FreeRTOS tick jitter (5 ms at `CONFIG_FREERTOS_HZ=200`)
plus Wi-Fi/flash-cache interrupts make software peak detection non-deterministic.
The FPGA detects systolic crests in hardware so the *quantisation* is fixed at the
sample period. It does **not** give sub-sample timing — see §7.2.

---

## 2. Hardware

### 2.1 Boards

| Role | Part | Notes |
|---|---|---|
| Active target | Vicharak **ShrikeFi** — ESP32-S3 + Renesas ForgeFPGA SLG47910C | The board everything runs on today |
| Verified baseline | Xilinx **Zynq-7000** `xc7z020clg400-1` | RTL parity target, Vivado ML 2022.2 OOC synthesis |
| MCU module | **ESP32-S3-WROOM-1-N8R2** | 8 MB flash, 2 MB PSRAM, PCB antenna, 25.5×18.0 mm |
| FPGA | **SLG47910C** | 1120 5-input LUTs, 140 CLBs, 50 MHz internal ring oscillator. Repo records the package as **WLCSP-20** (0.4 mm pitch) — verify before designing hardware |

### 2.2 Voltage rules (non-negotiable)

- ESP32-S3 and ForgeFPGA I/O are **3.3 V only**. Never apply 5 V to a GPIO, SPI
  or I2C line.
- 5 V exists **only** on USB VBUS and the PMS5003 supply pin.
- All FPGA I/O is 3.3 V maximum; exceeding it damages the part.

### 2.3 ESP32-S3 pin map

Authoritative source: `firmware/shrikefi/shrikefi_pinmap.h`. This table must never
diverge from that header.

| GPIO | Net | Direction | Connects to | Function |
|---|---|---|---|---|
| 1 | `I2C_SDA` | bidi | all I2C devices | shared serial data |
| 2 | `I2C_SCL` | out | all I2C devices | 100 kHz clock |
| 4 | `BUZZER_OUT` | out | buzzer driver FET gate | alarm |
| 5 | `SOS_CANCEL` | in | SOS button to GND | active-low, internal pull-up |
| 8 | `FPGA_EN` | out | FPGA enable/reset | boot-mode latch |
| 9 | `FPGA_PWR` | out | FPGA power switch | boot-mode latch |
| 10 | `SPI_SS_N` | out | FPGA `PIN_17` | active-low chip select |
| 11 | `SPI_MOSI` | out | FPGA `PIN_18` | MCU → FPGA samples |
| 12 | `SPI_SCK` | out | FPGA `PIN_16` | 1 MHz runtime / 16 MHz flash |
| 13 | `SPI_MISO` | in | FPGA `PIN_19` | `{beat, filtered[6:0]}` + IBI |
| 14 | `PMS_RX` | in | PMS5003 `TX` (pin 5) | UART1 RX, 9600 8N1 |
| 16 | `GPS_RX` | in | GNSS `TXD` | UART2 RX, 9600 8N1 |
| 17 | `GPS_TX` | out | GNSS `RXD` | UART2 TX |
| 18 | `PMS_TX` | out | PMS5003 `RX` (pin 4) | UART1 TX (optional) |
| 19 / 20 | `USB_D-` / `USB_D+` | bidi | USB-C | native USB JTAG/CDC |
| EN | `MCU_RESET` | in | reset button + RC | 10 kΩ pull-up, 1 µF to GND |

⚠️ **Note on EN/PWR naming:** Vicharak's documentation lists `EN=GPIO9, PWR=GPIO8`;
this repo uses the reverse names (`PIN_FPGA_PWR 9`, `PIN_FPGA_EN 8`). The header
documents this explicitly. Verify against your own wiring before flashing.

⚠️ **Strapping pins:** GPIO 0, 3, 45, 46 are sampled at reset. Do not attach
sensors, pull-ups or test loads to them.

⚠️ `PIN_MAX30102_INT` is **-1**: the MAX30102 interrupt is not routed. The driver
still writes `INT_ENABLE_1 = 0x40` (FIFO almost-full) but firmware polls.

### 2.4 I2C bus

| Address | Device | Notes |
|---|---|---|
| `0x3C` | SSD1306 OLED 128×64 | alt `0x3D` if ADDR high |
| `0x48` | TMP117 clinical skin temperature | `ADD0`→GND; ±0.1 °C class, 1/128 °C LSB |
| `0x57` | MAX30102 PPG (Red + IR) | part ID `0x15` |
| `0x68` | MPU6050 6-axis IMU | `AD0`→GND |
| `0x76` | BME280 env (T/P/RH) | `SDO`→GND |

Bus runs at **100 kHz** on GPIO1/2. Pull-ups 3.3 kΩ to 3.3 V.
MAX30102 core supply is **1.8 V** with `VLED+` at 3.3 V — a bare-IC design needs a
1.8 V rail plus an I2C level translator (see the PCB guide).

### 2.5 ForgeFPGA SPI link

| Signal | MCU | FPGA pin | Notes |
|---|---|---|---|
| SCK | GPIO12 | `PIN_16` | 1 MHz runtime, 16 MHz during bitstream load |
| SS_n | GPIO10 | `PIN_17` | driven manually per transaction |
| MOSI | GPIO11 | `PIN_18` | 8-bit optical sample |
| MISO | GPIO13 | `PIN_19` | bit7 = beat, bits[6:0] = filtered sample |
| Beat LED | — | `PIN_7` (`led_user`) | flashes on every crest |
| OSC_EN | — | tied high | runs from internal 50 MHz oscillator |

SPI mode 0. The bitstream is loaded over **this same bus** at every boot.
`docs/reference/shrike_board/Web_FPGA_programmer.ino` is the original Vicharak
reference sequence the firmware mirrors.

### 2.6 External connector pinouts

**PMS5003 — 8-pin 1.25 mm JST-GH.** Logic is 3.3 V; fan and laser need **5 V**.

| Pin | Function | Connect to |
|---|---|---|
| 1 | **VCC (5 V)** | 5 V / VBUS |
| 2 | **GND** | GND |
| 3 | `SET` mode select | 3.3 V or floating (normal run) |
| 4 | `RX` | ESP32 GPIO18 (optional) |
| 5 | `TX` | ESP32 GPIO14 |
| 6 | `RESET` | leave open (low = reset) |
| 7, 8 | NC | leave open |

9600 8N1. Measure 5 V between **pin 1 and pin 2** (expect ~4.9–5.1 V under load).

**GNSS (u-blox NEO-M8N breakout; Quectel L89-HS compatible):** `VCC`→3.3 V,
`GND`, `TXD`→GPIO16, `RXD`→GPIO17, RF→U.FL or ceramic antenna. NMEA-0183 at 9600.

**OLED:** 4-pin I2C module (GND, VCC, SCL, SDA). A bare 14-pin panel needs an
SSD1306 die plus charge pump and is a separate design.

### 2.7 Power (bench prototype)

Single-cell LiPo → MCP73831 charger → DW01A + FS8205A protection →
AP2112K-3.3 LDO → 3.3 V rail. USB-C 5 V feeds the charger and the PM sensor.
Partition offsets and NVS survive reflashing, so Wi-Fi credentials persist.

### 2.8 ESP32-S3 configuration (`firmware/shrikefi/sdkconfig.defaults`)

```
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=160
CONFIG_FREERTOS_HZ=200
CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1=y
CONFIG_PARTITION_TABLE_CUSTOM=y   (partitions.csv)
```
`sdkconfig` itself is **not tracked** (correct IDF practice); a fresh clone
regenerates it from `sdkconfig.defaults`, which carries `CONFIG_IDF_TARGET="esp32s3"`.

Partition table: `nvs 0x9000/0x6000`, `phy_init 0xF000/0x1000`,
`factory 0x10000/0x700000` (7 MB app partition on 8 MB flash).

---

## 3. FPGA design

### 3.1 RTL

| File | Contents |
|---|---|
| `hardware/common/moving_average_8tap.v` | 8-tap running-sum boxcar, O(1), zero DSP |
| `hardware/common/ppg_peak_detector.v` | 4-state systolic crest FSM + refractory + crest timestamp |
| `hardware/shrikefi/forgefpga_ppg_top.v` | ShrikeFi top (vendor flow compiles the **inlined copy** of the shared sources) |
| `hardware/zynq/axi_ppg_accelerator.v` | AXI4-Lite wrapper for the Zynq baseline |
| `hardware/shrikefi/spi_target.v` | SPI slave + `0xC3` IBI burst serializer |

⚠️ **Duplication trap:** the vendor flow compiles the inlined RTL inside
`forgefpga_ppg_top.v`, *not* `hardware/common/*.v`. Editing the common file alone
does not change the bitstream. Both copies carry warning headers; keep them in
sync (or the bitstream silently describes different logic).

### 3.2 Crest detection rules

- Arm requires `sample_in >= 120` (samples are centred on 120).
- `crest_fall_min = (peak_val > 48) ? (peak_val >> 2) : 6` → a 25 % drop from the
  running peak. Because arming forces `peak_val >= 120`, the 6-count floor is
  inactive in this build.
- `first_beat_seen` suppresses the first crest after reset so no spurious interval
  is emitted.
- The 2026-11-02 change latches the **crest timestamp** (32-bit cycle counter
  sampled at the crest), which is why FF count jumped by 64.

### 3.3 Published fit (2026-11-02, current)

| Resource | Value |
|---|---|
| CLB LUT5s | **533 / 1120 (47.59 %)** |
| FFs | **367** (362 CLB + 5 IOB) |
| CLBs | **118 / 140 (84.29 %)** — 22 free, **the binding constraint** |
| BRAMs / PLLs | 0 / 8, 0 / 1 (uses internal OSC) |
| Timing @ 50 MHz | **WNS +7.131 ns**, TNS 0, 0 failing endpoints of 1664 |
| Achievable Fmax | **77.712 MHz** |

Evidence: `hardware/shrikefi/synthesis_evidence/` (read its README — it tabulates
every past fit so nobody quotes the wrong one). History: 342 → 363 → 362 → 535 →
515 → 533 LUT5s.

**CLB occupancy, not LUTs, is the real limit.** The crest-timestamp change cost 13
CLBs for only 18 LUT5s. Assume any datapath widening needs a fitter run before you
claim it fits.

### 3.4 Zynq baseline (verified, do not misstate)

`xc7z020clg400-1`, Vivado ML 2022.2 OOC: **185 LUT, 16 LUTRAM, 266 FF, 0 DSP48,
0 BRAM**, WNS +5.603 ns, WHS +0.184 ns, **Fmax 69.45 MHz** against a 50 MHz
target. Testbench `hardware/zynq/tb_ppg_system.v` — 6/6 self-checking tests.

### 3.5 Boot / configuration sequence

1. Drive `PWR`/`EN` through the boot-mode latch.
2. Stream `FPGA_bitstream_MCU.bin` (**46,408 bytes**) over SPI2 at 16 MHz in
   256-byte chunks. Nothing is read back, so this call cannot confirm delivery and
   failure is non-fatal.
3. Runtime probe: send `0x55`, expect `0x00`. **`received 0x00` is the expected,
   successful result** — not a failure. The proof the FPGA is alive is that it
   drives MISO at all instead of leaving it floating.
4. Regenerate derived artefacts with:
   `python hardware/shrikefi/gen_flat_source.py --check` and
   `python hardware/shrikefi/convert_bitstream.py --check`. Both must pass before
   a fit or a flash is meaningful.

---

## 4. Firmware

### 4.1 Task / core map

| Task | Core | Prio | Stack | Job |
|---|---|---|---|---|
| `PPG_Accel` | 0 | 5 | 7168 | 100 Hz sample stream, FPGA SPI, SQI, IBI, HR/HRV |
| `Risk_Monitor` | 1 | 2 | 10240 | clinical engine, TinyML, status publish, OLED |
| `PMS5003_UART` | 1 | 3 | 4096 | PM frames |
| `GPS_UART` | 1 | 3 | 4096 | NMEA |
| `IMU_Monitor` | 1 | 3 | 4096 | motion / fall |
| `CAL_Console` | 1 | 1 | 4096 | serial calibration commands |
| HTTP server | 1 | — | — | SoftAP dashboard + JSON |

Wi-Fi is pinned to Core 1 specifically to protect Core 0's PPG timing.

### 4.2 Source layout

```
firmware/core/           portable logic (also compiled on the host)
  disaster_risk_engine.c   rule engine, NEWS2, Moran PSI, PM2.5 strain, cyclone
  clinical_vitals_engine.c NEWS2 scoring, plausibility gates
  hrv_analysis.c           RMSSD / IBI windowing
  spo2_engine.c            R/ratio → SpO2, moving-average filtering
  ppg_sqi.c                signal quality index (Elgarten/Karlen style)
  ppg_respiratory_rate.c   RSA-based RR (Charlton 2018)
  ppg_baseline_rr.c        personal RR baseline
  pm25_correction.c        EPA/Barkjohn + AirGradient PMS5003 correction
  pm25_aqi_avg.c           24 h PM2.5 mean (12 h floor)
  pressure_trend.c         barometric rate → cyclone risk
  skin_thermal.c           peripheral thermal state machine
  nn_risk_model.c          FP32 MLP
  nn_risk_model_int8.c     INT8 quantised MLP (the one deployed)
firmware/shrikefi/       ESP32-specific
  main_shrikefi.c          app_main, all tasks, PPG pipeline, host test
  shrikefi_pinmap.h        THE pin map
  shrikefi_link_driver.c   FPGA SPI + bitstream load
  max30102.c bme280.c tmp117.c mpu6050.c pms5003.c gnss_nmea.c
  web_status.c/.h          SoftAP, all HTTP routes, status JSON
  skin_calib.c/.h          per-session core offset (+NVS persistence)
  dashboard/valor_dashboard.html   the dashboard (gzipped into firmware)
  valor_dashboard_gz.h     generated: dashboard gzip byte array
```

### 4.3 PPG data flow (Core 0, 100 Hz)

1. MAX30102 FIFO → 18-bit Red + IR.
2. `max30102_scale_to_8bit_ch()` normalises IR to an 8-bit sample centred on 120.
3. A 32-sample (320 ms) boxcar smooths it — the FPGA's own 8 taps are only 80 ms
   wide at 100 Hz, which was too narrow and let the dicrotic notch split beats.
4. Sample goes to the FPGA over SPI; MISO returns `{beat, filtered[6:0]}`.
5. A beat latches a µs timer; the IBI pipeline filters intervals, and
   `IBI >= 10` valid intervals arms the risk engines.
6. Contact gate: `IR > 60,000` counts (empty shroud ≈ 18,000; finger 120k–185k).
   The gate keys on **absolute light budget only** — ratios are not usable on this
   hardware (with a finger, Red actually reads *higher* than IR).

### 4.4 Measured live example

```
[TELEMETRY] HR=89.6,SPO2=98.2,RMSSD=47.0,TEMP=27.4,HUM=58.5,PM25=394.8,
            RR=0.0,SQI=0.80,PI=0.98,HRVN=206,PMRAW=381.5,PMCONF=OK,SKINTEMP=33.7
[TRIAGE] NEWS2=1,LEVEL=1,FLAGS=0x00,RISK=CRITICAL,PSI=0.89,AHA=0.50,AIVALID=1
```

### 4.5 Serial console commands

| Command | Effect |
|---|---|
| `cal <oral_ref_C>` | anchor the current skin reading to a reference thermometer |
| `calstat` | print skin temp, thermal state, settled flag, stored calibration |

`/cal` refuses unless `skin_thermal` reports `STABLE`.

---

## 5. Clinical and risk logic

### 5.1 mNEWS2 triage

Royal College of Physicians NEWS2, modified for the available sensors.
`LEVEL` 0 normal → 3 critical. **The temperature row is deliberately withheld at
0** because NEWS2 defines it on core/tympanic values and a healthy fingertip
(30–34 °C) would score +3 hypothermia on every well subject.

### 5.2 Moran Physiological Strain Index (PSI)

Moran et al. (1998), scale 0–10. Implemented in `disaster_risk_engine.c`.

```
PSI = 5·(T_core − 37.0)/(39.5 − 37.0) + 5·(HR − 70)/(180 − 70)
```
Simplifies to **2 PSI points per °C** of core temperature. Constants:
`PSI_T_CORE_REST_C 37.0`, `PSI_T_CORE_LIMIT_C 39.5`, `PSI_HR_REST_BPM 70`,
`PSI_HR_LIMIT_BPM 180`, valid core band 30–45 °C.

**Two provenances, and the device publishes which one it used** (`psisrc`):

| `psisrc` | Meaning |
|---|---|
| `CORE` | TMP117 anchored to a reference this session, periphery `STABLE`, core in band |
| `EST` | heart-rate + ambient heuristic (fallback) |
| `NONE` | not computable this cycle |

A raw fingertip is **never** promoted to core — 30–34 °C would report calm during
real heat illness. A unit test asserts that comparison.

Known limitation: the heuristic's thermal term is conservative (50 °C ambient at
rest scores only ~1.13/10), so PSI is in practice heart-rate dominated. The acute
heat signal is carried by the Steadman heat-index card.

### 5.3 PM2.5 autonomic strain

**A project heuristic motivated by** the AHA statement (Brook et al., *Circulation*
2010) — the AHA does **not** define this formula or its weights. Never describe it
as an "AHA formula".

```
pm_factor       = clamp((PM2.5 − 15)/(150 − 15), 0, 1)
hrv_suppression = clamp((35 − RMSSD)/(35 − 5), 0, 1)
strain          = 0.5·pm_factor + 0.5·hrv_suppression
                  + 0.25·clamp((PM2.5−35)/115,0,1)·hrv_suppression
```

### 5.4 Other indices

- **Heat:** NOAA Steadman/Rothfusz heat index → category.
- **PM correction:** EPA/Barkjohn (2021) + AirGradient PMS5003 "atm" equations on
  a 60 s mean, with a confidence code (`OK`, `DUST`, `HUMID`, `RANGE`, `NODATA`,
  `SETTLING`). Published nRMSE 79 %→38 %, MAE 7→3 µg/m³; **inherited from the
  literature, not locally validated** (no reference instrument).
- **Cyclone:** barometric **rate of fall** (hPa/hr) plus absolute drop; requires
  both. A short window yields `RISK_UNKNOWN`, never `NORMAL`.
- **Cold/flood:** heuristic exposure estimate, explicitly not the Moran Cold Strain
  Index.
- **Fall:** MPU6050 kinematic FSM.

### 5.5 Skin temperature

Source is **TMP117 only**. There is no die-based estimator any more
(`skin_estimate.c` was deleted 2026-11-02). If the probe does not answer, no value
is published — never an estimate dressed as a measurement.

`skin_thermal.c` is a peripheral **state machine**, not an estimator:
`UNKNOWN / STABLE / COOLING / REWARMING / VASOCONSTRICTED`. Key constants: 12 s
settle, `|dT/dt| < 0.15 °C/min` → STABLE, ≤ −0.35 → COOLING, ≥ +0.50 → REWARMING,
29 °C absolute vasoconstriction floor, 35 % ambient coupling.

`skin_calib.c` provides a per-session core offset (core = skin + offset):

| Rule | Value |
|---|---|
| Accepts skin in | 20 – 42 °C |
| Accepts reference in | 30 – 43 °C |
| Accepts offset in | −2 … +12 °C |
| Validity | 4 hours |
| Invalidated by | > 3 °C skin drift from the calibration point |
| Persistence | **NVS** (`skincal` + `devclk` keys), survives reboot/reflash |

Typical measured pair: skin 33.7 °C, oral 36.8 °C → offset ≈ 3.1 °C.

⚠️ **The plateau depends on placement.** Measured 33.7 °C in one session and
33.1 °C in another with the same person — over half a degree, i.e. ~1 PSI. A
single-point additive offset is only valid for similar contact.

---

## 6. Web interface and API

SoftAP: SSID **`VALOR-0081`**, password **`valorhealth`**, `http://192.168.4.1`.
A 4-digit control PIN is generated per boot, shown on the OLED, and logged.

| Route | Purpose |
|---|---|
| `/` | light status page |
| `/desktop` | full dashboard (gzipped into firmware) |
| `/status.json` | the status snapshot (see below) |
| `/cal?ref=<oral_C>` | anchor skin→core; refuses if skin not settled |
| `/api/emergency/cancel` | silences an SOS — **requires PIN** |
| `/location` | sets location — **requires PIN** |

**The dashboard is compiled into the app image** (`valor_dashboard_gz.h`, 40,091
bytes gzipped from 142,250 bytes of HTML). Flashing updates it. Regenerate with
`python scripts/bundle_desktop_dashboard.py` — it is reproducible (`mtime=0`).
Editing the inline script requires recomputing **two** CSP hashes: the `<meta>`
one *and* the `h_root` HTTP header hash.

`/status.json`: **66 keys, 1,106 bytes worst case**, in a 1,536-byte static buffer
(430 bytes headroom). Key groups: vitals (`hr`, `spo2`, `rr`, `sqi`, `rmssd`),
environment (`temp`, `hum`, `pm25`, `pressure`), `skintemp`/`skinsrc`/`skincal`/
`coretemp`, `psi`/`aha`/`psisrc`, NEWS2 sub-scores, rule strings, `sos`, `loc`,
`uptime`. All floats print `%.2f` — never `%+.2f`, which produces invalid JSON.

---

## 7. Numbers you may quote

### 7.1 TinyML

| Metric | Value |
|---|---|
| Topology | 6 → 24 → 16 → 3 MLP |
| INT8 parameters | **619 bytes** |
| MACs | 576 |
| Teacher agreement (FP32) | 88.47 % |
| Teacher agreement (INT8, deployed) | 88.39 % |
| Inference latency | **0.44 µs** on an x86-64 host at `-O2` |
| ESP32-S3 latency | **not measured** — do not claim it |

"Weights are INT8" ≠ "integer MAC unit": the kernel dequantises and multiplies in
float on the SX's FPU.

### 7.2 Timing

**IBI / HRV resolution is 10 ms**, because the sensor delivers 100 Hz. The 50 MHz
FPGA clock gives a 20 ns counter LSB — that is *quantisation of the counter*, not
measurement resolution. Quote 10 ms. The FPGA's contribution is determinism, not
sub-sample precision.

### 7.3 Clinical validation (MIMIC-III)

| Metric | Value |
|---|---|
| Cohort | 16,387 records / 98 patients |
| Sensitivity | 58.18 % (patient-level 95 % CI 49.7 – 70.3) |
| Specificity | 98.19 % |
| Accuracy | 94.11 % |
| TP / FP / TN / FN | 971 / 267 / 14,451 / 698 |

All 698 false negatives land in ELEVATED, none in NORMAL. Reproduce with
`tools/mimic_clustered_stats.py`.

---

## 8. Build, flash, test

```powershell
# Firmware build / flash
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
cd firmware/shrikefi
idf.py build
idf.py -p COM4 flash monitor

# Host test suite (PPG pipeline, JSON validity, PSI/AHA, TMP117, control PIN)
cd firmware/shrikefi
run_host_test.bat --no-pause

# Gates
python scripts/check_presentation_claims.py   # numbers must match reality
python tools/check_doc_links.py               # every relative link resolves

# RTL
hardware/zynq/build_and_run.bat               # 6/6
hardware/shrikefi/build_shrikefi_sim.bat      # 13/13
```

CI (`.github/workflows/ci.yml`) runs: docs (numbers + links), Verilator lint,
secret scan (rejects committed Wi-Fi credentials and unverified TLS contexts),
RTL + firmware simulation, the MIMIC accuracy gate (floors: accuracy ≥ 93 %,
FP32/INT8 agreement ≥ 99 %), and the PSI/AHA regression suite.

---

## 9. Known limitations and open issues

**Deliberate non-claims**

- Not a medical device; no clinical validation of PSI, strain or SpO2 on humans.
- SpO2 is computed but unvalidated against a reference oximeter.
- HRV/RMSSD needs ~3 minutes of clean signal; **never demo HRV from a 15 s
  capture**.
- Heat-index and PM values are published-method estimates, not local measurements.

**Open defects / risks (state these honestly)**

| Item | Detail |
|---|---|
| PM2.5 stuck ≈ 400–470 µg/m³ | Frames pass checksum and drift smoothly, supply measured 4.72 V (in spec) → either genuinely dirty air or a fouled laser chamber. Take the sensor to clean air and watch it; a real sensor falls to 10–20 µg/m³ within ~30 s |
| `skin_thermal` state bug | Reports `STABLE` too early (seen at 32.3 °C while rising 4 °C/min) and `REWARMING` too late (flat within 0.05 °C for 40 s). Both directions wrong; its rate estimate needs fixing |
| `ppg_has_optical_contact` | Defined but unused in the device build (`-Wunused-function`); only the host test calls it |
| No IBI A/B comparison | The old bitstream was overwritten and is gitignored, so old-vs-new cannot be compared |
| FPGA clock never measured | 50 MHz is the datasheet value, assumed not measured |
| No power/battery measurement | No fuel gauge, no VBAT sense divider |
| Skin calibration is one point | Additive offset only; the gradient varies with vasomotor state and contact pressure |

**Do not say:** "20 ns resolution", "clinically validated", "medical grade",
"AHA formula", "diagnostic device", "ESP32 latency 0.44 µs".

---

## 10. Where the original hardware reference lives

`docs/reference/shrike_board/` — the unmodified Vicharak material:

| File | Contents |
|---|---|
| `shrike_pinouts_full.txt` | **original ShrikeFi pinout dump** |
| `shrike_getting_started.txt` | board bring-up |
| `shrike_bitstream_guide.txt` | bitstream/OTP procedure |
| `Web_FPGA_programmer.ino` | the reference SPI programming sequence |
| `esp.rs`, `universal.rs` | Rust tooling references |
| `vicharak_spi_target.v` | reference SPI target RTL |

`docs/reference/valor_rigorous_skin_temperature_calibration_fusion_master_spec.md` —
the skin-temperature calibration spec.

---

## 11. Glossary

| Term | Meaning |
|---|---|
| IBI | Inter-beat interval (ms) |
| HRV / RMSSD | Heart-rate variability; root mean square of successive differences |
| SQI | Signal quality index (0–1) |
| PI | Perfusion index (AC/DC %) |
| PSI | Physiological Strain Index (Moran 1998) |
| NEWS2 | National Early Warning Score 2 |
| AHA | American Heart Association (cited as motivation only) |
| CLB | Configurable Logic Block — the FPGA's binding resource |
| SoftAP | The ESP32 acting as a Wi-Fi access point |
| NVS | ESP32 non-volatile storage |
| OOC | Out-of-context synthesis (Vivado) |
| MAF | Moving average filter |

---

*Single consolidated context document. Supersedes the previous narrative guides
that were removed from this folder; their history remains in git.*
