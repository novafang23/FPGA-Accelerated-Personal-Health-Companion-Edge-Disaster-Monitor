/*
 * bme280.c
 * BME280 Temperature, Humidity & Pressure Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include "bme280.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
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
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/* Calibration Data Loading */
static int bme280_load_calibration(bme280_t *dev) {
    uint8_t buf[26];
    bme280_calib_t *c = &dev->calib;

    if (bme280_i2c_write_read(dev, BME280_REG_CALIB00, buf, 26) != 0) return -1;

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

    c->dig_H1 = buf[25];

    uint8_t hbuf[7];
    if (bme280_i2c_write_read(dev, BME280_REG_CALIB26, hbuf, 7) != 0) return -1;

    c->dig_H2 = (int16_t)((hbuf[1] << 8) | hbuf[0]);
    c->dig_H3 = hbuf[2];
    c->dig_H4 = (int16_t)((hbuf[3] << 4) | (hbuf[4] & 0x0F));
    c->dig_H5 = (int16_t)((hbuf[5] << 4) | (hbuf[4] >> 4));
    c->dig_H6 = (int8_t)hbuf[6];

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
    dev->addr = addr;
    dev->initialized = 0;
    dev->t_fine = 0;

    int chip_id = bme280_i2c_read_reg(dev, BME280_REG_CHIP_ID);
    if (chip_id < 0 || (uint8_t)chip_id != BME280_CHIP_ID) return -1;

    if (bme280_reset(dev) != 0) return -1;
    bme280_delay_ms(10);

    if (bme280_load_calibration(dev) != 0) return -1;

    /* Configure for weather monitoring */
    if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_HUM, BME280_OS_1X) != 0) return -1;
    if (bme280_i2c_write_reg(dev, BME280_REG_CONFIG,
                    (BME280_STANDBY_1000MS << 5) | (BME280_FILTER_4 << 2)) != 0) return -1;
    if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
                    (BME280_OS_2X << 5) | (BME280_OS_1X << 2) | BME280_MODE_FORCED) != 0) return -1;

    dev->initialized = 1;
    ESP_LOGI(TAG, "BME280 initialized successfully");
    return 0;
}

int bme280_read(bme280_t *dev, bme280_data_t *data) {
    if (!dev || !dev->initialized || !data) return -1;

    if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
                    (BME280_OS_2X << 5) | (BME280_OS_1X << 2) | BME280_MODE_FORCED) != 0) return -1;

    int timeout = 100;
    while (timeout-- > 0) {
        int status = bme280_i2c_read_reg(dev, BME280_REG_STATUS);
        if (status < 0) return -1;
        if (!(status & 0x08)) break;
        bme280_delay_ms(1);
    }
    if (timeout <= 0) return -1;

    uint8_t buf[8];
    if (bme280_i2c_write_read(dev, BME280_REG_PRESS_MSB, buf, 8) != 0) return -1;

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
    int32_t adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];

    data->temperature_c = bme280_compensate_temperature(dev, adc_T);
    data->humidity_pct  = bme280_compensate_humidity(dev, adc_H);
    data->pressure_hpa  = bme280_compensate_pressure(dev, adc_P);

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
    dev->addr = addr;
    dev->initialized = 0;
    dev->t_fine = 0;

    int chip_id = bme280_i2c_read_reg(dev, BME280_REG_CHIP_ID);
    if (chip_id < 0 || (uint8_t)chip_id != BME280_CHIP_ID) return -1;

    if (bme280_reset(dev) != 0) return -1;
    bme280_delay_ms(10);

    if (bme280_load_calibration(dev) != 0) return -1;

    if (bme280_i2c_write_reg(dev, BME280_REG_CTRL_HUM, BME280_OS_1X) != 0) return -1;
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
    if (bme280_i2c_write_read(dev, BME280_REG_PRESS_MSB, buf, 8) != 0) return -1;

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
    int32_t adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];

    data->temperature_c = bme280_compensate_temperature(dev, adc_T);
    data->humidity_pct  = bme280_compensate_humidity(dev, adc_H);
    data->pressure_hpa  = bme280_compensate_pressure(dev, adc_P);

    return 0;
}

int bme280_sleep(bme280_t *dev) {
    if (!dev || !dev->initialized) return -1;
    return bme280_i2c_write_reg(dev, BME280_REG_CTRL_MEAS,
                        (BME280_OS_1X << 5) | (BME280_OS_1X << 2) | BME280_MODE_SLEEP);
}