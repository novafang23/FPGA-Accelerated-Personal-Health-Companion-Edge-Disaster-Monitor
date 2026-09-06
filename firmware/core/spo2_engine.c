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
        .valid             = 0
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

        /* Strict Signal Quality Criteria:
         * 1. Sufficient optical DC level (finger properly covering sensor)
         * 2. Minimum pulsatile AC amplitude (true arterial pulsation, not noise)
         * 3. Physiological Perfusion Index (0.20% to 15.0%)
         */
        if (ir_dc >= SPO2_MIN_DC_IR && red_dc >= SPO2_MIN_DC_RED &&
            ir_ac >= SPO2_MIN_AC_IR && red_ac >= SPO2_MIN_AC_RED &&
            pi_ir >= SPO2_MIN_PERFUSION_INDEX && pi_ir <= SPO2_MAX_PERFUSION_INDEX) {

            float ratio_r = (red_ac / red_dc) / (ir_ac / ir_dc);
            state->ratio_r = ratio_r;

            /* Physiological R bound: human blood R is strictly between 0.35 and 1.65 */
            if (ratio_r >= SPO2_MIN_RATIO_R && ratio_r <= SPO2_MAX_RATIO_R) {
                window_valid = 1;

                /* Beer-Lambert empirical calibration curve */
                float raw_spo2 = 110.0f - 25.0f * ratio_r;

                /* Clamp to physiological clinical range [70, 100] */
                if (raw_spo2 > 100.0f) raw_spo2 = 100.0f;
                if (raw_spo2 < 70.0f)  raw_spo2 = 70.0f;

                /* Slew-rate limiting / outlier dampening to eliminate motion spikes */
                if (state->spo2_hist_count >= 2) {
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
                if (state->consecutive_valid >= SPO2_REQUIRED_VALID_WINDOWS) {
                    state->valid = 1;
                }
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