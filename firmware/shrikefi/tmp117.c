/*
 * tmp117.c
 * TI TMP117 High-Accuracy Digital Temperature Sensor Driver
 * SIH26181 Health Companion & Edge Disaster Monitor
 *
 * Deliberately small and dependency-free: two 16-bit register reads and a
 * decode. All the subtlety is in the decoding and the plausibility gate, which
 * are documented at each step because both have bitten this project's sibling
 * drivers before.
 */

#include "tmp117.h"

#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *TAG = "TMP117";
#else
#define ESP_LOGI(tag, ...) do {} while (0)
#define ESP_LOGW(tag, ...) do {} while (0)
#endif

/* Read a 16-bit big-endian register. Returns 0 on success. */
static int tmp117_read_u16(const tmp117_t *dev, uint8_t reg, uint16_t *out) {
    if (!out) return -1;
    uint8_t buf[2] = {0, 0};
    if (esp32_i2c_hal_read(dev->addr, reg, buf, 2) != I2C_HAL_SUCCESS) {
        return -1;
    }
    *out = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
    return 0;
}

/* Write a 16-bit big-endian register. Returns 0 on success. */
static int tmp117_write_u16(const tmp117_t *dev, uint8_t reg, uint16_t val) {
    uint8_t buf[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFFu) };
    return (esp32_i2c_hal_write(dev->addr, reg, buf, 2) == I2C_HAL_SUCCESS) ? 0 : -1;
}

int tmp117_init(tmp117_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr) {
    if (!dev || !i2c) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->i2c  = i2c;
    dev->addr = addr;

    /* 1. Device ID is the only reliable presence test.
     *
     * An I2C ACK proves *something* answers at this address. The TMP117 puts a
     * fixed 0x0117 in the low 12 bits of register 0x0F, so checking it separates
     * "the probe is fitted" from "another chip is strapped to 0x48" - a real
     * hazard here, because this board's I2C scan once reported 0x68 (MPU6050)
     * and 0x76 (BME280) on the same bus.
     *
     * This is also why the earlier "not responding at 0x48" result was
     * ambiguous: without a driver there was no ID check, so a fitted-but-dead
     * part and a missing part looked identical. */
    uint16_t id = 0;
    if (tmp117_read_u16(dev, TMP117_REG_DEVICE_ID, &id) != 0) {
        ESP_LOGW(TAG, "TMP117: Device NOT responding at I2C address 0x%02X! "
                      "(NO ACK - check 3.3V, GND, SDA=GPIO1, SCL=GPIO2)",
                 (unsigned)addr);
        return -1;
    }
    if ((id & TMP117_DEVICE_ID_MASK) != TMP117_DEVICE_ID_VAL) {
        ESP_LOGW(TAG, "TMP117: Device ID mismatch at 0x%02X: got 0x%04X, expected "
                      "low 12 bits 0x117 - a different chip is answering here",
                 (unsigned)addr, (unsigned)id);
        return -1;
    }

    /* 2. Configure one-shot conversions at 1 Hz.
     *
     * The probe is read once a second by the monitor task, so continuous
     * conversion would waste ~180 uA for nothing. 8x averaging + a 1 s cycle
     * keeps the part near its 3.5 uA average and still produces a fresh result
     * well inside one read interval. Bits:
     *   [12]    MOD     2  (1 = shutdown between conversions)
     *   [11:9]  CONV    4  (100b = 125 ms conversion cycle)
     *   [8:7]   AVG     1  (01b = 8 samples averaged)
     * The exact composite value is 0x0240; if a part rejects it the write is
     * ignored rather than treated as fatal, because the default configuration
     * also produces valid readings. */
    const uint16_t cfg = 0x0240u;
    if (tmp117_write_u16(dev, TMP117_REG_CONFIG, cfg) != 0) {
        ESP_LOGW(TAG, "TMP117: config write failed; continuing with power-on defaults");
    }

    dev->initialized = 1;
    ESP_LOGI(TAG, "TMP117 probe ready at 0x%02X (device ID 0x%04X)",
             (unsigned)addr, (unsigned)id);
    return 0;
}

int tmp117_read_temp(tmp117_t *dev, float *out_c) {
    if (!dev || !out_c || !dev->initialized) return -1;

    uint16_t raw = 0;
    if (tmp117_read_u16(dev, TMP117_REG_TEMP, &raw) != 0) {
        dev->error_count++;
        return -1;
    }

    /* Two's complement, units of 1/128 C.
     *
     * The cast to int16_t must happen BEFORE scaling. Masking the value to 12
     * bits instead - a common shortcut - turns every negative reading into a
     * huge positive one: -0.5 C (0xFFC0) becomes +127.5 C. Body temperatures
     * are positive, so that bug only shows up on a cold boot or a cold room,
     * which is exactly when a frostbite/hypothermia feature matters. */
    float t = (float)((int16_t)raw) * TMP117_LSB_C;

    /* Plausibility gate. A powered TMP117 cannot read -55 C in a room, and a
     * real contact reading above 60 C is not a body temperature. Values outside
     * the window are treated as a failed read so they can never be published as
     * clinical data - the same rule the rest of this project follows. */
    if (!(t >= TMP117_MIN_VALID_C && t <= TMP117_MAX_VALID_C)) {
        dev->error_count++;
        return -1;
    }

    dev->last_c = t;
    *out_c = t;
    return 0;
}

float tmp117_last_temp(const tmp117_t *dev) {
    return dev ? dev->last_c : 0.0f;
}
