/*
 * skin_estimate.h
 * Dual-Sensor Skin Temperature Estimation (MAX30102 Die Temp + BME280 Ambient)
 *
 * Physical Model (Thermal Divider):
 *   T_skin = b0 + b1 * T_die + b2 * T_amb
 *   Constraint: b1 + b2 ~ 1.0 (thermal divider equilibrium)
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
#define SKIN_EST_DEFAULT_B0          34.50f  /* Calibrated normal human skin baseline at 27.5C amb */
#define SKIN_EST_DEFAULT_B1          1.05f   /* Gain on contact delta (T_die - T_idle) */
#define SKIN_EST_DEFAULT_B2          0.15f   /* Ambient room correction gain */
#define SKIN_EST_DEFAULT_TAU_S       4.00f   /* Bidirectional Newton thermal lead time constant (s) */

#define SKIN_EST_MIN_REF_C          31.0f   /* Lower physiological skin display bound */
#define SKIN_EST_MAX_REF_C          42.0f   /* Upper physiological skin threshold */
#define SKIN_EST_VALID_MIN_MS       3000u   /* Publish lead-compensated estimate after 3s contact */
#define SKIN_EST_SETTLE_MIN_MS      15000u  /* Minimum 15 seconds contact before full settling */
#define SKIN_EST_MAX_SLOPE_C_PER_S  0.025f  /* Max slope for thermal equilibrium (>1 LSB/10s) */
#define SKIN_EST_HIST_LEN           16      /* Rolling slope window */

typedef struct {
    /* Calibration Coefficients */
    float b0;           /* Skin baseline at 27.5C amb (nominal 34.50C) */
    float b1;           /* Gain on contact delta (nominal 1.05) */
    float b2;           /* Ambient room correction gain (nominal 0.15) */
    float tau_lead_s;   /* Thermal lead time constant (s) */
    float cal_min_c;
    float cal_max_c;
    bool  is_calibrated;

    /* Live State */
    float last_raw_die_c;
    float last_filtered_die_c;
    float last_amb_c;
    float idle_die_c;               /* Pre-contact steady die baseline */
    float contact_baseline_die_c;   /* Frozen baseline at moment finger touched */
    float estimated_skin_c;
    float slope_c_per_sec;

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
