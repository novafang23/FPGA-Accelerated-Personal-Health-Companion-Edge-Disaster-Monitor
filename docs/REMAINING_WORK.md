# VALOR — Remaining Work

**Purpose:** the definitive list of what is left, ordered by judging value per hour of effort.
**Scored against:** the official Qualcomm problem statement (SIH26181, *Personal Health Companion*).
**GPS is deliberately excluded** — see "Out of scope" for why.

---

## Tier 1 — No hardware. Do these first.

### T1.1 — Stop publishing health data to a public broker  ✅ **DONE (2026-09-21)**

`CONFIG_SHRIKEFI_CLOUD_PUBLISH` added to `main/Kconfig.projbuild`, **default `n`**. The MQTT
client config, `esp_mqtt_client_start()` and `cloud_publish_health_data()` are all compiled out;
the broker URI and topic only exist inside the same `#ifdef`. Default build logs
*"Cloud publishing is disabled by config. Patient data stays on the edge."*

**Requirement 5 is now true instead of contradicted.**

Also removed in the same pass: `scripts/typesafe_triage.py`, which POSTed patient vitals
(HR, SpO2, RMSSD, RR, SQI) to `https://api.typesafe.ai/v1/systemone` — the same pattern the
MQTT blocker was raised for, just on the host side.

---

### T1.2 — SOS state machine + full-screen OLED emergency card  ✅ **DONE (2026-09-28)**

Requirement 6, and it needed no parts. `firmware/shrikefi/sos.{h,c}` holds the state machine —
`IDLE / ARMED / ACTIVE / CANCELLED` — deliberately free of hardware and FreeRTOS so its timers
are unit-tested on the host rather than only on the bench.

**Shipped:**
- **Trigger:** a CRITICAL verdict from `clinical_fuse_triage()`, confirmed over 3 s so a single
  artefact-corrupted second cannot latch. A critical flag with no finger on the sensor is
  ignored — the triage engines do not run without contact, so that flag is stale, not a finding.
- **Card:** 128×64, seven rows, replaces the dashboard entirely — `* MEDICAL EMERGENCY *`,
  `SOS ACTIVE T+MM:SS`, HR/SpO2, RR/SQI, `CONDITION:`, `LOC:`, `CALL 108 DO NOT MOVE`.
  Every line is ≤21 characters because the 5×7 font advances 6 px on a 128 px panel.
- **Cancel:** `sos_cancel_press()/release()` implement the 5 s hold so a knock cannot dismiss it.
- **Recovery stand-down:** 30 s of continuous non-critical clears a CRITICAL-triggered emergency
  automatically. This is not optional — with no button and no network yet, a false trigger would
  otherwise hold the screen until the battery died. A manual emergency is never auto-cleared.
- **Verified:** `test_sos_state_machine()` covers every transition; and the rendered card was
  checked against the real font table pixel-by-pixel (`EXACT MATCH`, 1465 lit pixels, 0
  mismatches) via the `[SOS CARD]` framebuffer dump, since the SSD1306 is write-only and the
  panel cannot be read back.

**Deliberate deviation:** the "prolonged loss of contact" trigger is implemented but **OFF by
default**. This is a fingertip device — the finger comes off between every measurement — so that
trigger would fire continuously in normal use. It is the right trigger for a worn device; leave
it enabled via `sos_config_t` when T2.3 (IMU) makes the device wearable.

**Still open in requirement 6:** the physical button (T2.2, ~₹20) to drive `sos_cancel_press()`
and a manual trigger; the SoftAP page (T1.3) to trigger and display it remotely; and the stored
location (T1.4) — the card currently says `LOC: UNSET` rather than showing a coordinate the
device never measured.


---

### T1.3 - SoftAP + local status page  DONE (2026-09-28)

**This is the strongest available demo and it needs no router.**

`firmware/shrikefi/web_status.{h,c}` raises an open access point and serves a live status page
from the device itself. The dashboard that existed before this needed a PC, a USB cable and a
browser gesture - the opposite of a field device. Now a rescuer's phone joins `VALOR-xxxx` and
reads the patient's status directly.

- `GET /` serves a 3.9 KB phone-sized page in the VALOR palette.
- `GET /status.json` serves the whole snapshot: HR, SpO2, RR, SQI, RMSSD, temp, humidity,
  PM2.5, NEWS2, level, flags, fused risk, SOS state and trigger, location, uptime, contact.
- The page **derives nothing** - every number is computed on the device, the same rule the
  desktop dashboard follows.
- The AP is **open by design**: it is an emergency status page, and a passphrase would be a
  credential a bystander does not have. Nothing confidential leaves the device over it.
- **Failure is never fatal.** Every init step is returned and logged rather than
  `ESP_ERROR_CHECK`'d, so a radio that will not come up leaves the device measuring.

**Deliberate deviation from the original sketch:** the page is published **always**, not only on
SOS, and the AP **replaces** the station path rather than joining it. Publishing always means a
rescuer who arrives *before* any alarm still gets a reading. AP+STA would force the access point
onto whatever channel the station associates on, dropping a connected phone mid-demo; and cloud
publishing is off by default, so the station connection had no function left to lose.
`CONFIG_SHRIKEFI_SOFTAP` selects this; turning it off restores `wifi_mqtt_init()`.

**Verified on hardware, end to end:** the PC joined `VALOR-0081`, `GET /status.json` returned
HTTP 200 with live values, `GET /` returned the page, an unknown path returned 404, and the PC
was returned to its own network afterwards. The page's JavaScript was then rendered against a
stubbed endpoint in headless Chrome and the DOM checked in three states - healthy, emergency and
no finger - including that an absent reading renders `--` rather than `0`.

**Still open:** the page is read-only. A manual SOS trigger and a cancel button from the phone
need a POST endpoint (a natural pairing with T2.2's button). T1.4 supplies the location it shows.


---

### T1.4 — Stored location in NVS

Requirement 6 bullet 3, with no GPS.

- A short location string ("Village / Block / District"), set once by the caregiver over the
  AP page or a serial command, stored in NVS.
- Shown on the SOS card and the AP page.

For rural disaster response this is arguably better than a satellite fix — village and block
names are how people actually describe where they are, and it survives being under debris or
indoors, where GPS does not. ~1 hour.

---

### T1.5 — Verify the respiratory-rate wiring on hardware

`ppg_respiratory_rate.c` and `ppg_sqi.c` were wired into the build but **have never run on the
device**. Flash and confirm:
- `RR=` and `SQI=` appear in the telemetry line
- RR lands in a plausible 10–20 br/min at rest
- NEWS2 no longer scores RR as a fixed normal 14

Without this the claim is unverified. ~30 minutes including a 3-minute capture.

---

### T1.6 — Fix the README badge  ✅ **DONE**

Now reads `MIMIC-III Benchmark 94.11% (demo subset)`. It is an accuracy figure on a 98-subject
demo subset, not clinical validation, and the same model's sensitivity is 58.18% — the
limitations section already says so. ~5 minutes.

---

### T1.7 — Dashboard images in the README  ✅ **DONE**

The web dashboard can be rendered headlessly, so the screenshots no longer need
to be taken by hand. `docs/images/valor_dashboard_{normal,heatwave,icu}.png` are
generated from the page itself at `--window-size=1440x1105`, and the README's
dashboard section shows them. No longer blocked on anyone supplying images.

Regenerate with:

```
chrome --headless=new --window-size=1440,1105 --virtual-time-budget=8000 \
       --screenshot=out.png "file:///.../valor_dashboard.html?sim=1&full=1&profile=Heat%20Wave%20%26%20Dehydration"
```

---

## Tier 2 — Small hardware purchases

### T2.1 — Skin temperature sensor  (highest-value part on this list)

**Requirement 1 lists body temperature. You do not measure it.**

```c
/* Skin temperature not available from current sensors */
env.skin_temp_c = 0.0f;
```

Consequence: the hypothermia path in `assess_skin_hypothermia()` is dead, and the heat engine —
your headline use case — runs on an ambient proxy. For a heat-wave project this is the weakest
link in your own story.

- Part: **MAX30205** (I²C, ±0.1 °C, designed for human body temperature) or **TMP117**.
- Wiring: the existing I²C bus at GPIO1/2. **No new pins.**
- Work: one driver file, feed `env.skin_temp_c`, re-enable the hypothermia assessment, feed the
  heat engine with real skin temperature.

Cost ~₹200. Effort ~3 hours. **Closes requirement 1's body-temperature gap and materially
strengthens 2 and 3.**

---

### T2.2 — SOS button + buzzer

- Button: any spare GPIO (`3, 4, 5, 6, 7, 15, 16, 17, 21`), input with pull-up.
- Buzzer: any spare GPIO (passive buzzer on LEDC for a two-tone alarm).
- Pattern: intermittent tone + LED strobe, distinct from the normal heartbeat LED.

Reaches people **physically nearby** — in a disaster, that is exactly who matters. ~1 hour plus
₹50 of parts.

---

### T2.3 — IMU (accelerometer)

Closes three gaps with one part: **activity levels, sleep quality, and fall detection**.

- Part: **LSM6DS3** or **MPU6050**, on the existing I²C bus. No new pins.
- Fall detection: free-fall → impact → post-impact inactivity, a well-understood threshold
  algorithm. Demos extremely well.
- Sleep: activity + stillness over hours, with a resting-HR/HRV component. Overnight wear needed.

Cost ~₹200. Effort ~4 hours. **Closes requirement 1 (activity, sleep) and requirement 6
(falls).**

---

## Tier 3 — Software, no hardware

### T3.1 — Barometric pressure trend → cyclone advisory

Free: the BME280 already measures pressure and the code does not use it for anything.

A falling barometric pressure is a cyclone precursor. A 3-hour pressure trend gives a genuine,
physically-grounded storm advisory with **zero new hardware**. ~1 hour.

Closes part of requirement 3 (flood/cyclone advisories), which is currently not instrumented.

---

### T3.2 — Dashboard history and daily summaries

Requirement 7 asks for *"daily health summaries and trend analysis"*. The current dashboard is
live-only.

- Persist rolling summaries (hourly HR/SpO2/RMSSD/risk) to NVS or SD.
- Show a trend line alongside the live view.

~4 hours.

---

### T3.3 — Cosmetic: log spam on repeated handover

When the finger is off, the FPGA still reports spurious crests (the scaler toggles between 0 and
120 around the `raw < 1000` threshold), and each one re-triggers a handover log line. Harmless,
but noisy. Gate the handover log, or suppress beats while `!optical_contact`. ~30 minutes.

---

### T3.4 — Rewrite the documents that still describe the retired 4-bit parallel link

The FPGA↔MCU link is 4-wire SPI (mode 0, SPI2_HOST), which is what
`firmware/shrikefi/shrikefi_link_driver.c` implements and what
`hardware/shrikefi/forgefpga_ppg_top.v` declares (`i_ss_n`, `i_sck`, `i_mosi`,
`o_miso`, `o_miso_oe`). Several documents still specify a synchronous 4-bit
parallel nibble bus with a strobe, a direction line, a command map and a
dedicated `irq_beat` pin. That design was retired before it was ever built.

`firmware/shrikefi/shrikefi_pinmap.h` has been corrected and its eight dead
parallel-bus aliases removed. The prose has not:

| File | What is wrong |
|---|---|
| `docs/SHRIKEFI_LINK_PROTOCOL.md` | Stale end to end (lines 4-109). Invented `CMD_*` command map, wrong pins for every signal, wrong timing section. Its pin numbers **collide** with the real SPI pins, so it is worse than merely old. |
| `docs/SHRIKEFI_HARDWARE_CONNECTIONS.md` | §1 pin table (lines 14-21) and §4 (127-138). GPIO 11 is listed as `rst_n` when it is actually MOSI — actively dangerous in a wiring guide. |
| `README.md` | Lines 288-294 (pin table), 300-306, 89, 537, 573, 703. Also 293-294 swap I²C and UART1. |
| `docs/MASTER_PROJECT_GUIDE.md` | Lines 111-138, 168, 184, 351, 805-807, 946. The headline is line 115/806: *"There is no AXI bus, no SPI controller, no I2C"* — in the judge-defence guide, for a link that **is** SPI. Line 807 then claims a command map is verified by the testbench; there is no command decode in the RTL and no `SHRIKEFI_CMD_*` use in the driver. |
| `docs/theory/THEORY_NOTES.md` | Lines 65, 91-115, 159, 180-184, 244-288, 304-306, 410-411. |
| `idea.md`, `ROADMAP.md` | Phase-2 text predates the ShrikeFi port. |

Correct pin table (from `hardware/shrikefi/forgefpga_pins.pcf` and
`shrikefi_pinmap.h`): ESP32 GPIO10→FPGA PIN_17 (CS), GPIO11→PIN_18 (MOSI),
GPIO12→PIN_16 (SCK), GPIO13→PIN_19 (MISO); GPIO8 = EN, GPIO9 = PWR. There is no
reset pin — the RTL resets from an internal power-on counter. The beat is bit 7
of the MISO byte, not an interrupt line. Note that the `.pcf` inline comments
append FPGA-side port designators (`-> GPIO3_IN` etc.) which are **not** ESP32
GPIO numbers; that collision is the likely origin of the wrong tables.

Also worth fixing while in here: `docs/MASTER_PROJECT_GUIDE.md:390` gives the IBI
conversion as `ibi_cycles / 50` when 1 tick = 20 ns makes it `/ 50,000` — the
same file prints it correctly at line 385, and the worked examples at 396-406
are physically impossible as a result (a 3,281-tick interval is 65.62 **µs**, not
65.62 ms). Its flash figures (744-745, 942) also still say 1 MB / 2 MB against
the real 7 MB app partition and 8 MB device.

---

### T3.5 — The respiratory rate no longer publishes at all on real data

The RR rail fix (54d9a27) stopped the estimator publishing a clamped
36.00 br/min as a reliable measurement — it could report the tachypnoea ceiling
for a healthy resting subject with **no** respiratory modulation. The fix rejects
lags whose implied rate falls outside [6, 36] br/min before they can win the
autocorrelation search, and refuses to certify any value `clamp_rr()` had to move.

The measured consequence: RR now publishes **0.0** (unavailable) on a good
contact. In a 90 s finger-on capture the HRV window reached 94 accepted intervals
— comfortably past the 30 the estimator needs — so it ran, but returned
`is_reliable = false` because confidence stayed under the 0.60 bar. The most
sensitive term in NEWS2 is therefore currently always absent.

That is safe and honest, but not useful. `conf_fm = best_r`, the raw
autocorrelation coefficient at the winning lag, is a weak confidence measure on
real PPG, where RSA depth is small. Options, cheapest first:

1. Earn confidence from RSA depth rather than from the ACF peak.
   `rsa_depth_ms` is already computed: a subject with under ~15 ms of
   respiratory modulation should report unavailable, and one with clear
   modulation should not be held back by a modest `best_r`.
2. Treat the FM and AM estimates as corroborating evidence — when both
   independently agree within a few br/min, that is worth more than either
   coefficient alone.
3. Validate against paced breathing at 6/min and 15/min **before** tuning any
   threshold. Without that, any change here is curve-fitting.

Do not simply lower the 0.60 threshold: that reinstates exactly the class of bug
54d9a27 removed.

---

## Tier 4 — Validation and evidence

### T4.1 — Validate against a reference device

**The last real gap.** Everything so far is internally consistent; nothing is verified against
ground truth. It converts *"our RMSSD reads 38 ms"* into *"our RMSSD agrees with ECG-derived RMSSD
to within X ms."*

**A Polar H10 (~₹8,000) is the textbook answer and the wrong buy here.** It is a one-time check
for a hackathon, and the same result is reachable for well under ₹1,000. Options, cheapest first:

| Option | Cost | What it gives | Catch |
|---|---|---|---|
| **Physiological plausibility tests** | ₹0 | RMSSD responds correctly to supine→standing (drops) and 6/min paced breathing (rises, RSA). Robust, well-documented effects. | Not absolute calibration — but it *is* evidence the metric tracks autonomic state. |
| **Samsung Watch (already owned)** | ₹0 | Continuous HR agreement; **ECG mode is raw 500 Hz** if you write the Wear OS app | ECG is on-demand, ~30 s, single-lead, needs Samsung app verification |
| **AD8232 ECG module** | **~₹700** | **Real single-lead ECG** — an independent modality, which is the actual scientific requirement | Needs careful analog work; good at rest, mains hum and motion are real |
| Borrow a chest strap | ₹0 | Same as H10 | Ask the college BME/EE lab, a gym, or a classmate |
| Fingertip oximeter | ~₹1,500 | Independent HR **and SpO₂** — your two headline vitals | No HRV |
| Polar H10 | ~₹8,000 | Gold-standard convenience, raw RR over BLE HRS | Overkill for one validation |

**Recommended stack: AD8232 (~₹700) + fingertip oximeter (~₹1,500).** Under ₹2,200 total, it
covers HRV (ECG, independent modality) and SpO₂ (independent device), and the AD8232 stays useful
as a demo component — *"we built our own ECG reference"* is a better story than *"we bought a
Polar."* A borrowed strap costs nothing and is strictly better if one is going.

**Do not use the watch's PPG-derived HRV number as ground truth.** That is wrist PPG validated
against finger PPG — the same sensor technology agreeing with itself, with shared failure modes.

**Method matters more than the device** (see [Sarhaddi et al., *PLOS ONE* 2022](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0268361) for a worked comparison):
simultaneous recording, resting supine 5 minutes, compare RMSSD/SDNN/mean HR, and report
**Bland–Altman limits of agreement — not just correlation.** A high *r* with wide LoA is worthless.
Disclose the artefact-removal rate, since the reference does not remove anything.

> **If none of this happens, do not claim validation.** State it as a limitation — the README
> already does. A scoped honest claim is worth more than a weak validation, and a judge will
> respect it more.

---

### T4.2 — Place your HRV in Indian normative range

**Currently at ~38 ms RMSSD, resting.** Cite [Singhal et al., *Annals of Neurosciences* 2026,
"Normative Heart Rate Variability Parameters Across Age and Gender in Healthy Adults from an Apex
Tertiary Care Centre in Southern India"](https://www.semanticscholar.org/reader/b2c64d4062c77c8a8e31226eb6fbc7a808b0ff87)
and state that the value sits inside the Indian normative range for the subject's age band.

This answers the question a judge is most likely to ask about MIMIC-III: *"that's a Boston
dataset — does it apply to Indian patients?"* ~2 hours.

---

### T4.3 — India-specific threshold sourcing

- **PM2.5 risk** → [GBD 2019 India, *Lancet Planetary Health* 2021;5(1):e25–e38](https://pubmed.ncbi.nlm.nih.gov/33357500/) instead of US-derived figures.
- **Heat thresholds** → **IMD criteria** (heat wave = ≥40 °C plains, ≥37 °C coastal, ≥30 °C
  hills), not the NWS/Rothfusz assumptions. Reference the [Ahmedabad Heat Action Plan](https://www.exemplars.health/stories/ahmedabad-indias-heat-action-plan).
- **MIMIC limitation** → state plainly that the ML head is trained on US ICU data and is not
  claimed to be India-validated, while the interpretable rule engine uses Indian and WHO sources.

~3 hours. Turns the project's biggest liability into evidence of rigour.

---

### T4.4 — Re-fit before quoting any FPGA resource figure — **DONE (2026-09-21)**

The 2026-09-21 fit is current: **363 / 1120 LUT5s (32.41%)**, 202 FFs, 75/140 CLBs,
PLL 0/1. It is the first run to include the H-01 slope gate.
`resource_utilization_spi_link.log` and every doc that quotes a footprint have
been updated to it. The older 443 (parallel link), 342 and 222 figures are all
real measurements of earlier netlists — see
`hardware/shrikefi/synthesis_evidence/README.md` for the history.

**One caveat carried forward:** the design declares no clock constraint, so the
fitter auto-constrains `clk` to 500 MHz and reports WNS −10.088 ns, which is
meaningless. Achievable period is 12,087 ps (82.73 MHz); at 50 MHz the margin is
+7.913 ns. Adding a real constraint to the flow is still open work — until then
the timing report cannot be cited either way.

---

### T4.5 — Give the FPGA flow a real clock constraint

Split out of T4.4 because the re-fit is done but this is not. `forgefpga_pins.pcf` declares only
`set_io` lines, so the fitter auto-constrains `clk` to 2000 ps (500 MHz) and every register in the
design "fails". The reported WNS of −10.088 ns is therefore unusable in either direction — it is
not evidence that timing fails, and not evidence that it closes.

The design's achievable period is 12,087 ps (82.73 MHz), so at the documented 50 MHz the margin is
+7.913 ns. That is a good sign, not a result.

**Do:** find whether the Renesas flow accepts a frequency constraint (an SDC, or a `set_frequency`
in the PCF — the repo's reference docs under `docs/reference/shrike_board/` do not say), apply
50 MHz, and re-run. Until that exists, quote the achievable period and state that the constraint
is missing rather than quoting WNS.

Do **not** invent a constraint syntax and commit it without re-running the fitter — a wrong
constraint is worse than none, because it produces a confident number.

---

## Out of scope — and why

| Item | Why not |
|---|---|
| **GPS** | Wrong sensor for this problem. Under debris, indoors, in a cyclone shelter there is no fix. A stored location plus a rescuer's phone is more reliable, and requirement 6 says location is *"when permitted by the user"* — it is explicitly optional. |
| **Fall detection without an IMU** | Not possible. Do not claim it. |
| **Sleep staging** | Requires overnight wear and an IMU. A one-line honest scoping statement is better than a thin implementation. |
| **Flood immersion sensing** | No water sensor. The code already reports an ambient proxy, capped at `RISK_HIGH`, and labels it `(ambient proxy)` in the log. **That is the correct honest answer — keep it.** |
| **Retraining the ML head on NFHS/ICMR-INDIAB** | Those are population surveys with different variables and no ICU physiology. You would be fitting noise, and it would be obvious to anyone who checks. |

---

## Requirement coverage

| # | Requirement | State today | What still closes it |
|---|---|---|---|
| 1 | Continuous monitoring | HR, SpO₂, RR, SQI — **body temperature not measured** | **T2.1** (MAX30205, ~₹200); activity + sleep need T2.3 |
| 2 | AI anomaly detection | **Strong** — real RR in NEWS2, SQI gating, INT8 TinyML head | fall detection via T2.3 |
| 3 | Disaster alerts | Heat + air quality good; **flood/cyclone not instrumented** | **T3.1** — free, BME280 pressure trend |
| 4 | Environmental awareness | **Strong** | — |
| 5 | Privacy-preserving edge AI | **Satisfied** — cloud publishing off by default | — |
| 6 | Emergency assistance | **Partial — SOS latch + OLED emergency card + local status page shipped (T1.2, T1.3)** | **T1.4** (stored location), a POST trigger endpoint, and a button (T2.2) |
| 7 | Wellness dashboard | Live only, no history or trends | T3.2 |
| 8 | Scalable deployment | Roadmap only | — |

---

## Suggested order

```
DONE (2026-09-21):  T1.1 MQTT privacy · T1.6 badge · T4.4 re-fit
DONE (2026-09-28):  T1.2 SOS state machine + OLED card · T1.3 SoftAP status page
                    T1.5 RR/SQI verified on hardware

Still open, no parts needed — in this order:
  1. T1.4  Stored location in NVS
  2. T3.1  BME280 pressure trend         <- free cyclone advisory
  3. T3.4  Correct the docs that still describe the retired 4-bit parallel link
  4. T3.5  Give the respiratory rate a confidence measure that works on real data
  5. T3.3  Handover log spam (~30 min)
  6. T1.7  README images (blocked on images from the user)

With parts (roughly ₹450 total):
  7. T2.1  MAX30205 skin temperature     <- highest-value part on this list
  8. T2.2  SOS button + buzzer           <- also unlocks the phone POST trigger
  9. T2.3  IMU (activity + sleep + falls)

Before submission:
 11. T4.1  Chest-strap validation        <- the only thing that changes what you can claim
 12. T4.2  Indian normative HRV range
 13. T4.3  India-specific threshold sourcing
 14. T4.5  Real clock constraint for the FPGA flow (see T4.4 caveat)
```
