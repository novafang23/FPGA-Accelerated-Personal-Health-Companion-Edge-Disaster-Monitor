/*
 * nn_risk_model.h
 * Neural Network Risk Assessment Model
 */

#ifndef NN_RISK_MODEL_H
#define NN_RISK_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Model Dimensions */
#define NN_INPUT_SIZE    6    /* HR, RMSSD, SpO2, Temp, Humidity, PM2.5 */
#define NN_HIDDEN_SIZE   12   /* Hidden layer neurons                   */
#define NN_OUTPUT_SIZE   3    /* Heat, Pollution, Flood risk scores     */

/* Feature Normalization Ranges */
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

/* Float32 Neural Network Model Structure */
typedef struct {
    /* Layer 1: Input(6) -> Hidden(12) */
    float W1[NN_HIDDEN_SIZE][NN_INPUT_SIZE];   /* Weight matrix  (12x6)  */
    float b1[NN_HIDDEN_SIZE];                   /* Bias vector    (12)    */

    /* Layer 2: Hidden(12) -> Output(3) */
    float W2[NN_OUTPUT_SIZE][NN_HIDDEN_SIZE];  /* Weight matrix  (3x12)  */
    float b2[NN_OUTPUT_SIZE];                   /* Bias vector    (3)     */
} nn_model_t;

/* Inference Result */
typedef struct {
    float heat_score;        /* [0.0, 1.0] -- heat/thermal risk     */
    float pollution_score;   /* [0.0, 1.0] -- air pollution risk    */
    float flood_score;       /* [0.0, 1.0] -- flood/hypothermia     */
} nn_output_t;

/* Get the default pre-trained float32 model. */
const nn_model_t* nn_get_default_model(void);

/* Run forward-pass inference on the float32 neural network. */
void nn_predict(
    const nn_model_t *model,
    float hr, float rmssd, float spo2,
    float temp, float hum, float pm25,
    nn_output_t *out
);

/* INT8 Quantized Model Support */
/* Include nn_risk_model_int8.h for INT8 API */

#ifdef __cplusplus
}
#endif

#endif /* NN_RISK_MODEL_H */
