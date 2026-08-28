/**
 * @file esp32_i2c_hal.h
 * @brief ESP32-S3 Hardware Abstraction Layer for I2C Sensors
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#ifndef ESP32_I2C_HAL_H
#define ESP32_I2C_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_HAL_SUCCESS  0
#define I2C_HAL_ERROR   -1
#define I2C_HAL_TIMEOUT -2

/**
 * @brief Initialize ESP32-S3 I2C Master Peripheral
 * @param sda_pin GPIO pin for SDA (default: GPIO 1)
 * @param scl_pin GPIO pin for SCL (default: GPIO 2)
 * @param clk_speed_hz I2C Clock frequency (e.g. 400000 for 400kHz Fast Mode)
 * @return 0 on success, negative error code on failure
 */
int esp32_i2c_hal_init(int sda_pin, int scl_pin, uint32_t clk_speed_hz);

/**
 * @brief Read sequential bytes from an I2C device register
 */
int esp32_i2c_hal_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Write sequential bytes to an I2C device register
 */
int esp32_i2c_hal_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len);

/**
 * @brief Write a single byte to an I2C device register
 */
int esp32_i2c_hal_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val);

/**
 * @brief Read a single byte from an I2C device register
 */
int esp32_i2c_hal_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_I2C_HAL_H */
