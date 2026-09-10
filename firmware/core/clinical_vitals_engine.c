/*
 * clinical_vitals_engine.c
 * NEWS2-Derived Clinical Physiological Triage Engine Implementation
 * Synthesized with DeepSeek-V4-Pro for Project SIH26181 (EdgeGuard)
 */

#include "clinical_vitals_engine.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* ---------------------------------------------------------------------
 * Internal fixed thresholds (NEWS2 standard)
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

void clinical_vitals_assess_full(float hr, float spo2, float rmssd, float rr, float sqi, clinical_assessment_t *out) {
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    out->level = CLINICAL_NORMAL;

    /* 1. Input validation & NaN/Inf guard */
    if (!isfinite(hr) || !isfinite(spo2) ||
        !(hr > 0.0f && hr < 350.0f && spo2 > 0.0f && spo2 <= 100.0f)) {
        snprintf(out->advisory, sizeof(out->advisory), "Clinical vitals: sensor calibrating or invalid");
        return;
    }

    /* 2. Compute canonical integer vitals (consistent rounding across all branches) */
    int hr_i   = (int)(hr + 0.5f);
    int spo2_i = (int)(spo2 + 0.5f);
    int rr_i   = (rr > 0.0f && isfinite(rr)) ? (int)(rr + 0.5f) : 14; /* default 14 if unavailable */
    int32_t rmssd_x10 = -1;
    if (isfinite(rmssd) && rmssd > 0.0f && rmssd < 500.0f) {
        rmssd_x10 = (int32_t)(rmssd * 10.0f + 0.5f);
    }

    out->hr    = hr;
    out->spo2  = spo2;
    out->rmssd = (rmssd > 0.0f && isfinite(rmssd)) ? rmssd : 0.0f;
    out->rr    = (float)rr_i;
    out->sqi   = (sqi > 0.0f && isfinite(sqi)) ? sqi : 0.95f;

    /* 3. Evaluate absolute life-threatening crisis using the exact same canonical integers */
    bool is_absolute_crisis = (spo2_i <= SPO2_CRITICAL_HYPOXIA_MAX) ||
                              (hr_i >= HR_CRITICAL_TACHY_MIN) ||
                              (hr_i <= HR_CRITICAL_BRADY_MAX) ||
                              (rr_i >= 30) || (rr_i <= 6);

    /* 4. Motion artifact rejection check via SQI (Elgendi 2016 / Karlen 2012):
     * If signal quality is low BUT patient is NOT in life-threatening collapse,
     * hold previous reliable reading to prevent false alarms.
     * BUT if an absolute crisis is detected, DO NOT suppress the emergency! */
    if (sqi > 0.0f && sqi < 0.70f && !is_absolute_crisis) {
        out->level = CLINICAL_ELEVATED; /* Mark as elevated uncertainty / holding */
        out->alert_flags = ALERT_SIGNAL_NOISE;
        snprintf(out->advisory, sizeof(out->advisory),
                 "SENSOR QUALITY LOW (SQI %.2f < 0.70): Motion artifact detected. Stabilize sensor, holding triage.", sqi);
        return;
    }

    uint8_t hr_score   = news2_hr_score(hr_i);
    uint8_t spo2_score = news2_spo2_score(spo2_i);
    uint8_t rr_score   = news2_rr_score(rr_i);
    uint8_t score      = hr_score + spo2_score + rr_score;
    out->news2_score   = score;

    uint8_t flags = ALERT_NONE;
    bool shock_present = false;

    if (spo2_i <= 91) flags |= ALERT_HYPOXIA;
    if (hr_i >= 111)  flags |= ALERT_TACHYCARDIA;
    if (hr_i <= 40)   flags |= ALERT_BRADYCARDIA;
    if (rr_i >= 25)   flags |= ALERT_TACHYPNEA;
    if (rr_i <= 8)    flags |= ALERT_BRADYPNEA;
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
                          (rr_i >= 30) || (rr_i <= 6);

    if (critical_vital || score >= 7 || (shock_present && score >= 5)) {
        out->level = CLINICAL_CRITICAL;
    } else if (score >= 4 || spo2_i <= 91 || hr_i >= 131 || hr_i <= 40 || rr_i >= 25 || shock_present) {
        out->level = CLINICAL_HIGH;
    } else if (score >= 1) {
        out->level = CLINICAL_ELEVATED;
    } else {
        out->level = CLINICAL_NORMAL;
    }

    char conditions[192] = {0};
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

void clinical_vitals_assess(float hr, float spo2, float rmssd, clinical_assessment_t *out) {
    clinical_vitals_assess_full(hr, spo2, rmssd, 14.0f, 0.95f, out);
}

/* Static buffer for unified advisory string */
static char s_fused_advisory_buf[512];

void clinical_fuse_triage(const clinical_assessment_t *clinical,
                          const risk_assessment_t *disaster,
                          risk_assessment_t *fused) {
    if (fused == NULL) return;

    risk_level_t disaster_level = RISK_NORMAL;
    const char *disaster_adv = "No environmental hazard";
    if (disaster != NULL) {
        *fused = *disaster;
        disaster_level = disaster->overall_risk;
        if (disaster->overall_advisory) disaster_adv = disaster->overall_advisory;
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

    /* Maximum risk wins */
    if (clin_level > disaster_level) {
        fused->overall_risk = clin_level;
        if (disaster_level > RISK_NORMAL) {
            snprintf(s_fused_advisory_buf, sizeof(s_fused_advisory_buf), "%s | Environmental: %s",
                     clinical->advisory, disaster_adv);
        } else {
            snprintf(s_fused_advisory_buf, sizeof(s_fused_advisory_buf), "%s", clinical->advisory);
        }
    } else {
        fused->overall_risk = disaster_level;
        if (clin_level > RISK_NORMAL) {
            snprintf(s_fused_advisory_buf, sizeof(s_fused_advisory_buf), "%s | Vitals: %s",
                     disaster_adv, clinical->advisory);
        } else {
            snprintf(s_fused_advisory_buf, sizeof(s_fused_advisory_buf), "%s", disaster_adv);
        }
    }

    fused->overall_advisory = s_fused_advisory_buf;
}
