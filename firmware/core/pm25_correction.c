/*
 * pm25_correction.c - see pm25_correction.h for the rationale and the limits.
 *
 * THE PUBLISHED EQUATIONS
 * -----------------------
 * Source 1: Barkjohn, K. K., Gantt, B., and Clements, A. L. (2021),
 *   "Development and application of a United States-wide correction for
 *   PM2.5 data collected with the PurpleAir sensor", Atmos. Meas. Tech., 14,
 *   4617-4637, doi:10.5194/amt-14-4617-2021. That work established the METHOD
 *   (a linear regression with a relative-humidity term) and the magnitude of
 *   the raw bias (about 40 % over-read in most of the US; RMSE 8 -> 3 ug/m3
 *   at a mean reference of 9 ug/m3). Its coefficients are PurpleAir-specific,
 *   because the PurpleAir and the PMS5003 are not the same instrument (the
 *   PurpleAir uses a 532 nm laser, the PMS5003 a 680 nm laser with a 90
 *   scattering angle), so they are NOT used here.
 *
 * Source 2: the PMS5003 "atm" (environmental) channel form of the same
 *   algorithm, published openly by AirGradient (the largest open PMS5003
 *   collocation network) at
 *   https://www.airgradient.com/documentation/correction-algorithms/
 *   THIS is the form this firmware implements, because the PMS5003 atm
 *   channel is the one the driver reads. AirGradient measured it over 17
 *   outdoor locations in 11 countries (12,513 daily PM2.5 points): the raw
 *   data over-reads by about 50 % above 10-20 ug/m3, and the correction brings
 *   the normalised RMSE from about 79 % down to about 38 %.
 *
 *   AGraw < 30          PM2.5 = 0.524*AGraw - 0.0862*RH + 5.75
 *   30 <= AGraw < 50    PM2.5 = [0.786*(AGraw/20 - 3/2)
 *                               + 0.524*(1 - (AGraw/20 - 3/2))]*AGraw
 *                              - 0.0862*RH + 5.75
 *   50 <= AGraw < 210   PM2.5 = 0.786*AGraw - 0.0862*RH + 5.75
 *   210 <= AGraw < 260  PM2.5 = [0.69*(AGraw/50 - 21/5)
 *                               + 0.786*(1 - (AGraw/50 - 21/5))]*AGraw
 *                              - 0.0862*RH*(1 - (AGraw/50 - 21/5))
 *                              + 2.966*(AGraw/50 - 21/5)
 *                              + 5.75*(1 - (AGraw/50 - 21/5))
 *                              + 8.84e-4*AGraw^2*(AGraw/50 - 21/5)
 *   260 <= AGraw        PM2.5 = 2.966 + 0.69*AGraw + 8.84e-4*AGraw^2
 *
 * The publisher notes that negative results can occur and should be clamped
 * to 0; that is done below.
 *
 * The humidity term is small by design: 0.0862 ug/m3 per %RH, i.e. about
 * 8.6 ug/m3 across the whole 0-100 % RH range. AirGradient measured the
 * effect of humidity on the same raw value over that range as "a few
 * micrograms", which is the same order. (The previous, private calibration
 * model in this firmware used a slope 15x steeper than this and could report a
 * hazardous atmosphere as clean air; see
 * docs/reports/PM25_CALIBRATION_2026-10-20.md.)
 */
#include "pm25_correction.h"

#include <math.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/* Rolling window                                                          */
/* ---------------------------------------------------------------------- */

typedef struct {
    float    value;
    uint32_t ts;
} pm25_sample_t;

static pm25_sample_t s_win[PM25_WINDOW_SAMPLES];
static uint8_t  s_head;    /* next slot to write */
static uint16_t s_count;   /* valid entries in the ring (0..PM25_WINDOW_SAMPLES) */
static uint32_t s_last_good_ms;  /* timestamp of the newest accepted sample */

void pm25_correction_reset(void)
{
    memset(s_win, 0, sizeof(s_win));
    s_head  = 0;
    s_count = 0;
    s_last_good_ms = 0;
}

/* ---------------------------------------------------------------------- */
/* The published correction                                                 */
/* ---------------------------------------------------------------------- */

static float pm25_published(float raw, float rh)
{
    float pm;

    if (raw < 30.0f) {
        pm = 0.524f * raw - 0.0862f * rh + 5.75f;
    } else if (raw < 50.0f) {
        const float t = raw / 20.0f - 1.5f;
        pm = (0.786f * t + 0.524f * (1.0f - t)) * raw - 0.0862f * rh + 5.75f;
    } else if (raw < 210.0f) {
        pm = 0.786f * raw - 0.0862f * rh + 5.75f;
    } else if (raw < 260.0f) {
        const float t = raw / 50.0f - 4.2f;
        pm = (0.69f * t + 0.786f * (1.0f - t)) * raw
            - 0.0862f * rh * (1.0f - t)
            + 2.966f * t
            + 5.75f * (1.0f - t)
            + 8.84e-4f * raw * raw * t;
    } else {
        pm = 2.966f + 0.69f * raw + 8.84e-4f * raw * raw;
    }

    return (pm < 0.0f) ? 0.0f : pm;
}

float pm25_correct(float raw, float rh_pct, float coarse_frac,
                   pm25_confidence_t *conf)
{
    pm25_confidence_t c = PM25_CONF_OK;
    float pm;

    if (!isfinite(raw) || raw < 0.0f) {
        if (conf) *conf = PM25_CONF_NO_DATA;
        return 0.0f;
    }

    if (raw <= 0.0f) {
        /* A zero reading is below the particle counter's detection limit. The
         * publisher does not correct it and neither do we: inventing a
         * positive value from a zero reading would put mass in the air that
         * was never measured. */
        if (conf) *conf = PM25_CONF_OK;
        return 0.0f;
    }

    /* Coarse, dust-dominated aerosol: the PMS5003 UNDER-reads PM2.5 when the
     * aerosol is dominated by coarse particles (Kaur, Mangin & Kelly 2026,
     * AMT 19, 1077 - they measured a mean bias error of about 50 % under
     * dust loading, with dust identified by low RH and a high coarse-to-
     * submicron mass ratio). The published correction only ever lowers the
     * reading, so applying it in a dust event would make the estimate worse.
     * Keep the sensor's own number and flag the reading instead. */
    if (raw >= PM25_DUST_MIN_PM25 && isfinite(coarse_frac) &&
        coarse_frac >= PM25_DUST_COARSE_FRAC) {
        if (conf) *conf = PM25_CONF_DUST;
        return raw;
    }

    /* No usable RH: apply the equation without the humidity term. That
     * over-estimates rather than under-estimates, which is the safe direction
     * for a hazard device. */
    const float rh = (isfinite(rh_pct) && rh_pct >= 0.0f) ? rh_pct : 0.0f;

    pm = pm25_published(raw, rh);

    if (raw > PM25_SENSOR_MAX) {
        c = PM25_CONF_OUT_OF_RANGE;
    } else if (rh_pct >= PM25_HUMIDITY_FLAG_PCT) {
        c = PM25_CONF_HUMIDITY;
    }

    if (conf) *conf = c;
    return pm;
}

/* Flag priority: DUST > OUT_OF_RANGE > HUMIDITY > SETTLING > OK. The settling
 * flag only applies while there is no other caveat, because "the window is
 * still filling" is the least important thing to say about a reading. */
static pm25_confidence_t pm25_confidence_escalate(pm25_confidence_t c,
                                                   uint16_t samples)
{
    if (c == PM25_CONF_OK && samples < PM25_MIN_SETTLED) {
        return PM25_CONF_SETTLING;
    }
    return c;
}

const char *pm25_confidence_str(pm25_confidence_t c)
{
    switch (c) {
    case PM25_CONF_OK:           return "OK";
    case PM25_CONF_SETTLING:     return "SETTLING";
    case PM25_CONF_HUMIDITY:     return "HUMID";
    case PM25_CONF_DUST:         return "DUST";
    case PM25_CONF_OUT_OF_RANGE: return "RANGE";
    case PM25_CONF_NO_DATA:      return "NODATA";
    default:                     return "OK";
    }
}

/* ---------------------------------------------------------------------- */
/* Rolling estimate                                                        */
/* ---------------------------------------------------------------------- */

void pm25_correction_push(uint32_t now_ms, float raw_pm25, float pm10,
                          float rh_pct, pm25_reading_t *out)
{
    pm25_reading_t r;
    float    sum   = 0.0f;
    uint16_t n     = 0;
    uint16_t i;
    float    coarse = 0.0f;

    memset(&r, 0, sizeof(r));
    r.raw      = raw_pm25;
    r.rh_used  = (isfinite(rh_pct) && rh_pct >= 0.0f) ? rh_pct : -1.0f;

    if (isfinite(raw_pm25) && raw_pm25 >= 0.0f) {
        s_win[s_head].value = raw_pm25;
        s_win[s_head].ts    = now_ms;
        s_head = (uint8_t)((s_head + 1u) % PM25_WINDOW_SAMPLES);
        if (s_count < PM25_WINDOW_SAMPLES) {
            s_count++;
        }
        s_last_good_ms = now_ms;
    } else {
        /* No fresh sample. If the last good one is older than the stale
         * limit, the reading is withdrawn: a dead sensor must not keep
         * reporting the last air quality it saw. */
        if (s_count == 0 ||
            (uint32_t)(now_ms - s_last_good_ms) > PM25_STALE_MS) {
            pm25_correction_reset();
            memset(&r, 0, sizeof(r));
            r.raw     = -1.0f;
            r.samples = 0;
            r.confidence = PM25_CONF_NO_DATA;
            if (out) *out = r;
            return;
        }
    }

    /* Walk backwards from the newest entry. The ring is time-ordered, so the
     * first entry older than the window ends the average. */
    for (i = 0; i < s_count; i++) {
        const uint16_t idx =
            (uint16_t)((s_head + PM25_WINDOW_SAMPLES - 1u - i) % PM25_WINDOW_SAMPLES);
        if ((uint32_t)(now_ms - s_win[idx].ts) > PM25_WINDOW_MS) {
            break;
        }
        sum += s_win[idx].value;
        n++;
    }

    if (n == 0) {
        /* No fresh sample inside the window (sensor absent or the task was
         * stalled). Do not publish a stale number as if it were current. */
        r.raw_mean    = raw_pm25;
        r.corrected   = 0.0f;
        r.samples     = 0;
        r.confidence  = PM25_CONF_NO_DATA;
        r.coarse_frac = 0.0f;
        if (out) *out = r;
        return;
    }

    r.raw_mean = sum / (float)n;
    r.samples  = n;

    if (isfinite(pm10) && pm10 > 0.0f && r.raw_mean > 0.0f) {
        coarse = (pm10 - r.raw_mean) / r.raw_mean;
        if (coarse < 0.0f) {
            coarse = 0.0f;
        }
    }
    r.coarse_frac = coarse;

    r.corrected  = pm25_correct(r.raw_mean, r.rh_used, coarse, &r.confidence);
    r.confidence = pm25_confidence_escalate(r.confidence, n);

    if (out) *out = r;
}
