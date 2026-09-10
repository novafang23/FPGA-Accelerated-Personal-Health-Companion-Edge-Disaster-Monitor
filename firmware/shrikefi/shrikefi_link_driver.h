/**
 * @file shrikefi_link_driver.h
 * @brief High-Speed 4-Bit Parallel Link Driver for ESP32-S3 <-> Renesas ForgeFPGA
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 *
 * Provides cycle-accurate communication over the 4-bit parallel GPIO bus
 * between the ESP32-S3 MCU and the on-board Renesas ForgeFPGA accelerator.
 *
 * ELECTRICAL NOTICE:
 * All GPIOs operate at 3.3V LVCMOS.
 */

#ifndef SHRIKEFI_LINK_DRIVER_H
#define SHRIKEFI_LINK_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command Nibbles matching FPGA protocol */
#define SHRIKEFI_CMD_NOP          0x0
#define SHRIKEFI_CMD_WRITE_RED    0x1
#define SHRIKEFI_CMD_WRITE_IR     0x2
#define SHRIKEFI_CMD_WRITE_THRESH 0x3
#define SHRIKEFI_CMD_READ_RED     0x4
#define SHRIKEFI_CMD_READ_IR      0x5
#define SHRIKEFI_CMD_READ_IBI     0x6
#define SHRIKEFI_CMD_CLEAR_IRQ    0x7
#define SHRIKEFI_CMD_READ_STATUS  0x8

/* Pin configuration structure */
typedef struct {
    int pin_strobe;    /**< Strobe clock GPIO */
    int pin_dir;       /**< Direction GPIO (0=Write to FPGA, 1=Read from FPGA) */
    int pin_data[4];   /**< 4-bit bidirectional data GPIOs [D0, D1, D2, D3] */
    int pin_irq;       /**< Beat detected interrupt GPIO */
} shrikefi_pins_t;

typedef enum {
    SHRIKEFI_OK = 0,
    SHRIKEFI_ERR_INVALID_ARG = -1,
    SHRIKEFI_ERR_TIMEOUT = -2,
    SHRIKEFI_ERR_FPGA_NOT_DETECTED = -3, /**< No ACK at FORGEFPGA_I2C_ADDR (expected
                                          *   when the FPGA self-configures from
                                          *   OTP/NVM or onboard QSPI flash)      */
    SHRIKEFI_ERR_I2C_WRITE = -4,         /**< A device ACKed but the bitstream
                                          *   transfer failed part-way            */
    SHRIKEFI_ERR_BITSTREAM_DISABLED = -5 /**< The I2C bitstream path was compiled
                                          *   out (CONFIG_SHRIKEFI_ENABLE_I2C_
                                          *   BITSTREAM_FLASH is not set). This is
                                          *   the normal build.                   */
} shrikefi_err_t;

/**
 * @brief Best-effort load of the ForgeFPGA bitstream over I2C at boot.
 *
 * Compiled out by default to keep 46 KB of bitstream data out of the flash image
 * (see CONFIG_SHRIKEFI_ENABLE_I2C_BITSTREAM_FLASH in main/Kconfig.projbuild).
 *
 * The SLG47910 is an FPGA, not a GreenPAK CMIC: it is not documented to expose a
 * hard I2C configuration port, and this design's pin constraints
 * (hardware/shrikefi/forgefpga_pins.pcf) declare no I2C or SPI configuration
 * interface. On this board the FPGA most likely self-configures from OTP/NVM or
 * from the onboard W25Q32JV QSPI flash at power-up, which means the probe below
 * is *expected* to find nothing.
 *
 * This function therefore reports honestly instead of pretending:
 *   SHRIKEFI_OK                       - a device ACKed and the image was written
 *   SHRIKEFI_ERR_BITSTREAM_DISABLED   - the path is compiled out. Normal build.
 *   SHRIKEFI_ERR_FPGA_NOT_DETECTED    - nothing ACKed at FORGEFPGA_I2C_ADDR.
 *                                       NORMAL on this board. Boot must continue.
 *   SHRIKEFI_ERR_I2C_WRITE            - a device ACKed but the write failed.
 *
 * CALLERS MUST NOT TREAT A NON-OK RESULT AS FATAL. The 4-bit parallel link is the
 * runtime bus between the ESP32-S3 and the FPGA and does not depend on this call.
 *
 * When enabled, note the transfer still cannot address a 46 KB image correctly:
 * the HAL takes an 8-bit register address, so the offset wraps every 256 bytes.
 * Widening it without the ForgeFPGA I2C programming specification would only move
 * the guess, so the path is kept as-is and labelled unverified.
 */
shrikefi_err_t shrikefi_fpga_flash_init(void);

/**
 * @brief Initialize ESP32 GPIOs for the 4-bit ForgeFPGA parallel link
 * @param pins Pointer to pin configuration struct (or NULL for default pins)
 */
shrikefi_err_t shrikefi_link_init(const shrikefi_pins_t *pins);

/**
 * @brief Set systolic peak detection threshold on FPGA
 * @param threshold Threshold value (e.g. 120)
 */
shrikefi_err_t shrikefi_set_threshold(uint8_t threshold);

/**
 * @brief Write raw 8-bit Red PPG optical sample to FPGA filter
 * @param sample 8-bit raw sample
 */
shrikefi_err_t shrikefi_write_red_sample(uint8_t sample);

/**
 * @brief Write raw 8-bit IR PPG optical sample to FPGA filter
 * @param sample 8-bit raw sample
 */
shrikefi_err_t shrikefi_write_ir_sample(uint8_t sample);

/**
 * @brief Read filtered 8-bit Red PPG sample from FPGA
 * @return Filtered 8-bit sample
 */
uint8_t shrikefi_read_filtered_red(void);

/**
 * @brief Read filtered 8-bit IR PPG sample from FPGA
 * @return Filtered 8-bit sample
 */
uint8_t shrikefi_read_filtered_ir(void);

/**
 * @brief Read 32-bit cycle-accurate Inter-Beat Interval (IBI) from FPGA
 * @return Number of 50MHz clock cycles (20ns per cycle)
 */
uint32_t shrikefi_read_ibi_cycles(void);

/**
 * @brief Clear hardware beat interrupt flag on FPGA (Write-1-to-Clear)
 */
void shrikefi_clear_irq(void);

/**
 * @brief Check if hardware interrupt pin is currently asserted
 * @return true if beat detected
 */
bool shrikefi_is_beat_detected(void);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_LINK_DRIVER_H */
