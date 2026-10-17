/*
 * pressure_trend.c — Barometric pressure trend (T3.1)
 * SIH26181 / VALOR
 *
 * See pressure_trend.h for why the trend and not the absolute value, and for the
 * window sizing.
 */

#include "pressure_trend.h"

#include <math.h>
#include <string.h>

/* Plausible sea-level-to-altitude band for a handheld field device, in hPa.
 *
 * The BME280 reports roughly 300 hPa at 9000 m and 1100 hPa at the bottom of a
 * mine. Anything outside this is a failed read or an uninitialised compensation
 * constant, and letting it into the fit would move the slope by hundreds of hPa
 * per hour and trigger a cyclone advisory in a living room. */
#define PRESSURE_MIN_VALID_HPA 300.0f
#define PRESSURE_MAX_VALID_HPA 1100.0f

void pressure_trend_init(pressure_trend_t *t) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
}

void pressure_trend_add(pressure_trend_t *t, uint32_t t_ms, float pressure_hpa) {
    if (!t) return;
    if (!isfinite(pressure_hpa)) return;
    if (pressure_hpa < PRESSURE_MIN_VALID_HPA || pressure_hpa > PRESSURE_MAX_VALID_HPA) return;

    /* Reject a timestamp that would fold the window back on itself. The MCU
     * clock is monotonic, so this only fires on a real anomaly - and if it does,
     * dropping the sample is better than producing a negative span. */
    if (t->count > 0) {
        int last = (t->head - 1 + PRESSURE_TREND_CAPACITY) % PRESSURE_TREND_CAPACITY;
        if ((int32_t)(t_ms - t->buf[last].t_ms) <= 0) return;
    }

    t->buf[t->head].pressure_hpa = pressure_hpa;
    t->buf[t->head].t_ms         = t_ms;
    t->head = (t->head + 1) % PRESSURE_TREND_CAPACITY;
    if (t->count < PRESSURE_TREND_CAPACITY) t->count++;
}

void pressure_trend_evaluate(const pressure_trend_t *t, pressure_trend_result_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!t || t->count == 0) return;

    /* Oldest-to-newest walk. The buffer is a ring, so the oldest entry is
     * count slots behind head once it is full. */
    int start = (t->head - t->count + PRESSURE_TREND_CAPACITY) % PRESSURE_TREND_CAPACITY;
    int n = t->count;
    int last_idx = (t->head - 1 + PRESSURE_TREND_CAPACITY) % PRESSURE_TREND_CAPACITY;

    /* Report what is held even when there is not enough to fit. A caller
     * debugging a stalled window needs to see "2 samples" and not zero, and
     * "no samples at all" has to stay distinguishable from "some, too few". */
    out->samples      = n;
    out->pressure_hpa = t->buf[last_idx].pressure_hpa;

    if (n < 2) return;   /* nothing to fit */

    const pressure_sample_t *first = &t->buf[start];
    const pressure_sample_t *last  = &t->buf[last_idx];

    out->span_ms  = last->t_ms - first->t_ms;
    /* Positive means the barometer fell, which is the direction that matters. */
    out->drop_hpa = first->pressure_hpa - last->pressure_hpa;

    if (out->span_ms < PRESSURE_TREND_MIN_SPAN_MS) {
        out->valid = false;   /* too short to distinguish a trend from noise */
        return;
    }

    /* Least-squares slope of pressure against time, in hPa per hour. Use
     * seconds relative to the first sample: this avoids large absolute uptime
     * values, retains millisecond-wrap behavior, and keeps the fit in float. */
    uint32_t first_ms = first->t_ms;
    float t_mean_s = 0.0f, p_mean = 0.0f;
    for (int i = 0; i < n; i++) {
        const pressure_sample_t *s = &t->buf[(start + i) % PRESSURE_TREND_CAPACITY];
        t_mean_s += (float)(uint32_t)(s->t_ms - first_ms) / 1000.0f;
        p_mean += s->pressure_hpa;
    }
    t_mean_s /= (float)n;
    p_mean /= (float)n;

    float sxx = 0.0f, sxy = 0.0f;
    for (int i = 0; i < n; i++) {
        const pressure_sample_t *s = &t->buf[(start + i) % PRESSURE_TREND_CAPACITY];
        float t_rel_s = (float)(uint32_t)(s->t_ms - first_ms) / 1000.0f;
        float dx = t_rel_s - t_mean_s;
        float dy = s->pressure_hpa - p_mean;
        sxx += dx * dx;
        sxy += dx * dy;
    }

    if (sxx <= 0.0f) {
        out->valid = false;   /* every sample shares a timestamp */
        return;
    }

    float slope_per_s = sxy / sxx;
    out->slope_hpa_per_hr = slope_per_s * 3600.0f;
    out->valid = true;
}

void pressure_trend_add_paced(pressure_trend_t *t, uint32_t t_ms,
                              float pressure_hpa, uint32_t *next_due_ms) {
    if (!t || !next_due_ms) return;

    /* First call, or the clock has passed the due time. `next_due_ms == 0` is the
     * initial state and is taken as "store now". */
    if (*next_due_ms == 0 || (int32_t)(t_ms - *next_due_ms) >= 0) {
        pressure_trend_add(t, t_ms, pressure_hpa);
        *next_due_ms = t_ms + PRESSURE_SAMPLE_INTERVAL_MS;
    }
}
