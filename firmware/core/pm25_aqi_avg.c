/* pm25_aqi_avg.c - see header. */
#include "pm25_aqi_avg.h"
#include <math.h>
#include <string.h>

#define MS_PER_MINUTE  60000u
#define MS_PER_HOUR    3600000u

void pm25_aqi_avg_init(pm25_aqi_avg_t *a)
{
    if (!a) return;
    memset(a, 0, sizeof(*a));
}

static void commit_minute(pm25_aqi_avg_t *a)
{
    if (a->min_n == 0u) return;
    const float minute_mean = a->min_sum / (float)a->min_n;
    a->hour_sum += minute_mean;
    a->hour_n++;
    a->min_sum = 0.0f;
    a->min_n = 0u;
}

static void commit_hour(pm25_aqi_avg_t *a)
{
    if (a->hour_n == 0u) return;
    const float hour_mean = a->hour_sum / (float)a->hour_n;
    a->hours[a->hours_head] = hour_mean;
    a->hours_head = (uint8_t)((a->hours_head + 1u) % PM25_AVG_HOURS);
    if (a->hours_n < PM25_AVG_HOURS) a->hours_n++;
    a->hour_sum = 0.0f;
    a->hour_n = 0u;
}

void pm25_aqi_avg_push(pm25_aqi_avg_t *a, float pm25_ugm3, uint32_t now_ms)
{
    if (!a) return;
    /* A negative value is the "no reading" sentinel. Ignoring it matters: a
     * sensor that drops out for an hour must not pull the 24-hour mean toward
     * zero and make the air look cleaner than it was. */
    if (!isfinite(pm25_ugm3) || pm25_ugm3 < 0.0f) return;

    if (!a->min_started) {
        a->min_started = true;
        a->min_start_ms = now_ms;
    }

    /* Rolling boundaries rather than equality tests: a 1 Hz caller will never
     * land exactly on a boundary, and a caller that is delayed must still roll
     * the buckets forward rather than stalling forever. */
    while ((uint32_t)(now_ms - a->min_start_ms) >= MS_PER_MINUTE) {
        a->min_start_ms += MS_PER_MINUTE;
        commit_minute(a);
        if (a->hour_n >= PM25_AVG_MIN_PER_HOUR) commit_hour(a);
    }

    a->min_sum += pm25_ugm3;
    a->min_n++;
}

bool pm25_aqi_avg_24h(const pm25_aqi_avg_t *a, float *out_mean, uint8_t *out_hours)
{
    if (out_hours) *out_hours = 0u;
    if (out_mean) *out_mean = 0.0f;
    if (!a || a->hours_n == 0u) return false;
    if (out_hours) *out_hours = a->hours_n;

    float sum = 0.0f;
    for (uint8_t i = 0; i < a->hours_n; i++) {
        sum += a->hours[i];
    }
    if (out_mean) *out_mean = sum / (float)a->hours_n;
    return a->hours_n >= PM25_AVG_MIN_HOURS;
}

bool pm25_aqi_avg_ready(const pm25_aqi_avg_t *a)
{
    return a && a->hours_n >= PM25_AVG_MIN_HOURS;
}
