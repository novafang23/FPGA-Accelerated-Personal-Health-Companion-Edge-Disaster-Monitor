/*
 * disaster_risk_engine.c
 * Multi-Disaster Health Risk Assessment Implementation
 */

#include "disaster_risk_engine.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Forward Declarations */
static risk_level_t assess_heat_risk(float bpm, float rmssd, float ambient_temp_c,
                                     float humidity_pct, const char **advisory);

static risk_level_t assess_pollution_risk(float bpm, float spo2, float pm25, float rmssd,
                                          const char **advisory);

static risk_level_t assess_flood_risk(float bpm, float skin_temp_c, float rmssd,
                                      const char **advisory);

/* Utility Functions */

const char *risk_level_to_string(risk_level_t level) {
    switch (level) {
    case RISK_UNKNOWN:
        return "UNKNOWN";
    case RISK_NORMAL:
        return "NORMAL";
    case RISK_MODERATE:
        return "MODERATE";
    case RISK_HIGH:
        return "ELEVATED";
    case RISK_CRITICAL:
        return "CRITICAL";
    default:
        return "INVALID";
    }
}

const char *risk_level_to_color(risk_level_t level) {
    switch (level) {
    case RISK_UNKNOWN:
        return "\033[90m"; /* Dark Gray */
    case RISK_NORMAL:
        return "\033[32m"; /* Green  */
    case RISK_MODERATE:
        return "\033[33m"; /* Yellow */
    case RISK_HIGH:
        return "\033[38;5;208m"; /* Orange */
    case RISK_CRITICAL:
        return "\033[31m"; /* Red    */
    default:
        return "\033[0m";
    }
}

/* Heat Wave — Cardio-Thermal Strain Index (CTSI) */
static risk_level_t assess_heat_risk(float bpm, float rmssd, float ambient_temp_c,
                                     float humidity_pct, const char **advisory) {
    /* Guard: heat-related illness is physiologically impossible in cold
     * environments. Without this check, a low RMSSD from hypothermia
     * (or any other non-heat cause) incorrectly inflates the CTSI
     * score, producing a false "thermal strain" advisory when the
     * actual problem is cold exposure. */
    if (ambient_temp_c < HEAT_TEMP_BASE_C) {
        *advisory = "Thermal status normal";
        return RISK_NORMAL;
    }

    /* Simplified Steadman Heat Index approximation */
    float heat_index = ambient_temp_c;
    float ctsi = 0.0f;

    if (humidity_pct > HEAT_HUMIDITY_BASE_PCT) {
        heat_index = ambient_temp_c + 0.5f * (humidity_pct - HEAT_HUMIDITY_BASE_PCT) * 0.1f;
    }

    /* Temperature component (0-40 points) */
    if (heat_index > HEAT_INDEX_CRITICAL)
        ctsi += 40.0f;
    else if (heat_index > HEAT_INDEX_HIGH)
        ctsi += 30.0f;
    else if (heat_index > HEAT_INDEX_MODERATE)
        ctsi += 20.0f;
    else if (heat_index > HEAT_INDEX_CAUTION)
        ctsi += 10.0f;

    /* Cardiovascular drift component (0-30 points) */
    if (bpm > HEAT_BPM_CRITICAL)
        ctsi += 30.0f;
    else if (bpm > HEAT_BPM_HIGH)
        ctsi += 20.0f;
    else if (bpm > HEAT_BPM_MODERATE)
        ctsi += 10.0f;

    /* HRV depression component (0-30 points) */
    if (rmssd < HEAT_RMSSD_CRITICAL)
        ctsi += 30.0f;
    else if (rmssd < HEAT_RMSSD_HIGH)
        ctsi += 20.0f;
    else if (rmssd < HEAT_RMSSD_MODERATE)
        ctsi += 10.0f;

    /* Map score to risk level */
    if (ctsi >= HEAT_CTSI_CRITICAL) {
        *advisory = "DANGER: Heat stroke imminent! Seek cooling, hydrate NOW";
        return RISK_CRITICAL;
    } else if (ctsi >= HEAT_CTSI_HIGH) {
        *advisory = "WARNING: Heat exhaustion risk. Move to shade, drink water";
        return RISK_HIGH;
    } else if (ctsi >= HEAT_CTSI_MODERATE) {
        *advisory = "CAUTION: Moderate heat strain. Stay hydrated, reduce exertion";
        return RISK_MODERATE;
    } else {
        *advisory = "Thermal status normal";
        return RISK_NORMAL;
    }
}

/* Air Pollution — Pollution Respiratory Strain Index (PRSI) */
static risk_level_t assess_pollution_risk(float bpm, float spo2, float pm25, float rmssd,
                                          const char **advisory) {
    float prsi = 0.0f;

    /* Guard: if the air itself is clean (PM2.5 at or below the lowest
     * caution threshold), don't let autonomic/SpO2 body-response
     * components inflate the score — those abnormal readings are
     * caused by something else (heat exhaustion, hypothermia, exertion)
     * not by air pollution. Only report pollution risk when the air
     * quality component itself is elevated. */
    if (pm25 <= POLLUTION_PM25_CAUTION) {
        *advisory = "Respiratory status normal";
        return RISK_NORMAL;
    }

    /* Air quality component (0-40 points) */
    if (pm25 > POLLUTION_PM25_CRITICAL)
        prsi += 40.0f;
    else if (pm25 > POLLUTION_PM25_HIGH)
        prsi += 30.0f;
    else if (pm25 > POLLUTION_PM25_MODERATE)
        prsi += 20.0f;
    else if (pm25 > POLLUTION_PM25_CAUTION)
        prsi += 10.0f;

    /* Oxygen desaturation component (0-40 points) */
    if (spo2 <= 0.0f) {
        /* SpO2 not yet valid or sensor unavailable — do not falsely penalize with hypoxia points */
    } else if (spo2 < POLLUTION_SPO2_CRITICAL)
        prsi += 40.0f;
    else if (spo2 < POLLUTION_SPO2_HIGH)
        prsi += 30.0f;
    else if (spo2 < POLLUTION_SPO2_MODERATE)
        prsi += 20.0f;
    else if (spo2 < POLLUTION_SPO2_CAUTION)
        prsi += 10.0f;

    /* Respiratory compensation — elevated pulse (0-15 points) */
    if (bpm > POLLUTION_BPM_CRITICAL)
        prsi += 15.0f;
    else if (bpm > POLLUTION_BPM_HIGH)
        prsi += 8.0f;

    /* Autonomic stress response (0-10 points) */
    if (rmssd < POLLUTION_RMSSD_CRIT)
        prsi += 10.0f;
    else if (rmssd < POLLUTION_RMSSD_HIGH)
        prsi += 5.0f;

    /* Map score to risk level */
    if (prsi >= POLLUTION_PRSI_CRITICAL) {
        *advisory = "DANGER: Severe respiratory distress! Use N95 mask, seek clean air";
        return RISK_CRITICAL;
    } else if (prsi >= POLLUTION_PRSI_HIGH) {
        *advisory = "WARNING: Respiratory strain. Wear mask, minimize outdoor exposure";
        return RISK_HIGH;
    } else if (prsi >= POLLUTION_PRSI_MODERATE) {
        *advisory = "CAUTION: Air quality affecting health. Consider wearing a mask";
        return RISK_MODERATE;
    } else {
        *advisory = "Respiratory status normal";
        return RISK_NORMAL;
    }
}

/* Flood / Hypothermia / Extreme Exertion */
static risk_level_t assess_flood_risk(float bpm, float skin_temp_c, float rmssd,
                                      const char **advisory) {
    float score = 0.0f;

    /* Skip if no skin temperature sensor data available */
    if (skin_temp_c <= 0.0f) {
        *advisory = "No skin temperature data available";
        return RISK_UNKNOWN;
    }

    /* Hypothermia indicators (0-40 points) */
    if (skin_temp_c < FLOOD_SKIN_TEMP_CRIT)
        score += 40.0f;
    else if (skin_temp_c < FLOOD_SKIN_TEMP_HIGH)
        score += 25.0f;
    else if (skin_temp_c < FLOOD_SKIN_TEMP_MOD)
        score += 10.0f;

    /* Cardiac stress: bradycardia in hypothermia OR extreme tachycardia (0-30 points) */
    if (bpm < FLOOD_BPM_BRADYCARDIA)
        score += 30.0f;
    else if (bpm > FLOOD_BPM_TACHY_EXTREME)
        score += 30.0f;
    else if (bpm > FLOOD_BPM_TACHY_MOD)
        score += 15.0f;

    /* Autonomic collapse (0-20 points) */
    if (rmssd < FLOOD_RMSSD_CRITICAL)
        score += 20.0f;
    else if (rmssd < FLOOD_RMSSD_HIGH)
        score += 10.0f;

    /* Map score to risk level */
    if (score >= FLOOD_SCORE_CRITICAL) {
        *advisory = "DANGER: Hypothermia/collapse risk! Seek warmth immediately";
        return RISK_CRITICAL;
    } else if (score >= FLOOD_SCORE_HIGH) {
        *advisory = "WARNING: Cold exposure stress. Dry off and seek shelter";
        return RISK_HIGH;
    } else if (score >= FLOOD_SCORE_MODERATE) {
        *advisory = "CAUTION: Monitor body temperature closely";
        return RISK_MODERATE;
    } else {
        *advisory = "Exposure status normal";
        return RISK_NORMAL;
    }
}

/*
 * Triage helper: Fuses individual modality risks into the overall risk assessment.
 * Resolves RISK_UNKNOWN triage:
 * - If ANY modality indicates a hazard (CRITICAL > HIGH > MODERATE), the worst hazard wins.
 * - If NO hazard is detected, but one or more active modalities are RISK_UNKNOWN,
 *   overall_risk is marked RISK_UNKNOWN with an informative advisory regarding unmonitored sensors.
 * - Only when all evaluated modalities are verified non-hazardous does it return RISK_NORMAL.
 */
static void finalize_overall_risk(risk_assessment_t *result, const char *normal_advisory) {
    risk_level_t levels[3];
    const char *advisories[3];
    risk_level_t worst = RISK_NORMAL;
    const char *worst_advisory = NULL;
    int have_hazard = 0;
    int have_unknown = 0;
    int have_normal = 0;
    unsigned int i;

    levels[0] = result->heat_risk;
    levels[1] = result->pollution_risk;
    levels[2] = result->flood_risk;

    advisories[0] = result->heat_advisory;
    advisories[1] = result->pollution_advisory;
    advisories[2] = result->flood_advisory;

    for (i = 0; i < 3; ++i) {
        if (levels[i] == RISK_UNKNOWN) {
            have_unknown = 1;
            continue;
        }

        if (levels[i] == RISK_NORMAL) {
            have_normal = 1;
            continue;
        }

        /* Any clinical or AI hazard overrides UNKNOWN / NORMAL */
        have_hazard = 1;
        if (levels[i] > worst) {
            worst = levels[i];
            worst_advisory = advisories[i];
        }
    }

    if (have_hazard) {
        result->overall_risk = worst;
        result->overall_advisory = worst_advisory;
    } else if (have_unknown) {
        result->overall_risk = RISK_UNKNOWN;
        result->overall_advisory = have_normal
            ? "Active modalities normal; other sensors unmonitored or missing"
            : "Insufficient sensor data; all evaluated modalities unknown";
    } else {
        result->overall_risk = RISK_NORMAL;
        result->overall_advisory = normal_advisory ? normal_advisory : "All vitals and environmental conditions normal";
    }
}

/* Master Assessment — Fuses All Disaster Engines */

void disaster_assess(const hrv_state_t *hrv, float spo2, float bpm, const env_sensors_t *env,
                     risk_assessment_t *result) {
    float rmssd;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    /* Require valid HRV data (minimum samples) and a valid env pointer */
    if (hrv == NULL || env == NULL || !hrv_is_ready(hrv)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        result->overall_advisory = "Insufficient HRV data for risk assessment";
        return;
    }

    rmssd = hrv->rmssd;

    /* Run each disaster-specific assessment engine */
    result->heat_risk = assess_heat_risk(bpm, rmssd, env->ambient_temp_c, env->humidity_pct,
                                         &result->heat_advisory);

    result->pollution_risk =
        assess_pollution_risk(bpm, spo2, env->pm25, rmssd, &result->pollution_advisory);

    result->flood_risk = assess_flood_risk(bpm, env->skin_temp_c, rmssd, &result->flood_advisory);

    /* Overall risk triage via robust multi-modality aggregation */
    finalize_overall_risk(result, "All vitals and environmental conditions normal");
}

/* Neural Network Risk Assessment */
static risk_level_t nn_score_to_risk(float score, const char **advisory, const char *crit_msg,
                                     const char *high_msg, const char *mod_msg,
                                     const char *norm_msg) {
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

void disaster_assess_nn(const hrv_state_t *hrv, float spo2, float bpm, const env_sensors_t *env,
                        risk_assessment_t *result) {
    float rmssd;
    nn_output_t nn_out;
    const nn_model_t *model;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    /* Require valid HRV data and a valid env pointer */
    if (hrv == NULL || env == NULL || !hrv_is_ready(hrv)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        result->overall_advisory = "Insufficient HRV data for AI risk assessment";
        return;
    }

    rmssd = hrv->rmssd;

    /* If SpO2 is not yet locked/calibrated (<= 0.0f), use neutral 96.0% baseline
     * so the neural network does not falsely interpret 0.0f as severe asphyxia/hypoxemia */
    float nn_spo2 = (spo2 > 0.0f) ? spo2 : 96.0f;

    /* Run neural network forward pass */
    model = nn_get_default_model();
    nn_predict(model, bpm, rmssd, nn_spo2, env->ambient_temp_c, env->humidity_pct, env->pm25,
               &nn_out);

    /* Map heat risk output neuron to risk level */
    result->heat_risk = nn_score_to_risk(
        nn_out.heat_score, &result->heat_advisory,
        "DANGER: Neural network detects heat stroke pattern! Seek cooling NOW",
        "WARNING: AI detects heat exhaustion risk. Move to shade, hydrate",
        "CAUTION: AI detects moderate thermal strain. Stay hydrated", "Thermal status normal (AI)");

    /* Map pollution risk output neuron to risk level */
    result->pollution_risk =
        nn_score_to_risk(nn_out.pollution_score, &result->pollution_advisory,
                         "DANGER: AI detects severe respiratory distress pattern! Use N95 mask",
                         "WARNING: AI detects respiratory strain. Wear mask, limit exposure",
                         "CAUTION: AI detects mild pollution impact. Consider wearing a mask",
                         "Respiratory status normal (AI)");

    /* Map flood/cold risk output neuron to risk level */
    result->flood_risk =
        nn_score_to_risk(nn_out.flood_score, &result->flood_advisory,
                         "DANGER: AI detects hypothermia/collapse pattern! Seek warmth NOW",
                         "WARNING: AI detects cold exposure stress. Dry off, seek shelter",
                         "CAUTION: AI detects mild exposure risk. Monitor body temperature",
                         "Exposure status normal (AI)");

    /* Overall risk triage via robust multi-modality aggregation */
    finalize_overall_risk(result, "All vitals normal -- AI risk engine clear");
}

/* Run AI-powered neural network disaster risk assessment (INT8 quantized). */
void disaster_assess_nn_int8(const hrv_state_t *hrv, float spo2, float bpm,
                             const env_sensors_t *env, risk_assessment_t *result,
                             nn_output_t *raw_out) {
    float rmssd;
    nn_output_t nn_out;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    /* Require valid HRV data and a valid env pointer */
    if (hrv == NULL || env == NULL || !hrv_is_ready(hrv)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        result->overall_advisory = "Insufficient HRV data for AI risk assessment (INT8)";
        if (raw_out != NULL) {
            memset(raw_out, 0, sizeof(nn_output_t));
        }
        return;
    }

    rmssd = hrv->rmssd;

    /* If SpO2 is not yet locked/calibrated (<= 0.0f), use neutral 96.0% baseline
     * so the neural network does not falsely interpret 0.0f as severe asphyxia/hypoxemia */
    float nn_spo2 = (spo2 > 0.0f) ? spo2 : 96.0f;

    /* Run INT8 neural network forward pass */
    nn_predict_int8(&nn_default_model_int8, &nn_quant_params, bpm, rmssd, nn_spo2,
                    env->ambient_temp_c, env->humidity_pct, env->pm25, &nn_out);

    /* Copy raw activations if caller requested them for telemetry/logging */
    if (raw_out != NULL) {
        *raw_out = nn_out;
    }

    /* Map heat risk output neuron to risk level */
    result->heat_risk = nn_score_to_risk(
        nn_out.heat_score, &result->heat_advisory,
        "DANGER: Neural network detects heat stroke pattern! Seek cooling NOW",
        "WARNING: AI detects heat exhaustion risk. Move to shade, hydrate",
        "CAUTION: AI detects moderate thermal strain. Stay hydrated", "Thermal status normal (AI)");

    /* Map pollution risk output neuron to risk level */
    result->pollution_risk =
        nn_score_to_risk(nn_out.pollution_score, &result->pollution_advisory,
                          "DANGER: AI detects severe respiratory distress pattern! Use N95 mask",
                          "WARNING: AI detects respiratory strain. Wear mask, limit exposure",
                          "CAUTION: AI detects mild pollution impact. Consider wearing a mask",
                          "Respiratory status normal (AI)");

    /* Map flood/cold risk output neuron to risk level */
    result->flood_risk =
        nn_score_to_risk(nn_out.flood_score, &result->flood_advisory,
                          "DANGER: AI detects hypothermia/collapse pattern! Seek warmth NOW",
                          "WARNING: AI detects cold exposure stress. Dry off, seek shelter",
                          "CAUTION: AI detects mild exposure risk. Monitor body temperature",
                          "Exposure status normal (AI)");

    /* Overall risk triage via robust multi-modality aggregation */
    finalize_overall_risk(result, "All vitals normal -- AI risk engine clear (INT8)");
}
