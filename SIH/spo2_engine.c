/*
 * spo2_engine.c
 * Pulse Oximetry (SpO2) Estimation Implementation
 */
#include "spo2_engine.h"
#include <stdint.h>

void spo2_init(spo2_state_t *state) {
    *state = (spo2_state_t){
        .red_min      = UINT32_MAX,
        .red_max      = 0,
        .ir_min       = UINT32_MAX,
        .ir_max       = 0,
        .sample_count = 0,
        .ratio_r      = 0.0f,
        .spo2         = 0.0f,
        .valid        = 0
    };
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
        float ir_ac = (float)(state->ir_max - state->ir_min);
        float ir_dc = (float)(state->ir_max + state->ir_min) / 2.0f;

        if (ir_ac > 0.0f && ir_dc > 0.0f && red_dc > 0.0f) {
            state->ratio_r = (red_ac / red_dc) / (ir_ac / ir_dc);

            /* Beer-Lambert empirical calibration curve */
            state->spo2 = 110.0f - 25.0f * state->ratio_r;

            /* Clamp to physiological range [0, 100] */
            if (state->spo2 > 100.0f)
                state->spo2 = 100.0f;
            if (state->spo2 < 0.0f)
                state->spo2 = 0.0f;

            state->valid = 1;
        } else {
            state->valid = 0;
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

int spo2_is_valid(const spo2_state_t *state) { return state->valid; }