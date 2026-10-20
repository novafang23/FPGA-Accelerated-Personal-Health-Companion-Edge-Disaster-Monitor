/*
 * clinical_vitals_engine.c
 * Clinical Physiological Triage Engine Implementation
 * Synthesized with DeepSeek-V4-Pro for Project SIH26181 (VALOR)
 */

#include "clinical_vitals_engine.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* ---------------------------------------------------------------------
 * Internal fixed thresholds (NEWS2 parameter bands, RCP 2017)
 *
 * The parameter bands below are the RCP NEWS2 (2017) values; SpO2 uses
 * Scale 1. The AGGREGATE level is the project's modified mNEWS2: it lowers the
 * high-risk threshold to a score of 4 and adds single-parameter and
 * absolute-crisis overrides for safety. The published score (out->news2_score)
 * is the standard NEWS2 sum; the level is mNEWS2.
 * ------------------------------------------------------------------- */
enum {
    /* NEWS2 HR score boundaries */
    HR_NEWS2_SCORE3_MAX  = 40,      /* <= 40 -> 3 */
    HR_NEWS2_SCORE1_MAX  = 50,      /* 41-50 -> 1 */
    HR_NEWS2_SCORE0_MAX  = 90,      /* 51-90 -> 0 */
    HR_NEWS2_SCORE1B_MAX = 110,     /* 91-110 -> 1 */
    HR_NEWS2_SCORE2_MAX  = 130,     /* 111-130 -> 2 */

    /* NEWS2 SpO2 score boundaries (Scale 1) */
    SPO2_NEWS2_SCORE0_MIN = 96,     /* >= 96 -> 0 */
    SPO2_NEWS2_SCORE1_MIN = 94,     /* 94-95 -> 1 */
    SPO2_NEWS2_SCORE2_MIN = 92,     /* 92-93 -> 2 */

    /* Absolute crisis thresholds */
    HR_CRITICAL_TACHY_MIN     = 150,
    HR_CRITICAL_BRADY_MAX     = 39,
    SPO2_CRITICAL_HYPOXIA_MAX = 85,

    /* RMSSD fixed-point threshold (80 = 8.0 ms autonomic collapse) */
    RMSSD_SHOCK_THRESHOLD_X10 = 80
};

static void append_condition(char *buf, size_t buf_size, size_t *len,
                             const char *fmt, float v) {
    if (buf == NULL || len == NULL || buf_size == 0) return;
    if (*len >= buf_size - 1) return;

    if (*len > 0) {
        if (*len + 3 >= buf_size) {
            *len = buf_size - 1;
            buf[*len] = '\0';
            return;
        }
        memcpy(buf + *len, " + ", 3);
        *len += 3;
    }

    size_t avail = buf_size - *len;
    int ret = snprintf(buf + *len, avail, fmt, (double)v);
    if (ret < 0) {
        buf[*len] = '\0';
        return;
    }
    if ((size_t)ret >= avail) {
        *len = buf_size - 1;
    } else {
        *len += (size_t)ret;
    }
    buf[*len] = '\0';
}

static uint8_t news2_hr_score(int hr) {
    if (hr <= HR_NEWS2_SCORE3_MAX)  return 3;   /* <= 40 */
    if (hr <= HR_NEWS2_SCORE1_MAX)  return 1;   /* 41-50 */
    if (hr <= HR_NEWS2_SCORE0_MAX)  return 0;   /* 51-90 */
    if (hr <= HR_NEWS2_SCORE1B_MAX) return 1;   /* 91-110 */
    if (hr <= HR_NEWS2_SCORE2_MAX)  return 2;   /* 111-130 */
    return 3;                                   /* >= 131 */
}

static uint8_t news2_spo2_score(int spo2) {
    if (spo2 >= SPO2_NEWS2_SCORE0_MIN) return 0;   /* >= 96 */
    if (spo2 >= SPO2_NEWS2_SCORE1_MIN) return 1;   /* 94-95 */
    if (spo2 >= SPO2_NEWS2_SCORE2_MIN) return 2;   /* 92-93 */
    return 3;                                      /* <= 91 */
}

static uint8_t news2_rr_score(int rr) {
    if (rr <= 8)  return 3;  /* Severe bradypnea */
    if (rr <= 11) return 1;  /* Mild bradypnea */
    if (rr <= 20) return 0;  /* Normal breathing (12-20) */
    if (rr <= 24) return 2;  /* Moderate tachypnea */
    return 3;                /* Severe tachypnea (>= 25) */
}

static uint8_t news2_temp_score(float temp) {
    if (temp <= 0.0f || !isfinite(temp)) return 0; /* Unmeasured / uninstrumented */
    if (temp <= 35.0f) return 3;                    /* Hypothermia (<= 35.0 C) */
    if (temp <= 36.0f) return 1;                    /* Mild cold stress (35.1 - 36.0 C) */
    if (temp <= 38.0f) return 0;                    /* Normal body temperature (36.1 - 38.0 C) */
    if (temp <= 39.0f) return 1;                    /* Pyrexia / mild fever (38.1 - 39.0 C) */
    return 2;                                       /* High fever (>= 39.1 C) */
}

void clinical_vitals_assess_ext(float hr, float spo2, float rmssd, float rr, float sqi, float temp_c, clinical_assessment_t *out) {
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    out->level = CLINICAL_NORMAL;

    /* 1. Input validation & NaN/Inf guard */
    if (!isfinite(hr) || !isfinite(spo2) ||
        !(hr > 0.0f && hr < 350.0f && spo2 > 0.0f && spo2 <= 100.0f)) {
        out->level = CLINICAL_ELEVATED;
        /* Mark the hold the same way the two SQI hold paths below do, so a
         * consumer can tell "holding, nothing measured" from a real clinical
         * elevation. Verified on hardware 2026-10-20: with no finger this path
         * ran and published LEVEL=1 with FLAGS=0x00, which the fusion then
         * mapped to RISK=MODERATE for a patient who was never measured. */
        out->alert_flags = ALERT_SIGNAL_NOISE;
        snprintf(out->advisory, sizeof(out->advisory), "Clinical vitals: sensor calibrating or invalid");
        return;
    }

    /* 2. Compute canonical integer vitals (consistent rounding across all branches) */
    int hr_i   = (int)(hr + 0.5f);
    int spo2_i = (int)(spo2 + 0.5f);
    /* NEWS2 needs a respiratory term even when RR is unavailable, so the
     * scoring path uses the population default. The OUTPUT must not: 14 is a
     * claim that the patient was breathing 14/min, and the dashboard would
     * publish that invented measurement. 0.0f is the established "absent"
     * sentinel in clinical_assessment_t. */
    const bool rr_valid = (rr > 0.0f && isfinite(rr));
    int rr_i = rr_valid ? (int)(rr + 0.5f) : 14;
    int32_t rmssd_x10 = -1;
    if (isfinite(rmssd) && rmssd > 0.0f && rmssd < 500.0f) {
        rmssd_x10 = (int32_t)(rmssd * 10.0f + 0.5f);
    }

    out->hr        = hr;
    out->spo2      = spo2;
    out->rmssd     = (rmssd > 0.0f && isfinite(rmssd)) ? rmssd : 0.0f;
    out->rr        = rr_valid ? (float)rr_i : 0.0f;
    out->sqi       = (sqi > 0.0f && isfinite(sqi)) ? sqi : 0.0f;
    out->body_temp = (temp_c > 0.0f && isfinite(temp_c)) ? temp_c : 0.0f;

    /* 3. Evaluate absolute life-threatening crisis using the exact same canonical integers.
     * Count the criteria rather than collapsing to one bool: on a poor signal a single
     * derived vital can be an optical artifact, so the override below needs corroboration. */
    int crisis_criteria = 0;
    if (spo2_i <= SPO2_CRITICAL_HYPOXIA_MAX)              crisis_criteria++; /* severe hypoxia   */
    if (hr_i >= HR_CRITICAL_TACHY_MIN)                    crisis_criteria++; /* extreme tachy    */
    if (hr_i <= HR_CRITICAL_BRADY_MAX)                    crisis_criteria++; /* extreme brady    */
    if (rr_i >= 30)                                       crisis_criteria++; /* extreme tachypnea*/
    if (rr_i <= 6)                                        crisis_criteria++; /* extreme bradypnea*/
    if (out->body_temp > 0.0f && out->body_temp <= 30.0f) crisis_criteria++; /* deep hypothermia */

    /* 4. Motion artifact rejection & Warm-up check (Elgendi 2016 / Karlen 2012):
     * If signal quality is zero (sensor disconnected or uncalibrated warmup),
     * hold triage unconditionally to prevent noise from hallucinating emergency alarms.
     * If signal quality is low (0.0 < SQI < 0.70) due to motion, hold triage unless the
     * crisis is CORROBORATED by signal quality or a second independent vital.
     *
     * The original override trusted a single crisis vital even at SQI 0.47-0.52. Live
     * testing (2026-10-17) showed exactly how that fails: an artifact-biased red/IR ratio
     * produced a stable-but-wrong SpO2 of ~80%, which alone (SpO2 <= 85) bypassed the hold
     * and latched a CRITICAL/SOS alarm on a healthy subject. A genuine collapse almost
     * always presents more than one crisis sign (e.g. hypoxia + extreme tachycardia), so
     * requiring two criteria on a poor signal suppresses the single-vital artifact while
     * still alarming on a real emergency. With a clean signal (SQI >= 0.70) one vital is
     * still trusted, because the quality gate exists to filter artifact, not to delay
     * an unambiguous reading. */
    bool crisis_corroborated = (out->sqi >= 0.70f) || (crisis_criteria >= 2);
    if (out->sqi == 0.0f) {
        out->level = CLINICAL_ELEVATED;
        out->alert_flags = ALERT_SIGNAL_NOISE;
        snprintf(out->advisory, sizeof(out->advisory),
                 "SENSOR WARMUP: Calibrating signal baseline, holding triage.");
        return;
    } else if (out->sqi < 0.70f && !crisis_corroborated) {
        out->level = CLINICAL_ELEVATED; /* Mark as elevated uncertainty / holding */
        out->alert_flags = ALERT_SIGNAL_NOISE;
        snprintf(out->advisory, sizeof(out->advisory),
                 "SENSOR QUALITY LOW (SQI %.2f < 0.70): Motion artifact detected. Stabilize sensor, holding triage.", (double)out->sqi);
        return;
    }

    uint8_t hr_score   = news2_hr_score(hr_i);
    uint8_t spo2_score = news2_spo2_score(spo2_i);
    uint8_t rr_score   = news2_rr_score(rr_i);
    uint8_t temp_score = news2_temp_score(out->body_temp);
    uint8_t score      = hr_score + spo2_score + rr_score + temp_score;
    out->news2_score   = score;
    out->news2_hr_pts   = hr_score;
    out->news2_spo2_pts = spo2_score;
    out->news2_rr_pts   = rr_score;
    out->news2_temp_pts = temp_score;

    uint8_t flags = ALERT_NONE;
    bool shock_present = false;

    if (spo2_i <= 91) flags |= ALERT_HYPOXIA;
    if (hr_i >= 111)  flags |= ALERT_TACHYCARDIA;
    if (hr_i <= 40)   flags |= ALERT_BRADYCARDIA;
    if (rr_i >= 25)   flags |= ALERT_TACHYPNEA;
    if (rr_i <= 8)    flags |= ALERT_BRADYPNEA;
    if (out->body_temp > 0.0f && out->body_temp <= 35.0f) flags |= ALERT_HYPOTHERMIA;
    if (rmssd_x10 >= 0 && rmssd_x10 < RMSSD_SHOCK_THRESHOLD_X10 && (hr_i >= 100 || spo2_i <= 94 || rr_i >= 24)) {
        flags |= ALERT_AUTONOMIC_SHOCK;
        shock_present = true;
    }
    if (sqi > 0.0f && sqi < 0.70f) {
        flags |= ALERT_SIGNAL_NOISE;
    }
    out->alert_flags = flags;

    bool critical_vital = (spo2_i <= SPO2_CRITICAL_HYPOXIA_MAX) ||
                          (hr_i >= HR_CRITICAL_TACHY_MIN) ||
                          (hr_i <= HR_CRITICAL_BRADY_MAX) ||
                          (rr_i >= 30) || (rr_i <= 6) ||
                          (out->body_temp > 0.0f && out->body_temp <= 30.0f);

    if (critical_vital || score >= 7 || (shock_present && score >= 5)) {
        out->level = CLINICAL_CRITICAL;
    } else if (score >= 4 || spo2_i <= 91 || hr_i >= 131 || hr_i <= 40 || rr_i >= 25 || shock_present || temp_score >= 2) {
        out->level = CLINICAL_HIGH;
    } else if (score >= 1) {
        out->level = CLINICAL_ELEVATED;
    } else {
        out->level = CLINICAL_NORMAL;
    }

    char conditions[128] = {0};
    size_t cond_len = 0;

    if (flags & ALERT_HYPOXIA) {
        const char *fmt = (spo2_i <= SPO2_CRITICAL_HYPOXIA_MAX)
                          ? "Severe Hypoxia (SpO2 %.0f%%)"
                          : "Hypoxia (SpO2 %.0f%%)";
        append_condition(conditions, sizeof(conditions), &cond_len, fmt, (float)spo2_i);
    }
    if (flags & ALERT_TACHYCARDIA) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Tachycardia (HR %.0f bpm)", (float)hr_i);
    }
    if (flags & ALERT_BRADYCARDIA) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Bradycardia (HR %.0f bpm)", (float)hr_i);
    }
    if (flags & ALERT_TACHYPNEA) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Tachypnea (RR %.0f bpm)", (float)rr_i);
    }
    if (flags & ALERT_BRADYPNEA) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Bradypnea (RR %.0f bpm)", (float)rr_i);
    }
    if (flags & ALERT_HYPOTHERMIA) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Hypothermia (Temp %.1f C)", out->body_temp);
    }
    if (out->body_temp >= 39.1f) {
        append_condition(conditions, sizeof(conditions), &cond_len, "High Fever (Temp %.1f C)", out->body_temp);
    }
    if (flags & ALERT_AUTONOMIC_SHOCK) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Autonomic/Shock (RMSSD %.1f ms)", rmssd);
    }
    if (flags & ALERT_SIGNAL_NOISE) {
        append_condition(conditions, sizeof(conditions), &cond_len, "Motion Artifact (SQI %.2f)", sqi);
    }

    const char *severity = "";
    const char *action = "";

    switch (out->level) {
        case CLINICAL_NORMAL:
            action = "Vitals within normal range";
            break;
        case CLINICAL_ELEVATED:
            severity = "ELEVATED: ";
            action = "Mild deviation. Monitor patient.";
            break;
        case CLINICAL_HIGH:
            severity = "HIGH: ";
            action = "Physiological strain - clinical evaluation required.";
            break;
        case CLINICAL_CRITICAL:
            severity = "CRITICAL: ";
            action = "Immediate medical intervention required!";
            break;
    }

    if (cond_len > 0) {
        snprintf(out->advisory, sizeof(out->advisory), "%s%s (mNEWS2=%u). %s", severity, conditions, score, action);
    } else if (out->level != CLINICAL_NORMAL) {
        snprintf(out->advisory, sizeof(out->advisory), "%smNEWS2 score %u. %s", severity, score, action);
    } else {
        snprintf(out->advisory, sizeof(out->advisory), "%s", action);
    }
}

void clinical_vitals_assess_full(float hr, float spo2, float rmssd, float rr, float sqi, clinical_assessment_t *out) {
    clinical_vitals_assess_ext(hr, spo2, rmssd, rr, sqi, 0.0f, out);
}

void clinical_vitals_assess(float hr, float spo2, float rmssd, clinical_assessment_t *out) {
    clinical_vitals_assess_full(hr, spo2, rmssd, 14.0f, 0.95f, out);
}

/* clinical_fuse_triage output relies on risk_assessment_t's internal buffer */

void clinical_fuse_triage(const clinical_assessment_t *clinical,
                          const risk_assessment_t *disaster,
                          risk_assessment_t *fused) {
    if (fused == NULL) return;

    risk_level_t disaster_level = RISK_NORMAL;
    const char *disaster_adv = "No environmental hazard";
    if (disaster != NULL) {
        *fused = *disaster;
        disaster_level = disaster->overall_risk;
        if (disaster->overall_advisory[0] != '\0') disaster_adv = disaster->overall_advisory;
    } else {
        memset(fused, 0, sizeof(*fused));
    }

    if (clinical == NULL) return;

    /* Map clinical level to disaster risk_level_t */
    risk_level_t clin_level = RISK_NORMAL;
    switch (clinical->level) {
        case CLINICAL_NORMAL:   clin_level = RISK_NORMAL;   break;
        case CLINICAL_ELEVATED: clin_level = RISK_MODERATE; break;
        case CLINICAL_HIGH:     clin_level = RISK_HIGH;     break;
        case CLINICAL_CRITICAL: clin_level = RISK_CRITICAL; break;
    }

    /* A "hold" is not a finding. The three hold paths in
     * clinical_vitals_assess_ext() (invalid input, SQI == 0 warm-up, SQI < 0.70
     * without corroboration) set ALERT_SIGNAL_NOISE and mark the level
     * ELEVATED so the panel can say "holding triage". ELEVATED maps to MODERATE
     * above, so before this check a device with nobody on the sensor published
     * RISK=MODERATE (hardware, 2026-10-20: NEWS2=0, LEVEL=1, FLAGS=0x00,
     * RISK=MODERATE). Nothing was measured, so nothing may be graded: the
     * clinical side contributes no level, a genuine environmental hazard still
     * stands on its own, and when neither is present the verdict is UNKNOWN
     * rather than a NORMAL that implies a patient was assessed. */
    const bool clinical_holding = (clinical->alert_flags & ALERT_SIGNAL_NOISE) != 0;
    if (clinical_holding) clin_level = RISK_NORMAL;

    if (clinical_holding && disaster_level <= RISK_NORMAL) {
        fused->overall_risk = RISK_UNKNOWN;
        snprintf(fused->overall_advisory, sizeof(fused->overall_advisory),
                 "Patient not assessed: %.200s | Environment: %s",
                 clinical->advisory,
                 (disaster_level == RISK_UNKNOWN) ? "not assessed" : "no hazard detected");
        return;
    }

    /* Maximum risk wins - but an UNKNOWN environmental assessment must not be
     * laundered into an all-clear just because the clinical side is normal.
     * RISK_UNKNOWN is -1, so the plain `clin_level > disaster_level` test turned
     * (clinical NORMAL, environmental UNKNOWN) into NORMAL and dropped the
     * unknown advisory. A measured critical vital still overrides, because that
     * is a finding rather than an absent one. */
    if (disaster_level == RISK_UNKNOWN) {
        if (clin_level > RISK_NORMAL) {
            fused->overall_risk = clin_level;
            snprintf(fused->overall_advisory, sizeof(fused->overall_advisory),
                     "%.240s | Environmental: not assessed", clinical->advisory);
        } else {
            fused->overall_risk = RISK_UNKNOWN;
            snprintf(fused->overall_advisory, sizeof(fused->overall_advisory),
                     "Environmental risk not assessed; %.240s", disaster_adv);
        }
    } else if (clin_level > disaster_level) {
        fused->overall_risk = clin_level;
        if (disaster_level > RISK_NORMAL) {
            snprintf(fused->overall_advisory, sizeof(fused->overall_advisory), "%.240s | Environmental: %.240s",
                     clinical->advisory, disaster_adv);
        } else {
            snprintf(fused->overall_advisory, sizeof(fused->overall_advisory), "%s", clinical->advisory);
        }
    } else {
        fused->overall_risk = disaster_level;
        if (clin_level > RISK_NORMAL) {
            snprintf(fused->overall_advisory, sizeof(fused->overall_advisory), "%.240s | Vitals: %.240s",
                     disaster_adv, clinical->advisory);
        } else {
            snprintf(fused->overall_advisory, sizeof(fused->overall_advisory), "%s", disaster_adv);
        }
    }
}
