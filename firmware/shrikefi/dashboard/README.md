# VALOR web dashboard

A single self-contained HTML file. No build step, no install, no CDN.

```
valor_dashboard.html      the whole dashboard (one file)
```

## Using it

The page has **three explicit states**, and it shows nothing until you pick one:

| State | How to get there | What is on screen |
|---|---|---|
| **idle** *(default)* | just open the file | every readout is `--`, scope shows *NO SIGNAL*. No invented data. |
| **live** | *Connect to device* → pick the COM port | real values from the board |
| **sim** | *Simulation*, then pick a profile | the six clinical profiles, labelled as a simulation |

**Live, from the board** — plug the ESP32-S3 in over USB, open the file in
**Chrome or Edge**, press *Connect to device*, and pick the COM port. It opens at
115200 and streams.

**With no hardware** — press *Simulation*. Six profiles drive the whole
dashboard, so a demo never depends on the board working.

**Nothing until you choose** — the idle state exists on purpose. An unconnected
dashboard showing a plausible heart rate is worse than one showing nothing, so
values stay blank until a device is connected or a simulation is explicitly
started.

### Command line / automated

```
launch_web_dashboard.bat              open idle; you choose Connect or Simulation
launch_web_dashboard.bat sim          start straight in the simulation
launch_web_dashboard.bat full         simulation, profile buttons hidden (kiosk)

?sim=1                                skip the gate, run the simulation
?sim=1&full=1                         ...and hide the six profile buttons
?sim=1&profile=Heat%20Wave%20%26%20Dehydration
```

> `?full=1` hides only the six profile buttons. It used to hide the whole
> controls container, which also hid **Connect** — so launching the kiosk preset
> left no way to attach the board. Connect now lives outside that container.

### Connecting: the thing that will trip you up first

**Only one program can hold a COM port.** Close `idf.py monitor`, PuTTY, the
Win32 dashboard, and the Arduino IDE before pressing Connect. If the port is
busy, `requestPort()` fails or the page sits at *NO DATA*.

Also: **put a finger on the sensor.** Until contact the device sends `NO_FINGER`
or `ACQUIRING`, and the dashboard correctly shows `--` rather than a stale value.

If nothing appears, check in this order: firmware flashed → port free → right
port → finger on the sensor.

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
| **Systolic est. 118 mmHg**, **Diastolic est. 76 mmHg** | No blood-pressure sensor and no BP algorithm anywhere in the project. Pure invention. | **Perfusion index** — computed by `ppg_sqi.c` and transmitted as `PI=` |
| **PTT 248 ms** | No pulse-transit-time measurement exists. | **RMSSD** and **signal quality** — both real |
| **AHA Autonomic …/10** | `disaster_calculate_aha_autonomic_strain()` returns **[0.0, 1.0]**. Showing it over 10 overstated it tenfold. | Same value on its real **/1.0** scale |
| NEWS2 tiles for **SBP** and **Consc** | The device measures neither, and the on-device score is HR + SpO₂ + RR only. The mock-up showed them as `+0`, which reads as "measured and normal". | Shown as **n/a — not instrumented**, greyed |
| **AUC 0.974** | Not measured anywhere in this project. | **Rule-vs-NN agreement 99.95%** (`accuracy_evaluator`); FP32↔INT8 tier agreement 97.32% |
| **6 ms latency** | Never benchmarked. | Removed |
| **940 nm IR reflectance** | MAX30102/MAX30100 use 660 nm red and **880 nm** IR. | "AC-coupled reflectance" |
| **BME688 · SHT40** | Neither part is on this board. | **BME280** (and labelled *ambient, not skin*) |
| **SDS011 · LSTM calibration** | Wrong sensor and wrong algorithm. | **PMS5003 · INT8 humidity calibration** |
| **Morlet CWT** | Not the method used. | **RSA modulation** (what `ppg_respiratory_rate.c` does) |
| NEWS2 point tables | Approximate, and wrong at several boundaries (e.g. HR 91–110 scored 0 instead of 1). | Transcribed from `news2_hr_score` / `news2_spo2_score` / `news2_rr_score` so the screen cannot disagree with the device |

Keep this list if the design is ever regenerated — it is easy to reintroduce a
plausible-looking number that nothing measures.

## The architecture change this forced

The page is a **pure view of the final triage**. The total NEWS2, `RISK`, the hazard
risks and the TinyML scores arrive from the board. The six NEWS2 sub-score tiles are
reconstructed locally from the displayed vitals using the same RCP tables so the
breakdown is visible; they cannot change the device's total score.

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
[TELEMETRY] HR=..,SPO2=..,RMSSD=..,TEMP=..,HUM=..,PM25=..,RR=..,SQI=..,
            RRDEPTH=..,RRCONF=..,PI=..
[TELEMETRY] NO_FINGER,TEMP=..,HUM=..,PM25=..
[TELEMETRY] ACQUIRING,HR=..,SPO2=..,RMSSD=..,TEMP=..,HUM=..,PM25=..
[TRIAGE]    NEWS2=..,LEVEL=..,FLAGS=0x..,RISK=..,NNHEAT=..,NNPOLL=..,NNFLOOD=..
            RHEAT=..,RPOLL=..,RFLOOD=..,PSI=..,AHA=..
[PPG]       <raw IR counts, 25 Hz decimated from the 100 Hz FIFO>
```

`SQI` arrives as 0–1 and is shown as a percentage. `NN*` arrive as 0–1 and are
shown as percentages. `PI` is the AC/DC perfusion index in percent and is now
shown live. Everything else is already in display units.

If no telemetry arrives for 3 seconds while connected, the header says so rather
than freezing on stale numbers.

The **PPG scope is real data in live mode**: it plots the `[PPG]` raw-IR stream
(the firmware decimates the 100 Hz FIFO 1:4). The ECG sparkline and RR envelope
are schematic visualisations, and both carry a visible `SCHEMATIC` tag so they
cannot be mistaken for measured ECG or respiration waveforms. The HR number
itself comes from the device's accepted IBI pipeline, not from the browser
waveform.

The **24-hour trajectory is a 24-slot RAM buffer since boot**, not a calendar
history persisted in NVS. It fills only as uptime accumulates, and slot 0 is the
device's current-hour average (not an instantaneous reading). The page now takes
that point from `[HISTORY]` rather than overwriting it with live vitals, and the
summary averages only hours that contain valid clinical values.

In simulation the chart is explicitly labelled **SYNTHETIC** and says it is not
device data; in live mode it shows `SINCE BOOT` and `n/24 hours populated`. The
chart's fixed display ranges are printed below it, and **EXPORT CSV** downloads
the 24 rows with `source, hour_offset, hr_bpm, spo2_pct, rmssd_ms, pm25_ugm3,
news2, valid` so a demo capture can be analysed off-device.

## ShrikeFi SoftAP dashboard

The ESP32-S3 serves a compact phone dashboard at `http://192.168.4.1` after you
join its `VALOR-xxxx` WiFi access point. This is a separate page embedded in
`firmware/shrikefi/web_status.c`; it is not the standalone HTML file above.
The standalone file is the richer USB Web Serial dashboard (or an explicitly
selected simulation). Both are self-contained and work without internet access.

The SoftAP page polls the device's status once per second. It shows ambient
temperature from the BME280 separately from the MAX30102 finger-temperature
estimate, which is an optical proxy and not a clinical thermometer. It also
shows current vitals, signal quality, triage, location, storm status, and the
manual SOS trigger/cancel controls. When the HTTP status request fails, current
readings are cleared instead of being left on screen as if they were live.

The SoftAP is open for emergency access. It does not provide authentication;
any client that joins can read the live status and use its controls. The status
and history APIs no longer grant cross-origin browser reads. Same-origin checks
also reject host-prefix lookalikes, but do not authenticate an AP client.

**Verification:** run `python firmware/shrikefi/dashboard/verify_dashboard.py`
for the standalone page and `python firmware/shrikefi/dashboard/verify_softap_page.py`
for the firmware-embedded page. The SoftAP page is compiled into the ESP-IDF
firmware; its JSON contract and origin check also have host regressions. These
software checks run without the board, but opening the actual SoftAP requires
the ESP32-S3 to be powered and connected.
