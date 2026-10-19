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

**Still open in requirement 6:** the physical emergency button and its wiring (T2.2). The phone
SoftAP page is implemented and includes manual SOS trigger/cancel controls; current firmware
build and browser fixture checks pass, but this audit cannot repeat live HTTP checks because
the ESP32-S3 is disconnected. Stored location (T1.4) is complete.


---

### T1.3 - SoftAP + local status page  DONE (2026-09-28)

**This is the strongest available demo and it needs no router.**

`firmware/shrikefi/web_status.{h,c}` raises an open access point and serves a live status page
from the device itself. The dashboard that existed before this needed a PC, a USB cable and a
browser gesture - the opposite of a field device. Now a rescuer's phone joins `VALOR-xxxx` and
reads the patient's status directly.

- `GET /` serves a compact phone-sized page embedded in `web_status.c`; it is a separate UI from the USB Web Serial HTML file.
- `GET /status.json` serves the whole snapshot: HR, SpO2, RR, SQI, RMSSD, temp, humidity,
  PM2.5, NEWS2, level, flags, fused risk, SOS state and trigger, location, uptime, contact,
  ambient pressure/trend, the MAX30102 finger-temperature estimate, motion and fall state.
- The page **derives nothing** - displayed status is computed on the device. It is intentionally
  smaller than the USB Web Serial dashboard and is not generated from that HTML file.
- The AP is **open by design** so a bystander does not need a credential in an emergency.
  This is a privacy tradeoff: anyone in WiFi range can read the live vitals and location.
  The `/location` and SOS controls require a same-origin browser request to limit cross-site
  drive-by actions. An exact `Origin` header is authoritative; when it is absent, an exact
  same-host `Referer` is accepted. This is not authentication; a client deliberately connected
  to the AP can still read the status and change the location or SOS state.
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

**Software follow-up (2026-10-15):** added phone SOS trigger/cancel POST handlers, same-origin
checks with exact host-boundary validation, strict script CSP, and no-cache/offline browser behavior.
The history endpoint no longer grants wildcard CORS reads. Both the status formatter and origin
check have host regressions. Extracted-page browser checks cover live values, SOS button feedback,
no-finger state, and stale-data clearing on fetch loss. The board was disconnected for this
follow-up, so the newer HTTP handlers and live WiFi path have not been exercised on hardware.


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
- **Same-origin browser write.** `/location` accepts an exact
  `Origin: http://192.168.4.1`; if Origin is absent, it accepts only a `Referer` with that exact
  host followed by `/` or end-of-string. A mismatched Origin is rejected even if Referer matches.
  This limits cross-site browser form submissions but is not an access-control boundary against
  a client that joins the intentionally open AP.
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

**The verification produced a finding rather than a pass.** At the time RR never
resolved to a value at all on real data - see **T3.5**, which is now resolved: three
estimator defects were found and fixed, and a paced staircase subsequently confirmed
that the estimator tracks respiration (cued 10 and 24 br/min reported 11.3 and 26.9).

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

### T2.2 — SOS button + buzzer (Hardware pending; Software done)

- Button: any spare GPIO (`3, 4, 5, 6, 7, 15, 16, 17, 21`), input with pull-up. The BOOT button (GPIO0) is supported via `CONFIG_SHRIKEFI_SOS_CANCEL_GPIO` out of the box.
- Buzzer: any spare GPIO (passive buzzer on LEDC for a two-tone alarm).
- Pattern: intermittent tone + LED strobe, distinct from the normal heartbeat LED.

Reaches people **physically nearby** — in a disaster, that is exactly who matters. ~1 hour plus
₹50 of parts.

**Software implementation DONE (2026-10-04):** `CONFIG_SHRIKEFI_SOS_CANCEL_GPIO` supports hardware cancel with 5s hold. Simulated in `host_test`.

---

### T2.3 — IMU (accelerometer)

Closes three gaps with one part: **activity levels, sleep quality, and fall detection**.

- Part: **LSM6DS3** or **MPU6050**, on the existing I²C bus. No new pins.
- Fall detection: free-fall → impact → post-impact inactivity, a well-understood threshold
  algorithm. Demos extremely well.
- Sleep: activity + stillness over hours, with a resting-HR/HRV component. Overnight wear needed.

Cost ~₹200. Effort ~4 hours. **Closes requirement 1 (activity, sleep) and requirement 6
(falls).** Physical fall/SOS trial on 2026-10-19: **1/1 passed** (10 s countdown,
buzzer, 5 s hold-to-cancel all verified). Still open: multi-trial/orientation
drops and false-positive characterisation.

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

### T3.2 — Dashboard history and daily summaries  ✅ **DONE (2026-10-19)**

Requirement 7 asks for *"daily health summaries and trend analysis"*. The dashboard is no
longer live-only: `web_status.c` keeps 24 hourly slots and the standalone page renders a
24-hour trajectory chart with mean/nadir/peak summaries.

**Persistence (2026-10-19).** The 24 completed hours are now written to NVS as a single
canonical 612-byte blob (12-byte header + 24 × 25-byte records: little-endian fields, FNV-1a
checksum, magic, version). One write happens when an hour rolls over, outside the status lock;
`web_status_history_init()` restores the saved array at boot and shifts it one slot older, so a
partial hour lost to power loss is dropped rather than promoted. It is still **not a calendar
record**: the board has no wall-clock time source, so each slot is an hour of device uptime and
the UI labels it `DEVICE HOURS`. The blob codec is host-tested (round-trip, checksum, bad
magic, truncation, too-small buffer) along with the hourly rollover and the zero-skip averages.

**Board check — DONE (2026-10-19, COM4).** A 302 s still-contact capture confirmed the
**on-device RAM history feed**: `[HISTORY] H0` published nine coherent running averages
(HR 74.9–76.8, SpO₂ 96.4–98.1, RMSSD 35.9→67.0, NEWS2 0–1). The **NVS write/restore round-trip
was then verified** on a session with over an hour of uptime: a rollover wrote the blob, and a
reboot logged `Restored NVS history: 2 of 24 slots populated, 1 with a heart-rate average`. The
blob also survived a re-flash (bootloader + partition table + app rewritten; the NVS partition at
`0x9000` was untouched). The restore message now reports populated slots rather than the 24 the
blob always encodes — the first version logged "24 completed hourly summaries" after minutes of
uptime, which over-claimed. Still open: only a multi-hour session can populate enough slots to
exercise a full 24-hour window and the oldest-slot drop.

**Ring wrap and eviction — DONE (2026-10-20, COM4).** The oldest-slot drop was exercised directly
by building the firmware with `VALOR_HISTORY_HOUR_SECONDS = 60` (a bench-only override; the shipped
default is 3600 s and the blob does not store the value). Over a 1560 s run with a finger present
early: 25 NVS writes at 58.5–60.8 s spacing (mean 59.9 s, one per completed hour), ring growth
3 → 24 slots at t ≈ 1281.8 s, then a steady 24 slots while the hour that had occupied H23 was
evicted at the next rollover. Heart rate survives the round-trip: `dbg blob` showed `hr_slots`
growing 2 → 3 → 4 as contact hours were added, and two separate reboots restored
`24 of 24 slots populated, 5 with a heart-rate average` with identical HR/SpO₂/RMSSD/NEWS2 values
(hours shifted two slots older on the second boot because the ring had rolled twice). Evidence:
[`reports/HISTORY_RING_FASTHOUR_2026-10-19.md`](reports/HISTORY_RING_FASTHOUR_2026-10-19.md).
The bench blob was then erased and the stored location restored by writing a generated NVS image
(`valor/loc`) at `0x9000`. **Open anomaly:** one boot right after that long run reported
`24 of 24 slots populated, 0 with a heart-rate average` although the in-RAM ring held an HR hour in
H23; it did not reproduce in three later restores and is recorded rather than explained. A future
multi-hour soak should watch for it. Still open: a genuine 23–24 h window with a calendar time
source.

---

### T3.3 — Cosmetic: log spam on repeated handover  ✅ **DONE (2026-10-06)**

When the finger is off, the FPGA still reported spurious crests (the scaler toggling between 0 and
120 around the `raw < 1000` threshold), and each one re-triggered a handover log line.
**Fixed:**
- In `main_shrikefi.c`, `ibi_pipeline_submit()` now gates the handover log on `old_source != IBI_SRC_NONE`, so waking up from finger-disconnect reset never logs spurious handovers.
- The sample stream writes zero (`shrikefi_write_red_sample(0)`, `shrikefi_write_ir_sample(0)`) whenever `!optical_contact`, preventing ambient sensor noise around threshold from driving the FPGA filter and peak detector.
- `shrikefi_link_reset_beat_tracking()` and `shrikefi_clear_irq()` clear any residual beat state on contact loss.

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

### T3.5 - Respiratory-rate confidence  RESOLVED (2026-09-28): the estimator tracks respiration

**The question is answered: respiration-from-IBI works on this hardware.** A paced
staircase run - 50 s settle, then 10 br/min for 65 s, then 24 br/min for 65 s, all in
ONE run so the subject's own drifting baseline appears in neither clean window -
produced this from the fully-established windows:

| phase | cued | reported p50 | frames published | conf p50 |
|---|---|---|---|---|
| A settle | - | never published | 0 of 14 | 0.50 |
| **P1** | **10 br/min** | **11.3** | 19 of 27 | 0.70 |
| **P2** | **24 br/min** | **26.9** | 14 of 27 | 0.61 |

A 14 br/min change in the cue produced a **15.6 br/min change in the estimate**. It
tracks. The ~0.1 Hz Mayer-wave reading that the earlier mitigation was built on is
refuted, and the whole A-B-A approach that preceded this was the problem - see below.

**Why the earlier runs could not answer it.** Three A-B-A runs (settle / paced /
recover) all failed, for reasons that had nothing to do with the estimator:

- The subject's un-paced rate is not stable between runs: **14.1 br/min** in one
  capture, **22.9** twenty minutes later. Comparing "paced" against "recovery"
  therefore compares two different things every time, and a paced rate that lands on
  the day's baseline is indistinguishable from no tracking - which is what happened
  when 15 br/min was used against a resting 14.1.
- 24 br/min for 70 s is hyperventilation. It drove the measured HR from 70 to 93, so
  the recovery window that followed was still perturbed and could not serve as a
  baseline.
- Telemetry runs at **0.89 Hz, not 1 Hz** (125 frames over 140 s). Reading the
  capture by frame index rather than elapsed time put the phase boundaries ~13 s out
  and made a warm-up artefact look like a measurement. The capture script now records
  elapsed time per row and writes a CSV.

The fix is structural: put two very different paced rates in one run. Whatever the
subject's own rate is, it is in neither clean window.

**A remaining ~+12% high reading, not an estimator bias.** P1 read 11.3 for a cued 10
and P2 read 26.9 for a cued 24 - both about 12% high. A synthetic probe at the same
rates, depths and beat-to-beat noise (`.capture/bias_probe.c`) reproduces **nothing**
of the sort: with adequate RSA depth the estimator is accurate to **under 1 br/min**
from 8 to 24 br/min, typically under 0.5. So the offset is most likely the subject
anticipating the cue, or a real-RSA waveform shape the sinusoid model does not
capture. Separating those needs an independent measurement of actual breathing, i.e.
**T4.1** - it cannot be settled by more paced runs.

**Band edges were broken, and are fixed** (`2164704`). Gating the autocorrelation
*search* by the band did not make the estimator refuse out-of-band rates; it made it
snap to the nearest admissible lag and report that. A genuine 8 br/min peaks at lag
8.5; with that excluded the winner was lag 7 (9.74), the parabola clamped, and
**9.09 was published as RELIABLE** - severe bradypnoea (NEWS2 +3) reported as mild
(+1), with the `clipped` guard never firing because 9.09 is just inside the band. The
header had claimed since the mitigation that "below 9 br/min this now reports
unavailable"; it did not. The band is now applied to the interpolated peak as a
verdict rather than as a search filter, so 8 br/min correctly reports unavailable and
10 and 12 still publish accurately (`test_rr_band_edges` asserts both directions).

**The breath-hold test: there is no Mayer wave, but the floor stays at 9.0 anyway.**

Run 2026-09-28 with the floor temporarily at 6.0 - necessary, because at 9 the estimator
refuses the band and the test could not tell a Mayer wave from nothing. Rationale:
respiratory sinus arrhythmia is driven by breathing and vanishes during apnoea, while a
baroreflex Mayer wave does not.

| phase | frames | RR>0 | RR p50 | depth p50 | conf p50 | HR p50 |
|---|---|---|---|---|---|---|
| A settle | 10 | 0 | - | 135.5 | 0.50 | 66.0 |
| **B HOLD** | 14 | 10 | **27.4** | 273.5 | 0.66 | **82.0** |
| C recover | 14 | 0 | - | 134.1 | 0.50 | 69.1 |

**The 6-7 br/min reading did not reappear.** During 40 s of genuine apnoea the estimator
published no slow rhythm at all, so there is no Mayer wave and the floor was guarding
against nothing. The original 158 ms observation was almost certainly the octave error
since fixed.

**But the same hold produced a reliable 27.4 br/min** - an emergency-grade tachypnoea -
while the subject was not breathing at all. HR rose 66 -> 82 (straining to hold), and
the estimator locked onto a 3-beat rhythm in the inter-beat series: 27.4 br/min at an
82 bpm heart rate is a 2.2 s period, three beats. The depth term was saturated at
273 ms, so the published confidence of 0.66 rested mostly on amplitude rather than on
periodicity. The failure mode is therefore **not confined to the slow end** - this
estimator will report a non-respiratory rhythm confidently at either end of the band.

So 9.0 stands, but for a different and better reason than the one it was set for: not
"avoid the Mayer band" but "this estimator will believe a rhythm that is not
breathing, so keep the band narrow, and NEWS2 scores <=8 as +3 so a false bradypnoea is
expensive." The band gate is now a clean refusal rather than a snap to the nearest lag,
so 7 br/min reports unavailable rather than as a confident 9.

**The false tachypnoea: found, and fixed.** The [RRDIAG] line added for the purpose
(see below) showed the estimator picking **lag 2 in 42 of 108 frames** - every second
of the breath hold and 25 s of normal breathing afterwards. Lag 2 is the first lag the
search scans, so it wins whenever its correlation clears the bar, and at the capture's
861 ms IBI it is 34.8 br/min: inside the band, so the `clipped` guard never fired, and
it was published as a reliable 27.9-30.4 br/min.

Three separate causes had to be fixed, none of which was visible from the rate alone:

1. **Lag 2 is not respiration.** A respiratory cycle occupying two heartbeats cannot be
   resolved by any beat-to-beat method - RSA needs several beats per breath, and at two
   beats the modulation is indistinguishable from beat *alternans*, an alternating
   long-short interval pattern that is a common artefact of PPG peak detection.
   `PPG_RR_MIN_LAG` now bars lag 2 from winning. It is still computed, because lag 3's
   parabola needs it as a neighbour.
2. **The harmonic search had to obey that bar too.** Barring lag 2 in the main search
   was not enough: with a winning lag of 4, lag 2 is a legitimate L/2, so the
   sub-multiple walk put it straight back. Both paths now respect `PPG_RR_MIN_LAG`.
3. **Interpolation was walking out-of-band peaks back into the band.** The integer
   winning lag must now already be inside the band before interpolation may refine it.
   The file already held that "a rate that only exists because we clamped it is not a
   rate we measured"; interpolation deserved the same rule and did not have it.

`test_rr_rejects_alternans` reproduces it: a respiratory sinusoid with a dominant lag-2
alternans. Pre-fix it publishes **34.11 br/min** for a true 12; post-fix it reports
**11.36 at lag 6**. The alternans signature is now also logged as `r1`, the lag-1
correlation - measured at **-0.937** during the artefact, where smooth respiratory
modulation gives a positive value.

**The cost, honestly:** barring lag 2 caps the highest reportable rate at about
`60000/(3*IBI)` - roughly 23 br/min at 70 bpm. That is not a regression, it is the
method's real ceiling. RSA cannot measure tachypnoea at a resting heart rate, and
pretending otherwise is what produced the false emergency reading.

**Also found:** the IBI buffer needs ~45 s to reach the 30 beats the estimator requires,
not the ~25 s assumed - so the settle phase of every paced run was still warming up and
could never have produced a baseline measurement.

**How the mitigation was arrived at (and why its premise is now doubtful).**

Three captures with a finger on the sensor, taken before the estimator had been tested
against pacing at all. The paced-breathing test that would have settled it could not be
completed at the time - two attempts produced zero accepted beats in 100 s and then no
finger at all - so the mitigation below is a judgement from the depth evidence, not a
measurement, and it is reversible in one line.

**The first paced attempt (2026-09-28), and the two defects it exposed.** Superseded by
the staircase result at the top of this section, but the defects it found are the reason
that result was possible.

Run at 15 br/min with audible cues, 138 telemetry frames. It produced `RR = 16.9`
in the only three frames that published - but those three landed in the *recovery*
phase, because the triage line is gated behind `vitals_ready`, which needs SpO2 to
have latched, and SpO2 took 95 s of that 100 s run. RR had been computed the whole
time (`ppg_update_respiration_and_sqi()` runs unconditionally at 1 Hz); it was
simply never printed on the `ACQUIRING` line. Fixed in `0b2f3e2`.

Re-run with the fields visible, 140 s A-B-A (settle / paced 15 / recover):

| phase | frames | RR>0 | RR p25 | RR p50 | RR p75 | depth p50 | conf p50 |
|---|---|---|---|---|---|---|---|
| A settle | 37 | 0 | - | - | - | 0.0 | 0.00 |
| B PACED 15 | 58 | 46 | 17.2 | 17.4 | 18.6 | 276.3 | 0.64 |
| C recover | 36 | 34 | 13.6 | 17.2 | 17.2 | 172.1 | 0.66 |

B and C are indistinguishable: 17.4 against 17.2. But that is not evidence about
respiration - see below. Note the depth term *did* respond, 172 -> 276 ms, so the
subject did change their breathing.

**Defect 1: the reported rate could only land on a grid.** The autocorrelation was
sampled at integer beat lags and the period formed as `lag * mean_ibi`, so the output
was quantised to `60000/(k*mean_ibi)`. At a resting 800 ms IBI that grid is 25.0,
18.75, 15.0, 12.5, 10.71 - steps of up to 6.25 br/min and up to 3.1 br/min of error
at the midpoint between two lags. At the 880 ms IBI of this capture, **15 br/min sits
almost exactly between lag 4 (17.0) and lag 5 (13.6)**, so the B-versus-C comparison
was deciding which of two adjacent lags won, not measuring respiration. An
instrument coarser than NEWS2's own 21 br/min decision point, and coarser than the
3 br/min FM/AM agreement band this file grants itself, cannot answer the question.

**Defect 2: octave errors.** A periodic series correlates at every *multiple* of its
period. When the fundamental lag is not near an integer, the sampled value at the
fundamental is pulled down while a harmonic can still land near one. A clean
21.9 br/min modulation at 800 ms scores 0.68 at its true lag 3 but **0.89 at lag 7** -
so taking the global maximum is not a choice between equal evidence, it is a
systematic bias toward reporting **half the true rate**. Measured: true 21.88 ->
reported 10.72, true 19.89 -> 9.74.

**This is why the T3.5 evidence needs re-reading.** The original mitigation rests on
session C reporting 6.6-7.3 br/min. At that capture's 880 ms IBI, 6.8 br/min is lag
10 - and lag 10 is also the **second harmonic of a genuine 13.6 br/min respiratory
rate**. Before the floor was raised, `PPG_RR_MIN_BPM` was 6.0, so lag 10 was
admissible and could win. The 6.6-7.3 observation is therefore equally consistent
with an octave error on normal breathing as with a Mayer wave. The 158 ms depth is
still unexplained by respiratory RSA (adult RSA is 20-60 ms) and is the remaining
argument for the Mayer reading - but it is no longer sufficient on its own.

**Fixes** (`72de249`): the correlation is stored at every lag and a parabola fitted
through the three points around the winner, recovering the sub-lag peak (clamped to
+/-0.5 lag so refinement cannot migrate to another peak); and the winner is now the
*lowest* local maximum reaching `PPG_RR_HARMONIC_FRAC` (0.60) of the strongest
admissible one, i.e. the fundamental rather than a harmonic. Worst-case error over
ten off-grid rates at two IBIs: **3.1 -> 0.27 br/min**, and 21.9 no longer reads 10.9.

The old test could not see either bug: it used 15 br/min at an 800 ms IBI, which is
*exactly* lag 5. `test_rr_resolution()` places every rate at the midpoint between two
adjacent lags - worst case by construction - at two IBIs; against the pre-fix
estimator it fails 4 of 10, worst 11.16 br/min.

**The band floor stays at 9.0** - but not for the reason below. The breath-hold test in
the resolved section above showed there is no Mayer wave, and found something else
instead. Note that defect 2 also undermines the *original* justification: at session C's
880 ms IBI, 6.8 br/min is lag 10, which is the second harmonic of a genuine 13.6 br/min
respiratory rate, and lag 10 was admissible while the floor was 6.0. Session C's reading
is therefore equally consistent with an octave error on normal breathing as with a Mayer
wave.

**What the captures showed**

| session | triage frames | RR published | behaviour |
|---|---|---|---|
| A | 118 | 4 frames, 7.6 br/min | flickering |
| B | 84 | 0 frames | silent |
| C | 62 | 48 frames, 6.6-7.3 br/min | stable |

Session C recorded the estimator's own inputs, and they are the reason for the
mitigation:

```
RSA depth (equivalent sinusoid peak-to-peak):  p50 = 157.9 ms   min 73  max 179
published rate:                                6.6 - 7.3 br/min
```

Adult RESPIRATORY RSA is **20-60 ms** peak-to-peak. Three to five times that, with
the rate pinned at the very bottom of the accepted band, is the signature of the
**~0.1 Hz Mayer wave** - a genuine, strong, periodic blood-pressure oscillation in
the inter-beat interval which is not breathing. A floor of 6 br/min put Mayer waves
*inside* the accepted range rather than outside it. Every observation fits: the large
amplitude, the rate at the floor, and the high confidence, because the estimator is
correctly detecting a real oscillation - just the wrong one.

**The mitigation: `PPG_RR_MIN_BPM` raised from 6.0 to 9.0.**

Not tuned to the observation, but set at the point where NEWS2's respiratory term
leaves its severe band (<=8 scores +3, 9-11 scores +1). Respiration-from-IBI cannot
separate slow breathing from a vasomotor oscillation, and it is worst at exactly the
rates where the clinical penalty is largest. So: **do not score in the band where you
cannot discriminate.** Below 9 br/min this now reports unavailable, and NEWS2 holds
the respiratory term neutral instead of claiming bradypnoea. Because a value that
`clamp_rr()` has to move is never certified, the existing clipping rule does the work.

**The cost, stated plainly:** genuine breathing at 6-9 br/min is no longer reported.
That trade was made on the depth evidence, not confirmed by paced breathing, and one
line reverses it.

**Before this, an untrusted value was producing the maximum clinical penalty** - RR
publishing 7.2 br/min in 77% of frames drove NEWS2 to +3 bradypnoea. Silent is better
than confidently wrong.

**The test that settled it.** A staircase - settle, then 10 br/min, then 24 br/min, all
in ONE 180 s run - rather than the A-B-A pacing tried first. See the resolved section at
the top of T3.5 for the result and for why the A-B-A design could not work.
`.capture/paced_cue.ps1` drives the cues itself (measured accurate to <20 ms) and
`.capture/lock_check.ps1` checks for a working beat lock first, because the earliest
failures were all finger contact, not firmware.

**Also unresolved: the depth threshold is saturated.** Depth runs 73-179 ms against a
10-35 ms scoring window, so the depth term always contributes its maximum and
discriminates nothing. `PPG_RSA_DEPTH_NONE_MS` / `PPG_RSA_DEPTH_CLEAR_MS` need
re-scaling from paced-breathing data.

**What was implemented and unit-verified** (commits fb8deb9, 13a5361, this one):

- Confidence scores two separate questions rather than using one coefficient for
  both: is the series periodic at the winning lag (autocorrelation rescaled so 0.25
  maps to 0 and 0.60 to 1), and is the modulation big enough to be respiratory (RSA
  depth). FM/AM agreement within 3 br/min is credited on top. The 0.60 bar is
  unchanged - lowering it would reinstate the railed-36 bug that 54d9a27 removed.
- RSA depth is `2*sqrt(2)*sigma` over the detrended series, not `max-min`, because a
  range is set by the single worst pair of beats and grows with sample count.
- **A worse bug the change exposed:** with the depth term added, the drift
  reproduction - a healthy subject with 55 ms of slow drift and no respiratory
  modulation - reported **26 br/min at 0.97 confidence**. The estimator removed only
  the MEAN, so slow drift won the autocorrelation search; it now removes the best-fit
  line first. Same input is now correctly silent.
- `ppg_rr_tracker_t` holds the published rate until confidence falls decisively below
  the bar (0.45 rather than 0.60) and publishes the median of recent accepted
  estimates. The hold is BOUNDED at 10 frames, because an unbounded one would be a
  stale value published as current - the defect removed from the SpO2 engine earlier.
- `RRDEPTH` and `RRCONF` are appended to `[TELEMETRY]` so the estimator's inputs are
  visible whether or not RR publishes. That is what made this finding possible: the
  published rate alone could not distinguish little RSA from a bar set too high.

| unit case | result |
|---|---|
| real RSA, 15 br/min, 40 ms modulation | rr=15.0 (true 15), conf=1.00, published |
| 7 br/min oscillation (Mayer band) | not published |
| the drift reproduction (no modulation) | not published (was 26.05 @ 0.97) |
| white jitter, 8 seeds | 0 of 8 published, worst confidence 0.48 |
| tracker: sub-bar dip / sustained dip | held / retracts within PPG_RR_HOLD_MAX |

---

### T3.6 — Web dashboard demo polish  **APPLIED 2026-10-18**

The standalone Web Serial dashboard was audited and the safe, high-value fixes
were applied. `verify_dashboard.py` and `verify_softap_page.py` pass.

**Done in this pass:**

- Live **perfusion index** is now real: the firmware appends `PI=` to
  `[TELEMETRY]`, and the page parses and displays it.
- **Device verdict** chip uses the board's `RISK` and `FLAGS`, not a local
  reconstruction. Simulation clearly says "simulation profile".
- ECG sparkline and RR envelope carry a visible **SCHEMATIC** tag; the RR card
  shows real `RRDEPTH`/`RRCONF` in live mode and says "synthetic envelope" in
  simulation.
- Copy corrected: "normal rate" instead of "normal sinus rhythm"; heat gauge
  says "heat index + physiological strain"; ML metrics use 99.95% rule/NN and
  97.32% INT8/FP32, not 100%.
- Idle `rr-note` no longer prints the literal `&middot;`; `?sim=1` keeps the
  Simulation toggle so an interactive demo can return to idle.
- `readLoop()` no longer reports a serial error on an intentional Disconnect;
  connect failures distinguish cancel from a busy/blocked port.
- DOM/history updates are throttled to ~5 Hz while the canvases stay at 60 fps;
  the simulated PPG now advances phase per sample and pre-fills the scope.
- 24-hour history accuracy: SpO2/RMSSD are averaged over their own valid samples
  instead of being divided by every HR sample, an empty current hour no longer
  keeps stale clinical values, and the page no longer overwrites slot 0 with an
  instantaneous reading. The chart labels the record `DEVICE HOURS`; host tests cover
  the hourly rollover, the zero-skip averages, and the persisted-blob codec.
- The history chart now prints its fixed display ranges, labels the
  simulation history **SYNTHETIC**, shows `n/24 populated` in live mode,
  uses an "hourly average" tooltip, and offers **EXPORT CSV** for the 24 rows.
- No-finger live PPG is labelled **AMBIENT** with a grey trace and a
  **“NO CONTACT — ambient IR (not a pulse)”** overlay; it is not painted as a
  red alarm. A compact Ta/RH/PM2.5 strip in the header keeps those values
  visible when the full environmental cards are below the fold.
- Accessibility: contrast raised for notes/units/n-a tiles, gate is a labelled
  dialog with focus on the first action, header has a persistent `h1`, status is
  a polite live region, history chart supports arrow keys, buttons have explicit
  types, and reduced-motion is respected.
- Persistent prototype/calibration disclaimer added.
- **Applied 2026-10-19:** the six NEWS2 sub-score tiles are no longer
  reconstructed in live mode. `[TRIAGE]` now carries `HRPTS/SPO2PTS/RRPTS` from
  `clinical_vitals_engine.c`, and the dashboard uses those device points. It
  shows `--` if a build has not published them yet. Simulation still uses the
  local RCP tables because there is no device to ask. The total was already
  device-computed and remains so.

**Remaining (deliberately not changed):**

- Optional launcher shortcuts for the six profiles.
- The SoftAP page still has the 3 s web SOS cancel; the physical hold remains
  5 s.

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

### T4.4 — Re-fit before quoting any FPGA resource figure — **DONE (2026-10-19)**

The peak/4 crest refit completed on 2026-10-19. It is **smaller** than the
2026-10-18 burst fit:

- **515 / 1120 LUT5s (45.98%)** — 605 LUTs free
- **303 FFs** (298 CLB + 5 IOB)
- **105 / 140 CLBs (75.00%)** — **35 CLBs free**
- **0 BRAM, PLL 0/1**
- Bitstream SHA-256 `d9d58daa59eda5a4d99dd8fe44060f31d8f2e0cfb2d5a5d29a9998b2abd055d0`
- Firmware header regenerated with `convert_bitstream.py`; board flashed and booted
  (`configuration COMPLETE! (46408 bytes loaded)`).

The 2026-10-18 fit was 535 LUT5s / 303 FFs / 108 CLBs; its jump from 362 was the
32-bit IBI burst (`0xC3` + cycle count), not fitter noise. The older 443
(parallel link), 222, 342, 363 and 362 figures are also real measurements of
earlier netlists — see `hardware/shrikefi/synthesis_evidence/README.md` for the
history. **CLB occupancy (105/140) is still the constraint to watch.**

**One caveat carried forward:** the design declares no clock constraint, so the
fitter auto-constrains `clk` to 500 MHz. The 2026-10-19 report gives WNS
−9.357 ns, which is meaningless against that constraint. Achievable period is
11,356 ps (88.059 MHz); at 50 MHz the margin is +8.644 ns. Adding a real
constraint to the flow is still open work — until then the timing report cannot
be cited either way. The previous 81.846 MHz / +7.782 ns figures are historical.

---

### T4.5 — Give the FPGA flow a real clock constraint

Split out of T4.4 because the re-fit is done but this is not. `forgefpga_pins.pcf` declares only
`set_io` lines, so the fitter auto-constrains `clk` to 2000 ps (500 MHz) and every register in the
design "fails". The reported WNS of −9.357 ns is therefore unusable in either direction — it is
not evidence that timing fails, and not evidence that it closes.

The 2026-10-19 design's achievable period is 11,356 ps (88.059 MHz), so at the documented 50 MHz
the margin is +8.644 ns. That is a good sign, not a result. The previous fit's 81.846 MHz /
+7.782 ns is historical.

**Do:** find whether the Renesas flow accepts a frequency constraint (an SDC, or a `set_frequency`
in the PCF — the repo's reference docs under `docs/reference/shrike_board/` do not say), apply
50 MHz, and re-run. Until that exists, quote the achievable period and state that the constraint
is missing rather than quoting WNS.

Do **not** invent a constraint syntax and commit it without re-running the fitter — a wrong
constraint is worse than none, because it produces a confident number.

---

### T4.6 — Flash and verify HRV readiness / OLED state — **FLASHED; FIRST CAPTURE DONE, HRV NOT SETTLED**

**Status: flashed on COM4; 95 s and 302 s still-contact captures assessed; HRV
still not settled.** The current tree passes the host suite, the 34-test Zynq
suite and the ESP-IDF build; the HRV readiness change itself is `a2000ba`
(firmware build fix `666faea`), and the 2026-10-19 peak/4 bitstream is in the
firmware header. Boot is verified (`configuration COMPLETE! (46408 bytes
loaded)`). Both captures removed the old 888/645-style pair signature. The 302 s
capture (2026-10-19) held SQI ≥ 0.70 for 94 % of frames and the SQI gate withheld
HRV during a motion episode, but **RMSSD took ~3 minutes to settle**
(31→77 ms) and settled at ~76 ms — higher than the 95 s capture's 46 ms clean
tail. That instability means HRV is **not** validated; see
[`reports/HRV_STILL_CAPTURE_2026-10-19.md`](reports/HRV_STILL_CAPTURE_2026-10-19.md).

With the board on COM4, follow
[`docs/HARDWARE_DEBUG_HANDOVER.md`](HARDWARE_DEBUG_HANDOVER.md) §6. The short
version:

1. Flash and confirm FPGA configuration plus the I2C scan (OLED, IMU, MAX30102,
   BME280). Expect `CYCLONE=UNKNOWN` initially; flashing resets the 30-minute
   pressure window.
2. Take one steady 60–90 s finger contact and one deliberately noisy contact.
   Capture `[PPG]`, `IBI`, `[TELEMETRY]` and `[TRIAGE]`.
3. Verify the OLED progression: `Reading... (n/10)` → (`HRV: Low Signal` below
   SQI 0.70) → `HRV: Stabilizing` only with SQI >= 0.70 and fewer than nine
   contiguous pairs → `HRV RMSSD:xx.xms`.
4. Confirm `n` does not freeze, RMSSD is plausible (not the old ~190 ms
   artefact), and OLED RMSSD matches telemetry.
5. Record whether the 2.5 s FPGA fallback hands over repeatedly during steady
   contact. If it does, compare a 5 s window or a true sticky detector on a
   later build; do not change it without a captured handover count.

**Do not** lower the 0.70 SQI gate just to make the OLED publish. This run
verifies readiness and display logic only; T4.1 independent HRV validation is
still open.

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
| 1 | Continuous monitoring | HR, SpO₂, RR, SQI and IMU motion energy — **body temperature not measured**; activity classification and sleep staging are not implemented | **T2.1** (MAX30205/TMP117, ~₹200) closes body temperature; activity would be new work on the already-fitted IMU, sleep is scoped out |
| 2 | AI anomaly detection | **Strong** — real RR in NEWS2, SQI gating, INT8 TinyML head, 5-state fall FSM on the fitted MPU6050 | fall detection is implemented and unit-tested but not field-validated (multi-trial drops pending) |
| 3 | Disaster alerts | Heat + air quality good; **cyclone done via the pressure trend (T3.1)**; flood still not instrumented | a water sensor; the flood path still runs on an ambient cold-stress proxy |
| 4 | Environmental awareness | **Strong** | — |
| 5 | Privacy-preserving edge AI | **Satisfied** — cloud publishing off by default | — |
| 6 | Emergency assistance | **Partial — SOS latch + OLED emergency card + local SoftAP page + stored location + POST `/api/emergency/trigger`/`cancel` all shipped and exercised (T1.2-T1.4)** | only the **physical** GPIO5 SOS cancel button (T2.2) remains unbuilt |
| 7 | Wellness dashboard | **24-slot hourly history + chart, NVS-persisted (device-hours, no wall clock)** | — |
| 8 | Scalable deployment | Roadmap only | — |

---

## Suggested order

```
DONE: T1.1-T1.7 core software/status; T2.3 IMU fitted (physical fall test
      1/1 passed 2026-10-19; false positives not yet characterised);
      T3.1, T3.2, T3.3, T3.4, T3.5; T4.4 fit recorded (515 LUT5s / 303 FFs / 105 CLBs).

BOARD RECONNECT GATE — do this before the next measurement:
  The HRV readiness fix and the 2026-10-19 peak/4 bitstream are flashed on COM4.
  Run docs/HARDWARE_DEBUG_HANDOVER.md §6 and the 90 s finger capture; live HRV
  behaviour is not yet hardware-validated.

Physical gates / user actions, in this order:
  1. Print the MAX30102 reflective cradle; assemble; recalibrate SpO2 at the fixed
     contact (rest + breath-hold against the oximeter). Do not claim calibrated
     SpO2 before this.
  2. Solder the GPIO5 SOS cancel button (5 s hold firmware ready). The buzzer
     path and web SOS trigger/cancel are already verified (buzzer in the
     2026-10-19 fall trial; web 3 s hold).
  3. Fit TMP117/MAX30205 on the existing I2C bus, feed env.skin_temp_c, and
     re-enable the hypothermia path. Until then flood/hypothermia is ambient proxy only.
  4. Set the 2.4 GHz router password to the rotated credential and verify station
     mode/MQTT. SoftAP demo is unaffected; the ESP32-S3 has no 5 GHz radio.
  5. Run the remaining physical end-to-end pass: GNSS fix, PMS5003, BME280,
     MPU6050. The fall/SOS path has one passing physical trial (countdown +
     buzzer + cancel verified); still run multi-trial/orientation drops and
     false-positive characterisation, and capture the logs.

Validation / evidence:
  6. T4.1 independent HRV/IBI reference (AD8232 or borrowed chest strap), compared
     with Bland-Altman. Internal consistency is not ground-truth validation.
  7. The RTL crest threshold is now **peak/4 (25 %)**, up from peak/8, after live
     captures still showed 888/645, 781/641 and 666/797 ms split pairs. The
     2026-10-19 bitstream (`d9d58daa…`, 515 LUT5s / 105 CLBs) is flashed.
     **First 95 s capture:** the old split-pair signature is absent, but a
     2380 ms startup/contact gap and two pair-like startup events remain; clean
     tail RMSSD 46.2 ms, firmware 52.7 ms while the 300-beat ring holds startup.
     Do **not** claim HRV fixed. Next: true-peak timestamping and/or a
     contact-settling gate, then a 5-minute still capture.
  8. RR: 6-12 validated, 18-19 marginal (20.7 published), 27 unreachable on finger
     PPG. A dedicated respiration sensor is the only route; do not publish 27.

FPGA / claims:
  9. T4.5 real 50 MHz clock constraint. Until it exists, quote the achievable
     11,356 ps / 88.059 MHz, not timing closure, and never the auto-constrained
     WNS of −9.357 ns.
 10. The current crest refit uses 105/140 CLBs (35 free). Check/optimise any new
     RTL against CLBs first; LUT headroom (605 free) is misleading.
 11. T4.2 Indian normative HRV range and T4.3 India-specific thresholds, if claiming
     India-specific clinical validity.

Optional polish:
  - T3.6 is applied, and the device now sends the NEWS2 sub-scores so live
    tiles are no longer reconstructed locally.
  - Raise the web SOS cancel hold from 3 s to 5 s to match the physical button.
  - Reconcile the remaining subjective latency claims (0.44/0.38/18 us) and
    "integer-only" wording.
  - Standalone mNEWS2/environment card, if time permits.
  - H5 Zynq PS I2C slave address (only affects a real Zynq board).
```
