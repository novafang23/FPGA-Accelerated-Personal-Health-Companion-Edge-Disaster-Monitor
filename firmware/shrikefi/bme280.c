/*
 * bme280.c
 * BME280 Temperature, Humidity & Pressure Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include "bme280.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BME280";

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
static const char *TAG __attribute__((unused)) = "BME280";
#endif

/* Internal I2C helpers using ESP32 I2C HAL */
static int bme280_i2c_write_reg(bme280_t *dev, uint8_t reg, uint8_t val) {
    return esp32_i2c_hal_write_byte(dev->addr, reg, val);
}

static int bme280_i2c_read_reg(bme280_t *dev, uint8_t reg) {
    uint8_t val;
    int ret = esp32_i2c_hal_read_byte(dev->addr, reg, &val);
    return (ret == I2C_HAL_SUCCESS) ? val : -1;
}

static int bme280_i2c_write_read(bme280_t *dev, uint8_t reg, uint8_t *data, size_t len) {
    return esp32_i2c_hal_read(dev->addr, reg, data, (uint16_t)len);
}

static void bme280_delay_ms(int ms) {
#ifdef ESP_PLATFORM
    if (ms <= 10) {
        esp_rom_delay_us(ms * 1000);
    } else {
        vTaskDelay(pdMS_TO_TICKS(ms) < 1 ? 1 : pdMS_TO_TICKS(ms));
    }
#elif defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/* Calibration Data Loading */
static int bme280_load_calibration(bme280_t *dev) {
    uint8_t buf[24];
    bme280_calib_t *c = &dev->calib;

    /* Read 24 bytes for Temperature and Pressure (T1..T3, P1..P9)
     * Both BMP280 and BME280 share these exact registers 0x88..0x9F */
    if (bme280_i2c_write_read(dev, BME280_REG_CALIB00, buf, 24) != 0) return -1;

    c->dig_T1 = (uint16_t)(buf[1] << 8) | buf[0];
    c->dig_T2 = (int16_t)((buf[3] << 8) | buf[2]);
    c->dig_T3 = (int16_t)((buf[5] << 8) | buf[4]);

    c->dig_P1 = (uint16_t)(buf[7] << 8) | buf[6];
    c->dig_P2 = (int16_t)((buf[9] << 8) | buf[8]);
    c->dig_P3 = (int16_t)((buf[11] << 8) | buf[10]);
    c->dig_P4 = (int16_t)((buf[13] << 8) | buf[12]);
    c->dig_P5 = (int16_t)((buf[15] << 8) | buf[14]);
    c->dig_P6 = (int16_t)((buf[17] << 8) | buf[16]);
    c->dig_P7 = (int16_t)((buf[19] << 8) | buf[18]);
    c->dig_P8 = (int16_t)((buf[21] << 8) | buf[20]);
    c->dig_P9 = (int16_t)((buf[23] << 8) | buf[22]);

    if (!dev->is_bmp280) {
        /* Read H1 from register 0xA1 (1 byte) */
        uint8_t h1_val = 0;
        int h1_ret = esp32_i2c_hal_read_byte(dev->addr, 0xA1, &h1_val);
        if (h1_ret != I2C_HAL_SUCCESS) return -1;
        c->dig_H1 = h1_val;

        /* Read H2..H6 from register 0xE1 (7 bytes) */
        uint8_t hbuf[7];
        if (bme280_i2c_write_read(dev, BME280_REG_CALIB26, hbuf, 7) != 0) return -1;

        c->dig_H2 = (int16_t)((hbuf[1] << 8) | hbuf[0]);
        c->dig_H3 = hbuf[2];
        c->dig_H4 = (int16_t)((hbuf[3] << 4) | (hbuf[4] & 0x0F));
        c->dig_H5 = (int16_t)((hbuf[5] << 4) | (hbuf[4] >> 4));
        c->dig_H6 = (int8_t)hbuf[6];
    } else {
        c->dig_H1 = 0;
        c->dig_H2 = 0;
        c->dig_H3 = 0;
        c->dig_H4 = 0;
        c->dig_H5 = 0;
        c->dig_H6 = 0;
    }

    return 0;
}

/* Compensation Formulas */
static float bme280_compensate_temperature(bme280_t *dev, int32_t adc_T) {
    bme280_calib_t *c = &dev->calib;

    float var1 = (((float)adc_T / 16384.0f) - ((float)c->dig_T1 / 1024.0f)) * (float)c->dig_T2;
    float var2 = ((((float)adc_T / 131072.0f) - ((float)c->dig_T1 / 8192.0f))
                 * (((float)adc_T / 131072.0f) - ((float)c->dig_T1 / 8192.0f))) * (float)c->dig_T3;

    dev->t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0f;
}

static float bme280_compensate_humidity(bme280_t *dev, int32_t adc_H) {
    bme280_calib_t *c = &dev->calib;

    float h = (float)dev->t_fine - 76800.0f;
    if (h < 1.0f && h > -1.0f) return 0.0f;

    h = (adc_H - ((float)c->dig_H4 * 64.0f + ((float)c->dig_H5 / 16384.0f) * h))
        * ((float)c->dig_H2 / 65536.0f
           * (1.0f + (float)c->dig_H6 / 67108864.0f * h
              * (1.0f + (float)c->dig_H3 / 67108864.0f * h)));
    h = h * (1.0f - (float)c->dig_H1 * h / 524288.0f);

    if (h > 100.0f) h = 100.0f;
    if (h < 0.0f)   h = 0.0f;
    return h;
}

static float bme280_compensate_pressure(bme280_t *dev, int32_t adc_P) {
    bme280_calib_t *c = &dev->calib;

    float var1 = ((float)dev->t_fine / 2.0f) - 64000.0f;
    float var2 = var1 * var1 * (float)c->dig_P6 / 32768.0f;
    var2 = var2 + var1 * (float)c->dig_P5 * 2.0f;
    var2 = (var2 / 4.0f) + ((float)c->dig_P4 * 65536.0f);
    var1 = ((float)c->dig_P3 * var1 * var1 / 524288.0f + (float)c->dig_P2 * var1) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * (float)c->dig_P1;

    if (var1 < 1.0f) return 0.0f;

    float p = 1048576.0f - (float)adc_P;
    p = (p - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = (float)c->dig_P9 * p * p / 2147483648.0f;
    var2 = p * (float)c->dig_P8 / 32768.0f;
    p = p + (var1 + var2 + (float)c->dig_P7) / 16.0f;

    return p / 100.0f;
}

/* Public API */
int bme280_reset(bme280_t *dev) {
    if (!dev || !dev->i2c) return -1;
    return bme280_i2c_write_reg(dev, BME280_REG_RESET, 0xB6);
}

int bme280_init(bme280_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr) {
    if (!dev || !i2c) return -1;

    dev->i2c = i2c;
    dev->initialized = 0;
    dev->t_fine = 0;
    dev->is_bmp280 = 0;

    /* Probe addresses: try requested address first, then fallback to alternate */
    uint8_t addrs_to_try[2];
    if (addr == BME280_I2C_ADDR_HIGH) {
        addrs_to_try[0] = BME280_I2C_ADDR_HIGH; // 0x77
        addrs_to_try[1] = BME280_I2C_ADDR_LOW;  // 0x76
    } else {
        addrs_to_try[0] = BME280_I2C_ADDR_LOW;  // 0x76
        addrs_to_try[1] = BME280_I2C_ADDR_HIGH; // 0x77
    }

    int chip_id = -1;
    uint8_t found_addr = 0;

    for (int i = 0; i < 2; i++) {
        dev->addr = addrs_to_try[i];
        int id = bme280_i2c_read_reg(dev, BME280_REG_CHIP_ID);
        if (id == BME280_CHIP_ID || id == BMP280_CHIP_ID ||
            id == BMP280_CHIP_ID_SAMPLE || id == BMP280_CHIP_ID_ALT) {
            chip_id = id;
            found_addr = addrs_to_try[i];
            break;
        }
    }

    if (chip_id < 0) {
        ESP_LOGE(TAG, "Neither BME280 nor BMP280 found on 0x76 or 0x77!");
        return -1;
    }

    dev->addr = found_addr;
    if (chip_id == BME280_CHIP_ID) {
        dev->is_bmp280 = 0;
        ESP_LOGI(TAG, "Detected BME280 at I2C 0x%02X (Chip ID: 0x%02X)", dev->addr, chip_id);
    } else {
        dev->is_bmp280 = 1;
        ESP_LOGI(TAG, "Detected BMP280 at I2C 0x%02X (Chip ID: 0x%02X)", dev->addr, chip_id);
    }

    if (bme280_reset(dev) != 0) return -1;
    bme280_delay_ms(10);

    if (bme280_load_calibration(dev) != 0) {
        ESP_LOGE(TAG, "Failed to load calibration parameters");
        return -1;
    }

    /* Configure for continuous weather monitoring (Normal Mode: auto hardware sampling every 1000ms) */
    if (!dev->is_bmp280) {
        if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_HUM, BME280_OS_1X) != 0) return -1;
    }
    if (bme280_i2c_write_reg(dev, BME280_REG_CONFIG,
                    (BME280_STANDBY_1000MS << 5) | (BME280_FILTER_4 << 2)) != 0) return -1;
    if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
                    (BME280_OS_2X << 5) | (BME280_OS_1X << 2) | BME280_MODE_NORMAL) != 0) return -1;

    dev->initialized = 1;
    ESP_LOGI(TAG, "%s initialized successfully (Continuous Normal Mode)", dev->is_bmp280 ? "BMP280" : "BME280");
    return 0;
}

int bme280_read(bme280_t *dev, bme280_data_t *data) {
    if (!dev || !dev->initialized || !data) return -1;

    /* Check status: if hardware is in middle of a measurement update, wait briefly */
    int timeout = 25;
    while (timeout-- > 0) {
        int status = bme280_i2c_read_reg(dev, BME280_REG_STATUS);
        if (status < 0) return -1;
        if (!(status & 0x08)) break; // measuring bit 3 is 0 -> measurement complete
        bme280_delay_ms(1);
    }

    uint8_t buf[8];
    size_t read_len = dev->is_bmp280 ? 6 : 8;
    if (bme280_i2c_write_read(dev, BME280_REG_PRESS_MSB, buf, read_len) != 0) return -1;

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);

    /* Guard against unmeasured reset value (0x80000) or bus disconnect (0x00000) */
    if (adc_T == 0x80000 || adc_T == 0) {
        /* Re-assert normal mode in case of momentary sensor brown-out */
        bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
            (BME280_OS_2X << 5) | (BME280_OS_1X << 2) | BME280_MODE_NORMAL);
        return -1;
    }

    data->temperature_c = bme280_compensate_temperature(dev, adc_T);
    data->pressure_hpa  = bme280_compensate_pressure(dev, adc_P);

    if (dev->is_bmp280) {
        data->humidity_pct = 50.0f; /* Nominal humidity fallback for BMP280 */
    } else {
        int32_t adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];
        if (adc_H == 0x8000) {
            data->humidity_pct = 50.0f;
        } else {
            data->humidity_pct  = bme280_compensate_humidity(dev, adc_H);
        }
    }

    return 0;
}

float bme280_read_temperature(bme280_t *dev) {
    bme280_data_t data;
    if (bme280_read(dev, &data) != 0) return -999.0f;
    return data.temperature_c;
}

/* Normal (Continuous) Mode Support */
int bme280_init_normal_mode(bme280_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr,
                            uint8_t standby, uint8_t filter) {
    if (!dev || !i2c) return -1;

    dev->i2c = i2c;
    dev->initialized = 0;
    dev->t_fine = 0;
    dev->is_bmp280 = 0;

    uint8_t addrs_to_try[2];
    if (addr == BME280_I2C_ADDR_HIGH) {
        addrs_to_try[0] = BME280_I2C_ADDR_HIGH;
        addrs_to_try[1] = BME280_I2C_ADDR_LOW;
    } else {
        addrs_to_try[0] = BME280_I2C_ADDR_LOW;
        addrs_to_try[1] = BME280_I2C_ADDR_HIGH;
    }

    int chip_id = -1;
    uint8_t found_addr = 0;

    for (int i = 0; i < 2; i++) {
        dev->addr = addrs_to_try[i];
        int id = bme280_i2c_read_reg(dev, BME280_REG_CHIP_ID);
        if (id == BME280_CHIP_ID || id == BMP280_CHIP_ID ||
            id == BMP280_CHIP_ID_SAMPLE || id == BMP280_CHIP_ID_ALT) {
            chip_id = id;
            found_addr = addrs_to_try[i];
            break;
        }
    }

    if (chip_id < 0) return -1;

    dev->addr = found_addr;
    dev->is_bmp280 = (chip_id != BME280_CHIP_ID);

    if (bme280_reset(dev) != 0) return -1;
    bme280_delay_ms(10);

    if (bme280_load_calibration(dev) != 0) return -1;

    if (!dev->is_bmp280) {
        if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_HUM, BME280_OS_1X) != 0) return -1;
    }
    if (bme280_i2c_write_reg(dev, BME280_REG_CONFIG,
                    ((standby & 0x07) << 5) | ((filter & 0x07) << 2)) != 0) return -1;
    if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
                    (BME280_OS_1X << 5) | (BME280_OS_1X << 2) | BME280_MODE_NORMAL) != 0) return -1;

    dev->initialized = 1;
    return 0;
}

int bme280_read_normal(bme280_t *dev, bme280_data_t *data) {
    if (!dev || !dev->initialized || !data) return -1;

    int status = bme280_i2c_read_reg(dev, BME280_REG_STATUS);
    if (status < 0) return -1;
    if (status & 0x08) return -1;

    uint8_t buf[8];
    size_t read_len = dev->is_bmp280 ? 6 : 8;
    if (bme280_i2c_write_read(dev, BME280_REG_PRESS_MSB, buf, read_len) != 0) return -1;

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);

    data->temperature_c = bme280_compensate_temperature(dev, adc_T);
    data->pressure_hpa  = bme280_compensate_pressure(dev, adc_P);

    if (dev->is_bmp280) {
        data->humidity_pct = 50.0f;
    } else {
        int32_t adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];
        data->humidity_pct  = bme280_compensate_humidity(dev, adc_H);
    }

    return 0;
}

int bme280_sleep(bme280_t *dev) {
    if (!dev || !dev->initialized) return -1;
    return bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
                        (BME280_OS_1X << 5) | (BME280_OS_1X << 2) | BME280_MODE_SLEEP);
}