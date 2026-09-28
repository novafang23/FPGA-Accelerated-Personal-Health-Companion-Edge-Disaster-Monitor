/*
 * spo2_engine.c
 * Pulse Oximetry (SpO2) Estimation Implementation
 */
#include "spo2_engine.h"
#include <stdint.h>
#include <math.h>

void spo2_init(spo2_state_t *state) {
    *state = (spo2_state_t){
        .red_sum           = 0.0,
        .red_sumsq         = 0.0,
        .ir_sum            = 0.0,
        .ir_sumsq          = 0.0,
        .red_ewma_mean     = 0.0,
        .red_ewma_var      = 0.0,
        .ir_ewma_mean      = 0.0,
        .ir_ewma_var       = 0.0,
        .ewma_started      = 0,
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
    /* Per-window sums, used for the DC and as a fallback. */
    state->red_sum   += (double)red_filtered;
    state->red_sumsq += (double)red_filtered * (double)red_filtered;
    state->ir_sum    += (double)ir_filtered;
    state->ir_sumsq  += (double)ir_filtered * (double)ir_filtered;

    /* Rolling mean and variance over ~2 s (SPO2_AC_TAU_SAMPLES).
     *
     * West's incremental form: the mean is updated first, then the variance from
     * the same deviation, which keeps it numerically well behaved and needs no
     * sample buffer. The point is that the amplitude no longer depends on where in
     * the cardiac cycle the 0.5 s reporting window starts - see the note in the
     * header for the measurement that showed this. */
    const double alpha = 1.0 / (double)SPO2_AC_TAU_SAMPLES;
    double rx = (double)red_filtered;
    double ix = (double)ir_filtered;

    if (!state->ewma_started) {
        state->red_ewma_mean = rx;
        state->ir_ewma_mean  = ix;
        state->red_ewma_var  = 0.0;
        state->ir_ewma_var   = 0.0;
        state->ewma_started  = 1;
    } else {
        double rd = rx - state->red_ewma_mean;
        state->red_ewma_mean += alpha * rd;
        state->red_ewma_var   = (1.0 - alpha) * (state->red_ewma_var + alpha * rd * rd);

        double id = ix - state->ir_ewma_mean;
        state->ir_ewma_mean += alpha * id;
        state->ir_ewma_var   = (1.0 - alpha) * (state->ir_ewma_var + alpha * id * id);
    }

    state->sample_count++;

    /* Compute SpO2 at end of each measurement window */
    if (state->sample_count >= SPO2_WINDOW_SIZE) {
        /* DC and AC both come from the rolling estimate, so neither depends on
         * the window's phase within the cardiac cycle. */
        const double PP_FROM_SIGMA = 2.0 * 1.4142135623730951;
        float red_dc = (float)state->red_ewma_mean;
        float ir_dc  = (float)state->ir_ewma_mean;
        float red_ac = (float)(PP_FROM_SIGMA * sqrt(state->red_ewma_var));
        float ir_ac  = (float)(PP_FROM_SIGMA * sqrt(state->ir_ewma_var));

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
        if (ir_dc < SPO2_MIN_DC_IR || red_dc < SPO2_MIN_DC_RED) {
            state->last_reject = 1;
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

                /* Beer-Lambert empirical calibration curve */
                float raw_spo2 = 110.0f - 25.0f * ratio_r;

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

                state->consecutive_valid++;

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
                    float lo = h[1];
                    float hi = h[n - 2];
                    settled = ((hi - lo) <= SPO2_STABLE_SPREAD_PCT);
                    state->last_spread = hi - lo;
                }
                state->last_settled = settled;

                if (state->consecutive_valid >= SPO2_REQUIRED_VALID_WINDOWS && settled) {
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
        state->red_sum   = 0.0;
        state->red_sumsq = 0.0;
        state->ir_sum    = 0.0;
        state->ir_sumsq  = 0.0;
    }
}

float spo2_get_value(const spo2_state_t *state) { return state->spo2; }

float spo2_get_perfusion_index(const spo2_state_t *state) { return state->perfusion_index; }

int spo2_is_valid(const spo2_state_t *state) { return state->valid; }