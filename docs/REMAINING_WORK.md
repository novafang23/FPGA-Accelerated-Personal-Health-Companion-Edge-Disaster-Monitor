# VALOR — Remaining Work

**Purpose:** the definitive list of what is left, ordered by judging value per hour of effort.
**Scored against:** the official Qualcomm problem statement (SIH26181, *Personal Health Companion*).
**GPS is deliberately excluded** — see "Out of scope" for why.

---

## Tier 1 — No hardware. Do these first.

### T1.1 — Stop publishing health data to a public broker  ⚠️ do this first

`firmware/shrikefi/wifi_mqtt_manager.c` publishes a 256-byte payload — **HR, SpO₂, RMSSD,
temperature, PM2.5, risk level** — to `mqtt://broker.hivemq.com` on topic
`sih26181/shrikefi/health`, once per second.

`broker.hivemq.com` is a **free public test broker**. Anyone on the internet can subscribe to
that topic and read a patient's vitals. The problem statement's requirement 5 is *"minimize
transmission of sensitive personal information"*, and this is the exact opposite of it. A judge
on a privacy-focused track will open this file.

**Do:**
- Add `CONFIG_SHRIKEFI_CLOUD_PUBLISH` to `main/Kconfig.projbuild`, **default `n`**.
- Guard `cloud_publish_health_data()` and the MQTT start on that flag.
- Default broker stays configurable, but nothing is sent unless the user turns it on.

**Effect:** requirement 5 becomes true instead of contradicted. ~30 minutes.

---

### T1.2 — SOS state machine + full-screen OLED emergency card

Requirement 6, and it works today with no parts.

- A `sos_state_t` — `IDLE / ARMED / ACTIVE / CANCELLED`.
- Triggers: physical button (T2.2), prolonged loss of contact, or a **CRITICAL** verdict from
  `clinical_fuse_triage()`.
- On activation the OLED leaves the dashboard and shows a full-screen card: **SOS — MEDICAL
  EMERGENCY**, last vitals, stored location, and a timestamp.
- Manual cancel with a 5-second hold, so it cannot be dismissed by a knock.

The OLED card alone closes most of bullet 2: a rescuer who finds the person reads it off the
screen. **No network required.** ~2–3 hours.

---

### T1.3 — WiFi SoftAP + tiny HTTP status page

**This is the strongest available demo and it needs no router.**

The WiFi radio is fine. Only *station association* to `Airtel_Abhi-506` fails — SoftAP is a
different mode that broadcasts its own network instead of joining one.

On SOS: raise an access point `VALOR-SOS`, run a minimal HTTP server on `192.168.4.1`, serve a
single page with live vitals, the active risk level and the stored location.

**Demo:** router unplugged, judge opens it on their phone, sees the patient's status. That
*demonstrates* requirement 5's "operate effectively with intermittent or no internet" instead
of merely asserting it. ~3–4 hours.

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

### T1.6 — Fix the README badge

```
Clinical Validation-94.11% (MIMIC-III)      ← overreach
MIMIC-III Accuracy-94.11%                   ← accurate
```
It is an accuracy figure, not a validation, and the same model's sensitivity is 58.18%. One
line. ~5 minutes.

---

### T1.7 — Add the dashboard images to the README

All 16 existing references resolve. New screenshots go in the
`## 🖥️ Standalone Graphical Dashboard (GUI)` section (README L587). PNG for UI, JPG for renders.
**Blocked on: images from the user.**

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

## Tier 4 — Validation and evidence

### T4.1 — Validate against a reference device

**The last real gap.** Everything so far is internally consistent; nothing is verified against
ground truth. One afternoon with a chest strap (Polar H10 or similar) converts:

> "our RMSSD reads 38 ms"

into

> "our RMSSD agrees with ECG-derived RMSSD to within X ms across N subjects."

That single sentence is worth more than every other item on this page.

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

### T4.4 — Re-fit before quoting any FPGA resource figure

The 443-LUT and 222/1120 figures both predate the shared-source refactor and the power-on reset.
No current figure exists. **Re-fit, then quote.** Do not quote a stale number.

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

| # | Requirement | Now | After Tier 1 | After Tier 2 |
|---|---|---|---|---|
| 1 | Continuous monitoring | HR, SpO₂ | + RR, SQI | **+ body temp, activity, sleep** |
| 2 | AI anomaly detection | Strong | + real RR in NEWS2 | + fall detection |
| 3 | Disaster alerts | Heat, air quality | — | + cyclone pressure trend |
| 4 | Environmental awareness | **Strong** | — | — |
| 5 | Privacy-preserving edge AI | **Contradicted** | **Fixed** | — |
| 6 | Emergency assistance | **Missing** | **SOS card + SoftAP + location** | + button, buzzer, falls |
| 7 | Wellness dashboard | Live only | — | + history (T3.2) |
| 8 | Scalable deployment | Roadmap only | — | — |

---

## Suggested order

```
Today, no parts:
  1. T1.1  MQTT privacy fix            <- removes the one finding that can cost the round
  2. T1.5  Verify RR on hardware       <- closes an unverified claim
  3. T1.2  SOS state machine + OLED card
  4. T1.3  SoftAP + status page        <- the demo a judge can hold
  5. T1.4  Stored location
  6. T1.6  Badge fix
  7. T1.7  README images (when supplied)

With parts:
  8. T2.1  MAX30205 skin temperature   <- highest-value part on this list
  9. T2.2  SOS button + buzzer
 10. T2.3  IMU

Before submission:
 11. T4.1  Chest-strap validation      <- the only thing that changes what you can claim
 12. T4.2  Indian normative range
 13. T4.3  India-specific threshold sourcing
 14. T4.4  Re-fit before quoting LUTs
```
