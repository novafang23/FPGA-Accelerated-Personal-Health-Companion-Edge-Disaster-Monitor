# Skin-to-Core Validation Protocol — VALOR (oral reference)

**Status: DRAFT — nothing in this protocol has been run yet.**

> **Why this document was rewritten.** The first version fit a *global*
> skin→core regression on one subject. That is the wrong experiment: a
> fingertip sits 3–7 °C below core and the gap is set by vasoconstriction and
> room temperature, so no global coefficient generalises and the fit would
> fail for reasons that teach you nothing. What follows is the published
> method for continuous monitoring — an **individualised correction factor**:
> one reference reading per session, then track the offset
> ([J Appl Physiol 2010](https://pubmed.ncbi.nlm.nih.gov/20504906/)).

---

## 1. What this protocol establishes

**A per-session core offset** `offset = T_oral − T_skin`, with a measured
distribution of that offset across sessions.

That's it. It answers three concrete questions:

1. What is my skin-to-core offset today? (expect `≈ 4–5 °C`)
2. Is that offset **stable** across sessions, or does it move?
3. If it moves, by how much — i.e. how often must it be redone?

If the offset is stable (SD < ~0.5 °C), you have a genuine personal
calibration and can state it with a number. If it is not stable, that is also
a real finding: it means the offset must be taken per session, and the honest
claim is "trend only".

**What it cannot establish:** a general skin→core function that works on other
people. One subject, one site, one room.

---

## 2. Equipment

| Item | Why |
|---|---|
| Meditech DT-112 (0.1 °C digital, rigid tip) | Oral reference. Confirm on the box it is rated for oral use. |
| VALOR board, TMP117 + BME280 fitted | Skin + ambient |
| Phone on `VALOR-0081` | To call `/cal` |
| Stopwatch | Timing discipline is most of the measurement quality |
| A chair, no sun, no draught | Sessions in a room that is NOT changing temperature |

**Do not start if:** caffeine, a meal, or exercise in the last 30 min; an
AC/fan cycling; or your hands already feel cold.

---

## 3. Procedure (one session ≈ 8 min)

### Phase A — settle (3 min)
1. Board powered, **no finger** on the sensor.
2. Sit still, hands open on your knees — not clasped, not in pockets.

### Phase B — establish contact (1 min)
3. Place the **index finger** of one hand on the sensor. Same finger, same
   pressure, same position, every session.
4. Wait until the OLED stops saying `ACQUIRING` (≥12 s) and the page shows a
   settled skin value.

### Phase C — the paired calibration (1 min) ← the important part
5. **Keep the finger on the sensor.** Put the thermometer under the tongue —
   same sublingual pocket, left or right, every time.
6. Lips closed, breathe through the nose, **do not talk**.
7. When it beeps, note the value to 0.1 °C.
8. Still holding the finger, open `http://192.168.4.1/cal?ref=36.6` on the phone
   (substitute your reading).
9. The response tells you the offset it computed. Record it.

> **The one rule that matters:** the finger must be on the sensor *while* the
> thermometer is in your mouth. Taking the oral reading first and then placing
> the finger measures two different moments, and the offset is then meaningless.
> This is the most common way this kind of dataset is ruined.

### Phase D — repeat across sessions
10. Do this on **different days, at different times, in different rooms**.
11. One row per session in `tools/temp_validation_log.csv`.

---

## 4. The log file

One row per **session** (not per second):

```csv
session_id,timestamp_s,condition,ref_method,t_ref_c,t_skin_c,t_amb_c,hum_pct,hr_bpm,pi_pct,spo2_pct,contact_ok,notes
S01,300,baseline,oral,36.6,31.4,24.5,52,72,2.1,98,1,left sublingual; cold hands
S02,300,baseline,oral,36.4,33.1,27.0,49,68,1.8,98,1,left sublingual; warm room
```

Because the fitter reads `offset = t_ref_c − t_skin_c` per row, one row per
session is exactly right — this is a *distribution of offsets*, not a
regression dataset. Ten sessions gives you ten offsets and their SD.

**Minimum:** 8 sessions. Below that the fitter refuses.

---

## 5. Honesty requirements

Always state:

> *"Within-subject, n=1, oral reference. The skin-to-core offset is measured
> per session and is not claimed to transfer to other people. NEWS2's
> temperature row remains uninstrumented because a peripheral site is not a
> core-equivalent site."*

Do not say: "calibrated body temperature", "validated", "clinically accurate".

---

## 6. Expected outcome

| Outcome | Meaning | What to claim |
|---|---|---|
| Offset SD < 0.5 °C | Stable personal offset | "Personal offset X ± SD °C, re-checked per session" |
| Offset SD 0.5–1.5 °C | Moves with conditions | "Offset must be taken per session; trend-valid" |
| Offset SD > 1.5 °C | Site too variable | "Trend only — absolute core not claimable" |

All three are honest results. The third is the most likely if you test in
genuinely different rooms, and reporting it is stronger than pretending
otherwise.

**Irreducible limit:** a single fingertip probe cannot give absolute core
temperature without a reference reading. No amount of data fixes the anatomy —
only changing the site (axillary) or adding a second probe would, and both are
ruled out on this build.
