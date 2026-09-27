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

### T1.4 - Stored location in NVS  DONE (2026-09-28)

Requirement 6 bullet 3, with no GPS. `firmware/shrikefi/location.{h,c}` stores one short
location string in NVS and shows it on both the emergency card and the local status page.

- **Set from a phone.** The status page carries a form that POSTs to `/location`; the body is
  decoded as `application/x-www-form-urlencoded`, so a keyboard's `Village Hulimavu, Kolar`
  arrives intact through `+` and `%XX`. A phone on the access point is the only input device
  this system has - there is no keypad, and a serial console would reintroduce the laptop and
  cable the local page exists to remove.
- **Sanitised, not trusted.** Leading and trailing whitespace is trimmed, interior whitespace
  collapses to one space (dropping it outright would weld `Ward 3 Kolar` into `Ward3Kolar`),
  and anything outside printable ASCII is discarded - a control byte from a terminal, or a
  UTF-8 continuation byte whose lead byte was dropped, would otherwise corrupt the display or
  split a glyph. Text that sanitises to nothing is **rejected with HTTP 400** and the stored
  value is left alone, rather than silently wiping a location already in use.
- **Persisted in spirit as well as in fact.** The in-memory value is updated before the NVS
  write, so a location set during an emergency is still visible this session even if the flash
  write fails; the failure is logged rather than swallowed.
- **Card and page differ on purpose.** The 128 px card has 16 usable columns after its `LOC: `
  prefix and the page has room for anything, so `location_card_text()` shortens for the panel
  and marks it with `...` rather than cutting a district name in half with no indication. A
  reader who sees the marker knows to check the phone page for the full string.

**Verified:** `test_location_store()` covers trimming, whitespace collapsing, control-byte
rejection, buffer-bounded truncation, `+`/`%XX` decoding, the field-boundary rule (`bloc=` is
not `loc=`), the card-shortening rule at 16 and 17 characters, and that a value sanitising to
nothing does not clear what is stored. That test immediately caught a real bug: the field
search used a key length of 5 for `"loc="`, so `strncmp` compared the value's first byte as
well and the setter silently never matched anything.

On hardware, end to end: a fresh device reported `loc = UNSET`; `POST /location` with
`loc=Village+Hulimavu%2C+Kolar` returned **303** and the page then reported
`Village Hulimavu, Kolar`; a whitespace-only POST returned **400** and left the value unchanged;
and after a **reboot** the page still reported `Village Hulimavu, Kolar`. The emergency card was
then re-checked pixel by pixel against the real font with that location stored - `EXACT MATCH`,
`LOC: Village Hulim...` filling exactly 21 columns with no clipping.

---

### T1.5 — Verify the respiratory-rate wiring on hardware  ✅ **DONE (2026-09-28)**

Verified on the device. `SQI=` appears in the telemetry line and reads a plausible
0.86–0.92 with a good contact; `RR=` appears and is correctly published as `0.0`
("unavailable") rather than the fixed normal 14, so NEWS2 is no longer scoring a
defaulted respiratory term. Run with a finger on for 90 s: 84–94 intervals accepted.

**The verification produced a finding rather than a pass.** RR never resolves to a
value at all on real data - see **T3.5**, which is the follow-on: the estimator runs
(it has far more than the 30 intervals it needs) but its confidence measure, the raw
autocorrelation peak, does not reach the reliability bar on real finger PPG.

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

### T3.1 - Barometric pressure trend -> cyclone advisory  DONE (2026-09-28)

Free: the BME280 was already measuring pressure and the firmware read it on every
poll and discarded it. `firmware/core/pressure_trend.{h,c}` keeps a window of the
last three hours (one stored sample a minute, 180 samples, 1.4 KB) and fits a
least-squares slope to it; `assess_cyclone_risk()` turns that rate into a hazard
level alongside heat, pollution and flood. Closes the cyclone half of requirement
3's last gap.

**Why the trend and not the reading.** Absolute pressure is nearly useless for a
weather advisory - sea-level pressure varies by tens of hPa with altitude and
season, so any fixed threshold fires in the hills and stays silent on the coast.
The rate of fall is the signal, which is why this keeps a history at all.

**Thresholds**, from the standard reading of a barograph: about 1 hPa/hr is a
developing low, 3 hPa in 3 h is a shipping-forecast storm warning, and 5 hPa in
3 h is a rapidly deepening system. Expressed as a rate because that is what the
fit measures, and paired with a **0.5 hPa minimum drop** so noise cannot reach a
threshold on its own.

**That drop floor is load-bearing, and the hardware proved it.** With the window
temporarily shortened to 2 s samples to exercise the path end to end, the fitted
rate swung between **-5.99 and +3.25 hPa/hr** on a barometer that was visibly
steady to one decimal place - a 2 s cadence resolves the BME280's own jitter into
an alarming slope. Rate alone at -5.99 hPa/hr is past the -2.0 critical threshold
and would have raised a cyclone warning. The actual fall over the window was
about 0.05 hPa, far below the 0.5 hPa floor, and the classification correctly
stayed NORMAL throughout. A rate-only implementation would be crying cyclone on a
calm day.

**A design flaw the hardware also caught.** The assessment was first placed inside
the `vitals_ready` branch, which requires ten accepted beats. On the bench the
fitted trend was visibly moving while the published level stayed UNKNOWN forever -
because nobody had a finger on the sensor. Weather does not need a finger, and an
untouched device is exactly when a storm warning matters. It is now evaluated and
assessed every second, outside that branch, and only folded into the fused risk
when an assessment runs.

Also: `disaster_assess()` marks `cyclone_risk` UNKNOWN explicitly rather than
leaving the memset's zero, because zero is RISK_NORMAL and that would be an
all-clear for a hazard nobody evaluated. The overall fusion tallies UNKNOWN only
for the three core modalities - the storm trend is additional, so its UNKNOWN
means "no history yet", not "sensor missing", and counting it would have flipped
every all-normal verdict to UNKNOWN for the first half hour after boot.

**Verified:** `test_pressure_trend()` covers the slope sign convention, the
minimum-span gate, rejection of implausible readings (0, NaN, 5000 hPa), rejection
of a timestamp that would fold the window backwards, ring wrap keeping the newest
span, and the one-sample-a-minute pacing. `test_cyclone_risk()` covers the
threshold ladder, the drop floor, and that a rise is not scored. The page's storm
row was rendered against a stubbed endpoint in headless Chrome, including that a
falling trend renders with a minus sign and an unfilled window reads UNKNOWN
rather than a reassuring NORMAL.

On hardware: the trend computes from live BME280 data, the JSON carries
`pressure`/`ptrend`/`storm`, and with the shipping 30-minute window the advisory
correctly reads UNKNOWN until the window is long enough to mean anything.

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

### T3.4 — Rewrite the documents that still describe the retired 4-bit parallel link  DONE (2026-09-28)

**DONE (2026-09-28).** Every document listed above has been corrected, and the two
source-level artefacts with it: `shrikefi_link_driver.h` no longer carries a
`SHRIKEFI_CMD_*` codebook or a `shrikefi_pins_t` describing a strobe and a
direction line (neither was referenced anywhere), and `run_vivado_synth.tcl` now
reads the shared RTL from `../common/`, which it had silently failed to find since
the platform split - the script could not run at all.

Two things worth recording, because they are the reason this took a rewrite rather
than a find-and-replace:

- **The `.pcf` comment trap.** `forgefpga_pins.pcf` annotates its pads as
  `spi_sck PIN_16 # ... (F_SP_CLK) -> GPIO3_IN`. The `-> GPIO3_IN` suffix is an
  FPGA-side port designator, not an ESP32 GPIO number. Reading it as one is almost
  certainly where the wrong pin tables came from, and it is called out in the
  protocol document so the next person does not repeat it.
- **The figures could not be verified.** `shrikefi_pinout.png` and
  `shrikefi_waveform.png` were produced for the retired design, and their contents
  cannot be checked from the repository - the pinout SVG carries an embedded raster,
  so its labels are not searchable text. Rather than guess, the references were
  removed and the tables in `SHRIKEFI_LINK_PROTOCOL.md` are declared authoritative;
  both figures need regenerating from those tables before reuse.

**Still stale, deliberately untouched:** the point-in-time reports under `reports/`
(`CODE_REVIEW_COMPREHENSIVE_AUDIT.md`, `FPGA_PPG_WAVEFORM_ANALYSIS.md`) quote
superseded figures, and the Zynq-side `irq_beat` references in `README.md` and
`THEORY_NOTES.md` are correct for the Zynq accelerator, which genuinely has that
interrupt - do not "fix" those. `AGENTS.md` was also corrected but is gitignored,
so that fix is local to this checkout.

---

### T3.5 - Respiratory-rate confidence  IMPLEMENTED (2026-09-28), hardware confirmation PENDING

The estimator ran correctly on real contacts - 84 to 94 accepted intervals, well
past the 30 it needs - and still published RR as `0.0` (unavailable), because its
confidence WAS the raw autocorrelation coefficient, which on real finger PPG peaks
around 0.3-0.5 and so never cleared the 0.60 bar. The most sensitive term in NEWS2
was therefore permanently absent.

**The fix, in two parts.**

1. **Confidence now answers two separate questions**, because one coefficient was
   being asked to answer both:
   - *is the series periodic* at the winning lag? (the autocorrelation, rescaled so
     0.25 maps to 0 and 0.60 to 1)
   - *is the modulation big enough* to be respiratory? (RSA depth)
   FM/AM agreement within 3 br/min is credited on top, since those are independent
   measurements of the same physiology and their agreement is evidence neither
   coefficient carries. **The 0.60 bar itself is unchanged** - lowering it would
   reinstate the railed-36 bug that 54d9a27 removed.

2. **RSA depth is now `2*sqrt(2)*sigma` over the detrended series, not `max-min`.**
   A range is set by the single worst pair of beats in the window and grows with
   sample count, so thirty beats of pure jitter read as "30 ms of RSA" - exactly
   the confusion the depth term exists to prevent. For a true sinusoid the
   estimator recovers the peak-to-peak exactly (verified: 40 ms injected, 39.9 ms
   measured).

**A worse bug that the change exposed.** Adding the depth term made the drift
reproduction - a healthy 78 bpm subject with 55 ms of slow drift and **no
respiratory modulation at all** - report **26 br/min at 0.97 confidence**. The old
code reported an unreliable 36 for the same input; the new code reported a
*confident* wrong number, which is worse because a confident number gets acted on
in NEWS2's most sensitive term.

Root cause: the estimator removed only the MEAN. A slow drift in heart rate is not
respiration, but it correlates strongly with itself at short lags and therefore
wins the autocorrelation search. It now removes the best-fit straight line first,
which is what separates a trend from an oscillation and makes the depth measure
mean anything. Same input now yields `conf=0.51, reliable=0`.

**Verified by unit test:**

| case | result |
|---|---|
| real RSA, 15 br/min, 40 ms modulation | `rr=15.0` (true 15), `conf=1.00`, **published** |
| the drift reproduction (no modulation) | `conf=0.51`, **not published** (was 26.05 @ 0.97) |
| white jitter, 8 seeds | **0 of 8 published**, worst confidence 0.48 |
| the old rail reproduction | no longer rails, and now correctly refuses to publish |

**NOT YET CONFIRMED ON HARDWARE.** Two 90-100 s captures both came back entirely
`NO_FINGER` (`IR=0`), so the triage branch never ran and RR could not be
evaluated. The outstanding question is the one the unit tests cannot answer:
whether real finger PPG produces enough RSA depth and enough autocorrelation to
clear the gate. Until a capture with a finger on the sensor shows a plausible RR,
this stays open.

**And the thresholds are a first calibration, not measurements.** They come from
first principles and the RSA literature (adult resting RSA is typically 20-60 ms
peak-to-peak). They have not been validated against paced breathing. Doing that -
6/min and 15/min against a phone timer, checking the estimate lands within 1-2
br/min - is what would turn these numbers into evidence rather than guesses, and
it is still the right next step for this task.

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
| 3 | Disaster alerts | Heat + air quality good; **cyclone done via the pressure trend (T3.1)**; flood still not instrumented | a water sensor; the flood path still runs on an ambient cold-stress proxy |
| 4 | Environmental awareness | **Strong** | — |
| 5 | Privacy-preserving edge AI | **Satisfied** — cloud publishing off by default | — |
| 6 | Emergency assistance | **Partial — SOS latch + OLED emergency card + local status page + stored location shipped (T1.2-T1.4)** | a POST SOS trigger endpoint, and a button (T2.2) |
| 7 | Wellness dashboard | Live only, no history or trends | T3.2 |
| 8 | Scalable deployment | Roadmap only | — |

---

## Suggested order

```
DONE (2026-09-21):  T1.1 MQTT privacy · T1.6 badge · T4.4 re-fit
DONE (2026-09-28):  T1.2 SOS card · T1.3 SoftAP status page · T1.4 stored location
                    T1.5 RR/SQI verified · T3.4 documentation corrected

Still open, no parts needed — in this order:
  1. T3.5  Give the respiratory rate a confidence measure that works on real data
  2. T3.3  Handover log spam (~30 min)
  3. T1.7  README images (blocked on images from the user)

With parts (roughly ₹450 total):
  4. T2.1  MAX30205 skin temperature     <- highest-value part on this list
  5. T2.2  SOS button + buzzer           <- also unlocks the phone POST trigger
  6. T2.3  IMU (activity + sleep + falls)

Before submission:
 11. T4.1  Chest-strap validation        <- the only thing that changes what you can claim
 12. T4.2  Indian normative HRV range
 13. T4.3  India-specific threshold sourcing
 14. T4.5  Real clock constraint for the FPGA flow (see T4.4 caveat)
```
