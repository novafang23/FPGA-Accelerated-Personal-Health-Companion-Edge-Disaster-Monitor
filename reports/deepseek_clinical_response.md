```c
/* =====================================================================
 * Project SIH26181 - EdgeGuard
 * clinical_vitals_engine.h
 * =====================================================================
 * NEWS2-derived physiological triage engine for wearable/ICU edge device.
 *
 * Assumes existing disaster_risk_engine.h defines:
 *   typedef enum { RISK_NORMAL, RISK_ELEVATED, RISK_HIGH, RISK_CRITICAL } risk_level_t;
 *   typedef struct {
 *       risk_level_t risk_level;
 *       char advisory[256];   // or similar
 *       ...
 *   } risk_assessment_t;
 *
 * Adjust field names in clinical_fuse_triage() if your existing header differs.
 * =====================================================================
 */

#ifndef CLINICAL_VITALS_ENGINE_H
#define CLINICAL_VITALS_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "disaster_risk_engine.h"

/* Clinical triage levels */
typedef enum {
    CLINICAL_NORMAL = 0,
    CLINICAL_ELEVATED = 1,
    CLINICAL_HIGH = 2,
    CLINICAL_CRITICAL = 3
} clinical_risk_level_t;

/* Alert flags for specific physiological crises */
typedef enum {
    ALERT_NONE            = 0,
    ALERT_HYPOXIA         = (1u << 0),
    ALERT_TACHYCARDIA     = (1u << 1),
    ALERT_BRADYCARDIA     = (1u << 2),
    ALERT_AUTONOMIC_SHOCK = (1u << 3)
} clinical_alert_flags_t;

#define CLINICAL_ADVISORY_MAX 256u

/* Full clinical assessment output */
typedef struct {
    clinical_risk_level_t  level;            /* overall clinical risk */
    uint8_t                alert_flags;      /* bitmask of clinical_alert_flags_t */
    uint8_t                news2_score;      /* partial NEWS2 score: HR + SpO2 only */
    float                  hr;               /* heart rate, bpm */
    float                  spo2;             /* oxygen saturation, % */
    float                  rmssd;            /* RMSSD, ms (HRV, autonomic tone) */
    char                   advisory[CLINICAL_ADVISORY_MAX];
} clinical_assessment_t;

/* Assess physiological vital signs.
 * Any invalid sensor reading is mapped to CLINICAL_NORMAL without alert flags.
 */
void clinical_vitals_assess(float hr,
                            float spo2,
                            float rmssd,
                            clinical_assessment_t *out);

/* Fuse disaster risk and clinical risk. Worst risk wins.
 * Advisory combines both sources with '+' separator.
 */
void clinical_fuse_triage(const clinical_assessment_t *clinical,
                          const risk_assessment_t *disaster,
                          risk_assessment_t *fused);

#ifdef __cplusplus
}
#endif

#endif /* CLINICAL_VITALS_ENGINE_H */
```

```c
/* =====================================================================
 * Project SIH26181 - EdgeGuard
 * clinical_vitals_engine.c
 * =====================================================================
 * Fixed-point / integer-optimized NEWS2-derived triage for ESP32-S3.
 * All NEWS2 thresholds are applied after one-time rounding/scaling.
 * RMSSD is internally handled in x10 fixed point for shock detection.
 * =====================================================================
 */

#include "clinical_vitals_engine.h"

#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------
 * Internal fixed thresholds
 * ------------------------------------------------------------------- */
enum {
    /* NEWS2 HR score boundaries */
    HR_NEWS2_SCORE3_MAX = 40,      /* <= 40 -> 3 */
    HR_NEWS2_SCORE1_MAX = 50,      /* 41-50 -> 1 */
    HR_NEWS2_SCORE0_MAX = 90,      /* 51-90 -> 0 */
    HR_NEWS2_SCORE1B_MAX = 110,    /* 91-110 -> 1 */
    HR_NEWS2_SCORE2_MAX = 130,     /* 111-130 -> 2 */

    /* NEWS2 SpO2 score boundaries (Scale 1) */
    SPO2_NEWS2_SCORE0_MIN = 96,    /* >= 96 -> 0 */
    SPO2_NEWS2_SCORE1_MIN = 94,    /* 94-95 -> 1 */
    SPO2_NEWS2_SCORE2_MIN = 92,    /* 92-93 -> 2 */

    /* Absolute crisis thresholds */
    HR_CRITICAL_TACHY_MIN      = 150,
    HR_CRITICAL_BRADY_MAX      = 39,
    SPO2_CRITICAL_HYPOXIA_MAX  = 85,

    /* RMSSD fixed-point threshold for autonomic/shock flag:
     * 80 = 8.0 ms. Conservative threshold, used only with abnormal vitals.
     */
    RMSSD_SHOCK_THRESHOLD_X10 = 80
};

/* ---------------------------------------------------------------------
 * Helper: append a formatted condition string to a buffer.
 * Uses snprintf with truncation protection against advisory overflow.
 * ------------------------------------------------------------------- */
static void append_condition(char *buf, size_t buf_size, size_t *len,
                             const char *fmt, float v)
{
    if (buf == NULL || len == NULL || buf_size == 0) {
        return;
    }

    /* If the buffer is already completely full, stop. */
    if (*len >= buf_size - 1) {
        return;
    }

    /* Insert " + " between multiple conditions. */
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
        /* snprintf truncated the string and null-terminated it at avail-1. */
        *len = buf_size - 1;
    } else {
        *len += (size_t)ret;
    }

    buf[*len] = '\0';
}

/* ---------------------------------------------------------------------
 * NEWS2 partial scoring - heart rate
 * ------------------------------------------------------------------- */
static uint8_t news2_hr_score(int hr)
{
    if (hr <= HR_NEWS2_SCORE3_MAX)       return 3;   /* <= 40 */
    if (hr <= HR_NEWS2_SCORE1_MAX)       return 1;   /* 41-50 */
    if (hr <= HR_NEWS2_SCORE0_MAX)       return 0;   /* 51-90 */
    if (hr <= HR_NEWS2_SCORE1B_MAX)      return 1;   /* 91-110 */
    if (hr <= HR_NEWS2_SCORE2_MAX)       return 2;   /* 111-130 */
    return 3;                                          /* >= 131 */
}

/* ---------------------------------------------------------------------
 * NEWS2 partial scoring - SpO2 (scale 1, no oxygen hypercapnic scale)
 * ------------------------------------------------------------------- */
static uint8_t news2_spo2_score(int spo2)
{
    if (spo2 >= SPO2_NEWS2_SCORE0_MIN)   return 0;   /* >= 96 */
    if (spo2 >= SPO2_NEWS2_SCORE1_MIN)   return 1;   /* 94-95 */
    if (spo2 >= SPO2_NEWS2_SCORE2_MIN)   return 2;   /* 92-93 */
    return 3;                                          /* <= 91 */
}

/* ---------------------------------------------------------------------
 * Clinical Vitals Assessment
 * ------------------------------------------------------------------- */
void clinical_vitals_assess(float hr,
                            float spo2,
                            float rmssd,
                            clinical_assessment_t *out)
{
    if (out == NULL) {
        return;
    }

    memset(out, 0, sizeof(*out));
    out->level = CLINICAL_NORMAL;

    /* ---------- Input validation / false-positive prevention ---------- */
    if (!(hr > 0.0f && hr < 350.0f &&
          spo2 > 0.0f && spo2 <= 100.0f)) {
        snprintf(out->advisory, sizeof(out->advisory),
                 "Clinical vitals: invalid sensor data.");
        return;
    }

    /* ---------- Integer conversion / fixed-point preparation ---------- */
    int hr_i   = (int)(hr + 0.5f);       /* round to nearest bpm */
    int spo2_i = (int)spo2;              /* floor: do not over-estimate SpO2 */

    int32_t rmssd_x10 = -1;
    if (rmssd > 0.0f && rmssd < 500.0f) {
        rmssd_x10 = (int32_t)(rmssd * 10.0f + 0.5f);
    }

    out->hr    = hr;
    out->spo2  = spo2;
    out->rmssd = (rmssd > 0.0f) ? rmssd : 0.0f;

    /* ---------- NEWS2 partial score ---------- */
    uint8_t hr_score   = news2_hr_score(hr_i);
    uint8_t spo2_score = news2_spo2_score(spo2_i);
    uint8_t score      = hr_score + spo2_score;

    out->news2_score = score;

    /* ---------- Alert flags ---------- */
    uint8_t flags = ALERT_NONE;
    bool shock_present = false;

    /* Hypoxia */
    if (spo2_i <= 91) {
        flags |= ALERT_HYPOXIA;
    }

    /* Tachycardia */
    if (hr_i >= 111) {
        flags |= ALERT_TACHYCARDIA;
    }

    /* Bradycardia */
    if (hr_i <= 40) {
        flags |= ALERT_BRADYCARDIA;
    }

    /* Autonomic / shock: low RMSSD with concurrent abnormal vitals */
    if (rmssd_x10 >= 0 &&
        rmssd_x10 < RMSSD_SHOCK_THRESHOLD_X10 &&
        (hr_i >= 100 || spo2_i <= 94)) {
        flags |= ALERT_AUTONOMIC_SHOCK;
        shock_present = true;
    }

    out->alert_flags = flags;

    /* ---------- Overall clinical risk level ---------- */
    bool critical_vital = (spo2_i <= SPO2_CRITICAL_HYPOXIA_MAX) ||
                          (hr_i >= HR_CRITICAL_TACHY_MIN) ||
                          (hr_i <= HR_CRITICAL_BRADY_MAX);

    if (critical_vital || score >= 5 || (shock_present && score >= 4)) {
        out->level = CLINICAL_CRITICAL;
    } else if (score >= 3 ||
               spo2_i <= 91 ||
               hr_i >= 131 ||
               hr_i <= 40 ||
               shock_present) {
        out->level = CLINICAL_HIGH;
    } else if (score >= 1) {
        out->level = CLINICAL_ELEVATED;
    } else {
        out->level = CLINICAL_NORMAL;
    }

    /* ---------- Advisory construction ---------- */
    char conditions[192];
    conditions[0] = '\0';
    size_t cond_len = 0;

    if (flags & ALERT_HYPOXIA) {
        const char *fmt = (spo2_i <= SPO2_CRITICAL_HYPOXIA_MAX)
                          ? "Severe Hypoxia (SpO2 %.0f%%)"
                          : "Hypoxia (SpO2 %.0f%%)";
        append_condition(conditions, sizeof(conditions), &cond_len,
                         fmt, (float)spo2_i);
    }

    if (flags & ALERT_TACHYCARDIA) {
        append_condition(conditions, sizeof(conditions), &cond_len,
                         "Tachycardia (HR %.0f bpm)", (float)hr_i);
    }

    if (flags & ALERT_BRADYCARDIA) {
        append_condition(conditions, sizeof(conditions), &cond_len,
                         "Bradycardia (HR %.0f bpm)", (float)hr_i);
    }

    if (flags & ALERT_AUTONOMIC_SHOCK) {
        append_condition(conditions, sizeof(conditions), &cond_len,
                         "Autonomic/Shock (RMSSD %.1f ms)", rmssd);
    }

    const char *severity = "";
    const char *action = "";

    switch (out->level) {
        case CLINICAL_NORMAL:
            action = "Clinical vitals within normal limits.";
            break;
        case CLINICAL_ELEVATED:
            severity = "ELEVATED: ";
            action = "Mild physiological deviation. Repeat assessment.";
            break;
        case CLINICAL_HIGH:
            severity = "HIGH: ";
            action = "Physiological deterioration - urgent clinical review.";
            break;
        case CLINICAL_CRITICAL:
            severity = "CRITICAL: ";
            action = "Immediate emergency medical response required.";
            break;
        default:
            break;
    }

    if (cond_len > 0) {
        snprintf(out->advisory, sizeof(out->advisory), "%s%s. %s",
                 severity, conditions, action);
    } else if (out->level != CLINICAL_NORMAL) {
        snprintf(out->advisory, sizeof(out->advisory),
                 "%sNEWS2 partial score %u. %s",
                 severity, score, action);
    } else {
        snprintf(out->advisory, sizeof(out->advisory), "%s", action);
    }
}

/* ---------------------------------------------------------------------
 * Fused Disaster + Clinical Triage
 * ------------------------------------------------------------------- */
void clinical_fuse_triage(const clinical_assessment_t *clinical,
                          const risk_assessment_t *disaster,
                          risk_assessment_t *fused)
{
    if (fused == NULL) {
        return;
    }

    char disaster_advisory[sizeof(fused->advisory)];
    risk_level_t disaster_level = RISK_NORMAL;
    bool has_disaster = false;

    if (disaster != NULL) {
        disaster_level = disaster->risk_level;
        snprintf(disaster_advisory, sizeof(disaster_advisory), "%s",
                 disaster->advisory);
        has_disaster = true;
    } else {
        disaster_advisory[0] = '\0';
    }

    /* Clinical unavailable or NULL */
    if (clinical == NULL) {
        if (has_disaster) {
            *fused = *disaster;
        } else {
            memset(fused, 0, sizeof(*fused));
            fused->risk_level = RISK_NORMAL;
            snprintf(fused->advisory, sizeof(fused->advisory),
                     "No clinical or disaster data.");
        }
        return;
    }

    /* Map clinical triage level onto disaster risk_level_t */
    risk_level_t clinical_mapped;
    switch (clinical->level) {
        case CLINICAL_NORMAL:
            clinical_mapped = RISK_NORMAL;
            break;
        case CLINICAL_ELEVATED:
            clinical_mapped = RISK_ELEVATED;
            break;
        case CLINICAL_HIGH:
            clinical_mapped = RISK_HIGH;
            break;
        case CLINICAL_CRITICAL:
            clinical_mapped = RISK_CRITICAL;
            break;
        default:
            clinical_mapped = RISK_NORMAL;
            break;
    }

    /* Worst risk wins */
    risk_level_t fused_level = (clinical_mapped > disaster_level)
                               ? clinical_mapped
                               : disaster_level;

    /* Copy disaster struct to preserve any vendor-specific fields */
    if (has_disaster && fused != disaster) {
        *fused = *disaster;
    } else if (!has_disaster) {
        memset(fused, 0, sizeof(*fused));
    }

    fused->risk_level = fused_level;

    const char *clin_adv = clinical->advisory;
    const char *dis_adv  = has_disaster ? disaster_advisory
                                        : "No environmental hazard";

    /* Order the advisory: most severe source first */
    if (clinical_mapped >= disaster_level) {
        snprintf(fused->advisory, sizeof(fused->advisory),
                 "%s + %s", clin_adv, dis_adv);
    } else {
        snprintf(fused->advisory, sizeof(fused->advisory),
                 "%s + %s", dis_adv, clin_adv);
    }
}
```

---

## Verification Strategy Against MIMIC-III

### 1. Threshold unit tests
- Exercise exact NEWS2 boundary values:
  - HR: 39, 40, 41, 50, 51, 90, 91, 110, 111, 130, 131, 150, 151.
  - SpO₂: 85, 86, 91, 92, 93, 94, 95, 96.
  - Invalid inputs: `hr = 0`, `spo2 = 0`, `spo2 = 101`, `hr = 350`, `NaN`.
- Assert expected `news2_score`, `alert_flags`, and overall `level`.

### 2. MIMIC-III aggregate replay
For each of the 98 ICU patients, evaluate the engine on:
- `hr_min`, `hr_mean`, `hr_max`
- `spo2_min`, `spo2_mean`, `spo2_max`

Expected exemplars:
| Subject | Extreme Reading | Expected Result |
|---------|-----------------|-----------------|
| 10013 | SpO₂ 60 / HR 113 | `CLINICAL_CRITICAL` hypoxia + tachycardia flag |
| 10027 | HR 176 | `CLINICAL_CRITICAL` tachycardia |
| 10032 | HR 118 / SpO₂ 88 | `CLINICAL_HIGH` |
| 10036 | SpO₂ 80 | `CLINICAL_CRITICAL` hypoxia |
| 10042 | HR 29 | `CLINICAL_CRITICAL` bradycardia |

### 3. Sensitivity/specificity audit
- Define a “true emergency” reference as:
  - SpO₂ ≤ 85%, OR
  - HR ≥ 150 bpm, OR
  - HR ≤ 39 bpm, OR
  - RMSSD < 8 ms with HR ≥ 100 or SpO₂ ≤ 94.
- Compute sensitivity: proportion of true emergencies assigned `CLINICAL_HIGH` or `CLINICAL_CRITICAL`.
- Compute specificity: proportion of non-emergency readings assigned `CLINICAL_NORMAL` or `CLINICAL_ELEVATED`.
- Target: sensitivity ≥ 98%, specificity ≥ 90% on the 30,785 MIMIC-III vital readings.

### 4. Embedded performance
- Run the entire `clinical_vitals_assess()` on ESP32-S3.
- Measure bounded execution:
  - Average < 100 µs with -O2.
  - Stack usage < 1 KB.
  - No dynamic allocation or recursion.

### 5. Clinical + disaster fusion test
- Benign environment + `CLINICAL_CRITICAL` → fused risk `RISK_CRITICAL`, advisory contains both clinical and environmental context.
- Critical environment + `CLINICAL_NORMAL` → fused risk `RISK_CRITICAL`, advisory still includes clinical note.
- Both critical → fused risk `RISK_CRITICAL`, advisory combines both crisis strings, matching the required format:

```
CRITICAL: Severe Hypoxia (SpO2 84%) + High Smoke Hazard. Immediate emergency medical response required.
```