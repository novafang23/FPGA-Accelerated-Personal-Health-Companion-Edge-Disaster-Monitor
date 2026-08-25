/*
 * spo2_engine.c — Pulse Oximetry (SpO2) Estimation Implementation
 * SIH26181: AI-Powered Personal Health Companion
 *
 * Clinical reference:
 *   SpO2 >= 95%  → Normal
 *   SpO2 92-94%  → Mild hypoxemia (monitor closely during pollution events)
 *   SpO2 88-91%  → Moderate hypoxemia (medical attention recommended)
 *   SpO2 < 88%   → Severe hypoxemia (emergency)
 */

#include "spo2_engine.h"

void spo2_init(spo2_state_t *state) {
  *state = (spo2_state_t){
      .red_min      = 255,
      .red_max      = 0,
      .ir_min       = 255,
      .ir_max       = 0,
      .sample_count = 0,
      .ratio_r      = 0.0f,
      .spo2         = 0.0f,
      .valid        = 0
  };
}

void spo2_add_samples(spo2_state_t *state, uint8_t red_filtered,
                      uint8_t ir_filtered) {
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
    state->red_min = 255;
    state->red_max = 0;
    state->ir_min = 255;
    state->ir_max = 0;
  }
}

float spo2_get_value(const spo2_state_t *state) { return state->spo2; }

int spo2_is_valid(const spo2_state_t *state) { return state->valid; }
