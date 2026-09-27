/*
 * pressure_trend.h — Barometric pressure trend (T3.1)
 * SIH26181 / VALOR: Personal Health Companion & Edge Disaster Monitor
 *
 * The BME280 already measures barometric pressure and the firmware already reads
 * it - and then throws it away. A falling barometer is the oldest storm
 * precursor there is, so this turns a value that is already on the I2C bus into
 * a cyclone/storm advisory at zero hardware cost.
 *
 * WHY THIS DOES NOT JUST REPORT THE INSTANTANEOUS PRESSURE
 * Absolute pressure is nearly useless for a weather advisory: sea-level pressure
 * varies by tens of hPa with altitude and with the season, so a fixed threshold
 * would fire in the hills and stay silent on the coast. What carries information
 * is the RATE OF FALL over hours, which is why this keeps a history and fits a
 * slope to it rather than comparing a number to a constant.
 *
 * This file is arithmetic only - windowing, a least-squares slope and a drop.
 * The thresholds that turn a slope into a risk level live with the other hazard
 * thresholds in disaster_risk_engine.h, so that all four hazards are calibrated
 * in one place. It has no ESP-IDF, no allocation and no I/O, so the windowing and
 * the fit are unit-tested on the host.
 */

#ifndef SHRIKEFI_PRESSURE_TREND_H
#define SHRIKEFI_PRESSURE_TREND_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Capacity is chosen so the window covers a useful span at a sane cadence.
 *
 * The BME280 is read at 1 Hz. Storing every reading would need 10,800 samples to
 * cover three hours, so the caller subsamples: one stored sample per
 * PRESSURE_SAMPLE_INTERVAL_MS. At the default 60 s cadence, 180 samples covers
 * exactly three hours in 1.4 KB.
 *
 * The caller owns the cadence because it owns the clock; this module only stores
 * what it is given and reports the span it actually has.
 */
#define PRESSURE_TREND_CAPACITY 180

/* Store one sample per minute. Callers should compare against their own next-due
 * timestamp rather than calling this at 1 Hz. */
#define PRESSURE_SAMPLE_INTERVAL_MS (60u * 1000u)

/* Below this span the slope is not a weather trend, it is sensor noise and the
 * diurnal wiggle. Reported as invalid rather than guessed at. */
#define PRESSURE_TREND_MIN_SPAN_MS (30u * 60u * 1000u)

typedef struct {
    float    pressure_hpa;
    uint32_t t_ms;
} pressure_sample_t;

typedef struct {
    pressure_sample_t buf[PRESSURE_TREND_CAPACITY];
    int               head;   /* next write index */
    int               count;  /* valid entries, saturating at capacity */
} pressure_trend_t;

typedef struct {
    bool     valid;              /* enough span to judge                   */
    float    slope_hpa_per_hr;   /* least-squares fit; negative = falling   */
    float    drop_hpa;           /* oldest pressure minus newest: + = fell  */
    uint32_t span_ms;            /* time actually covered                   */
    int      samples;            /* samples the fit used                    */
    float    pressure_hpa;       /* most recent reading                     */
} pressure_trend_result_t;

/* Reset the window. Safe to call at any time. */
void pressure_trend_init(pressure_trend_t *t);

/* Add a reading. Non-finite or out-of-range values are ignored: a BME280 that
 * returns 0 or NaN must not enter the fit and drag the slope with it. */
void pressure_trend_add(pressure_trend_t *t, uint32_t t_ms, float pressure_hpa);

/*
 * Fit a straight line to the stored samples and report the trend.
 *
 * Out-of-order timestamps are tolerated (the fit uses each sample's own
 * timestamp), but a sample with a timestamp at or before the previous one is
 * rejected on insert so a clock glitch cannot fold the window back on itself.
 */
void pressure_trend_evaluate(const pressure_trend_t *t, pressure_trend_result_t *out);

/* Convenience: adds at most one sample per PRESSURE_SAMPLE_INTERVAL_MS, so the
 * caller can call it at 1 Hz and let the module pace the window. `next_due_ms`
 * is caller-owned state, initialised to 0. */
void pressure_trend_add_paced(pressure_trend_t *t, uint32_t t_ms,
                              float pressure_hpa, uint32_t *next_due_ms);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_PRESSURE_TREND_H */
