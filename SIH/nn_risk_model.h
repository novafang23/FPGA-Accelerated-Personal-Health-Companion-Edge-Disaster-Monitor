/*
 * nn_risk_model.h — TinyML Neural Network Risk Assessment Model
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * A lightweight 2-layer feedforward neural network for multi-disaster
 * health risk prediction, designed for edge inference on resource-
 * constrained platforms (Qualcomm Hexagon DSP / AI Engine, ARM Cortex-M/A).
 *
 * Architecture:
 *   Input Layer  : 6 neurons  (normalized sensor features)
 *   Hidden Layer : 12 neurons (ReLU activation)
 *   Output Layer : 3 neurons  (Sigmoid activation → risk probabilities)
 *
 * Input Feature Vector (6 features):
 *   [0] Heart Rate        (BPM)
 *   [1] HRV RMSSD         (ms)
 *   [2] SpO2              (%)
 *   [3] Ambient Temp      (°C)
 *   [4] Humidity           (%)
 *   [5] PM2.5             (µg/m³)
 *
 * Output Risk Probabilities (3 outputs):
 *   [0] Heat Risk          (0.0 = safe, 1.0 = critical)
 *   [1] Pollution Risk     (0.0 = safe, 1.0 = critical)
 *   [2] Flood/Cold Risk    (0.0 = safe, 1.0 = critical)
 *
 * Model Size: 123 parameters (492 bytes), single-pass inference < 1 µs
 *
 * On Qualcomm platforms, this model would be compiled to Hexagon DSP
 * microcode via the Qualcomm Neural Processing SDK (SNPE/QNN) for
 * hardware-accelerated inference on the Hexagon NPU.
 */

#ifndef NN_RISK_MODEL_H
#define NN_RISK_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Model Dimensions ---- */
#define NN_INPUT_SIZE    6    /* HR, RMSSD, SpO2, Temp, Humidity, PM2.5 */
#define NN_HIDDEN_SIZE   12   /* Hidden layer neurons                   */
#define NN_OUTPUT_SIZE   3    /* Heat, Pollution, Flood risk scores     */

/* ---- Feature Normalization Ranges ----
 * Min-max scaling to [0, 1] based on clinical/environmental ranges.
 * These ranges cover the full physiological and environmental spectrum
 * relevant to Indian disaster scenarios.
 */
#define NN_HR_MIN        40.0f     /* Bradycardia floor        */
#define NN_HR_MAX        200.0f    /* Extreme tachycardia      */
#define NN_RMSSD_MIN     1.0f      /* Near-zero HRV            */
#define NN_RMSSD_MAX     100.0f    /* Strong parasympathetic   */
#define NN_SPO2_MIN      70.0f     /* Severe hypoxemia         */
#define NN_SPO2_MAX      100.0f    /* Normal saturation        */
#define NN_TEMP_MIN      -10.0f    /* Winter cold              */
#define NN_TEMP_MAX      55.0f     /* Extreme heat wave        */
#define NN_HUM_MIN       0.0f      /* Dry                      */
#define NN_HUM_MAX       100.0f    /* Fully saturated          */
#define NN_PM25_MIN      0.0f      /* Clean air                */
#define NN_PM25_MAX      500.0f    /* Severe AQI 500+          */

/* ---- Neural Network Model Structure ---- */
typedef struct {
    /* Layer 1: Input(6) → Hidden(12) */
    float W1[NN_HIDDEN_SIZE][NN_INPUT_SIZE];   /* Weight matrix  (12×6)  */
    float b1[NN_HIDDEN_SIZE];                   /* Bias vector    (12)    */

    /* Layer 2: Hidden(12) → Output(3) */
    float W2[NN_OUTPUT_SIZE][NN_HIDDEN_SIZE];  /* Weight matrix  (3×12)  */
    float b2[NN_OUTPUT_SIZE];                   /* Bias vector    (3)     */
} nn_model_t;

/* ---- Inference Result ---- */
typedef struct {
    float heat_score;        /* [0.0, 1.0] — heat/thermal risk     */
    float pollution_score;   /* [0.0, 1.0] — air pollution risk    */
    float flood_score;       /* [0.0, 1.0] — flood/hypothermia     */
} nn_output_t;

/*
 * Get the default pre-trained model.
 * Returns a pointer to statically-allocated model weights.
 *
 * In production, these weights would be loaded from a Qualcomm QNN
 * model file (.dlc) trained on clinical datasets. For this prototype,
 * weights are embedded directly in firmware.
 */
const nn_model_t* nn_get_default_model(void);

/*
 * Run forward-pass inference on the neural network.
 *
 * Parameters:
 *   model  — pointer to the trained model weights
 *   hr     — heart rate (BPM)
 *   rmssd  — HRV RMSSD (ms)
 *   spo2   — blood oxygen saturation (%)
 *   temp   — ambient temperature (°C)
 *   hum    — relative humidity (%)
 *   pm25   — PM2.5 concentration (µg/m³)
 *   out    — pointer to output struct (filled on return)
 *
 * Computational cost: 108 multiply-accumulates (MACs)
 * Latency: < 1 µs on ARM Cortex-A9 @ 667 MHz
 *          < 100 ns on Qualcomm Hexagon DSP @ 1.4 GHz
 */
void nn_predict(
    const nn_model_t *model,
    float hr, float rmssd, float spo2,
    float temp, float hum, float pm25,
    nn_output_t *out
);

#ifdef __cplusplus
}
#endif

#endif /* NN_RISK_MODEL_H */
