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
    int32_t xi = (int32_t)roundf(x);
    if (xi > 255) return 255;
    if (xi < 0)   return 0;
    return (uint8_t)xi;
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

    /* Quantize hidden (post-ReLU) activations to act1 space.
     * act1_zp is a uint8_t zero-point in [0,255] (asymmetric quant, since
     * ReLU output is >= 0) -- the quantized code must be stored as uint8_t,
     * not int8_t, or any code above 127 silently wraps/clips wrong. */
    uint8_t hidden_q[NN_HIDDEN_SIZE];
    for (i = 0; i < NN_HIDDEN_SIZE; i++) {
        float val = hidden[i] / qparams->act1_scale + (float)qparams->act1_zp;
        hidden_q[i] = clamp_uint8(val);
    }

    /* Layer 2: Hidden(12) -> Output(3)
     * Dequantize hidden_q back to float for the MAC (signed subtraction --
     * hidden_q and act1_zp are both uint8_t, so promote to float/int first
     * or an underflow wraps to a huge unsigned value). */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sum = qparams->b2_scale * (float)model->b2[i];
        for (j = 0; j < NN_HIDDEN_SIZE; j++) {
            float hidden_val = ((float)hidden_q[j] - (float)qparams->act1_zp) * qparams->act1_scale;
            sum += qparams->W2_scale * (float)model->W2[i][j] * hidden_val;
        }
        out_logit[i] = sum;
    }

    /* Apply sigmoid to the real-valued logit, THEN fake-quantize the
     * post-sigmoid activation with act2_scale/act2_zp -- those parameters
     * were calibrated in Python on a2 = sigmoid(z2), i.e. on the probability
     * output, not on the pre-sigmoid logit. Quantizing before sigmoid (the
     * old code) applied the wrong scale to the wrong tensor. */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sigmoid_out = 1.0f / (1.0f + expf(-out_logit[i]));
        float q_val = sigmoid_out / qparams->act2_scale + (float)qparams->act2_zp;
        uint8_t out_q = clamp_uint8(q_val);
        out_logit[i] = ((float)out_q - (float)qparams->act2_zp) * qparams->act2_scale;
    }

    out->heat_score      = out_logit[0];
    out->pollution_score = out_logit[1];
    out->flood_score     = out_logit[2];
}