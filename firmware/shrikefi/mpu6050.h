/*
 * mpu6050.h
 * MPU6050 6-Axis IMU Sensor Driver (Accelerometer + Gyroscope)
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>
#include "esp32_i2c_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C Addresses (AD0 pin: GND -> 0x68, 3.3V -> 0x69) */
#define MPU6050_I2C_ADDR_DEFAULT  0x68
#define MPU6050_I2C_ADDR_ALT      0x69
#define MPU6050_I2C_ADDR          MPU6050_I2C_ADDR_DEFAULT

/* Device Identification */
#define MPU6050_WHO_AM_I_VAL      0x68

/* Key Register Map */
#define MPU6050_REG_SMPLRT_DIV    0x19
#define MPU6050_REG_CONFIG        0x1A
#define MPU6050_REG_GYRO_CONFIG   0x1B
#define MPU6050_REG_ACCEL_CONFIG  0x1C
#define MPU6050_REG_FIFO_EN       0x23
#define MPU6050_REG_INT_PIN_CFG   0x37
#define MPU6050_REG_INT_ENABLE    0x38
#define MPU6050_REG_INT_STATUS    0x3A
#define MPU6050_REG_ACCEL_XOUT_H  0x3B
#define MPU6050_REG_ACCEL_XOUT_L  0x3C
#define MPU6050_REG_ACCEL_YOUT_H  0x3D
#define MPU6050_REG_ACCEL_YOUT_L  0x3E
#define MPU6050_REG_ACCEL_ZOUT_H  0x3F
#define MPU6050_REG_ACCEL_ZOUT_L  0x40
#define MPU6050_REG_TEMP_OUT_H    0x41
#define MPU6050_REG_TEMP_OUT_L    0x42
#define MPU6050_REG_GYRO_XOUT_H   0x43
#define MPU6050_REG_GYRO_XOUT_L   0x44
#define MPU6050_REG_GYRO_YOUT_H   0x45
#define MPU6050_REG_GYRO_YOUT_L   0x46
#define MPU6050_REG_GYRO_ZOUT_H   0x47
#define MPU6050_REG_GYRO_ZOUT_L   0x48
#define MPU6050_REG_USER_CTRL     0x6A
#define MPU6050_REG_PWR_MGMT_1    0x6B
#define MPU6050_REG_PWR_MGMT_2    0x6C
#define MPU6050_REG_WHO_AM_I      0x75

/* Accelerometer Full Scale Ranges */
typedef enum {
    MPU6050_ACCEL_FS_2G  = 0x00, /* +/- 2g  (16384 LSB/g) */
    MPU6050_ACCEL_FS_4G  = 0x08, /* +/- 4g  (8192 LSB/g)  */
    MPU6050_ACCEL_FS_8G  = 0x10, /* +/- 8g  (4096 LSB/g)  - Recommended for fall impact */
    MPU6050_ACCEL_FS_16G = 0x18  /* +/- 16g (2048 LSB/g)  */
} mpu6050_accel_fs_t;

/* Gyroscope Full Scale Ranges */
typedef enum {
    MPU6050_GYRO_FS_250DPS  = 0x00, /* +/- 250 dps  (131.0 LSB/dps) */
    MPU6050_GYRO_FS_500DPS  = 0x08, /* +/- 500 dps  (65.5 LSB/dps)  */
    MPU6050_GYRO_FS_1000DPS = 0x10, /* +/- 1000 dps (32.8 LSB/dps)  */
    MPU6050_GYRO_FS_2000DPS = 0x18  /* +/- 2000 dps (16.4 LSB/dps)  - Recommended for falls */
} mpu6050_gyro_fs_t;

/* Digital Low Pass Filter (DLPF) Config */
typedef enum {
    MPU6050_DLPF_260HZ = 0x00,
    MPU6050_DLPF_184HZ = 0x01,
    MPU6050_DLPF_94HZ  = 0x02,
    MPU6050_DLPF_44HZ  = 0x03, /* Recommended for human motion & vibration filtering */
    MPU6050_DLPF_21HZ  = 0x04,
    MPU6050_DLPF_10HZ  = 0x05,
    MPU6050_DLPF_5HZ   = 0x06
} mpu6050_dlpf_t;

/* Calibrated Engineering Units Output */
typedef struct {
    float ax;      /* Acceleration X in g (1g = 9.80665 m/s^2) */
    float ay;      /* Acceleration Y in g */
    float az;      /* Acceleration Z in g */
    float gx;      /* Angular velocity X in degrees/sec (dps) */
    float gy;      /* Angular velocity Y in degrees/sec (dps) */
    float gz;      /* Angular velocity Z in degrees/sec (dps) */
    float temp_c;  /* Die temperature in degrees Celsius */
    float a_mag;   /* Total acceleration magnitude: sqrtf(ax^2 + ay^2 + az^2) in g */
    float g_mag;   /* Total angular rate magnitude: sqrtf(gx^2 + gy^2 + gz^2) in dps */
} mpu6050_data_t;

/* Driver Handle */
typedef struct {
    esp32_i2c_handle_t *i2c;
    uint8_t             addr;
    int                 initialized;
    mpu6050_accel_fs_t  accel_fs;
    mpu6050_gyro_fs_t   gyro_fs;
    float               accel_scale; /* LSB to g conversion */
    float               gyro_scale;  /* LSB to dps conversion */
} mpu6050_t;

/**
 * @brief Initialize MPU6050 IMU sensor
 * Configures PLL clock with X-gyro reference, sets default ±8g accel and ±2000 dps gyro,
 * and sets 44 Hz DLPF for noise immunity.
 * @param dev Pointer to driver instance
 * @param i2c Initialized I2C HAL handle
 * @param addr I2C address (MPU6050_I2C_ADDR_DEFAULT or MPU6050_I2C_ADDR_ALT)
 * @return 0 on success, negative error code on failure
 */
int mpu6050_init(mpu6050_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr);

/**
 * @brief Configure Accelerometer Full Scale Range
 */
int mpu6050_set_accel_fs(mpu6050_t *dev, mpu6050_accel_fs_t fs);

/**
 * @brief Configure Gyroscope Full Scale Range
 */
int mpu6050_set_gyro_fs(mpu6050_t *dev, mpu6050_gyro_fs_t fs);

/**
 * @brief Configure Digital Low Pass Filter (DLPF)
 */
int mpu6050_set_dlpf(mpu6050_t *dev, mpu6050_dlpf_t dlpf);

/**
 * @brief Read raw 14-byte sensor burst
 */
int mpu6050_read_raw(mpu6050_t *dev,
                     int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *temp,
                     int16_t *gx, int16_t *gy, int16_t *gz);

/**
 * @brief Read calibrated engineering unit data and total magnitudes
 * @param dev Initialized MPU6050 driver
 * @param data Output structure for acceleration, gyro, and temperature
 * @return 0 on success, negative on error
 */
int mpu6050_read(mpu6050_t *dev, mpu6050_data_t *data);

/**
 * @brief Test I2C connection and verify WHO_AM_I register (0x75 == 0x68)
 * @return 0 if device responded with valid WHO_AM_I, negative on error
 */
int mpu6050_test_connection(mpu6050_t *dev);

/**
 * @brief Put MPU6050 to sleep to conserve power
 */
int mpu6050_sleep(mpu6050_t *dev);

/**
 * @brief Wake MPU6050 from sleep
 */
int mpu6050_wake(mpu6050_t *dev);

#ifndef ESP_PLATFORM
/**
 * @brief Host test helper: inject mock raw sensor values for deterministic testing
 */
void mpu6050_set_mock_raw(int16_t ax, int16_t ay, int16_t az,
                          int16_t temp,
                          int16_t gx, int16_t gy, int16_t gz);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_H */
