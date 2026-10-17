/*
 * mpu6050.c
 * MPU6050 6-Axis IMU Sensor Driver (Accelerometer + Gyroscope)
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 */

#include "mpu6050.h"
#include <string.h>
#include <math.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MPU6050";

static void mpu6050_delay_ms(int ms) {
    if (ms <= 10) {
        esp_rom_delay_us(ms * 1000);
    } else {
        vTaskDelay(pdMS_TO_TICKS(ms) < 1 ? 1 : pdMS_TO_TICKS(ms));
    }
}
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
static const char *TAG __attribute__((unused)) = "MPU6050";

static void mpu6050_delay_ms(int ms) __attribute__((unused));
static void mpu6050_delay_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static int16_t s_mock_ax = 0;
static int16_t s_mock_ay = 0;
static int16_t s_mock_az = 4096; /* 1.0g at 4096 LSB/g (8g full scale) */
static int16_t s_mock_temp = 0;
static int16_t s_mock_gx = 0;
static int16_t s_mock_gy = 0;
static int16_t s_mock_gz = 0;

void mpu6050_set_mock_raw(int16_t ax, int16_t ay, int16_t az,
                          int16_t temp,
                          int16_t gx, int16_t gy, int16_t gz) {
    s_mock_ax = ax;
    s_mock_ay = ay;
    s_mock_az = az;
    s_mock_temp = temp;
    s_mock_gx = gx;
    s_mock_gy = gy;
    s_mock_gz = gz;
}
#endif

static int mpu6050_write_reg(mpu6050_t *dev, uint8_t reg, uint8_t val) {
    return esp32_i2c_hal_write_byte(dev->addr, reg, val);
}

#ifdef ESP_PLATFORM
static int mpu6050_write_config_reg(mpu6050_t *dev, uint8_t reg, uint8_t value) {
    if (mpu6050_write_reg(dev, reg, value) == I2C_HAL_SUCCESS) return 0;
    ESP_LOGE(TAG, "I2C write failed for register 0x%02X on MPU6050 0x%02X",
             reg, dev->addr);
    return -1;
}
#endif

static int mpu6050_read_reg(mpu6050_t *dev, uint8_t reg, uint8_t *val) __attribute__((unused));
static int mpu6050_read_reg(mpu6050_t *dev, uint8_t reg, uint8_t *val) {
    return esp32_i2c_hal_read_byte(dev->addr, reg, val);
}

static int mpu6050_read_bytes(mpu6050_t *dev, uint8_t reg, uint8_t *data, uint16_t len) __attribute__((unused));
static int mpu6050_read_bytes(mpu6050_t *dev, uint8_t reg, uint8_t *data, uint16_t len) {
    return esp32_i2c_hal_read(dev->addr, reg, data, len);
}

static void update_scale_factors(mpu6050_t *dev) {
    switch (dev->accel_fs) {
        case MPU6050_ACCEL_FS_2G:  dev->accel_scale = 1.0f / 16384.0f; break;
        case MPU6050_ACCEL_FS_4G:  dev->accel_scale = 1.0f / 8192.0f;  break;
        case MPU6050_ACCEL_FS_8G:  dev->accel_scale = 1.0f / 4096.0f;  break;
        case MPU6050_ACCEL_FS_16G: dev->accel_scale = 1.0f / 2048.0f;  break;
        default:                   dev->accel_scale = 1.0f / 4096.0f;  break;
    }

    switch (dev->gyro_fs) {
        case MPU6050_GYRO_FS_250DPS:  dev->gyro_scale = 1.0f / 131.0f; break;
        case MPU6050_GYRO_FS_500DPS:  dev->gyro_scale = 1.0f / 65.5f;  break;
        case MPU6050_GYRO_FS_1000DPS: dev->gyro_scale = 1.0f / 32.8f;  break;
        case MPU6050_GYRO_FS_2000DPS: dev->gyro_scale = 1.0f / 16.4f;  break;
        default:                      dev->gyro_scale = 1.0f / 16.4f;  break;
    }
}

int mpu6050_test_connection(mpu6050_t *dev) {
    if (!dev) return -1;
#ifndef ESP_PLATFORM
    return 0;
#else
    uint8_t who = 0;
    if (mpu6050_read_reg(dev, MPU6050_REG_WHO_AM_I, &who) != I2C_HAL_SUCCESS) {
        return -1;
    }
    /* WHO_AM_I bits [6:1] store 6-bit I2C address (0x34 << 1 = 0x68) */
    /* Accept MPU6050 (0x68) as well as MPU6500/MPU9250/ICM20600 family (0x70 - 0x75) */
    if ((who & 0x7E) != 0x68 && who != 0x68 &&
        who != 0x70 && who != 0x71 && who != 0x73 && who != 0x74 && who != 0x75) {
        ESP_LOGW(TAG, "WHO_AM_I mismatch: expected 0x68 or 0x70-0x75, got 0x%02X", who);
        return -2;
    }
    return 0;
#endif
}

int mpu6050_init(mpu6050_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr) {
    if (!dev) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->i2c = i2c;
    dev->addr = (addr != 0) ? addr : MPU6050_I2C_ADDR_DEFAULT;
    dev->accel_fs = MPU6050_ACCEL_FS_8G;   /* Default ±8g for fall detection */
    dev->gyro_fs  = MPU6050_GYRO_FS_2000DPS; /* Default ±2000 dps for fast tumbling */
    update_scale_factors(dev);

#ifdef ESP_PLATFORM
    /* 1. Reset device */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x80) != 0) return -1;
    mpu6050_delay_ms(50);

    /* 2. Wake up and select PLL with X-axis gyroscope clock reference */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x01) != 0) {
        ESP_LOGE(TAG, "Failed to wake MPU6050 at 0x%02X", dev->addr);
        return -1;
    }
    mpu6050_delay_ms(10);

    /* 3. Verify communication via WHO_AM_I */
    if (mpu6050_test_connection(dev) != 0) {
        ESP_LOGE(TAG, "MPU6050 connection test failed at 0x%02X", dev->addr);
        return -1;
    }

    /* 4. Set Sample Rate Divider: 1kHz / (1 + 4) = 200 Hz internal sample rate */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_SMPLRT_DIV, 0x04) != 0) return -1;

    /* 5. Set DLPF (Digital Low Pass Filter) to 44 Hz (filters out mechanical hum & motor vibration) */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_CONFIG,
                                 (uint8_t)MPU6050_DLPF_44HZ) != 0) return -1;

    /* 6. Configure Gyroscope FS to ±2000 dps */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_GYRO_CONFIG,
                                 (uint8_t)dev->gyro_fs) != 0) return -1;

    /* 7. Configure Accelerometer FS to ±8g */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_ACCEL_CONFIG,
                                 (uint8_t)dev->accel_fs) != 0) return -1;

    /* 8. Enable FIFO / Interrupt configuration if needed (default: bypass, polled) */
    if (mpu6050_write_config_reg(dev, MPU6050_REG_INT_PIN_CFG, 0x00) != 0) return -1;
    if (mpu6050_write_config_reg(dev, MPU6050_REG_INT_ENABLE, 0x00) != 0) return -1;

    ESP_LOGI(TAG, "MPU6050 initialized OK at 0x%02X (Accel: +/-8g, Gyro: +/-2000 dps, DLPF: 44Hz)", dev->addr);
#endif

    dev->initialized = 1;
    return 0;
}

int mpu6050_set_accel_fs(mpu6050_t *dev, mpu6050_accel_fs_t fs) {
    if (!dev || !dev->initialized) return -1;
    dev->accel_fs = fs;
    update_scale_factors(dev);
    return mpu6050_write_reg(dev, MPU6050_REG_ACCEL_CONFIG, (uint8_t)fs);
}

int mpu6050_set_gyro_fs(mpu6050_t *dev, mpu6050_gyro_fs_t fs) {
    if (!dev || !dev->initialized) return -1;
    dev->gyro_fs = fs;
    update_scale_factors(dev);
    return mpu6050_write_reg(dev, MPU6050_REG_GYRO_CONFIG, (uint8_t)fs);
}

int mpu6050_set_dlpf(mpu6050_t *dev, mpu6050_dlpf_t dlpf) {
    if (!dev || !dev->initialized) return -1;
    return mpu6050_write_reg(dev, MPU6050_REG_CONFIG, (uint8_t)dlpf);
}

int mpu6050_sleep(mpu6050_t *dev) {
    if (!dev || !dev->initialized) return -1;
    /* Set SLEEP bit (bit 6) in PWR_MGMT_1 */
    return mpu6050_write_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x40);
}

int mpu6050_wake(mpu6050_t *dev) {
    if (!dev || !dev->initialized) return -1;
    /* Clear SLEEP bit and select PLL with X gyro */
    return mpu6050_write_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x01);
}

int mpu6050_read_raw(mpu6050_t *dev,
                     int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *temp,
                     int16_t *gx, int16_t *gy, int16_t *gz) {
    if (!dev || !dev->initialized) return -1;

#ifndef ESP_PLATFORM
    if (ax)   *ax   = s_mock_ax;
    if (ay)   *ay   = s_mock_ay;
    if (az)   *az   = s_mock_az;
    if (temp) *temp = s_mock_temp;
    if (gx)   *gx   = s_mock_gx;
    if (gy)   *gy   = s_mock_gy;
    if (gz)   *gz   = s_mock_gz;
    return 0;
#else
    uint8_t buf[14];
    if (mpu6050_read_bytes(dev, MPU6050_REG_ACCEL_XOUT_H, buf, 14) != I2C_HAL_SUCCESS) {
        return -1;
    }

    if (ax)   *ax   = (int16_t)(((uint16_t)buf[0]  << 8) | buf[1]);
    if (ay)   *ay   = (int16_t)(((uint16_t)buf[2]  << 8) | buf[3]);
    if (az)   *az   = (int16_t)(((uint16_t)buf[4]  << 8) | buf[5]);
    if (temp) *temp = (int16_t)(((uint16_t)buf[6]  << 8) | buf[7]);
    if (gx)   *gx   = (int16_t)(((uint16_t)buf[8]  << 8) | buf[9]);
    if (gy)   *gy   = (int16_t)(((uint16_t)buf[10] << 8) | buf[11]);
    if (gz)   *gz   = (int16_t)(((uint16_t)buf[12] << 8) | buf[13]);
    return 0;
#endif
}

int mpu6050_read(mpu6050_t *dev, mpu6050_data_t *data) {
    if (!dev || !dev->initialized || !data) return -1;

    int16_t raw_ax = 0, raw_ay = 0, raw_az = 0;
    int16_t raw_temp = 0;
    int16_t raw_gx = 0, raw_gy = 0, raw_gz = 0;

    if (mpu6050_read_raw(dev, &raw_ax, &raw_ay, &raw_az,
                         &raw_temp, &raw_gx, &raw_gy, &raw_gz) != 0) {
        return -1;
    }

    /* Convert to engineering units using single-precision floating point */
    data->ax = (float)raw_ax * dev->accel_scale;
    data->ay = (float)raw_ay * dev->accel_scale;
    data->az = (float)raw_az * dev->accel_scale;

    /* Temperature in °C per datasheet: Temp_C = (TEMP_OUT / 340.0) + 36.53 */
    data->temp_c = ((float)raw_temp / 340.0f) + 36.53f;

    data->gx = (float)raw_gx * dev->gyro_scale;
    data->gy = (float)raw_gy * dev->gyro_scale;
    data->gz = (float)raw_gz * dev->gyro_scale;

    /* Compute magnitudes: sqrtf() on ESP32-S3 uses single-precision hardware FPU */
    data->a_mag = sqrtf(data->ax * data->ax + data->ay * data->ay + data->az * data->az);
    data->g_mag = sqrtf(data->gx * data->gx + data->gy * data->gy + data->gz * data->gz);

    return 0;
}
