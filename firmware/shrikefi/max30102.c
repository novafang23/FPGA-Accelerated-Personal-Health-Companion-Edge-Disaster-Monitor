/*
 * max30102.c
 * MAX30102 Pulse Oximetry & Heart-Rate Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include "max30102.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30102";

#else
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#define ESP_LOGI(tag, ...) do {} while(0)
#define ESP_LOGE(tag, ...) do {} while(0)
#define ESP_LOGW(tag, ...) do {} while(0)
static const char *TAG __attribute__((unused)) = "MAX30102";
#endif

/* Internal I2C helpers using ESP32 I2C HAL */
static int max30102_i2c_write_reg(max30102_t *dev, uint8_t reg, uint8_t val) {
    (void)dev;
    return esp32_i2c_hal_write_byte(MAX30102_I2C_ADDR, reg, val);
}

static int max30102_i2c_read_reg(max30102_t *dev, uint8_t reg) {
    uint8_t val;
    (void)dev;
    int ret = esp32_i2c_hal_read_byte(MAX30102_I2C_ADDR, reg, &val);
    return (ret == I2C_HAL_SUCCESS) ? val : -1;
}

static int max30102_i2c_write_read(max30102_t *dev, uint8_t reg, uint8_t *data, size_t len) {
    (void)dev;
    return esp32_i2c_hal_read(MAX30102_I2C_ADDR, reg, data, (uint16_t)len);
}

static void max30102_delay_ms(int ms) {
#ifdef ESP_PLATFORM
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/* Initialization */
int max30102_reset(max30102_t *dev) {
    if (!dev || !dev->i2c) return -1;

    if (max30102_i2c_write_reg(dev, MAX30102_REG_MODE_CONFIG, MAX30102_MODE_RESET) != 0) {
        return -1;
    }

    int timeout = 100;
    while (timeout-- > 0) {
        int val = max30102_i2c_read_reg(dev, MAX30102_REG_MODE_CONFIG);
        if (val < 0) return -1;
        if (!(val & MAX30102_MODE_RESET)) break;
        max30102_delay_ms(1);
    }
    return (timeout > 0) ? 0 : -1;
}

int max30102_init(max30102_t *dev, esp32_i2c_handle_t *i2c) {
    if (!dev || !i2c) return -1;

    dev->i2c = i2c;
    dev->initialized = 0;

    /* Verify part ID */
    int part_id = max30102_i2c_read_reg(dev, MAX30102_REG_PART_ID);
    if (part_id < 0 || (uint8_t)part_id != MAX30102_EXPECTED_PART_ID) {
        ESP_LOGE(TAG, "Part ID mismatch: expected 0x%02X, got 0x%02X", MAX30102_EXPECTED_PART_ID, part_id);
        return -1;
    }

    if (max30102_reset(dev) != 0) return -1;

    /* FIFO Configuration: 4-sample averaging, rollover enabled, A_FULL at 17 */
    if (max30102_i2c_write_reg(dev, MAX30102_REG_FIFO_CONFIG,
                    MAX30102_FIFO_SMP_AVE_4 | MAX30102_FIFO_ROLLOVER_EN | MAX30102_FIFO_A_FULL_17) != 0) {
        return -1;
    }

    /* SpO2 Configuration: 4096nA range, 100 Hz, 411µs pulse width (18-bit) */
    if (max30102_i2c_write_reg(dev, MAX30102_REG_SPO2_CONFIG,
                    MAX30102_SPO2_ADC_RANGE_4096 | MAX30102_SPO2_SR_100 | MAX30102_SPO2_PW_411) != 0) {
        return -1;
    }

    /* LED Pulse Amplitude: ~7.2mA each */
    if (max30102_i2c_write_reg(dev, MAX30102_REG_LED1_PA, 0x24) != 0) return -1;
    if (max30102_i2c_write_reg(dev, MAX30102_REG_LED2_PA, 0x24) != 0) return -1;

    /* Clear FIFO pointers */
    max30102_i2c_write_reg(dev, MAX30102_REG_FIFO_WR_PTR, 0x00);
    max30102_i2c_write_reg(dev, MAX30102_REG_OVF_COUNTER, 0x00);
    max30102_i2c_write_reg(dev, MAX30102_REG_FIFO_RD_PTR, 0x00);

    /* Enable SpO2 mode (Red + IR) */
    if (max30102_i2c_write_reg(dev, MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2) != 0) return -1;

    /* Enable FIFO almost-full interrupt */
    if (max30102_i2c_write_reg(dev, MAX30102_REG_INT_ENABLE_1, 0x40) != 0) return -1;

    dev->initialized = 1;
    ESP_LOGI(TAG, "MAX30102 initialized successfully");
    return 0;
}

/* FIFO Operations */
int max30102_fifo_available(max30102_t *dev) {
    if (!dev || !dev->initialized) return -1;

    int wr = max30102_i2c_read_reg(dev, MAX30102_REG_FIFO_WR_PTR);
    int rd = max30102_i2c_read_reg(dev, MAX30102_REG_FIFO_RD_PTR);
    if (wr < 0 || rd < 0) return -1;

    int count = wr - rd;
    if (count < 0) count += 32;
    return count;
}

int max30102_read_sample(max30102_t *dev, max30102_sample_t *sample) {
    if (!dev || !dev->initialized || !sample) return -1;

    uint8_t fifo_data[6];
    if (max30102_i2c_write_read(dev, MAX30102_REG_FIFO_DATA, fifo_data, 6) != 0) {
        return -1;
    }

    sample->red = ((uint32_t)(fifo_data[0] & 0x03) << 16) |
                  ((uint32_t)fifo_data[1] << 8) | ((uint32_t)fifo_data[2]);

    sample->ir = ((uint32_t)(fifo_data[3] & 0x03) << 16) |
                 ((uint32_t)fifo_data[4] << 8) | ((uint32_t)fifo_data[5]);

    return 0;
}

int max30102_read_fifo(max30102_t *dev, max30102_sample_t *buf, int max_samples) {
    if (!dev || !buf || max_samples <= 0) return -1;

    int available = max30102_fifo_available(dev);
    if (available <= 0) return available;

    int to_read = (available < max_samples) ? available : max_samples;
    for (int i = 0; i < to_read; i++) {
        if (max30102_read_sample(dev, &buf[i]) != 0) return i;
    }
    return to_read;
}

/* Temperature */
float max30102_read_temperature(max30102_t *dev) {
    if (!dev || !dev->initialized) return -999.0f;

    if (max30102_i2c_write_reg(dev, MAX30102_REG_TEMP_CONFIG, 0x01) != 0) return -999.0f;

    int timeout = 100;
    while (timeout-- > 0) {
        int val = max30102_i2c_read_reg(dev, MAX30102_REG_TEMP_CONFIG);
        if (val < 0) return -999.0f;
        if (!(val & 0x01)) break;
        max30102_delay_ms(1);
    }
    if (timeout <= 0) return -999.0f;

    int temp_int = max30102_i2c_read_reg(dev, MAX30102_REG_TEMP_INT);
    int temp_frac = max30102_i2c_read_reg(dev, MAX30102_REG_TEMP_FRAC);
    if (temp_int < 0 || temp_frac < 0) return -999.0f;

    return (float)(int8_t)temp_int + ((float)temp_frac * 0.0625f);
}

/* Power Management */
int max30102_shutdown(max30102_t *dev) {
    if (!dev || !dev->initialized) return -1;
    int mode = max30102_i2c_read_reg(dev, MAX30102_REG_MODE_CONFIG);
    if (mode < 0) return -1;
    return max30102_i2c_write_reg(dev, MAX30102_REG_MODE_CONFIG, (uint8_t)(mode | MAX30102_MODE_SHDN));
}

int max30102_wakeup(max30102_t *dev) {
    if (!dev || !dev->initialized) return -1;
    int mode = max30102_i2c_read_reg(dev, MAX30102_REG_MODE_CONFIG);
    if (mode < 0) return -1;
    return max30102_i2c_write_reg(dev, MAX30102_REG_MODE_CONFIG, (uint8_t)(mode & ~MAX30102_MODE_SHDN));
}

/* Adaptive LED Current Control */
int max30102_adjust_led_current(max30102_t *dev, uint32_t red_sample, uint32_t ir_sample) {
    if (!dev || !dev->initialized) return -1;

    const uint32_t TARGET_LOW  = (1u << 17);
    const uint32_t TARGET_HIGH = (1u << 18) * 3 / 4;
    const uint8_t STEP = 4;

    uint8_t red_pa = max30102_i2c_read_reg(dev, MAX30102_REG_LED1_PA);
    uint8_t ir_pa  = max30102_i2c_read_reg(dev, MAX30102_REG_LED2_PA);
    if (red_pa == 0xFF || ir_pa == 0xFF) return -1;

    if (red_sample < TARGET_LOW && red_pa < 255 - STEP) red_pa += STEP;
    else if (red_sample > TARGET_HIGH && red_pa > STEP) red_pa -= STEP;

    if (ir_sample < TARGET_LOW && ir_pa < 255 - STEP) ir_pa += STEP;
    else if (ir_sample > TARGET_HIGH && ir_pa > STEP) ir_pa -= STEP;

    if (max30102_i2c_write_reg(dev, MAX30102_REG_LED1_PA, red_pa) != 0) return -1;
    if (max30102_i2c_write_reg(dev, MAX30102_REG_LED2_PA, ir_pa) != 0) return -1;

    return 0;
}