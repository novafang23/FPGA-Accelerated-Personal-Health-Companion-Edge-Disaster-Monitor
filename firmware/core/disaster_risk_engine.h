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
#include "nn_risk_model_int8.h"

/* --- Heat Risk Thresholds --- */
#define HEAT_TEMP_BASE_C        27.0f
#define HEAT_HUMIDITY_BASE_PCT  40.0f
#define HEAT_INDEX_CRITICAL     54.0f
#define HEAT_INDEX_HIGH         45.0f
#define HEAT_INDEX_MODERATE     40.0f
#define HEAT_INDEX_CAUTION      35.0f
/* The NOAA/NWS Rothfusz regression is fitted over a limited domain and diverges
 * badly outside it -- unclamped it returns 106 C for 46.5 C / 68 % RH and 180 C
 * for 50 C / 90 % RH. The published NWS heat-index table tops out around
 * 57.8 C (136 F), so clamp to that. This cannot change triage: every heat
 * branch only tests heat_index > 54/45/40/35, all of which stay true. */
#define HEAT_INDEX_MAX_C        58.0f
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

/* --- Flood/Cold Risk Thresholds ---
 *
 * `FLOOD_SKIN_TEMP_*` are SKIN-temperature thresholds and must only ever be fed
 * a measured skin temperature. Skin and air temperature are not interchangeable:
 * normal skin is ~33-35 C while normal air is ~25 C. Feeding ambient air into
 * these would classify a pleasant 28 C afternoon as severe hypothermia.
 */
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

/* --- Cold-Stress Proxy (used only when no skin-temperature sensor exists) ---
 *
 * The ShrikeFi build carries a BME280, which measures AMBIENT AIR. When
 * `env->skin_temp_c` is 0 (no skin sensor) the engine falls back to this
 * ambient-air cold-stress estimate rather than reporting RISK_UNKNOWN forever.
 *
 * The proxy scores three things: air temperature, relative humidity (wet cold
 * removes heat far faster than dry cold, which is the dominant mechanism in
 * flooding), and the same cardiac / autonomic terms the clinical path uses.
 * It is deliberately CAPPED AT RISK_HIGH: without a measured skin or core
 * temperature a CRITICAL hypothermia call cannot be justified.
 *
 * NOTE this is an exposure-risk estimate, not a hypothermia diagnosis. Ambient
 * air temperature is a weak proxy for core temperature - wind, immersion,
 * clothing and wetness dominate - so the advisory text says "cold-stress
 * exposure" and never claims a measured clinical state.
 *
 * Framework and cold physiology:
 *   Moran DS, Castellani JW, O'Brien C, Young AJ, Pandolf KB. "Evaluating
 *   physiological strain during cold exposure using a new cold strain index."
 *   Am J Physiol. 1999;277(2):R556-64. doi:10.1152/ajpregu.1999.277.2.R556
 *   (the cold analogue of the Moran PSI already used for heat stress)
 *   Castellani JW, Young AJ. "Human physiological responses to cold exposure."
 *   Auton Neurosci. 2016;196:63-74.
 */
#define COLD_AMBIENT_VALID_MIN_C (-20.0f) /* outside this band the reading is  */
#define COLD_AMBIENT_VALID_MAX_C ( 65.0f) /* treated as unusable -> RISK_UNKNOWN */
#define COLD_AMBIENT_SEVERE_C      0.0f
#define COLD_AMBIENT_HIGH_C        5.0f
#define COLD_AMBIENT_MOD_C        10.0f
#define COLD_AMBIENT_MILD_C       15.0f
#define COLD_AMBIENT_COOL_C       20.0f
#define COLD_HUMIDITY_VHIGH_PCT   85.0f
#define COLD_HUMIDITY_HIGH_PCT    70.0f
#define COLD_HUMIDITY_MOD_PCT     55.0f

#ifdef __cplusplus
extern "C" {
#endif

/* Risk Levels */
typedef enum {
    RISK_UNKNOWN   = -1, /* Sensor unavailable / insufficient data            */
    RISK_NORMAL    = 0,  /* Green  -- all parameters within safe range       */
    RISK_MODERATE  = 1,  /* Yellow -- early signs, take precautionary action  */
    RISK_HIGH      = 2,  /* Orange -- significant strain, intervene now       */
    RISK_CRITICAL  = 3   /* Red    -- imminent danger, emergency response     */
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
 * Run multi-disaster health risk assessment (rule-based).
 */
void disaster_assess(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
);

/* Run AI-powered neural network disaster risk assessment (float32). */
void disaster_assess_nn(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
);

/* Run AI-powered neural network disaster risk assessment (INT8 quantized).
 * raw_out: optional pointer to receive raw model activations (pass NULL if unneeded). */
void disaster_assess_nn_int8(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result,
    nn_output_t         *raw_out
);

/* Peer-Reviewed Clinical Biomarker Models:
 * 1. Moran et al. (1998) / Buller et al. (2013) - Physiological Strain Index (PSI) [0.0 - 10.0]
 * 2. Brook et al. AHA Scientific Statement (2010) - PM2.5 Autonomic Depression Index [0.0 - 1.0]
 */
float disaster_calculate_moran_psi(float bpm, float ambient_temp_c, float humidity_pct);
float disaster_calculate_aha_autonomic_strain(float pm25, float rmssd);

#ifdef __cplusplus
}
#endif

#endif /* DISASTER_RISK_ENGINE_H */
