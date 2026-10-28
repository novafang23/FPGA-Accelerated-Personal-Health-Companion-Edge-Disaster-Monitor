/*
 * hrv_analysis.c
 * Heart Rate Variability Analysis Implementation
 *
 * RMSSD and SDNN follow the Task Force of the European Society of Cardiology
 * and the North American Society of Pacing and Electrophysiology, "Heart rate
 * variability: standards of measurement, physiological interpretation, and
 * clinical use", Circulation 1996;93:1043-1065.
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
    /* When the ring overwrites its oldest IBI, the next-oldest entry loses its
     * predecessor from the active window. Do not count that now-out-of-window
     * difference in RMSSD. */
    if (state->count == HRV_BUFFER_SIZE) {
        int new_oldest = (state->head + 1) % HRV_BUFFER_SIZE;
        state->pair_valid[new_oldest] = 0;
    }

    uint8_t pair_valid = 0;
    if (state->has_previous_ibi) {
        int prev_idx = (state->head - 1 + HRV_BUFFER_SIZE) % HRV_BUFFER_SIZE;
        float prev_ibi = state->ibi_ms[prev_idx];
        /* Physiological consecutive difference gate (Task Force 1996, Kamath 2012).
         * Extreme single-beat jumps (> 120 ms) reflect ectopic beats or optical motion
         * artifacts. Invalidate this transition so it does not inflate RMSSD, while
         * still allowing the beat to contribute to mean HR and count. */
        if (fabsf(ibi_ms - prev_ibi) <= HRV_MAX_DELTA_IBI_MS) {
            pair_valid = 1;
        }
    }

    state->ibi_ms[state->head] = ibi_ms;
    state->pair_valid[state->head] = pair_valid;
    state->head = (state->head + 1) % HRV_BUFFER_SIZE;
    if (state->count < HRV_BUFFER_SIZE) {
        state->count++;
    }
    state->has_previous_ibi = 1;
}

void hrv_break_sequence(hrv_state_t *state) {
    if (state == NULL) return;
    state->has_previous_ibi = 0;
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
        if (!state->pair_valid[idx_curr]) continue;
        float diff = state->ibi_ms[idx_curr] - state->ibi_ms[idx_prev];
        sd_sum += diff * diff;
        sd_count++;
    }
    state->rmssd = (sd_count > 0) ? sqrtf(sd_sum / (float)sd_count) : 0.0f;
}

float hrv_get_rmssd(const hrv_state_t *state)  { return state ? state->rmssd   : 0.0f; }
float hrv_get_sdnn(const hrv_state_t *state)    { return state ? state->sdnn    : 0.0f; }
float hrv_get_mean_hr(const hrv_state_t *state) { return state ? state->mean_hr : 0.0f; }
int hrv_is_ready(const hrv_state_t *state) {
    if (state == NULL || state->count < HRV_MIN_SAMPLES) return 0;

    /* Support summary/snapshot structs (e.g., Core 1 hrv_snapshot or host test
     * profiles) that populate rmssd/sdnn/count directly without a ring buffer. */
    if (state->head == 0 && state->count < HRV_BUFFER_SIZE && state->rmssd > 0.0f) return 1;

    int n = state->count;
    int start = (state->head - n + HRV_BUFFER_SIZE) % HRV_BUFFER_SIZE;
    int valid_pairs = 0;
    for (int i = 1; i < n; i++) {
        int idx = (start + i) % HRV_BUFFER_SIZE;
        if (state->pair_valid[idx]) valid_pairs++;
    }
    return valid_pairs >= (HRV_MIN_SAMPLES - 1);
}

/* Publication gate. Structural readiness (hrv_is_ready) is not enough: a
 * window with fewer than 10-20 beats yields a high RSA-phase variance. Wait
 * for the Ultra-Short-Term stabilization threshold (HRV_SETTLE_MIN_SAMPLES = 30
 * beats) and a window spanning at least HRV_SETTLE_MIN_SPAN_MS (20.0 s of contact). */
int hrv_is_settled(const hrv_state_t *state) {
    if (state == NULL || !hrv_is_ready(state)) return 0;

    /* Summary/snapshot structs carry no interval history to measure a span
     * against. They may publish only if they also claim the settled fill
     * count; a summary claiming a 300-beat RMSSD while carrying a 50-beat
     * count is exactly the under-filled case this gate exists to suppress. */
    if (state->head == 0 && state->count < HRV_BUFFER_SIZE && state->rmssd > 0.0f) {
        return state->count >= HRV_SETTLE_MIN_SAMPLES;
    }

    if (state->count < HRV_SETTLE_MIN_SAMPLES) return 0;

    /* Walk the window exactly as hrv_compute() does, so the fill, span and
     * contiguity checks all describe the same set of intervals. */
    int n = state->count;
    int start = (state->head - n + HRV_BUFFER_SIZE) % HRV_BUFFER_SIZE;
    int valid_pairs = 0;
    float span_ms = 0.0f;
    for (int i = 0; i < n; i++) {
        int idx = (start + i) % HRV_BUFFER_SIZE;
        span_ms += state->ibi_ms[idx];
        if (i > 0 && state->pair_valid[idx]) valid_pairs++;
    }

    /* The window must span the settling period. Summing the intervals states
     * the requirement on the window itself ("covers >= 120 s of contact")
     * rather than on a wall clock, so it cannot drift from the sample data and
     * it is testable without a timer. Rejected intervals break the sequence and
     * are not added to the window, so this counts contiguous beats only. */
    if (span_ms < HRV_SETTLE_MIN_SPAN_MS) return 0;

    return valid_pairs >= (HRV_MIN_SAMPLES - 1);
}

/* ---------------------------------------------------------------------------
 * Causal median filter over the IBI series (see hrv_analysis.h for rationale).
 * ------------------------------------------------------------------------- */

void hrv_median_init(hrv_median_t *m) {
    if (m == NULL) return;
    memset(m, 0, sizeof(*m));
}

/* Insertion sort of at most HRV_MEDIAN_WINDOW floats, then take the middle.
 * For an even count n/2 selects the upper of the two middle values. The median
 * is a control input to the ratio gate, not a published IBI, so this small
 * upward bias is acceptable; it slightly favours rejecting short intervals,
 * which is the split-beat side. */
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

