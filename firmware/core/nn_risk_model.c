/*
 * nn_risk_model.c
 * Neural Network Inference Engine
 */

#include "nn_risk_model.h"
#include <math.h>

/* Activation Functions */

static float relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

static float sigmoid(float x) {
    /* Clamp input to prevent exp() overflow on embedded platforms */
    if (x > 10.0f)  return 1.0f;
    if (x < -10.0f) return 0.0f;
    return 1.0f / (1.0f + expf(-x));
}

/* Feature Normalization */
static float normalize(float val, float min_val, float max_val) {
    float norm = (val - min_val) / (max_val - min_val);
    /* Clamp to [0, 1] for out-of-range inputs */
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return norm;
}

/* Pre-Trained Model Weights */
static const nn_model_t default_model = {
    .W1 = {
        { -0.209653f, 0.071563f, -0.019694f, 2.547914f, 0.077832f, -0.006686f },
        { -0.094303f, 0.019173f, -0.204177f, 0.193224f, -0.005397f, 1.209145f },
        { 0.136150f, -0.028538f, -0.097490f, -0.029265f, -0.022585f, -1.655558f },
        { -0.129236f, -2.241302f, -0.022442f, 0.168423f, -0.025690f, 0.011390f },
        { -2.695350f, 0.050177f, 0.008985f, -0.680204f, -0.006281f, -0.006359f },
        { 2.561570f, 0.014297f, 0.008333f, -0.559549f, -0.014102f, 0.001478f },
        { -0.133565f, -1.797345f, -0.009500f, -0.124247f, 0.002529f, -0.002592f },
        { -2.478358f, 0.036539f, -0.013534f, 0.209093f, -0.005599f, 0.013468f },
        { -0.357925f, -0.392179f, 0.057965f, 0.336234f, 0.072505f, -0.054250f },
        { 0.179363f, -0.015975f, -0.004582f, 1.433926f, 0.031462f, 0.031649f },
        { 2.059166f, 0.026810f, 0.006878f, 0.404925f, -0.005335f, -0.118194f },
        { 0.117345f, -0.010669f, 2.212852f, -0.009590f, 0.014208f, 0.144723f },
    },
    .b1 = { -1.343471f, 0.174710f, 0.570973f, 0.480818f, 1.455440f, -0.332047f, 0.902364f, 1.565056f, 0.226584f, -0.236748f, 0.028698f, -1.345479f },
    .W2 = {
        { 1.719136f, -0.105358f, 0.006297f, 0.579343f, -0.249187f, 0.859911f, 1.603623f, -1.348615f, 0.529018f, 0.474774f, -0.954846f, 0.008088f },
        { -0.007918f, 1.139766f, -1.556529f, 0.532269f, 0.115689f, 0.475415f, 0.556787f, -0.801858f, -0.029283f, 0.232011f, -0.446566f, -2.112335f },
        { 1.793049f, -0.104360f, 0.013492f, 1.983948f, 2.664196f, 2.323677f, 0.214436f, -1.801123f, -0.038993f, -1.257355f, -1.728464f, 0.034584f },
    },
    .b2 = { -0.647866f, 1.134963f, -0.329307f },
};

/* Public API */

const nn_model_t* nn_get_default_model(void) {
    return &default_model;
}

void nn_predict(
    const nn_model_t *model,
    float hr, float rmssd, float spo2,
    float temp, float hum, float pm25,
    nn_output_t *out
) {
    int i, j;
    float input[NN_INPUT_SIZE];
    float hidden[NN_HIDDEN_SIZE];
    float output[NN_OUTPUT_SIZE];

    /* Feature Normalization */
    input[0] = normalize(hr,    NN_HR_MIN,    NN_HR_MAX);
    input[1] = normalize(rmssd, NN_RMSSD_MIN, NN_RMSSD_MAX);
    input[2] = normalize(spo2,  NN_SPO2_MIN,  NN_SPO2_MAX);
    input[3] = normalize(temp,  NN_TEMP_MIN,  NN_TEMP_MAX);
    input[4] = normalize(hum,   NN_HUM_MIN,   NN_HUM_MAX);
    input[5] = normalize(pm25,  NN_PM25_MIN,  NN_PM25_MAX);

    /* Hidden Layer */
    for (i = 0; i < NN_HIDDEN_SIZE; i++) {
        float sum = model->b1[i];
        for (j = 0; j < NN_INPUT_SIZE; j++) {
            sum += model->W1[i][j] * input[j];
        }
        hidden[i] = relu(sum);
    }

    /* Output Layer */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sum = model->b2[i];
        for (j = 0; j < NN_HIDDEN_SIZE; j++) {
            sum += model->W2[i][j] * hidden[j];
        }
        output[i] = sigmoid(sum);
    }

    /* Store results */
    out->heat_score      = output[0];
    out->pollution_score = output[1];
    out->flood_score     = output[2];
}
