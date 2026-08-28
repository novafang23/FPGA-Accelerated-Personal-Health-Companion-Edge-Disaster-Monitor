/*
 * nn_risk_model_int8.c
 * INT8 Quantized Neural Network Inference Engine
 * Per-tensor symmetric quantization for weights, asymmetric for activations
 * Computes in float using dequantized weights to match fake-quant behavior
 */

#include "nn_risk_model_int8.h"
#include <math.h>
#include <stdint.h>

#include "nn_risk_model_int8.c.inc"

/* Feature Normalization Ranges (must match nn_risk_model.h) */
#define NN_HR_MIN        40.0f
#define NN_HR_MAX        200.0f
#define NN_RMSSD_MIN     1.0f
#define NN_RMSSD_MAX     100.0f
#define NN_SPO2_MIN      70.0f
#define NN_SPO2_MAX      100.0f
#define NN_TEMP_MIN      -10.0f
#define NN_TEMP_MAX      55.0f
#define NN_HUM_MIN       0.0f
#define NN_HUM_MAX       100.0f
#define NN_PM25_MIN      0.0f
#define NN_PM25_MAX      500.0f

/* Clamp float to int8 range */
static int8_t clamp_int8(float x) {
    int32_t xi = (int32_t)roundf(x);
    if (xi > 127) return 127;
    if (xi < -128) return -128;
    return (int8_t)xi;
}

/* Sigmoid approximation */
static float sigmoid_int32(int32_t x, float scale) {
    float fx = x * scale;
    if (fx > 10.0f) return 1.0f;
    if (fx < -10.0f) return 0.0f;
    return 1.0f / (1.0f + expf(-fx));
}

void nn_predict_int8(
    const nn_model_int8_t *model,
    const nn_quant_params_t *qparams,
    float hr, float rmssd, float spo2,
    float temp, float hum, float pm25,
    nn_output_t *out
) {
    int i, j;
    float input[NN_INPUT_SIZE];
    float hidden[NN_HIDDEN_SIZE];
    int32_t output_acc[NN_OUTPUT_SIZE];

    /* Normalize inputs to [0,1] float (matching fake-quant) */
    input[0] = (hr    - NN_HR_MIN)    / (NN_HR_MAX    - NN_HR_MIN);
    input[1] = (rmssd - NN_RMSSD_MIN) / (NN_RMSSD_MAX - NN_RMSSD_MIN);
    input[2] = (spo2  - NN_SPO2_MIN)  / (NN_SPO2_MAX  - NN_SPO2_MIN);
    input[3] = (temp  - NN_TEMP_MIN)  / (NN_TEMP_MAX  - NN_TEMP_MIN);
    input[4] = (hum   - NN_HUM_MIN)   / (NN_HUM_MAX   - NN_HUM_MIN);
    input[5] = (pm25  - NN_PM25_MIN)  / (NN_PM25_MAX  - NN_PM25_MIN);

    /* Clamp to [0,1] */
    for (i = 0; i < NN_INPUT_SIZE; i++) {
        if (input[i] < 0.0f) input[i] = 0.0f;
        if (input[i] > 1.0f) input[i] = 1.0f;
    }

    /* Layer 1: Input(6) -> Hidden(12)
     * Use dequantized weights and bias (matching fake-quant) */
    for (i = 0; i < NN_HIDDEN_SIZE; i++) {
        float sum = qparams->b1_scale * (float)model->b1[i];
        for (j = 0; j < NN_INPUT_SIZE; j++) {
            sum += qparams->W1_scale * (float)model->W1[i][j] * input[j];
        }
        /* Apply ReLU */
        if (sum < 0.0f) sum = 0.0f;
        /* Quantize to act1 space for next layer */
        hidden[i] = sum;
    }

    /* Quantize hidden activations to act1 space (INT8) */
    int8_t hidden_q[NN_HIDDEN_SIZE];
    for (i = 0; i < NN_HIDDEN_SIZE; i++) {
        float val = hidden[i] / qparams->act1_scale + qparams->act1_zp;
        hidden_q[i] = clamp_int8(val);
    }

    /* Layer 2: Hidden(12) -> Output(3)
     * Hidden activations are quantized to act1 space, dequantize for MAC */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sum = qparams->b2_scale * (float)model->b2[i];
        for (j = 0; j < NN_HIDDEN_SIZE; j++) {
            float hidden_val = (hidden_q[j] - qparams->act1_zp) * qparams->act1_scale;
            sum += qparams->W2_scale * (float)model->W2[i][j] * hidden_val;
        }
        /* Quantize to act2 space */
        output_acc[i] = (int32_t)roundf(sum / qparams->act2_scale + qparams->act2_zp);
    }

    /* Dequantize and apply sigmoid */
    out->heat_score      = sigmoid_int32(output_acc[0], qparams->act2_scale);
    out->pollution_score = sigmoid_int32(output_acc[1], qparams->act2_scale);
    out->flood_score     = sigmoid_int32(output_acc[2], qparams->act2_scale);
}