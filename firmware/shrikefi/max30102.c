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
    /* pdMS_TO_TICKS() truncates, so a request shorter than one tick becomes
     * vTaskDelay(0), which is only a yield. That is not a wait: the soft-reset
     * and temperature poll loops would spin on back-to-back I2C transactions
     * and exhaust their iteration budget before the device cleared the bit.
     * Busy-wait sub-tick intervals, and round longer intervals UP so a 29 ms
     * conversion never becomes 25 ms of sleep. */
    if (ms <= 0) return;
    if (ms < portTICK_PERIOD_MS) {
        esp_rom_delay_us((uint32_t)ms * 1000u);
    } else {
        vTaskDelay((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
    }
#elif defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/* Initialization */
int max30102_reset(max30102_t *dev) {
    if (!dev || !dev->i2c) return -1;

    uint8_t mode_reg = dev->is_max30100 ? MAX30100_REG_MODE_CONFIG : MAX30102_REG_MODE_CONFIG;
    if (max30102_i2c_write_reg(dev, mode_reg, MAX30102_MODE_RESET) != 0) {
        return -1;
    }

    int timeout = 100;
    while (timeout-- > 0) {
        int val = max30102_i2c_read_reg(dev, mode_reg);
        if (val < 0) return -1;
        if (!(val & MAX30102_MODE_RESET)) break;
        max30102_delay_ms(1);
    }
    return (timeout >= 0) ? 0 : -1;
}

int max30102_recover_fifo(max30102_t *dev) {
    if (!dev || !dev->initialized) return -1;

    uint8_t wr_reg   = dev->is_max30100 ? MAX30100_REG_FIFO_WR_PTR : MAX30102_REG_FIFO_WR_PTR;
    uint8_t ovf_reg  = dev->is_max30100 ? MAX30100_REG_OVF_COUNTER : MAX30102_REG_OVF_COUNTER;
    uint8_t rd_reg   = dev->is_max30100 ? MAX30100_REG_FIFO_RD_PTR : MAX30102_REG_FIFO_RD_PTR;
    uint8_t mode_reg = dev->is_max30100 ? MAX30100_REG_MODE_CONFIG : MAX30102_REG_MODE_CONFIG;

    /* 1. Reset FIFO pointers with 3 discrete register writes.
     * Note: Real MAX30102 silicon does not auto-increment pointer addresses
     * on writes starting at FIFO_WR_PTR (0x04). Individual writes guarantee
     * reliable pointer clearing on both genuine Maxim and clone silicon. */
    if (max30102_i2c_write_reg(dev, wr_reg, 0x00) != 0) return -1;
    if (max30102_i2c_write_reg(dev, ovf_reg, 0x00) != 0) return -1;
    if (max30102_i2c_write_reg(dev, rd_reg, 0x00) != 0) return -1;

    /* 2. Check if sensor browned out or reset to Shutdown (0x80 or 0x00).
     * If already running in SpO2 mode, DO NOT re-write MODE_CONFIG or LED currents!
     * Writing MODE_CONFIG resets the internal ADC timing and extinguishes the LEDs,
     * which was the primary cause of LED blinking churn. */
    int cur_mode = max30102_i2c_read_reg(dev, mode_reg);
    if (cur_mode < 0) {
        /* Never reprogram MODE_CONFIG after a failed read. A transient I2C
         * error used to look like brownout here (-1 != target_mode), causing
         * the driver to rewrite MODE_CONFIG and LED currents and visibly blink
         * the MAX30102. FIFO recovery must fail closed instead. */
        return -1;
    }
    uint8_t target_mode = dev->is_max30100 ? 0x03 : MAX30102_MODE_SPO2;
    if (cur_mode != (int)target_mode) {
        /* FIFO recovery is intentionally write-minimal. Do not rewrite
         * MODE_CONFIG, SPO2_CONFIG, or LED currents from this error path:
         * even a valid but stale mode read can cause a visible emitter blink.
         * Let the persistent-failure path perform a full, explicit init. */
        return -1;
    }
    return 0;
}

int max30102_init(max30102_t *dev, esp32_i2c_handle_t *i2c) {
    if (!dev || !i2c) return -1;

    dev->i2c = i2c;
    dev->initialized = 0;
    dev->is_max30100 = 0;

    /* Verify part ID (0x15 for MAX30102, 0x11 for MAX30100) */
    int part_id = max30102_i2c_read_reg(dev, MAX30102_REG_PART_ID);
    if (part_id < 0) {
        ESP_LOGE(TAG, "Part ID register read failed at 0x%02X", MAX30102_I2C_ADDR);
        return -1;
    }
    if (part_id == MAX30100_EXPECTED_PART_ID) {
        dev->is_max30100 = 1;
        ESP_LOGI(TAG, "Detected MAX30100-compatible sensor (Part ID: 0x11)");
    } else if (part_id == MAX30102_EXPECTED_PART_ID) {
        dev->is_max30100 = 0;
        ESP_LOGI(TAG, "Detected MAX30102 sensor (Part ID: 0x15)");
    } else {
        ESP_LOGE(TAG, "Part ID mismatch: expected 0x15 or 0x11, got 0x%02X", part_id);
        return -1;
    }

    if (max30102_reset(dev) != 0) return -1;

    if (dev->is_max30100) {
        /* MAX30100 Configuration (Datasheet Table 3) */
        /* Clear pointers first */
        max30102_i2c_write_reg(dev, MAX30100_REG_FIFO_WR_PTR, 0x00);
        max30102_i2c_write_reg(dev, MAX30100_REG_OVF_COUNTER, 0x00);
        max30102_i2c_write_reg(dev, MAX30100_REG_FIFO_RD_PTR, 0x00);

        /* Mode: SpO2 (0x03: both Red and IR LEDs enabled) */
        max30102_i2c_write_reg(dev, MAX30100_REG_MODE_CONFIG, 0x03);

        /* SpO2 config: 100Hz + Hi-Res enabled + 1600us pulse width (16-bit ADC)
         * Bit 6 = 1 (Hi-Res EN = 0x40)
         * Bits [4:2] = 001 (100 Hz = 0x04)
         * Bits [1:0] = 11 (1600us = 0x03)
         * -> 0x40 | 0x04 | 0x03 = 0x47 */
        max30102_i2c_write_reg(dev, MAX30100_REG_SPO2_CONFIG, 0x47);

        /* LED currents: Red = 0x08 (27.1mA), IR = 0x08 (27.1mA) */
        max30102_i2c_write_reg(dev, MAX30100_REG_LED_CONFIG, 0x88);

        int r_mode = max30102_i2c_read_reg(dev, MAX30100_REG_MODE_CONFIG);
        int r_spo2 = max30102_i2c_read_reg(dev, MAX30100_REG_SPO2_CONFIG);
        int r_led  = max30102_i2c_read_reg(dev, MAX30100_REG_LED_CONFIG);
        int r_wr   = max30102_i2c_read_reg(dev, MAX30100_REG_FIFO_WR_PTR);
        int r_rd   = max30102_i2c_read_reg(dev, MAX30100_REG_FIFO_RD_PTR);
        ESP_LOGI(TAG, "MAX30100 Configured: Mode=0x%02X SpO2=0x%02X LED=0x%02X (WR=%d, RD=%d)",
                 r_mode, r_spo2, r_led, r_wr, r_rd);
        (void)r_mode; (void)r_spo2; (void)r_led; (void)r_wr; (void)r_rd;
    } else {
        /* FIFO Configuration: no averaging (full 100 Hz FIFO), rollover enabled, A_FULL at 17.
         * 100 Hz cuts the peak-detection quantisation from 40 ms to 10 ms, which is what
         * RSA needs (20-60 ms of IBI modulation); the ADC is already at 100 Hz below. */
        if (max30102_i2c_write_reg(dev, MAX30102_REG_FIFO_CONFIG,
                        MAX30102_FIFO_SMP_AVE_1 | MAX30102_FIFO_ROLLOVER_EN | MAX30102_FIFO_A_FULL_17) != 0) {
            return -1;
        }

        /* SpO2 Configuration: 4096nA range, 100 Hz, 411µs pulse width (18-bit) */
        if (max30102_i2c_write_reg(dev, MAX30102_REG_SPO2_CONFIG,
                        MAX30102_SPO2_ADC_RANGE_4096 | MAX30102_SPO2_SR_100 | MAX30102_SPO2_PW_411) != 0) {
            return -1;
        }

        /* LED Pulse Amplitude: ~15.8mA each for clear capillary penetration */
        if (max30102_i2c_write_reg(dev, MAX30102_REG_LED1_PA, MAX30102_DEFAULT_LED_PA) != 0) return -1;
        if (max30102_i2c_write_reg(dev, MAX30102_REG_LED2_PA, MAX30102_DEFAULT_LED_PA) != 0) return -1;

        /* Clear FIFO pointers */
        max30102_i2c_write_reg(dev, MAX30102_REG_FIFO_WR_PTR, 0x00);
        max30102_i2c_write_reg(dev, MAX30102_REG_OVF_COUNTER, 0x00);
        max30102_i2c_write_reg(dev, MAX30102_REG_FIFO_RD_PTR, 0x00);

        /* Enable SpO2 mode (Red + IR) */
        if (max30102_i2c_write_reg(dev, MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2) != 0) return -1;

        /* Enable FIFO almost-full interrupt */
        if (max30102_i2c_write_reg(dev, MAX30102_REG_INT_ENABLE_1, 0x40) != 0) return -1;
    }

    dev->initialized = 1;
    ESP_LOGI(TAG, "MAX3010x initialized successfully");
    return 0;
}

/* FIFO Operations */
int max30102_fifo_available(max30102_t *dev, int *dropped, int *saturated) {
    if (!dev || !dev->initialized) return -1;

    uint8_t wr_reg = dev->is_max30100 ? MAX30100_REG_FIFO_WR_PTR : MAX30102_REG_FIFO_WR_PTR;
    uint8_t ptrs[3];
    int wr = 0, ovf = 0, rd = 0;

    /* Burst read all 3 FIFO tracking registers in a single transaction (WR_PTR, OVF, RD_PTR).
     * Order is identical in both MAX30102 (0x04-0x06) and MAX30100 (0x02-0x04).
     * This slashes I2C bus transactions by 66% and prevents pointer skew. */
    if (max30102_i2c_write_read(dev, wr_reg, ptrs, 3) == 0) {
        wr  = ptrs[0];
        ovf = ptrs[1];
        rd  = ptrs[2];
    } else {
        /* Fallback to individual register reads if burst read fails or is unsupported */
        uint8_t ovf_reg = dev->is_max30100 ? MAX30100_REG_OVF_COUNTER : MAX30102_REG_OVF_COUNTER;
        uint8_t rd_reg  = dev->is_max30100 ? MAX30100_REG_FIFO_RD_PTR : MAX30102_REG_FIFO_RD_PTR;
        int r_wr = max30102_i2c_read_reg(dev, wr_reg);
        int r_ovf = max30102_i2c_read_reg(dev, ovf_reg);
        int r_rd = max30102_i2c_read_reg(dev, rd_reg);
        if (r_wr < 0 || r_ovf < 0 || r_rd < 0) {
            return -1;
        }
        wr = r_wr;
        ovf = r_ovf;
        rd = r_rd;
    }

    int max_ovf = dev->is_max30100 ? 15 : 31;
    if (saturated) *saturated = (ovf == max_ovf) ? 1 : 0;
    if (dropped) *dropped = (ovf > 0 && ovf < max_ovf) ? ovf : 0;

    int mask = dev->is_max30100 ? 15 : 31;
    int count = (wr - rd) & mask;
    if (count == 0 && ovf > 0) {
        count = dev->is_max30100 ? 16 : 32;
    }
    return count;
}

int max30102_read_sample(max30102_t *dev, max30102_sample_t *sample) {
    if (!dev || !dev->initialized || !sample) return -1;

    if (dev->is_max30100) {
        uint8_t fifo_data[4];
        if (max30102_i2c_write_read(dev, MAX30100_REG_FIFO_DATA, fifo_data, 4) != 0) {
            return -1;
        }
        // MAX30100 returns 16-bit IR first, then 16-bit Red
        sample->ir  = ((uint32_t)fifo_data[0] << 8) | fifo_data[1];
        sample->red = ((uint32_t)fifo_data[2] << 8) | fifo_data[3];
        // Scale to 18-bit range so downstream filters stay aligned
        sample->ir  <<= 2;
        sample->red <<= 2;
    } else {
        uint8_t fifo_data[6];
        if (max30102_i2c_write_read(dev, MAX30102_REG_FIFO_DATA, fifo_data, 6) != 0) {
            return -1;
        }

        sample->red = ((uint32_t)(fifo_data[0] & 0x03) << 16) |
                      ((uint32_t)fifo_data[1] << 8) | ((uint32_t)fifo_data[2]);

        sample->ir = ((uint32_t)(fifo_data[3] & 0x03) << 16) |
                     ((uint32_t)fifo_data[4] << 8) | ((uint32_t)fifo_data[5]);
    }

    return 0;
}

int max30102_read_fifo(max30102_t *dev, max30102_sample_t *buf, int max_samples) {
    if (!dev || !buf || max_samples <= 0) return -1;

    int available = max30102_fifo_available(dev, NULL, NULL);
    if (available <= 0) return available;

    int to_read = (available < max_samples) ? available : max_samples;
    for (int i = 0; i < to_read; i++) {
        if (max30102_read_sample(dev, &buf[i]) != 0) return i;
    }
    return to_read;
}

/* On-chip Die Temperature Acquisition */
int max30102_read_die_temp(max30102_t *dev, float *out_temp_c) {
    if (!dev || !dev->initialized || !out_temp_c || dev->is_max30100) return -1;

    /* Write 0x01 to initiate one-shot die temperature conversion */
    if (max30102_i2c_write_reg(dev, MAX30102_REG_TEMP_CONFIG, 0x01) != 0) {
        return -1;
    }

    /* Wait for conversion bit to self-clear (typically ~29ms on MAX30102) */
    for (int i = 0; i < 6; i++) {
        max30102_delay_ms(10);
        int cfg = max30102_i2c_read_reg(dev, MAX30102_REG_TEMP_CONFIG);
        if (cfg < 0) return -1;
        if ((cfg & 0x01) == 0) break;
    }

    int ti = max30102_i2c_read_reg(dev, MAX30102_REG_TEMP_INT);
    int tf = max30102_i2c_read_reg(dev, MAX30102_REG_TEMP_FRAC);
    if (ti < 0 || tf < 0) return -1;

    /* 8-bit signed integer + 4-bit fractional (0.0625 C per LSB) */
    *out_temp_c = (float)((int8_t)ti) + 0.0625f * (float)(tf & 0x0F);
    return 0;
}

/* Power Management */
int max30102_shutdown(max30102_t *dev) {
    if (!dev || !dev->initialized) return -1;
    uint8_t mode_reg = dev->is_max30100 ? 0x06 : MAX30102_REG_MODE_CONFIG;
    int mode = max30102_i2c_read_reg(dev, mode_reg);
    if (mode < 0) return -1;
    return max30102_i2c_write_reg(dev, mode_reg, (uint8_t)(mode | MAX30102_MODE_SHDN));
}

int max30102_wakeup(max30102_t *dev) {
    if (!dev || !dev->initialized) return -1;
    uint8_t mode_reg = dev->is_max30100 ? 0x06 : MAX30102_REG_MODE_CONFIG;
    int mode = max30102_i2c_read_reg(dev, mode_reg);
    if (mode < 0) return -1;
    return max30102_i2c_write_reg(dev, mode_reg, (uint8_t)(mode & ~MAX30102_MODE_SHDN));
}

/* Adaptive LED Current Control */
int max30102_adjust_led_current(max30102_t *dev, uint32_t red_sample, uint32_t ir_sample) {
    if (!dev || !dev->initialized) return -1;

    if (dev->is_max30100) {
        int cfg = max30102_i2c_read_reg(dev, 0x09);
        if (cfg < 0) return -1;
        uint8_t red_curr = (uint8_t)((cfg >> 4) & 0x0F);
        uint8_t ir_curr  = (uint8_t)(cfg & 0x0F);

        const uint32_t TARGET_LOW  = 40000;
        const uint32_t TARGET_HIGH = 220000;

        if (red_sample < TARGET_LOW && red_curr < 0x0F) red_curr++;
        else if (red_sample > TARGET_HIGH && red_curr > 0x01) red_curr--;

        if (ir_sample < TARGET_LOW && ir_curr < 0x0F) ir_curr++;
        else if (ir_sample > TARGET_HIGH && ir_curr > 0x01) ir_curr--;

        uint8_t new_cfg = (red_curr << 4) | ir_curr;
        if (new_cfg != (uint8_t)cfg) {
            return max30102_i2c_write_reg(dev, 0x09, new_cfg);
        }
        return 0;
    }

    /* Use a wide dead-band around the normal tissue signal. The previous
     * 35k..180k window made a fingertip sitting near 180k hunt LED current
     * every adjustment period, which is visible as optical flicker and also
     * injects steps into the PPG waveform. */
    const uint32_t TARGET_LOW  = 50000;
    const uint32_t TARGET_HIGH = 220000;
    const uint8_t  STEP = 1;
    const uint8_t  MIN_PA = 0x24; /* ~7.2 mA minimum (keeps LEDs reliably on) */
    const uint8_t  MAX_PA = 0x5A; /* ~18.0 mA maximum (prevents 3.3V power dip / brownout) */

    int red_pa_i = max30102_i2c_read_reg(dev, MAX30102_REG_LED1_PA);
    int ir_pa_i  = max30102_i2c_read_reg(dev, MAX30102_REG_LED2_PA);
    if (red_pa_i < 0 || ir_pa_i < 0) return -1;

    uint8_t red_pa = (uint8_t)red_pa_i;
    uint8_t ir_pa  = (uint8_t)ir_pa_i;

    if (red_sample < TARGET_LOW && red_pa + STEP <= MAX_PA) red_pa += STEP;
    else if (red_sample > TARGET_HIGH && red_pa >= MIN_PA + STEP) red_pa -= STEP;

    if (ir_sample < TARGET_LOW && ir_pa + STEP <= MAX_PA) ir_pa += STEP;
    else if (ir_sample > TARGET_HIGH && ir_pa >= MIN_PA + STEP) ir_pa -= STEP;

    if (red_pa != (uint8_t)red_pa_i) {
        if (max30102_i2c_write_reg(dev, MAX30102_REG_LED1_PA, red_pa) != 0) return -1;
    }
    if (ir_pa != (uint8_t)ir_pa_i) {
        if (max30102_i2c_write_reg(dev, MAX30102_REG_LED2_PA, ir_pa) != 0) return -1;
    }

    return 0;
}

int max30102_get_led_current(max30102_t *dev, uint8_t *red_curr, uint8_t *ir_curr) {
    if (!dev || !dev->initialized) return -1;

    if (dev->is_max30100) {
        int cfg = max30102_i2c_read_reg(dev, MAX30100_REG_LED_CONFIG);
        if (cfg < 0) return -1;
        if (red_curr) *red_curr = (uint8_t)((cfg >> 4) & 0x0F);
        if (ir_curr)  *ir_curr  = (uint8_t)(cfg & 0x0F);
        return 0;
    }

    int r = max30102_i2c_read_reg(dev, MAX30102_REG_LED1_PA);
    int i = max30102_i2c_read_reg(dev, MAX30102_REG_LED2_PA);
    if (r < 0 || i < 0) return -1;
    if (red_curr) *red_curr = (uint8_t)r;
    if (ir_curr)  *ir_curr  = (uint8_t)i;
    return 0;
}
