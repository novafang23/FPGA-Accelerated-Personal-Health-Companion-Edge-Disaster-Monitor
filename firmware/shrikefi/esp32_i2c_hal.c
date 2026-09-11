/**
 * @file esp32_i2c_hal.c
 * @brief ESP-IDF I2C Driver Implementation for ShrikeFi
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#include "esp32_i2c_hal.h"

#ifdef ESP_PLATFORM
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG __attribute__((unused)) = "I2C_HAL";
static i2c_port_t s_i2c_num = I2C_NUM_0;
static esp32_i2c_handle_t s_i2c_handle = { .port = I2C_NUM_0, .initialized = 0 };
static int s_sda_pin = 1;
static int s_scl_pin = 2;

int esp32_i2c_hal_init(int sda_pin, int scl_pin, uint32_t clk_speed_hz) {
    s_sda_pin = sda_pin;
    s_scl_pin = scl_pin;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed_hz,
    };
    esp_err_t err = i2c_param_config(s_i2c_num, &conf);
    if (err != ESP_OK) return I2C_HAL_ERROR;
    err = i2c_driver_install(s_i2c_num, conf.mode, 0, 0, 0);
    if (err == ESP_OK) {
        s_i2c_handle.initialized = 1;
        return I2C_HAL_SUCCESS;
    }
    return I2C_HAL_ERROR;
}

esp32_i2c_handle_t* esp32_i2c_hal_get_handle(void) {
    return s_i2c_handle.initialized ? &s_i2c_handle : NULL;
}

int esp32_i2c_hal_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    esp_err_t err = i2c_master_write_read_device(s_i2c_num, dev_addr, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len) {
    uint8_t buf[256];
    if (len + 1 > sizeof(buf)) return I2C_HAL_ERROR;
    buf[0] = reg_addr;
    for (int i = 0; i < len; i++) buf[i + 1] = data[i];
    esp_err_t err = i2c_master_write_to_device(s_i2c_num, dev_addr, buf, len + 1, 1000 / portTICK_PERIOD_MS);
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_write_raw(uint8_t dev_addr, const uint8_t *data, uint16_t len) {
    esp_err_t err = i2c_master_write_to_device(s_i2c_num, dev_addr, data, len, 1000 / portTICK_PERIOD_MS);
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val) {
    return esp32_i2c_hal_write(dev_addr, reg_addr, &val, 1);
}

int esp32_i2c_hal_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val) {
    return esp32_i2c_hal_read(dev_addr, reg_addr, val, 1);
}

int esp32_i2c_hal_probe(uint8_t dev_addr) {
    /* ESP-IDF v5.x rejects NULL data in i2c_master_write_to_device().
     * Use the cmd-link API to send just an address byte for probing. */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(s_i2c_num, cmd, 50 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

void esp32_i2c_hal_scan(void) {
    ESP_LOGI("I2C_SCAN", "Scanning I2C bus (SDA=GPIO%d, SCL=GPIO%d)...", s_sda_pin, s_scl_pin);
    int count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (esp32_i2c_hal_probe(addr) == I2C_HAL_SUCCESS) {
            const char *desc = "Unknown";
            if (addr == 0x3C || addr == 0x3D) desc = "SSD1306 OLED";
            else if (addr == 0x57) desc = "MAX30100/MAX30102 PPG";
            else if (addr == 0x76 || addr == 0x77) desc = "BME280 Env";
            else if (addr == 0x08) desc = "ForgeFPGA";
            ESP_LOGI("I2C_SCAN", " -> Found device at 0x%02X (%s)", addr, desc);
            count++;
        }
    }
    if (count == 0) {
        ESP_LOGW("I2C_SCAN", "No I2C devices responded! Check wiring/pullups.");
    } else {
        ESP_LOGI("I2C_SCAN", "Scan complete: %d device(s) found.", count);
    }
}

#else
/* Stub simulation implementation for PC tests */
#include <string.h>

int esp32_i2c_hal_init(int sda_pin, int scl_pin, uint32_t clk_speed_hz) {
    (void)sda_pin; (void)scl_pin; (void)clk_speed_hz;
    return I2C_HAL_SUCCESS;
}

esp32_i2c_handle_t* esp32_i2c_hal_get_handle(void) {
    return NULL;
}

int esp32_i2c_hal_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    (void)dev_addr; (void)reg_addr;
    memset(data, 0, len);
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len) {
    (void)dev_addr; (void)reg_addr; (void)data; (void)len;
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_write_raw(uint8_t dev_addr, const uint8_t *data, uint16_t len) {
    (void)dev_addr; (void)data; (void)len;
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val) {
    (void)dev_addr; (void)reg_addr; (void)val;
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val) {
    (void)dev_addr; (void)reg_addr;
    *val = 0;
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_probe(uint8_t dev_addr) {
    (void)dev_addr;
    return I2C_HAL_SUCCESS;
}

void esp32_i2c_hal_scan(void) {
    // stub
}
#endif
