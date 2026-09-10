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

/* Pre-Trained Model Weights.
 * This is #include'd from the file train_nn_risk_model.py actually writes,
 * instead of a hand-pasted literal, so retraining takes effect the moment
 * nn_risk_model_trained.c.inc is regenerated -- no manual copy-paste step
 * that's easy to forget. */
#include "nn_risk_model_trained.c.inc"

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
    float h1[NN_HIDDEN1_SIZE];
    float h2[NN_HIDDEN2_SIZE];
    float output[NN_OUTPUT_SIZE];

    /* Feature Normalization */
    input[0] = normalize(hr,    NN_HR_MIN,    NN_HR_MAX);
    input[1] = normalize(rmssd, NN_RMSSD_MIN, NN_RMSSD_MAX);
    input[2] = normalize(spo2,  NN_SPO2_MIN,  NN_SPO2_MAX);
    input[3] = normalize(temp,  NN_TEMP_MIN,  NN_TEMP_MAX);
    input[4] = normalize(hum,   NN_HUM_MIN,   NN_HUM_MAX);
    input[5] = normalize(pm25,  NN_PM25_MIN,  NN_PM25_MAX);

    /* Layer 1: Input -> Hidden1 (ReLU) */
    for (i = 0; i < NN_HIDDEN1_SIZE; i++) {
        float sum = model->b1[i];
        for (j = 0; j < NN_INPUT_SIZE; j++) {
            sum += model->W1[i][j] * input[j];
        }
        h1[i] = relu(sum);
    }

    /* Layer 2: Hidden1 -> Hidden2 (ReLU) */
    for (i = 0; i < NN_HIDDEN2_SIZE; i++) {
        float sum = model->b2[i];
        for (j = 0; j < NN_HIDDEN1_SIZE; j++) {
            sum += model->W2[i][j] * h1[j];
        }
        h2[i] = relu(sum);
    }

    /* Layer 3: Hidden2 -> Output (Sigmoid) */
    for (i = 0; i < NN_OUTPUT_SIZE; i++) {
        float sum = model->b3[i];
        for (j = 0; j < NN_HIDDEN2_SIZE; j++) {
            sum += model->W3[i][j] * h2[j];
        }
        output[i] = sigmoid(sum);
    }

    /* Store results */
    out->heat_score      = output[0];
    out->pollution_score = output[1];
    out->flood_score     = output[2];
}
