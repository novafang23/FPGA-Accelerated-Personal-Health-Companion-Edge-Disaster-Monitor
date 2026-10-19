/*
 * clinical_vitals_engine.h
 * NEWS2-Derived Clinical Physiological Triage Engine
 * Synthesized with DeepSeek-V4-Pro for Project SIH26181 (VALOR)
 */

#ifndef CLINICAL_VITALS_ENGINE_H
#define CLINICAL_VITALS_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "disaster_risk_engine.h"

/* Clinical triage levels */
typedef enum {
    CLINICAL_NORMAL   = 0,
    CLINICAL_ELEVATED = 1,
    CLINICAL_HIGH     = 2,
    CLINICAL_CRITICAL = 3
} clinical_risk_level_t;

/* Alert flags for specific physiological crises */
typedef enum {
    ALERT_NONE            = 0,
    ALERT_HYPOXIA         = (1u << 0),
    ALERT_TACHYCARDIA     = (1u << 1),
    ALERT_BRADYCARDIA     = (1u << 2),
    ALERT_AUTONOMIC_SHOCK = (1u << 3),
    ALERT_TACHYPNEA       = (1u << 4), /* RR >= 25 bpm: critical respiratory distress */
    ALERT_BRADYPNEA       = (1u << 5), /* RR <= 8 bpm: severe respiratory depression */
    ALERT_SIGNAL_NOISE    = (1u << 6), /* PPG SQI < 0.70: motion artifact rejection */
    ALERT_HYPOTHERMIA     = (1u << 7)  /* Body temp <= 35.0 C: hypothermia crisis */
} clinical_alert_flags_t;

#define CLINICAL_ADVISORY_MAX 256u

/* Full clinical assessment output */
typedef struct {
    clinical_risk_level_t  level;            /* Overall clinical risk */
    uint8_t                alert_flags;      /* Bitmask of clinical_alert_flags_t */
    uint8_t                news2_score;      /* mNEWS2 score: HR + SpO2 + RR + Temp */
    /* Per-parameter NEWS2 points. Published over serial (HRPTS/SPO2PTS/RRPTS)
     * so a display renders the device's own breakdown instead of re-deriving it
     * from the raw vitals. All four are zero until the SQI gate opens, because
     * the engine returns before scoring while the signal is held. Temp points
     * stay zero until a real body-temperature sensor is fitted. */
    uint8_t                news2_hr_pts;     /* NEWS2 HR points (0-3)   */
    uint8_t                news2_spo2_pts;   /* NEWS2 SpO2 points (0-3) */
    uint8_t                news2_rr_pts;     /* NEWS2 RR points (0-3)   */
    uint8_t                news2_temp_pts;   /* NEWS2 temp points (0-3) */
    float                  hr;               /* Heart rate (bpm) */
    float                  spo2;             /* Oxygen saturation (%) */
    float                  rmssd;            /* RMSSD (ms) - HRV autonomic tone */
    float                  rr;               /* Respiratory rate (breaths/min) */
    float                  sqi;              /* Signal Quality Index [0.0, 1.0] */
    float                  body_temp;        /* Body/skin temperature (°C); 0.0f = uninstrumented */
    char                   advisory[CLINICAL_ADVISORY_MAX];
} clinical_assessment_t;

/* Extended physiological vital signs assessment with 6th vital sign (temperature). */
void clinical_vitals_assess_ext(float hr,
                                float spo2,
                                float rmssd,
                                float rr,
                                float sqi,
                                float temp_c,
                                clinical_assessment_t *out);

/* Extended physiological vital signs assessment (mNEWS2 with RR & SQI gating). */
void clinical_vitals_assess_full(float hr,
                                 float spo2,
                                 float rmssd,
                                 float rr,
                                 float sqi,
                                 clinical_assessment_t *out);

/* Standard physiological vital signs assessment (backwards compatible). */
void clinical_vitals_assess(float hr,
                            float spo2,
                            float rmssd,
                            clinical_assessment_t *out);

/* Fuse environmental disaster risk and physiological clinical risk */
void clinical_fuse_triage(const clinical_assessment_t *clinical,
                          const risk_assessment_t *disaster,
                          risk_assessment_t *fused);

#ifdef __cplusplus
}
#endif

#endif /* CLINICAL_VITALS_ENGINE_H */
