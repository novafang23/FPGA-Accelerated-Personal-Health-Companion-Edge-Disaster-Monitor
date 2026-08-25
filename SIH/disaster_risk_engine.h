/*
 * disaster_risk_engine.h — Multi-Disaster Health Risk Assessment Module
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * Fuses physiological signals (HR, HRV, SpO2) with environmental sensors
 * (temperature, humidity, PM2.5/AQI) to generate real-time early warnings
 * for Indian disaster scenarios:
 *
 *   1. Heat Wave    → Cardio-Thermal Strain Index (CTSI)
 *   2. Air Pollution → Pollution Respiratory Strain Index (PRSI)
 *   3. Flood/Cold   → Hypothermia & Exertion Collapse Detector
 */

#ifndef DISASTER_RISK_ENGINE_H
#define DISASTER_RISK_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include "hrv_analysis.h"
#include "nn_risk_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Risk Levels ---- */
typedef enum {
    RISK_NORMAL   = 0,   /* Green  — all parameters within safe range       */
    RISK_MODERATE = 1,   /* Yellow — early signs, take precautionary action  */
    RISK_HIGH     = 2,   /* Orange — significant strain, intervene now       */
    RISK_CRITICAL = 3    /* Red    — imminent danger, emergency response     */
} risk_level_t;

/* ---- Environmental Sensor Inputs ---- */
typedef struct {
    float ambient_temp_c;   /* Ambient temperature (degrees Celsius)          */
    float humidity_pct;     /* Relative humidity (0-100%)                     */
    float pm25;             /* PM2.5 concentration (micrograms per m^3)       */
    float skin_temp_c;      /* Skin temperature (degrees C), 0 if unavailable */
} env_sensors_t;

/* ---- Risk Assessment Output ---- */
typedef struct {
    risk_level_t heat_risk;
    risk_level_t pollution_risk;
    risk_level_t flood_risk;
    risk_level_t overall_risk;

    const char *heat_advisory;
    const char *pollution_advisory;
    const char *flood_advisory;
    const char *overall_advisory;
} risk_assessment_t;

/* Convert risk level enum to human-readable string */
const char* risk_level_to_string(risk_level_t level);

/* Get ANSI color escape code for the risk level */
const char* risk_level_to_color(risk_level_t level);

/*
 * Run multi-disaster health risk assessment.
 *
 * Inputs:
 *   hrv   — pointer to HRV analysis state (NULL if unavailable)
 *   spo2  — current SpO2 percentage (0-100)
 *   bpm   — current heart rate in beats per minute
 *   env   — environmental sensor readings
 *
 * Output:
 *   result — filled with per-disaster and overall risk levels + advisories
 */
void disaster_assess(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
);

/*
 * Run AI-powered neural network disaster risk assessment.
 *
 * Uses a 2-layer feedforward neural network (6→12→3) to predict
 * risk levels from the same sensor inputs. The NN captures inter-
 * parameter correlations that threshold-based scoring may miss.
 *
 * On Qualcomm platforms, this inference would run on the Hexagon NPU
 * via SNPE/QNN SDK for hardware-accelerated prediction.
 */
void disaster_assess_nn(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
);

#ifdef __cplusplus
}
#endif

#endif /* DISASTER_RISK_ENGINE_H */
