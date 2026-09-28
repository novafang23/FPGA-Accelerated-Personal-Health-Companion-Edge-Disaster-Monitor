/*
 * spo2_engine.c
 * Pulse Oximetry (SpO2) Estimation Implementation
 */
#include "spo2_engine.h"
#include <stdint.h>

void spo2_init(spo2_state_t *state) {
    *state = (spo2_state_t){
        .red_min           = UINT32_MAX,
        .red_max           = 0,
        .ir_min            = UINT32_MAX,
        .ir_max            = 0,
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
        .last_spread       = 0.0f,
        .last_settled      = 0
    };
    for(int i = 0; i < SPO2_MA_FILTER_SIZE; i++) {
        state->spo2_history[i] = 0.0f;
    }
}

void spo2_add_samples(spo2_state_t *state, uint32_t red_filtered,
                      uint32_t ir_filtered) {
    /* Track min/max within the current measurement window */
    if (red_filtered < state->red_min)
        state->red_min = red_filtered;
    if (red_filtered > state->red_max)
        state->red_max = red_filtered;
    if (ir_filtered < state->ir_min)
        state->ir_min = ir_filtered;
    if (ir_filtered > state->ir_max)
        state->ir_max = ir_filtered;

    state->sample_count++;

    /* Compute SpO2 at end of each measurement window */
    if (state->sample_count >= SPO2_WINDOW_SIZE) {
        float red_ac = (float)(state->red_max - state->red_min);
        float red_dc = (float)(state->red_max + state->red_min) / 2.0f;
        float ir_ac  = (float)(state->ir_max - state->ir_min);
        float ir_dc  = (float)(state->ir_max + state->ir_min) / 2.0f;

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
                    float lo = 1e9f, hi = -1e9f;
                    for (int k = 0; k < SPO2_STABLE_MIN_WINDOWS; k++) {
                        int idx = (state->spo2_hist_idx - 1 - k + 2 * SPO2_MA_FILTER_SIZE)
                                  % SPO2_MA_FILTER_SIZE;
                        float h = state->spo2_history[idx];
                        if (h < lo) lo = h;
                        if (h > hi) hi = h;
                    }
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

        /* Reset window trackers for next measurement */
        state->sample_count = 0;
        state->red_min = UINT32_MAX;
        state->red_max = 0;
        state->ir_min = UINT32_MAX;
        state->ir_max = 0;
    }
}

float spo2_get_value(const spo2_state_t *state) { return state->spo2; }

float spo2_get_perfusion_index(const spo2_state_t *state) { return state->perfusion_index; }

int spo2_is_valid(const spo2_state_t *state) { return state->valid; }