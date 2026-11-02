/*
 * pm25_aqi_avg.h - 24-hour PM2.5 averaging for a correct EPA AQI basis.
 *
 * WHY THIS EXISTS
 * ---------------
 * The EPA AQI for PM2.5 is defined on a 24-HOUR average (with a "NowCast"
 * variant for hourly reporting). The firmware was applying the EPA breakpoints
 * directly to a 60-second rolling mean, which is a category error: a 1-minute
 * mean is far noisier and peaks far higher than a 24-hour mean, so the reported
 * AQI was systematically inflated and jittery. The same mistake made the
 * "x times the WHO 24-hour guideline" line compare a minute against a day.
 *
 * This module accumulates a real 24-hour mean at hourly resolution, which is
 * how the standard is actually computed:
 *
 *   - 1 Hz samples are summed into the current minute bucket
 *   - at each minute boundary the minute mean is committed to an hourly
 *     accumulator
 *   - at each hour boundary the hourly mean is pushed into a 24-slot ring
 *   - the 24-hour mean is the mean of the hourly values present
 *
 * It deliberately reports how many hours are behind the mean. An AQI computed
 * from 40 minutes of data is not a 24-hour AQI, and the caller must be able to
 * say so rather than present it as one.
 */
#ifndef PM25_AQI_AVG_H
#define PM25_AQI_AVG_H

#include <stdint.h>
#include <stdbool.h>

#define PM25_AVG_HOURS          24
#define PM25_AVG_MIN_PER_HOUR   60

/* Below this many complete hours, the 24-hour mean is not reported as such.
 * EPA's NowCast needs at least 12 hours of data to be meaningful; use the same
 * floor here rather than inventing a stricter or looser one. */
#define PM25_AVG_MIN_HOURS      12

typedef struct {
    /* current minute accumulation */
    float    min_sum;
    uint32_t min_n;
    uint32_t min_start_ms;
    bool     min_started;

    /* current hour accumulation (sum of minute means) */
    float    hour_sum;
    uint32_t hour_n;

    /* completed hourly means, newest at head */
    float    hours[PM25_AVG_HOURS];
    uint8_t  hours_n;
    uint8_t  hours_head;
} pm25_aqi_avg_t;

void pm25_aqi_avg_init(pm25_aqi_avg_t *a);

/* Push one sample (call at ~1 Hz). Negative means "no reading" and is ignored,
 * so a sensor dropout does not drag the average toward clean air. */
void pm25_aqi_avg_push(pm25_aqi_avg_t *a, float pm25_ugm3, uint32_t now_ms);

/*
 * Mean over the completed hours available.
 * Returns true only when at least PM25_AVG_MIN_HOURS hours are present, so the
 * caller can distinguish "24-hour AQI" from "not enough data yet".
 * *out_hours receives how many hours are actually behind the value.
 */
bool pm25_aqi_avg_24h(const pm25_aqi_avg_t *a, float *out_mean, uint8_t *out_hours);

/* 1 when the 24-hour mean should be used for a standards-based AQI, 0 when
 * only the short-term mean is available. Convenience for the publisher. */
bool pm25_aqi_avg_ready(const pm25_aqi_avg_t *a);

#endif /* PM25_AQI_AVG_H */
