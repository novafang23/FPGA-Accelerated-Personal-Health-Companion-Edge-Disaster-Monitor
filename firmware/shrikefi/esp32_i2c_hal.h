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

/* Opaque I2C handle for sensor drivers */
#ifdef ESP_PLATFORM
#include "driver/i2c.h"
typedef struct {
    i2c_port_t port;
    int initialized;
} esp32_i2c_handle_t;
#else
typedef void esp32_i2c_handle_t;
#endif

/**
 * @brief Get the internal I2C handle for sensor drivers
 * @return Pointer to I2C handle, or NULL if not initialized
 */
esp32_i2c_handle_t* esp32_i2c_hal_get_handle(void);

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
 * @brief Write raw bytes directly to an I2C device without a register prefix
 */
int esp32_i2c_hal_write_raw(uint8_t dev_addr, const uint8_t *data, uint16_t len);

/**
 * @brief Write a single byte to an I2C device register
 */
int esp32_i2c_hal_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val);

/**
 * @brief Read a single byte from an I2C device register
 */
int esp32_i2c_hal_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val);

/**
 * @brief Check if an I2C device exists at given address (returns 0 if ACKed, negative if NACK)
 */
int esp32_i2c_hal_probe(uint8_t dev_addr);

/**
 * @brief Scan all I2C addresses (0x01 to 0x7E) and log active devices
 */
void esp32_i2c_hal_scan(void);

/**
 * @brief Issue 9 recovery clock pulses and a STOP condition to free a stuck I2C bus
 * @return 0 on success, negative error code on failure
 */
int esp32_i2c_hal_recover_bus(void);

/**
 * @brief Acquire recursive I2C bus lock for atomic multi-transaction sequences
 */
void esp32_i2c_hal_lock(void);

/**
 * @brief Release recursive I2C bus lock
 */
void esp32_i2c_hal_unlock(void);

#ifndef ESP_PLATFORM
void esp32_i2c_hal_sim_set_max30102_temp(int8_t tint, uint8_t tfrac);
void esp32_i2c_hal_sim_set_max30102_fifo_ptrs(uint8_t wr, uint8_t ovf, uint8_t rd);
void esp32_i2c_hal_sim_set_max30102_mode(uint8_t mode);
uint8_t esp32_i2c_hal_sim_get_max30102_mode(void);
uint32_t esp32_i2c_hal_sim_get_max30102_mode_writes(void);
uint32_t esp32_i2c_hal_sim_get_raw_writes(void);
uint32_t esp32_i2c_hal_sim_get_raw_bytes(void);
void esp32_i2c_hal_sim_reset_counters(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ESP32_I2C_HAL_H */
