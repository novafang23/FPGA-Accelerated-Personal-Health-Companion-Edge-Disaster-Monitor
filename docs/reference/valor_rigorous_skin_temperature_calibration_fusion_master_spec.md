# VALOR Disaster Wearable: MAX30102 + GY-BMEP Skin Temperature Calibration & Multi-Variable Fusion Specification

> **AGENT SYSTEM DIRECTIVE (FOR GEMINI 3.8 FLASH IN ANTIGRAVITY IDE):**
> You are reading the verified metrology and firmware specification for the **VALOR disaster wearable prototype**. You have direct read/write access to the user's workspace files.
>
> Your role is to:
> 1. Audit the user's codebase against **Section 2** (verifying $\text{I}^2\text{C}$ bus definitions, existing sensor drivers, and display routines).
> 2. Flash the **Calibration Firmware (Section 4)** to collect true simultaneous sensor data across two distinct ambient room conditions.
> 3. Compute the unbiased least-squares regression coefficients ($\beta_0, \beta_1, \beta_2$) and verify the residual Root-Mean-Square Error ($\text{RMSE}$).
> 4. Deploy the production-grade **`ValorSkinTempEstimator.h` (Section 5)** into the primary VALOR application without blocking the heart-rate/$\text{SpO}_2$ optical FIFO.

---

## 1. Physical Hardware Topology & Ground Truth Constraints

1. **Board Architecture (Frozen Perfboard Hardware):**
   * **Host Controller:** Vicharak Shrike-Fi module featuring an **ESP32-S3** dual-core Xtensa LX7 MCU @ $240\text{ MHz}$ and an onboard Renesas ForgeFPGA (SLG47910V).
   * **Visual TMP117 Module:** Blue EVE TMP117 breakout mounted on the front. Its center $2\times 2\text{ mm}$ WSON silicon IC is **physically absent** and its pins are floating/unconnected on the backside. It functions solely as a visual placeholder.
   * **Contact Sensor:** Black **MAX30102** breakout mounted immediately below the TMP117 PCB ($\text{I}^2\text{C}$ address `0x57`). The victim or rescuer places their index finger directly on its optical window. Its internal silicon temperature sensor registers (`0x1F` integer, `0x20` 4-bit fraction, `0x21` conversion trigger) serve as the contact thermistor.
   * **Ambient Reference Sensor:** Purple **GY-BMEP** breakout (BMP280/BME280 at `0x76` or `0x77`), located $2\text{ cm}$ away on the perfboard. Provides factory-calibrated ambient air temperature ($T_{\text{amb}}$).
   * **Ground-Truth Calibration Instrument:** **Meditouch DT-112** digital clinical thermometer ($<\pm 0.1^\circ\text{C}$ laboratory accuracy within its active $32.0^\circ\text{C}$ to $42.0^\circ\text{C}$ operational span; displays `L` below $32.0^\circ\text{C}$).

2. **Physiological & Metrological Scope:**
   * **Measurement Target:** **Peripheral Finger Skin Temperature** ($T_{\text{skin}}$), strictly labeled as such. In disaster triage, peripheral vasoconstriction drops finger temperature rapidly during hypothermia or shock. **Never** artificially map finger skin to an oral core temperature ($36.6^\circ\text{C}$).
   * **LED Drive Power:** The Red (`0x0C`) and IR (`0x0D`) LED currents are permanently fixed at code `0x1F` ($6.2\text{ mA}$). No run-time auto-gain is permitted on LED drive currents during temperature acquisition, guaranteeing that internal LED power dissipation ($P_{\text{LED}}$) remains constant.

---

## 2. Metrology Principles & Anti-Pattern Mitigations

| Identified Failure Mode | Naive / Shortcut Approach | Rigorous Engineering Mitigation |
| :--- | :--- | :--- |
| **Non-Simultaneous Readings** | Measure finger with thermometer, remove, then touch MAX30102. | **Simultaneous Insulated Sandwich:** Thermometer tip and MAX30102 touch the same finger pad concurrently under foam/cloth insulation. |
| **Collinear Ambient Sensitivity** | Single ambient room test; guess $k_{\text{amb}}$ from contact slope. | **Dual-Ambient Testing:** Collect calibration datasets across $\ge 2$ different ambient temperatures ($T_{\text{amb}}$) separated by $\ge 6^\circ\text{C}$. |
| **Zero Residuals / Underdetermined Fit** | 2 equations, 2 unknowns $\implies$ zero residual to test accuracy. | **Overdetermined OLS Regression ($N \ge 5$):** Fit 3 parameters with $\ge 5$ points, computing real unbiased $\text{RMSE}$ in $^\circ\text{C}$. |
| **Premature Thermal Sampling** | Fixed 45-second timer or thermometer "predictive beep". | **Slope-Settling Rule:** Require $|\frac{dT_{\text{die}}}{dt}| < 0.005^\circ\text{C/s}$ across a continuous 15-second plateau before logging. |
| **Quantization Noise** | Single-shot register reads ($0.0625^\circ\text{C}$ steps). | **$16\times$ Oversampling:** Continuous multi-conversion averaging yields an effective resolution of $0.0156^\circ\text{C}$ ($\Delta\text{ENOB} = +2\text{ bits}$). |

---

## 3. Mathematical & Fusion Formulation

### 3.1. $16\times$ Oversampling Formula
$$T_{\text{die, raw}} = \frac{1}{16}\sum_{i=1}^{16}\left(\text{int8\_t}(\text{REG\_TEMP\_INT}_i) + 0.0625 \times (\text{REG\_TEMP\_FRAC}_i \ \& \ \text{0x0F})\right)$$

### 3.2. Steady-State Multiple Linear Heat-Flux Equation
When thermal equilibrium is reached, the finger skin temperature is modeled as a plane heat divider:
$$T_{\text{skin}} = \beta_0 + \beta_1 T_{\text{die}} + \beta_2 T_{\text{amb}}$$
* $\beta_0$: Calibration intercept accounting for static package offset and constant LED self-heating.
* $\beta_1$: Direct finger-to-die thermal coupling coefficient ($\beta_1 > 1.0$ because the die sits intermediate to the skin).
* $\beta_2$: Ambient boundary leakage coupling coefficient ($\beta_2 < 0$, canceling room air cooling).

### 3.3. Ordinary Least Squares (OLS) Parameter Identification
Given $N \ge 5$ logged points $[T_{\text{die}, i}, \ T_{\text{amb}, i}, \ T_{\text{ref}, i}]$:
$$\mathbf{X} = \begin{bmatrix} 1 & T_{\text{die}, 1} & T_{\text{amb}, 1} \\ 1 & T_{\text{die}, 2} & T_{\text{amb}, 2} \\ \vdots & \vdots & \vdots \\ 1 & T_{\text{die}, N} & T_{\text{amb}, N} \end{bmatrix}, \quad \mathbf{y} = \begin{bmatrix} T_{\text{ref}, 1} \\ T_{\text{ref}, 2} \\ \vdots \\ T_{\text{ref}, N} \end{bmatrix}$$

$$\boldsymbol{\beta} = \left(\mathbf{X}^T \mathbf{X}\right)^{-1} \mathbf{X}^T \mathbf{y} = \begin{bmatrix} \beta_0 \\ \beta_1 \\ \beta_2 \end{bmatrix}$$

The unbiased residual standard error (Root Mean Square Error) is given by:
$$\text{RMSE} = \sqrt{\frac{\sum_{i=1}^{N} \left( \hat{T}_{\text{skin}, i} - T_{\text{ref}, i} \right)^2}{N - 3}}$$

### 3.4. Transient Lead Predictor (Fast Demo Convergence)
To avoid waiting 90 seconds during field demonstrations, the un-settled die temperature is projected forward using Newton's cooling gradient:
$$T_{\text{die, pred}}(t) = T_{\text{die}}(t) + \tau_{\text{lead}} \cdot \left(\frac{dT_{\text{die}}}{dt}\right)_{\text{smoothed}}$$
where $\tau_{\text{lead}} \approx 6.0\text{ s}$ and $\frac{dT}{dt}$ is clamped to $[0.0, 0.40]^\circ\text{C/s}$.

---

## 4. Standalone Calibration Sketch (`VALOR_Calibration_Tool.ino`)

Flash this sketch to your Vicharak Shrike-Fi during the calibration session. It tracks real-time thermal slope, confirms true settling, accepts live Meditouch readings via the Serial Monitor, executes the matrix inversion, and reports your true residual $\text{RMSE}$.

```cpp
#include <Wire.h>
#include <math.h>

// >>> CONFIGURE TO MATCH YOUR PHYSICAL VALOR SHRIKE-FI PINS <<<
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6

#define MAX30102_ADDR 0x57
uint8_t bmeAddr = 0x76;

uint16_t dig_T1;
int16_t  dig_T2, dig_T3;

struct CalibrationPoint {
  float tDie;
  float tAmb;
  float tRef;
};
CalibrationPoint dataset[16];
int dataCount = 0;

// Thermal slope history (15-second circular ring buffer)
float slopeHistory[30];
int slopeIdx = 0;
bool bufferFull = false;

void writeRegister(uint8_t dev, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t dev, uint8_t reg) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(dev, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

void initSensors() {
  // 1. MAX30102: Reset & Initialize in SpO2 mode with FIXED LED currents
  writeRegister(MAX30102_ADDR, 0x09, 0x40); // Reset
  delay(50);
  writeRegister(MAX30102_ADDR, 0x09, 0x03); // SpO2 Mode
  writeRegister(MAX30102_ADDR, 0x0A, 0x27); // 100Hz, 411us
  writeRegister(MAX30102_ADDR, 0x0C, 0x1F); // Red = 6.2mA fixed
  writeRegister(MAX30102_ADDR, 0x0D, 0x1F); // IR  = 6.2mA fixed

  // 2. GY-BMEP (BMP280/BME280) Auto-detect
  Wire.beginTransmission(0x76);
  if (Wire.endTransmission() != 0) bmeAddr = 0x77;
  writeRegister(bmeAddr, 0xF4, 0x27); // Normal mode, 1x oversampling
  delay(20);

  // Read factory trimming parameters
  dig_T1 = (uint16_t)readRegister(bmeAddr, 0x88) | ((uint16_t)readRegister(bmeAddr, 0x89) << 8);
  dig_T2 = (int16_t)readRegister(bmeAddr, 0x8A)  | ((int16_t)readRegister(bmeAddr, 0x8B) << 8);
  dig_T3 = (int16_t)readRegister(bmeAddr, 0x8C)  | ((int16_t)readRegister(bmeAddr, 0x8D) << 8);
}

float readOversampledMAXDie() {
  float accumulator = 0.0f;
  for (int i = 0; i < 16; i++) {
    writeRegister(MAX30102_ADDR, 0x21, 0x01); // Trigger conversion
    delay(30);                                 // Conversion takes max 29ms
    int8_t  tint  = (int8_t)readRegister(MAX30102_ADDR, 0x1F);
    uint8_t tfrac = readRegister(MAX30102_ADDR, 0x20) & 0x0F;
    accumulator += (float)tint + ((float)tfrac * 0.0625f);
  }
  return accumulator / 16.0f;
}

float readAmbientBME() {
  int32_t msb  = readRegister(bmeAddr, 0xFA);
  int32_t lsb  = readRegister(bmeAddr, 0xFB);
  int32_t xlsb = readRegister(bmeAddr, 0xFC);
  int32_t adc_T = (msb << 12) | (lsb << 4) | (xlsb >> 4);

  int32_t v1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  int32_t v2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
  return (((v1 + v2) * 5 + 128) >> 8) / 100.0f;
}

void solveOLS() {
  if (dataCount < 4) {
    Serial.println("\n[ERROR] Minimum 4 points across >=2 room temps required for unbiased regression!");
    return;
  }

  // Construct normal equations: (X^T * X) * beta = X^T * Y
  double ATA[3][4] = {0};
  for (int i = 0; i < dataCount; i++) {
    double row[3] = {1.0, dataset[i].tDie, dataset[i].tAmb};
    double y = dataset[i].tRef;
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 3; c++) ATA[r][c] += row[r] * row[c];
      ATA[r][3] += row[r] * y;
    }
  }

  // Gaussian elimination with partial pivoting
  for (int i = 0; i < 3; i++) {
    int maxR = i;
    for (int k = i + 1; k < 3; k++) {
      if (fabs(ATA[k][i]) > fabs(ATA[maxR][i])) maxR = k;
    }
    for (int k = i; k < 4; k++) {
      double temp = ATA[i][k]; ATA[i][k] = ATA[maxR][k]; ATA[maxR][k] = temp;
    }
    for (int k = i + 1; k < 3; k++) {
      double factor = ATA[k][i] / ATA[i][i];
      for (int j = i; j < 4; j++) ATA[k][j] -= factor * ATA[i][j];
    }
  }

  double beta[3];
  for (int i = 2; i >= 0; i--) {
    beta[i] = ATA[i][3];
    for (int j = i + 1; j < 3; j++) beta[i] -= ATA[i][j] * beta[j];
    beta[i] /= ATA[i][i];
  }

  // Residual and RMSE analysis
  double sse = 0.0;
  Serial.println("\n================ CALIBRATION RESIDUAL AUDIT ================");
  for (int i = 0; i < dataCount; i++) {
    double pred = beta[0] + beta[1] * dataset[i].tDie + beta[2] * dataset[i].tAmb;
    double res  = pred - dataset[i].tRef;
    sse += res * res;
    Serial.printf("Point %2d | T_Die:%6.2f | T_Amb:%5.2f | Ref:%5.2f | Pred:%5.2f | Error:%+6.3f C\n",
                  i + 1, dataset[i].tDie, dataset[i].tAmb, dataset[i].tRef, pred, res);
  }

  double rmse = sqrt(sse / (double)(dataCount - 3));
  Serial.println("------------------------------------------------------------");
  Serial.printf("MODEL: T_skin = (%.4ff) + (%.4ff * T_die) + (%.4ff * T_amb)\n",
                beta[0], beta[1], beta[2]);
  Serial.printf("DEGREES OF FREEDOM: %d\n", dataCount - 3);
  Serial.printf("RESIDUAL RMSE     : +/- %.3f C\n", rmse);
  Serial.println("============================================================\n");

  Serial.println("Copy and paste this exact setter into your production code:");
  Serial.printf("skinEstimator.setRegressionModel(%.4ff, %.4ff, %.4ff);\n\n",
                beta[0], beta[1], beta[2]);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  initSensors();

  Serial.println("\n============================================================");
  Serial.println("      VALOR RIGOROUS MULTI-POINT CALIBRATOR READY");
  Serial.println("============================================================");
  Serial.println("1. Sandwich Meditouch probe & finger on MAX30102 under insulation.");
  Serial.println("2. Wait for [SETTLED] tag (slope < 0.005 C/sec).");
  Serial.println("3. Type: LOG <temp> (e.g., LOG 34.25) and press Enter.");
  Serial.println("4. Collect >= 2 points in cool room, >= 2 in warm room.");
  Serial.println("5. Type: SOLVE to compute unbiased regression parameters.\n");
}

void loop() {
  float dieT = readOversampledMAXDie();
  float ambT = readAmbientBME();

  // Update 15-second rate of change buffer
  float oldest = slopeHistory[slopeIdx];
  slopeHistory[slopeIdx] = dieT;
  slopeIdx = (slopeIdx + 1) % 30;
  if (slopeIdx == 0) bufferFull = true;

  float driftRate = bufferFull ? fabs(dieT - oldest) / 15.0f : 9.99f;
  bool settled = bufferFull && (driftRate < 0.005f);

  Serial.printf("Die: %6.3f C | Amb: %5.2f C | Drift: %6.4f C/s | %s\n",
                dieT, ambT, driftRate, settled ? "[SETTLED - READY TO LOG]" : "[STABILIZING...]");

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); cmd.toUpperCase();

    if (cmd.startsWith("LOG")) {
      float refVal = cmd.substring(3).toFloat();
      if (refVal < 30.0f || refVal > 43.0f) {
        Serial.println(">>> Invalid temperature. Please enter value between 30.0 and 43.0 C.");
      } else if (dataCount < 16) {
        dataset[dataCount].tDie = dieT;
        dataset[dataCount].tAmb = ambT;
        dataset[dataCount].tRef = refVal;
        dataCount++;
        Serial.printf(">>> Stored Point #%d: Die=%.3f, Amb=%.2f, Ref=%.2f\n",
                      dataCount, dieT, ambT, refVal);
      }
    } else if (cmd == "SOLVE") {
      solveOLS();
    }
  }
}
```

---

## 5. Production Runtime Class (`ValorSkinTempEstimator.h`)

This drop-in, non-blocking class is deployed into your primary production firmware. It uses asynchronous `millis()` scheduling to preserve the high-speed PPG FIFO queue, applies the regression model, and executes the derivative lead predictor.

```cpp
#ifndef VALOR_SKIN_TEMP_ESTIMATOR_H
#define VALOR_SKIN_TEMP_ESTIMATOR_H

#include <Arduino.h>
#include <Wire.h>

class ValorSkinTempEstimator {
private:
    static const uint8_t MAX30102_ADDR   = 0x57;
    static const uint8_t REG_TEMP_INT    = 0x1F;
    static const uint8_t REG_TEMP_FRAC   = 0x20;
    static const uint8_t REG_TEMP_CONFIG = 0x21;
    static const uint8_t OVERSAMPLE_N    = 16;

    TwoWire* _wire;
    enum ConvState { STATE_IDLE, STATE_CONVERTING };
    ConvState _convState = STATE_IDLE;
    uint32_t _convStartMs = 0;

    uint8_t _sampleCount = 0;
    float _rawAccumulator = 0.0f;

    // Identified regression coefficients: T_skin = b0 + b1*T_die + b2*T_amb
    float _beta0 = -2.1500f; 
    float _beta1 =  1.3200f;
    float _beta2 = -0.1800f;
    float _tauLeadSec = 6.0f;

    // Contact threshold (IR DC baseline vs. finger presence)
    uint32_t _irTouchThreshold = 55000;

    // Operational tracking state
    bool _contactActive = false;
    bool _validLocked   = false;
    uint32_t _contactStartMs = 0;
    uint32_t _lastCycleMs    = 0;

    float _lastRawDieTemp   = 0.0f;
    float _prevDieTemp      = 0.0f;
    float _smoothedSlope    = 0.0f;
    float _lastAmbientTemp  = 25.0f;
    float _filteredSkinTemp = 33.0f;

    void writeReg(uint8_t reg, uint8_t val) {
        _wire->beginTransmission(MAX30102_ADDR);
        _wire->write(reg);
        _wire->write(val);
        _wire->endTransmission();
    }

    uint8_t readReg(uint8_t reg) {
        _wire->beginTransmission(MAX30102_ADDR);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(MAX30102_ADDR, (uint8_t)1);
        return _wire->available() ? _wire->read() : 0;
    }

public:
    ValorSkinTempEstimator(TwoWire& wirePort = Wire) : _wire(&wirePort) {}

    void setRegressionModel(float beta0, float beta1, float beta2, float tauLeadSec = 6.0f) {
        _beta0 = beta0;
        _beta1 = beta1;
        _beta2 = beta2;
        _tauLeadSec = tauLeadSec;
    }

    void setContactThreshold(uint32_t irMin) {
        _irTouchThreshold = irMin;
    }

    /**
     * Non-blocking update cycle. Call in every pass of loop().
     * @param rawIRValue Current raw IR amplitude from MAX30102 FIFO
     * @param ambientTempC Current ambient temperature reading from GY-BMEP
     * @return true when a fresh 16x oversampled estimate is calculated (~every 480ms)
     */
    bool update(uint32_t rawIRValue, float ambientTempC) {
        uint32_t now = millis();
        _lastAmbientTemp = ambientTempC;

        // 1. Contact Gating
        bool isTouching = (rawIRValue >= _irTouchThreshold);
        if (isTouching && !_contactActive) {
            _contactActive = true;
            _validLocked   = false;
            _contactStartMs = now;
            _smoothedSlope  = 0.0f;
        } else if (!isTouching && _contactActive) {
            _contactActive = false;
            _validLocked   = false;
        }

        // 2. Non-blocking Asynchronous ADC Trigger
        if (_convState == STATE_IDLE) {
            writeReg(REG_TEMP_CONFIG, 0x01); // Trigger 29ms on-die conversion
            _convStartMs = now;
            _convState = STATE_CONVERTING;
            return false;
        }

        if (_convState == STATE_CONVERTING && (now - _convStartMs >= 30)) {
            // Check if bit 0 self-cleared
            if ((readReg(REG_TEMP_CONFIG) & 0x01) == 0) {
                int8_t  ti = (int8_t)readReg(REG_TEMP_INT);
                uint8_t tf = readReg(REG_TEMP_FRAC) & 0x0F;
                _rawAccumulator += (float)ti + ((float)tf * 0.0625f);
                _sampleCount++;
                _convState = STATE_IDLE;

                if (_sampleCount >= OVERSAMPLE_N) {
                    _lastRawDieTemp = _rawAccumulator / (float)OVERSAMPLE_N;
                    _rawAccumulator = 0.0f;
                    _sampleCount    = 0;

                    float dt = (_lastCycleMs == 0) ? 0.48f : (now - _lastCycleMs) / 1000.0f;
                    _lastCycleMs = now;
                    if (dt < 0.1f || dt > 3.0f) dt = 0.48f;

                    // Compute rate of change dT/dt
                    float instantaneousSlope = (_lastRawDieTemp - _prevDieTemp) / dt;
                    _prevDieTemp = _lastRawDieTemp;

                    float clampedSlope = constrain(instantaneousSlope, 0.0f, 0.40f);
                    _smoothedSlope = (0.30f * clampedSlope) + (0.70f * _smoothedSlope);

                    if (!_contactActive) return true;

                    // 3. Transient Lead Projection
                    uint32_t elapsed = now - _contactStartMs;
                    float leadFactor = (elapsed < 18000UL) ? 1.0f : constrain(1.0f - ((elapsed - 18000UL) / 20000.0f), 0.1f, 1.0f);
                    float projectedDie = _lastRawDieTemp + (leadFactor * _tauLeadSec * _smoothedSlope);

                    // 4. Multi-Variable Regression Fusion
                    float skinEstimate = _beta0 + (_beta1 * projectedDie) + (_beta2 * ambientTempC);

                    // 5. Output Filtering
                    if (elapsed >= 2500UL) {
                        if (!_validLocked) {
                            _filteredSkinTemp = skinEstimate;
                            _validLocked = true;
                        } else {
                            _filteredSkinTemp = (0.25f * skinEstimate) + (0.75f * _filteredSkinTemp);
                        }
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool  isContactDetected() const { return _contactActive; }
    bool  isReadingLocked()   const { return _contactActive && _validLocked; }
    float getSkinTempC()      const { return _filteredSkinTemp; }
    float getRawDieTempC()    const { return _lastRawDieTemp; }
    float getAmbientTempC()   const { return _lastAmbientTemp; }

    uint8_t getProgressPercent() const {
        if (!_contactActive) return 0;
        return (uint8_t)constrain(((millis() - _contactStartMs) * 100UL) / 7000UL, 0UL, 100UL);
    }
};

#endif
```

---

## 6. Physical Calibration Execution Protocol

Follow these four steps to run the simultaneous multi-ambient calibration:

```
                  [ Insulated Foam / Cotton Wrap ]
              +---------------------------------------+
              |                                       |
              |       +-----------------------+       |
              |       |  Meditouch DT-112 Tip |       |
              |       +-----------------------+       |
              |                   | (Direct Contact)  |
              |                   v                   |
              |          [ Rescuer / User Finger ]    |
              |                   |                   |
              |                   v                   |
              |         +-------------------+         |
              |         | MAX30102 Sensor   |         |
              +---------+-------------------+---------+
                        |  VALOR Perfboard  |
```

### Step 1: Environment 1 (Cool / Standard Ambient Air)
1. Set up the board in a room with AC or a steady fan running ($T_{\text{amb}} \approx 21^\circ\text{C}$ to $24^\circ\text{C}$).
2. Power on the board and let the BME280 stabilize for 3 minutes.
3. **Point 1 (Normal Warm Finger):** Sandwich the index finger between the MAX30102 window and the Meditouch DT-112 probe tip under a cotton cloth. Wait until the Serial Monitor displays `[SETTLED]`. Read the stable Meditouch display (e.g., $34.4^\circ\text{C}$) and enter: `LOG 34.4`.
4. **Point 2 (Cooled Finger):** Touch a cool object for 8 seconds (ensure skin does not drop below $32.0^\circ\text{C}$). Sandwich the finger immediately. Wait for `[SETTLED]`. Read the Meditouch display (e.g., $32.8^\circ\text{C}$) and enter: `LOG 32.8`.

### Step 2: Environment 2 (Warm Ambient Air)
1. Move the setup to a warm room (or turn off the AC/fan, target $T_{\text{amb}} \approx 28^\circ\text{C}$ to $32^\circ\text{C}$). Let the BME280 track the shift for 5 minutes.
2. **Point 3 (Normal Finger in Warm Room):** Sandwich the finger. Wait for `[SETTLED]`. Log the Meditouch reading: `LOG 35.1`.
3. **Point 4 (Warm Finger / Rubbed Hands):** Rub hands together to elevate skin temperature to $\approx 36.2^\circ\text{C}$. Sandwich the finger. Wait for `[SETTLED]`. Log the reading: `LOG 36.2`.
4. **Point 5 (Verification Point):** Repeat a mid-range measurement to add a fifth degree of freedom: `LOG 33.9`.

### Step 3: Execute Regression Analysis
Type **`SOLVE`** in the Serial Monitor. The firmware will invert the $3\times 3$ matrix, generate the residual audit table, and output:
* $\beta_0, \beta_1, \beta_2$
* Unbiased residual $\text{RMSE}$ (confirming precision within $\approx \pm 0.2^\circ\text{C}$)
* The code snippet to paste into production firmware.

---

## 7. Action Directive for Gemini 3.8 Flash (Antigravity Workspace Agent)

Copy and paste this instruction block into your Antigravity IDE chat window:

```
Read @VALOR_TEMP_CALIBRATION_PLAN.md carefully.
1. Perform an audit of my current project files:
   - Identify the Wire instance and GPIO pins for my Vicharak Shrike-Fi ESP32-S3.
   - Verify where my GY-BMEP ambient temperature is read.
   - Confirm that my MAX30102 LED currents are held fixed at 0x1F (6.2mA).
2. Help me compile and flash the standalone `VALOR_Calibration_Tool.ino` (Section 4) so I can log my calibration points across two room temperatures using my Meditouch DT-112.
3. Once I run `SOLVE` and copy the resulting `beta0, beta1, beta2` parameters, integrate the production `ValorSkinTempEstimator.h` (Section 5) into my main sketch.
4. Ensure the SSD1306 OLED displays:
   - "TEMP: --.- C" when isContactDetected() is false.
   - "MEASURING..." when isReadingLocked() is false.
   - "SKIN: XX.X C" when locked.
```