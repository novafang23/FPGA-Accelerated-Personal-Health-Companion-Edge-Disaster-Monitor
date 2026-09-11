/**
 * @file shrikefi_link_driver.c
 * @brief Implementation of 4-bit parallel GPIO link for ESP32-S3 <-> ForgeFPGA
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#include "shrikefi_link_driver.h"
#include "shrikefi_pinmap.h"
#include "forgefpga_bitstream.h"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#include "driver/spi_master.h"
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

#include "esp32_i2c_hal.h"

static const char* LINK_TAG __attribute__((unused)) = "SHRIKEFI_LINK";

/* Default ShrikeFi Pin Mapping — Official Vicharak Shrike-Fi Internal Bus */
static shrikefi_pins_t s_pins = {
    .pin_strobe = PIN_FPGA_SCK,   /* GPIO 12: Link Clock */
    .pin_dir    = PIN_FPGA_SS,    /* GPIO 10: Direction / Data Bit 0 */
    .pin_data   = {PIN_FPGA_SS, PIN_FPGA_MOSI, PIN_FPGA_SCK, PIN_FPGA_MISO}, /* GPIO 10-13 */
    .pin_irq    = PIN_FPGA_MISO   /* GPIO 13: Beat Interrupt / Status */
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
    /* Keep FPGA powered up and hardware enabled */
    gpio_config_t pwr_en_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_FPGA_PWR) | (1ULL << PIN_FPGA_EN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&pwr_en_conf);
    gpio_set_level((gpio_num_t)PIN_FPGA_PWR, 1);
    gpio_set_level((gpio_num_t)PIN_FPGA_EN, 1);

    gpio_config_t out_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << s_pins.pin_strobe) | (1ULL << s_pins.pin_dir),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&out_conf);

    gpio_config_t irq_conf = {
        .intr_type = GPIO_INTR_DISABLE,
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
    ESP_LOGI(LINK_TAG, "=========================================================");
    ESP_LOGI(LINK_TAG, "  Programming Renesas ForgeFPGA SLG47910 via SPI2        ");
    ESP_LOGI(LINK_TAG, "  Bitstream: FPGA_bitstream_MCU.bin (%lu bytes)          ",
             (unsigned long)forgefpga_bitstream_length);
    ESP_LOGI(LINK_TAG, "  Internal Shrike-Fi Pins: PWR=%d EN=%d SS=%d MOSI=%d SCK=%d MISO=%d",
             PIN_FPGA_PWR, PIN_FPGA_EN, PIN_FPGA_SS, PIN_FPGA_MOSI, PIN_FPGA_SCK, PIN_FPGA_MISO);
    ESP_LOGI(LINK_TAG, "=========================================================");

    /* 1. Configure PWR, EN, SS as outputs */
    gpio_config_t out_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_FPGA_PWR) | (1ULL << PIN_FPGA_EN) | (1ULL << PIN_FPGA_SS),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&out_conf);

    /* 2. Initialize SPI bus on SPI2_HOST */
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_FPGA_MISO,
        .mosi_io_num = PIN_FPGA_MOSI,
        .sclk_io_num = PIN_FPGA_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 16000000, /* 16 MHz */
        .mode = 0,                  /* SPI Mode 0 */
        .spics_io_num = -1,         /* Manual CS control */
        .queue_size = 1,
    };

    spi_device_handle_t spi;
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(LINK_TAG, "SPI bus initialize failed (err %d)", ret);
        return SHRIKEFI_ERR_TIMEOUT;
    }

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(LINK_TAG, "SPI bus add device failed (err %d)", ret);
        return SHRIKEFI_ERR_TIMEOUT;
    }

    /* 3. Power-up & Reset Sequence (per Vicharak shrike-rs universal flasher specification) */
    /* Step A: Reset: low PWR, high EN */
    gpio_set_level((gpio_num_t)PIN_FPGA_PWR, 0);
    gpio_set_level((gpio_num_t)PIN_FPGA_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Step B: Power down everything */
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 0);
    gpio_set_level((gpio_num_t)PIN_FPGA_EN, 0);
    gpio_set_level((gpio_num_t)PIN_FPGA_PWR, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Step C: Enable and Power Up */
    gpio_set_level((gpio_num_t)PIN_FPGA_EN, 1);
    gpio_set_level((gpio_num_t)PIN_FPGA_PWR, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Step D: Assert SS (Active Low) for SPI transfer */
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);
    esp_rom_delay_us(2000);
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 0);

    /* 4. Transfer bitstream in 4096-byte DMA chunks */
    uint32_t offset = 0;
    while (offset < forgefpga_bitstream_length) {
        uint32_t chunk_len = forgefpga_bitstream_length - offset;
        if (chunk_len > 4096) chunk_len = 4096;

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = chunk_len * 8; /* in bits */
        t.tx_buffer = &forgefpga_bitstream[offset];
        t.rx_buffer = NULL;

        ret = spi_device_transmit(spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(LINK_TAG, "Bitstream transmission failed at offset %lu (err %d)",
                     (unsigned long)offset, ret);
            gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);
            spi_bus_remove_device(spi);
            spi_bus_free(SPI2_HOST);
            return SHRIKEFI_ERR_I2C_WRITE;
        }

        offset += chunk_len;
    }

    /* 5. De-assert SS to finalize FPGA boot */
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(LINK_TAG, "ForgeFPGA SLG47910 configuration COMPLETE! (%lu bytes loaded)",
             (unsigned long)forgefpga_bitstream_length);

    /* Release SPI master driver so pins can transition to runtime 4-bit bus mode */
    spi_bus_remove_device(spi);
    spi_bus_free(SPI2_HOST);

    return SHRIKEFI_OK;
#else
    /* Host / simulation build: no FPGA to configure. */
    return SHRIKEFI_OK;
#endif
}
