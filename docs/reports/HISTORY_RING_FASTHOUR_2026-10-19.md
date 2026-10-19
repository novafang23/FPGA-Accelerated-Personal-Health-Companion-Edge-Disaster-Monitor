# 24-hour history ring: wrap and eviction on hardware

**Date:** 2026-10-19 / 2026-10-20
**Board:** ESP32-S3 on COM4, ForgeFPGA bitstream 46,408 bytes loaded
**Firmware:** T3.2 NVS history (`f95f1ca` + `835c460`), built with
`VALOR_HISTORY_HOUR_SECONDS` set to 60 for the bench run only
**Purpose:** prove on hardware what the host test can only prove in simulation —
that the ring fills one slot per rollover and that the oldest hour is evicted
once all 24 slots are populated.

## Why the hour length was compressed

A real 24-hour window needs roughly 23-24 hours of continuous power, which is a
soak test, not a functional check. The ring logic is driven entirely by
`VALOR_HISTORY_HOUR_SECONDS`; nothing else depends on the value and the on-flash
blob does not store it. Building with 60 s makes one "hour" a minute, so 24
rollovers happen in about 24 minutes and the wrap plus eviction can be observed
directly.

The shipped firmware uses the default **3600 s**, restored before the final
flash. The compressed build was never intended as the delivered image.

## What was measured

Capture: 1560 s of serial output after a reset, finger on the sensor for the
first ~80 s so that early hours carry a heart rate (that value is the marker used
to watch a specific hour move through the ring).

| Observation | Result |
| --- | --- |
| NVS writes | 25 |
| Write spacing | min 58.5 s, max 60.8 s, mean 59.9 s |
| First / last write | t = 63.0 s / t = 1501.3 s |
| Ring growth | 3 populated slots at t = 63.0 s -> 24 at t = 1281.8 s |
| Ring after wrap | stayed at 24 slots for the rest of the run |
| Eviction | the prior session's 75.3 BPM hour sat in H23 at t = 1281.8 s and was gone by t = 1345.9 s, with the ring still at 24 slots |

The 25 writes at ~60 s spacing confirm one flash write per completed hour and no
extra writes in between. The ring reaching 24 and then staying at 24 while an
hour disappears is the eviction evidence: slots are shifted `23 <- 22 ... 1 <- 0`
and slot 23 is discarded.

Marker trajectory (hours that carry a heart rate) walked one slot older per
rollover, as expected from the shift:

```
t = 61.8 s   H0, H1     (the hour just completed)
t = 120.6 s  H1, H2
t = 182.1 s  H2, H3
...
t = 1281.8 s H19, H20, H21, H23   <- 75.3 BPM hour in H23
t = 1345.9 s H20, H21, H22        <- gone, ring still 24 slots
```

## Persistence of clinical values

Instrumentation decoded the encoded blob again immediately after writing it:

```
61.8 dbg roll: cnt=51 hr0=906 hr1=906 hr2=0
61.8 dbg blob: cn=24 hr_slots=2 sum_hr=1618
120.6 dbg blob: cn=24 hr_slots=3 sum_hr=2389
182.1 dbg blob: cn=24 hr_slots=4 sum_hr=3088
```

`hr_slots` and `sum_hr` grow as hours with contact are added, so the encoder
carries heart rate out of RAM into the 612-byte blob.

Two independent reboots restored the same five hours with the same values:

```
WEB_STATUS: Restored NVS history: 24 of 24 slots populated, 5 with a heart-rate average
[HISTORY] H1,HR=72.9,SPO2=93.6,RMSSD=75.7,NEWS2=1,PM25=41.7
[HISTORY] H2,HR=74.6,SPO2=96.7,RMSSD=76.3,NEWS2=1
[HISTORY] H3,HR=73.2,SPO2=97.7,RMSSD=77.2,NEWS2=1
[HISTORY] H4,HR=72.9,SPO2=97.0,RMSSD=104.2,NEWS2=4
[HISTORY] H5,HR=90.7,SPO2=93.3,RMSSD=98.1,NEWS2=2
```

The first reboot restored those five hours at H1-H5; the second restored the
same values two slots older (H3-H7) because the ring had rolled twice in between.
Encode, NVS write, and restore are therefore correct end-to-end, including heart
rate, SpO2, RMSSD and NEWS2.

## Open anomaly

One boot, immediately after the 26-minute bench run, reported:

```
Restored NVS history: 24 of 24 slots populated, 0 with a heart-rate average
```

even though the in-RAM ring at the end of that run contained an hour at H23 with
HR 73.9. Three later restores from the same firmware were correct, so this was
not reproduced and the cause is not known. It is recorded here rather than
explained away. It does not affect the ring/eviction evidence above and it did
not reproduce in the persistence checks, but a one-off silent drop of clinical
values across a reboot is the kind of thing that should be watched in a longer
soak test.

## Bench cleanup

The 60 s build left 24 fake "hours" in the device's NVS. They were erased and the
stored location was restored by writing a generated NVS image (24,576 bytes at
0x9000 containing only `valor/loc = KIET COLLEGE GHAZIABAD`), then flashing the
normal 3600 s build. The boot after that shows `No stored history; the chart
starts empty` and `[LOC] KIET COLLEGE GHAZIABAD`.

## What this does and does not show

Shows:

- one NVS write per completed hour, at the expected spacing;
- the ring fills to 24 slots and then wraps;
- the oldest hour is evicted when the 25th hour completes;
- heart rate, SpO2, RMSSD and NEWS2 survive encode, flash and reboot.

Does not show:

- a real 24-hour soak with a calendar time source (the device still measures
  hours of uptime, not wall-clock hours);
- clinical validity of the stored values — SpO2 remains prototype-calibrated and
  HRV remains unvalidated;
- that the open anomaly above cannot recur.
