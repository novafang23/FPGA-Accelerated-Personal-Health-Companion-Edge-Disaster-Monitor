/**
 * @file shrikefi_link_driver.c
 * @brief Implementation of 4-bit parallel GPIO link for ESP32-S3 <-> ForgeFPGA
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#include "shrikefi_link_driver.h"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define DELAY_NS() esp_rom_delay_us(1)
#else
#include <stdio.h>
#define DELAY_NS() ((void)0)
#define ESP_LOGI(tag, ...) do {} while(0)
#define ESP_LOGE(tag, ...) do {} while(0)
#define ESP_LOGW(tag, ...) do {} while(0)
#endif

#include "forgefpga_bitstream.h"
#include "esp32_i2c_hal.h"

#define FORGEFPGA_I2C_ADDR 0x08

static const char* LINK_TAG = "SHRIKEFI_LINK";

/* Default ShrikeFi Pin Mapping */
static shrikefi_pins_t s_pins = {
    .pin_strobe = 4,   /* GPIO 4: Link Strobe */
    .pin_dir    = 5,   /* GPIO 5: Direction (0=Write, 1=Read) */
    .pin_data   = {6, 7, 8, 9}, /* GPIO 6-9: Data Bus D0-D3 */
    .pin_irq    = 10   /* GPIO 10: IRQ Beat */
};

static bool s_initialized = false;

#ifdef ESP_PLATFORM
static void set_bus_direction(bool read_mode) {
    gpio_set_level((gpio_num_t)s_pins.pin_dir, read_mode ? 1 : 0);
    for (int i = 0; i < 4; i++) {
        if (read_mode) {
            gpio_set_direction((gpio_num_t)s_pins.pin_data[i], GPIO_MODE_INPUT);
        } else {
            gpio_set_direction((gpio_num_t)s_pins.pin_data[i], GPIO_MODE_OUTPUT);
        }
    }
    DELAY_NS();
}

static void pulse_strobe(void) {
    gpio_set_level((gpio_num_t)s_pins.pin_strobe, 1);
    DELAY_NS();
    gpio_set_level((gpio_num_t)s_pins.pin_strobe, 0);
    DELAY_NS();
}

static void write_nibble(uint8_t nibble) {
    for (int i = 0; i < 4; i++) {
        gpio_set_level((gpio_num_t)s_pins.pin_data[i], (nibble >> i) & 1);
    }
    pulse_strobe();
}

static uint8_t read_nibble(void) {
    uint8_t val = 0;
    for (int i = 0; i < 4; i++) {
        val |= (gpio_get_level((gpio_num_t)s_pins.pin_data[i]) & 1) << i;
    }
    pulse_strobe();
    return val;
}
#else
/* Emulated registers for host test */
static uint8_t s_sim_reg_th = 120;
static uint8_t s_sim_reg_red = 0;
static uint8_t s_sim_reg_ir = 0;
static uint32_t s_sim_ibi = 3280;
static bool s_sim_irq = false;
#endif

shrikefi_err_t shrikefi_link_init(const shrikefi_pins_t *pins) {
    if (pins != NULL) {
        s_pins = *pins;
    }

#ifdef ESP_PLATFORM
    gpio_config_t out_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << s_pins.pin_strobe) | (1ULL << s_pins.pin_dir),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&out_conf);

    gpio_config_t irq_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << s_pins.pin_irq),
        .pull_down_en = 1,
        .pull_up_en = 0
    };
    gpio_config(&irq_conf);

    set_bus_direction(false);
    gpio_set_level((gpio_num_t)s_pins.pin_strobe, 0);
#endif

    s_initialized = true;
    return SHRIKEFI_OK;
}

shrikefi_err_t shrikefi_set_threshold(uint8_t threshold) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_WRITE_THRESH);
    write_nibble((threshold >> 4) & 0x0F);
    write_nibble(threshold & 0x0F);
#else
    s_sim_reg_th = threshold;
#endif
    return SHRIKEFI_OK;
}

shrikefi_err_t shrikefi_write_red_sample(uint8_t sample) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_WRITE_RED);
    write_nibble((sample >> 4) & 0x0F);
    write_nibble(sample & 0x0F);
#else
    s_sim_reg_red = sample;
#endif
    return SHRIKEFI_OK;
}

shrikefi_err_t shrikefi_write_ir_sample(uint8_t sample) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_WRITE_IR);
    write_nibble((sample >> 4) & 0x0F);
    write_nibble(sample & 0x0F);
#else
    s_sim_reg_ir = sample;
#endif
    return SHRIKEFI_OK;
}

uint8_t shrikefi_read_filtered_red(void) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_READ_RED);

    set_bus_direction(true);
    uint8_t high = read_nibble();
    uint8_t low = read_nibble();
    set_bus_direction(false);

    return (high << 4) | (low & 0x0F);
#else
    return s_sim_reg_red;
#endif
}

uint8_t shrikefi_read_filtered_ir(void) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_READ_IR);

    set_bus_direction(true);
    uint8_t high = read_nibble();
    uint8_t low = read_nibble();
    set_bus_direction(false);

    return (high << 4) | (low & 0x0F);
#else
    return s_sim_reg_ir;
#endif
}

uint32_t shrikefi_read_ibi_cycles(void) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_READ_IBI);

    set_bus_direction(true);
    uint32_t ibi = 0;
    for (int i = 0; i < 8; i++) {
        ibi = (ibi << 4) | (read_nibble() & 0x0F);
    }
    set_bus_direction(false);

    return ibi;
#else
    return s_sim_ibi;
#endif
}

void shrikefi_clear_irq(void) {
#ifdef ESP_PLATFORM
    set_bus_direction(false);
    write_nibble(SHRIKEFI_CMD_CLEAR_IRQ);
#else
    s_sim_irq = false;
#endif
}

bool shrikefi_is_beat_detected(void) {
#ifdef ESP_PLATFORM
    return gpio_get_level((gpio_num_t)s_pins.pin_irq) == 1;
#else
    return s_sim_irq;
#endif
}

shrikefi_err_t shrikefi_fpga_flash_init(void) {
#ifdef ESP_PLATFORM
    ESP_LOGI(LINK_TAG, "Probing ForgeFPGA configuration interface (I2C 0x%02X)...",
             FORGEFPGA_I2C_ADDR);

    /* Probe. A NACK here is the EXPECTED outcome on this board -- see the
     * function docs in shrikefi_link_driver.h. It means the FPGA configures
     * itself from OTP/NVM or onboard QSPI flash, which is the normal case. */
    uint8_t probe_byte = 0;
    if (esp32_i2c_hal_read_byte(FORGEFPGA_I2C_ADDR, 0x00, &probe_byte) != I2C_HAL_SUCCESS) {
        ESP_LOGI(LINK_TAG, "No I2C configuration interface at 0x%02X. ForgeFPGA is "
                           "expected to self-configure from OTP/NVM or onboard QSPI "
                           "flash. Continuing -- the 4-bit link is unaffected.",
                 FORGEFPGA_I2C_ADDR);
        return SHRIKEFI_ERR_FPGA_NOT_DETECTED;
    }

    /* Something answered. This is not expected, and the transfer below cannot be
     * correct for a 46 KB image because the HAL's register address is 8 bits and
     * wraps every 256 bytes. We attempt it rather than silently skipping, but say
     * so plainly. */
    ESP_LOGW(LINK_TAG, "Device ACKed at I2C 0x%02X -- attempting bitstream write "
                       "(%lu bytes). NOTE: this path is unverified; the 8-bit I2C "
                       "address field cannot address a 46 KB image correctly.",
             FORGEFPGA_I2C_ADDR, (unsigned long)forgefpga_bitstream_length);

    uint32_t offset = 0;
    while (offset < forgefpga_bitstream_length) {
        uint16_t chunk_size = 16;
        if (offset + chunk_size > forgefpga_bitstream_length) {
            chunk_size = (uint16_t)(forgefpga_bitstream_length - offset);
        }

        int err = esp32_i2c_hal_write(FORGEFPGA_I2C_ADDR, (uint8_t)(offset & 0xFF),
                                      &forgefpga_bitstream[offset], chunk_size);
        if (err != 0) {
            ESP_LOGE(LINK_TAG, "Bitstream write failed at offset %lu (err %d). "
                               "Configuration interface left as-is; continuing.",
                     (unsigned long)offset, err);
            return SHRIKEFI_ERR_I2C_WRITE;
        }

        offset += chunk_size;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(LINK_TAG, "ForgeFPGA bitstream transmitted (%lu bytes).",
             (unsigned long)forgefpga_bitstream_length);
#else
    /* Host / simulation build: no FPGA to configure. */
#endif
    return SHRIKEFI_OK;
}
