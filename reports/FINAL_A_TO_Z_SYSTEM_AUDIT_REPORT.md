> [!WARNING]
> **SUPERSEDED — this report predates the fixes it audits (dated 09-MAY-2026).**
>
> Several findings below have since been resolved in the code. Notably it flags a
> **GPIO3 strapping conflict on the FPGA reset line**; the current pin map uses
> **GPIO11** (`PIN_FPGA_RST_N` in `firmware/shrikefi/shrikefi_pinmap.h`), which is
> not an ESP32-S3 strapping pin.
>
> Its "not jury-ready" and "critical hardware/software mismatch" verdicts describe
> the tree as of that date, not the current one. Kept for audit-trail purposes.
> For current, verified status see `reports/CLINICAL_AI_ACCURACY_REPORT.md` and the
> README's "Reproducibility & Evidence Provenance" section.

---
# Project SIH26181: Final A-to-Z DeepSeek System Audit & Certification

# EdgeGuard / ShrikeFi — Independent System Architecture Audit Report

**Project:** SIH26181 EdgeGuard / ShrikeFi  
**Audit scope:** Supplied source slices: pin map, FPGA Verilog / C link driver, MAX30100/MAX30102 driver, INT8 neural network, clinical/disaster engines  
**Auditor role:** Chief Hardware Architect / Embedded Systems Auditor / Biomedical AI Principal  
**Date:** 09-MAY-2026  

---

# 0. Executive Summary

The architecture is well-structured and the high-level split between ESP32-S3, ForgeFPGA, and clinical peripherals is reasonable. However, the supplied code is **not zero-flaw ready**. There are **critical hardware/software mismatches**, especially in MAX30100 support and clinical alarm gating, plus multiple assumptions that must be validated before any formal “zero-flaw” or jury-ready certification can be issued.

### Final Certification

| Certification | Status |
|---|---|
| Approved as zero-flaw | **No** |
| Approved for hardware prototyping | **Conditional** |
| Approved for clinical demonstration | **Not until critical fixes are made** |
| Jury readiness | **Not yet — corrective action required** |

---

# 1. Electrical & Interconnect Integrity

## 1.1 Pin Map Summary

| Signal | ESP32-S3 GPIO | Voltage domain | Evaluation |
|---|---|---|---|
| I2C SDA | GPIO1 | 3.3 V | Acceptable |
| I2C SCL | GPIO2 | 3.3 V | Acceptable |
| PMSA003 UART1 RX | GPIO14 | 3.3 V logic | Acceptable |
| PMSA003 UART1 TX | GPIO18 | 3.3 V logic | Acceptable |
| FPGA link strobe | GPIO4 | 3.3 V | Acceptable |
| FPGA link direction | GPIO5 | 3.3 V | Acceptable |
| FPGA data D0-D3 | GPIO6-9 | 3.3 V | Acceptable |
| FPGA beat IRQ | GPIO10 | 3.3 V | Acceptable |
| FPGA reset | GPIO3 | 3.3 V | **Potential strapping/boot conflict** |

## 1.2 Findings: Electrical

### F-101 — GPIO3 is a strapping pin on ESP32-S3  
GPIO3 on ESP32-S3 is a boot strapping pin. Using it for `PIN_FPGA_RST_N` means the FPGA reset line can affect, or be affected by, the ESP32-S3 boot state. This can cause boot-mode instability, unintended reset behavior, or FPGA reset glitches during power-up.

**Severity:** Major  
**Recommendation:** Move `PIN_FPGA_RST_N` to a non-strapping GPIO, e.g. GPIO11/GPIO12/GPIO13, and ensure the FPGA reset is driven by a normal GPIO with proper default state.

### F-102 — I2C bus pull-ups need real measurement  
The design correctly calls for 2.2 kΩ pull-ups on SDA/SCL to 3.3 V. However, many SSD1306, BME280, and MAX30100/MAX30102 modules already include onboard pull-ups. If all devices are assembled on a custom carrier, the parallel pull-up resistance may drop too low or too high.

**Severity:** Moderate  
**Recommendation:** Measure actual bus pull-up resistance and total bus capacitance at 400 kHz Fast-Mode. Verify rise time < 120 ns.

### F-103 — 5 V isolation strategy is correct but needs power decoupling  
The PMSA003 VCC is correctly isolated from GPIO logic by using only USB-C VBUS. However, PMSA003 uses a laser and fan motor, causing current spikes.

**Severity:** Moderate  
**Recommendation:** Place bulk capacitance near the PMSA003 connector, use an EMI filter or ferrite bead on the 5 V rail if needed, and ensure no 5 V node is adjacent to any GPIO.

### F-104 — Bidirectional FPGA data bus contention risk  
The 4-bit FPGA data bus is bidirectional. The C driver changes ESP32 GPIO direction with a 1 µs settling delay. The Verilog side must use `link_dout_oe` correctly and release the bus before the ESP32 can drive it.

**Severity:** Moderate  
**Recommendation:** Verify that `link_dout_oe` is deasserted before `link_dir` changes from read to write. If the FPGA output-enable is only updated on a clock edge, add a small direction-turnaround delay before the ESP32 drives the bus.

### F-105 — PMSA003 passive-mode comment is inaccurate  
The comment states:

> “The sensor outputs frames continuously (~1 Hz) in passive mode.”

That is not how the PMSA003 passive mode works. In passive mode, the sensor only responds to commands. It does not output continuously. Continuous output is active mode.

**Severity:** Low / documentation  
**Recommendation:** Correct the comment and ensure firmware sends passive-mode command frames if passive mode is intended.

---

# 2. Verilog ↔ C Protocol Coherence

## 2.1 Command Nibbles

The C header and Verilog localparams match exactly.

| Command | Verilog | C header | Value |
|---|---|---|---|
| NOP | `4'h0` | `0x0` | 0x0 |
| WRITE_RED | `4'h1` | `0x1` | 0x1 |
| WRITE_IR | `4'h2` | `0x2` | 0x2 |
| WRITE_THRESH | `4'h3` | `0x3` | 0x3 |
| READ_RED | `4'h4` | `0x4` | 0x4 |
| READ_IR | `4'h5` | `0x5` | 0x5 |
| READ_IBI | `4'h6` | `0x6` | 0x6 |
| CLEAR_IRQ | `4'h7` | `0x7` | 0x7 |
| READ_STATUS | `4'h8` | `0x8` | 0x8 |

**Verdict:** Command byte coherence is good.

## 2.2 FSM State Coherence

The Verilog state definitions cover:

- WRITE_RED high/low
- WRITE_IR high/low
- WRITE_THRESH high/low
- READ_RED high/low
- READ_IR high/low
- READ_IBI nibbles 0–7
- READ_STATUS

This is structurally coherent with the expected nibble counts.

## 2.3 Findings: Link Driver

### F-201 — `DELAY_NS()` is actually 1 microsecond  
In `shrikefi_link_driver.c`:

```c
#define DELAY_NS() esp_rom_delay_us(1)
```

The name says nanoseconds but the implementation delays **1 µs**, i.e. 1000 ns.

**Severity:** Moderate  
**Impact:** The link may still work, but it is not a “cycle-accurate high-speed” link. Throughput is limited by 1 µs per strobe edge.

**Recommendation:** Either rename to `DELAY_US()` and document the protocol timing, or replace with a true nanosecond delay if the FPGA protocol requires higher speed.

### F-202 — Read sampling before strobe must be proven  
`read_nibble()` reads the GPIO levels **before** pulsing the strobe:

```c
static uint8_t read_nibble(void) {
    uint8_t val = 0;
    for (...) {
        val |= (gpio_get_level(...) & 1) << i;
    }
    pulse_strobe();
    return val;
}
```

This is correct only if the FPGA presents valid data after entering the read state and advances state on the strobe edge. The supplied Verilog snippet does not show the actual FSM transition logic.

**Severity:** Moderate  
**Recommendation:** Provide the complete FSM case logic and confirm that FPGA outputs are valid before the ESP32 reads.

### F-203 — Need explicit FPGA output-enable truth table  
The Verilog has `link_dout_oe`, but the supplied snippet does not show how it is generated. If `link_dout_oe` is not gated by both `link_dir` and current FSM state, GPIO contention can occur.

**Severity:** High  
**Recommendation:** Add a truth table:

| Mode | `link_dir` | `link_dout_oe` | ESP32 data pins |
|---|---|---|---|
| ESP32 write | 0 | 0 | Output |
| FPGA read | 1 | 1 | Input |

---

# 3. Sensor & Driver Compatibility

## 3.1 MAX30102 Detection

The driver reads the part ID and attempts to distinguish:

- MAX30102: expected `0x15`
- MAX30100: expected `0x11`

This is conceptually correct.

## 3.2 Critical Finding — MAX30100 Register Map Is Wrong

The supplied MAX30100 path uses incorrect register addresses.

### Actual MAX30100 register map

| Register | Address |
|---|---|
| INTR_ENABLE_1 | 0x02 |
| INTR_ENABLE_2 | 0x03 |
| FIFO_WR_PTR | 0x04 |
| OVF_COUNTER | 0x05 |
| FIFO_RD_PTR | 0x06 |
| FIFO_DATA | 0x07 |
| MODE_CONFIG | 0x08 |
| SPO2_CONFIG | 0x09 |
| LED_CONFIG | 0x0A |

### Driver-supplied MAX30100 code

```c
max30102_i2c_write_reg(dev, 0x02, 0x00); // FIFO WR PTR  -> wrong
max30102_i2c_write_reg(dev, 0x03, 0x00); // OVF COUNTER  -> wrong
max30102_i2c_write_reg(dev, 0x04, 0x00); // FIFO RD PTR  -> wrong
max30102_i2c_write_reg(dev, 0x06, 0x03); // Mode         -> wrong
max30102_i2c_write_reg(dev, 0x07, 0x47); // SpO2         -> wrong
max30102_i2c_write_reg(dev, 0x09, 0x88); // LED          -> wrong
```

This writes:

- FIFO write pointer to `INTR_ENABLE_1`
- OVF counter to `INTR_ENABLE_2`
- FIFO read pointer to `FIFO_WR_PTR`
- Mode config to `FIFO_RD_PTR`
- SpO2 config to `FIFO_DATA`
- LED config to `SPO2_CONFIG`

**This means the MAX30100 path will not configure, reset, or read the sensor correctly.** In fact, because the reset function also uses `0x06` as the MAX30100 mode register, the reset timeout will likely never clear because the reset bit is not in the mode register. The driver may hang for the full timeout or fail.

**Severity:** **Critical**

### F-301 — MAX30100 reset register is wrong  
`max30102_reset()` selects:

```c
uint8_t mode_reg = dev->is_max30100 ? 0x06 : MAX30102_REG_MODE_CONFIG;
```

For MAX30100, the correct mode configuration register is `0x08`. Using `0x06` targets `FIFO_RD_PTR`.

**Recommendation:** Correct the MAX30100 register map immediately. Minimum required:

```c
#define MAX30100_REG_FIFO_WR_PTR  0x04
#define MAX30100_REG_OVF_COUNTER  0x05
#define MAX30100_REG_FIFO_RD_PTR  0x06
#define MAX30100_REG_MODE_CONFIG  0x08
#define MAX30100_REG_SPO2_CONFIG  0x09
#define MAX30100_REG_LED_CONFIG   0x0A
```

## 3.3 FIFO Byte Ordering and 16-to-18-bit Normalization

The supplied driver slice **does not include FIFO read logic** for either sensor. This is critical.

| Sensor | FIFO bytes per sample | Sample bit depth |
|---|---|---|
| MAX30100 | 4 bytes per sample: IR high, IR low, Red high, Red low | 16-bit |
| MAX30102 | 6 bytes per sample: 3 bytes Red, 3 bytes IR, 18-bit packed | 18-bit |

The driver must:

- Provide separate MAX30100 FIFO read routine.
- Provide separate MAX30102 FIFO read routine.
- Normalize MAX30100 16-bit samples to the same scale as MAX30102 18-bit samples.

**Severity:** Critical  
**Recommendation:** Implement and test explicit functions:

```c
int max30100_doc_read_fifo(uint16_t *red, uint16_t *ir);
int max30102_doc_read_fifo(uint32_t *red, uint32_t *ir);
```

Then normalize all samples to a common internal range, e.g. 18-bit or 32-bit.

---

# 4. TinyML INT8 Model Audit

## 4.1 Tensor Size and Memory

The network is:

```
Input:     6
Hidden 1: 24
Hidden 2: 16
Output:    3
```

### ROM estimate

| Tensor | Shape | Elements | Bytes |
|---|---|---|---|
| W1 | 24×6 | 144 | 144 |
| b1 | 24 | 24 | 24 |
| W2 | 16×24 | 384 | 384 |
| b2 | 16 | 16 | 16 |
| W3 | 3×16 | 48 | 48 |
| b3 | 3 | 3 | 3 |
| **Total weights** | — | **619** | **619 bytes** |

Including quantization params:

- 9 float values × 4 bytes = 36 bytes
- 6 int8 zps + 3 uint8 zps = 9 bytes

### RAM estimate

Internal stack usage is small:

```c
float input[6];
float h1[24];
float h2[16];
float out_logit[3];
uint8_t h1_q[24];
```

Roughly **< 300 bytes stack**, which is acceptable for ESP32-S3.

**Verdict:** Memory footprint is good.

## 4.2 Quantization Mathematics

The forward pass uses **dequantized float weights**:

```c
sum += qparams->W1_scale * (float)model->W1[i][j] * input[j];
```

This is acceptable for the ESP32-S3 FPU, but it is **not integer-only inference**.

### F-401 — Weight and bias zero points are not applied  
The code declares:

```c
int8_t W1_zp, W2_zp, W3_zp;
int8_t b1_zp, b2_zp, b3_zp;
```

But the supplied inference code only uses scales:

```c
qparams->b1_scale * (float)model->b1[i]
qparams->W1_scale * (float)model->W1[i][j]
```

It does **not** subtract `W1_zp` or `b1_zp`.

If the generated quantization uses nonzero zero points for weights or biases, the inferred output will be mathematically wrong.

**Severity:** High  
**Recommendation:** Either:

1. Guarantee all weight/bias zero points are zero in the generated `nn_risk_model_int8.c.inc`, or
2. Change the dequantization formula to:

```c
scale * ((float)q_val - zp)
```

### F-402 — Input quantization is absent  
The network normalizes inputs to `[0,1]` and directly multiplies by dequantized weights. If the original fake-quant training pipeline also quantized the input activations, this driver is not mathematically identical to the exported model.

**Severity:** Moderate  
**Recommendation:** Confirm the Python export script’s fake-quant path. If the input is quantized to `uint8` with a scale, replicate that quantization in C or explicitly re-export the model without input quantization.

### F-403 — `clamp_uint8()` can overflow  
```c
int32_t xi = (int32_t)roundf(x);
```

If `x` is NaN, infinite, or outside the valid `int32_t` range, the conversion is undefined/unspecified.

**Severity:** Moderate  
**Recommendation:** Clamp `x` to `[0,255]` before casting:

```c
if (!isfinite(x)) return 0;
if (x <= 0.0f) return 0;
if (x >= 255.0f) return 255;
return (uint8_t)roundf(x);
```

### F-404 — No NULL pointer validation  
`nn_predict_int8()` dereferences `model`, `qparams`, and `out` without checking for NULL.

**Severity:** Low  
**Recommendation:** Add NULL checks and return an error code.

---

# 5. Clinical Validity & Edge-Case Risk

## 5.1 NEWS2 Scoring

The supplied heart-rate and SpO2 boundaries match standard NEWS2 Scale 1 for most ranges.

| Parameter | Range | Score |
|---|---|---|
| HR ≤ 40 | ≤ 40 | 3 |
| HR 41–50 | 41–50 | 1 |
| HR 51–90 | 51–90 | 0 |
| HR 91–110 | 91–110 | 1 |
| HR 111–130 | 111–130 | 2 |
| HR ≥ 131 | ≥ 131 | 3 |
| SpO2 ≥ 96 | ≥ 96 | 0 |
| SpO2 94–95 | 94–95 | 1 |
| SpO2 92–93 | 92–93 | 2 |
| SpO2 ≤ 91 | ≤ 91 | 3 |

This is good.

## 5.2 Major Clinical Risk — SQI Gating Can Mask Critical Events

The supplied `clinical_vitals_assess_full()` snippet does:

```c
if (sqi > 0.0f && sqi < 0.70f) {
    out->level = CLINICAL_ELEVATED;
    out->alert_flags = ALERT_SIGNAL_NOISE;
    out->hr    = hr;
    out->spo2  = spo2;
}
```

If this path returns early, a patient with:

- SpO2 = 85%
- HR = 150
- Poor SQI due to motion

would be classified as only **ELEVATED** for signal noise, not **CRITICAL**. That is clinically unsafe.

**Severity:** **Critical**  
**Recommendation:** Never allow SQI gating to suppress absolute crisis thresholds. Implement crisis override:

```c
if (sqi < 0.70f &&
    spo2 > SPO2_CRITICAL_HYPOXIA_MAX &&
    hr < HR_CRITICAL_TACHY_MIN &&
    hr > HR_CRITICAL_BRADY_MAX) {
    // hold for suspected noise only
} else {
    // always process NEWS2/critical thresholds first
}
```

Absolute low SpO2 and extreme HR/RR should always be scored regardless of SQI.

## 5.3 SQI Morphology Edge Cases

### F-501 — Skewness division by zero not shown  
The SQI code computes variance and skewness, but the supplied snippet stops before the final sigma and skewness calculation. If variance is zero, the skewness denominator may be zero.

**Severity:** Moderate  
**Recommendation:** Add:

```c
if (variance < 1e-6f) {
    skew_score = 0.5f;
} else {
    skew = m3 / (variance * sqrtf(variance));
}
```

## 5.4 Respiratory Rate Engine

The supplied snippet has a guard:

```c
if (var_sum < 1e-3f) return;
```

This is good. However, the rest of the autocorrelation and peak detection is not shown.

### F-502 — Missing full autocorrelation validation  
The code computes an autocorrelation over IBI values, but the normalization and denominator are not shown. Need proof that autocorrelation does not divide by zero for flat, slow, or highly regular signals.

**Severity:** Moderate  
**Recommendation:** Provide the full function and test with synthetic constant IBI, sudden IBI shifts, and sparse beats.

## 5.5 Heat Risk / Disaster Engine

### F-503 — Heat index formula is physiologically incomplete  
The supplied formula:

```c
heat_index = ambient_temp_c + 0.5f * (humidity_pct - HEAT_HUMIDITY_BASE_PCT) * 0.1f;
```

is a severe simplification. For example, at 45 °C and 80% humidity, this adds only 2 °C, giving 47 °C. A clinically meaningful heat index would be higher.

This can produce **false low risk** in dangerous humid heat.

**Severity:** Moderate  
**Recommendation:** Use a validated heat-index formula or NOAA/NWS Steadman approximation.

### F-504 — Advisory pointer not checked for NULL  
Functions like `assess_heat_risk()` assign:

```c
*advisory = "Thermal status normal";
```

without checking `advisory != NULL`.

**Severity:** Low  
**Recommendation:** Add pointer guards if the API allows optional advisory output.

---

# 6. Final Verdict and Certification

## 6.1 Critical Blockers

| ID | Blocker | Severity |
|---|---|---|
| CB-1 | MAX30100 register map is incorrect | Critical |
| CB-2 | SQI motion gating can suppress crisis alarms | Critical |
| CB-3 | Quantized model zero-point math may be wrong | High |
| CB-4 | GPIO3 strapping pin used for FPGA reset | Major |
| CB-5 | Bidirectional FPGA bus direction/contention not proven | Major |

## 6.2 Certification Decision

### ❌ NOT CERTIFIED AS ZERO-FLAW

Because of the critical clinical safety issue and MAX30100 driver mismatch,  
**I cannot issue a zero-flaw certification for this code as supplied.**

### ✅ Conditional Prototype Certification

The architecture is directionally sound, but only a **conditional development-use certification** can be issued if the following mandatory actions are completed:

1. Fix the MAX30100 register map and test with a real Green MAX30100 module.
2. Add absolute clinical crisis overrides regardless of SQI.
3. Validate all INT8 dequantization formulas against the Python export.
4. Move FPGA reset away from GPIO3.
5. Prove bidirectional FPGA bus timing and output-enable behavior.
6. Add zero-variance and NULL-pointer guards.
7. Correct PMSA003 passive-mode documentation and verify UART mode.

## 6.3 Formal Statement for Jury

> Based on the supplied source slices, this project is **not yet ready for a zero-flaw clinical or hardware certification**. The system architecture is promising and many interfaces are coherent, but the MAX30100 driver is critically wrong, and the motion-artifact gating path could mask genuine crisis physiology. I recommend a focused corrective sprint before hardware demonstration or clinical demos.

---

**Auditor Signature**

**Chief Hardware Architect, Embedded Systems Auditor, Biomedical AI Principal**  
Smart India Hackathon 2026 / Qualcomm Hardware Challenge  
Date: 09-MAY-2026

**Zero-Flaw Certification:** **NOT GRANTED**  
**Conditional Readiness:** **DEVELOPMENT ONLY**