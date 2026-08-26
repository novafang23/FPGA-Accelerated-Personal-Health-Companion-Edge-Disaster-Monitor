/*
 * disaster_risk_engine.c
 * Multi-Disaster Health Risk Assessment
 *
 * Notes:
 * - Thresholds and tuning parameters are centralized as macros near the top
 *   of this file to make behavior explicit and easy to document.
 * - Advisory strings are static const so callers may safely reference them.
 * - A lightweight NN stub is provided when the model is not linked; the
 *   rule-based engines are the primary safety path.
 *
 * Changes in this branch: extracted magic numbers, added input validation,
 * and added a small test harness in SIH/tests to verify boundary conditions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "disaster_risk_engine.h"

#include <math.h>

/* Configuration: named thresholds and defaults */
#define DEFAULT_RMSSD             50.0f

/* Heat index thresholds (degrees C) */
#define HEAT_INDEX_HUMIDITY_MIN   40.0f
#define HEAT_INDEX_TEMP_SCORE_4   54.0f
#define HEAT_INDEX_TEMP_SCORE_3   45.0f
#define HEAT_INDEX_TEMP_SCORE_2   40.0f
#define HEAT_INDEX_TEMP_SCORE_1   35.0f
#define HEAT_CTSI_CRITICAL        70.0f
#define HEAT_CTSI_HIGH            50.0f
#define HEAT_CTSI_MODERATE        30.0f

/* Heart rate thresholds (bpm) */
#define BPM_CRITICAL_THRESHOLD    130.0f
#define BPM_HIGH_THRESHOLD        110.0f
#define BPM_MODERATE_THRESHOLD    95.0f

/* HRV (RMSSD) thresholds (ms) */
#define RMSSD_CRITICAL_THRESHOLD  10.0f
#define RMSSD_HIGH_THRESHOLD      20.0f
#define RMSSD_MODERATE_THRESHOLD  35.0f

/* Pollution thresholds */
#define PM25_CRITICAL             300.0f
#define PM25_HIGH                 150.0f
#define PM25_MODERATE             75.0f
#define PM25_LOW                  35.0f

#define SPO2_CRITICAL             88.0f
#define SPO2_HIGH                 92.0f
#define SPO2_MODERATE             94.0f
#define SPO2_LOW                  96.0f

#define PRSI_CRITICAL_SCORE       70.0f
#define PRSI_HIGH_SCORE           50.0f
#define PRSI_MODERATE_SCORE       30.0f

/* Flood / hypothermia thresholds */
#define SKIN_TEMP_MISSING         0.0f
#define SKIN_TEMP_CRITICAL        28.0f
#define SKIN_TEMP_HIGH            32.0f
#define SKIN_TEMP_MODERATE        34.0f

#define FLOOD_CRITICAL_SCORE      60.0f
#define FLOOD_HIGH_SCORE          40.0f
#define FLOOD_MODERATE_SCORE      20.0f

/* Advisory messages (const to ensure callers receive stable pointers) */
static const char *AD_HEAT_CRITICAL = "DANGER: Heat stroke imminent! Seek cooling, hydrate NOW";
static const char *AD_HEAT_HIGH     = "WARNING: Heat exhaustion risk. Move to shade, drink water";
static const char *AD_HEAT_MOD      = "CAUTION: Moderate heat strain. Stay hydrated, reduce exertion";
static const char *AD_HEAT_NORMAL   = "Thermal status normal";

static const char *AD_POLLUTION_CRIT = "DANGER: Severe respiratory distress! Use N95 mask, seek clean air";
static const char *AD_POLLUTION_HIGH = "WARNING: Respiratory strain. Wear mask, minimize outdoor exposure";
static const char *AD_POLLUTION_MOD  = "CAUTION: Air quality affecting health. Consider wearing a mask";
static const char *AD_POLLUTION_NORM = "Respiratory status normal";

static const char *AD_FLOOD_CRIT = "DANGER: Hypothermia/collapse risk! Seek warmth immediately";
static const char *AD_FLOOD_HIGH = "WARNING: Cold exposure stress. Dry off and seek shelter";
static const char *AD_FLOOD_MOD  = "CAUTION: Monitor body temperature closely";
static const char *AD_FLOOD_NORM = "Exposure status normal";

/* Forward Declarations */
static risk_level_t assess_heat_risk(
    float bpm, float rmssd, float ambient_temp_c, float humidity_pct,
    const char **advisory
);

static risk_level_t assess_pollution_risk(
    float bpm, float spo2, float pm25, float rmssd,
    const char **advisory
);

static risk_level_t assess_flood_risk(
    float bpm, float skin_temp_c, float rmssd,
    const char **advisory
);

/* Utility Functions */

const char* risk_level_to_string(risk_level_t level) {
    switch (level) {
        case RISK_NORMAL:   return "NORMAL";
        case RISK_MODERATE: return "MODERATE";
        case RISK_HIGH:     return "HIGH";
        case RISK_CRITICAL: return "CRITICAL";
        default:            return "UNKNOWN";
    }
}

const char* risk_level_to_color(risk_level_t level) {
    switch (level) {
        case RISK_NORMAL:   return "\033[32m";       /* Green  */
        case RISK_MODERATE: return "\033[33m";       /* Yellow */
        case RISK_HIGH:     return "\033[38;5;208m"; /* Orange */
        case RISK_CRITICAL: return "\033[31m";       /* Red    */
        default:            return "\033[0m";
    }
}

/* Heat Wave — Cardio-Thermal Strain Index (CTSI) */
static risk_level_t assess_heat_risk(
    float bpm, float rmssd, float ambient_temp_c, float humidity_pct,
    const char **advisory
) {
    /* Simplified Steadman Heat Index approximation */
    float heat_index = ambient_temp_c;
    float ctsi = 0.0f;

    if (ambient_temp_c > HEAT_INDEX_HUMIDITY_MIN && humidity_pct > HEAT_INDEX_HUMIDITY_MIN) {
        heat_index = ambient_temp_c + 0.5f * (humidity_pct - HEAT_INDEX_HUMIDITY_MIN) * 0.1f;
    }

    /* Temperature component (0-40 points) */
    if (heat_index > HEAT_INDEX_TEMP_SCORE_4)      ctsi += 40.0f;
    else if (heat_index > HEAT_INDEX_TEMP_SCORE_3) ctsi += 30.0f;
    else if (heat_index > HEAT_INDEX_TEMP_SCORE_2) ctsi += 20.0f;
    else if (heat_index > HEAT_INDEX_TEMP_SCORE_1) ctsi += 10.0f;

    /* Cardiovascular drift component (0-30 points) */
    if (bpm > BPM_CRITICAL_THRESHOLD)      ctsi += 30.0f;
    else if (bpm > BPM_HIGH_THRESHOLD)     ctsi += 20.0f;
    else if (bpm > BPM_MODERATE_THRESHOLD) ctsi += 10.0f;

    /* HRV depression component (0-30 points) */
    if (rmssd < RMSSD_CRITICAL_THRESHOLD)  ctsi += 30.0f;
    else if (rmssd < RMSSD_HIGH_THRESHOLD) ctsi += 20.0f;
    else if (rmssd < RMSSD_MODERATE_THRESHOLD) ctsi += 10.0f;

    /* Map score to risk level */
    if (ctsi >= HEAT_CTSI_CRITICAL) {
        *advisory = AD_HEAT_CRITICAL;
        return RISK_CRITICAL;
    } else if (ctsi >= HEAT_CTSI_HIGH) {
        *advisory = AD_HEAT_HIGH;
        return RISK_HIGH;
    } else if (ctsi >= HEAT_CTSI_MODERATE) {
        *advisory = AD_HEAT_MOD;
        return RISK_MODERATE;
    } else {
        *advisory = AD_HEAT_NORMAL;
        return RISK_NORMAL;
    }
}

/* Air Pollution — Pollution Respiratory Strain Index (PRSI) */
static risk_level_t assess_pollution_risk(
    float bpm, float spo2, float pm25, float rmssd,
    const char **advisory
) {
    float prsi = 0.0f;

    /* Air quality component (0-40 points) */
    if (pm25 > PM25_CRITICAL)      prsi += 40.0f;
    else if (pm25 > PM25_HIGH)     prsi += 30.0f;
    else if (pm25 > PM25_MODERATE) prsi += 20.0f;
    else if (pm25 > PM25_LOW)      prsi += 10.0f;

    /* Oxygen desaturation component (0-40 points) */
    if (spo2 < SPO2_CRITICAL)      prsi += 40.0f;
    else if (spo2 < SPO2_HIGH)     prsi += 30.0f;
    else if (spo2 < SPO2_MODERATE) prsi += 20.0f;
    else if (spo2 < SPO2_LOW)      prsi += 10.0f;

    /* Respiratory compensation — elevated pulse (0-15 points) */
    if (bpm > POLLUTION_BPM_CRITICAL)        prsi += 15.0f;
    else if (bpm > POLLUTION_BPM_HIGH)   prsi += 8.0f;

    /* Autonomic stress response (0-10 points) */
    if (rmssd < POLLUTION_RMSSD_CRIT)       prsi += 10.0f;
    else if (rmssd < POLLUTION_RMSSD_HIGH)  prsi += 5.0f;

    /* Map score to risk level */
    if (prsi >= PRSI_CRITICAL_SCORE) {
        *advisory = AD_POLLUTION_CRIT;
        return RISK_CRITICAL;
    } else if (prsi >= PRSI_HIGH_SCORE) {
        *advisory = AD_POLLUTION_HIGH;
        return RISK_HIGH;
    } else if (prsi >= PRSI_MODERATE_SCORE) {
        *advisory = AD_POLLUTION_MOD;
        return RISK_MODERATE;
    } else {
        *advisory = AD_POLLUTION_NORM;
        return RISK_NORMAL;
    }
}

/* Flood / Hypothermia / Extreme Exertion */
static risk_level_t assess_flood_risk(
    float bpm, float skin_temp_c, float rmssd,
    const char **advisory
) {
    float score = 0.0f;

    /* Skip if no skin temperature sensor data available */
    if (skin_temp_c <= 0.0f) {
        *advisory = "No skin temperature data available";
        return RISK_NORMAL;
    }

    /* Hypothermia indicators (0-40 points) */
    if (skin_temp_c < SKIN_TEMP_CRITICAL)      score += 40.0f;
    else if (skin_temp_c < SKIN_TEMP_HIGH)     score += 25.0f;
    else if (skin_temp_c < SKIN_TEMP_MODERATE) score += 10.0f;

    /* Cardiac stress: bradycardia in hypothermia OR extreme tachycardia (0-30 points) */
    if (bpm < 50.0f)               score += 30.0f;
    else if (bpm > 150.0f)         score += 30.0f;
    else if (bpm > BPM_CRITICAL_THRESHOLD)    score += 15.0f;

    /* Autonomic collapse (0-20 points) */
    if (rmssd < FLOOD_RMSSD_CRITICAL)              score += 20.0f;
    else if (rmssd < FLOOD_RMSSD_HIGH)        score += 10.0f;

    /* Map score to risk level */
    if (score >= FLOOD_CRITICAL_SCORE) {
        *advisory = AD_FLOOD_CRIT;
        return RISK_CRITICAL;
    } else if (score >= FLOOD_HIGH_SCORE) {
        *advisory = AD_FLOOD_HIGH;
        return RISK_HIGH;
    } else if (score >= FLOOD_MODERATE_SCORE) {
        *advisory = AD_FLOOD_MOD;
        return RISK_MODERATE;
    } else {
        *advisory = AD_FLOOD_NORM;
        return RISK_NORMAL;
    }
}

/* Master Assessment — Fuses All Disaster Engines */

void disaster_assess(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
) {
    float rmssd;
    /* Local safe copies of environmental inputs (use conservative defaults when missing) */
    float ambient_temp_c = 25.0f;
    float humidity_pct   = 50.0f;
    float pm25           = 0.0f;
    float skin_temp_c    = 0.0f;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    /* Use healthy default RMSSD if HRV data is unavailable or NaN */
    rmssd = (hrv != NULL && !isnan(hrv->rmssd)) ? hrv->rmssd : DEFAULT_RMSSD;

    /* Safely copy environment readings or use defaults when env is NULL or contains NaNs */
    if (env != NULL) {
        if (!isnan(env->ambient_temp_c)) ambient_temp_c = env->ambient_temp_c;
        if (!isnan(env->humidity_pct))   humidity_pct   = env->humidity_pct;
        if (!isnan(env->pm25))           pm25           = env->pm25;
        if (!isnan(env->skin_temp_c))    skin_temp_c    = env->skin_temp_c;
    }

    /* Run each disaster-specific assessment engine */
    result->heat_risk = assess_heat_risk(
        bpm, rmssd, ambient_temp_c, humidity_pct,
        &result->heat_advisory
    );

    result->pollution_risk = assess_pollution_risk(
        bpm, spo2, pm25, rmssd,
        &result->pollution_advisory
    );

    result->flood_risk = assess_flood_risk(
        bpm, skin_temp_c, rmssd,
        &result->flood_advisory
    );

    /* Overall risk = worst of all individual assessments */
    result->overall_risk     = result->heat_risk;
    result->overall_advisory = result->heat_advisory;

    if (result->pollution_risk > result->overall_risk) {
        result->overall_risk     = result->pollution_risk;
        result->overall_advisory = result->pollution_advisory;
    }
    if (result->flood_risk > result->overall_risk) {
        result->overall_risk     = result->flood_risk;
        result->overall_advisory = result->flood_advisory;
    }

    /* Positive affirmation when all clear */
    if (result->overall_risk == RISK_NORMAL) {
        result->overall_advisory = "All vitals and environmental conditions normal";
    }
}

#ifdef INCLUDE_NN
/* Neural Network Risk Assessment */
static risk_level_t nn_score_to_risk(float score, const char **advisory,
                                     const char *crit_msg, const char *high_msg,
                                     const char *mod_msg, const char *norm_msg) {
    if (score >= 0.70f) {
        *advisory = crit_msg;
        return RISK_CRITICAL;
    } else if (score >= 0.50f) {
        *advisory = high_msg;
        return RISK_HIGH;
    } else if (score >= 0.25f) {
        *advisory = mod_msg;
        return RISK_MODERATE;
    } else {
        *advisory = norm_msg;
        return RISK_NORMAL;
    }
}

void disaster_assess_nn(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
) {
    float rmssd;
    nn_output_t nn_out;
    const nn_model_t *model;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    rmssd = (hrv != NULL) ? hrv->rmssd : DEFAULT_RMSSD;

    /* Run neural network forward pass */
    model = nn_get_default_model();
    nn_predict(model, bpm, rmssd, spo2,
               env->ambient_temp_c, env->humidity_pct, env->pm25,
               &nn_out);

    /* Map heat risk output neuron to risk level */
    result->heat_risk = nn_score_to_risk(
        nn_out.heat_score, &result->heat_advisory,
        "DANGER: Neural network detects heat stroke pattern! Seek cooling NOW",
        "WARNING: AI detects heat exhaustion risk. Move to shade, hydrate",
        "CAUTION: AI detects moderate thermal strain. Stay hydrated",
        "Thermal status normal (AI)"
    );

    /* Map pollution risk output neuron to risk level */
    result->pollution_risk = nn_score_to_risk(
        nn_out.pollution_score, &result->pollution_advisory,
        "DANGER: AI detects severe respiratory distress pattern! Use N95 mask",
        "WARNING: AI detects respiratory strain. Wear mask, limit exposure",
        "CAUTION: AI detects mild pollution impact. Consider wearing a mask",
        "Respiratory status normal (AI)"
    );

    /* Map flood/cold risk output neuron to risk level */
    result->flood_risk = nn_score_to_risk(
        nn_out.flood_score, &result->flood_advisory,
        "DANGER: AI detects hypothermia/collapse pattern! Seek warmth NOW",
        "WARNING: AI detects cold exposure stress. Dry off, seek shelter",
        "CAUTION: AI detects mild exposure risk. Monitor body temperature",
        "Exposure status normal (AI)"
    );

    /* Overall risk = worst of all neural network predictions */
    result->overall_risk     = result->heat_risk;
    result->overall_advisory = result->heat_advisory;

    if (result->pollution_risk > result->overall_risk) {
        result->overall_risk     = result->pollution_risk;
        result->overall_advisory = result->pollution_advisory;
    }
    if (result->flood_risk > result->overall_risk) {
        result->overall_risk     = result->flood_risk;
        result->overall_advisory = result->flood_advisory;
    }

    if (result->overall_risk == RISK_NORMAL) {
        result->overall_advisory = "All vitals normal — AI risk engine clear";
    }
}
#else
/* Lightweight stub when NN model is not linked into the build: fall back to rule-based assess */
void disaster_assess_nn(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
) {
    /* Reuse rule-based assessment when NN isn't available */
    disaster_assess(hrv, spo2, bpm, env, result);
}
#endif
