/*
 * disaster_risk_engine.h
 * Multi-Disaster Health Risk Assessment Module
 */
#ifndef DISASTER_RISK_ENGINE_H
#define DISASTER_RISK_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include "hrv_analysis.h"
#include "nn_risk_model.h"

/* --- Heat Risk Thresholds --- */
#define HEAT_TEMP_BASE_C        27.0f
#define HEAT_HUMIDITY_BASE_PCT  40.0f
#define HEAT_INDEX_CRITICAL     54.0f
#define HEAT_INDEX_HIGH         45.0f
#define HEAT_INDEX_MODERATE     40.0f
#define HEAT_INDEX_CAUTION      35.0f
#define HEAT_BPM_CRITICAL       130.0f
#define HEAT_BPM_HIGH           110.0f
#define HEAT_BPM_MODERATE       95.0f
#define HEAT_RMSSD_CRITICAL     10.0f
#define HEAT_RMSSD_HIGH         20.0f
#define HEAT_RMSSD_MODERATE     35.0f
#define HEAT_CTSI_CRITICAL      70.0f
#define HEAT_CTSI_HIGH          50.0f
#define HEAT_CTSI_MODERATE      30.0f

/* --- Pollution Risk Thresholds --- */
#define POLLUTION_PM25_CRITICAL 300.0f
#define POLLUTION_PM25_HIGH     150.0f
#define POLLUTION_PM25_MODERATE 75.0f
#define POLLUTION_PM25_CAUTION  35.0f
#define POLLUTION_SPO2_CRITICAL 88.0f
#define POLLUTION_SPO2_HIGH     92.0f
#define POLLUTION_SPO2_MODERATE 94.0f
#define POLLUTION_SPO2_CAUTION  96.0f
#define POLLUTION_BPM_CRITICAL  120.0f
#define POLLUTION_BPM_HIGH      100.0f
#define POLLUTION_RMSSD_CRIT    15.0f
#define POLLUTION_RMSSD_HIGH    25.0f
#define POLLUTION_PRSI_CRITICAL 70.0f
#define POLLUTION_PRSI_HIGH     50.0f
#define POLLUTION_PRSI_MODERATE 30.0f

/* --- Flood/Cold Risk Thresholds --- */
#define FLOOD_SKIN_TEMP_CRIT    28.0f
#define FLOOD_SKIN_TEMP_HIGH    32.0f
#define FLOOD_SKIN_TEMP_MOD     34.0f
#define FLOOD_BPM_BRADYCARDIA   50.0f
#define FLOOD_BPM_TACHY_EXTREME 150.0f
#define FLOOD_BPM_TACHY_MOD     130.0f
#define FLOOD_RMSSD_CRITICAL    8.0f
#define FLOOD_RMSSD_HIGH        15.0f
#define FLOOD_SCORE_CRITICAL    60.0f
#define FLOOD_SCORE_HIGH        40.0f
#define FLOOD_SCORE_MODERATE    20.0f

#ifdef __cplusplus
extern "C" {
#endif

/* Risk Levels */
typedef enum {
    RISK_NORMAL   = 0,   /* Green  — all parameters within safe range       */
    RISK_MODERATE = 1,   /* Yellow — early signs, take precautionary action  */
    RISK_HIGH     = 2,   /* Orange — significant strain, intervene now       */
    RISK_CRITICAL = 3    /* Red    — imminent danger, emergency response     */
} risk_level_t;

/* Environmental Sensor Inputs */
typedef struct {
    float ambient_temp_c;   /* Ambient temperature (degrees Celsius)          */
    float humidity_pct;     /* Relative humidity (0-100%)                     */
    float pm25;             /* PM2.5 concentration (micrograms per m^3)       */
    float skin_temp_c;      /* Skin temperature (degrees C), 0 if unavailable */
} env_sensors_t;

/* Risk Assessment Output */
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

/* Run AI-powered neural network disaster risk assessment. */
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
