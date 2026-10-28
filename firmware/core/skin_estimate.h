/*
 * skin_estimate.h
 * Dual-Sensor Skin Temperature Estimation (MAX30102 Die Temp + BME280 Ambient)
 *
 * Physical Model (Median-of-5 Peak-Holding Monotonic Thermal Conduction):
 *   T_target = b0 + b1 * delta_die + b2 * (T_amb - 27.5)
 *   Strictly non-decreasing first-order thermal convergence during warm contact.
 *
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#ifndef SKIN_ESTIMATE_H
#define SKIN_ESTIMATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Clinical / Sensor Range Constraints */
#define SKIN_EST_DEFAULT_B0          34.10f  /* Calibrated peripheral skin baseline at 27.5C amb */
#define SKIN_EST_DEFAULT_B1          0.55f   /* Gain on bounded contact delta (T_die_peak - T_base) */
#define SKIN_EST_DEFAULT_B2          0.12f   /* Ambient room correction gain */
#define SKIN_EST_DEFAULT_TAU_S       0.00f   /* Deprecated derivative lead (0 = disabled for monotonic stability) */

#define SKIN_EST_INITIAL_TOUCH_C    32.20f   /* Starting anchor when finger first makes optical contact */
#define SKIN_EST_MIN_REF_C          31.0f    /* Lower physiological skin display bound (<31C = COLD) */
#define SKIN_EST_MAX_REF_C          36.8f    /* Upper peripheral skin display bound */
#define SKIN_EST_EQUIL_MAX_C        36.35f   /* Hard upper equilibrium ceiling against LED self-heating */
#define SKIN_EST_MAX_RISE_PER_SEC   0.28f    /* Maximum upward slew rate per 1 Hz update (C/s) */
#define SKIN_EST_VALID_MIN_MS       3000u    /* Publish monotonic estimate after 3s continuous contact */
#define SKIN_EST_SETTLE_MIN_MS      12000u   /* Minimum 12 seconds contact before thermal equilibrium lock */
#define SKIN_EST_MAX_SLOPE_C_PER_S  0.050f   /* Max residual slope for settled flag */
#define SKIN_EST_HIST_LEN           16       /* Rolling slope window */
#define SKIN_EST_MED_LEN            5        /* 5-sample median filter window for spike rejection */

typedef struct {
    /* Calibration Coefficients */
    float b0;           /* Skin baseline at 27.5C amb (nominal 34.10C) */
    float b1;           /* Gain on contact delta (nominal 0.55) */
    float b2;           /* Ambient room correction gain (nominal 0.12) */
    float tau_lead_s;   /* Retained for struct compatibility (0.0f) */
    float cal_min_c;
    float cal_max_c;
    bool  is_calibrated;

    /* Live State */
    float last_raw_die_c;
    float last_filtered_die_c;
    float last_amb_c;
    float idle_die_c;               /* Pre-contact steady die baseline */
    float contact_baseline_die_c;   /* Effective baseline at moment finger touched */
    float contact_peak_die_c;       /* Peak median-filtered die temp during current contact */
    float estimated_skin_c;
    float slope_c_per_sec;

    /* 5-sample median filter state */
    float   med_buf[SKIN_EST_MED_LEN];
    uint8_t med_count;
    uint8_t med_idx;

    uint32_t contact_start_ms;
    uint32_t last_sample_ms;
    bool     contact_active;
    bool     is_settled;
    bool     is_valid;
    bool     is_below_range;

    /* Rolling History for Slope Calculation */
    float    die_history[SKIN_EST_HIST_LEN];
    uint32_t time_history_ms[SKIN_EST_HIST_LEN];
    uint8_t  hist_count;
    uint8_t  hist_idx;
} skin_estimate_t;

/**
 * @brief Initialize skin temperature estimator with default or stored coefficients.
 */
void skin_estimate_init(skin_estimate_t *st, float b0, float b1, float b2, bool is_calibrated);

/**
 * @brief Update estimator with 1 Hz die and ambient temperature samples.
 * @param st Pointer to estimator state
 * @param raw_die_c MAX30102 die temperature reading in Celsius
 * @param amb_c BME280 ambient temperature reading in Celsius
 * @param finger_present True if optical PPG contact is confirmed
 * @param now_ms System time in milliseconds
 */
void skin_estimate_update(skin_estimate_t *st, float raw_die_c, float amb_c,
                          bool finger_present, uint32_t now_ms);

/**
 * @brief Reset contact and settling state (e.g. when finger is removed).
 */
void skin_estimate_reset(skin_estimate_t *st);

/**
 * @brief Update calibration parameters dynamically.
 */
void skin_estimate_set_calibration(skin_estimate_t *st, float b0, float b1, float b2,
                                  float min_c, float max_c);

#ifdef __cplusplus
}
#endif

#endif /* SKIN_ESTIMATE_H */
