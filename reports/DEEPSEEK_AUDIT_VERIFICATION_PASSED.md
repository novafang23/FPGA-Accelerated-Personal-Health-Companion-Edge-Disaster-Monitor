> [!WARNING]
> **PARTIALLY SUPERSEDED — this is a pre-fix review snapshot.**
>
> Its headline blocker (CB-2, the low-SQI crisis-suppression ordering) has since
> been resolved. `clinical_vitals_engine.c` now (1) validates inputs and rejects
> NaN/out-of-range, (2) derives canonical integer vitals, (3) evaluates
> `is_absolute_crisis` from those same integers, and *only then* (4) applies the
> low-SQI suppression branch — which is precisely the fix this report asks for.
> The "not fully closed" verdict describes the code at review time.
>
> Its pin-map and INT8 observations remain accurate. Kept for audit-trail purposes.

---
# Project SIH26181: DeepSeek Audit Verification & Sign-Off

## Audit Review — SIH26181 EdgeGuard Post-Fix

### 1. CB-2 SQI Crisis Suppression — **Not completely resolved**

The crisis override is directionally correct: it attempts to detect an absolute crisis *before* the low-SQI suppression branch.

However, there is a **clinically significant rounding mismatch**:

- The `is_absolute_crisis` check uses **raw floating-point values** before integer conversion.
- The later `critical_vital` / NEWS2 path uses the **rounded or truncated integer vitals**.

Examples where a critical reading can still be suppressed:

```text
SpO2 = 85.4%  => raw condition: spO2 <= 85.0 is false
                 later: spo2_i = (int)85.4 = 85 => critical hypoxia
                 => critical value suppressed under low SQI
```

```text
HR = 149.6 bpm => raw condition: hr >= 150.0 is false
                 later: hr_i = round(149.6) = 150 => critical tachycardia
                 => critical value suppressed under low SQI
```

```text
RR = 31.6 bpm => raw condition: rr >= 32.0 is false
                later: rr_i = round(31.6) = 32 => critical tachypnea
                => critical value suppressed under low SQI
```

Also, the low-SQI branch executes **before input validation**. If SQI is low and HR/SpO₂/RR are invalid or NaN, the code can copy non-physiological values into the output instead of returning the “calibrating/invalid” advisory.

**Recommended fix:**

- Compute and validate the integer vitals first:
  - `hr_i = (int)(hr + 0.5f)`
  - `spo2_i = (int)spo2` or use `lroundf()`
  - `rr_i = (rr > 0.0f) ? (int)(rr + 0.5f) : 14`
- Run basic physiologic range validation.
- Build `is_absolute_crisis` from those **same integer values**.
- Then run the low-SQI suppression branch.

Until that is done, the CB-2 blocker is **not fully closed**.

---

### 2. MAX30100 Register Macros & Strapping Pin Changes — **Solid in the reviewed excerpt**

This is the strongest part of the fix.

**Pinmap:**

- The ESP32-S3 strapping pins are avoided: GPIO0, GPIO3, GPIO45, GPIO46 are not used.
- GPIO11 as FPGA reset is non-strapping and acceptable.
- I²C on GPIO1/GPIO2 is fine.
- MAX30102 INT is not connected (`-1`), so no strapping conflict.
- The 5 V / 3.3 V rule is clearly documented.

No strapping or voltage-rail issues found in the shown pinmap.

**MAX30100 registers:**

- The register map for the MAX30100 is internally consistent:
  - `MODE_CONFIG` = 0x06
  - `SPO2_CONFIG` = 0x07
  - `LED_CONFIG` = 0x09
- The driver correctly detects `Part ID 0x11` vs `0x15`.
- Separate MAX30100/MAX30102 configuration paths are present.
- Readback of the MAX30100 configuration is implemented.

Remaining bring-up caveat:

- Confirm that `MAX30102_MODE_RESET` maps to the appropriate reset bit for both parts.
- Verify the stated LED current math for the MAX30100 `0x88` value against the actual datasheet formula for the board’s LED configuration.

For hackathon hardware bring-up, this is acceptable.

---

### 3. INT8 Zero-Point Offset Math & Clamp Guards — **Mathematically sound with one condition**

The shown layer 1 dequantization is correct:

```c
sum = bias_scale * (bias_q - bias_zp)
      + Σ W_scale * (W_q - W_zp) * input
```

This is the correct general asymmetric dequantization form.

The clamp guard:

```c
static uint8_t clamp_uint8(float x) {
    if (!isfinite(x) || x <= 0.0f) return 0;
    if (x >= 255.0f) return 255;
    return (uint8_t)roundf(x);
}
```

is also correct for asymmetric uint8 activations. It handles:

- NaN / Inf
- negative values
- values above 255
- rounding to nearest integer

**Condition:**

This is sound **only if** the serialized model’s `W_zp` and `bias_zp` actually follow the same convention.  
For standard TensorFlow Lite-style INT8 models:

- weights may be symmetric, so `W_zp == 0`
- biases are usually int32 with zero-point `0`

If your generated model uses nonzero bias zero-points, then the code is correct for that model.  
If `bias_zp` is mistakenly nonzero while the model was exported with zero-centered biases, the dequantized bias will be wrong.

The partial file does not show the remainder of the MCU inference path, so I cannot certify layers 2/output without seeing the full function.  
The shown fragment, however, is mathematically sound.

---

### 4. Final Verdict — **Conditionally ready for SIH 2026 / Qualcomm Hardware Challenge**

**Not fully ready as-is.**  
**Ready for demonstration after one targeted patch.**

The fixes are broadly in the right direction:

- Pinmap / strapping: ✅ solid
- MAX30100 register clarity: ✅ solid
- INT8 zero-point/clamp: ✅ sound, assuming model export consistency
- NOAA heat index equation: ✅ standard Rothfusz equation used
- SQI crisis override: ⚠️ still has a rounding/validation edge case

**Required before live demo:**

1. Move the low-SQI branch after input validation and integer conversion.
2. Build `is_absolute_crisis` from the same rounded integer vitals used later for triage.
3. Reject or sanitize invalid/NaN vitals **before** the low-SQI branch.

After that, the demo firmware is reasonable for a hackathon/prototype setting.

**Still not production or medical-device ready** — that would require a full clinical risk assessment, SDLC documentation, and hardware-in-the-loop testing under controlled conditions.  
For SIH + Qualcomm Hardware Challenge, it can be made ready with the above patch.