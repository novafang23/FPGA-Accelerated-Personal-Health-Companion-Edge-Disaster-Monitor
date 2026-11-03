/* Standalone test runner for the two clinical biomarker indices in
 * disaster_risk_engine.c: Moran's Physiological Strain Index and the PM2.5
 * autonomic-strain heuristic.
 *
 * Compile with:
 *   gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. test_moran_aha.c \
 *       disaster_risk_engine.c nn_risk_model.c nn_risk_model_int8.c \
 *       hrv_analysis.c pm25_correction.c clinical_vitals_engine.c spo2_engine.c \
 *       ppg_sqi.c ppg_respiratory_rate.c ppg_baseline_rr.c pressure_trend.c \
 *       skin_thermal.c -lm -o test_moran_aha
 *
 * These tests exist because neither index had any assertion behind it. They were
 * printed in a host-test profile and never compared against an expected value,
 * which is how a constant +0.83 offset on a rested subject survived, and how a
 * hard +0.25 step at 35 ug/m3 survived in the particulate index.
 */
#include "disaster_risk_engine.h"

#include <math.h>
#include <stdio.h>

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

static int near(float a, float b, float tol)
{
    return fabsf(a - b) <= tol;
}

int main(void)
{
    /* ---- Moran PSI: the resting baseline ------------------------------ */
    /* A rested subject must read 0, not 0.83. The estimate starts at
     * PSI_T_CORE_REST_C, so when the formula's baseline disagreed with it every
     * subject carried a constant offset. */
    float rest = disaster_calculate_moran_psi(70.0f, 24.5f, 48.0f);
    CHECK(near(rest, 0.0f, 0.02f), "Rested subject at 70 bpm in a 24.5 C room scores 0");

    float cool = disaster_calculate_moran_psi(60.0f, 15.0f, 40.0f);
    CHECK(near(cool, 0.0f, 0.02f), "Cool room, low heart rate, still 0");

    /* ---- Moran PSI: the documented reference scenario ----------------- */
    /* 44 C, 60% RH, 145 bpm. T_core_est = 37 + 0.015*75 + 0.025*17 = 38.550
     *   t_strain = 5*(38.550-37)/(39.5-37) = 3.100
     *   hr_strain = 5*(145-70)/(180-70)    = 3.409
     * PSI = 6.509. Before the corrections this reference scenario read 6.701:
     * the -0.19 comes from removing the constant 0.83 rest offset (which was
     * larger) and unifying the core-estimate threshold with the PSI baseline at
     * 70 bpm instead of 75 (which raises the estimate slightly at high HR). */
    float heat = disaster_calculate_moran_psi(145.0f, 44.0f, 60.0f);
    CHECK(near(heat, 6.51f, 0.02f), "44 C / 60% RH / 145 bpm scores 6.51");

    /* ---- Moran PSI: monotonicity -------------------------------------- */
    {
        int rising = 1;
        float prev = disaster_calculate_moran_psi(70.0f, 30.0f, 50.0f);
        for (float hr = 80.0f; hr <= 180.0f; hr += 10.0f) {
            float v = disaster_calculate_moran_psi(hr, 30.0f, 50.0f);
            if (v < prev - 1e-4f) rising = 0;
            prev = v;
        }
        CHECK(rising, "PSI does not decrease as heart rate rises");
    }
    {
        int rising = 1;
        float prev = disaster_calculate_moran_psi(100.0f, 25.0f, 50.0f);
        for (float t = 28.0f; t <= 50.0f; t += 2.0f) {
            float v = disaster_calculate_moran_psi(100.0f, t, 50.0f);
            if (v < prev - 1e-4f) rising = 0;
            prev = v;
        }
        CHECK(rising, "PSI does not decrease as ambient temperature rises");
    }

    /* ---- Moran PSI: bounds -------------------------------------------- */
    {
        int bounded = 1;
        for (float t = 20.0f; t <= 55.0f; t += 5.0f) {
            for (float hr = 40.0f; hr <= 200.0f; hr += 20.0f) {
                float v = disaster_calculate_moran_psi(hr, t, 90.0f);
                if (!(v >= 0.0f && v <= 10.0f)) bounded = 0;
            }
        }
        CHECK(bounded, "PSI stays inside [0, 10] across the plausible range");
    }
    /* ---- Moran PSI: what the thermal term actually contributes --------- */
    /* Worth pinning down, because it is much smaller than the scale suggests.
     * At 50 C and 50% RH the heuristic estimate only lifts core temperature by
     * 0.56 C, so a resting subject scores 1.13/10. The core-temperature clamp at
     * PSI_T_CORE_MAX_C (41.5 C) is unreachable from realistic ambient conditions:
     * it would need a heat index near 208 C. PSI is therefore dominated by heart
     * rate, and the acute heat signal is carried by the Steadman heat-index card,
     * not by this index. This test documents that behaviour rather than endorsing
     * it - if the 0.025 C-per-degree coefficient is ever retuned, this fails. */
    CHECK(near(disaster_calculate_moran_psi(70.0f, 50.0f, 50.0f), 1.13f, 0.02f),
          "Resting subject at 50 C scores 1.13: the thermal term is conservative");
    CHECK(disaster_calculate_moran_psi(200.0f, 55.0f, 100.0f) <= 10.0f,
          "PSI never exceeds 10 even at the edge of the plausible band");

    /* ---- Moran PSI: bradycardia is a measurement, not a missing value -- */
    /* A genuine 25 bpm in 44 C heat is severe. It used to return exactly 0
     * because everything below 30 bpm was treated as the "no reading" sentinel,
     * discarding the thermal term as well as the cardiovascular one. */
    float brady = disaster_calculate_moran_psi(25.0f, 44.0f, 50.0f);
    CHECK(brady > 0.5f, "25 bpm bradycardia in 44 C still carries the heat term");
    CHECK(brady < 2.0f, "25 bpm bradycardia does not invent cardiovascular strain");

    /* ...while a true zero means no reading at all. */
    CHECK(disaster_calculate_moran_psi(0.0f, 44.0f, 50.0f) == 0.0f,
          "Heart rate 0 is the no-reading sentinel and returns 0");

    /* ---- Moran PSI: unusable inputs ------------------------------------ */
    /* Non-finite values must not reach the arithmetic: NaN compares false
     * against every clamp, so it would be serialised into the status JSON as
     * "psi":nan and the dashboard would fail to parse it. */
    CHECK(disaster_calculate_moran_psi(NAN, 30.0f, 50.0f) == 0.0f,
          "NaN heart rate returns 0 instead of propagating NaN");
    CHECK(disaster_calculate_moran_psi(90.0f, NAN, 50.0f) == 0.0f,
          "NaN ambient returns 0 instead of inventing a thermal term");
    CHECK(disaster_calculate_moran_psi(90.0f, 30.0f, NAN) == 0.0f,
          "NaN humidity returns 0");
    CHECK(disaster_calculate_moran_psi(90.0f, INFINITY, 50.0f) == 0.0f,
          "Infinite ambient returns 0");
    /* -100 C is the firmware's "never sampled" sentinel for ambient (s_last_temp
     * in main_shrikefi.c), so it must not produce a score. */
    CHECK(disaster_calculate_moran_psi(90.0f, -100.0f, 50.0f) == 0.0f,
          "Ambient -100 (never sampled) returns 0 instead of scoring from HR alone");
    /* 0 C, by contrast, is a legitimate winter reading and must still score.
     *   t_core = 37 + 0.015*20 = 37.3 (no heat term below 28 C ambient)
     *   PSI = 5*0.3/2.5 + 5*20/110 = 0.600 + 0.909 = 1.509 */
    CHECK(near(disaster_calculate_moran_psi(90.0f, 0.0f, 50.0f), 1.51f, 0.02f),
          "Ambient 0 C is a real reading and still scores");
    CHECK(disaster_calculate_moran_psi(90.0f, 30.0f, 150.0f) == 0.0f,
          "Humidity above 100% is rejected");

    /* ---- AHA autonomic strain: baseline ------------------------------- */
    CHECK(disaster_calculate_aha_autonomic_strain(10.0f, 50.0f) == 0.0f,
          "Clean air and healthy RMSSD score 0");
    CHECK(disaster_calculate_aha_autonomic_strain(0.0f, 50.0f) == 0.0f,
          "No particulate sensor (0) is not treated as clean air");

    /* ---- AHA: continuity at the acute band ---------------------------- */
    /* The hard +0.25 switch at pm25 > 35 made the gauge jump a quarter of its
     * range for a 0.1 ug/m3 change. */
    {
        float a = disaster_calculate_aha_autonomic_strain(35.0f, 19.0f);
        float b = disaster_calculate_aha_autonomic_strain(35.1f, 19.0f);
        CHECK(fabsf(a - b) < 0.02f, "AHA strain is continuous across 35 ug/m3");
    }
    {
        float prev = disaster_calculate_aha_autonomic_strain(15.0f, 19.0f);
        int rising = 1;
        for (float pm = 16.0f; pm <= 150.0f; pm += 4.0f) {
            float v = disaster_calculate_aha_autonomic_strain(pm, 19.0f);
            if (v < prev - 1e-4f) rising = 0;
            prev = v;
        }
        CHECK(rising, "AHA strain does not decrease as PM2.5 rises");
    }
    {
        float prev = disaster_calculate_aha_autonomic_strain(120.0f, 5.0f);
        int falling = 1;
        for (float r = 6.0f; r <= 60.0f; r += 4.0f) {
            float v = disaster_calculate_aha_autonomic_strain(120.0f, r);
            if (v > prev + 1e-4f) falling = 0;
            prev = v;
        }
        CHECK(falling, "AHA strain does not increase as RMSSD recovers");
    }

    /* ---- AHA: bounds and saturation ----------------------------------- */
    {
        int bounded = 1;
        for (float pm = 0.0f; pm <= 200.0f; pm += 10.0f) {
            for (float r = 5.0f; r <= 100.0f; r += 5.0f) {
                float v = disaster_calculate_aha_autonomic_strain(pm, r);
                if (!(v >= 0.0f && v <= 1.0f)) bounded = 0;
            }
        }
        CHECK(bounded, "AHA strain stays inside [0, 1]");
    }
    CHECK(near(disaster_calculate_aha_autonomic_strain(150.0f, 5.0f), 1.0f, 0.001f),
          "Saturated exposure and suppression reach 1.0");

    /* ---- AHA: unusable inputs ----------------------------------------- */
    CHECK(disaster_calculate_aha_autonomic_strain(NAN, 20.0f) == 0.0f,
          "NaN PM2.5 returns 0 instead of propagating NaN");
    CHECK(disaster_calculate_aha_autonomic_strain(120.0f, NAN) == 0.0f,
          "NaN RMSSD returns 0 instead of scoring from PM alone");
    CHECK(disaster_calculate_aha_autonomic_strain(120.0f, 0.0f) == 0.0f,
          "RMSSD 0 (no usable HRV) returns 0");
    CHECK(disaster_calculate_aha_autonomic_strain(120.0f, 900.0f) == 0.0f,
          "Implausible RMSSD above 500 ms is rejected");

    /* ---- Moran PSI against an anchored core temperature ---------------- */
    /* 37.0 C at rest is the baseline, so it must score 0 by construction. */
    CHECK(disaster_calculate_moran_psi_core(70.0f, 37.0f) == 0.0f,
          "Anchored core of 37.0 C at rest scores 0");

    /* 38.5 C / 145 bpm:
     *   t_strain = 5*(38.5-37)/(39.5-37) = 3.000
     *   hr_strain = 5*75/110             = 3.409
     * PSI = 6.409 */
    CHECK(near(disaster_calculate_moran_psi_core(145.0f, 38.5f), 6.41f, 0.02f),
          "Anchored core 38.5 C at 145 bpm scores 6.41");

    /* 40.0 C at rest is a high fever with no cardiac strain: 5*3/2.5 = 6.0. */
    CHECK(near(disaster_calculate_moran_psi_core(70.0f, 40.0f), 6.0f, 0.02f),
          "Anchored core 40 C at rest scores 6.0 from temperature alone");

    /* ---- why a raw fingertip reading must never be used as core -------- */
    /* A healthy fingertip is 30-34 C, so feeding it to the same formula during a
     * 44 C heat exposure reports far LESS strain than the heuristic does. That is
     * the whole reason main_shrikefi.c requires a calibration before choosing the
     * CORE path; if this assertion ever fails, revisit that guard. */
    {
        float from_raw_skin = disaster_calculate_moran_psi_core(145.0f, 33.0f);
        float heuristic     = disaster_calculate_moran_psi(145.0f, 44.0f, 60.0f);
        CHECK(from_raw_skin < heuristic,
              "Raw skin as core under-reports strain versus the heuristic");
    }

    /* ---- bounded, monotone, and rejects implausible core -------------- */
    {
        int rising = 1;
        float prev = disaster_calculate_moran_psi_core(100.0f, 37.0f);
        for (float c = 37.5f; c <= 41.0f; c += 0.5f) {
            float v = disaster_calculate_moran_psi_core(100.0f, c);
            if (v < prev - 1e-4f) rising = 0;
            prev = v;
        }
        CHECK(rising, "Anchored PSI does not decrease as core temperature rises");
    }
    CHECK(disaster_calculate_moran_psi_core(145.0f, 25.0f) == 0.0f,
          "Implausible core of 25 C is rejected, not scored");
    CHECK(disaster_calculate_moran_psi_core(145.0f, 46.0f) == 0.0f,
          "Implausible core of 46 C is rejected, not scored");
    CHECK(disaster_calculate_moran_psi_core(145.0f, NAN) == 0.0f,
          "NaN core returns 0 instead of propagating NaN");
    CHECK(disaster_calculate_moran_psi_core(0.0f, 38.5f) == 0.0f,
          "No heart rate with a valid core still returns 0");
    CHECK(disaster_calculate_moran_psi_core(200.0f, 41.0f) <= 10.0f,
          "Anchored PSI never exceeds 10");

    printf("\nTest summary: %d / %d checks passed\n",
           (s_checks_total - s_checks_failed),
           s_checks_total);

    return (s_checks_failed == 0) ? 0 : 1;
}
