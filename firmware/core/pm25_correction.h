/*
 * pm25_correction.h - PMS5003 PM2.5 correction for the ShrikeFi/VALOR device.
 *
 * WHY THIS MODULE EXISTS
 * -----------------------
 * The PMS5003 reports an optical particle count and converts it to a mass
 * concentration with an assumed particle density and size. That assumption is
 * wrong in a way that is well documented: raw PMS5003 readings overestimate
 * ambient PM2.5 by roughly 40-60 % (about 40 % for the PurpleAir in most of
 * the US - Barkjohn, Gantt & Clements 2021, AMT 14, 4617-4637; and about 50 %
 * for the PMS5003 above 10-20 ug/m3 - AirGradient, 12,513 daily collocation
 * points across 17 outdoor locations in 11 countries).
 * The overestimate is partly a fixed optical bias and partly a humidity
 * effect: hygroscopic growth of the aerosol at high relative humidity scatters
 * more laser light, so the sensor reads high (Hua et al. 2021; Tejada et al.
 * 2022).
 *
 * We therefore correct the reading - but with the *published* correction, not
 * with a private model:
 *
 *   - US EPA / Barkjohn, Gantt & Clements (2021), "Development and
 *     application of a United States-wide correction for PM2.5 data collected
 *     with the PurpleAir sensor", Atmos. Meas. Tech. 14, 4617-4637,
 *     doi:10.5194/amt-14-4617-2021. This work established the method and the
 *     size of the bias. Its coefficients are PurpleAir-specific and are NOT
 *     used here, because the PurpleAir (532 nm laser) and the PMS5003 (680 nm
 *     laser, 90 deg scattering angle) are different instruments.
 *   - The PMS5003 "atm" (environmental) channel form of that algorithm, as
 *     published openly by AirGradient at
 *     https://www.airgradient.com/documentation/correction-algorithms/
 *     (they run the largest open PMS5003 collocation network in the world).
 *     THIS is the form implemented here, because the atm channel is the one
 *     the driver reads.
 *
 * The equations are reproduced in pm25_correction.c next to their source.
 * They are fitted on averaged data, so the estimate here is computed from a
 * rolling mean of the raw reading rather than from a single 1 s sample.
 *
 * HONEST LIMITS (see docs/reports/PM25_CALIBRATION_2026-10-20.md)
 * ----------------------------------------------------------------
 *  - The correction was fitted on ambient aerosol in North America and
 *    Europe. AirGradient measured it improving accuracy in 14 of 17 outdoor
 *    sites and making it worse in 3 (Sydney: reference noise at low
 *    concentrations; Bogota: the reference instrument itself over-reads;
 *    Guatemala City: unverified reference). We have no collocation reference
 *    instrument, so we cannot validate the correction locally; we can only
 *    inherit the published performance.
 *  - The relative humidity we feed in comes from the BME280 (enclosure air),
 *    not from the sensor's own hygrometer, which the published fit used. The
 *    published coefficient is 0.0862 ug/m3 per %RH, so even a 20-point
 *    difference in the RH source moves the estimate by under 2 ug/m3 - an
 *    order of magnitude below the published residual error of the correction
 *    itself.
 *  - Under elevated dust loading the PMS5003 *under*-reads PM2.5 (Kaur,
 *    Mangin & Kelly 2026, AMT 19, 1077), i.e. the published correction makes
 *    that case worse. pm25_correct() detects a coarse, dust-dominated aerosol
 *    and suppresses the correction there instead.
 */
#ifndef PM25_CORRECTION_H
#define PM25_CORRECTION_H

#include <stdbool.h>
#include <stdint.h>

/* Rolling window for the published value. The PMS5003 is noisy per sample and
 * the published correction was fitted on averaged data, so the estimate is
 * computed from a rolling mean, not from a single 1 s reading. 60 s keeps the
 * response to a genuine change in air quality within a minute while removing
 * the short-term scatter. */
#define PM25_WINDOW_MS          60000u
#define PM25_WINDOW_SAMPLES     64u
/* Samples needed inside the window before the estimate is called settled. */
#define PM25_MIN_SETTLED        20u
/* A reading older than this is no longer published. The sensor can fail
 * silently (it keeps its last value on the bus), and a stale number read as
 * "the air is still clean" is the dangerous failure for a hazard device. */
#define PM25_STALE_MS           10000u

/* Coarse-aerosol (dust) detection: coarse fraction = (PM10 - PM2.5)/PM2.5.
 * 1.0 means "PM10 is at least twice PM2.5", which separates dust-dominated
 * aerosol (coarse fraction well above 1) from smoke and haze (near 0.1-0.3).
 * This is a heuristic, in the same class as the PPG signal-quality index. */
#define PM25_DUST_COARSE_FRAC   1.0f
#define PM25_DUST_MIN_PM25      35.0f

/* PMS5003 specified PM2.5 range is 0-500 ug/m3. */
#define PM25_SENSOR_MAX         500.0f

/* Above this RH the published correction is doing the most work (it removes
 * the hygroscopic over-read), so the residual uncertainty is largest and the
 * reading is flagged. */
#define PM25_HUMIDITY_FLAG_PCT  80.0f

typedef enum {
    PM25_CONF_OK = 0,        /* corrected, no caveat                        */
    PM25_CONF_SETTLING,      /* fewer than PM25_MIN_SETTLED samples in window*/
    PM25_CONF_HUMIDITY,      /* RH >= 80 %: residual hygroscopic uncertainty */
    PM25_CONF_DUST,          /* coarse/dust aerosol: correction suppressed  */
    PM25_CONF_OUT_OF_RANGE,  /* raw above the sensor's specified range      */
    PM25_CONF_NO_DATA        /* no usable sensor reading                    */
} pm25_confidence_t;

typedef struct {
    float    raw;          /* latest instantaneous PMS5003 atm-channel value  */
    float    raw_mean;     /* rolling mean of raw inside the window          */
    float    corrected;    /* published estimate of the true PM2.5           */
    float    rh_used;      /* RH actually used by the correction             */
    float    coarse_frac;  /* (PM10 - PM2.5) / PM2.5                         */
    uint16_t samples;      /* samples inside the rolling window              */
    pm25_confidence_t confidence;
} pm25_reading_t;

/* Drop the rolling window (call after a sensor reset or a gap in sampling). */
void pm25_correction_reset(void);

/* Feed one raw sample and get the published estimate.
 * now_ms    : monotonic milliseconds
 * raw_pm25  : PMS5003 pm2_5_atm channel, ug/m3 (0 if the sensor is absent)
 * pm10      : PMS5003 pm10_atm channel, ug/m3 (0 if unavailable)
 * rh_pct    : relative humidity used for the humidity term, or a negative
 *             value if no usable RH reading is available
 */
void pm25_correction_push(uint32_t now_ms, float raw_pm25, float pm10,
                          float rh_pct, pm25_reading_t *out);

/* The published correction itself, exposed for testing.
 * Returns the estimated true PM2.5 concentration in ug/m3. */
float pm25_correct(float raw_pm25, float rh_pct, float coarse_frac,
                   pm25_confidence_t *conf);

/* Short human-readable code for the confidence flag (wire and UI). */
const char *pm25_confidence_str(pm25_confidence_t c);

#endif /* PM25_CORRECTION_H */
