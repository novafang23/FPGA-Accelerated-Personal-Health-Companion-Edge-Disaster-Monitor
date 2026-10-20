/* Standalone test runner for pm25_correction.
 * Compile with:
 *   gcc -std=c99 -Wall -Werror -O2 test_pm25_correction.c pm25_correction.c -lm -o test_pm25
 *
 * The expectations below are taken from the published source of the
 * correction (see pm25_correction.c) and from the failure modes that the
 * previous, private calibration model showed:
 *   - a non-zero raw reading must never be published as 0 ug/m3
 *   - the humidity term must stay small (0.0862 ug/m3 per %RH)
 *   - a coarse, dust-dominated aerosol must not be lowered
 *   - the estimate must be continuous across the published band boundaries
 */
#include "pm25_correction.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int s_checks_total = 0;
static int s_checks_failed = 0;

#define CHECK(cond, message)                                      \
    do {                                                          \
        ++s_checks_total;                                         \
        if (cond) {                                               \
            printf("PASS: %s\n", message);                        \
        } else {                                                  \
            printf("FAIL: %s\n", message);                        \
            ++s_checks_failed;                                    \
        }                                                         \
    } while (0)

static float corrected(float raw, float rh)
{
    pm25_confidence_t c;
    return pm25_correct(raw, rh, 0.0f, &c);
}

int main(void)
{
    pm25_confidence_t conf;
    pm25_reading_t    reading;
    float v;
    int   i;

    /* ---- published equation, band 1 (raw < 30) ------------------------ */
    /* 0.524*20 - 0.0862*40 + 5.75 = 12.80 */
    v = corrected(20.0f, 40.0f);
    printf("raw 20, RH 40 -> %.2f ug/m3\n", v);
    CHECK(fabsf(v - 12.80f) < 0.02f, "Band 1 matches the published equation");

    /* ---- band 2 (30 <= raw < 50) -------------------------------------- */
    /* t = 40/20 - 1.5 = 0.5 -> (0.786*0.5 + 0.524*0.5)*40 - 0.0862*40 + 5.75
     *                     = 0.655*40 - 3.448 + 5.75 = 28.50 */
    v = corrected(40.0f, 40.0f);
    printf("raw 40, RH 40 -> %.2f ug/m3\n", v);
    CHECK(fabsf(v - 28.50f) < 0.05f, "Band 2 matches the published equation");

    /* ---- band 3 (50 <= raw < 210) ------------------------------------- */
    /* 0.786*100 - 0.0862*50 + 5.75 = 78.6 - 4.31 + 5.75 = 80.04 */
    v = corrected(100.0f, 50.0f);
    printf("raw 100, RH 50 -> %.2f ug/m3\n", v);
    CHECK(fabsf(v - 80.04f) < 0.05f, "Band 3 matches the published equation");

    /* ---- band 4 (210 <= raw < 260) ------------------------------------ */
    /* t = 230/50 - 4.2 = 0.4
     * = (0.69*0.4 + 0.786*0.6)*230 - 0.0862*50*0.6 + 2.966*0.4
     *   + 5.75*0.6 + 8.84e-4*230^2*0.4
     * = (0.276 + 0.4716)*230 - 2.586 + 1.1864 + 3.45 + 18.70
     * = 171.83 - 2.586 + 1.1864 + 3.45 + 18.70 = 192.58 */
    v = corrected(230.0f, 50.0f);
    printf("raw 230, RH 50 -> %.2f ug/m3\n", v);
    CHECK(fabsf(v - 192.58f) < 0.20f, "Band 4 matches the published equation");

    /* ---- band 5 (raw >= 260) ------------------------------------------ */
    /* 2.966 + 0.69*300 + 8.84e-4*300^2 = 2.966 + 207 + 79.56 = 289.53 */
    v = corrected(300.0f, 50.0f);
    printf("raw 300, RH 50 -> %.2f ug/m3\n", v);
    CHECK(fabsf(v - 289.53f) < 0.20f, "Band 5 matches the published equation");

    /* ---- continuity across the band boundaries ------------------------ */
    {
        const float edges[4] = {30.0f, 50.0f, 210.0f, 260.0f};
        int ok = 1;
        for (i = 0; i < 4; i++) {
            const float lo = corrected(edges[i] - 0.01f, 50.0f);
            const float hi = corrected(edges[i] + 0.01f, 50.0f);
            const float jump = fabsf(hi - lo);
            printf("boundary %.0f: %.3f -> %.3f (jump %.3f)\n",
                   edges[i], lo, hi, jump);
            if (jump > 0.30f) ok = 0;
        }
        CHECK(ok, "The estimate is continuous across every published band");
    }

    /* ---- the humidity term is small ----------------------------------- */
    /* 0.0862 ug/m3 per %RH -> 8.62 ug/m3 across the whole 0-100 % range */
    {
        const float d0 = corrected(100.0f, 0.0f);
        const float d1 = corrected(100.0f, 100.0f);
        const float slope = (d0 - d1) / 100.0f;
        printf("humidity slope: %.4f ug/m3 per %%RH\n", slope);
        CHECK(fabsf(slope - 0.0862f) < 0.002f,
              "The humidity term stays at the published 0.0862 ug/m3 per %RH");
        CHECK((d0 - d1) < 12.0f,
              "A 0-100 %% RH swing moves the estimate by less than 12 ug/m3");
    }

    /* ---- a non-zero reading is never published as zero ---------------- */
    for (i = 1; i <= 120; i++) {
        const float raw = (float)i * 5.0f;
        const float est = corrected(raw, 95.0f);
        if (est <= 0.0f) {
            printf("FAIL: raw %.0f at RH 95 %% published as %.3f\n", raw, est);
            CHECK(0, "A non-zero raw reading is never published as zero");
            break;
        }
        if (i == 120) {
            CHECK(1, "A non-zero raw reading is never published as zero");
        }
    }

    /* ---- a zero reading stays zero (no invented mass) ----------------- */
    v = corrected(0.0f, 40.0f);
    CHECK(v == 0.0f, "A zero raw reading is published as zero, not as a floor value");

    /* ---- negative and non-finite input ------------------------------- */
    v = pm25_correct(-5.0f, 40.0f, 0.0f, &conf);
    CHECK(v == 0.0f && conf == PM25_CONF_NO_DATA,
          "A negative raw reading is rejected as no data");

    v = pm25_correct(NAN, 40.0f, 0.0f, &conf);
    CHECK(v == 0.0f && conf == PM25_CONF_NO_DATA,
          "A non-finite raw reading is rejected as no data");

    v = pm25_correct(50.0f, NAN, 0.0f, &conf);
    CHECK(isfinite((double)v) && v > 0.0f,
          "A non-finite RH is ignored instead of poisoning the estimate");

    /* ---- the estimate is never negative ------------------------------- */
    v = corrected(5.0f, 100.0f);
    CHECK(v >= 0.0f, "The estimate is never negative");

    /* ---- dust: the correction is suppressed, the raw value is kept ---- */
    v = pm25_correct(100.0f, 50.0f, 1.6f, &conf);
    printf("raw 100, coarse fraction 1.6 -> %.2f ug/m3 (%s)\n", v,
           pm25_confidence_str(conf));
    CHECK(v == 100.0f && conf == PM25_CONF_DUST,
          "A dust-dominated aerosol keeps the sensor's own value and is flagged");

    /* A haze-like aerosol is not dust and is still corrected */
    v = pm25_correct(100.0f, 50.0f, 0.25f, &conf);
    CHECK(v < 100.0f && conf != PM25_CONF_DUST,
          "A fine-particle aerosol is corrected normally");

    /* ---- flags -------------------------------------------------------- */
    v = pm25_correct(100.0f, 85.0f, 0.2f, &conf);
    CHECK(conf == PM25_CONF_HUMIDITY, "RH >= 80 %% is flagged as a humidity caveat");

    v = pm25_correct(600.0f, 50.0f, 0.2f, &conf);
    printf("raw 600 -> %.2f ug/m3 (%s)\n", v, pm25_confidence_str(conf));
    CHECK(conf == PM25_CONF_OUT_OF_RANGE && v > 600.0f,
          "A raw value above the sensor range is extrapolated and flagged");

    v = pm25_correct(100.0f, 50.0f, 0.2f, &conf);
    CHECK(conf == PM25_CONF_OK, "An ordinary reading carries no caveat");

    /* ---- the rolling window ------------------------------------------- */
    pm25_correction_reset();
    for (i = 0; i < 30; i++) {
        pm25_correction_push((uint32_t)(i * 1000u), 20.0f, 24.0f, 40.0f, &reading);
    }
    printf("window: raw_mean %.2f, corrected %.2f, %u samples (%s)\n",
           reading.raw_mean, reading.corrected, reading.samples,
           pm25_confidence_str(reading.confidence));
    CHECK(reading.samples == 30, "The window counts every sample in range");
    CHECK(fabsf(reading.raw_mean - 20.0f) < 0.01f, "The window mean is the raw mean");
    CHECK(fabsf(reading.corrected - 12.80f) < 0.05f,
          "The published value is the correction of the window mean");
    CHECK(reading.confidence == PM25_CONF_OK, "A settled window carries no caveat");
    CHECK(fabsf(reading.coarse_frac - 0.20f) < 0.01f,
          "The coarse fraction is (PM10 - PM2.5) / PM2.5");

    /* A window that is still filling is flagged */
    pm25_correction_reset();
    for (i = 0; i < 5; i++) {
        pm25_correction_push((uint32_t)(i * 1000u), 20.0f, 24.0f, 40.0f, &reading);
    }
    CHECK(reading.confidence == PM25_CONF_SETTLING,
          "A window with fewer than 20 samples is flagged as settling");

    /* Samples older than the window drop out */
    pm25_correction_reset();
    for (i = 0; i < 30; i++) {
        pm25_correction_push((uint32_t)(i * 1000u), 20.0f, 24.0f, 40.0f, &reading);
    }
    pm25_correction_push(120000u, 20.0f, 24.0f, 40.0f, &reading);
    printf("after a 90 s gap: %u samples in window\n", reading.samples);
    CHECK(reading.samples == 1,
          "Samples older than 60 s leave the window");

    /* A single dropped read keeps the last value (the sensor is noisy and a
     * dropped frame is not a dead sensor) */
    pm25_correction_reset();
    for (i = 0; i < 30; i++) {
        pm25_correction_push((uint32_t)(i * 1000u), 20.0f, 24.0f, 40.0f, &reading);
    }
    pm25_correction_push(30000u, -1.0f, 0.0f, 40.0f, &reading);
    CHECK(reading.confidence != PM25_CONF_NO_DATA && reading.corrected > 0.0f,
          "A single dropped read does not withdraw the published value");

    /* A sensor that has gone quiet does not publish a stale number */
    pm25_correction_push(200000u, -1.0f, 0.0f, 40.0f, &reading);
    CHECK(reading.confidence == PM25_CONF_NO_DATA && reading.corrected == 0.0f &&
          reading.raw < 0.0f,
          "A silent sensor publishes no value instead of a stale one");

    /* ---- flag names --------------------------------------------------- */
    CHECK(strcmp(pm25_confidence_str(PM25_CONF_OK), "OK") == 0 &&
          strcmp(pm25_confidence_str(PM25_CONF_DUST), "DUST") == 0 &&
          strcmp(pm25_confidence_str(PM25_CONF_HUMIDITY), "HUMID") == 0 &&
          strcmp(pm25_confidence_str(PM25_CONF_OUT_OF_RANGE), "RANGE") == 0,
          "The confidence codes are stable for the wire format");

    printf("\nTest summary: %d / %d checks passed\n",
           (s_checks_total - s_checks_failed),
           s_checks_total);

    return (s_checks_failed == 0) ? 0 : 1;
}
