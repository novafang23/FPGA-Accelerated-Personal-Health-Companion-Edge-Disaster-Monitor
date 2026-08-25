/*
 * hrv_analysis.c — Heart Rate Variability Analysis Implementation
 * SIH26181: AI-Powered Personal Health Companion
 *
 * Medical reference:
 *   RMSSD < 20 ms  → High sympathetic stress (heat exhaustion, fatigue)
 *   RMSSD 20-50 ms → Normal range
 *   RMSSD > 50 ms  → Strong parasympathetic tone (relaxed state)
 *
 *   SDNN < 50 ms   → Reduced overall HRV (autonomic dysfunction risk)
 *   SDNN 50-100 ms → Healthy range
 */

#include "hrv_analysis.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

void hrv_init(hrv_state_t *state) {
    memset(state, 0, sizeof(hrv_state_t));
}

void hrv_add_ibi(hrv_state_t *state, float ibi_ms) {
    state->ibi_ms[state->head] = ibi_ms;
    state->head = (state->head + 1) % HRV_BUFFER_SIZE;
    if (state->count < HRV_BUFFER_SIZE) {
        state->count++;
    }
}

void hrv_compute(hrv_state_t *state) {
    if (state->count < 2) {
        state->rmssd   = 0.0f;
        state->sdnn    = 0.0f;
        state->mean_hr = 0.0f;
        state->mean_ibi = 0.0f;
        return;
    }

    int n = state->count;
    /* Start index of the oldest valid entry in the circular buffer */
    int start = (state->head - n + HRV_BUFFER_SIZE) % HRV_BUFFER_SIZE;

    /* ---- Mean IBI ---- */
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int idx = (start + i) % HRV_BUFFER_SIZE;
        sum += state->ibi_ms[idx];
    }
    state->mean_ibi = sum / (float)n;
    state->mean_hr  = 60000.0f / state->mean_ibi;

    /* ---- SDNN: Standard Deviation of NN intervals ---- */
    float var_sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int idx = (start + i) % HRV_BUFFER_SIZE;
        float diff = state->ibi_ms[idx] - state->mean_ibi;
        var_sum += diff * diff;
    }
    state->sdnn = sqrtf(var_sum / (float)n);

    /* ---- RMSSD: Root Mean Square of Successive Differences ---- */
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

float hrv_get_rmssd(const hrv_state_t *state)  { return state->rmssd;   }
float hrv_get_sdnn(const hrv_state_t *state)    { return state->sdnn;    }
float hrv_get_mean_hr(const hrv_state_t *state) { return state->mean_hr; }
int   hrv_is_ready(const hrv_state_t *state)    { return state->count >= 3; }
