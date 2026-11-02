/*
 * Host test for the TMP117 driver.
 *
 * Exercises the two things most likely to be wrong in a temperature driver:
 *   1. Two's-complement decoding (negative temperatures must stay negative)
 *   2. The device-ID check (a different chip at 0x48 must be rejected)
 *
 * The host I2C HAL already simulates a TMP117 at 0x48 with settable raw
 * temperature and device ID, so this needs no hardware.
 */
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "tmp117.h"

static int failures = 0;

static void check(int cond, const char *what) {
    printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) failures++;
}

int main(void) {
    printf("TMP117 driver host test\n");
    printf("=======================\n");

    esp32_i2c_handle_t *hal = esp32_i2c_hal_get_handle();
    tmp117_t dev;

    /* --- 1. Presence + device ID --- */
    esp32_i2c_hal_sim_set_tmp117_present(true);
    esp32_i2c_hal_sim_set_tmp117_device_id(0x0117);
    int rc = tmp117_init(&dev, hal, TMP117_I2C_ADDR);
    check(rc == 0, "init succeeds with device ID 0x0117 at 0x48");

    /* --- 2. Straightforward positive reading --- */
    /* 33.125 C -> 33.125 / 0.0078125 = 4240 = 0x1090 (the simulator default) */
    esp32_i2c_hal_sim_set_tmp117_raw_temp(0x1090);
    float t = 0.0f;
    rc = tmp117_read_temp(&dev, &t);
    check(rc == 0, "read succeeds");
    check(fabsf(t - 33.125f) < 0.01f, "33.125 C decodes exactly");

    /* --- 3. THE NEGATIVE CASE: two's complement, not masked --- */
    /* -0.5 C -> -0.5 / 0.0078125 = -64 -> 0xFFC0 as a 16-bit signed value.
     * A 12-bit mask would give 0xFC0 = 4032 -> +31.5 C, a silent 32 C error. */
    esp32_i2c_hal_sim_set_tmp117_raw_temp((int16_t)-64);
    rc = tmp117_read_temp(&dev, &t);
    check(rc == 0, "read of a negative value succeeds");
    check(fabsf(t - (-0.5f)) < 0.01f, "-0.5 C decodes as -0.5, not +31.5");

    /* --- 4. Another negative: -10.0 C (1280 LSB below zero) --- */
    esp32_i2c_hal_sim_set_tmp117_raw_temp((int16_t)(-10.0f / 0.0078125f));
    rc = tmp117_read_temp(&dev, &t);
    check(rc == 0 && fabsf(t - (-10.0f)) < 0.01f, "-10.0 C decodes correctly");

    /* --- 5. LSB resolution: one LSB is 7.8125 mC --- */
    esp32_i2c_hal_sim_set_tmp117_raw_temp((int16_t)4241);
    tmp117_read_temp(&dev, &t);
    check(fabsf(t - 33.1328125f) < 0.001f, "one LSB = 0.0078125 C");

    /* --- 6. Implausible readings are REJECTED, not published --- */
    /* 100 C is beyond any body temperature: a bad read, not a fever. */
    esp32_i2c_hal_sim_set_tmp117_raw_temp((int16_t)(100.0f / 0.0078125f));
    rc = tmp117_read_temp(&dev, &t);
    check(rc != 0, "100 C rejected as implausible");
    /* 0.0f is returned by a failed read; make sure last_c was not corrupted. */
    check(fabsf(tmp117_last_temp(&dev) - 33.13f) < 0.02f,
          "last good value preserved after a rejected read");

    /* --- 7. Wrong device ID is rejected --- */
    tmp117_t bad;
    esp32_i2c_hal_sim_set_tmp117_device_id(0x0ABC);   /* not a TMP117 */
    rc = tmp117_init(&bad, hal, TMP117_I2C_ADDR);
    check(rc != 0, "init rejects a non-TMP117 device ID at 0x48");

    /* --- 8. Absent device is rejected cleanly --- */
    esp32_i2c_hal_sim_set_tmp117_present(false);
    rc = tmp117_init(&bad, hal, TMP117_I2C_ADDR);
    check(rc != 0, "init fails cleanly when no device answers");

    /* --- 9. Reads fail cleanly with no device --- */
    tmp117_t nodev;
    esp32_i2c_hal_sim_set_tmp117_present(true);
    esp32_i2c_hal_sim_set_tmp117_device_id(0x0117);
    tmp117_init(&nodev, hal, TMP117_I2C_ADDR);
    esp32_i2c_hal_sim_set_tmp117_present(false);
    rc = tmp117_read_temp(&nodev, &t);
    check(rc != 0, "read fails when the device disappears (hot-unplug)");

    /* --- 10. NULL safety --- */
    check(tmp117_init(NULL, hal, 0x48) != 0, "init rejects NULL dev");
    check(tmp117_read_temp(NULL, &t) != 0, "read rejects NULL dev");
    check(tmp117_read_temp(&nodev, NULL) != 0, "read rejects NULL out");
    check(tmp117_last_temp(NULL) == 0.0f, "last_temp handles NULL");

    printf("\n%s (%d failure%s)\n",
           failures ? "FAILED" : "ALL TMP117 CHECKS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
