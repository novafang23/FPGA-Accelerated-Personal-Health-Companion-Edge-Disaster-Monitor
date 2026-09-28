/**
 * @file shrikefi_link_driver.h
 * @brief Full-duplex SPI2 link driver for ESP32-S3 <-> Renesas ForgeFPGA
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 *
 * Drives the 4-wire SPI link (mode 0, SPI2_HOST) between the ESP32-S3 and the
 * on-board Renesas ForgeFPGA, and delivers the FPGA bitstream over that same bus
 * at boot.
 *
 * One 8-bit full-duplex transaction per optical sample (~100 Hz): the MCU sends
 * the raw IR sample on MOSI and receives {beat_latched, filt_sample[6:0]} on
 * MISO. There is no command map, no strobe, no direction line, no address and no
 * interrupt pin - the beat is bit 7 of the returned byte, and the MCU derives
 * the IBI from its own esp_timer_get_time() deltas. Full specification:
 * docs/SHRIKEFI_LINK_PROTOCOL.md.
 *
 * Earlier revisions of this header declared a "4-bit parallel GPIO bus" with a
 * SHRIKEFI_CMD_* nibble codebook and a shrikefi_pins_t carrying a strobe, a
 * direction line and an irq pin. That design was retired before it was built,
 * and no constant in it was ever referenced by the driver. It has been removed
 * rather than left to describe a bus that does not exist.
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

typedef enum {
    SHRIKEFI_OK = 0,
    SHRIKEFI_ERR_INVALID_ARG = -1,
    SHRIKEFI_ERR_TIMEOUT = -2,
    SHRIKEFI_ERR_FPGA_NOT_DETECTED = -3, /**< Reserved. Was returned by the retired
                                          *   I2C probe path; the SPI2 programming
                                          *   sequence is open-loop and cannot
                                          *   detect an absent device.            */
    SHRIKEFI_ERR_SPI_WRITE = -4,         /**< SPI2 came up but the bitstream
                                          *   transfer failed part-way            */
    SHRIKEFI_ERR_BITSTREAM_DISABLED = -5 /**< Reserved. Returned by the retired
                                          *   build-time bitstream switch; the
                                          *   image is now always embedded.      */
} shrikefi_err_t;

/**
 * @brief Deliver the ForgeFPGA bitstream over SPI2 at boot.
 *
 * The image (46,408 bytes) is always embedded and always sent. The boot log
 * reaches "configuration COMPLETE! (46408 bytes loaded)". The 0x55 probe that
 * follows is NOT proof of anything: the RTL overwrites its response register
 * with {beat_latched, filt_sample[6:0]} on the clock after reset, so the reply
 * is 0x00 and the probe byte is never echoed. What proves a configured design is
 * running is the MISO line read showing ACTIVELY DRIVEN BY FPGA, and the
 * "[FPGA ACCEL] Systolic crest detected!" lines that follow once a finger is on
 * the sensor.
 *
 * The image is streamed in 256-byte chunks after the Vicharak
 * Web_FPGA_programmer.ino reset sequence (PWR=0/EN=0/SS=1 -> PWR=1/EN=1/SS=0,
 * SS toggled per chunk at 16 MHz, SPI mode 0).
 *
 * The sequence is OPEN-LOOP: SPI writes cannot tell us whether a ForgeFPGA is
 * actually listening, so SHRIKEFI_OK means "the bytes were clocked out", not
 * "the device confirmed receipt".
 *
 * Return values:
 *   SHRIKEFI_OK            - the image was clocked out over SPI2
 *   SHRIKEFI_ERR_TIMEOUT   - SPI2 bus/device setup or DMA alloc failed
 *   SHRIKEFI_ERR_SPI_WRITE - a chunk failed mid-transfer
 *
 * CALLERS MUST NOT TREAT A NON-OK RESULT AS FATAL. The SPI link is the runtime
 * bus between the ESP32-S3 and the FPGA and does not depend on this call.
 *
 * The ForgeFPGA pin constraints (hardware/shrikefi/forgefpga_pins.pcf) declare
 * the SPI link on PIN_16-19 but no I2C configuration port, so if the FPGA is
 * instead self-configuring from OTP/NVM or the onboard W25Q32JV QSPI flash, this
 * transfer is redundant rather than harmful.
 */
shrikefi_err_t shrikefi_fpga_flash_init(void);

/**
 * @brief Bring up the runtime SPI link on SPI2_HOST
 *
 * Takes no pin argument: the link pins are compile-time constants in
 * shrikefi_pinmap.h, and the bus is configured by shrikefi_fpga_flash_init().
 * (This used to accept a shrikefi_pins_t describing the retired parallel bus.
 * The argument was ignored by the implementation and the type is gone.)
 */
shrikefi_err_t shrikefi_link_init(void);

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
 * @brief Inter-beat interval, in 50 MHz cycles, since the previous beat
 *
 * NOT read from the FPGA. The RTL keeps a 32-bit cycle counter, but there is no
 * room for it in the 8-bit SPI reply and it is never transmitted. The driver
 * timestamps each rising beat flag with esp_timer_get_time() and differences
 * consecutive timestamps, converting to 50 MHz-equivalent cycles. The value is
 * numerically right (the 50 MHz factor cancels downstream) but its resolution is
 * the SPI poll cadence - about 10 ms at ~100 Hz - not the FPGA's 20 ns tick.
 */
uint32_t shrikefi_read_ibi_cycles(void);

/**
 * @brief Clear the latched beat flag
 *
 * There is no interrupt line and no write-1-to-clear register: the beat is bit 7
 * of the MISO byte and this clears the driver's latched copy of it.
 */
void shrikefi_clear_irq(void);

/**
 * @brief Has a beat been latched since the last shrikefi_clear_irq()?
 * @return true if a systolic crest has been reported
 *
 * Polled, not interrupt-driven - there is no beat interrupt pin on this board.
 */
bool shrikefi_is_beat_detected(void);

/* True while the FPGA is actually driving MISO - i.e. still configured and
 * executing. False means it has stopped, which reading the SPI replies cannot
 * detect, because a frozen chip answers from a frozen register with a constant
 * byte that looks like a valid zero sample. */
bool shrikefi_link_fpga_alive(void);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_LINK_DRIVER_H */
