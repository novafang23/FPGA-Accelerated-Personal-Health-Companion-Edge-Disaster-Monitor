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

/* ---------------------------------------------------------------------------
 * Causal median filter over the IBI series (see hrv_analysis.h for rationale).
 * ------------------------------------------------------------------------- */

void hrv_median_init(hrv_median_t *m) {
    if (m == NULL) return;
    memset(m, 0, sizeof(*m));
}

/* Insertion sort of at most HRV_MEDIAN_WINDOW (5) floats, then take the middle.
 * For an even count n/2 selects the upper of the two middle values; the filter
 * output feeds HRV statistics only and never feeds back into the rejection
 * rule, so that bias is harmless. */
static float hrv_median_of(const float *v, int n) {
    float s[HRV_MEDIAN_WINDOW];
    for (int i = 0; i < n; i++) s[i] = v[i];
    for (int i = 1; i < n; i++) {
        float key = s[i];
        int   j   = i - 1;
        while (j >= 0 && s[j] > key) { s[j + 1] = s[j]; j--; }
        s[j + 1] = key;
    }
    return s[n / 2];
}

float hrv_median_push(hrv_median_t *m, float ibi_ms) {
    if (m == NULL) return ibi_ms;

    m->buf[m->head] = ibi_ms;
    m->head = (m->head + 1) % HRV_MEDIAN_WINDOW;
    if (m->count < HRV_MEDIAN_WINDOW) m->count++;

    /* Not enough history to know what "normal" is yet: pass the value through
     * rather than replacing a real measurement with a guess. */
    if (m->count < HRV_MEDIAN_MIN) return ibi_ms;
    return hrv_median_of(m->buf, m->count);
}

float hrv_median_value(const hrv_median_t *m) {
    /* Deliberately NOT "count < HRV_MEDIAN_WINDOW". Waiting for a full window
     * would leave the first ~10 intervals of every session judged on absolute
     * bounds alone, and an artefact landing there is accepted and then pinned
     * into the HRV series. A partly-filled window is safe to act on here
     * because the caller feeds it with every plausible interval, accepted or
     * rejected, so a wrong early estimate corrects itself as data arrives. */
    if (m == NULL || m->count < HRV_MEDIAN_MIN_REF) return 0.0f;
    return hrv_median_of(m->buf, m->count);
}

