/*
 * disaster_risk_engine.c — Multi-Disaster Health Risk Assessment Implementation
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * Scoring methodology:
 *   Each disaster engine computes a composite score (0-100) from weighted
 *   physiological + environmental sub-scores. The score maps to risk levels:
 *     0-29  → NORMAL
 *     30-49 → MODERATE
 *     50-69 → HIGH
 *     70+   → CRITICAL
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "disaster_risk_engine.h"

/* ================================================================
 *  Forward Declarations
 * ================================================================ */
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

/* ================================================================
 *  Utility Functions
 * ================================================================ */

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

/* ================================================================
 *  Heat Wave — Cardio-Thermal Strain Index (CTSI)
 * ================================================================
 *
 *  Biological basis:
 *    In extreme heat, cutaneous vasodilation diverts blood to the skin
 *    for cooling. To maintain cardiac output, heart rate rises
 *    ("cardiovascular drift") while HRV drops (sympathetic dominance).
 *    This precedes heat exhaustion → heat stroke by 15-30 minutes.
 *
 *  Inputs scored:
 *    1. Heat Index (temperature + humidity interaction)
 *    2. Resting heart rate elevation
 *    3. HRV depression (low RMSSD = high sympathetic activation)
 */
static risk_level_t assess_heat_risk(
    float bpm, float rmssd, float ambient_temp_c, float humidity_pct,
    const char **advisory
) {
    /* Simplified Steadman Heat Index approximation */
    float heat_index = ambient_temp_c;
    float ctsi = 0.0f;

    if (ambient_temp_c > 27.0f && humidity_pct > 40.0f) {
        heat_index = ambient_temp_c + 0.5f * (humidity_pct - 40.0f) * 0.1f;
    }

    /* Temperature component (0-40 points) */
    if (heat_index > 54.0f)      ctsi += 40.0f;
    else if (heat_index > 45.0f) ctsi += 30.0f;
    else if (heat_index > 40.0f) ctsi += 20.0f;
    else if (heat_index > 35.0f) ctsi += 10.0f;

    /* Cardiovascular drift component (0-30 points) */
    if (bpm > 130.0f)       ctsi += 30.0f;
    else if (bpm > 110.0f)  ctsi += 20.0f;
    else if (bpm > 95.0f)   ctsi += 10.0f;

    /* HRV depression component (0-30 points) */
    if (rmssd < 10.0f)       ctsi += 30.0f;
    else if (rmssd < 20.0f)  ctsi += 20.0f;
    else if (rmssd < 35.0f)  ctsi += 10.0f;

    /* Map score to risk level */
    if (ctsi >= 70.0f) {
        *advisory = "DANGER: Heat stroke imminent! Seek cooling, hydrate NOW";
        return RISK_CRITICAL;
    } else if (ctsi >= 50.0f) {
        *advisory = "WARNING: Heat exhaustion risk. Move to shade, drink water";
        return RISK_HIGH;
    } else if (ctsi >= 30.0f) {
        *advisory = "CAUTION: Moderate heat strain. Stay hydrated, reduce exertion";
        return RISK_MODERATE;
    } else {
        *advisory = "Thermal status normal";
        return RISK_NORMAL;
    }
}

/* ================================================================
 *  Air Pollution — Pollution Respiratory Strain Index (PRSI)
 * ================================================================
 *
 *  Biological basis:
 *    High PM2.5 causes acute airway inflammation and alveolar gas
 *    exchange impairment. SpO2 drops as respiratory compensation
 *    fails, while heart rate rises to maintain oxygen delivery.
 *
 *  Inputs scored:
 *    1. PM2.5 concentration (AQI proxy)
 *    2. SpO2 desaturation
 *    3. Compensatory tachycardia
 *    4. Autonomic stress response (HRV)
 */
static risk_level_t assess_pollution_risk(
    float bpm, float spo2, float pm25, float rmssd,
    const char **advisory
) {
    float prsi = 0.0f;

    /* Air quality component (0-40 points) */
    if (pm25 > 300.0f)       prsi += 40.0f;
    else if (pm25 > 150.0f)  prsi += 30.0f;
    else if (pm25 > 75.0f)   prsi += 20.0f;
    else if (pm25 > 35.0f)   prsi += 10.0f;

    /* Oxygen desaturation component (0-40 points) */
    if (spo2 < 88.0f)        prsi += 40.0f;
    else if (spo2 < 92.0f)   prsi += 30.0f;
    else if (spo2 < 94.0f)   prsi += 20.0f;
    else if (spo2 < 96.0f)   prsi += 10.0f;

    /* Respiratory compensation — elevated pulse (0-15 points) */
    if (bpm > 120.0f)        prsi += 15.0f;
    else if (bpm > 100.0f)   prsi += 8.0f;

    /* Autonomic stress response (0-10 points) */
    if (rmssd < 15.0f)       prsi += 10.0f;
    else if (rmssd < 25.0f)  prsi += 5.0f;

    /* Map score to risk level */
    if (prsi >= 70.0f) {
        *advisory = "DANGER: Severe respiratory distress! Use N95 mask, seek clean air";
        return RISK_CRITICAL;
    } else if (prsi >= 50.0f) {
        *advisory = "WARNING: Respiratory strain. Wear mask, minimize outdoor exposure";
        return RISK_HIGH;
    } else if (prsi >= 30.0f) {
        *advisory = "CAUTION: Air quality affecting health. Consider wearing a mask";
        return RISK_MODERATE;
    } else {
        *advisory = "Respiratory status normal";
        return RISK_NORMAL;
    }
}

/* ================================================================
 *  Flood / Hypothermia / Extreme Exertion
 * ================================================================
 *
 *  Biological basis:
 *    Immersion in cold floodwater triggers rapid cutaneous cooling,
 *    peripheral vasoconstriction, and eventually cardiac arrhythmia.
 *    Heavy physical exertion during rescue causes extreme tachycardia
 *    with autonomic collapse (very low HRV).
 *
 *  Inputs scored:
 *    1. Skin temperature drop (hypothermia indicator)
 *    2. Bradycardia (cold) or extreme tachycardia (exertion)
 *    3. Autonomic collapse (very low RMSSD)
 */
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
    if (skin_temp_c < 28.0f)       score += 40.0f;
    else if (skin_temp_c < 32.0f)  score += 25.0f;
    else if (skin_temp_c < 34.0f)  score += 10.0f;

    /* Cardiac stress: bradycardia in hypothermia OR extreme tachycardia (0-30 points) */
    if (bpm < 50.0f)               score += 30.0f;
    else if (bpm > 150.0f)         score += 30.0f;
    else if (bpm > 130.0f)         score += 15.0f;

    /* Autonomic collapse (0-20 points) */
    if (rmssd < 8.0f)              score += 20.0f;
    else if (rmssd < 15.0f)        score += 10.0f;

    /* Map score to risk level */
    if (score >= 60.0f) {
        *advisory = "DANGER: Hypothermia/collapse risk! Seek warmth immediately";
        return RISK_CRITICAL;
    } else if (score >= 40.0f) {
        *advisory = "WARNING: Cold exposure stress. Dry off and seek shelter";
        return RISK_HIGH;
    } else if (score >= 20.0f) {
        *advisory = "CAUTION: Monitor body temperature closely";
        return RISK_MODERATE;
    } else {
        *advisory = "Exposure status normal";
        return RISK_NORMAL;
    }
}

/* ================================================================
 *  Master Assessment — Fuses All Disaster Engines
 * ================================================================ */

void disaster_assess(
    const hrv_state_t   *hrv,
    float                spo2,
    float                bpm,
    const env_sensors_t *env,
    risk_assessment_t   *result
) {
    float rmssd;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    /* Use healthy default RMSSD if HRV data is unavailable */
    rmssd = (hrv != NULL) ? hrv->rmssd : 50.0f;

    /* Run each disaster-specific assessment engine */
    result->heat_risk = assess_heat_risk(
        bpm, rmssd, env->ambient_temp_c, env->humidity_pct,
        &result->heat_advisory
    );

    result->pollution_risk = assess_pollution_risk(
        bpm, spo2, env->pm25, rmssd,
        &result->pollution_advisory
    );

    result->flood_risk = assess_flood_risk(
        bpm, env->skin_temp_c, rmssd,
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

/* ================================================================
 *  Neural Network Risk Assessment
 * ================================================================
 *
 *  Uses the TinyML feedforward neural network (6→12→3) to predict
 *  disaster risk from sensor features. The NN captures non-linear
 *  inter-parameter correlations that threshold-based scoring misses.
 *
 *  Sigmoid output mapping:
 *    [0.0, 0.25) → NORMAL     (safe operating region)
 *    [0.25, 0.5) → MODERATE   (early warning)
 *    [0.5, 0.7)  → HIGH       (intervention needed)
 *    [0.7, 1.0]  → CRITICAL   (emergency response)
 */
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

    rmssd = (hrv != NULL) ? hrv->rmssd : 50.0f;

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

