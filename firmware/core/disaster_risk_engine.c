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
#include <math.h>


/* Forward Declarations */
static risk_level_t assess_heat_risk(float bpm, float rmssd, float ambient_temp_c,
                                     float humidity_pct, const char **advisory);

static risk_level_t assess_pollution_risk(float bpm, float spo2, float pm25, float rmssd,
                                          const char **advisory);

static risk_level_t assess_skin_hypothermia(float bpm, float skin_temp_c, float rmssd,
                                            const char **advisory);

static risk_level_t assess_cold_stress_proxy(float bpm, const env_sensors_t *env, float rmssd,
                                             const char **advisory);

static risk_level_t assess_flood_risk(float bpm, const env_sensors_t *env, float rmssd,
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

static float calculate_nws_heat_index(float temp_c, float hum_pct) {
    if (temp_c < 27.0f) return temp_c; /* Below 80F, heat index == ambient */

    float tf = temp_c * 1.8f + 32.0f;
    float rh = hum_pct;
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    /* NOAA / NWS Rothfusz regression equation */
    float hi_f = -42.379f + 2.04901523f * tf + 10.14333127f * rh
               - 0.22475541f * tf * rh - 0.00683783f * tf * tf
               - 0.05481717f * rh * rh + 0.00122874f * tf * tf * rh
               + 0.00085282f * tf * rh * rh - 0.00000199f * tf * tf * rh * rh;

    /* NWS adjustments to the Rothfusz regression. Without these the formula is
     * wrong in very dry (<13% RH) and very humid (>85% RH) air. The current
     * 28-30 C / 66-67% conditions fall outside both, but the card must be
     * correct in the edge bands too. */
    if (rh < 13.0f && tf >= 80.0f && tf <= 112.0f) {
        hi_f -= ((13.0f - rh) / 4.0f) * sqrtf((17.0f - fabsf(tf - 95.0f)) / 17.0f);
    } else if (rh > 85.0f && tf >= 80.0f && tf <= 87.0f) {
        hi_f += ((rh - 85.0f) / 10.0f) * ((87.0f - tf) / 5.0f);
    }

    float hi_c = (hi_f - 32.0f) / 1.8f;
    /* Clamp to the published table range before anything displays this value.
     * The regression diverges outside its fitted domain (it returns 106 C at
     * 46.5 C / 68 % RH). Triage is unaffected -- see HEAT_INDEX_MAX_C. */
    if (hi_c > HEAT_INDEX_MAX_C) hi_c = HEAT_INDEX_MAX_C;
    /* Return the NWS heat index itself. It CAN be below the air temperature in
     * very dry air (the NWS table gives ~87 F at 95 F / 10 % RH). Flooring it at
     * the ambient, as this used to, silently discarded the low-humidity
     * adjustment and overstated the heat category in dry heat. */
    return hi_c;
}

/* -------------------------------------------------------------------------
 * Literature-based environmental condition
 * ------------------------------------------------------------------------- */

float disaster_calculate_heat_index_c(float temp_c, float humidity_pct) {
    return calculate_nws_heat_index(temp_c, humidity_pct);
}

int disaster_epa_pm25_aqi(float pm25_ugm3) {
    /* A negative PM2.5 means "no reading" (see env_sensors_t). It must not be
     * mapped onto the lowest AQI band, which would report the air as "Good"
     * on the strength of a measurement that does not exist. -1 is the
     * "not assessed" marker; the pages and dashboards render it as such. */
    if (pm25_ugm3 < 0.0f) return -1;
    if (!(pm25_ugm3 > 0.0f)) return 0;
    /* US EPA AQI technical assistance document, PM2.5 breakpoints effective
     * 2024-05-06 (the 2024 revision lowered the Good/Moderate boundary to
     * 9.0 ug/m3). Above 325.4 ug/m3 the AQI is capped at 500. */
    static const float c_lo[6] = {0.0f,   9.1f,   35.5f,  55.5f,  125.5f, 225.5f};
    static const float c_hi[6] = {9.0f,   35.4f,  55.4f,  125.4f, 225.4f, 325.4f};
    static const int   i_lo[6] = {0,      51,     101,    151,    201,    301};
    static const int   i_hi[6] = {50,     100,    150,    200,    300,    500};

    for (int i = 0; i < 6; i++) {
        if (pm25_ugm3 <= c_hi[i]) {
            float c = pm25_ugm3;
            if (c < c_lo[i]) c = c_lo[i];
            float aqi = ((float)(i_hi[i] - i_lo[i]) / (c_hi[i] - c_lo[i])) *
                            (c - c_lo[i]) + (float)i_lo[i];
            int a = (int)(aqi + 0.5f);
            if (a < 0) a = 0;
            if (a > 500) a = 500;
            return a;
        }
    }
    return 500;
}

const char *disaster_epa_pm25_category(int aqi) {
    if (aqi < 0)    return "Not assessed";
    if (aqi <= 50)  return "Good";
    if (aqi <= 100) return "Moderate";
    if (aqi <= 150) return "Unhealthy for Sensitive Groups";
    if (aqi <= 200) return "Unhealthy";
    if (aqi <= 300) return "Very Unhealthy";
    return "Hazardous";
}

const char *disaster_nws_heat_category(float heat_index_c) {
    /* NWS heat-index categories, converted from the published Fahrenheit
     * bands: 80/90/103/125 F = 26.7/32.2/39.4/51.7 C. */
    if (heat_index_c < 26.7f) return "Normal";
    if (heat_index_c < 32.2f) return "Caution";
    if (heat_index_c < 39.4f) return "Extreme Caution";
    if (heat_index_c < 51.7f) return "Danger";
    return "Extreme Danger";
}

void disaster_environment_condition_text(const env_sensors_t *env,
                                         char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!env) return;

    int aqi = disaster_epa_pm25_aqi(env->pm25);
    const char *pcat = disaster_epa_pm25_category(aqi);
    float hi = disaster_calculate_heat_index_c(env->ambient_temp_c, env->humidity_pct);
    const char *hcat = disaster_nws_heat_category(hi);
    const float who_24h = 15.0f;

    /* No PM2.5 reading: say so. Do not report the air as good, and do not
     * report a WHO multiple for a measurement that was never taken. */
    if (env->pm25 < 0.0f) {
        snprintf(out, cap,
                 "PM2.5 not assessed (sensor unavailable). "
                 "Heat index %.1f C: %s (NWS).", hi, hcat);
        return;
    }

    float who_x = (who_24h > 0.0f) ? env->pm25 / who_24h : 0.0f;

    snprintf(out, cap,
             "PM2.5 %.1f ug/m3: %s (EPA AQI %d, 2024). "
             "WHO 2021 24-h guideline 15 ug/m3 %s (%.1fx). "
             "Heat index %.1f C: %s (NWS).",
             env->pm25, pcat, aqi,
             (who_x > 1.0f) ? "exceeded" : "met", who_x,
             hi, hcat);
}

/* Heat Wave — Cardio-Thermal Strain Index (CTSI) */
static risk_level_t assess_heat_risk(float bpm, float rmssd, float ambient_temp_c,
                                     float humidity_pct, const char **advisory) {
    if (ambient_temp_c < -40.0f || ambient_temp_c > 85.0f ||
        humidity_pct < 0.0f || humidity_pct > 100.0f) {
        if (advisory) *advisory = "Heat risk unavailable: ambient sensor has no valid reading";
        return RISK_UNKNOWN;
    }
    /* Guard: heat-related illness is physiologically impossible in cold
     * environments. Without this check, a low RMSSD from hypothermia
     * (or any other non-heat cause) incorrectly inflates the CTSI
     * score, producing a false "thermal strain" advisory when the
     * actual problem is cold exposure. */
    if (ambient_temp_c < HEAT_TEMP_BASE_C) {
        if (advisory) *advisory = "Thermal status normal";
        return RISK_NORMAL;
    }

    /* NOAA / NWS heat index (Rothfusz regression). */
    float heat_index = calculate_nws_heat_index(ambient_temp_c, humidity_pct);
    float ctsi = 0.0f;

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

    /* HRV depression component (0-30 points). rmssd == 0 means "no valid HRV",
     * not "maximally suppressed". */
    if (rmssd > 0.0f) {
        if (rmssd < HEAT_RMSSD_CRITICAL)
            ctsi += 30.0f;
        else if (rmssd < HEAT_RMSSD_HIGH)
            ctsi += 20.0f;
        else if (rmssd < HEAT_RMSSD_MODERATE)
            ctsi += 10.0f;
    }

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

    /* No PM2.5 reading at all (sensor absent, unreadable, or no fresh sample
     * inside the rolling window). The pollution modality is then NOT
     * ASSESSED - it is not "normal". A missing measurement must never be
     * reported as clean air. */
    if (pm25 < 0.0f) {
        *advisory = "PM2.5 sensor unavailable - air quality not assessed";
        return RISK_UNKNOWN;
    }

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

    /* Autonomic stress response (0-10 points). rmssd == 0 means "no valid
     * HRV", not "maximal suppression". */
    if (rmssd > 0.0f) {
        if (rmssd < POLLUTION_RMSSD_CRIT)
            prsi += 10.0f;
        else if (rmssd < POLLUTION_RMSSD_HIGH)
            prsi += 5.0f;
    }

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
/* Clinical path: a real skin temperature is available, so use the measured
 * skin-temperature thresholds directly. */
static risk_level_t assess_skin_hypothermia(float bpm, float skin_temp_c, float rmssd,
                                            const char **advisory) {
    float score = 0.0f;

    /* Hypothermia indicators (0-40 points) */
    if (skin_temp_c < FLOOD_SKIN_TEMP_CRIT)
        score += 40.0f;
    else if (skin_temp_c < FLOOD_SKIN_TEMP_HIGH)
        score += 25.0f;
    else if (skin_temp_c < FLOOD_SKIN_TEMP_MOD)
        score += 10.0f;

    /* Cardiac stress: bradycardia in hypothermia OR extreme tachycardia (0-30 points).
     * bpm == 0 means "no valid vitals", not bradycardia. */
    if (bpm > 0.0f) {
        if (bpm < FLOOD_BPM_BRADYCARDIA)
            score += 30.0f;
        else if (bpm > FLOOD_BPM_TACHY_EXTREME)
            score += 30.0f;
        else if (bpm > FLOOD_BPM_TACHY_MOD)
            score += 15.0f;
    }

    /* Autonomic collapse (0-20 points). rmssd == 0 means "no valid HRV". */
    if (rmssd > 0.0f) {
        if (rmssd < FLOOD_RMSSD_CRITICAL)
            score += 20.0f;
        else if (rmssd < FLOOD_RMSSD_HIGH)
            score += 10.0f;
    }

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

/* Ambient-proxy path: no skin-temperature sensor, so estimate COLD-STRESS
 * EXPOSURE from ambient air + humidity + the same cardiac/autonomic terms.
 * See the rationale and citations in disaster_risk_engine.h.
 *
 * Deliberately capped at RISK_HIGH - a CRITICAL hypothermia call requires a
 * measured skin or core temperature, which this sensor set cannot provide. */
static risk_level_t assess_cold_stress_proxy(float bpm, const env_sensors_t *env, float rmssd,
                                             const char **advisory) {
    float score = 0.0f;
    float t  = env->ambient_temp_c;
    float rh = env->humidity_pct;

    /* Air temperature (0-32 points) */
    if (t < COLD_AMBIENT_SEVERE_C)
        score += 32.0f;
    else if (t < COLD_AMBIENT_HIGH_C)
        score += 28.0f;
    else if (t < COLD_AMBIENT_MOD_C)
        score += 20.0f;
    else if (t < COLD_AMBIENT_MILD_C)
        score += 10.0f;
    else if (t < COLD_AMBIENT_COOL_C)
        score += 5.0f;

    /* Humidity (0-13 points). Wet cold removes heat far faster than dry cold.
     * Gated on the temperature band: 80 % RH at 48 C is not a cold stressor,
     * and without this gate a hot humid day scored as cold exposure. */
    if (t < COLD_AMBIENT_COOL_C) {
        if (rh >= COLD_HUMIDITY_VHIGH_PCT)
            score += 13.0f;
        else if (rh >= COLD_HUMIDITY_HIGH_PCT)
            score += 8.0f;
        else if (rh >= COLD_HUMIDITY_MOD_PCT)
            score += 4.0f;
    }

    /* Cardiac stress (0-30 points). Cold exposure is U-shaped: shivering
     * thermogenesis drives early tachycardia, while deep hypothermia drives
     * bradycardia. Both ends are warning signs.
     * bpm == 0 means "no valid vitals", not bradycardia: without this guard a
     * missing finger scored +30 here and +20 below and produced a false flood
     * warning on a warm day. */
    if (bpm > 0.0f) {
        if (bpm < FLOOD_BPM_BRADYCARDIA)
            score += 30.0f;
        else if (bpm > FLOOD_BPM_TACHY_EXTREME)
            score += 30.0f;
        else if (bpm > FLOOD_BPM_TACHY_MOD)
            score += 15.0f;
    }

    /* Autonomic strain (0-20 points): cold reduces parasympathetic tone.
     * rmssd == 0 means "no valid HRV", not maximal suppression. */
    if (rmssd > 0.0f) {
        if (rmssd < FLOOD_RMSSD_CRITICAL)
            score += 20.0f;
        else if (rmssd < FLOOD_RMSSD_HIGH)
            score += 10.0f;
    }

    if (score >= FLOOD_SCORE_CRITICAL) {
        *advisory = "DANGER: severe cold-stress exposure (ambient estimate). Dry, insulate and shelter the patient now";
        return RISK_HIGH;   /* capped - see function comment */
    } else if (score >= FLOOD_SCORE_HIGH) {
        *advisory = "WARNING: cold-stress exposure (ambient estimate). Wet clothing is the main risk - dry off and shelter";
        return RISK_HIGH;
    } else if (score >= FLOOD_SCORE_MODERATE) {
        *advisory = "CAUTION: cold-stress exposure (ambient estimate). Keep the patient dry and monitor";
        return RISK_MODERATE;
    }
    *advisory = "Cold-stress exposure low for current ambient conditions (air temperature estimate)";
    return RISK_NORMAL;
}

/* Dispatch on what the sensor set can actually support. */
static risk_level_t assess_flood_risk(float bpm, const env_sensors_t *env, float rmssd,
                                      const char **advisory) {
    /* Preferred: a measured skin temperature. */
    if (env->skin_temp_c > 0.0f) {
        return assess_skin_hypothermia(bpm, env->skin_temp_c, rmssd, advisory);
    }

    /* Fallback: ambient air is usable (BME280 present and reporting). */
    if (env->ambient_temp_c > COLD_AMBIENT_VALID_MIN_C &&
        env->ambient_temp_c < COLD_AMBIENT_VALID_MAX_C) {
        return assess_cold_stress_proxy(bpm, env, rmssd, advisory);
    }

    /* Neither: report the blind spot rather than guessing. */
    *advisory = "No skin-temperature sensor and no usable ambient reading";
    return RISK_UNKNOWN;
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
    risk_level_t levels[4];
    const char *advisories[4];
    risk_level_t worst = RISK_NORMAL;
    const char *worst_advisory = NULL;
    int have_hazard = 0;
    int have_unknown = 0;
    int have_normal = 0;
    unsigned int i;

    levels[0] = result->heat_risk;
    levels[1] = result->pollution_risk;
    levels[2] = result->flood_risk;
    levels[3] = result->cyclone_risk;

    advisories[0] = result->heat_advisory;
    advisories[1] = result->pollution_advisory;
    advisories[2] = result->flood_advisory;
    advisories[3] = result->cyclone_advisory;

    for (i = 0; i < 4; ++i) {
        if (levels[i] == RISK_UNKNOWN) {
            /* Tally UNKNOWN only for the three CORE modalities. The cyclone
             * trend (index 3) is an additional hazard, so its UNKNOWN means
             * "no pressure history yet" rather than "a sensor is missing".
             * Counting it would make every device report "other sensors
             * unmonitored or missing" for the first half hour after boot, and
             * would flip an all-normal verdict to UNKNOWN whenever the
             * barometer had not been watched long enough. */
            if (i < 3) have_unknown = 1;
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
        snprintf(result->overall_advisory, sizeof(result->overall_advisory), "%s", worst_advisory);
    } else if (have_unknown) {
        result->overall_risk = RISK_UNKNOWN;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory), "%s", have_normal
            ? "Active modalities normal; other sensors unmonitored or missing"
            : "Insufficient sensor data; all evaluated modalities unknown");
    } else {
        result->overall_risk = RISK_NORMAL;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory), "%s", normal_advisory ? normal_advisory : "All vitals and environmental conditions normal");
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

    /* The cyclone/storm hazard comes from a pressure TREND, which these
     * functions do not compute, so it must be marked unavailable EXPLICITLY.
     * A memset leaves it at zero, and zero is RISK_NORMAL - an all-clear
     * reported for a hazard nobody evaluated, which is the exact failure this
     * codebase keeps having to remove. assess_cyclone_risk() overwrites this
     * when the caller has a trend worth judging. */
    result->cyclone_risk = RISK_UNKNOWN;
    result->cyclone_advisory = "Storm risk not evaluated: no pressure trend";

    /* A valid env pointer is required. HRV is NOT: the environmental hazards do
     * not depend on the patient, and a missing finger must not hide a heat,
     * pollution or flood warning (observed on hardware: the phone showed
     * UNKNOWN with valid BME280/PMS readings whenever the finger was off).
     * The physiological terms are treated as unavailable: rmssd = 0, and
     * bpm/spo2 arrive as 0 when there is no contact. The sub-assessors guard
     * rmssd > 0 so an absent HRV is not scored as a collapsed one. */
    if (env == NULL) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory), "Environmental sensor unavailable");
        return;
    }

    const bool vitals_valid = (hrv != NULL && hrv_is_ready(hrv));
    rmssd = vitals_valid ? hrv->rmssd : 0.0f;

    /* Run each disaster-specific assessment engine */
    result->heat_risk = assess_heat_risk(bpm, rmssd, env->ambient_temp_c, env->humidity_pct,
                                         &result->heat_advisory);

    result->pollution_risk =
        assess_pollution_risk(bpm, spo2, env->pm25, rmssd, &result->pollution_advisory);

    result->flood_risk = assess_flood_risk(bpm, env, rmssd, &result->flood_advisory);

    /* Overall risk triage via robust multi-modality aggregation */
    finalize_overall_risk(result, "All vitals and environmental conditions normal");
}

/* Neural Network Risk Assessment */

/* Every feature the network sees must be a real number.
 *
 * The network's arithmetic has no way to represent "missing": a non-finite
 * input makes every layer-1 pre-activation non-finite, the ReLU passes it
 * through, the uint8 requantizer clamps it to 0, and the network then returns
 * a finite, plausible-looking score derived only from the biases. Measured on
 * the real kernel, a NaN humidity produced heat=0.008 / pollution=0.191 /
 * flood=0.034 - a number that looks like a measurement but is an artefact of
 * the bias vector.
 *
 * The device path already filters non-finite environment values before it
 * gets here (ambient_valid() range-checks temperature and humidity, and the
 * PM2.5 path is an integer conversion), so this guard is defence in depth:
 * a sensor or a future caller that passes NaN gets an honest UNKNOWN instead
 * of a fabricated score. */
static bool nn_features_finite(float bpm, float spo2, const env_sensors_t *env) {
    if (!isfinite(bpm) || !isfinite(spo2)) return false;
    if (!isfinite(env->ambient_temp_c) || !isfinite(env->humidity_pct) ||
        !isfinite(env->pm25)) {
        return false;
    }
    return true;
}

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

/* The float32 model is the REFERENCE implementation. The device runs the INT8
 * model (disaster_assess_nn_int8 below); disaster_assess_nn() has no caller in
 * the ESP32-S3 firmware, so compiling it there would put the 2.5 kB of float32
 * weights (nn_risk_model_trained.c.inc) and the duplicate inference path into
 * the shipped image for nothing. The Zynq, host-test and Win32 builds keep it:
 * they use it as the reference the INT8 output is compared against.
 *
 * Default is ON so every existing build is unaffected unless it opts out. */
#ifndef NN_ENABLE_FLOAT32_MODEL
#define NN_ENABLE_FLOAT32_MODEL 1
#endif

#if NN_ENABLE_FLOAT32_MODEL
void disaster_assess_nn(const hrv_state_t *hrv, float spo2, float bpm, const env_sensors_t *env,
                        risk_assessment_t *result) {
    float rmssd;
    nn_output_t nn_out;
    const nn_model_t *model;

    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(risk_assessment_t));

    /* The cyclone/storm hazard comes from a pressure TREND, which these
     * functions do not compute, so it must be marked unavailable EXPLICITLY.
     * A memset leaves it at zero, and zero is RISK_NORMAL - an all-clear
     * reported for a hazard nobody evaluated, which is the exact failure this
     * codebase keeps having to remove. assess_cyclone_risk() overwrites this
     * when the caller has a trend worth judging. */
    result->cyclone_risk = RISK_UNKNOWN;
    result->cyclone_advisory = "Storm risk not evaluated: no pressure trend";

    /* Require valid HRV data, a valid env pointer, AND a published RMSSD.
     *
     * hrv_is_ready() only says the window holds enough contiguous pairs to
     * compute a value; it does NOT say a value was published. The SQI and
     * settling gates in main_shrikefi.c zero g_state.hrv_rmssd while the
     * accepted-beat count keeps climbing, so an unpublished RMSSD arrives
     * here as 0.0f. Zero is not a physiological RMSSD, and
     * normalize() maps it to the most-suppressed end of the feature
     * (NN_RMSSD_MIN is 1.0f), so the network would read "no HRV measured" as
     * "HRV collapsed" and could raise a heat/pollution/cold alarm on a
     * measurement that does not exist. SpO2 gets a neutral baseline below for
     * the same class of reason; for RMSSD the honest answer is to decline to
     * score and let the caller show UNKNOWN rather than to substitute a
     * value. */
    if (hrv == NULL || env == NULL || !hrv_is_ready(hrv) || !(hrv->rmssd > 0.0f)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory), "Insufficient HRV data for AI risk assessment");
        return;
    }

    if (!nn_features_finite(bpm, spo2, env)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory),
                 "Non-finite sensor value: AI risk assessment declined");
        return;
    }

    rmssd = hrv->rmssd;

    /* PM2.5: a negative value means "no reading at all" (see env_sensors_t).
     * The network gets a neutral clean-air value so its arithmetic stays
     * finite, and the pollution channel is then marked NOT ASSESSED below.
     * Substituting a clean-air value into a risk channel would turn a missing
     * measurement into an all-clear. */
    const bool  pm_valid = (env->pm25 >= 0.0f);
    const float nn_pm25  = pm_valid ? env->pm25 : 0.0f;

    /* If SpO2 is not yet locked/calibrated (<= 0.0f), use neutral 96.0% baseline
     * so the neural network does not falsely interpret 0.0f as severe asphyxia/hypoxemia */
    float nn_spo2 = (spo2 > 0.0f) ? spo2 : 96.0f;

    /* Run neural network forward pass */
    model = nn_get_default_model();
    nn_predict(model, bpm, rmssd, nn_spo2, env->ambient_temp_c, env->humidity_pct, nn_pm25,
               &nn_out);

    /* Map heat risk output neuron to risk level */
    result->heat_risk = nn_score_to_risk(
        nn_out.heat_score, &result->heat_advisory,
        "DANGER: Neural network detects heat stroke pattern! Seek cooling NOW",
        "WARNING: AI detects heat exhaustion risk. Move to shade, hydrate",
        "CAUTION: AI detects moderate thermal strain. Stay hydrated", "Thermal status normal (AI)");

    /* Map pollution risk output neuron to risk level */
    if (pm_valid) {
        result->pollution_risk =
            nn_score_to_risk(nn_out.pollution_score, &result->pollution_advisory,
                             "DANGER: AI detects severe respiratory distress pattern! Use N95 mask",
                             "WARNING: AI detects respiratory strain. Wear mask, limit exposure",
                             "CAUTION: AI detects mild pollution impact. Consider wearing a mask",
                             "Respiratory status normal (AI)");
    } else {
        result->pollution_risk = RISK_UNKNOWN;
        result->pollution_advisory =
            "PM2.5 sensor unavailable - AI pollution channel not assessed";
    }

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
#endif /* NN_ENABLE_FLOAT32_MODEL */

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

    /* The cyclone/storm hazard comes from a pressure TREND, which these
     * functions do not compute, so it must be marked unavailable EXPLICITLY.
     * A memset leaves it at zero, and zero is RISK_NORMAL - an all-clear
     * reported for a hazard nobody evaluated, which is the exact failure this
     * codebase keeps having to remove. assess_cyclone_risk() overwrites this
     * when the caller has a trend worth judging. */
    result->cyclone_risk = RISK_UNKNOWN;
    result->cyclone_advisory = "Storm risk not evaluated: no pressure trend";

    /* Require valid HRV data, a valid env pointer, AND a published RMSSD.
     * Same reasoning as disaster_assess_nn(): 0.0f means "no HRV published",
     * not "HRV collapsed", and the INT8 path clamps the resulting negative
     * normalized input to 0.0f - the most-suppressed end of the feature. A
     * missing measurement must not become a risk score. */
    if (hrv == NULL || env == NULL || !hrv_is_ready(hrv) || !(hrv->rmssd > 0.0f)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory), "Insufficient HRV data for AI risk assessment (INT8)");
        if (raw_out != NULL) {
            memset(raw_out, 0, sizeof(nn_output_t));
        }
        return;
    }

    if (!nn_features_finite(bpm, spo2, env)) {
        result->heat_risk = RISK_UNKNOWN;
        result->pollution_risk = RISK_UNKNOWN;
        result->flood_risk = RISK_UNKNOWN;
        result->overall_risk = RISK_UNKNOWN;
        snprintf(result->overall_advisory, sizeof(result->overall_advisory),
                 "Non-finite sensor value: AI risk assessment declined (INT8)");
        if (raw_out != NULL) {
            memset(raw_out, 0, sizeof(nn_output_t));
        }
        return;
    }

    rmssd = hrv->rmssd;

    /* PM2.5: a negative value means "no reading at all" (see env_sensors_t).
     * The network gets a neutral clean-air value so its arithmetic stays
     * finite, and the pollution channel is then marked NOT ASSESSED below.
     * Substituting a clean-air value into a risk channel would turn a missing
     * measurement into an all-clear. */
    const bool  pm_valid = (env->pm25 >= 0.0f);
    const float nn_pm25  = pm_valid ? env->pm25 : 0.0f;

    /* If SpO2 is not yet locked/calibrated (<= 0.0f), use neutral 96.0% baseline
     * so the neural network does not falsely interpret 0.0f as severe asphyxia/hypoxemia */
    float nn_spo2 = (spo2 > 0.0f) ? spo2 : 96.0f;

    /* Run INT8 neural network forward pass */
    nn_predict_int8(&nn_default_model_int8, &nn_quant_params, bpm, rmssd, nn_spo2,
                    env->ambient_temp_c, env->humidity_pct, nn_pm25, &nn_out);

    /* Copy raw activations if caller requested them for telemetry/logging.
     * A negative pollution score is the wire marker for "this channel was not
     * assessed" (no PM2.5 reading). */
    if (raw_out != NULL) {
        *raw_out = nn_out;
        if (!pm_valid) {
            raw_out->pollution_score = -1.0f;
        }
    }

    /* Map heat risk output neuron to risk level */
    result->heat_risk = nn_score_to_risk(
        nn_out.heat_score, &result->heat_advisory,
        "DANGER: Neural network detects heat stroke pattern! Seek cooling NOW",
        "WARNING: AI detects heat exhaustion risk. Move to shade, hydrate",
        "CAUTION: AI detects moderate thermal strain. Stay hydrated", "Thermal status normal (AI)");

    /* Map pollution risk output neuron to risk level */
    if (pm_valid) {
        result->pollution_risk =
            nn_score_to_risk(nn_out.pollution_score, &result->pollution_advisory,
                              "DANGER: AI detects severe respiratory distress pattern! Use N95 mask",
                              "WARNING: AI detects respiratory strain. Wear mask, limit exposure",
                              "CAUTION: AI detects mild pollution impact. Consider wearing a mask",
                              "Respiratory status normal (AI)");
    } else {
        result->pollution_risk = RISK_UNKNOWN;
        result->pollution_advisory =
            "PM2.5 sensor unavailable - AI pollution channel not assessed";
    }

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

/* Clinical biomarker models. The Moran PSI formula is published; the AHA and
 * cold-stress indicators below are project heuristics, not published formulas. */

/* Moran et al. (1998) PSI: combines cardiovascular strain and thermal load.
 * Returns scale [0.0, 10.0]. The PSI formula itself is Moran's; the core
 * temperature it needs is NOT measured, so it is estimated with a simple
 * heuristic below. Buller et al. (2013) is cited only for the HR-based
 * estimation idea - their published method is a Kalman filter, not this linear
 * model. */
float disaster_calculate_moran_psi(float bpm, float ambient_temp_c, float humidity_pct) {
    if (bpm <= 30.0f) return 0.0f;

    /* Heuristic core-temperature estimate (NOT the Buller 2013 model). This
     * heat_index is also a crude ambient+humidity proxy, not the NWS Rothfusz
     * heat index used by the condition card. */
    float heat_index = ambient_temp_c;
    if (humidity_pct > 40.0f) {
        heat_index = ambient_temp_c + 0.5f * (humidity_pct - 40.0f) * 0.1f;
    }

    float t_core_est = 37.0f;
    if (bpm > 75.0f) {
        t_core_est += 0.015f * (bpm - 75.0f);
    }
    if (heat_index > 28.0f) {
        t_core_est += 0.025f * (heat_index - 28.0f);
    }
    if (t_core_est > 41.5f) t_core_est = 41.5f;
    if (t_core_est < 36.5f) t_core_est = 36.5f;

    /* Moran's exact PSI formula:
     * PSI = 5 * (T_core - T_core_0)/(39.5 - T_core_0) + 5 * (HR - HR_0)/(180 - HR_0)
     * Baseline T_core_0 = 36.5 C, HR_0 = 70 bpm */
    float t_strain = 5.0f * (t_core_est - 36.5f) / (39.5f - 36.5f);
    float hr_strain = 5.0f * (bpm - 70.0f) / (180.0f - 70.0f);

    if (t_strain < 0.0f) t_strain = 0.0f;
    if (hr_strain < 0.0f) hr_strain = 0.0f;

    float psi = t_strain + hr_strain;
    if (psi > 10.0f) psi = 10.0f;
    if (psi < 0.0f)  psi = 0.0f;
    return psi;
}

/* Heuristic autonomic-strain index MOTIVATED by the American Heart Association
 * statement (Brook et al., Circulation 2010): PM2.5 is associated with acute
 * autonomic dysfunction. The AHA statement does not define this formula; the
 * weights below are the project's. Returns [0.0, 1.0]. */
float disaster_calculate_aha_autonomic_strain(float pm25, float rmssd) {
    if (pm25 <= 15.0f || rmssd <= 0.0f) return 0.0f;

    /* Baseline healthy RMSSD ~ 45-50 ms. When PM2.5 > 35 ug/m3 and RMSSD < 20 ms,
     * autonomic strain increases sharply towards 1.0 */
    float pm_factor = (pm25 - 15.0f) / 135.0f; /* 15 -> 0, 150 -> 1.0 */
    if (pm_factor > 1.0f) pm_factor = 1.0f;

    float hrv_suppression = 0.0f;
    if (rmssd < 35.0f) {
        hrv_suppression = (35.0f - rmssd) / 30.0f;
        if (hrv_suppression > 1.0f) hrv_suppression = 1.0f;
    }

    float strain = pm_factor * 0.50f + hrv_suppression * 0.50f;
    if (pm25 > 35.0f && rmssd < 20.0f) {
        strain += 0.25f; /* Acute AHA autonomic depression trigger */
    }

    if (strain > 1.0f) strain = 1.0f;
    if (strain < 0.0f) strain = 0.0f;
    return strain;
}

/* Barometric pressure trend -> cyclone / storm risk (T3.1).
 *
 * Reads the RATE OF FALL, not the absolute pressure: sea-level pressure varies
 * by tens of hPa with altitude and season, so a fixed threshold would fire in
 * the hills and stay silent on the coast. Thresholds and their meteorological
 * basis are in disaster_risk_engine.h.
 *
 * A trend that is not yet usable yields RISK_UNKNOWN, never RISK_NORMAL. The
 * distinction matters: RISK_NORMAL is a statement that the barometer is steady,
 * and claiming that before the window is long enough to know would be inventing
 * an observation. */
void assess_cyclone_risk(const pressure_trend_result_t *trend,
                         risk_level_t *risk, const char **advisory) {
    if (!risk || !advisory) return;

    if (!trend || !trend->valid) {
        *risk = RISK_UNKNOWN;
        *advisory = "Storm risk not evaluated: pressure trend window too short";
        return;
    }

    float slope = trend->slope_hpa_per_hr;
    float drop  = trend->drop_hpa;

    /* Require BOTH a rate and an absolute fall. The rate alone can be reached by
     * a fraction of a hPa of sensor drift across a short window, and the drop
     * alone says nothing about how fast it happened. */
    bool falling_enough = (drop >= CYCLONE_MIN_DROP_HPA);

    if (falling_enough && slope <= CYCLONE_FALL_CRITICAL_HPA_PER_HR) {
        *risk = RISK_CRITICAL;
        *advisory = "Rapidly falling barometer - cyclone precursor. Secure the "
                    "site and move to shelter inland; do not wait for a warning.";
    } else if (falling_enough && slope <= CYCLONE_FALL_HIGH_HPA_PER_HR) {
        *risk = RISK_HIGH;
        *advisory = "Barometer falling fast - storm developing. Expect high wind "
                    "and heavy rain; secure loose equipment and seek shelter.";
    } else if (falling_enough && slope <= CYCLONE_FALL_MODERATE_HPA_PER_HR) {
        *risk = RISK_MODERATE;
        *advisory = "Barometer falling - a low is developing. Wind and rain likely "
                    "within hours; plan for deteriorating weather.";
    } else {
        *risk = RISK_NORMAL;
        *advisory = "Barometric pressure steady - no storm signature in the trend.";
    }
}

