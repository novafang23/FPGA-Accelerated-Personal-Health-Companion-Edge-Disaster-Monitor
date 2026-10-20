# PM2.5 calibration — the published correction replaces a private network

**Date:** 2026-10-20
**Scope:** the PMS5003 → published-PM2.5 path in the ShrikeFi/VALOR firmware
**Status:** implemented, unit-tested, host-tested; the on-device capture is
still open because the bench board was disconnected from USB during the final
verification (see *Hardware verification* below).

---

## 1. Why this changed

The firmware used to publish a PM2.5 estimate produced by a small INT8 network
that lived in `firmware/core/pm25_calibration_int8.c`. Its own header said:

> PROVENANCE: these weights are specific to this build. The training data and
> script are not part of this repository, so the model cannot be independently
> reproduced here.

That is a private, unreproducible model standing between the sensor and the
number a rescuer would act on. Its behaviour was measured on this machine
(`%TEMP%\pm25_audit\sweep.c`, linked against the real module) and it was not
acceptable for a hazard device:

| Raw PMS5003 | RH | Published by the old model | Ratio |
|---|---|---|---|
| 20 µg/m³ | 20 % | 13.9 | 0.70 |
| 60 µg/m³ | 30 % | 33.7 | 0.56 |
| 100 µg/m³ | 30 % | 51.6 | 0.52 |
| 500 µg/m³ | 25 % | 234.2 | 0.47 |
| 20 µg/m³ | 95 % | 0.0 | 0.00 |
| 55 µg/m³ | 90 % | 0.0 | 0.00 |
| 0 µg/m³ | any | 6.0 | — (inflates a clean reading) |
| 600 µg/m³ | any | 600.0 (pass-through) | 1.00 |

Findings:

1. **It cut the reading by 31–53 % in dry air.** There is no humidity
   justification for that at RH 20–30 %.
2. **It collapsed small readings to zero at RH ≥ 90 %** — a value of 20–55
   µg/m³ (hazardous for a rescuer in smoke) became "no particles". This is
   the dangerous direction: the device would report a clean atmosphere when
   the air was not clean.
3. **It inflated a zero reading to 6 µg/m³**, i.e. it put mass in the air that
   was never measured.
4. **It had a hard discontinuity** at raw > 508 (pass-through) while
   everything below it was scaled by ~0.5.
5. **Its humidity slope was ~15× steeper** than the published one
   (≈1.3 µg/m³ per %RH against 0.0862 µg/m³ per %RH), which is why small
   readings vanished in humid air.

The old module also could not be reproduced or defended: no training script, no
data, no citation for its weights.

**Decision:** delete the private model and publish a corrected estimate using
the algorithm that is published in the peer-reviewed literature and openly
documented for exactly this sensor channel.

## 2. The published correction that is now used

**Method and magnitude of the bias**
Barkjohn, Gantt & Clements (2021), *Development and application of a
United States-wide correction for PM2.5 data collected with the PurpleAir
sensor*, Atmos. Meas. Tech. **14**, 4617–4637, doi:10.5194/amt-14-4617-2021.
Their finding: raw PurpleAir data overestimate PM2.5 by about 40 % in most of
the US, and a linear regression **including a relative-humidity term**
reduces RMSE from 8 to 3 µg/m³ at a mean reference of 9 µg/m³.

**The PMS5003 form actually implemented**
AirGradient publishes the PMS5003 "atm" (environmental) channel form of that
same algorithm openly, at
<https://www.airgradient.com/documentation/correction-algorithms/>. Their
collocation study of the correction: 17 outdoor locations in 11 countries,
12,513 daily PM2.5 points against BAM/Grimm/Teledyne/TEOM references — raw
data over-read by about 50 % above 10–20 µg/m³, nRMSE ≈ 79 % → ≈ 38 %, MAE
7 → 3 µg/m³, improved at 14 of 17 sites and worse at 3 (Sydney: reference
noise at low concentration; Bogotá: the reference instrument itself over-reads
in chamber tests; Guatemala City: unverified reference).

The firmware implements those published equations verbatim
(`firmware/core/pm25_correction.c`, each band annotated with its source):

```
AGraw < 30          PM2.5 = 0.524*AGraw - 0.0862*RH + 5.75
30 <= AGraw < 50    PM2.5 = [0.786*(AGraw/20 - 3/2)
                           + 0.524*(1 - (AGraw/20 - 3/2))]*AGraw
                          - 0.0862*RH + 5.75
50 <= AGraw < 210   PM2.5 = 0.786*AGraw - 0.0862*RH + 5.75
210 <= AGraw < 260  PM2.5 = [0.69*(AGraw/50 - 21/5)
                           + 0.786*(1 - (AGraw/50 - 21/5))]*AGraw
                          - 0.0862*RH*(1 - (AGraw/50 - 21/5))
                          + 2.966*(AGraw/50 - 21/5)
                          + 5.75*(1 - (AGraw/50 - 21/5))
                          + 8.84e-4*AGraw^2*(AGraw/50 - 21/5)
260 <= AGraw        PM2.5 = 2.966 + 0.69*AGraw + 8.84e-4*AGraw^2
```

Negative results are clamped to 0, as the publisher advises.

**Why the coefficients are not the PurpleAir ones**
The PurpleAir uses a 532 nm laser; the PMS5003 uses a 680 nm laser with a 90°
scattering angle (Kaur, Mangin & Kelly 2026, AMT **19**, 1077). They are
different instruments, so the PMS5003-specific form is the one used. The
Barkjohn paper supplies the *method* and the *magnitude*, not the constants.

**Why the humidity effect is real**
Hua et al. (2021), Aerosol Air Quality Research (doi:10.1080/02786826.2021.
1873911): "PM2.5 was significantly positively correlated with relative
humidity due to particle hygroscopic growth". Tejada et al. (2022, Univ. of
Minnesota): "LCPMS sensors report larger particle counts when the RH is
increased and is regarded as an overestimation error … widely attributed to"
hygroscopic growth. AirGradient measured the effect on the same raw value
between 0 % and 100 % RH as "only a few micrograms" — the same order as the
published coefficient (0.0862 µg/m³ per %RH ⇒ 8.6 µg/m³ over the full
range).

## 3. What the firmware now does

```
PMS5003 frame ──► pm2_5_atm (raw)  ─┐
PMS5003 frame ──► pm10_atm (coarse) ─┼─► pm25_correction_push()
BME280        ──► humidity %RH    ─┘        │
                                               ├─ 60 s rolling window (64 samples)
                                               ├─ coarse fraction = (PM10-PM2.5)/PM2.5
                                               ├─ published banded equations
                                               └─ confidence code
                                                        │
        ┌───────────────────────────────────────────────┼────────────────┐
        ▼                        ▼                      ▼                ▼
  env.pm25 (published)   PMRAW / PMCONF on the    OLED page 3      dashboards
  (>=-1, -1 = no         wire + SoftAP JSON       (T / PM + flag)   (raw + band
  reading)                                                          + caveat)
```

Design rules the new path follows:

* **A missing reading is never 0.** `env.pm25` is negative when there is no
  reading; the pollution modality then reports `RISK_UNKNOWN` ("PM2.5 sensor
  unavailable - air quality not assessed"), the EPA AQI helper returns −1
  ("Not assessed"), and the AI model's pollution channel is refused. A
  0 µg/m³ reading from the sensor is kept as a real reading ("no particles
  detected"), because that is what it means.
* **The estimate is a rolling mean, not a 1 s sample.** The published fit was
  made on averaged data and the PMS5003 is noisy per sample.
* **Staleness withdraws the number.** If the sensor produces no fresh sample
  for 10 s, the window is dropped and the reading is withdrawn rather than
  keeping the last air quality on screen.
* **Dust suppresses the correction.** Under elevated dust loading the
  PMS5003 *under*-reads PM2.5 (Kaur, Mangin & Kelly 2026: mean bias error
  ≈ 50 %, dust identified by RH < 50 % and a high coarse-to-submicron mass
  ratio). A correction that only ever lowers the reading would make that case
  worse, so a coarse-dominated aerosol keeps the sensor's own number and is
  flagged `DUST`.
* **The confidence code travels with the number.** `OK`, `SETTLING`, `HUMID`
  (RH ≥ 80 %), `DUST`, `RANGE` (raw > 500 µg/m³), `NODATA`. The dashboards
  show it next to the value; they never show a corrected number without its
  caveat.

## 4. Measured behaviour of the new module

`firmware/core/test_pm25_correction.c` — **29/29 checks pass**:

| Case | Result |
|---|---|
| Band 1 (raw 20, RH 40) | 12.78 µg/m³ |
| Band 2 (raw 40) | 28.50 µg/m³ |
| Band 3 (raw 100, RH 50) | 80.04 µg/m³ |
| Band 4 (raw 230) | 192.58 µg/m³ |
| Band 5 (raw 300) | 289.53 µg/m³ |
| Continuity at the four band edges | jump ≤ 0.014 µg/m³ |
| Humidity slope | 0.0862 µg/m³ per %RH (published value) |
| Non-zero raw never published as 0 (sweep 5…600 at RH 95) | pass |
| A zero raw stays 0 | pass |
| Negative / NaN raw | `NODATA` |
| NaN RH | ignored |
| Estimate never negative | pass |
| Dust (coarse fraction 1.6) | keeps the raw value, flagged `DUST` |
| Fine aerosol (coarse fraction 0.25) | corrected normally |
| RH 85 % | flagged `HUMID` |
| Raw 600 | flagged `RANGE`, extrapolated (735.2) |
| 30 samples in the window | samples 30, raw mean 20.00, corrected 12.78, `OK` |
| 5 samples in the window | `SETTLING` |
| 90 s gap | the reading is withdrawn (`NODATA`) |
| One dropped read | the published value is kept |

Other suites, after the change:

| Suite | Result |
|---|---|
| `firmware/shrikefi/run_host_test.bat` (14 profiles) | pass, incl. `[PM Guard] No PM2.5 reading -> pollution UNKNOWN` |
| Zynq engine suite (`test_disaster_risk_engine.c`) | `ALL TESTS PASSED`, incl. the new `test_missing_pm25_is_not_clean_air` |
| `verify_dashboard.py` (headless Chrome DOM) | 14/14 cases, incl. `pm_published`, `pm_humid`, `pm_nodata` |
| `verify_softap_page.py` | 3/3 |
| ESP-IDF build | clean; binary 1,001,200 bytes |

## 5. Honest limits

* **No local validation.** We have no collocation reference instrument, so we
  cannot verify the correction here. What we can claim is that we now use the
  published algorithm for this exact channel, and that the published
  performance is nRMSE ≈ 38 % across 17 sites — not that our device is
  accurate to any particular figure in the field.
* **The correction was fitted on ambient aerosol in North America and
  Europe.** It improved accuracy at 14 of 17 AirGradient sites and made it
  worse at 3. A local dust or biomass-burning aerosol can fall outside the
  fit; the `DUST` flag is our guard against the known opposite bias, not a
  substitute for a reference measurement.
* **Our humidity comes from the BME280**, not the PMS5003's own hygrometer
  (which the published fit used, and which is itself biased low by 10–20 %,
  Mathieu-Campbell et al. 2024). The published coefficient is 0.0862 µg/m³
  per %RH, so a 20-point RH-source difference moves the estimate by under
  2 µg/m³ — an order of magnitude below the correction's own residual error.
* **The estimate is a 60 s mean**, so a sudden smoke event is visible within
  about a minute, not instantly.
* **This is an environmental estimate, not a clinical measurement.** It has
  never been validated against a reference instrument and must not be quoted
  as one.

## 6. Hardware verification - RECORDED 2026-10-20 (COM4)

Flashed image: 1,001,712 bytes (0xf48f0), esptool hash-verified. Boot log shows
`PMS5003 initialized OK` and the ForgeFPGA bitstream loaded (46,408 bytes).

At boot, before the first PMS5003 frame:

    PM25=-1.0, PMRAW=-1.0, PMCONF=NODATA

so the no-reading sentinel survives end to end and is not published as 0.

After the 20-sample window filled (ambient 27.2 C, 60.8 %RH):

    PMRAW=168.3  PM25=132.8  PMCONF=SETTLING   (OK after roughly 3 minutes)

Check: 0.786 x 168.3 - 0.0862 x 60.8 + 5.75 = 132.8, matching the published
value. Raw stayed at 165-168 ug/m3 in this room. No reference instrument was
available, so this proves the chain and the confidence transitions, not the
absolute accuracy. The dashboard raw-versus-published pair was not opened in
this capture and the OLED page-3 flag was not photographed.

**Policy issue resolved from the same capture (A1, 2026-10-20).** At
PM25=132.8 the rule engine printed `RPOLL=NORMAL` even though the AQI function
maps 132.8 to 208 (Very Unhealthy). The 75-150 band contributes 20 PRSI points
and the MODERATE cut is 30, so normal vitals could not reach it. The engine now
applies an EPA-AQI floor inside `assess_pollution_risk()`: AQI 151-200 raises
the pollution verdict to at least MODERATE, 201-300 to HIGH, 301+ to CRITICAL;
101-150 stays NORMAL with a sensitive-groups advisory. The floor only raises
the PRSI verdict and never replaces a higher one. With clean vitals, 132.8 now
reports `RPOLL=HIGH` (overall HIGH) and 400 reports CRITICAL. The PRSI
thresholds and the AI model are unchanged; see `test_pollution_aqi_floor()`.

## 7. Reproduction

```bash
# Unit tests for the correction itself (29 checks)
cd firmware/core
gcc -std=c99 -Wall -Wextra -Werror -pedantic -O2 test_pm25_correction.c pm25_correction.c -lm -o test_pm25 && ./test_pm25

# Device + engine suites
cd firmware/shrikefi && cmd /c run_host_test.bat --no-pause
cd firmware/zynq   && make test_engine && ./test_engine

# Dashboard DOM checks (headless Chrome, no device needed)
python firmware/shrikefi/dashboard/verify_dashboard.py
python firmware/shrikefi/dashboard/verify_softap_page.py
```

## 8. Evidence index

| Claim | Where |
|---|---|
| Published equations and their source | `firmware/core/pm25_correction.c` (header + `pm25_published()`) |
| Rationale and limits | `firmware/core/pm25_correction.h` |
| Unit tests (29) | `firmware/core/test_pm25_correction.c` |
| Missing reading → UNKNOWN (engine) | `firmware/core/disaster_risk_engine.c` (`assess_pollution_risk`, both NN entry points) |
| Missing reading → "Not assessed" AQI | `firmware/core/disaster_risk_engine.c` (`disaster_epa_pm25_aqi`, `disaster_epa_pm25_category`) |
| Device chain | `firmware/shrikefi/main_shrikefi.c` (PMS block, telemetry, status publish, OLED) |
| SoftAP page | `firmware/shrikefi/web_status.c` (JSON + page) |
| Win32 dashboard | `firmware/shrikefi/shrikefi_dashboard.c` (PM2.5 card) |
| Web dashboard | `firmware/shrikefi/dashboard/valor_dashboard.html` (PM card + history) |
| Removed module | `firmware/core/pm25_calibration_int8.c` (deleted; `git rm`) |
