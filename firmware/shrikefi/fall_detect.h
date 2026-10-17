/*
 * fall_detect.h
 * 3-Stage Biomechanical Fall Detection Engine & Motion Energy Analyzer
 * SIH26181 Health Companion & Edge Disaster Monitor
 *
 * Algorithm architecture:
 *   Stage 1: Free-Fall Phase (Weightlessness: A_mag < 0.60g)
 *   Stage 2: Impact Phase (High Deceleration Spike: A_mag > 2.50g, G_mag >= 80 dps)
 *   Stage 3: Post-Impact Inactivity Phase (Immobility & Stillness: |A_mag - 1.0g| < 0.30g for >= 1.5s)
 *
 * Designed in pure C99 for ESP32-S3 single-precision hardware FPU (zero double, zero RTOS).
 * Fully host-testable on PC without hardware attached.
 */

#ifndef FALL_DETECT_H
#define FALL_DETECT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fall Detection FSM States */
typedef enum {
    FALL_STATE_MONITORING = 0, /* Normal gait / baseline motion monitoring */
    FALL_STATE_FREE_FALL,     /* Free-fall weightlessness detected (A_mag < 0.60g) */
    FALL_STATE_IMPACT,        /* Impact shock peak detected (A_mag > 2.50g) */
    FALL_STATE_INACTIVITY,    /* Evaluating post-impact stillness / incapacitation */
    FALL_STATE_CONFIRMED      /* Fall verified and latched */
} fall_state_t;

/* Tuning parameters */
typedef struct {
    float    free_fall_thresh_g;        /* Acceleration floor for weightlessness (default: 0.60g) */
    float    impact_thresh_g;           /* Acceleration ceiling for impact shock (default: 2.50g) */
    float    gyro_impact_thresh_dps;    /* Angular rate threshold for rotational impact (default: 80.0 dps) */
    float    stillness_accel_tol_g;     /* Allowable accel deviation from 1.0g during rest (default: 0.30g) */
    float    stillness_gyro_thresh_dps; /* Allowable gyro rotation during stillness (default: 40.0 dps) */
    uint32_t free_fall_max_ms;          /* Max time between free-fall and impact (default: 800 ms) */
    uint32_t post_impact_settle_ms;     /* Rebound settling time before inactivity check (default: 400 ms) */
    uint32_t inactivity_duration_ms;    /* Duration of stillness required to confirm incapacitation (default: 1500 ms) */
    float    soft_impact_thresh_g;      /* Soft-fall path (slump/faint, no free-fall): impact floor (default: 1.80g, 0 disables) */
    float    soft_posture_min_deg;      /* Soft-fall path: min posture change required to confirm (default: 55 deg) */
} fall_config_t;

/* Fall Detector State Instance */
typedef struct {
    fall_config_t cfg;
    fall_state_t  state;

    uint32_t      state_entry_ms;
    uint32_t      free_fall_start_ms;
    uint32_t      impact_ms;
    uint32_t      inactivity_start_ms;

    /* Pre-fall and post-fall orientation tracking */
    float         baseline_ax;
    float         baseline_ay;
    float         baseline_az;
    float         post_fall_ax;
    float         post_fall_ay;
    float         post_fall_az;

    /* Real-time kinematic metrics */
    float         current_a_mag;        /* Instantaneous total acceleration (g) */
    float         current_g_mag;        /* Instantaneous total angular velocity (dps) */
    float         motion_energy;        /* Filtered dynamic acceleration (|A - 1.0g|) (g) */

    /* Impact peak metrics recorded during fall event */
    float         impact_a_mag;         /* Peak acceleration reached during impact (g) */
    float         impact_g_mag;         /* Peak angular velocity reached during impact (dps) */

    bool          soft_path;            /* Event entered via slump path (no free-fall); needs posture confirm */
    bool          fall_confirmed;
    uint32_t      fall_confirmed_ms;
} fall_detector_t;

/**
 * @brief Load default fall detection thresholds and timing windows
 */
void fall_detect_default_config(fall_config_t *cfg);

/**
 * @brief Initialize fall detection engine
 * @param det Pointer to detector state instance
 * @param cfg Custom configuration, or NULL for defaults
 */
void fall_detect_init(fall_detector_t *det, const fall_config_t *cfg);

/**
 * @brief Reset detector back to baseline MONITORING state
 */
void fall_detect_reset(fall_detector_t *det);

/**
 * @brief Update fall detection state machine with new IMU sample
 *
 * @param det Pointer to detector instance
 * @param now_ms Monotonic millisecond timestamp
 * @param ax X-axis acceleration in g
 * @param ay Y-axis acceleration in g
 * @param az Z-axis acceleration in g
 * @param gx X-axis angular velocity in dps
 * @param gy Y-axis angular velocity in dps
 * @param gz Z-axis angular velocity in dps
 * @return true on the exact transition when a fall is CONFIRMED, false otherwise
 */
bool fall_detect_update(fall_detector_t *det, uint32_t now_ms,
                        float ax, float ay, float az,
                        float gx, float gy, float gz);

/**
 * @brief Get current FSM state
 */
fall_state_t fall_detect_get_state(const fall_detector_t *det);

/**
 * @brief Get human-readable string for fall state
 */
const char *fall_state_name(fall_state_t s);

/**
 * @brief Get filtered dynamic motion energy in g (for PPG SQI motion artifact rejection)
 * Healthy resting: < 0.08g. Walking: 0.10g - 0.25g. Vigorous motion/running: > 0.35g.
 */
float fall_detect_get_motion_energy(const fall_detector_t *det);

/**
 * @brief Check if current motion is quiet (< 0.10g dynamic motion)
 */
bool fall_detect_is_motion_quiet(const fall_detector_t *det);

/**
 * @brief Check if fall alarm is currently active / confirmed
 */
bool fall_detect_is_confirmed(const fall_detector_t *det);

/**
 * @brief Get peak impact acceleration recorded during latest fall (in g)
 */
float fall_detect_get_impact_accel(const fall_detector_t *det);

/**
 * @brief Get peak impact angular velocity recorded during latest fall (in dps)
 */
float fall_detect_get_impact_gyro(const fall_detector_t *det);

/**
 * @brief Calculate posture orientation change angle in degrees
 * Compares pre-fall baseline gravity vector against post-fall stillness gravity vector.
 * Returns angle in degrees [0.0, 180.0]. Typical fall resulting in lying down: > 50-60 deg.
 */
float fall_detect_get_posture_angle_deg(const fall_detector_t *det);

/**
 * @brief Clear a confirmed fall alarm and return to MONITORING
 */
void fall_detect_clear_alarm(fall_detector_t *det);

#ifdef __cplusplus
}
#endif

#endif /* FALL_DETECT_H */
