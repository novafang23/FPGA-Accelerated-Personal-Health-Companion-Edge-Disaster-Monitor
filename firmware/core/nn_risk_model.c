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
        { -0.209833f,  0.072789f, -0.020244f,  2.547783f,  0.077890f, -0.006233f },
        { -0.093441f,  0.019074f, -0.204326f,  0.194599f, -0.005375f,  1.209286f },
        {  0.136419f, -0.028576f, -0.097752f, -0.029071f, -0.022326f, -1.655743f },
        { -0.130202f, -2.240849f, -0.022094f,  0.169095f, -0.026191f,  0.011137f },
        { -2.695268f,  0.049833f,  0.009109f, -0.680536f, -0.006513f, -0.005892f },
        {  2.561465f,  0.013708f,  0.008444f, -0.560077f, -0.014107f,  0.001502f },
        { -0.133336f, -1.795159f, -0.009608f, -0.125028f,  0.002989f, -0.002440f },
        { -2.478677f,  0.036038f, -0.013320f,  0.209338f, -0.005801f,  0.013733f },
        { -0.361249f, -0.397144f,  0.060808f,  0.340282f,  0.070072f, -0.053094f },
        {  0.179512f, -0.015233f, -0.005108f,  1.432641f,  0.031005f,  0.031384f },
        {  2.058246f,  0.025932f,  0.007225f,  0.405939f, -0.005403f, -0.118477f },
        {  0.117599f, -0.010628f,  2.212906f, -0.009460f,  0.014320f,  0.144627f },
    },
    .b1 = { -1.343528f,  0.174328f,  0.570941f,  0.481745f,  1.454804f, -0.331796f,  0.902218f,  1.565012f,  0.228646f, -0.236927f,  0.029432f, -1.345512f },
    .W2 = {
        {  1.718732f, -0.105301f,  0.006202f,  0.580319f, -0.246806f,  0.858964f,  1.602564f, -1.349903f,  0.537189f,  0.473972f, -0.952487f,  0.007979f },
        { -0.007954f,  1.139447f, -1.556182f,  0.532575f,  0.116499f,  0.474987f,  0.556647f, -0.802373f, -0.027134f,  0.231917f, -0.446390f, -2.112116f },
        {  1.792566f, -0.105240f,  0.013179f,  1.984097f,  2.663684f,  2.323762f,  0.214463f, -1.799940f, -0.039697f, -1.256814f, -1.728963f,  0.034719f },
    },
    .b2 = { -0.650193f,  1.135125f, -0.329304f },
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
