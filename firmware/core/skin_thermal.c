/* skin_thermal.c - peripheral thermal state from a measured skin temperature.
 * Extracted from the removed die-based estimator; see the header for why. */
#include "skin_thermal.h"
#include <math.h>
#include <string.h>

void skin_thermal_init(skin_thermal_t *t)
{
    if (!t) return;
    memset(t, 0, sizeof(*t));
    t->state = SKIN_THERM_UNKNOWN;
    t->pending = SKIN_THERM_UNKNOWN;
}

void skin_thermal_reset(skin_thermal_t *t)
{
    if (!t) return;
    t->baseline_c = 0.0f;
    t->trend_c = 0.0f;
    t->rate_c_per_min = 0.0f;
    t->amb_baseline_c = 0.0f;
    t->trend_amb_c = 0.0f;
    t->ring_n = 0u;
    t->ring_head = 0u;
    t->state = SKIN_THERM_UNKNOWN;
    t->pending = SKIN_THERM_UNKNOWN;
    t->pending_n = 0u;
    t->valid = false;
    t->contact_start_ms = 0u;
    t->contact_active = false;
    memset(t->ring, 0, sizeof(t->ring));
    memset(t->ring_ms, 0, sizeof(t->ring_ms));
}

const char *skin_therm_state_name(skin_therm_state_t s)
{
    switch (s) {
        case SKIN_THERM_STABLE:          return "STABLE";
        case SKIN_THERM_COOLING:         return "COOLING";
        case SKIN_THERM_REWARMING:       return "REWARMING";
        case SKIN_THERM_VASOCONSTRICTED: return "VASOCONSTRICTED";
        case SKIN_THERM_UNKNOWN:
        default:                         return "UNKNOWN";
    }
}

/* Least-squares slope (C/min) over the ring, plus the raw trend. */
static float ring_rate(const skin_thermal_t *t, float *trend_out)
{
    const uint8_t n = t->ring_n;
    if (n < SKIN_THERM_MIN_SAMPLES) {
        if (trend_out) *trend_out = 0.0f;
        return 0.0f;
    }
    const uint8_t oldest = (n >= SKIN_THERM_RING_SIZE) ? t->ring_head : 0u;
    const uint32_t t_ref = t->ring_ms[oldest];

    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    float newest = 0.0f;
    uint8_t counted = 0u;
    for (uint8_t i = 0; i < n; i++) {
        const uint8_t idx = (uint8_t)((oldest + i) % SKIN_THERM_RING_SIZE);
        const double x = (double)(t->ring_ms[idx] - t_ref) / 1000.0;
        const double y = (double)t->ring[idx];
        sx += x; sy += y; sxx += x * x; sxy += x * y;
        newest = t->ring[idx];
        counted++;
    }
    if (counted < SKIN_THERM_MIN_SAMPLES) {
        if (trend_out) *trend_out = 0.0f;
        return 0.0f;
    }
    if (trend_out) *trend_out = newest - t->baseline_c;

    const double den = (double)counted * sxx - sx * sx;
    if (fabs(den) < 1e-9) return 0.0f;
    return (float)(((double)counted * sxy - sx * sy) / den * 60.0);
}

void skin_thermal_update(skin_thermal_t *t, float skin_c, float amb_c,
                         bool contact, uint32_t now_ms)
{
    if (!t) return;

    /* Liveness = real reading + contact held past the settle window. A reading
     * outside the display range is NOT treated as "no data": a genuinely cold
     * finger is the case this exists to report. */
    if (!contact) {
        t->contact_active = false;
        t->contact_start_ms = 0u;
    } else if (!t->contact_active) {
        t->contact_active = true;
        t->contact_start_ms = now_ms ? now_ms : 1u;
    }

    const uint32_t dur = (t->contact_active && now_ms >= t->contact_start_ms)
                         ? (now_ms - t->contact_start_ms) : 0u;
    const bool live = t->contact_active && skin_c > 0.0f &&
                      dur >= SKIN_THERM_SETTLE_MS;

    if (!live) {
        t->state = SKIN_THERM_UNKNOWN;
        t->valid = false;
        t->trend_c = 0.0f;
        t->rate_c_per_min = 0.0f;
        t->ring_n = 0u;
        t->ring_head = 0u;
        return;
    }

    if (t->ring_n == 0u) {
        t->baseline_c = skin_c;
        t->amb_baseline_c = amb_c;
    }

    t->ring[t->ring_head] = skin_c;
    t->ring_ms[t->ring_head] = now_ms;
    t->ring_head = (uint8_t)((t->ring_head + 1u) % SKIN_THERM_RING_SIZE);
    if (t->ring_n < SKIN_THERM_RING_SIZE) t->ring_n++;

    float raw_trend = 0.0f;
    float rate = ring_rate(t, &raw_trend);

    /* Common-mode rejection: a room that cools drags the finger with it, so the
     * raw trend cannot separate "the environment changed" from "the subject
     * constricted". Remove the ambient component; bound it so a bad ambient
     * reading cannot manufacture a trend on its own. */
    float amb_delta = 0.0f;
    if (amb_c > 5.0f && amb_c < 50.0f) {
        amb_delta = amb_c - t->amb_baseline_c;
        if (amb_delta >  SKIN_THERM_AMB_MAX_C) amb_delta =  SKIN_THERM_AMB_MAX_C;
        if (amb_delta < -SKIN_THERM_AMB_MAX_C) amb_delta = -SKIN_THERM_AMB_MAX_C;
    }
    const float amb_component = amb_delta * SKIN_THERM_AMB_COUPLING;
    const float trend = raw_trend - amb_component;

    if (rate >  SKIN_THERM_RATE_MAX_C_MIN) rate =  SKIN_THERM_RATE_MAX_C_MIN;
    if (rate < -SKIN_THERM_RATE_MAX_C_MIN) rate = -SKIN_THERM_RATE_MAX_C_MIN;

    t->trend_amb_c = amb_component;
    t->trend_c = trend;
    t->rate_c_per_min = rate;
    t->valid = (t->ring_n >= SKIN_THERM_MIN_SAMPLES);

    skin_therm_state_t want;
    if (!t->valid) {
        want = SKIN_THERM_STABLE;
    } else if (skin_c <= SKIN_THERM_COLD_ABS_C) {
        want = SKIN_THERM_VASOCONSTRICTED;
    } else if (trend <= SKIN_THERM_COOLING_C && rate <= -SKIN_THERM_RATE_MIN_C_MIN) {
        want = SKIN_THERM_COOLING;
    } else if (trend >= SKIN_THERM_REWARMING_C && rate >= SKIN_THERM_RATE_MIN_C_MIN) {
        want = SKIN_THERM_REWARMING;
    } else {
        want = SKIN_THERM_STABLE;
    }

    /* Hysteresis: a single noisy sample must not flip the published label.
     * VASOCONSTRICTED is exempt - it is safety-relevant and must not wait. */
    if (want == SKIN_THERM_VASOCONSTRICTED) {
        t->state = want;
        t->pending = want;
        t->pending_n = 0u;
        return;
    }
    if (want == t->state) {
        t->pending_n = 0u;
        t->pending = want;
        return;
    }
    if (want == t->pending) {
        if (t->pending_n < 0xFFu) t->pending_n++;
    } else {
        t->pending = want;
        t->pending_n = 1u;
    }
    if (t->pending_n >= SKIN_THERM_DWELL) {
        t->state = want;
        t->pending_n = 0u;
    }
}
