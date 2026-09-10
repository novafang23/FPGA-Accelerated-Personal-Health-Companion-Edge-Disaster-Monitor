/*
 * hrv_analysis.c
 * Heart Rate Variability Analysis Implementation
 */

#include "hrv_analysis.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

void hrv_init(hrv_state_t *state) {
    if (state == NULL) return;
    memset(state, 0, sizeof(hrv_state_t));
}

void hrv_add_ibi(hrv_state_t *state, float ibi_ms) {
    if (state == NULL) return;
    /* Reject non-physiological intervals. A zero or negative IBI would make
     * mean_hr = 60000/mean_ibi divide by zero and would poison both SDNN and
     * RMSSD, so bad samples are dropped at the door rather than averaged in.
     * The "!(ibi_ms > 0.0f)" form also rejects NaN. */
    if (!(ibi_ms > 0.0f) || ibi_ms > 10000.0f) {
        return;
    }
    state->ibi_ms[state->head] = ibi_ms;
    state->head = (state->head + 1) % HRV_BUFFER_SIZE;
    if (state->count < HRV_BUFFER_SIZE) {
        state->count++;
    }
}

void hrv_compute(hrv_state_t *state) {
    if (state == NULL) return;
    if (state->count < 2) {
        state->rmssd   = 0.0f;
        state->sdnn    = 0.0f;
        state->mean_hr = 0.0f;
        state->mean_ibi = 0.0f;
        return;
    }

    int n = state->count;
    int start = (state->head - n + HRV_BUFFER_SIZE) % HRV_BUFFER_SIZE;
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int idx = (start + i) % HRV_BUFFER_SIZE;
        sum += state->ibi_ms[idx];
    }
    state->mean_ibi = sum / (float)n;
    /* mean_ibi is guaranteed > 0: hrv_add_ibi() rejects non-positive intervals
     * and only values it accepted are ever counted. */
    state->mean_hr  = (state->mean_ibi > 0.0f) ? (60000.0f / state->mean_ibi) : 0.0f;

    float var_sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int idx = (start + i) % HRV_BUFFER_SIZE;
        float diff = state->ibi_ms[idx] - state->mean_ibi;
        var_sum += diff * diff;
    }
    /* SDNN is the *sample* standard deviation (N-1 denominator), which is the
     * convention used in the HRV literature. n >= 2 here, so no division by
     * zero is possible. */
    state->sdnn = sqrtf(var_sum / (float)(n - 1));

    float sd_sum  = 0.0f;
    int   sd_count = 0;
    for (int i = 1; i < n; i++) {
        int idx_prev = (start + i - 1) % HRV_BUFFER_SIZE;
        int idx_curr = (start + i)     % HRV_BUFFER_SIZE;
        float diff = state->ibi_ms[idx_curr] - state->ibi_ms[idx_prev];
        sd_sum += diff * diff;
        sd_count++;
    }
    state->rmssd = (sd_count > 0) ? sqrtf(sd_sum / (float)sd_count) : 0.0f;
}

float hrv_get_rmssd(const hrv_state_t *state)  { return state ? state->rmssd   : 0.0f; }
float hrv_get_sdnn(const hrv_state_t *state)    { return state ? state->sdnn    : 0.0f; }
float hrv_get_mean_hr(const hrv_state_t *state) { return state ? state->mean_hr : 0.0f; }
int   hrv_is_ready(const hrv_state_t *state)    { return state && state->count >= HRV_MIN_SAMPLES; }
