/*
 * bme280.h
 * BME280 Temperature, Humidity & Pressure Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "esp32_i2c_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C Address (depends on SDO pin wiring) */
#define BME280_I2C_ADDR_LOW   0x76
#define BME280_I2C_ADDR_HIGH  0x77
#define BME280_I2C_ADDR       BME280_I2C_ADDR_LOW

/* Chip ID */
#define BME280_CHIP_ID        0x60

/* Register Map */
#define BME280_REG_CHIP_ID    0xD0
#define BME280_REG_RESET      0xE0
#define BME280_REG_CTRL_HUM   0xF2
#define BME280_REG_STATUS     0xF3
#define BME280_REG_CTRL_MEAS  0xF4
#define BME280_REG_CONFIG     0xF5

#define BME280_REG_PRESS_MSB  0xF7
#define BME280_REG_TEMP_MSB   0xFA
#define BME280_REG_HUM_MSB    0xFD

#define BME280_REG_CALIB00    0x88
#define BME280_REG_CALIB26    0xE1

/* Oversampling settings */
#define BME280_OS_SKIP  0x00
#define BME280_OS_1X    0x01
#define BME280_OS_2X    0x02
#define BME280_OS_4X    0x03
#define BME280_OS_8X    0x04
#define BME280_OS_16X   0x05

/* Operating modes */
#define BME280_MODE_SLEEP   0x00
#define BME280_MODE_FORCED  0x01
#define BME280_MODE_NORMAL  0x03

/* Standby time */
#define BME280_STANDBY_1000MS  0x05

/* IIR filter coefficient */
#define BME280_FILTER_OFF   0x00
#define BME280_FILTER_4     0x02

/* Calibration Data */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

/* Compensated Output */
typedef struct {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
} bme280_data_t;

/* Driver State */
typedef struct {
    esp32_i2c_handle_t *i2c;
    uint8_t addr;
    bme280_calib_t calib;
    int32_t t_fine;
    int initialized;
} bme280_t;

/*
 * Initialize the BME280 sensor.
 *   dev  — pointer to driver state
 *   i2c  — initialized ESP32 I2C handle
 *   addr — I2C address (BME280_I2C_ADDR_LOW or HIGH)
 * Returns 0 on success, -1 on failure.
 */
int bme280_init(bme280_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr);

/*
 * Trigger a forced measurement and read compensated data.
 *   dev  — initialized BME280 driver
 *   data — output structure for temperature, humidity, pressure
 * Returns 0 on success, -1 on error.
 */
int bme280_read(bme280_t *dev, bme280_data_t *data);

/*
 * Read only temperature (faster, skips humidity/pressure).
 * Returns temperature in °C, or -999.0f on error.
 */
float bme280_read_temperature(bme280_t *dev);

/* Soft-reset the sensor */
int bme280_reset(bme280_t *dev);

/* Initialize BME280 in normal (continuous) mode */
int bme280_init_normal_mode(bme280_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr,
                            uint8_t standby, uint8_t filter);

/* Read latest data from normal mode (non-blocking) */
int bme280_read_normal(bme280_t *dev, bme280_data_t *data);

/* Set sensor to sleep mode to save power */
int bme280_sleep(bme280_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H */