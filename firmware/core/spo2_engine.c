/*
 * spo2_engine.c
 * Pulse Oximetry (SpO2) Estimation Implementation
 */
#include "spo2_engine.h"
#include <stdint.h>
#include <math.h>

void spo2_init(spo2_state_t *state) {
    *state = (spo2_state_t){
        .red_mean          = 0.0f,
        .red_m2            = 0.0f,
        .ir_mean           = 0.0f,
        .ir_m2             = 0.0f,
        .sample_count      = 0,
        .ratio_r           = 0.0f,
        .perfusion_index   = 0.0f,
        .consecutive_valid = 0,
        .spo2_hist_idx     = 0,
        .spo2_hist_count   = 0,
        .spo2              = 0.0f,
        .valid             = 0,
        .windows_total     = 0,
        .windows_valid     = 0,
        .last_reject       = 0,
        .last_ir_dc        = 0.0f,
        .last_ir_ac        = 0.0f,
        .last_red_ac       = 0.0f,
        .last_red_dc       = 0.0f,
        .last_spread       = 0.0f,
        .last_spread_raw   = 0.0f,
        .last_settled      = 0
    };
    for(int i = 0; i < SPO2_MA_FILTER_SIZE; i++) {
        state->spo2_history[i] = 0.0f;
    }
}

void spo2_add_samples(spo2_state_t *state, uint32_t red_filtered,
                      uint32_t ir_filtered) {
    /* Welford's online variance avoids subtracting two nearly equal large
     * values (E[x^2] - E[x]^2), which loses the small pulsatile AC component
     * when the optical DC level is tens of thousands of counts. It also keeps
     * the ESP32-S3 target on its native single-precision FPU. */
    state->sample_count++;
    float n = (float)state->sample_count;
    float red = (float)red_filtered;
    float ir = (float)ir_filtered;
    float red_delta = red - state->red_mean;
    state->red_mean += red_delta / n;
    state->red_m2 += red_delta * (red - state->red_mean);
    float ir_delta = ir - state->ir_mean;
    state->ir_mean += ir_delta / n;
    state->ir_m2 += ir_delta * (ir - state->ir_mean);

    /* Compute SpO2 at end of each measurement window */
    if (state->sample_count >= SPO2_WINDOW_SIZE) {
        float red_var = state->red_m2 / n;
        float ir_var  = state->ir_m2 / n;
        if (red_var < 0.0f) red_var = 0.0f;
        if (ir_var  < 0.0f) ir_var  = 0.0f;

        /* 2*sqrt(2)*sigma is the peak-to-peak of an equivalent sinusoid. Both
         * channels use the same estimator, so their RATIO - which is all R is -
         * stays unbiased and the calibration curve below still applies. */
        const float PP_FROM_SIGMA = 2.0f * 1.41421356f;
        float red_ac = PP_FROM_SIGMA * sqrtf(red_var);
        float ir_ac  = PP_FROM_SIGMA * sqrtf(ir_var);
        float red_dc = state->red_mean;
        float ir_dc  = state->ir_mean;

        /* Calculate Perfusion Index: PI = (AC / DC) * 100% */
        float pi_ir = (ir_dc > 0.0f) ? ((ir_ac / ir_dc) * 100.0f) : 0.0f;
        state->perfusion_index = pi_ir;

        int window_valid = 0;

        /* Record what this window actually measured, so a slow acquisition can be
         * attributed to a specific gate rather than guessed at. */
        state->windows_total++;
        state->last_ir_dc  = ir_dc;
        state->last_ir_ac  = ir_ac;
        state->last_red_ac = red_ac;
        state->last_red_dc = red_dc;
        state->last_reject = 0;

        /* Strict Signal Quality Criteria, checked one at a time so a rejection
         * can be attributed to the specific gate that caused it:
         * 1. Sufficient optical DC level (finger properly covering sensor)
         * 2. Minimum pulsatile AC amplitude (true arterial pulsation, not noise)
         * 3. Physiological Perfusion Index (0.15% to 15.0%)
         * 4. Physiological ratio R (0.35 to 1.65)
         *
         * Recording which one failed costs nothing here and saves guessing: the
         * acquisition gate below is deliberately slow to satisfy, and without
         * this there is no way to tell a sensor still settling from a finger
         * that is not perfusing. */
        /* Check for active finger placement or seating movement.
         * If the DC baseline jumped by >15% since the last window, the finger is
         * still physically compressing or shifting on the optical sensor. */
        int dc_stable = 1;
        if (state->prev_ir_dc > 0.0f) {
            float dc_step = fabsf(ir_dc - state->prev_ir_dc) / state->prev_ir_dc;
            if (dc_step > 0.15f) {
                dc_stable = 0;
            }
        }
        state->prev_ir_dc = ir_dc; /* remember THIS window for the next comparison */

        if (ir_dc < SPO2_MIN_DC_IR || red_dc < SPO2_MIN_DC_RED) {
            state->last_reject = 1;
        } else if (!dc_stable) {
            state->last_reject = 1; /* Unsettled finger seating transient */
            if (!state->valid) {
                state->consecutive_valid = 0;
                state->spo2_hist_count = 0;
                state->spo2_hist_idx = 0;
            }
        } else if (ir_ac < SPO2_MIN_AC_IR || red_ac < SPO2_MIN_AC_RED) {
            state->last_reject = 2;
        } else if (pi_ir < SPO2_MIN_PERFUSION_INDEX || pi_ir > SPO2_MAX_PERFUSION_INDEX) {
            state->last_reject = 3;
        } else {

            float ratio_r = (red_ac / red_dc) / (ir_ac / ir_dc);
            state->ratio_r = ratio_r;

            /* Physiological R bound: human blood R is strictly between 0.35 and 1.65 */
            if (ratio_r >= SPO2_MIN_RATIO_R && ratio_r <= SPO2_MAX_RATIO_R) {
                window_valid = 1;
                state->windows_valid++;

                /* Empirical calibration curve with the 2-point sensor calibration
                 * applied (see SPO2_CAL_INTERCEPT / SPO2_CAL_SLOPE). */
                float raw_spo2 = SPO2_CAL_INTERCEPT - SPO2_CAL_SLOPE * ratio_r;

                /* Clamp to physiological clinical range [70, 100] */
                if (raw_spo2 > 100.0f) raw_spo2 = 100.0f;
                if (raw_spo2 < 70.0f)  raw_spo2 = 70.0f;

                /* Slew-rate limiting / outlier dampening, to reject motion spikes.
                 *
                 * Skipped until the first value has been published. The limiter
                 * compares against state->spo2, the running mean, so during
                 * acquisition it clamps every sample to (mean + 2) and the mean
                 * then chases its own clamp - convergence becomes a slow crawl.
                 * Measured on a re-acquisition ramp: 28 windows to a trustworthy
                 * value with the limiter always on, 8 with it gated. There is no
                 * established reading to protect while acquiring, so applying it
                 * there buys nothing and delays first SpO2 by ~20 seconds.
                 *
                 * Once valid, it is back on and does its job: damping the motion
                 * spikes it was written for. */
                if (state->valid && state->spo2_hist_count >= 2) {
                    float max_step = 2.0f; /* Max 2% change per 1-second window */
                    if (raw_spo2 > state->spo2 + max_step) {
                        raw_spo2 = state->spo2 + max_step;
                    } else if (raw_spo2 < state->spo2 - max_step) {
                        raw_spo2 = state->spo2 - max_step;
                    }
                }

                /* Add to moving average history */
                state->spo2_history[state->spo2_hist_idx] = raw_spo2;
                state->spo2_hist_idx = (state->spo2_hist_idx + 1) % SPO2_MA_FILTER_SIZE;
                if (state->spo2_hist_count < SPO2_MA_FILTER_SIZE) {
                    state->spo2_hist_count++;
                }

                /* Calculate average */
                float sum = 0.0f;
                for (int i = 0; i < state->spo2_hist_count; i++) {
                    sum += state->spo2_history[i];
                }
                state->spo2 = sum / (float)state->spo2_hist_count;

                if (state->consecutive_valid < SPO2_REQUIRED_VALID_WINDOWS) {
                    state->consecutive_valid++;
                }

                /* Has the smoothed estimate stopped moving?
                 *
                 * Compared over the most recent few entries rather than the
                 * whole 8-window history: the slew limiter below allows 2% per
                 * window, so a value still climbing at full rate spans 16%
                 * across a full history and would never look flat. The last few
                 * windows capture the thing that matters - whether it is still
                 * converging. Once `valid` latches it is never un-set by this
                 * test, so a genuine desaturation still tracks and displays. */
                int settled = 0;
                if (state->spo2_hist_count >= SPO2_STABLE_MIN_WINDOWS) {
                    float h[SPO2_MA_FILTER_SIZE];
                    int n = 0;
                    for (int k = 0; k < SPO2_STABLE_MIN_WINDOWS; k++) {
                        int idx = (state->spo2_hist_idx - 1 - k + 2 * SPO2_MA_FILTER_SIZE)
                                  % SPO2_MA_FILTER_SIZE;
                        h[n++] = state->spo2_history[idx];
                    }
                    /* Plain max-min, kept for the diagnostic: this is what the
                     * gate used to test. */
                    float raw_lo = h[0], raw_hi = h[0];
                    for (int i = 1; i < n; i++) {
                        if (h[i] < raw_lo) raw_lo = h[i];
                        if (h[i] > raw_hi) raw_hi = h[i];
                    }
                    state->last_spread_raw = raw_hi - raw_lo;

                    /* Detect active acquisition drift / transient ramp.
                     * In a genuine transient (e.g. LED settling or finger placement),
                     * readings drift monotonically by > 0.80% each window.
                     * A settled signal oscillates gently around the physiological mean. */
                    int monotonic_drift = 0;
                    if (n >= 3) {
                        int rising  = (h[0] > h[1] + 0.80f) && (h[1] > h[2] + 0.80f);
                        int falling = (h[0] < h[1] - 0.80f) && (h[1] < h[2] - 0.80f);
                        monotonic_drift = (rising || falling);
                    }

                    /* Insertion sort (n = 8) so the extremes can be dropped. */
                    for (int i = 1; i < n; i++) {
                        float key = h[i];
                        int j = i - 1;
                        while (j >= 0 && h[j] > key) { h[j + 1] = h[j]; j--; }
                        h[j + 1] = key;
                    }

                    /* Spread with the single lowest and highest entry discarded.
                     *
                     * The gate has to tell "still converging" from "settled", and
                     * max-min cannot: it is decided by the single worst PAIR of
                     * entries, so one noisy window blocks latching until that
                     * entry falls out of the ring - up to 8 windows, 4 s, for one
                     * outlier.
                     *
                     * Measured on hardware: 14 consecutive valid windows, twice
                     * the 8 required, held back only by a 3.38% spread against the
                     * 2% bar - and that spread collapsed below the bar within one
                     * second. A signal still converging does not do that; an
                     * outlier leaving the buffer at the end of its 8-window life
                     * does exactly that.
                     *
                     * Dropping one entry from each end still catches a genuine
                     * ramp, because a converging estimate is monotonic and the
                     * span barely changes when its extremes are removed. What it
                     * no longer does is let a single bad window veto the reading
                     * for four seconds. */
                    float lo, hi;
                    if (n >= 5) {
                        lo = h[1];
                        hi = h[n - 2];
                        float trimmed_sum = 0.0f;
                        for (int i = 1; i < n - 1; i++) trimmed_sum += h[i];
                        state->spo2 = trimmed_sum / (float)(n - 2);
                    } else {
                        lo = h[0];
                        hi = h[n - 1];
                        float full_sum = 0.0f;
                        for (int i = 0; i < n; i++) full_sum += h[i];
                        state->spo2 = full_sum / (float)n;
                    }
                    settled = (!monotonic_drift && ((hi - lo) <= SPO2_STABLE_SPREAD_PCT));
                    state->last_spread = hi - lo;
                }
                state->last_settled = settled;

                if (state->consecutive_valid >= SPO2_REQUIRED_VALID_WINDOWS && settled) {
                    if (!state->valid) {
                        /* When validity first latches, re-seed the moving average with the settled
                         * reading so pre-settling transient entries do not drag the average down. */
                        for (int i = 0; i < SPO2_MA_FILTER_SIZE; i++) {
                            state->spo2_history[i] = state->spo2;
                        }
                        state->spo2_hist_count = SPO2_MA_FILTER_SIZE;
                    }
                    state->valid = 1;
                }
            } else {
                state->last_reject = 4;
            }
        }

        if (!window_valid) {
            /* Poor signal, touching too gently, or finger lifted */
            if (ir_dc < 800.0f || ir_ac < 4.0f) {
                /* Finger off or severe loss of contact -> flush history instantly */
                state->consecutive_valid = 0;
                state->valid = 0;
                state->spo2_hist_count = 0;
                state->spo2_hist_idx = 0;
                state->spo2 = 0.0f;
            } else {
                /* Momentary noise/tremor: degrade confidence slowly for a grace period */
                if (state->consecutive_valid > 0) {
                    state->consecutive_valid--;
                }
                if (state->consecutive_valid == 0) {
                    state->valid = 0;
                }
            }
        }

        /* Reset window accumulators for next measurement */
        state->sample_count = 0;
        state->red_mean = 0.0f;
        state->red_m2   = 0.0f;
        state->ir_mean  = 0.0f;
        state->ir_m2    = 0.0f;
    }
}

float spo2_get_value(const spo2_state_t *state) { return state->spo2; }

float spo2_get_perfusion_index(const spo2_state_t *state) { return state->perfusion_index; }

int spo2_is_valid(const spo2_state_t *state) { return state->valid; }
