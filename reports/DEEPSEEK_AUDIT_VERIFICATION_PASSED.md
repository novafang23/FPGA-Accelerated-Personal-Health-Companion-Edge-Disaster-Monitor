# Project SIH26181: DeepSeek Audit Verification & Sign-Off

# Final Verification Review — SIH26181 EdgeGuard

**Reviewer:** Senior Systems Auditor & Biomedical AI Principal
**Scope:** Post-remediation re-audit of CB-2, CB-4/F-101, CB-1/F-301, F-401/F-403, F-503
**Verdict target:** SIH 2026 + Qualcomm Hardware Challenge demo readiness

---

## 1. CB-2 — SQI Crisis Suppression Override

**Status: ✅ RESOLVED — with one caveat to lock before demo.**

The fix is anatomically correct. The critical flaw in the prior revision was that motion-artifact gating (`SQI < 0.70 → hold triage`) could mask a genuine physiologic collapse. The new control flow now:

1. Canonicalizes `hr_i`, `spo2_i`, `rr_i`, `rmssd_x10` **first**
2. Evaluates `is_absolute_crisis` against *those same integers*
3. Only then applies the SQI hold — and only when `!is_absolute_crisis`

This eliminates the failure mode where a rounding-pathway divergence (e.g. `spo2 = 85.5` stored as float but compared as `<= 85`) would let a false negative slip past the crisis gate. The integer canonicalization is the right primitive — it guarantees the crisis predicate and the display/advisory path agree bit-for-bit.

**Crisis triggers evaluated (all correct against NEWS2 / Resus Council):**
- `spo2_i ≤ 85` → severe hypoxemia (matches Scale 1 low band)
- `hr_i ≥ 150` → symptomatic tachyarrhythmia threshold
- `hr_i ≤ 39` → profound bradycardia
- `rr_i ≥ 30` or `rr_i ≤ 6` → apnea-imminent borders

**AUTONOMIC_SHOCK gate** (`rmssd_x10 < 80` AND any of HR≥100 / SpO2≤94 / RR≥24) is defensible — it requires corroborating physiology, so a lone low RMSSD from a noisy window won't fire.

**Caveats to close before demo (non-blocking, but I want them logged):**

1. **Low-SQI + absolute crisis path**: you correctly bypass the hold, but the output does **not** set `ALERT_SIGNAL_NOISE` when SQI is low in the crisis branch. Add it so the UI can render *"EMERGENCY — signal degraded, verify manually"* rather than a clean alarm that looks trustworthy. A paramedic must know the number is suspect.
2. **`rmssd_x10` sentinel `-1`**: the shock gate is `rmssd_x10 >= 0 && rmssd_x10 < 80`. That is correct, but ensure the caller never passes a *legitimate* RMSSD of 0.0 (would produce `rmssd_x10 = 0` → false shock). Your guard `rmssd > 0.0f` already excludes it — good.
3. **Truncated source**: the snippet you posted cuts off mid-expression at `bool critical_vital = ...`. I cannot verify the remainder of `clinical_vitals_assess_full`. The logic *shown* is correct; the *unseen* branch (level scoring + advisory composition) must be confirmed to consume `is_absolute_crisis` in a way that forces `CLINICAL_CRITICAL` regardless of NEWS2 sum. **Ship me the tail.**

**Result: CB-2 is functionally resolved. Recommend: add `ALERT_SIGNAL_NOISE` to crisis branch; confirm the level assignment tail.**

---

## 2. CB-4 / F-101 — Pinmap Boot Strapping + CB-1 / F-301 — MAX30100 Register Clarity

**Status: ✅ RESOLVED on register map; ✅ RESOLVED on strapping; ⚠️ one hardware caveat.**

### 2a. MAX30100 register separation (CB-1 / F-301)

The old driver conflated MAX30100 and MAX30102 register addresses — a classic silicon-killer in the MAX30xxx family because `0x09` is `MODE_CONFIG` on the 30100 but `MODE_CONFIG` on the 30102 is `0x09` too, yet `0x0A` is `SPO2_CONFIG` on the 30102 while on the 30100 it's reserved. The new header now disambiguates:

| Register | MAX30102 | MAX30100 | Verdict |
|---|---|---|---|
| MODE_CONFIG | 0x09 | 0x06 | ✅ distinct macros |
| SPO2_CONFIG | 0x0A | 0x07 | ✅ distinct |
| LED_PA / LED_CONFIG | 0x0C / 0x0D | 0x09 | ✅ distinct |
| PART_ID expected | 0x15 | 0x11 | ✅ correct per datasheets |

The runtime branch `dev->is_max30100` set from PART_ID read is the correct detection strategy — **never trust the PCB silkscreen**. Register writes now route through `mode_reg` chosen by `is_max30100`, so a shipped MAX30100 in a MAX30102 footprint won't corrupt its MODE_CONFIG.

**Sub-caveat:** MAX30100 SpO2_CONFIG `0x47` decodes as Hi-Res (`0x40`) + 100 Hz sample (`0x04`) + 1600 µs pulse width (`0x03`). This is correct per Maxim Table 4. However, 1600 µs @ 100 Hz **saturates** on a strong-perfusion subject; given you're using `LED_CONFIG 0x88 = 27.1 mA` per LED, that is on the high side. For a wrist/hand demo, drop to `0x11` per LED (14.2 mA) which gives headroom for facial flush. Not a correctness bug — a calibration risk.

### 2b. Pinmap boot strapping (CB-4 / F-101)

ESP32-S3 strapping pins are **GPIO 0, 3, 45, 46** (plus the ROM USB pins 19/20 matter for DFU). Your new pinmap:

- I2C on GPIO 1 / 2 → non-strapping ✅
- UART1 to PMSA003 on 14 / 18 → non-strapping ✅
- FPGA parallel bus on 4, 5, 6, 7, 8, 9 → non-strapping ✅
- FPGA reset on 11 → explicitly flagged "non-strapping pin" ✅
- `PIN_MAX30102_INT = -1` with justification ✅

**No GPIO 0/3/45/46 is assigned to any peripheral.** Boot will not depend on the state of a sensor connector. This is the correct fix — the previous assignment had a strapping pin controlling a peripheral level, meaning a disconnected sensor could put the chip into download mode.

**One caveat to log:** GPIO 19/20 (USB D+/D−) are **not** claimed in the pinmap. That's correct — the ShrikeFi uses those for the native USB-CDC. But if anyone on the team wires PMSA003 or FPGA strapping to a breakout that overlaps 19/20, USB will drop on enumeration. Add a comment to the pinmap naming 19/20 as reserved. One-line fix.

**Result: both resolved. Recommend: reduce MAX30100 LED current on demo unit; document GPIO 19/20 as reserved.**

---

## 3. F-401 / F-403 — INT8 Zero-Point Offsets and Clamp Guards

**Status: ⚠️ MATHEMATICALLY SOUND FOR SYMMETRIC WEIGHTS; ACTIVATION PATH NEEDS VERIFICATION.**

### 3a. Weight dequantization

```
sum = b1_scale * (b1[i] - b1_zp)
      + Σ W1_scale * (W1[i][j] - W1_zp) * input[j]
```

For **per-tensor symmetric quantization** the zero-point should be **0**, and subtracting it is a no-op — correct but redundant. If you ever fall back to asymmetric per-tensor weights (which the header comment says you don't), the formula generalizes correctly *only if* you also apply input zero-point subtraction:

```
real = W_scale * (W_q - W_zp) * (input_q - input_zp)
```

Since you are computing in float with pre-normalized input ∈ [0,1] (input_zp implicitly 0.0), the code is consistent. **The math is right, but the correctness is predicated on the training script having used symmetric weight quantization. If `train_nn_risk_model.py` emits asymmetric weights for W1/W2, this is silently wrong.** Verify in the `.inc` source that `W1_zp == 0`.

### 3b. `clamp_uint8` guard

```c
if (!isfinite(x) || x <= 0.0f) return 0;
if (x >= 255.0f) return 255;
return (uint8_t)roundf(x);
```

**Correct.** Handles:
- NaN/Inf → 0 (safe escape hatch) ✅
- Negative → 0 (ReLU semantics) ✅
- Overflow → 255 saturate ✅
- Normal → roundf before cast (avoids truncation bias) ✅

This is the right shape. Two notes:

1. The header says "per-tensor symmetric quantization for weights, asymmetric for activations". The asymmetric activation path uses uint8 codes, so `clamp_uint8` is the correct range. **But `h1_q` is being quantized using `qparams->act1_scale` and `act1_zp` — I do not see those fields applied in the snippet you posted.** The hidden-layer quantize step is truncated. Verify it reads `clamp_uint8(h1[i] / qparams->act1_scale + qparams->act1_zp)` or equivalent. **Currently unverifiable.**

2. `isfinite` inside a hot loop: on Xtensa LX7 without an FPU-accelerated `isfinite`, this is a handful of cycles per activation. At 24+24+1 activations @ 50 Hz, negligible. Fine.

**Result: weight path mathematically sound. Activation zero-point application unverified (snippet truncated). Require full `.c` file and a diff against the training script's quantization code.**

---

## 4. F-503 — NOAA / NWS Steadman Heat Index

**Status: ✅ RESOLVED — correctly implemented.**

The Rothfusz regression coefficients are exactly the NWS published set:

```
-42.379 + 2.04901523·T + 10.14333127·RH
- 0.22475541·T·RH - 0.00683783·T²
- 0.05481717·RH² + 0.00122874·T²·RH
+ 0.00085282·T·RH² - 0.00000199·T²·RH²
```

**Every coefficient matches NWS Technical Attachment SR 90-23.** The `< 27 °C → return T` short-circuit is the correct domain guard (Rothfusz only valid ≥ 80 °F). The `hi_c > temp_c` final check is also correct — Rothfusz can dip below ambient in dry air, which is non-physical for a heat index.

**Downstream CTSI integration is sound:** the `ambient_temp_c < HEAT_TEMP_BASE_C → normal` guard is documented and prevents the hypothermia-RMSSD false-positive you flagged. Good defensive engineering.

**One minor addition:** NWS publishes **two correction terms** for extreme regimes (low humidity RH < 13%, high humidity RH > 85% with T between 80–87 °F). Neither is in your code. For SIH demo latitudes (typically 15–30 °C ambient, 40–90% RH), the base Rothfusz is accurate to ±1 °C. **Acceptable for demo; add a README note that corrections are omitted by design and under which regimes.**

**Result: F-503 fully resolved.**

---

## 4. Final Verdict — Demo Readiness

**Overall: 🟢 GREEN TO DEMO — with three pre-flight items that must be closed tonight.**

| ID | Item | Severity | Blocking? |
|---|---|---|---|
| R-1 | Post full `clinical_vitals_assess_full` tail and confirm `is_absolute_crisis` forces `CLINICAL_CRITICAL` | Medium | Yes for safety claims |
| R-2 | Add `ALERT_SIGNAL_NOISE` in crisis branch (low SQI + absolute crisis) | Low | No (UX) |
| R-3 | Confirm `W1_zp == 0` and `W2_zp == 0` in `.inc` (symmetric weights) | High | Yes for accuracy claims |
| R-4 | Post hidden-layer activation quantize lines (act1_scale/zp application) | High | Yes for accuracy claims |
| R-5 | Lower MAX30100 LED_CONFIG from `0x88` → `0x11` for demo perfusion headroom | Low | No (calibration) |
| R-6 | Add GPIO 19/20 reserved comment to pinmap | Trivial | No |

**None of R-1 through R-4 implies the fixes are wrong** — they imply the snippets provided are truncated at exactly the points a reviewer cannot infer. **I cannot sign off on a "resolved" claim for code I cannot see.**

### What has genuinely improved

- **CB-2** is a real architectural fix, not a patch. Integer canonicalization before predicate evaluation is the pattern I would mandate across the whole stack.
- **Pinmap** now respects ESP32-S3 strapping topology. A disconnected HX711-class sensor can no longer brick boot.
- **MAX30100/30102** are now first-class distinct devices with PART_ID-driven runtime branch.
- **Heat index** is now a documented, verified NWS equation rather than a hand-tuned approximation.

### What I want the team to walk into the demo hall knowing

The three things a judge from Qualcomm or a clinical advisor will probe:

1. *"What happens when your sensor's wrong?"* → You now have a defensible answer: SQI gating holds triage, absolute crisis overrides the hold, and (pending R-1) the level is forced critical.
2. *"Can a bad boot sequence brick your device?"* → No. Strapping pins are free.
3. *"Is your ML real INT8 or float-in-disguise?"* → You dequantize per-tensor symmetric weights and re-quantize activations with an INT8 code range. **Be ready to show the training script matches.** That is the question R-3/R-4 protect against.

**Recommendation: resolve R-1 and R-3/R-4 by 21:00 today, run a full end-to-end bench with a hand-hypoxia simulation (breath-hold + rapid movement) to prove the CB-2 override fires under realistic SQI collapse, then freeze.**

You are demo-ready **conditional** on those four artifacts landing. Send the tails and I will close the loop.

— *Auditor sign-off pending artifact delivery.*