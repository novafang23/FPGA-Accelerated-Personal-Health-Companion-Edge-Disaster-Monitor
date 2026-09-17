/* VALOR (Vital and Atmospheric Logic for Offline Rescue) PM2.5 low-cost sensor calibration module.
 * 3 -> 8 -> 4 -> 1 fully-connected ReLU network.
 * Quantization: symmetric int8 activations, int32 biases/accumulators.
 */
#include "pm25_calibration_int8.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/* Input quantization scales.
 * PM2.5 raw input is divided by 4.0 so that 0..~508 ug/m3 maps to int8 0..127.
 * Temperature and humidity use 1:1 mapping (their natural ranges fit int8).
 */
#define PM_INPUT_SCALE_Q      4.0f
#define TEMP_INPUT_SCALE_Q    1.0f
#define RH_INPUT_SCALE_Q      1.0f

/* Valid operating envelope for the quantized NN */
#define PM25_RAW_MAX_INPUT    508.0f
#define PM25_TEMP_MIN         (-40.0f)
#define PM25_TEMP_MAX         125.0f

/* Fixed-power-of-two requantization shifts */
#define L1_SHIFT              6U
#define L2_SHIFT              6U
#define OUT_SHIFT             6U

#define L1_ROUND              (1U << (L1_SHIFT - 1U))
#define L2_ROUND              (1U << (L2_SHIFT - 1U))
#define OUT_SCALE_DIV         (1U << OUT_SHIFT)

/* Layer 1: 3 inputs -> 8 neurons, ReLU. */
static const int8_t s_w1[8][3] = {
    {  59,  -4,   0 },  /* linear base, scaled by 0.5 */
    {  26,   0,  64 },  /* RH + 0.1*PM - 80 correction detector */
    {   0,   0,   0 },
    {   0,   0,   0 },
    {   0,   0,   0 },
    {   0,   0,   0 },
    {   0,   0,   0 },
    {   0,   0,   0 }
};

static const int32_t s_b1[8] = {
     271,      /* 8.47/2 * 64 */
   -5120,      /* -80 * 64 */
    -64,
    -64,
    -64,
    -64,
    -64,
    -64
};

/* Layer 2: 8 inputs -> 4 neurons, ReLU.
 * Neurons 0 and 1 pass through h1[0] and h1[1].
 * Neurons 2 and 3 are intentionally disabled.
 */
static const int8_t s_w2[4][8] = {
    { 64,  0, 0, 0, 0, 0, 0, 0 },
    {  0, 64, 0, 0, 0, 0, 0, 0 },
    {  0,  0, 0, 0, 0, 0, 0, 0 },
    {  0,  0, 0, 0, 0, 0, 0, 0 }
};

static const int32_t s_b2[4] = {
       0,
       0,
    -128,
    -128
};

/* Output layer: 4 -> 1, linear. */
static const int8_t s_w3[1][4] = {
    { 127, -85, 0, 0 }
};

static const int32_t s_b3[1] = { 0 };

static int8_t saturate_i8(int32_t value);

static int8_t saturate_i8(int32_t value)
{
    if (value > 127) {
        return 127;
    }
    if (value < -128) {
        return -128;
    }
    return (int8_t)value;
}

void pm25_calibration_init(void)
{
    /* Network weights and biases are compile-time constants. No runtime init needed. */
}

bool pm25_is_humidity_distorted(float raw_pm25, float humidity_pct)
{
    if (!isfinite((double)raw_pm25) || !isfinite((double)humidity_pct)) {
        return false;
    }

    if ((raw_pm25 <= 0.0f) || (humidity_pct < 0.0f) || (humidity_pct > 100.0f)) {
        return false;
    }

    return (humidity_pct > 60.0f) && (raw_pm25 > 10.0f);
}

float pm25_calibrate_nn_int8(float raw_pm25, float temp_c, float humidity_pct)
{
    int8_t q_in[3];
    int8_t h1[8];
    int8_t h2[4];
    int32_t q_pm;
    int32_t q_temp;
    int32_t q_rh;
    int32_t acc;
    int32_t out_acc;
    float result;
    uint32_t i;
    uint32_t j;

    /* Integrity and range guards. Never return a negative PM value. */
    if (!isfinite((double)raw_pm25) ||
        !isfinite((double)temp_c) ||
        !isfinite((double)humidity_pct)) {
        return (raw_pm25 > 0.0f) ? raw_pm25 : 0.0f;
    }

    if (raw_pm25 < 0.0f) {
        return 0.0f;
    }

    if (raw_pm25 > PM25_RAW_MAX_INPUT) {
        return raw_pm25; /* pass-through above the calibrated quantized range */
    }

    if ((temp_c < PM25_TEMP_MIN) ||
        (temp_c > PM25_TEMP_MAX) ||
        (humidity_pct < 0.0f) ||
        (humidity_pct > 100.0f)) {
        return raw_pm25; /* corrupted or missing sensor data: pass-through */
    }

    /* Quantize inputs to signed int8. */
    q_pm = (int32_t)lroundf(raw_pm25 / PM_INPUT_SCALE_Q);
    q_temp = (int32_t)lroundf(temp_c / TEMP_INPUT_SCALE_Q);
    q_rh = (int32_t)lroundf(humidity_pct / RH_INPUT_SCALE_Q);

    q_in[0] = saturate_i8(q_pm);
    q_in[1] = saturate_i8(q_temp);
    q_in[2] = saturate_i8(q_rh);

    /* Layer 1: 8 neurons, ReLU. */
    for (i = 0U; i < 8U; ++i) {
        acc = s_b1[i];
        acc += (int32_t)q_in[0] * (int32_t)s_w1[i][0];
        acc += (int32_t)q_in[1] * (int32_t)s_w1[i][1];
        acc += (int32_t)q_in[2] * (int32_t)s_w1[i][2];

        if (acc < 0) {
            h1[i] = 0;
        } else {
            h1[i] = saturate_i8((acc + (int32_t)L1_ROUND) >> L1_SHIFT);
        }
    }

    /* Layer 2: 4 neurons, ReLU. */
    for (i = 0U; i < 4U; ++i) {
        acc = s_b2[i];
        for (j = 0U; j < 8U; ++j) {
            acc += (int32_t)h1[j] * (int32_t)s_w2[i][j];
        }

        if (acc < 0) {
            h2[i] = 0;
        } else {
            h2[i] = saturate_i8((acc + (int32_t)L2_ROUND) >> L2_SHIFT);
        }
    }

    /* Output layer: 1 neuron, linear. */
    out_acc = s_b3[0];
    for (i = 0U; i < 4U; ++i) {
        out_acc += (int32_t)h2[i] * (int32_t)s_w3[0][i];
    }

    result = (float)out_acc / (float)OUT_SCALE_DIV;

    if (result < 0.0f) {
        result = 0.0f;
    }

    return result;
}
