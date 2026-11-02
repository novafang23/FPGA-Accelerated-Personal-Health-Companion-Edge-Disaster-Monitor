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
    /* Peripheral thermal state belongs to a contact, so it resets with one. */
    st->pt_baseline_c = 0.0f;
    st->pt_trend_c = 0.0f;
    st->pt_rate_c_per_min = 0.0f;
    st->pt_ring_n = 0u;
    st->pt_ring_head = 0u;
    st->pt_state = SKIN_THERM_UNKNOWN;
    st->pt_valid = false;
    st->pt_pending = SKIN_THERM_UNKNOWN;
    st->pt_pending_n = 0u;
    st->pt_amb_baseline_c = 0.0f;
    st->pt_trend_amb_c = 0.0f;
    memset(st->pt_ring, 0, sizeof(st->pt_ring));
    memset(st->pt_ring_ms, 0, sizeof(st->pt_ring_ms));
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

bool skin_estimate_set_led_offset(skin_estimate_t *st, float offset_c) {
    if (!st) return false;
    /* A physically plausible emitter self-heating step is a few degrees. A
     * negative or huge value means the probe moved between the two reads (or
     * a finger touched it), so the measurement is discarded rather than
     * trusted: a wrong offset silently biases every later reading. */
    if (!(offset_c >= SKIN_EST_LED_OFFSET_MIN_C && offset_c <= SKIN_EST_LED_OFFSET_MAX_C)) {
        return false;
    }
    st->led_offset_c = offset_c;
    st->led_offset_valid = true;
    return true;
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

        /* Baseline = the die temperature the finger has to beat.
         *
         * This used to be `fmaxf(raw_base, amb + 5.5)`. The 5.5 was a guess at
         * how much the MAX30102 heats itself with the LEDs running, and on
         * hardware it was larger than the real self-heating (+3.7 C measured),
         * so the guard exceeded the die reading the finger ever produced:
         * delta clamped to 0 and the thermometer contributed nothing at all.
         *
         * Now:
         *  - if the boot-time LED offset was MEASURED, baseline is
         *    ambient + that measured offset, which is what the die reads with
         *    no finger;
         *  - otherwise use the die's own idle reading. It already contains
         *    self-heating, so it is a valid "no finger" reference; the result
         *    is flagged weak_signal so the UI can say the reading is not
         *    die-driven rather than silently publishing a constant.
         *
         * Either way the number is derived from what the sensor measured, never
         * from a fabricated ambient margin. */
        if (st->led_offset_valid && st->last_amb_c >= 5.0f) {
            st->contact_baseline_die_c = st->last_amb_c + st->led_offset_c;
        } else {
            st->contact_baseline_die_c = raw_base;
            st->weak_signal = true;
        }
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
        /* Normal / warm finger path.
         *
         * This USED to be a strict peak-hold: target came from
         * contact_peak_die_c, which only ever rises, and the estimate was only
         * ever allowed to climb. Combined with the cooling path's requirement
         * that the die fall BELOW the idle baseline (-0.80 C), that made a
         * perfectly ordinary event invisible:
         *
         *     idle die 29.06 C, finger-on die 30.07 C
         *     -> the finger WARMS the die, so it never goes below idle
         *     -> the cooling branch never fires
         *     -> the peak never falls, so the estimate stays pinned
         *     -> pt_trend_c ~ 0 forever and COOLING was UNREACHABLE.
         *
         * A warm finger that starts cooling is the common case (cold stress
         * starting during a session), so the warm path now tracks the PRESENT
         * die reading rather than its peak. Movement is still bounded on both
         * directions so sensor noise cannot make the published number jump,
         * and the median filter upstream already removes single-sample spikes.
         *
         * A slow release (FALL_RATE) rather than the rise rate is deliberate:
         * skin sheds heat slower than the model converges, so following a
         * falling die too eagerly would over-report cooling. */
        float peak_delta = fmaxf(0.0f, fminf(st->contact_peak_die_c - st->contact_baseline_die_c, 3.80f));
        float pres_delta = fmaxf(0.0f, fminf(st->last_filtered_die_c - st->contact_baseline_die_c, 3.80f));
        /* Use the present value, but let a genuinely large peak still count
         * for the weak-signal diagnostic so the page can still say "the die
         * barely moved, this number is the model constant". */
        st->die_delta_c = st->contact_peak_die_c - st->contact_baseline_die_c;
        st->weak_signal = (peak_delta < 0.30f);

        target_c = st->b0 + (st->b1 * pres_delta) + (st->b2 * amb_corr);
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
        } else if (diff < 0.0f) {
            /* Bounded decay. Without this the number can only ratchet upward,
             * so a cooling finger is invisible no matter how long it cools. */
            float step = SKIN_EST_FALL_FRACTION * diff;   /* negative */
            if (step < -SKIN_EST_MAX_FALL_PER_SEC) {
                step = -SKIN_EST_MAX_FALL_PER_SEC;
            }
            st->estimated_skin_c += step;
            if (st->estimated_skin_c < target_c) {
                st->estimated_skin_c = target_c;
            }
        }
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

    /* Peripheral thermal state. Only meaningful once the estimate is both
     * settled and in range: before that the "trend" is just the estimator
     * walking to its own equilibrium, not the subject's periphery moving. */
    skin_estimate_update_thermal(st, now_ms);
}

const char *skin_therm_state_name(skin_therm_state_t s) {
    switch (s) {
        case SKIN_THERM_STABLE:          return "STABLE";
        case SKIN_THERM_COOLING:         return "COOLING";
        case SKIN_THERM_REWARMING:       return "REWARMING";
        case SKIN_THERM_VASOCONSTRICTED: return "VASOCONSTRICTED";
        case SKIN_THERM_UNKNOWN:
        default:                         return "UNKNOWN";
    }
}

/* Least-squares slope (C per minute) over the ring buffer. Using a fit rather
 * than (newest - oldest) makes the rate insensitive to a single bad sample,
 * which matters because this drives a cold-stress decision. */
static float pt_ring_rate_c_per_min(const skin_estimate_t *st, float *trend_out) {
    const uint8_t n = st->pt_ring_n;
    if (n < SKIN_EST_PT_MIN_SAMPLES) {
        if (trend_out) *trend_out = 0.0f;
        return 0.0f;
    }

    /* Oldest sample is at head when the ring is full, else at index 0. */
    const uint8_t oldest = (n >= SKIN_EST_PT_RING) ? st->pt_ring_head : 0u;
    const uint32_t t_ref = st->pt_ring_ms[oldest];

    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    float newest = 0.0f;
    uint8_t counted = 0u;
    for (uint8_t i = 0; i < n; i++) {
        const uint8_t idx = (uint8_t)((oldest + i) % SKIN_EST_PT_RING);
        /* Seconds since the oldest sample. Wrapping-safe unsigned subtraction. */
        const double x = (double)(st->pt_ring_ms[idx] - t_ref) / 1000.0;
        const double y = (double)st->pt_ring[idx];
        sx += x; sy += y; sxx += x * x; sxy += x * y;
        newest = st->pt_ring[idx];
        counted++;
    }
    if (counted < SKIN_EST_PT_MIN_SAMPLES) {
        if (trend_out) *trend_out = 0.0f;
        return 0.0f;
    }

    if (trend_out) *trend_out = newest - st->pt_baseline_c;

    const double den = (double)counted * sxx - sx * sx;
    if (fabs(den) < 1e-9) return 0.0f;   /* all samples at the same instant */
    const double slope_per_s = ((double)counted * sxy - sx * sy) / den;
    return (float)(slope_per_s * 60.0);
}

void skin_estimate_update_thermal(skin_estimate_t *st, uint32_t now_ms) {
    if (!st) return;

    /* Liveness, not display range and not a momentary settle flicker.
     *
     * Two gates were wrong here and together made the state machine blind in
     * exactly the situations it exists to report:
     *
     *  1. `is_valid` additionally demands the reading sit inside
     *     [cal_min_c, cal_max_c] (~31-36.8 C), a DISPLAY bound. A genuinely
     *     cold fingertip reaches ~26.5 C on the cooling path and is then
     *     marked is_valid=false / is_below_range=true, so a cooling finger
     *     reported UNKNOWN forever.
     *  2. `is_settled` is recomputed each tick from |target - estimate| <=
     *     0.35 C. While a finger is actively warming or cooling, the estimate
     *     legitimately lags its moving target, so is_settled flickers false
     *     and the trend ring -- the very thing that detects movement -- was
     *     flushed mid-event.
     *
     * Track whenever contact is present and we have a past the 12 s settle
     * window. A reading below range is reported as VASOCONSTRICTED by the
     * floor check rather than suppressing the state entirely. */
    const uint32_t dur_ms = (now_ms >= st->contact_start_ms)
                            ? (now_ms - st->contact_start_ms) : 0u;
    const bool live = st->contact_active &&
                      dur_ms >= SKIN_EST_SETTLE_MIN_MS &&
                      st->estimated_skin_c > 0.0f &&
                      st->estimated_skin_c < SKIN_EST_MAX_REF_C;
    if (!live) {
        /* Drop the ring so a new contact cannot inherit the last one's
         * baseline. Trend/state go back to "we know nothing yet". */
        st->pt_state = SKIN_THERM_UNKNOWN;
        st->pt_valid = false;
        st->pt_trend_c = 0.0f;
        st->pt_rate_c_per_min = 0.0f;
        st->pt_ring_n = 0u;
        st->pt_ring_head = 0u;
        return;
    }

    /* Freeze both baselines the first time we become live, so the ambient
     * component is measured from the same instant as the skin baseline. */
    if (st->pt_ring_n == 0u) {
        st->pt_baseline_c = st->estimated_skin_c;
        st->pt_amb_baseline_c = st->last_amb_c;
    }

    st->pt_ring[st->pt_ring_head] = st->estimated_skin_c;
    st->pt_ring_ms[st->pt_ring_head] = now_ms;
    st->pt_ring_head = (uint8_t)((st->pt_ring_head + 1u) % SKIN_EST_PT_RING);
    if (st->pt_ring_n < SKIN_EST_PT_RING) st->pt_ring_n++;

    float raw_trend = 0.0f;
    float rate = pt_ring_rate_c_per_min(st, &raw_trend);

    /* ---- Ambient common-mode rejection --------------------------------
     * A room that cools drags the finger with it, so the raw trend alone
     * cannot tell "the environment changed" from "the subject constricted".
     * The BME280 observes the common-mode part; remove its coupling so what
     * remains is the differential signal that belongs to the subject.
     *
     * Bounded so a bad/absent ambient reading can never manufacture a trend:
     * beyond SKIN_EST_PT_AMB_MAX_C the swing is treated as a sensor fault or
     * a moved probe, and no correction is applied at all. */
    float amb_delta = 0.0f;
    if (st->last_amb_c > 5.0f && st->last_amb_c < 50.0f) {
        amb_delta = st->last_amb_c - st->pt_amb_baseline_c;
        if (amb_delta > SKIN_EST_PT_AMB_MAX_C)  amb_delta = SKIN_EST_PT_AMB_MAX_C;
        if (amb_delta < -SKIN_EST_PT_AMB_MAX_C) amb_delta = -SKIN_EST_PT_AMB_MAX_C;
    }
    const float amb_component = amb_delta * SKIN_EST_PT_COUPLING;
    const float trend = raw_trend - amb_component;

    /* Clamp to what skin can physically do. The estimator slews at up to
     * 0.35 C/s chasing its target; publishing that as a skin rate would claim
     * 21 C/min, which no fingertip under a probe can achieve. Clamping keeps
     * the DIRECTION (which is what the state depends on) while refusing to
     * report a magnitude that is an artefact of the model. */
    if (rate > SKIN_EST_PT_RATE_MAX_C_MIN)  rate = SKIN_EST_PT_RATE_MAX_C_MIN;
    if (rate < -SKIN_EST_PT_RATE_MAX_C_MIN) rate = -SKIN_EST_PT_RATE_MAX_C_MIN;
    /* The rate is a slope over the SAME ring the trend came from, so the
     * ambient component is already present in it exactly once. Do not subtract
     * it again here -- that would double-count the correction and bias every
     * reported rate toward "no trend". */
    st->pt_trend_amb_c = amb_component;
    st->pt_trend_c = trend;
    st->pt_rate_c_per_min = rate;
    st->pt_valid = (st->pt_ring_n >= SKIN_EST_PT_MIN_SAMPLES);

    /* ---- Classify the instantaneous candidate ------------------------- */
    skin_therm_state_t want;
    if (!st->pt_valid) {
        /* Baseline frozen, but not enough samples yet to claim a direction. */
        want = SKIN_THERM_STABLE;
    } else if (st->estimated_skin_c <= SKIN_EST_PT_COLD_ABS_C || st->is_below_range) {
        /* Absolute floor wins over trend: a finger at 27 C is shut down
         * regardless of which way it is drifting. Fires for a settled reading
         * below the display range too -- that is the coldest case of all. */
        want = SKIN_THERM_VASOCONSTRICTED;
    } else if (trend <= SKIN_EST_PT_COOLING_C && rate <= -SKIN_EST_PT_RATE_MIN_C_MIN) {
        want = SKIN_THERM_COOLING;
    } else if (trend >= SKIN_EST_PT_REWARMING_C && rate >= SKIN_EST_PT_RATE_MIN_C_MIN) {
        want = SKIN_THERM_REWARMING;
    } else {
        want = SKIN_THERM_STABLE;
    }

    /* ---- Hysteresis ---------------------------------------------------
     * A single noisy sample must not flip the published label; the OLED and
     * the web tile would chatter during an ordinary contact. A new state is
     * published only once it has held for SKIN_EST_PT_DWELL consecutive ticks.
     * VASOCONSTRICTED is exempt: it is a safety-relevant floor and must not be
     * delayed by a dwell timer. */
    if (want == SKIN_THERM_VASOCONSTRICTED) {
        st->pt_state = want;
        st->pt_pending = want;
        st->pt_pending_n = 0u;
        return;
    }
    if (want == st->pt_state) {
        st->pt_pending_n = 0u;
        st->pt_pending = want;
        return;
    }
    if (want == st->pt_pending) {
        if (st->pt_pending_n < 0xFFu) st->pt_pending_n++;
    } else {
        st->pt_pending = want;
        st->pt_pending_n = 1u;
    }
    if (st->pt_pending_n >= SKIN_EST_PT_DWELL) {
        st->pt_state = want;
        st->pt_pending_n = 0u;
    }
}
