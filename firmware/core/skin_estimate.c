/*
 * skin_estimate.c
 * Dual-Sensor Skin Temperature Estimation (MAX30102 Die Temp + BME280 Ambient)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include "skin_estimate.h"
#include <string.h>
#include <math.h>

static float median_of_buf(const float *buf, uint8_t count) {
    float sorted[SKIN_EST_MED_LEN];
    uint8_t n = (count <= SKIN_EST_MED_LEN) ? count : SKIN_EST_MED_LEN;
    if (n == 0u) return 0.0f;

    for (uint8_t i = 0u; i < n; i++) {
        sorted[i] = buf[i];
    }
    for (uint8_t i = 1u; i < n; i++) {
        float key = sorted[i];
        int j = (int)i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
    return sorted[n / 2u];
}

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
    st->contact_peak_die_c = 0.0f;
    st->contact_start_ms = 0u;
    st->contact_active = false;
    st->is_settled = false;
    st->is_valid = false;
    st->is_below_range = false;
    st->hist_count = 0u;
    st->hist_idx = 0u;
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

    if (amb_c >= 5.0f && amb_c <= 65.0f) {
        st->last_amb_c = amb_c;
    }

    /* Update 5-sample median and low-pass filtered die temperature */
    if (raw_die_c >= 15.0f && raw_die_c <= 65.0f) {
        st->last_raw_die_c = raw_die_c;
        st->med_buf[st->med_idx] = raw_die_c;
        st->med_idx = (uint8_t)((st->med_idx + 1u) % SKIN_EST_MED_LEN);
        if (st->med_count < SKIN_EST_MED_LEN) {
            st->med_count++;
        }
        float med_die_c = median_of_buf(st->med_buf, st->med_count);

        if (st->last_filtered_die_c <= 10.0f) {
            st->last_filtered_die_c = med_die_c;
            st->idle_die_c = med_die_c;
        } else {
            st->last_filtered_die_c = 0.65f * st->last_filtered_die_c + 0.35f * med_die_c;
            if (!finger_present) {
                st->idle_die_c = 0.85f * st->idle_die_c + 0.15f * st->last_filtered_die_c;
            }
        }
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
        float raw_base = (st->idle_die_c >= 15.0f) ? st->idle_die_c : st->last_filtered_die_c;
        /* Guard against cold-boot LED self-heating: steady MAX30102 idle die temp
         * with active LEDs sits ~5.5 C above ambient room temperature. */
        float min_led_idle = (st->last_amb_c >= 10.0f) ? (st->last_amb_c + 5.5f) : raw_base;
        st->contact_baseline_die_c = fmaxf(raw_base, min_led_idle);
        st->contact_peak_die_c = st->last_filtered_die_c;
        st->estimated_skin_c = SKIN_EST_INITIAL_TOUCH_C;
        st->hist_count = 0u;
        st->hist_idx = 0u;
    }

    /* Track peak median-filtered die temperature during this contact session */
    if (st->last_filtered_die_c > st->contact_peak_die_c) {
        st->contact_peak_die_c = st->last_filtered_die_c;
    }

    /* Store in circular buffer for diagnostic slope tracking */
    st->die_history[st->hist_idx] = st->last_filtered_die_c;
    st->time_history_ms[st->hist_idx] = now_ms;
    st->hist_idx = (uint8_t)((st->hist_idx + 1u) % SKIN_EST_HIST_LEN);
    if (st->hist_count < SKIN_EST_HIST_LEN) {
        st->hist_count++;
    }

    if (st->hist_count >= 3u) {
        uint8_t lookback = (st->hist_count > 8u) ? 8u : (uint8_t)(st->hist_count - 1u);
        uint8_t oldest_idx = (uint8_t)((st->hist_idx + SKIN_EST_HIST_LEN - 1u - lookback) % SKIN_EST_HIST_LEN);
        float oldest_val = st->die_history[oldest_idx];
        uint32_t oldest_ms = st->time_history_ms[oldest_idx];
        float dt_sec = (now_ms > oldest_ms) ? ((float)(now_ms - oldest_ms) * 0.001f) : (float)lookback;
        st->slope_c_per_sec = (dt_sec >= 0.5f) ? ((st->last_filtered_die_c - oldest_val) / dt_sec) : 0.0f;
    } else {
        st->slope_c_per_sec = 0.0f;
    }

    uint32_t contact_dur_ms = (now_ms >= st->contact_start_ms)
                              ? (now_ms - st->contact_start_ms)
                              : 0u;

    float amb_corr = (st->last_amb_c >= 10.0f) ? (st->last_amb_c - 27.5f) : 0.0f;
    float actual_idle_c = (st->idle_die_c >= 15.0f) ? st->idle_die_c : st->contact_baseline_die_c;
    float cooling_delta = st->last_filtered_die_c - actual_idle_c;
    float target_c;

    if (cooling_delta < -0.80f) {
        /* Cold-finger / hypothermia cooling path: finger actively cools the die below actual idle */
        float cold_delta = fmaxf(-5.0f, cooling_delta);
        target_c = st->b0 + (1.80f * cold_delta) + (st->b2 * amb_corr);
        float diff = target_c - st->estimated_skin_c;
        if (diff < 0.0f) {
            float step = fmaxf(-0.35f, 0.30f * diff);
            st->estimated_skin_c += step;
        }
    } else {
        /* Normal / warm finger path: strictly non-decreasing peak-hold integrator */
        float peak_delta = fmaxf(0.0f, fminf(st->contact_peak_die_c - st->contact_baseline_die_c, 3.80f));
        target_c = st->b0 + (st->b1 * peak_delta) + (st->b2 * amb_corr);
        target_c = fmaxf(33.40f, fminf(target_c, SKIN_EST_EQUIL_MAX_C));

        float diff = target_c - st->estimated_skin_c;
        if (diff > 0.0f) {
            float step = 0.26f * diff;
            if (diff > 0.10f && step < 0.07f) {
                step = 0.07f;
            }
            if (step > SKIN_EST_MAX_RISE_PER_SEC) {
                step = SKIN_EST_MAX_RISE_PER_SEC;
            }
            if (st->estimated_skin_c + step > target_c) {
                st->estimated_skin_c = target_c;
            } else {
                st->estimated_skin_c += step;
            }
        }
        /* Strictly non-decreasing: never subtract when diff <= 0.0f */
    }

    if (contact_dur_ms >= SKIN_EST_SETTLE_MIN_MS &&
        st->hist_count >= 12u &&
        fabsf(target_c - st->estimated_skin_c) <= 0.35f) {
        st->is_settled = true;
    } else {
        st->is_settled = false;
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
