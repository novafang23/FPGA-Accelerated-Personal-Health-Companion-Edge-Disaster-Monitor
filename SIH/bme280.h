/*
 * bme280.h — BME280 Temperature, Humidity & Pressure Sensor Driver
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * Used for ambient environment monitoring during heat waves, floods, etc.
 * I2C Address: 0x76 (SDO=GND) or 0x77 (SDO=VDD)
 */

#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "i2c_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C Address (depends on SDO pin wiring) */
#define BME280_I2C_ADDR_LOW   0x76  /* SDO → GND */
#define BME280_I2C_ADDR_HIGH  0x77  /* SDO → VDD */
#define BME280_I2C_ADDR       BME280_I2C_ADDR_LOW  /* Default */

/* Chip ID */
#define BME280_CHIP_ID        0x60

/* Register Map */
#define BME280_REG_CHIP_ID    0xD0
#define BME280_REG_RESET      0xE0
#define BME280_REG_CTRL_HUM   0xF2
#define BME280_REG_STATUS     0xF3
#define BME280_REG_CTRL_MEAS  0xF4
#define BME280_REG_CONFIG     0xF5

/* Raw data registers (burst read 0xF7–0xFE) */
#define BME280_REG_PRESS_MSB  0xF7
#define BME280_REG_TEMP_MSB   0xFA
#define BME280_REG_HUM_MSB    0xFD

/* Calibration data registers */
#define BME280_REG_CALIB00    0x88  /* T1..T3, P1..P9 (26 bytes) */
#define BME280_REG_CALIB26    0xE1  /* H2..H6 (7 bytes)          */

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

/* Standby time (for normal mode) */
#define BME280_STANDBY_1000MS  0x05  /* 1 second between measurements */

/* IIR filter coefficient */
#define BME280_FILTER_OFF   0x00
#define BME280_FILTER_4     0x02

/* ---- Calibration Data ---- */
typedef struct {
    /* Temperature */
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    /* Pressure */
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    /* Humidity */
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

/* ---- Compensated Output ---- */
typedef struct {
    float temperature_c;   /* Degrees Celsius      */
    float humidity_pct;    /* Relative humidity (%) */
    float pressure_hpa;    /* Pressure (hPa)        */
} bme280_data_t;

/* ---- Driver State ---- */
typedef struct {
    i2c_handle_t    *i2c;
    uint8_t          addr;          /* Active I2C address          */
    bme280_calib_t   calib;         /* Factory calibration data    */
    int32_t          t_fine;        /* Fine temperature for compensation */
    int              initialized;
} bme280_t;

/*
 * Initialize the BME280 sensor.
 *   dev  — pointer to driver state
 *   i2c  — initialized I2C handle
 *   addr — I2C address (BME280_I2C_ADDR_LOW or HIGH)
 * Returns 0 on success, -1 on failure.
 */
int bme280_init(bme280_t *dev, i2c_handle_t *i2c, uint8_t addr);

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

/*
 * Soft-reset the sensor.
 */
int bme280_reset(bme280_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H */
