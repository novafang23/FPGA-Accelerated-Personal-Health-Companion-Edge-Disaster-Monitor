/*
 * skin_estimate.c
 * Dual-Sensor Skin Temperature Estimation (MAX30102 Die Temp + BME280 Ambient)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include "skin_estimate.h"
#include <string.h>
#include <math.h>

void skin_estimate_init(skin_estimate_t *st, float b0, float b1, float b2, bool is_calibrated) {
    if (!st) return;
    memset(st, 0, sizeof(*st));

    st->b0 = (b0 >= 20.0f && b0 <= 45.0f) ? b0 : SKIN_EST_DEFAULT_B0;
    st->b1 = (b1 >= 0.1f && b1 <= 5.0f) ? b1 : SKIN_EST_DEFAULT_B1;
    st->b2 = b2;
    st->tau_lead_s = SKIN_EST_DEFAULT_TAU_S;
    st->cal_min_c = SKIN_EST_MIN_REF_C;
    st->cal_max_c = SKIN_EST_MAX_REF_C;
    st->is_calibrated = is_calibrated;
}

void skin_estimate_reset(skin_estimate_t *st) {
    if (!st) return;
    st->estimated_skin_c = 0.0f;
    st->slope_c_per_sec = 0.0f;
    st->contact_start_ms = 0;
    st->contact_active = false;
    st->is_settled = false;
    st->is_valid = false;
    st->is_below_range = false;
    st->hist_count = 0;
    st->hist_idx = 0;
    memset(st->die_history, 0, sizeof(st->die_history));
    memset(st->time_history_ms, 0, sizeof(st->time_history_ms));
}

void skin_estimate_set_calibration(skin_estimate_t *st, float b0, float b1, float b2,
                                  float min_c, float max_c) {
    if (!st) return;
    st->b0 = (b0 >= 20.0f && b0 <= 45.0f) ? b0 : SKIN_EST_DEFAULT_B0;
    st->b1 = (b1 >= 0.1f && b1 <= 5.0f) ? b1 : SKIN_EST_DEFAULT_B1;
    st->b2 = b2;
    st->cal_min_c = (min_c >= 20.0f) ? min_c : SKIN_EST_MIN_REF_C;
    st->cal_max_c = (max_c <= 50.0f && max_c > min_c) ? max_c : SKIN_EST_MAX_REF_C;
    st->is_calibrated = true;
}

void skin_estimate_update(skin_estimate_t *st, float raw_die_c, float amb_c,
                          bool finger_present, uint32_t now_ms) {
    if (!st) return;

    /* Update idle die & ambient readings whenever plausible */
    if (raw_die_c >= 15.0f && raw_die_c <= 65.0f) {
        st->last_raw_die_c = raw_die_c;
        if (st->last_filtered_die_c <= 10.0f) {
            st->last_filtered_die_c = raw_die_c;
            st->idle_die_c = raw_die_c;
        } else {
            st->last_filtered_die_c = 0.70f * st->last_filtered_die_c + 0.30f * raw_die_c;
            if (!finger_present) {
                st->idle_die_c = 0.85f * st->idle_die_c + 0.15f * st->last_filtered_die_c;
            }
        }
    }
    if (amb_c >= 5.0f && amb_c <= 65.0f) {
        st->last_amb_c = amb_c;
    }
    st->last_sample_ms = now_ms;

    /* If finger is not present on MAX30102, reset contact and settling state */
    if (!finger_present) {
        skin_estimate_reset(st);
        return;
    }

    /* Start of new contact */
    if (!st->contact_active) {
        st->contact_active = true;
        st->contact_start_ms = now_ms;
        st->contact_baseline_die_c = (st->idle_die_c >= 15.0f) ? st->idle_die_c : st->last_filtered_die_c;
        st->hist_count = 0;
        st->hist_idx = 0;
    }

    /* Store in circular buffer for real-time slope evaluation */
    st->die_history[st->hist_idx] = st->last_filtered_die_c;
    st->time_history_ms[st->hist_idx] = now_ms;
    st->hist_idx = (uint8_t)((st->hist_idx + 1) % SKIN_EST_HIST_LEN);
    if (st->hist_count < SKIN_EST_HIST_LEN) {
        st->hist_count++;
    }

    /* Compute slope across recent window using true elapsed milliseconds */
    if (st->hist_count >= 3) {
        /* Use up to 8 samples (~8-9s) lookback for responsive derivative lead */
        uint8_t lookback = (st->hist_count > 8) ? 8 : (uint8_t)(st->hist_count - 1);
        uint8_t oldest_idx = (uint8_t)((st->hist_idx + SKIN_EST_HIST_LEN - 1 - lookback) % SKIN_EST_HIST_LEN);
        float oldest_val = st->die_history[oldest_idx];
        uint32_t oldest_ms = st->time_history_ms[oldest_idx];
        float dt_sec = (now_ms > oldest_ms) ? ((float)(now_ms - oldest_ms) * 0.001f) : (float)lookback;
        if (dt_sec >= 0.5f) {
            st->slope_c_per_sec = (st->last_filtered_die_c - oldest_val) / dt_sec;
        } else {
            st->slope_c_per_sec = 0.0f;
        }
    } else {
        st->slope_c_per_sec = 0.0f;
    }

    uint32_t contact_dur_ms = (now_ms >= st->contact_start_ms)
                              ? (now_ms - st->contact_start_ms)
                              : 0u;

    /* Thermal equilibrium check: >=15s contact, slope <= 0.025 C/s */
    if (contact_dur_ms >= SKIN_EST_SETTLE_MIN_MS &&
        st->hist_count >= 12 &&
        fabsf(st->slope_c_per_sec) <= SKIN_EST_MAX_SLOPE_C_PER_S) {
        st->is_settled = true;
    } else {
        st->is_settled = false;
    }

    /* Bidirectional Newton thermal lead compensation */
    float clamped_slope = fmaxf(-0.25f, fminf(st->slope_c_per_sec, 0.30f));
    float die_pred = st->last_filtered_die_c + (st->tau_lead_s * clamped_slope);

    /* Contact temperature shift relative to pre-contact baseline */
    float delta_die = die_pred - st->contact_baseline_die_c;

    /* Thermal divider fusion */
    float amb_corr = (st->last_amb_c >= 10.0f) ? (st->last_amb_c - 27.5f) : 0.0f;
    float raw_est = st->b0 + (st->b1 * delta_die) + (st->b2 * amb_corr);
    if (st->estimated_skin_c <= 10.0f) {
        st->estimated_skin_c = raw_est;
    } else {
        st->estimated_skin_c = 0.65f * st->estimated_skin_c + 0.35f * raw_est;
    }

    if (contact_dur_ms >= SKIN_EST_VALID_MIN_MS) {
        if (st->estimated_skin_c < st->cal_min_c) {
            st->is_below_range = true;
            st->is_valid = false;
        } else if (st->estimated_skin_c > st->cal_max_c) {
            st->is_below_range = false;
            st->is_valid = false;
        } else {
            st->is_below_range = false;
            st->is_valid = true;
        }
    } else {
        st->is_below_range = false;
        st->is_valid = false;
    }
}
