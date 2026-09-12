/**
 * @file shrikefi_link_driver.c
 * @brief Implementation of full-duplex SPI link for ESP32-S3 <-> ForgeFPGA SLG47910
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#include "shrikefi_link_driver.h"
#include "shrikefi_pinmap.h"
#include "forgefpga_bitstream.h"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#define DELAY_NS() esp_rom_delay_us(1)
#else
#include <stdio.h>
#include <string.h>
#define DELAY_NS() ((void)0)
#define ESP_LOGI(tag, ...) do {} while(0)
#define ESP_LOGE(tag, ...) do {} while(0)
#define ESP_LOGW(tag, ...) do {} while(0)
#endif

#include "esp32_i2c_hal.h"

static const char* LINK_TAG __attribute__((unused)) = "SHRIKEFI_LINK";

#ifdef ESP_PLATFORM
static spi_device_handle_t s_spi_runtime = NULL;
static uint8_t  s_last_filtered_ir = 0;
static uint8_t  s_last_filtered_red = 0;
static bool     s_beat_detected_latched = false;
static bool     s_last_beat = false;
static uint64_t s_last_beat_time_us = 0;
static uint32_t s_sim_ibi = 3280;
static uint8_t  s_sim_reg_th = 120;
#else
static uint8_t  s_sim_reg_th = 120;
static uint8_t  s_sim_reg_red = 0;
static uint8_t  s_sim_reg_ir = 0;
static uint32_t s_sim_ibi = 3280;
static bool     s_sim_irq = false;
#endif

static bool s_initialized = false;

shrikefi_err_t shrikefi_link_init(const shrikefi_pins_t *pins) {
    (void)pins;

#ifdef ESP_PLATFORM
    /* Keep SS de-asserted (HIGH) for runtime SPI communication */
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);

    /* Initialize runtime SPI device on SPI2_HOST (1 MHz, Manual CS on PIN_FPGA_SS) */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, /* 1 MHz full-duplex runtime link */
        .mode = 0,                  /* Mode 0 (CPOL=0, CPHA=0) */
        .spics_io_num = -1,         /* Manual CS control via gpio_set_level */
        .queue_size = 1,
    };

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_runtime);
    if (ret != ESP_OK) {
        ESP_LOGW(LINK_TAG, "Runtime SPI device add returned %d (bus already active)", ret);
    } else {
        ESP_LOGI(LINK_TAG, "ForgeFPGA runtime SPI link active on SPI2 (1 MHz, CS=GPIO%d, MISO=GPIO%d, MOSI=GPIO%d, SCK=GPIO%d)",
                 PIN_FPGA_SS, PIN_FPGA_MISO, PIN_FPGA_MOSI, PIN_FPGA_SCK);

        /* Perform liveness handshake over MISO with active-low SS assert */
        spi_transaction_t probe_t;
        memset(&probe_t, 0, sizeof(probe_t));
        probe_t.length = 8;
        probe_t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
        probe_t.tx_data[0] = 0x55;

        gpio_set_level((gpio_num_t)PIN_FPGA_SS, 0);
        esp_err_t probe_ret = spi_device_transmit(s_spi_runtime, &probe_t);
        gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);

        if (probe_ret == ESP_OK) {
            uint8_t probe_rx = probe_t.rx_data[0];
            ESP_LOGI(LINK_TAG, "ForgeFPGA runtime link handshake: probe sent 0x55, received 0x%02X", probe_rx);
        }

        /* Test physical MISO drive state (overpowers weak pull-up/pull-down if actively driven by FPGA) */
        gpio_set_pull_mode((gpio_num_t)PIN_FPGA_MISO, GPIO_PULLDOWN_ONLY);
        esp_rom_delay_us(100);
        int pd_val = gpio_get_level((gpio_num_t)PIN_FPGA_MISO);

        gpio_set_pull_mode((gpio_num_t)PIN_FPGA_MISO, GPIO_PULLUP_ONLY);
        esp_rom_delay_us(100);
        int pu_val = gpio_get_level((gpio_num_t)PIN_FPGA_MISO);

        gpio_set_pull_mode((gpio_num_t)PIN_FPGA_MISO, GPIO_FLOATING);
        ESP_LOGI(LINK_TAG, "MISO Pin 13 Physical Line Test: Pulldown=%d, Pullup=%d (%s)",
                 pd_val, pu_val, (pd_val == pu_val) ? "ACTIVELY DRIVEN BY FPGA" : "FLOATING/TRISTATE");
    }
#endif

    s_initialized = true;
    return SHRIKEFI_OK;
}

shrikefi_err_t shrikefi_set_threshold(uint8_t threshold) {
    s_sim_reg_th = threshold;
    return SHRIKEFI_OK;
}

shrikefi_err_t shrikefi_write_red_sample(uint8_t sample) {
#ifdef ESP_PLATFORM
    s_last_filtered_red = sample;
#else
    s_sim_reg_red = sample;
#endif
    return SHRIKEFI_OK;
}

shrikefi_err_t shrikefi_write_ir_sample(uint8_t sample) {
#ifdef ESP_PLATFORM
    if (s_spi_runtime == NULL) {
        return SHRIKEFI_OK;
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.tx_data[0] = sample;

    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 0);
    esp_err_t ret = spi_device_transmit(s_spi_runtime, &t);
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);

    if (ret == ESP_OK) {
        uint8_t rx = t.rx_data[0];
        uint8_t tx = sample;
        /* Log roughly 4 times per second, indefinitely.
         * This was capped at the first 10 samples, which all occur within the
         * first 0.1 s of boot with no finger on the sensor - so the FPGA's reply
         * was never observable during an actual measurement, which is precisely
         * when it matters.
         *
         * TEMPORARY DIAGNOSTIC: logs EVERY transaction rather than a decimated
         * one, so the waveform the FPGA receives (tx) and returns (filtered) can
         * be reconstructed offline at the full 100 Hz sample rate. This is what
         * settles why the peak detector fires twice ~330 ms apart: ONE peak per
         * cardiac cycle in filtered means the split is digital (the SPI
         * beat-flag path); TWO means it is in the AC scaling or the 8-tap
         * average. Costs ~2.5 KB/s on the console. Revert to the decimated
         * (% 25) form once the cause is identified. */
        static int s_dbg_cnt = 0;
        printf("FG %d %u %u %u\n", s_dbg_cnt++, tx, rx & 0x7F, (rx >> 7) & 1);
        fflush(stdout);
        s_last_filtered_ir = rx & 0x7F;
        bool beat = ((rx >> 7) & 1) != 0;
        if (beat && !s_last_beat) {
            uint64_t now_us = esp_timer_get_time();
            if (s_last_beat_time_us > 0) {
                uint32_t delta_us = (uint32_t)(now_us - s_last_beat_time_us);
                s_sim_ibi = delta_us * 50; /* 50 MHz cycles */
            }
            s_last_beat_time_us = now_us;
            s_beat_detected_latched = true;
            ESP_LOGI(LINK_TAG, "[FPGA ACCEL] Systolic crest detected! Filtered=%d | Pin 16 Blue LED pulsing", s_last_filtered_ir);
        }
        s_last_beat = beat;
    }
#else
    s_sim_reg_ir = sample;
#endif
    return SHRIKEFI_OK;
}

uint8_t shrikefi_read_filtered_red(void) {
#ifdef ESP_PLATFORM
    return s_last_filtered_red;
#else
    return s_sim_reg_red;
#endif
}

uint8_t shrikefi_read_filtered_ir(void) {
#ifdef ESP_PLATFORM
    return s_last_filtered_ir;
#else
    return s_sim_reg_ir;
#endif
}

uint32_t shrikefi_read_ibi_cycles(void) {
#ifdef ESP_PLATFORM
    return s_sim_ibi;
#else
    return s_sim_ibi;
#endif
}

void shrikefi_clear_irq(void) {
#ifdef ESP_PLATFORM
    s_beat_detected_latched = false;
#else
    s_sim_irq = false;
#endif
}

bool shrikefi_is_beat_detected(void) {
#ifdef ESP_PLATFORM
    return s_beat_detected_latched;
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
        .clock_speed_hz = 16000000, /* 16 MHz flashing (Official Web_FPGA_programmer.ino SPI_CLOCK) */
        .mode = 0,                  /* SPI Mode 0 */
        .spics_io_num = -1,         /* Manual CS control during bitstream boot */
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

    /* 3. Reset FPGA per Vicharak official Web_FPGA_programmer.ino reference:
     *    PWR=0, EN=0, SS=1 -> delay 3ms
     *    PWR=1, EN=1, SS=0 -> delay 10ms (boot mode latch)
     *    SS=1              -> delay 1ms
     */
    gpio_set_level((gpio_num_t)PIN_FPGA_PWR, 0);
    gpio_set_level((gpio_num_t)PIN_FPGA_EN, 0);
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);
    vTaskDelay(pdMS_TO_TICKS(5));

    gpio_set_level((gpio_num_t)PIN_FPGA_PWR, 1);
    gpio_set_level((gpio_num_t)PIN_FPGA_EN, 1);
    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 0);
    vTaskDelay(pdMS_TO_TICKS(15));

    gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);
    vTaskDelay(pdMS_TO_TICKS(2));

    /* 4. Stream bitstream in 256-byte chunks with SS toggling LOW/HIGH per chunk */
    uint8_t *dma_chunk = (uint8_t *)heap_caps_malloc(256, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!dma_chunk) {
        ESP_LOGE(LINK_TAG, "Failed to allocate 256B DMA chunk buffer!");
        spi_bus_remove_device(spi);
        return SHRIKEFI_ERR_TIMEOUT;
    }

    uint32_t offset = 0;
    while (offset < forgefpga_bitstream_length) {
        uint32_t chunk_len = forgefpga_bitstream_length - offset;
        if (chunk_len > 256) chunk_len = 256;

        memcpy(dma_chunk, &forgefpga_bitstream[offset], chunk_len);

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = chunk_len * 8; /* in bits */
        t.tx_buffer = dma_chunk;
        t.rx_buffer = NULL;

        gpio_set_level((gpio_num_t)PIN_FPGA_SS, 0);
        ret = spi_device_transmit(spi, &t);
        gpio_set_level((gpio_num_t)PIN_FPGA_SS, 1);

        if (ret != ESP_OK) {
            ESP_LOGE(LINK_TAG, "Bitstream transmission failed at offset %lu (err %d)",
                     (unsigned long)offset, ret);
            free(dma_chunk);
            spi_bus_remove_device(spi);
            return SHRIKEFI_ERR_I2C_WRITE;
        }

        offset += chunk_len;
    }

    /* 5. Allow FPGA to start User Mode */
    free(dma_chunk);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(LINK_TAG, "ForgeFPGA SLG47910 configuration COMPLETE! (%lu bytes loaded)",
             (unsigned long)forgefpga_bitstream_length);

    /* Release bitstream flasher device from bus, preserving SPI2_HOST for runtime communication */
    spi_bus_remove_device(spi);

    return SHRIKEFI_OK;
#else
    /* Host / simulation build: no FPGA to configure. */
    return SHRIKEFI_OK;
#endif
}
