/*
 * i2c_hal.h — I2C Hardware Abstraction Layer for Zynq PS
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * Provides a minimal I2C master interface for bare-metal Zynq applications.
 * On real hardware this wraps Xilinx XIicPs driver; for PC simulation it
 * provides stub implementations.
 */

#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* I2C instance handle (opaque — platform-specific internals) */
typedef struct {
    uint32_t base_addr;    /* I2C controller base address */
    uint32_t clk_freq_hz;  /* SCL clock frequency         */
    int      initialized;
} i2c_handle_t;

/* Zynq PS I2C controller base addresses */
#define ZYNQ_I2C0_BASEADDR  0xE0004000U
#define ZYNQ_I2C1_BASEADDR  0xE0005000U

/* Common I2C clock frequencies */
#define I2C_CLK_100KHZ  100000U
#define I2C_CLK_400KHZ  400000U

/*
 * Initialize an I2C controller.
 *   handle     — pointer to i2c_handle_t to initialize
 *   base_addr  — controller base address (ZYNQ_I2C0_BASEADDR or ZYNQ_I2C1_BASEADDR)
 *   clk_freq   — desired SCL frequency (I2C_CLK_100KHZ or I2C_CLK_400KHZ)
 * Returns 0 on success, -1 on failure.
 */
int i2c_init(i2c_handle_t *handle, uint32_t base_addr, uint32_t clk_freq);

/*
 * Write data to an I2C slave.
 *   handle    — initialized I2C handle
 *   slave_addr — 7-bit I2C slave address
 *   data      — pointer to data buffer to send
 *   len       — number of bytes to send
 * Returns 0 on success, -1 on failure/NACK.
 */
int i2c_write(const i2c_handle_t *handle, uint8_t slave_addr,
              const uint8_t *data, size_t len);

/*
 * Read data from an I2C slave.
 *   handle     — initialized I2C handle
 *   slave_addr — 7-bit I2C slave address
 *   data       — pointer to buffer to receive data
 *   len        — number of bytes to read
 * Returns 0 on success, -1 on failure/NACK.
 */
int i2c_read(const i2c_handle_t *handle, uint8_t slave_addr,
             uint8_t *data, size_t len);

/*
 * Write a register address then read data (combined write-read transaction).
 * This is the standard pattern for reading sensor registers.
 *   handle     — initialized I2C handle
 *   slave_addr — 7-bit I2C slave address
 *   reg_addr   — register address byte to write first
 *   data       — pointer to buffer to receive data
 *   len        — number of bytes to read
 * Returns 0 on success, -1 on failure.
 */
int i2c_write_read(const i2c_handle_t *handle, uint8_t slave_addr,
                   uint8_t reg_addr, uint8_t *data, size_t len);

/*
 * Write a single register (address + 1 data byte).
 * Returns 0 on success, -1 on failure.
 */
int i2c_write_reg(const i2c_handle_t *handle, uint8_t slave_addr,
                  uint8_t reg_addr, uint8_t value);

/*
 * Read a single register and return its value.
 * Returns register value on success, -1 on failure.
 */
int i2c_read_reg(const i2c_handle_t *handle, uint8_t slave_addr,
                 uint8_t reg_addr);

#ifdef __cplusplus
}
#endif

#endif /* I2C_HAL_H */
