/* Standalone test runner for pm25_calibration_int8.
 * Compile with:
 *   gcc -std=c99 -Wall -Werror -O2 test_pm25_calibration.c pm25_calibration_int8.c -lm -o test_pm25
 */
#include "pm25_calibration_int8.h"

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

int main(void)
{
    float a;
    float b;
    float c;
    float d;

    pm25_calibration_init();

    /* Normal ambient conditions */
    a = pm25_calibrate_nn_int8(20.0f, 25.0f, 40.0f);
    printf("Normal conditions result: %.3f ug/m3\n", a);
    CHECK((a > 12.0f) && (a < 16.0f), "Normal condition output is in expected range");

    /* High-humidity fog/moisture distortion */
    b = pm25_calibrate_nn_int8(80.0f, 20.0f, 85.0f);
    printf("High-humidity result: %.3f ug/m3\n", b);
    CHECK((b > 20.0f) && (b < 30.0f), "High-humidity overestimation is suppressed");

    /* Wildfire / disaster smoke */
    c = pm25_calibrate_nn_int8(250.0f, 35.0f, 30.0f);
    printf("Wildfire smoke result: %.3f ug/m3\n", c);
    CHECK((c > 100.0f) && (c < 130.0f), "Wildfire smoke is tracked without over-dampening");

    /* Negative raw input must clamp to zero */
    d = pm25_calibrate_nn_int8(-5.0f, 25.0f, 40.0f);
    printf("Negative raw result: %.3f ug/m3\n", d);
    CHECK(d == 0.0f, "Negative raw PM is clamped to zero");

    /* 0% RH edge */
    d = pm25_calibrate_nn_int8(20.0f, 25.0f, 0.0f);
    printf("0%% RH result: %.3f ug/m3\n", d);
    CHECK(d >= 0.0f, "0%% RH output is non-negative");

    /* 100% RH edge */
    d = pm25_calibrate_nn_int8(20.0f, 25.0f, 100.0f);
    printf("100%% RH result: %.3f ug/m3\n", d);
    CHECK(d >= 0.0f, "100%% RH output is non-negative");

    /* Extreme but physically possible low temperature */
    d = pm25_calibrate_nn_int8(20.0f, -30.0f, 40.0f);
    printf("Extreme low temp result: %.3f ug/m3\n", d);
    CHECK(isfinite((double)d) && (d >= 0.0f), "Extreme low temperature output is valid");

    /* Humidity distortion detector */
    CHECK(pm25_is_humidity_distorted(80.0f, 85.0f) == true,
          "Humidity distortion detector triggers at RH > 60%% with PM");
    CHECK(pm25_is_humidity_distorted(20.0f, 40.0f) == false,
          "Humidity distortion detector stays quiet in normal conditions");
    CHECK(pm25_is_humidity_distorted(250.0f, 30.0f) == false,
          "Humidity distortion detector stays quiet in dry wildfire smoke");

    /* Out-of-range raw PM is passed through gracefully */
    d = pm25_calibrate_nn_int8(600.0f, 25.0f, 40.0f);
    printf("Over-range raw result: %.3f ug/m3\n", d);
    CHECK(d == 600.0f, "Over-range raw PM is passed through");

    /* Invalid humidity is passed through */
    d = pm25_calibrate_nn_int8(20.0f, 25.0f, 120.0f);
    printf("Invalid humidity result: %.3f ug/m3\n", d);
    CHECK(d == 20.0f, "Invalid humidity input is passed through");

    printf("\nTest summary: %d / %d checks passed\n",
           (s_checks_total - s_checks_failed),
           s_checks_total);

    return (s_checks_failed == 0) ? 0 : 1;
}
