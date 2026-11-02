# VALOR — Complete Study & Defence Sheet
**SIH26181 · FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor**
*Generated 2026-10-30, 09:30 IST — every number below was measured on hardware or read from source this session.*

> **Read this first.** `docs/VALOR_COMPLETE_GUIDE_2.pdf` is an image-only PDF (0 fonts, 127
> images) exported 24 Oct. It cannot be searched, quoted, or updated, and it predates every
> fix made on 29–30 Oct. This file supersedes it for anything you say out loud.

---

# Part 1 — The 90-second pitch

> "A disaster site loses four things at once: the patient's vitals, the air they're breathing,
> the weather closing in, and the ability to call for help.
>
> VALOR is a single wearable + edge node that measures all of it and works with **no internet,
> no router, no laptop**. An ESP32-S3 reads a MAX30102 pulse oximeter, a BME280, a PMS5003 dust
> sensor, an IMU and a GPS. A Renesas ForgeFPGA runs the beat detector in hardware so beat timing
> is crystal-locked instead of being jittered by Wi-Fi interrupts. A 619-parameter INT8 neural
> network scores heat, pollution and flood risk on-device.
>
> Everything is on your phone: the board hosts its own Wi-Fi access point and serves its own
> dashboard from flash. Join `VALOR-0081`, open the page, done.
>
> And when it can't measure something, **it says so** instead of inventing a number — that's the
> part I'd like to show you."

**Why that lands:** the differentiator isn't a feature, it's the *honesty mechanism*. Most
projects show perfect numbers. You show a system that refuses to fabricate. That is the story.

---

# Part 2 — Architecture

```
        ┌─────────────── ESP32-S3 (160 MHz, dual core) ────────────────┐
        │                                                             │
        │  Core 0                        Core 1                       │
        │  PPG_Accel task (7 KB)         Risk_Monitor task (10 KB)    │
        │   ├─ MAX30102 @ 100 Hz         ├─ HRV / HRV settle gate     │
        │   ├─ SPI → ForgeFPGA           ├─ SpO2 engine               │
        │   └─ SQI window                ├─ TinyML INT8 inference     │
        │                                ├─ NEWS2 clinical engine     │
        │  IMU_Monitor (40 Hz)           ├─ Disaster rule engine      │
        │  PMS5003_UART                  ├─ OLED dashboard            │
        │  GPS_UART                      └─ HTTP server (SoftAP)      │
        └─────────────────────────────────────────────────────────────┘
```

## I²C bus (GPIO1 SDA / GPIO2 SCL, 100 kHz)

| Addr | Device | Purpose |
|---|---|---|
| `0x3C` | SSD1306 OLED | 128×64 onboard display |
| `0x57` | MAX30102 | Red/IR PPG + on-die thermometer |
| `0x68` | MPU6050 | 6-axis IMU → fall detection |
| `0x76` | BME280 | Temperature / humidity / pressure |
| `0x48` | TMP117 | **Not populated** — dedicated skin probe, part pending |

## Other buses

| Interface | Pins | Device |
|---|---|---|
| UART1 | RX GPIO14, TX GPIO18 (unused) | PMS5003 PM2.5 |
| UART2 | RX GPIO16, TX GPIO17 | NEO-M8N / L89HS GNSS |
| SPI2 | SCK 12, SS 10, MOSI 11, MISO 13 | ForgeFPGA SLG47910 |
| GPIO4 | — | Active buzzer |
| GPIO5 | — | SOS cancel button (hold 5 s) |

**Voltage rule:** every GPIO is 3.3 V LVCMOS. 5 V only for PMS5003 VCC. Never 5 V on a GPIO.

---

# Part 3 — The RTL (Verilog) — learn these three modules

## 3.1 `ppg_peak_detector.v` — the systolic crest detector FSM

This is the heart of the FPGA story. **Four states:**

```verilog
localparam STATE_ARMED      = 2'b00;
localparam STATE_RISING     = 2'b01;
localparam STATE_PEAK_FOUND = 2'b10;
localparam STATE_REFRACTORY = 2'b11;
```

### State behaviour

| State | What it does | Transition out |
|---|---|---|
| **ARMED** | Re-seed `peak_val` with the current sample | → RISING when `sample_in >= dyn_threshold` **AND** `sample_in > prev_sample` |
| **RISING** | Track the running maximum (`peak_val`) | → PEAK_FOUND when the sample falls ≥ `crest_fall_min` below `peak_val` |
| **PEAK_FOUND** | Commit the beat, latch `ibi_cycles` | Always → REFRACTORY |
| **REFRACTORY** | Count down a 250 ms holdoff | → ARMED when `refractory_cnt == 0` |

### The crest rule — *this is your best answer in the whole Q&A*

```verilog
wire [DATA_WIDTH-1:0] crest_fall_min =
    (peak_val > 8'd48) ? (peak_val >> 2) : 8'd6;
```

- **Strong signal** (peak > 48): require the sample to fall **25 %** below the running peak
- **Weak signal** (peak ≤ 48): require a **fixed drop of 6 counts**

**Why not "two consecutive decreasing samples"?**
> "That rule truncates the peak at the first pair of noisy decreasing samples on the *rising*
> edge, so the measured peak is short and the interval is wrong."

**Why was it changed from 12.5 % (peak/8) to 25 % (peak/4)?**
> "At 12.5 % the dicrotic notch — the secondary bounce after the systolic peak — was deep enough
> to confirm a *false* crest. Measured on hardware: interval pairs of 888/645 ms, 781/641 ms,
> 666/797 ms against a true ~730 ms rhythm. A real systolic downstroke falls well past 25 % of
> the peak, so peak/4 rejects the notch **without** a time-based holdoff."

That specificity — real measured numbers, a named physiological feature, a stated fix — is what
separates a team that simulated from a team that measured.

### Two more details worth knowing

**`first_beat_seen` guard:** the first beat after reset has no preceding interval, so it sets
`first_beat_seen` and emits no IBI. Only from the 2nd beat is an interval valid.

**Why REFRACTORY re-arms on the timer alone:**
> "An earlier version also required the signal to fall back below `dyn_threshold` before
> re-arming. In a real recording the pulse tail is still above threshold at 250 ms, so that extra
> condition silently dropped beats."

**Refractory period:** `12_500_000` cycles @ 50 MHz = **250 ms** (max reliable rate 240 BPM).

## 3.2 `moving_average_8tap.v` — the boxcar filter

```verilog
reg [DATA_WIDTH-1:0] shift_reg [0:7];
reg [DATA_WIDTH+2:0] running_sum;   // +3 bits prevents overflow for 8 samples

wire [DATA_WIDTH+2:0] next_sum =
    running_sum + {3'b000, data_in} - {3'b000, shift_reg[7]};

data_out <= next_sum[DATA_WIDTH+2:3];   // ÷8 is a 3-bit right shift
```

**The three things a judge may ask:**

1. **Why `+3` bits?** Eight 8-bit samples sum to at most 2040, which needs 11 bits. 8+3 = 11. ✅
2. **Why is ÷8 free?** Dividing by 8 is a 3-bit right shift — no divider, no DSP block. This is
   why the design uses **0 DSP48 slices**.
3. **Is it a true running sum?** Yes — add the newest, subtract the oldest. One add, one subtract,
   constant time per sample, regardless of window length.

**Latency:** the output is registered, so `data_out` trails `data_in` by one clock after
`data_valid`.

## 3.3 `spi_target.v` — 8-bit SPI mode 0 target

- **Two-flop synchronisers** on `sck`, `ss_n`, `mosi` — these arrive from the ESP32 in a
  different clock domain, so every input crossing into the FPGA's 50 MHz domain is synchronised.
- Shifts on the synchronised SCK edge, counts `bit_cnt` up to `BIT_MAX`.
- Asserts `o_rx_data_valid` for one cycle when a full byte is received.
- Drives `o_miso` from `tx_shift` with a tri-state enable `o_miso_oe`.

**Mode 0** = CPOL 0, CPHA 0: data sampled on the **rising** SCK edge.

## 3.4 `axi_ppg_accelerator.v` — the Zynq reference

Register map:

| Offset | Name | Access | Meaning |
|---|---|---|---|
| `0x00` | `REG_RED_RAW` | R/W | Red raw sample [7:0] |
| `0x04` | `REG_RED_FILTERED` | RO | Red filtered output |
| `0x08` | `REG_IBI_CYCLES` | RO | Inter-beat interval in clock cycles |
| `0x0C` | `REG_STATUS_THRESH` | RO/W1C | `[0]` beat flag, **write-1-to-clear** |
| `0x10` | `REG_IR_RAW` | R/W | IR raw sample |
| `0x14` | `REG_IR_FILTERED` | RO | IR filtered output |

**Why W1C for the beat flag?**
> "A plain read-to-clear would lose beats if the CPU read the register twice. Write-1-to-clear
> means the flag stays asserted until software explicitly acknowledges it, so a beat can never be
> lost to a duplicate read."

**The read-handshake fix:** the AR-accept condition is
`~s_axi_arready && s_axi_arvalid && ~s_axi_rvalid` — a new read address is refused while a
response is in flight, preventing the address latch from being overwritten mid-transaction.

## 3.5 The dual-target story

| | Zynq-7000 | ForgeFPGA SLG47910 |
|---|---|---|
| Part | `xc7z020clg400-1` | SLG47910 |
| Role | Architectural reference | **Shipping hardware** |
| Logic | 185 LUT, 266 FF | **533 LUT5 / 1120 (47.59%)**, 367 FF |
| CLBs | — | 118/140 (84.3 %) — **the real constraint** |
| DSP / BRAM | 0 / 0 | n/a |
| Timing | WNS +5.603 ns, WHS +0.184 ns, Fmax 69.45 MHz | Fmax 77.71 MHz, WNS +7.131 ns |
| Clock target | 50 MHz | 50 MHz |
| Cost | — | **sub-$0.50** |

**The killer Q&A answer:** *"The Zynq proves it costs almost nothing — 185 LUTs and zero DSP for a
complete dual-channel peak detector. The ForgeFPGA proves it can ship on a sub-dollar chip. Same
RTL, two very different budgets."*

**If asked about headroom:** CLBs are at 75 % (35 free), LUT5s only at 46 %. **Check new logic
against CLB count first**, not LUT count — CLBs are the binding constraint.

---

# Part 4 — The AI / TinyML

## 4.1 Architecture

```
6 inputs → 24 (ReLU) → 16 (ReLU) → 3 outputs (sigmoid)
```

- **619 parameters** (576 weights + 43 biases), **619 bytes** of INT8 storage
- Outputs: `heat_score`, `pollution_score`, `flood_score`, each in [0, 1]

## 4.2 Input normalisation

| Feature | Range | Why |
|---|---|---|
| HR | 40–200 bpm | bradycardia floor → extreme tachycardia |
| RMSSD | 1–100 ms | near-zero HRV → strong parasympathetic |
| SpO₂ | 70–100 % | severe hypoxaemia → normal |
| Temp | −10–55 °C | winter cold → heat wave |
| Humidity | 0–100 % | dry → saturated |
| PM2.5 | 0–500 µg/m³ | clean → AQI 500+ |

Each is `(value − MIN) / (MAX − MIN)` then clamped to [0, 1].

## 4.3 Quantisation

Per-tensor scales, symmetric INT8 weights (zero-point 0), asymmetric uint8 activations (ReLU/sigmoid
outputs are always ≥ 0). Real values:

```c
.W1_scale = 0.03131007f,  .W2_scale = 0.02121726f,  .W3_scale = 0.02267252f,
.b1_scale = 0.01159789f,  .b2_scale = 0.01001114f,  .b3_scale = 0.03963214f,
```

**Measured INT8 vs FP32 agreement: 99.95 %** — reproduced by running
`accuracy_evaluator` on `data/mimic/mimic_eval_feed.csv` this session.

## 4.4 The five things that make it defensible

1. **Every layer checks `isfinite()`.** A NaN feature would otherwise clamp to 0 and leave a
   plausible bias-only output.
2. **`nn_inputs_valid()` gates inputs** — HR 30–220, ambient −20–65 °C, humidity 0–100 %,
   SpO₂ 50–100 % or absent.
3. **RMSSD must be *published*, not just present.** Zero is not a physiological RMSSD; it maps to
   the most-suppressed end of the feature and would fake a collapsed HRV.
4. **Declines rather than guesses.** No HRV ⇒ all three channels `RISK_UNKNOWN` with a reason
   string, and `AIVALID=0`.
5. **The float32 model is not in the device image** (`NN_ENABLE_FLOAT32_MODEL=0`) — the shipped
   binary contains only the 619 INT8 weights. It exists as the reference the INT8 output is
   compared against.

## 4.5 The MIMIC-III numbers — say these together, always

```

Total rows 16,387 · 98 ICU patients
Accuracy      94.11 %
Specificity   98.19 %
Sensitivity   58.18 %   ← say this yourself, first
PPV 78.43 % · NPV 95.39 % · F1 66.80 %
```

**If you only say 94.11 %, a judge will find 58.18 % and you lose the exchange. If you say it
first, you win it.**

### The defence, verbatim

> "94.11 % accuracy, 98.19 % specificity — and **58.18 % sensitivity**, which I'll say plainly:
> we miss about four in ten crises on raw ICU data. That's expected, because the ML layer is *not*
> the safety layer. In the deployed pipeline, deterministic clinical overrides fire on
> SpO₂ ≤ 91 %, HR ≤ 40 or ≥ 131, and shock index > 1.0 — those bypass the model entirely.
> The network is an advisory coprocessor; the rules are the floor."

### Why is sensitivity low? (if pushed)

> "MIMIC's labels come from our own triage rule applied to raw CHARTEVENTS. ICU vitals oscillate
> across thresholds minute to minute, so adjacent rows can carry opposite labels. A 94 % accurate
> model on that signal is near the label-noise ceiling — which is exactly why we don't rely on it
> for the critical decision."

### The honest caveat you should volunteer

> "MIMIC-III v1.4 CHARTEVENTS has no beat-to-beat waveforms, so the RMSSD column in that
> evaluation is **synthetic**. Our real HRV validation is the FPGA timing testbench and the
> on-device settle gate, not MIMIC."

---

# Part 5 — Clinical engines

## 5.1 mNEWS2 (modified early warning score)

| Parameter | Score 3 | 2 | 1 | 0 | 1 | 2 | 3 |
|---|---|---|---|---|---|---|---|
| HR | ≤40 | — | 41–50 | 51–90 | 91–110 | 111–130 | ≥131 |
| SpO₂ | — | 92–93 | 94–95 | ≥96 | — | — | ≤91 |
| RR | ≤8 | — | 9–11 | 12–20 | — | 21–24 | ≥25 |
| Temp | — | — | 35.1–36.0 | 36.1–38.0 | 38.1–39.0 | ≥39.1 | ≤35.0 |

**Bands:** 0 = Normal · 1–3 = Low · **4–6 = Medium** · ≥7 = High

> **Why 4 and not 5?** Standard NHS NEWS2 calls 5–6 medium. VALOR escalates at **4** because in an
> austere disaster zone, evacuation takes hours — you cannot wait for a bedside reassessment.

**Deliberately excluded, and say why:** SBP and consciousness. No cuff, no GCS entry. They are
rendered as `n/a` rather than scored as `+0`, because an assumed-normal score is worse than an
absent one.

## 5.2 The HRV publication gate

RMSSD does not publish on the first few beats:

| Gate | Value |
|---|---|
| Minimum beats | 30 (ultra-short-term standard) |
| Minimum span | 20 s of continuous contact |
| Minimum valid pairs | 15 |
| Median filter | 11-beat window |
| Delta-IBI gate | > 120 ms jump breaks the sequence |

**Measured on hardware tonight: HRV published at 45 s** (RMSSD 59.1 ms).

> "RMSSD from fewer than 30 beats is dominated by RSA phase variance — you'd be measuring which
> part of the breathing cycle you started on, not vagal tone."

- **Moran PSI** (Moran 1998, 0–10 scale): physiological strain from HR plus a *heuristic* core-temperature proxy derived from HR and ambient/humidity. The code says plainly that this is **not** the Buller 2013 model, and it does **not** use the skin probe.

- **AHA autonomic strain** (Brook et al., AHA 2010, 0–1): PM2.5 combined with RMSSD

**Be honest about the core-temperature estimate inside PSI:**
> "The `t_core_est` inside PSI is a heuristic — `37.0 + 0.015 × (HR − 75)` — not the Buller 2013
> model. It's clearly marked in the source. I'd rather ship a labelled approximation than claim a
> validated core temperature we can't measure."

## 5.4 Disaster risk channels

| Channel | Input | Threshold |
|---|---|---|
| Heat | ambient T, humidity, HR, PSI | NWS Rothfusz heat index |
| Pollution | PM2.5, humidity | EPA AQI 2024 + WHO 2021 24-h guideline |
| Flood / cold shock | barometric trend, ambient T, skin T | pressure drop + hypothermia bands |
| Storm | pressure slope over ≥30 min | cyclone risk from hPa/hr |

**The flood question — your weakest tile, so own it:**

> "The flood channel isn't a water detector. It scores **cold-water immersion and cold-shock risk**
> from what we can actually measure: ambient temperature, humidity, barometric pressure trend.
> Submersion drives rapid heat loss, and the barometer catches the pressure signature of the storm
> that floods you. With RMSSD published it also picks up the autonomic dive response. When the
> inputs aren't there, the model **declines and prints `--` rather than a fake all-clear** — that's
> what `AIVALID=0` means."

---

# Part 6 — The optical path (learn this, it's where the bugs were)

## 6.1 Signal chain

```
MAX30102 18-bit IR @ 100 Hz
   ↓ 4-sample boxcar (software, ~40 ms)          ← replaces the sensor's old FIFO averaging
   ├─→ SQI window (3 s)
   ├─→ SpO2 engine (1 s window, Welford variance)
   └─→ 8-bit scale (centre 120, ÷4 gain) → SPI → ForgeFPGA
```

## 6.2 Contact gate — the most important fix of the session

```c
#define PPG_CONTACT_MIN_IR_COUNTS  60000u
```

A finger must return **> 60,000 counts** on IR or Red. Measured values:

| Condition | IR | Red | Verdict |
|---|---|---|---|
| Ambient air (bare sensor) | 4,400 | 5,400 | reject |
| **Empty enclosure (black tape reflects)** | **18,300** | **17,700** | **reject** |
| Just below bar | 59,999 | 59,999 | reject |
| **Real finger** | **128,000** | **134,000** | **accept** |

### The bug this fixed — tell this story, it's your best one

> "With the 3D-printed shroud on and **nothing in it**, the black inner tape bounces the LEDs
> straight back. That produced IR 18,300 — above the old 7,000 threshold — so the firmware
> reported **LOCKED, HR 98.9 BPM, RMSSD 59.4 ms, SQI 0.79, SpO₂ 93.8 %** for a completely empty
> enclosure. A healthy-looking patient who was not there. That is the most dangerous failure a
> medical device can have, and we only found it because we looked at the board with nothing in it."

### Why not use perfusion index (PI)?

Because it **doesn't separate them** — and this is a great measured answer:

| Condition | DC | AC | PI |
|---|---|---|---|
| Empty shroud | 18,230 | ~400 | **2.19 %** |
| Real finger | 128,700 | ~700 | **0.66 %** |

> "The reflection has **three times** the perfusion index of a real finger, because flicker and
> 50 Hz pickup are bigger single-sample excursions than a smooth cardiac pulse. We implemented a
> PI gate, measured it on hardware, found it **admitted** the empty enclosure, and removed it.
> What works is the absolute light budget — skin returns an order of magnitude more light than a
> taped plastic cavity."

**That is a real engineering story: hypothesis → implementation → measurement → reversal.**

## 6.3 LED drive and AC gain

| Parameter | Value | Why |
|---|---|---|
| LED current | `0x24` (~7.2 mA) | `0x36` saturated at IR 226,700 (86 % of full scale) inside the enclosure |
| AC centre | 120 | must match FPGA `dyn_threshold` |
| AC divider | **4** | dropped from 8 when LED current was reduced — at ÷8 the 8-bit IR no longer crossed the crest threshold of 120 and `beats=0` |

> "Lowering the LED current fixed saturation but broke the beat detector, because it shrank the AC
> swing below what the FPGA's fixed threshold needed. We compensated by halving the AC divider.
> One fix exposed the next — which is why we measured at every step."

## 6.4 SpO₂ engine

- **Welford's online variance** (not sum-of-squares) — subtracts two nearly-equal large numbers
  otherwise, losing the small AC component when DC is ~160,000
- AC = **2√2 · σ** — the peak-to-peak of an equivalent sinusoid
- Windows: 1 s (100 samples), 8-value history, **trimmed mean** (drop highest + lowest)
- Gate: 3 valid windows, spread ≤ 3.5 %, plus monotonic-drift rejection
- DC stability: reject a window if the DC baseline jumped >15 % since the previous window
- Calibration: `SpO2 = 101.8 − 6.67·R` (2-point fit; **note the shallow slope compresses dynamic
  range and may under-report severe desaturation**)

**Measured on hardware tonight:** SpO₂ latches in ~17 s at 96.5–98.2 %.

## 6.5 Acquisition timings — memorise these

| Event | Measured |
|---|---|
| SQI starts computing | ~3 s |
| **HR locks** | **~10 s** |
| **SpO₂ latches** | **~17 s** |
| **HRV (RMSSD) publishes** | **45 s** |
| Finger removed → all zeros | < 1 s |

> "45 seconds from finger-in to fully settled. I know that number because we measured it."

---

# Part 7 — Skin temperature (know exactly what it is and isn't)

## 7.1 The model

```
T_skin = b0 + b1·(peak_die − baseline) + b2·(T_amb − 27.5)

b0 = 34.10 °C   (nominal, NOT fitted)
b1 = 0.55
b2 = 0.12
```

The baseline is `T_amb + measured_self_heating`.

## 7.2 What we fixed tonight — and the bug is the interesting part

The code hardcoded the LED self-heating as `ambient + 5.5 °C`. On the real board the die only sat
**+3.66 °C** above ambient, so the assumed baseline (30.90) was **higher than the peak the finger
ever produced** (30.07). The delta clamped to zero:

> **The die thermometer contributed exactly 0.00 °C. The published 33.9 °C was the constant `b0`
> plus an ambient term — the sensor was doing nothing.**

The board now **measures** its self-heating at boot:

```
[SKINCAL] LED self-heating MEASURED: die_warm=28.44 amb=24.05 -> offset=4.39 C
```
And publishes the provenance so the UI can be honest:
And publishes the delta so the UI can be honest:
`skinstale` — true when the published value was *held* from the last good read rather than read fresh this cycle, so a frozen number cannot pass as a live one.

**Measured tonight:** TMP117 probe reading room air at 26.2 °C, then **34.1 °C** with a fingertip on it over ~60 s. That is a real contact thermometer, not a model.

## 7.3 What to say

> "Skin temperature is **measured**, not estimated. There is a TI TMP117 contact thermometer on
> the I²C bus at 0x48 — ±0.1 °C, 1/128 °C resolution. The device checks its
> device ID at boot, so it can tell a real probe from anything else answering at that address.
> Watch: finger on, it climbs from room temperature to about 34 °C and holds there.
>
> We used to *model* skin temperature from the MAX30102’s on-die thermometer. We removed it on
> 2 November. The die is a junction sensor heated by its own emitters; making a skin temperature
> out of it needed a boot-time LED self-heating measurement, two separate validity gates and an
> “uncalibrated” label. It was a model output that looked like a measurement, so it went.
> If no probe answers we now publish **nothing** — `skintemp: null` — rather than a guess.
>
> And it stays **out of the NEWS2 score**. NEWS2 scores core/tympanic temperature; a fingertip
> reads 30–34 °C and would score +3 hypothermia on every healthy person. A clinical score
> should not consume a number that cannot be defended."

## 7.4 The honest limitations (volunteer these)

1. **We removed the estimate rather than tune it.** The die model was never fitted against a
   reference, and no amount of tuning fixes a sensor that heats itself.
2. **A fingertip is not a core site.** It reads 30–34 °C against a 36.5–37.5 °C core,
   and that gap is vasoconstriction, not a calibratable offset. We report the *trend* and label
   it as such.
3. **The core offset needs one reference reading per session.** `http://192.168.4.1/cal?ref=36.6`
   records an oral reading taken with the probe in place and derives the offset from it. It
   expires after 4 hours or when skin drifts more than 3 °C from where it was calibrated.
4. **The offset does not survive a reboot yet.** It lives in RAM; persisting it to NVS is open
   work.

## 7.5 If asked "shouldn't skin temp be 97 °F?"

> "97 °F is **core** temperature. A resting fingertip in a 25 °C room is normally 30–34 °C — the
> periphery runs 4–8 °C below core. Our reading is physiological; what's unproven is its precision,
> which is exactly why it's labelled uncalibrated."

---

# Part 8 — The dashboards

## 8.1 Two UIs, don't confuse them

| URL | Size | Purpose |
|---|---|---|
| `http://192.168.4.1` | inline HTML in `web_status.c` | phone status page |
| `http://192.168.4.1/desktop` | **38.7 KB gzip → 139 KB** | full desktop dashboard |

**SoftAP:** SSID `VALOR-0081`, **open** (no password), channel 6, 4 clients max.
Boot log: `Status page up: join WiFi 'VALOR-0081' (open), then open http://192.168.4.1`.

## 8.2 Why it works with no internet

- Dashboard served from **flash**, not the internet
- **Zero** external URLs — no CDN, no fonts, no analytics
- Chrome loads a private address fine; you only lose internet, not the network

**Say this:** *"A disaster site has no internet. `VALOR-0081` **is** the network."*

## 8.3 Security measures worth mentioning

- **CSP with a SHA-256 hash** pinning the one inline script — a single character change breaks it,
  which is the point
- `Cache-Control: no-store` on JSON, `Content-Length` + `no-cache` on the dashboard
- **CORS** on all JSON endpoints (for `file://` dashboard use)
- **Same-origin check** on SOS trigger/cancel and location POST
- SOS cancel requires `hold_ms=3000` **and the 4-digit control PIN** — a bare POST, or a
  correct hold without the PIN, is refused

## 8.4 JSON payload — 40+ fields

`/status.json` (official measured length: **757–820 bytes**)

```
hr, spo2, rr, sqi, rmssd, prv, hrvn, rrconf, rrdepth, pi,
temp, hum,
skintemp, skinsrc, skinstale, skincal, skintherm, skintrend, skinrate,
coretemp, calstat, calref, calsite,
pm25, pm25raw, pm25conf, pm2524h, aqi24, aqibasis,
pressure, ptrend, pdrop, pspan, pvalid,
news2, level, flags, hrpts, spo2pts, rrpts, temppts,
```

`/ppg.json` — real 25 Hz IR waveform:
`{"n":40,"fs":25,"samples":[165432,165498,...]}`

`/api/history` — 24-hour ring (hourly averages, persisted to NVS)

---

# Part 9 — Numbers you must not get wrong

| ✅ Say | ❌ Never say | Note |
|---|---|---|
| ESP32-S3 @ **160 MHz** | 240 MHz | `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=160` |
| **6→24→16→3, 619 params** | 16-16-8-3 / 576 MAC | the fact sheet is wrong |
| **94.11 % acc · 98.19 % spec · 58.18 % sens** | "94 % accurate" alone | always all three |
| **88.39 %** = teacher-student distillation fidelity | "88 % clinical accuracy" | it's agreement with our own rule engine |
| Inference **0.44 µs on x86 host** | "0.44 µs on device" | sub-ms on ESP32 |
| ForgeFPGA **533 LUT5 / 118 of 140 CLBs** | "443 LUTs" | 443 was a superseded fit |
| Zynq **185 LUT, 0 DSP, 0 BRAM** | — | WNS +5.603 ns |
| **45 s** full acquisition | "instant" | finger-in → HRV published |
| **SpO₂ ±2 %** is our accuracy claim | "clinical grade" | it's an uncalibrated MAX30102 |

---

# Part 10 — Hard Q&A, with answers

**"Why an FPGA when the ESP32 has a 240 MHz CPU?"**
> "Not speed — **determinism**. FreeRTOS ticks and Wi-Fi bursts inject 5–20 ms of jitter, which on
> an 800 ms interval is a 1.25 % timing error that corrupts RMSSD. The FPGA timestamps at 50 MHz =
> an 800 ms interval is a 1.25 % timing error that corrupts RMSSD. The FPGA counts the
> interval on a 50 MHz counter, so the CPU and the radio cannot perturb it at all.
>
> The counter’s LSB is 20 ns, but do not call that the resolution. A crest can only be
> located to the nearest sample, and the MAX30102 delivers one sample per 10 ms — so the
> honest figure is **10 ms**, limited by the sensor, not by the FPGA. What the FPGA buys
> is *determinism*: the same 10 ms quantisation every beat, with no OS jitter on top.
**"How do you know the FPGA output is correct?"**
> "Three testbenches: 6/6 on the Zynq AXI system, 4/4 on the common peak detector, 13/13 on the
> ForgeFPGA SPI system — including the dicrotic-notch rejection and beat-spacing checks. And the
> arithmetic was verified **bit-exact** against a Verilog model by replaying the hardware SPI log:
> 100 % of received bytes matched the reference moving-average output."

**"What happens if the FPGA fails?"**
> "The MCU falls back to a software beat detector. It's less deterministic but not blind. The link
> is checked at boot and the MISO line is probed with weak pulls to detect a floating line — though
> that check can't prove a frozen FPGA, and the source says so."

**"Is this medically validated?"**
> "No, and we don't claim it. It's a triage **aid**. HRV has no ECG-Holter validation — that's an
> open roadmap item. SpO₂ is a MAX30102 with a 2-point calibration, good to roughly ±2 %, not a
> clinical oximeter. What *is* verified is the arithmetic, the timing, and the failure modes."

**"Why does the file say 'uncalibrated'?"**
> "Because it is. The coefficients are nominal defaults, not a fit — we couldn't fit them without a
> reference thermometer. So the code has a boolean that marks the estimate as uncalibrated, the UI
> prints it, and NEWS2 ignores it. When the TMP117 lands, one calibration run flips the flag and
> the label changes automatically."

**"What was your hardest bug?"**
> "The empty enclosure. With the shroud fitted and nothing in it, the black inner tape reflected
> the LEDs back hard enough that the firmware reported a full healthy patient — HR 99, SpO₂ 94,
> SQI 0.79. We fixed it with a light-budget threshold after trying and *rejecting* a perfusion-index
> test that made it worse. The lesson is that a device must be tested in its failure state, not
> just its success state."

---

# Part 11 — Demo script (rehearse this)

| Step | Action | Say |
|---|---|---|
| 1 | Phone joins `VALOR-0081`, open `http://192.168.4.1/desktop` | "No internet, no router, no laptop." |
| 2 | Point at empty tunnel | "Nothing in it — and notice it shows nothing. No invented patient." |
| 3 | Finger **in the tunnel** | "HR locks in about 10 seconds." |
| 4 | Wait ~17 s | "SpO₂ now — it waits for 3 stable windows." |
| 5 | Wait to ~45 s | "HRV needs 30 beats. Watch it climb." |
| 6 | Palm the sensor / move | "SQI drops, triage holds — it won't score a bad signal." |
| 7 | Trigger SOS → cancel (hold 3 s) | "Countdown, buzzer, and a 3-second hold to cancel so it can't be a pocket-tap." |
| 8 | Finger out | "Everything clears in under a second." |

**Backup if Wi-Fi misbehaves:** the board's OLED shows the same vitals, and the dashboard has a
**Simulation** button for the six clinical profiles. Use simulation only if you *say* it's
simulation.

---

# Part 12 — Things that are genuinely not done (know them)

1. **TMP117 not populated** — pads exist, part on backorder. Skin temp is an estimate.
2. **SpO₂ R bounds are empirical** (0.10–2.20), wider than physiological, to avoid rejecting real
   desaturation. The calibration curve compresses dynamic range.
3. **No battery voltage monitoring** — no ADC read, no SoC field.
4. **No ECG-Holter validation** of HRV (roadmap T4.1).
5. **MIMIC RMSSD is synthetic** (no waveforms in CHARTLABELS).
6. **Vivado `.runs/` was overwritten** — the 185-LUT figure is preserved evidence, not a rerun.
7. **Wi-Fi dashboard waveform is real** (`/ppg.json`), but the *serial* oscilloscope remains the
   reference for full 100 Hz fidelity — Wi-Fi is 25 Hz decimated.

---

# Part 13 — One-page cheat card

```
HARDWARE      ESP32-S3 160 MHz · ForgeFPGA SLG47910 · MAX30102 · BME280 · PMS5003 · MPU6050 · GNSS
BUSES         I2C GPIO1/2 @100k · SPI2 12/10/11/13 · UART1 GPIO14 · UART2 GPIO16/17
FPGA          peak detector FSM 4 states · 8-tap MAC ÷8 · 250 ms refractory · peak/4 crest rule
              533 LUT5 (47.6%) · 118/140 CLB (84.3%) · 367 FFs · Fmax 77.71 MHz
              IBI 20 ns counter LSB, 10 ms resolution (100 Hz sensor)
ZYNQ          185 LUT · 0 DSP · 0 BRAM · WNS +5.603 ns · Fmax 69.45 MHz
AI            6→24→16→3 · 619 INT8 params · 99.95% INT8/FP32 agreement
MIMIC         16,387 rows · 94.11 acc · 98.19 spec · 58.18 sens
CLINICAL      mNEWS2 med at 4 · HRV gate 30 beats/20 s · PSI 0-10 · AHA 0-1
OPTICS        IR>60,000 = contact · LED 0x24 · AC÷4 · SpO2 = 101.8-6.67R
TIMING        HR 10 s · SpO2 17 s · HRV 45 s · clear <1 s
WI-FI         VALOR-0081 open · 192.168.4.1 · 38.7 KB gzip → 139 KB · 40+ JSON fields
```
