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

/* Clamp a rounded float to the UINT8 range [0,255] used by the asymmetric
 * activation quantization (quantize_asymmetric() in train_nn_risk_model.py
 * always produces uint8 codes, since ReLU and sigmoid outputs are >= 0). */
static uint8_t clamp_uint8(float x) {
    if (!isfinite(x) || x <= 0.0f) return 0;
    if (x >= 255.0f) return 255;
    return (uint8_t)roundf(x);
}

void nn_predict_int8(
    const nn_model_int8_t *model,
    const nn_quant_params_t *qparams,
    float hr, float rmssd, float spo2,
    float temp, float hum, float pm25,
    nn_output_t *out
) {
    if (!model || !qparams || !out) return;

    int i, j;
    float input[NN_INPUT_SIZE];
    float h1[NN_HIDDEN1_SIZE];
    float h2[NN_HIDDEN2_SIZE];
    float out_logit[NN_OUTPUT_SIZE];

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

    /* Layer 1: Input(6) -> Hidden1(24)
     * Use dequantized weights and bias with explicit zero-point offset */
    for (i = 0; i < NN_HIDDEN1_SIZE; i++) {
        float sum = qparams->b1_scale * (float)((int32_t)model->b1[i] - (int32_t)qparams->b1_zp);
        for (j = 0; j < NN_INPUT_SIZE; j++) {
            sum += qparams->W1_scale * (float)((int32_t)model->W1[i][j] - (int32_t)qparams->W1_zp) * input[j];
        }
        /* Apply ReLU */
        if (sum < 0.0f) sum = 0.0f;
        h1[i] = sum;
    }

    /* Quantize Hidden1 activations to act1 space */
    uint8_t h1_q[NN_HIDDEN1_SIZE];
    for (i = 0; i < NN_HIDDEN1_SIZE; i++) {
        float val = h1[i] / qparams->act1_scale + (float)qparams->act1_zp;
        h1_q[i] = clamp_uint8(val);
    }

    /* Layer 2: Hidden1(24) -> Hidden2(16) */
    for (i = 0; i < NN_HIDDEN2_SIZE; i++) {
        float sum = qparams->b2_scale * (float)((int32_t)model->b2[i] - (int32_t)qparams->b2_zp);
        for (j = 0; j < NN_HIDDEN1_SIZE; j++) {
            float h1_val = ((float)h1_q[j] - (float)qparams->act1_zp) * qparams->act1_scale;
            sum += qparams->W2_scale * (float)((int32_t)model->W2[i][j] - (int32_t)qparams->W2_zp) * h1_val;
        }
        /* Apply ReLU */
        if (sum < 0.0f) sum = 0.0f;
        h2[i] = sum;
    }

    /* Quantize Hidden2 activations to act2 space */
    uint8_t h2_q[NN_HIDDEN2_SIZE];
    for (i = 0; i < NN_HIDDEN2_SIZE; i++) {
        float val = h2[i] / qparams->act2_scale + (float)qparams->act2_zp;
        h2_q[i] = clamp_uint8(val);
    }

    /* Layer 3: Hidden2(16) -> Output(3) */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sum = qparams->b3_scale * (float)((int32_t)model->b3[i] - (int32_t)qparams->b3_zp);
        for (j = 0; j < NN_HIDDEN2_SIZE; j++) {
            float h2_val = ((float)h2_q[j] - (float)qparams->act2_zp) * qparams->act2_scale;
            sum += qparams->W3_scale * (float)((int32_t)model->W3[i][j] - (int32_t)qparams->W3_zp) * h2_val;
        }
        out_logit[i] = sum;
    }

    /* Apply sigmoid to the real-valued logit, THEN fake-quantize the
     * post-sigmoid activation with act3_scale/act3_zp */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sigmoid_out = 1.0f / (1.0f + expf(-out_logit[i]));
        float q_val = sigmoid_out / qparams->act3_scale + (float)qparams->act3_zp;
        uint8_t out_q = clamp_uint8(q_val);
        out_logit[i] = ((float)out_q - (float)qparams->act3_zp) * qparams->act3_scale;
    }

    out->heat_score      = out_logit[0];
    out->pollution_score = out_logit[1];
    out->flood_score     = out_logit[2];
}