/*
 * fall_detect.c
 * 3-Stage Biomechanical Fall Detection Engine & Motion Energy Analyzer
 * SIH26181 Health Companion & Edge Disaster Monitor
 *
 * Implements the 3-stage kinematic fall state machine:
 *   Free-fall -> Impact -> Post-impact immobility / stillness
 *
 * The numeric thresholds below are engineering defaults for this board
 * (0.60 g free-fall, 2.50 g impact, 80 dps, 1.5 s stillness). They are not
 * taken from one specific paper; tune them against captured falls and
 * activities of daily living for the final device.
 */

#include "fall_detect.h"
#include <string.h>
#include <math.h>

void fall_detect_default_config(fall_config_t *cfg) {
    if (!cfg) return;
    cfg->free_fall_thresh_g        = 0.60f;  /* Weightlessness threshold (< 0.60g) */
    cfg->impact_thresh_g           = 2.50f;  /* Impact shock peak (> 2.50g) */
    cfg->gyro_impact_thresh_dps    = 80.0f;  /* Accompanying rotational jerk (>= 80 dps) */
    cfg->stillness_accel_tol_g     = 0.30f;  /* |A - 1.0g| <= 0.30g during rest */
    cfg->stillness_gyro_thresh_dps = 40.0f;  /* Angular rate <= 40 dps during rest */
    cfg->free_fall_max_ms          = 800u;   /* Max 800ms between free-fall and impact */
    cfg->post_impact_settle_ms     = 400u;   /* 400ms physical rebound settling */
    cfg->inactivity_duration_ms    = 1500u;  /* 1.5s sustained stillness confirms fall */
    cfg->soft_impact_thresh_g      = 1.80f;  /* Slump/faint path: moderate impact, no free-fall */
    cfg->soft_posture_min_deg      = 55.0f;  /* ...confirmed only if body orientation changed this much */
}

void fall_detect_init(fall_detector_t *det, const fall_config_t *cfg) {
    if (!det) return;
    memset(det, 0, sizeof(*det));
    if (cfg) {
        det->cfg = *cfg;
    } else {
        fall_detect_default_config(&det->cfg);
    }
    det->state = FALL_STATE_MONITORING;
    det->baseline_az = 1.0f; /* Normal gravity orientation */
}

void fall_detect_reset(fall_detector_t *det) {
    if (!det) return;
    det->state               = FALL_STATE_MONITORING;
    det->state_entry_ms      = 0;
    det->free_fall_start_ms  = 0;
    det->impact_ms           = 0;
    det->inactivity_start_ms = 0;
    det->impact_a_mag        = 0.0f;
    det->impact_g_mag        = 0.0f;
    det->post_fall_ax        = 0.0f;
    det->post_fall_ay        = 0.0f;
    det->post_fall_az        = 0.0f;
    det->fall_confirmed      = false;
    det->soft_path           = false;
    det->fall_confirmed_ms   = 0;
}

void fall_detect_clear_alarm(fall_detector_t *det) {
    if (!det) return;
    det->fall_confirmed      = false;
    det->soft_path           = false;
    det->inactivity_start_ms = 0;
    det->impact_a_mag        = 0.0f;
    det->impact_g_mag        = 0.0f;
    det->post_fall_ax        = 0.0f;
    det->post_fall_ay        = 0.0f;
    det->post_fall_az        = 0.0f;
    det->state               = FALL_STATE_MONITORING;
}

fall_state_t fall_detect_get_state(const fall_detector_t *det) {
    return det ? det->state : FALL_STATE_MONITORING;
}

const char *fall_state_name(fall_state_t s) {
    switch (s) {
        case FALL_STATE_MONITORING: return "MONITORING";
        case FALL_STATE_FREE_FALL:  return "FREE_FALL";
        case FALL_STATE_IMPACT:     return "IMPACT";
        case FALL_STATE_INACTIVITY: return "INACTIVITY";
        case FALL_STATE_CONFIRMED:  return "FALL_CONFIRMED";
        default:                    return "UNKNOWN";
    }
}

float fall_detect_get_motion_energy(const fall_detector_t *det) {
    return det ? det->motion_energy : 0.0f;
}

bool fall_detect_is_motion_quiet(const fall_detector_t *det) {
    return det ? (det->motion_energy < 0.10f) : true;
}

bool fall_detect_is_confirmed(const fall_detector_t *det) {
    return (det != NULL && det->fall_confirmed);
}

float fall_detect_get_impact_accel(const fall_detector_t *det) {
    return det ? det->impact_a_mag : 0.0f;
}

float fall_detect_get_impact_gyro(const fall_detector_t *det) {
    return det ? det->impact_g_mag : 0.0f;
}

float fall_detect_get_posture_angle_deg(const fall_detector_t *det) {
    if (!det) return 0.0f;
    float b_len = sqrtf(det->baseline_ax * det->baseline_ax +
                        det->baseline_ay * det->baseline_ay +
                        det->baseline_az * det->baseline_az);
    float p_len = sqrtf(det->post_fall_ax * det->post_fall_ax +
                        det->post_fall_ay * det->post_fall_ay +
                        det->post_fall_az * det->post_fall_az);
    if (b_len < 0.1f || p_len < 0.1f) return 0.0f;

    float dot = (det->baseline_ax * det->post_fall_ax +
                 det->baseline_ay * det->post_fall_ay +
                 det->baseline_az * det->post_fall_az) / (b_len * p_len);
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;

    return acosf(dot) * (180.0f / 3.14159265f);
}

bool fall_detect_update(fall_detector_t *det, uint32_t now_ms,
                        float ax, float ay, float az,
                        float gx, float gy, float gz) {
    if (!det) return false;

    /* 1. Calculate instant vector magnitudes using single-precision FPU */
    float a_mag = sqrtf(ax * ax + ay * ay + az * az);
    float g_mag = sqrtf(gx * gx + gy * gy + gz * gz);
    det->current_a_mag = a_mag;
    det->current_g_mag = g_mag;

    /* 2. Compute dynamic acceleration |A - 1.0g| and update smoothed motion energy */
    float dyn_a = fabsf(a_mag - 1.0f);
    det->motion_energy = 0.85f * det->motion_energy + 0.15f * dyn_a;

    bool newly_confirmed = false;

    /* 3. Execute 3-Stage Biomechanical Fall State Machine */
    switch (det->state) {
        case FALL_STATE_MONITORING:
            /* Track baseline orientation during normal activity. Freeze it while
             * the user is quiet AND the current gravity vector is far from the
             * baseline (e.g. lying down for bed rest) so the "upright" reference
             * is not dragged to a lying posture, which would hide a later fall's
             * posture change. Walking/activity re-enables adaptation. */
            {
                float cos_b = 1.0f;
                float b_len = sqrtf(det->baseline_ax * det->baseline_ax +
                                    det->baseline_ay * det->baseline_ay +
                                    det->baseline_az * det->baseline_az);
                if (b_len >= 0.1f && a_mag >= 0.1f) {
                    cos_b = (det->baseline_ax * ax + det->baseline_ay * ay +
                             det->baseline_az * az) / (b_len * a_mag);
                }
                bool far_from_baseline = (cos_b < 0.5736f); /* cos(55 deg) */
                bool freeze = fall_detect_is_motion_quiet(det) && far_from_baseline;
                if (!freeze) {
                    det->baseline_ax = 0.95f * det->baseline_ax + 0.05f * ax;
                    det->baseline_ay = 0.95f * det->baseline_ay + 0.05f * ay;
                    det->baseline_az = 0.95f * det->baseline_az + 0.05f * az;
                }
            }

            /* Stage 1 Trigger: Acceleration drops into free-fall weightlessness */
            if (a_mag <= det->cfg.free_fall_thresh_g) {
                det->state               = FALL_STATE_FREE_FALL;
                det->free_fall_start_ms  = now_ms;
                det->state_entry_ms      = now_ms;
                det->inactivity_start_ms = 0;
                det->impact_a_mag        = 0.0f;
                det->impact_g_mag        = 0.0f;
                det->soft_path           = false;
            } else if (det->cfg.soft_impact_thresh_g > 0.0f &&
                       a_mag >= det->cfg.soft_impact_thresh_g) {
                /* Soft-fall path: slump/faint without weightlessness. Enter
                 * inactivity evaluation; confirmation additionally requires a
                 * large posture change so a bump or hard step is rejected. */
                det->state               = FALL_STATE_INACTIVITY;
                det->impact_ms           = now_ms;
                det->state_entry_ms      = now_ms;
                det->impact_a_mag        = a_mag;
                det->impact_g_mag        = g_mag;
                det->inactivity_start_ms = 0;
                det->soft_path           = true;
            }
            break;

        case FALL_STATE_FREE_FALL:
            /* Check if free-fall timed out without impact (e.g. elevator or tossed onto soft bed) */
            if ((now_ms - det->free_fall_start_ms) > det->cfg.free_fall_max_ms) {
                det->state = FALL_STATE_MONITORING;
                det->soft_path = false;
                break;
            }

            /* Stage 2 Trigger: High deceleration impact shock + accompanying rotational jerk */
            if (a_mag >= det->cfg.impact_thresh_g &&
                (det->cfg.gyro_impact_thresh_dps <= 0.0f || g_mag >= det->cfg.gyro_impact_thresh_dps)) {
                det->impact_ms           = now_ms;
                det->state_entry_ms      = now_ms;
                det->impact_a_mag        = a_mag;
                det->impact_g_mag        = g_mag;
                det->inactivity_start_ms = 0;
                det->soft_path           = false;

                /* Immediately enter post-impact inactivity evaluation (Stage 3) */
                det->state = FALL_STATE_INACTIVITY;
            }
            break;

        case FALL_STATE_IMPACT:
            /* Transitory state: cleanly enter post-impact inactivity monitoring */
            det->state = FALL_STATE_INACTIVITY;
            det->soft_path = false;
            det->inactivity_start_ms = 0;
            break;

        case FALL_STATE_INACTIVITY: {
            uint32_t elapsed_since_impact = (now_ms >= det->impact_ms) ? (now_ms - det->impact_ms) : 0u;

            /* Allow initial physical rebound settling time */
            if (elapsed_since_impact < det->cfg.post_impact_settle_ms) {
                if (a_mag > det->impact_a_mag) det->impact_a_mag = a_mag;
                if (g_mag > det->impact_g_mag) det->impact_g_mag = g_mag;
                break;
            }

            /* Check if user got up and is moving vigorously (walking / waving / recovered) */
            if (dyn_a > 0.65f || g_mag > 120.0f) {
                /* Subject is active and recovered -> Cancel false alarm */
                det->state = FALL_STATE_MONITORING;
                det->soft_path = false;
                det->inactivity_start_ms = 0;
                det->impact_a_mag = 0.0f;
                det->impact_g_mag = 0.0f;
                det->post_fall_ax = 0.0f;
                det->post_fall_ay = 0.0f;
                det->post_fall_az = 0.0f;
                break;
            }

            /* Check stillness condition: acceleration near 1.0g and low rotational rate */
            bool is_still = (dyn_a <= det->cfg.stillness_accel_tol_g) &&
                            (g_mag <= det->cfg.stillness_gyro_thresh_dps);

            if (is_still) {
                /* Start continuous stillness timer and smooth post-fall orientation */
                if (det->inactivity_start_ms == 0) {
                    det->inactivity_start_ms = now_ms;
                    det->post_fall_ax = ax;
                    det->post_fall_ay = ay;
                    det->post_fall_az = az;
                } else {
                    det->post_fall_ax = 0.90f * det->post_fall_ax + 0.10f * ax;
                    det->post_fall_ay = 0.90f * det->post_fall_ay + 0.10f * ay;
                    det->post_fall_az = 0.90f * det->post_fall_az + 0.10f * az;
                }

                uint32_t still_duration = now_ms - det->inactivity_start_ms;
                if (still_duration >= det->cfg.inactivity_duration_ms) {
                    if (det->soft_path &&
                        fall_detect_get_posture_angle_deg(det) < det->cfg.soft_posture_min_deg) {
                        /* Bump / hard step: still upright afterwards -> not a fall */
                        det->state = FALL_STATE_MONITORING;
                        det->soft_path = false;
                        det->inactivity_start_ms = 0;
                        det->impact_a_mag = 0.0f;
                        det->impact_g_mag = 0.0f;
                        det->post_fall_ax = 0.0f;
                        det->post_fall_ay = 0.0f;
                        det->post_fall_az = 0.0f;
                        break;
                    }
                    /* Stage 3 Confirmed: Subject has remained incapacitated on ground */
                    det->state             = FALL_STATE_CONFIRMED;
                    det->fall_confirmed    = true;
                    det->fall_confirmed_ms = now_ms;
                    det->soft_path         = false;
                    newly_confirmed        = true;
                }
            } else {
                /* Continuous stillness was interrupted by movement -> reset stillness start */
                det->inactivity_start_ms = 0;

                /* If sustained stillness is not achieved within 4.4s total, stand down */
                if (elapsed_since_impact > (det->cfg.post_impact_settle_ms + det->cfg.inactivity_duration_ms + 2500u)) {
                    det->state = FALL_STATE_MONITORING;
                    det->soft_path = false;
                    det->impact_a_mag = 0.0f;
                    det->impact_g_mag = 0.0f;
                    det->post_fall_ax = 0.0f;
                    det->post_fall_ay = 0.0f;
                    det->post_fall_az = 0.0f;
                }
            }
            break;
        }

        case FALL_STATE_CONFIRMED:
            /* Auto-standby after 60 seconds if not cleared */
            if ((now_ms - det->fall_confirmed_ms) > 60000u) {
                det->fall_confirmed = false;
                det->soft_path = false;
                det->impact_a_mag = 0.0f;
                det->impact_g_mag = 0.0f;
                det->post_fall_ax = 0.0f;
                det->post_fall_ay = 0.0f;
                det->post_fall_az = 0.0f;
                det->state = FALL_STATE_MONITORING;
            }
            break;

        default:
            det->state = FALL_STATE_MONITORING;
            det->soft_path = false;
            break;
    }

    return newly_confirmed;
}
