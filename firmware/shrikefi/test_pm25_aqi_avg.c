/* Tests for the 24-hour PM2.5 averaging used for a standards-correct EPA AQI. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pm25_aqi_avg.h"

static int fails = 0;
static void ck(int cond, const char *what) {
    printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) fails++;
}

#define MIN_MS  60000u
#define HR_MS   3600000u

/* Feed `seconds` of 1 Hz samples at a constant value. */
static void feed(pm25_aqi_avg_t *a, float v, uint32_t seconds, uint32_t *t) {
    for (uint32_t i = 0; i < seconds; i++) {
        *t += 1000u;
        pm25_aqi_avg_push(a, v, *t);
    }
}

int main(void) {
    pm25_aqi_avg_t a;
    float mean = -1.0f;
    uint8_t hours = 0;

    printf("24-hour PM2.5 averaging tests\n=============================\n");

    /* 1. Empty is not ready and reports nothing ------------------------- */
    pm25_aqi_avg_init(&a);
    ck(!pm25_aqi_avg_ready(&a), "empty buffer is not ready");
    ck(!pm25_aqi_avg_24h(&a, &mean, &hours), "empty does not claim a 24-hour mean");
    ck(hours == 0, "reports zero hours");

    /* 2. A few minutes is NOT a 24-hour AQI ----------------------------- */
    uint32_t t = 0;
    feed(&a, 100.0f, 40 * 60, &t);      /* 40 minutes */
    pm25_aqi_avg_24h(&a, &mean, &hours);
    ck(!pm25_aqi_avg_ready(&a), "40 minutes is not a 24-hour basis");
    ck(hours < PM25_AVG_MIN_HOURS, "reports fewer hours than the floor");

    /* 3. 11 hours still not ready; 12 complete hours is ------------------
     * NOTE: the bucket only counts COMPLETE hours. Feeding exactly 12 h of
     * samples leaves the 12th hour still being filled, so 11 are complete.
     * That is correct - an incomplete hour must not be averaged in - so the
     * test has to feed past the boundary to reach 12 complete hours. */
    pm25_aqi_avg_init(&a); t = 0;
    feed(&a, 50.0f, 11 * 3600, &t);
    ck(!pm25_aqi_avg_ready(&a), "11 hours is below the 12-hour floor");
    feed(&a, 50.0f, 3600 + 300, &t);    /* past the 12th hour boundary */
    ck(pm25_aqi_avg_ready(&a), "12 complete hours reaches the floor");

    /* 4. Constant input averages to that constant ---------------------- */
    ck(pm25_aqi_avg_24h(&a, &mean, &hours), "12-hour mean is reported");
    ck(fabsf(mean - 50.0f) < 0.5f, "constant 50 -> mean ~50");
    printf("     (12 h constant: mean=%.2f hours=%u)\n", (double)mean, hours);

    /* 5. A step change is averaged, not ignored ------------------------ */
    pm25_aqi_avg_init(&a); t = 0;
    feed(&a, 20.0f, 12 * 3600, &t);
    feed(&a, 220.0f, 12 * 3600, &t);
    pm25_aqi_avg_24h(&a, &mean, &hours);
    printf("     (12 h @20 then 12 h @220: mean=%.1f, expect ~120)\n", (double)mean);
    ck(fabsf(mean - 120.0f) < 15.0f, "half clean / half smoky averages near 120");

    /* 6. Sensor dropout must not drag the mean toward clean air --------- */
    pm25_aqi_avg_init(&a); t = 0;
    feed(&a, 200.0f, 12 * 3600, &t);
    float mean_before = 0.0f; uint8_t h_before = 0;
    pm25_aqi_avg_24h(&a, &mean_before, &h_before);
    /* Two hours of -1 ("no reading") must change nothing. */
    feed(&a, -1.0f, 2 * 3600, &t);
    float mean_after = 0.0f; uint8_t h_after = 0;
    pm25_aqi_avg_24h(&a, &mean_after, &h_after);
    printf("     (dropout: before=%.1f after=%.1f)\n", (double)mean_before, (double)mean_after);
    ck(fabsf(mean_before - mean_after) < 0.01f,
       "no-reading sentinel does not pull the mean toward zero");

    /* 7. NaN is ignored, not propagated -------------------------------- */
    pm25_aqi_avg_push(&a, NAN, t += 1000u);
    pm25_aqi_avg_24h(&a, &mean, &hours);
    ck(isfinite(mean), "NaN sample does not poison the mean");

    /* 8. Ring wraps without losing the recent 24 hours ----------------- */
    pm25_aqi_avg_init(&a); t = 0;
    feed(&a, 100.0f, 48 * 3600, &t);    /* two full days */
    pm25_aqi_avg_24h(&a, &mean, &hours);
    ck(hours == PM25_AVG_HOURS, "ring holds exactly 24 hours after wrap");
    ck(fabsf(mean - 100.0f) < 1.0f, "constant input survives the wrap");

    /* 9. NULL safety ---------------------------------------------------- */
    pm25_aqi_avg_push(NULL, 10.0f, 1000);
    ck(!pm25_aqi_avg_24h(NULL, &mean, &hours), "24h(NULL) is false, no crash");
    ck(!pm25_aqi_avg_ready(NULL), "ready(NULL) is false");
    ck(pm25_aqi_avg_24h(&a, NULL, NULL), "NULL out-params tolerated");
    pm25_aqi_avg_init(NULL);
    ck(1, "init(NULL) is safe");

    printf("\n%s (%d failure%s)\n",
           fails ? "FAILED" : "ALL 24-HOUR AVERAGING CHECKS PASSED",
           fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
