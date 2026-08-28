/*
 * nn_risk_model_int8.c
 * INT8 Quantized Neural Network Inference Engine
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

/* Quantized normalize: float -> int8 */
static int8_t quantize_input(float val, float min_val, float max_val, float scale, int8_t zp) {
    float norm = (val - min_val) / (max_val - min_val);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    int32_t q = (int32_t)roundf(norm / scale + zp);
    if (q > 127) q = 127;
    if (q < -128) q = -128;
    return (int8_t)q;
}

/* Clamp int32 to int8 range */
static int8_t clamp_int8(int32_t x) {
    if (x > 127) return 127;
    if (x < -128) return -128;
    return (int8_t)x;
}

/* ReLU for int32 accumulator */
static int32_t relu_int32(int32_t x) {
    return (x > 0) ? x : 0;
}

/* Sigmoid approximation using lookup table or polynomial */
/* Using fast sigmoid: x / (1 + |x|) scaled appropriately */
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
    /* Suppress unused static function warnings */
    (void)quantize_input;
    (void)clamp_int8;
    (void)relu_int32;
    (void)sigmoid_int32;
    int i, j;
    int8_t input_q[NN_INPUT_SIZE];
    int8_t hidden_q[NN_HIDDEN_SIZE];
    int32_t output_acc[NN_OUTPUT_SIZE];

    /* Quantize inputs to INT8 */
    input_q[0] = quantize_input(hr,    NN_HR_MIN,    NN_HR_MAX,    qparams->W1_scale, qparams->W1_zp);
    input_q[1] = quantize_input(rmssd, NN_RMSSD_MIN, NN_RMSSD_MAX, qparams->W1_scale, qparams->W1_zp);
    input_q[2] = quantize_input(spo2,  NN_SPO2_MIN,  NN_SPO2_MAX,  qparams->W1_scale, qparams->W1_zp);
    input_q[3] = quantize_input(temp,  NN_TEMP_MIN,  NN_TEMP_MAX,  qparams->W1_scale, qparams->W1_zp);
    input_q[4] = quantize_input(hum,   NN_HUM_MIN,   NN_HUM_MAX,   qparams->W1_scale, qparams->W1_zp);
    input_q[5] = quantize_input(pm25,  NN_PM25_MIN,  NN_PM25_MAX,  qparams->W1_scale, qparams->W1_zp);

    /* Layer 1: Input(6) -> Hidden(12) with INT8 MAC accumulation into int32 */
    for (i = 0; i < NN_HIDDEN_SIZE; i++) {
        /* bias in Q8.0 (INT8 shifted to Q8.0) */
        int32_t sum = (int32_t)model->b1[i] << 8;
        for (j = 0; j < NN_INPUT_SIZE; j++) {
            sum += (int32_t)model->W1[i][j] * (int32_t)input_q[j];
        }
        /* Apply ReLU and requantize to act1 scale */
        sum = relu_int32(sum);
        /* Requantize for next layer: sum * W1_scale / act1_scale */
        float val = (sum * qparams->W1_scale) / qparams->act1_scale + qparams->act1_zp;
        hidden_q[i] = clamp_int8((int32_t)roundf(val));
    }

    /* Layer 2: Hidden(12) -> Output(3) */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        int32_t sum = (int32_t)model->b2[i] << 8;
        for (j = 0; j < NN_HIDDEN_SIZE; j++) {
            sum += (int32_t)model->W2[i][j] * (int32_t)hidden_q[j];
        }
        output_acc[i] = sum;
    }

    /* Dequantize and apply sigmoid */
    out->heat_score      = sigmoid_int32(output_acc[0], qparams->act2_scale);
    out->pollution_score = sigmoid_int32(output_acc[1], qparams->act2_scale);
    out->flood_score     = sigmoid_int32(output_acc[2], qparams->act2_scale);
}