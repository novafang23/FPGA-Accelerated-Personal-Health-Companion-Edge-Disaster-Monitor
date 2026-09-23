# VALOR web dashboard

A single self-contained HTML file. No build step, no install, no CDN.

```
valor_dashboard.html      the whole dashboard (one file)
```

## Using it

**Live, from the board** — plug the ESP32-S3 in over USB, open the file in
**Chrome or Edge**, click *Connect to device*, and pick the COM port. It opens
the port at 115200 and streams.

**With no hardware** — click *Run simulation*, or open it with `?sim=1`. Six
clinical profiles drive the whole dashboard, so a demo never depends on the
board being present or working.

**Unattended display** — `?sim=1&full=1` skips the gate and hides the profile
buttons. That is what `launch_web_dashboard.bat` uses.

Open a profile directly with `?sim=1&profile=Heat%20Wave`.

> **Firefox and Safari have no Web Serial API**, so live mode is Chrome/Edge
> only. Simulation works in every browser. The page detects this and says so
> rather than failing silently.

## Why this exists

The Win32 GDI dashboard (`shrikefi_dashboard.c`) still works and is still built —
this is additive, not a replacement. It exists because the Figma design is
web-native: glass cards (`backdrop-filter: blur`), neon glow via `box-shadow`,
gradient area fills, canvas animation. GDI has no equivalent for any of those,
so porting the design to it would have meant approximating away the parts that
make it look good.

Web Serial was the deciding factor: one HTML file can talk to the ESP32 over USB
with no server, no installer and no driver, which removes the usual reason to
write a native app.

## What was deliberately not carried over from the Figma mock-up

The mock-up was a visual design and contained several numbers the device cannot
produce. Showing them would have meant putting false clinical claims on screen,
so each was replaced. This table is the record of that.

| Mock-up showed | Problem | Replaced with |
|---|---|---|
| **Systolic est. 118 mmHg**, **Diastolic est. 76 mmHg** | No blood-pressure sensor and no BP algorithm anywhere in the project. Pure invention. | **Perfusion index** — actually computed (`ppg_sqi.c`) |
| **PTT 248 ms** | No pulse-transit-time measurement exists. | **RMSSD** and **signal quality** — both real |
| **AHA Autonomic …/10** | `disaster_calculate_aha_autonomic_strain()` returns **[0.0, 1.0]**. Showing it over 10 overstated it tenfold. | Same value on its real **/1.0** scale |
| NEWS2 tiles for **SBP** and **Consc** | The device measures neither, and the on-device score is HR + SpO₂ + RR only. The mock-up showed them as `+0`, which reads as "measured and normal". | Shown as **n/a — not instrumented**, greyed |
| **AUC 0.974** | Not measured anywhere in this project. | **Rule-vs-NN agreement 100%** (`accuracy_evaluator`) |
| **6 ms latency** | Never benchmarked. | Removed |
| **940 nm IR reflectance** | MAX30102/MAX30100 use 660 nm red and **880 nm** IR. | "AC-coupled reflectance" |
| **BME688 · SHT40** | Neither part is on this board. | **BME280** (and labelled *ambient, not skin*) |
| **SDS011 · LSTM calibration** | Wrong sensor and wrong algorithm. | **PMS5003 · INT8 humidity calibration** |
| **Morlet CWT** | Not the method used. | **RSA modulation** (what `ppg_respiratory_rate.c` does) |
| NEWS2 point tables | Approximate, and wrong at several boundaries (e.g. HR 91–110 scored 0 instead of 1). | Transcribed from `news2_hr_score` / `news2_spo2_score` / `news2_rr_score` so the screen cannot disagree with the device |

Keep this list if the design is ever regenerated — it is easy to reintroduce a
plausible-looking number that nothing measures.

## The architecture change this forced

The page is a **pure view**. It computes no clinical logic.

That matters because of what the firmware used to send. `[TELEMETRY]` carried
raw vitals only, so the old Win32 dashboard had to re-derive NEWS2, the three
hazard risks and the TinyML scores on the PC — a second implementation of the
clinical logic, off-device, in a project whose entire claim is edge-first. Two
implementations of the same thing, with nothing keeping them in step.

The firmware now also prints:

```
[TRIAGE] NEWS2=6,LEVEL=2,FLAGS=0x03,RISK=CRITICAL,NNHEAT=0.815,NNPOLL=0.034,
         NNFLOOD=0.241,RHEAT=CRITICAL,RPOLL=NORMAL,RFLOOD=UNKNOWN,PSI=6.70,AHA=0.42
```

So the ESP32 stays the single source of truth and any display is just a
renderer. As a side effect this also fixed two cited indices that were only ever
computed in the host test harness and never on the live path: **Moran PSI**
(Moran 1998) and the **AHA autonomic strain** (Brook et al., *Circulation* 2010)
are now calculated for real on-device.

`LEVEL` is the `clinical_risk_level_t` ordinal and `FLAGS` is the
`clinical_alert_flags_t` bitmask — enum mirrors, not derived logic.

## Serial contract

```
[TELEMETRY] HR=..,SPO2=..,RMSSD=..,TEMP=..,HUM=..,PM25=..,RR=..,SQI=..
[TELEMETRY] NO_FINGER,TEMP=..,HUM=..,PM25=..
[TELEMETRY] ACQUIRING,HR=..,SPO2=..,RMSSD=..,TEMP=..,HUM=..,PM25=..
[TRIAGE]    NEWS2=..,LEVEL=..,FLAGS=0x..,RISK=..,NNHEAT=..,NNPOLL=..,NNFLOOD=..
            RHEAT=..,RPOLL=..,RFLOOD=..,PSI=..,AHA=..
[PPG]       <raw IR counts, ~100 Hz>
```

`SQI` arrives as 0–1 and is shown as a percentage. `NN*` arrive as 0–1 and are
shown as percentages. Everything else is already in display units.

If no telemetry arrives for 3 seconds while connected, the header says so rather
than freezing on stale numbers.

## Next step (not done)

Serve this same file from the ESP32 over SoftAP, so a rescuer's phone can open it
with no router and no cable. That is task **T1.3** in `docs/REMAINING_WORK.md`,
and it closes requirement 6 — it needs an HTTP server and a SoftAP in the
firmware, no change to this file.
