# HRV still-capture evidence — 2026-10-19 (COM4)

Raw serial log: 302 s, 12,382 timestamped lines (kept off-repo; ~650 KB of
per-sample `[PPG]` rows). Analysis script reproduces every number below from the
log. This file records the measured numbers and what they do and do not show.

## Setup

- Board: ESP32-S3 + ForgeFPGA SLG47910 on **COM4**, 115200 baud.
- Firmware: tree at commit `f95f1ca` plus the T3.2 NVS-history work; peak/4 crest
  bitstream 46,408 bytes.
- Boot verified before capture: `configuration COMPLETE! (46408 bytes loaded)`;
  I²C `0x3C` OLED, `0x57` MAX30102, `0x68` MPU6050, `0x76` BME280; OLED,
  BME280, PMS5003, GNSS, MPU6050, SOS button all initialized.
- Contact: one fingertip, still, held for the full 302 s. The board was reset at
  capture start, so the run includes the cold-start transient.

## Measured results

| Quantity | Result |
|---|---|
| Capture length | 302 s |
| `[TELEMETRY]` frames | 280 (279 with SQI > 0) |
| First HR frame | t = 13.7 s |
| HR | 70.3 – 84.1 BPM (median 76.3, mean 76.5) |
| SpO₂ | 94.0 – 99.0 % (median 96.6, mean 96.6) |
| SQI | 0.42 – 0.92 (median 0.87); **263/279 frames (94 %) ≥ 0.70** |
| PI | 0.46 – 5.49 % (median 1.19) |
| RMSSD published | 255 frames, 31.3 – 77.5 ms (median 68.9) |
| IBI rows | 361 total: **318 ok**, 8 prime, 17 gap, 11 high, 7 low (all source 2 / FPGA) |
| ok IBI | min 635, median 786, max 948 ms (mean 788) |
| low IBI | 585 – 661 ms (median 615) |
| high IBI | 968 – 1461 ms (median 1362) |
| gap IBI | 1502 – 2420 ms (median 1556) |
| PPG status | LOCKED 515, ACQUIRING 49 optical frames |
| SpO₂ label | VALID 542, `CALC/--` 22 optical frames |
| Split-pair candidates | **0** (consecutive ok IBIs, ratio 1.6 – 2.5) |
| `[HISTORY] H0` prints | 9 |

## RMSSD settling (published values over time)

```
t=  20.7  RMSSD=31.3      t= 177.6  RMSSD=75.3
t=  42.8  RMSSD=50.8      t= 202.1  RMSSD=77.5
t=  65.1  RMSSD=55.9      t= 224.2  RMSSD=76.1
t=  87.3  RMSSD=57.7      t= 246.2  RMSSD=76.4
t= 109.5  RMSSD=65.5      t= 268.2  RMSSD=75.7
t= 133.7  RMSSD=68.4      t= 292.1  RMSSD=76.5
t= 155.7  RMSSD=68.8      t= 293.8  RMSSD=76.5  (last published)
```

The value climbs as the 300-interval window fills; it is only broadly stable
after ~3 minutes. **The first 1–2 minutes of an RMSSD reading must be discarded.**

## End-of-capture motion episode (gate behaviour)

From t ≈ 294 s the contact degraded:

- PI spiked to 5 – 8 %; `[SPO2DIAG]` `spread`/`raw` rose to 5 – 7;
- SpO₂ went `CALC/--` and `[TELEMETRY] SPO2=0.0` for a stretch;
- SQI fell 0.70 → 0.54; `FLAGS=0x40` appeared in `[TRIAGE]`;
- **RMSSD was withheld to 0.0 ms** rather than publishing a bad value.

The 0.70 SQI gate therefore did its job: HRV disappeared while the signal was
poor instead of reporting a motion-contaminated RMSSD.

## On-device history feed

`[HISTORY] H0` published nine coherent running averages during the run, e.g.
`H0,HR=76.8,SPO2=98.1,RMSSD=35.9,NEWS2=0,PM25=63.5` at t=34 s rising to
`H0,HR=76.5,SPO2=96.5,RMSSD=67.0,NEWS2=1,PM25=63.5` at t=292 s. This is the
first on-device confirmation that the hourly-history feed accumulates and
publishes. The **NVS write/restore was verified separately** later the same day:
after over an hour of uptime a rollover wrote the blob, and the next boot logged
`Restored NVS history: 2 of 24 slots populated, 1 with a heart-rate average`
(see `docs/REMAINING_WORK.md` T3.2). No hour elapsed during *this* capture, so what
this report covers is the RAM feed only.

## What this does and does not show

**Shows:** steady-contact HR/SpO₂/SQI are internally consistent; the old
split-peak (888/645-style) IBI signature did not recur in 318 accepted intervals;
the SQI gate withholds HRV during motion; the on-device history feed works. It
also provides the settling curve the 2026-10-20 contact-settling gate is
calibrated against (240 of 300 beats and a 120 s span before RMSSD is published).

**Does not show:** that RMSSD is accurate. 76 ms is higher than the 46 ms clean
tail of capture 3 and could still contain PPG timing jitter rather than true RR
variation. SpO₂ has only a rough two-point calibration. Independent ECG /
chest-strap comparison (T4.1) is still required before any RMSSD or SpO₂ accuracy
claim, and the design's HRV numbers must not be presented as clinical.
