/**
 * @file esp32_i2c_hal.c
 * @brief ESP-IDF I2C Driver Implementation for ShrikeFi
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#include "esp32_i2c_hal.h"

#ifdef ESP_PLATFORM
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG __attribute__((unused)) = "I2C_HAL";
static i2c_port_t s_i2c_num = I2C_NUM_0;
static esp32_i2c_handle_t s_i2c_handle = { .port = I2C_NUM_0, .initialized = 0 };
static int s_sda_pin = 1;
static int s_scl_pin = 2;
static uint32_t s_clk_speed_hz = 400000;
static SemaphoreHandle_t s_i2c_mutex = NULL;

static inline void i2c_lock(void) {
    if (s_i2c_mutex) xSemaphoreTakeRecursive(s_i2c_mutex, portMAX_DELAY);
}

static inline void i2c_unlock(void) {
    if (s_i2c_mutex) xSemaphoreGiveRecursive(s_i2c_mutex);
}

/* Recover only a genuinely stuck bus. Never clock a bus whose SCL line is
 * being held low, and never bit-bang when both lines are already high. */
static int recover_stuck_bus_locked(void) {
    int sda_idle = gpio_get_level((gpio_num_t)s_sda_pin);
    int scl_idle = gpio_get_level((gpio_num_t)s_scl_pin);
    if (sda_idle != 0 && scl_idle != 0) return I2C_HAL_SUCCESS;
    if (scl_idle == 0) return I2C_HAL_ERROR;

    gpio_reset_pin((gpio_num_t)s_sda_pin);
    gpio_reset_pin((gpio_num_t)s_scl_pin);
    gpio_config_t pins = {
        .pin_bit_mask = (1ULL << s_sda_pin) | (1ULL << s_scl_pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&pins) != ESP_OK) return I2C_HAL_ERROR;
    gpio_set_level((gpio_num_t)s_sda_pin, 1);
    gpio_set_level((gpio_num_t)s_scl_pin, 1);
    esp_rom_delay_us(20);
    for (int pulse = 0; pulse < 9 && gpio_get_level((gpio_num_t)s_sda_pin) == 0; ++pulse) {
        gpio_set_level((gpio_num_t)s_scl_pin, 0);
        esp_rom_delay_us(10);
        gpio_set_level((gpio_num_t)s_scl_pin, 1);
        esp_rom_delay_us(10);
    }
    /* STOP: SDA low -> high while SCL is high. */
    gpio_set_level((gpio_num_t)s_scl_pin, 1);
    gpio_set_level((gpio_num_t)s_sda_pin, 0);
    esp_rom_delay_us(10);
    gpio_set_level((gpio_num_t)s_sda_pin, 1);
    esp_rom_delay_us(20);

    /* Re-attach pins to the hardware I2C controller after bit-banging */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = s_sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = s_scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = s_clk_speed_hz,
    };
    (void)i2c_param_config(s_i2c_num, &conf);

    return (gpio_get_level((gpio_num_t)s_sda_pin) && gpio_get_level((gpio_num_t)s_scl_pin))
           ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_init(int sda_pin, int scl_pin, uint32_t clk_speed_hz) {
    if (s_i2c_mutex == NULL) {
        s_i2c_mutex = xSemaphoreCreateRecursiveMutex();
    }
    i2c_lock();
    s_sda_pin = sda_pin;
    s_scl_pin = scl_pin;
    s_clk_speed_hz = clk_speed_hz;
#ifdef ESP_PLATFORM
    /* Log the pins and speed up front: when a bus is dead, the first question is
     * always "which pins did it actually try?". */
    ESP_LOGI(TAG, "I2C init: port=%d SDA=GPIO%d SCL=GPIO%d speed=%lu Hz",
             (int)s_i2c_num, sda_pin, scl_pin, (unsigned long)clk_speed_hz);
#endif
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed_hz,
    };
    esp_err_t err = i2c_param_config(s_i2c_num, &conf);
    if (err != ESP_OK) {
#ifdef ESP_PLATFORM
        /* This used to return silently. A failed init here makes every sensor
         * read fail with an all-ones/garbage value, which looks exactly like a
         * wiring fault -- so say it loudly and name the pins. */
        ESP_LOGE(TAG, "i2c_param_config FAILED (%s) for SDA=GPIO%d SCL=GPIO%d -- "
                      "all sensor reads will now fail",
                 esp_err_to_name(err), sda_pin, scl_pin);
#endif
        i2c_unlock();
        return I2C_HAL_ERROR;
    }
    err = i2c_driver_install(s_i2c_num, conf.mode, 0, 0, 0);
    if (err == ESP_OK) {
        s_i2c_handle.initialized = 1;
#ifdef ESP_PLATFORM
        /* Electrical diagnostic: driver init can succeed while the external
         * bus is open or held low. Report idle line levels before interpreting
         * a missing ACK as a sensor/software fault. */
        ESP_LOGI(TAG, "I2C idle levels after init: SDA=%d SCL=%d",
                 gpio_get_level((gpio_num_t)s_sda_pin),
                 gpio_get_level((gpio_num_t)s_scl_pin));
#endif
        i2c_unlock();
        return I2C_HAL_SUCCESS;
    }
#ifdef ESP_PLATFORM
    i2c_unlock();
    ESP_LOGE(TAG, "i2c_driver_install FAILED (%s) on port %d -- all sensor reads "
                  "will now fail", esp_err_to_name(err), (int)s_i2c_num);
#endif
    return I2C_HAL_ERROR;
}

esp32_i2c_handle_t* esp32_i2c_hal_get_handle(void) {
    return s_i2c_handle.initialized ? &s_i2c_handle : NULL;
}

int esp32_i2c_hal_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    i2c_lock();
    esp_err_t err = i2c_master_write_read_device(s_i2c_num, dev_addr, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
    i2c_unlock();
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len) {
    uint8_t buf[256];
    if (len + 1 > sizeof(buf)) return I2C_HAL_ERROR;
    buf[0] = reg_addr;
    for (int i = 0; i < len; i++) buf[i + 1] = data[i];
    i2c_lock();
    esp_err_t err = i2c_master_write_to_device(s_i2c_num, dev_addr, buf, len + 1, 1000 / portTICK_PERIOD_MS);
    i2c_unlock();
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_write_raw(uint8_t dev_addr, const uint8_t *data, uint16_t len) {
    i2c_lock();
    esp_err_t err = i2c_master_write_to_device(s_i2c_num, dev_addr, data, len, 1000 / portTICK_PERIOD_MS);
    i2c_unlock();
    return (err == ESP_OK) ? I2C_HAL_SUCCESS : I2C_HAL_ERROR;
}

int esp32_i2c_hal_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val) {
    return esp32_i2c_hal_write(dev_addr, reg_addr, &val, 1);
}

int esp32_i2c_hal_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val) {
    return esp32_i2c_hal_read(dev_addr, reg_addr, val, 1);
}

int esp32_i2c_hal_probe(uint8_t dev_addr) {
    i2c_lock();
    /* ESP-IDF v5.x rejects NULL data in i2c_master_write_to_device().
     * Use the cmd-link API to send just an address byte for probing. */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(s_i2c_num, cmd, 50 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    i2c_unlock();
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

void esp32_i2c_hal_lock(void) { i2c_lock(); }
void esp32_i2c_hal_unlock(void) { i2c_unlock(); }

int esp32_i2c_hal_recover_bus(void) {
    i2c_lock();
    int rc = recover_stuck_bus_locked();
    i2c_unlock();
    return rc;
}

#else
/* Stub simulation implementation for PC tests */
#include <string.h>

static int8_t   s_sim_temp_int = 0;
static uint8_t  s_sim_temp_frac = 0;
static uint8_t  s_sim_fifo_wr = 0;
static uint8_t  s_sim_fifo_ovf = 0;
static uint8_t  s_sim_fifo_rd = 0;
static uint8_t  s_sim_mode = 0x03; /* default MAX30102_MODE_SPO2 */
static uint32_t s_sim_mode_writes = 0;
static uint32_t s_sim_raw_writes = 0;
static uint32_t s_sim_raw_bytes = 0;

void esp32_i2c_hal_sim_set_max30102_temp(int8_t tint, uint8_t tfrac) {
    s_sim_temp_int = tint;
    s_sim_temp_frac = tfrac;
}

void esp32_i2c_hal_sim_set_max30102_fifo_ptrs(uint8_t wr, uint8_t ovf, uint8_t rd) {
    s_sim_fifo_wr = wr;
    s_sim_fifo_ovf = ovf;
    s_sim_fifo_rd = rd;
}

void esp32_i2c_hal_sim_set_max30102_mode(uint8_t mode) {
    s_sim_mode = mode;
}

uint8_t esp32_i2c_hal_sim_get_max30102_mode(void) {
    return s_sim_mode;
}

uint32_t esp32_i2c_hal_sim_get_max30102_mode_writes(void) {
    return s_sim_mode_writes;
}

uint32_t esp32_i2c_hal_sim_get_raw_writes(void) {
    return s_sim_raw_writes;
}

uint32_t esp32_i2c_hal_sim_get_raw_bytes(void) {
    return s_sim_raw_bytes;
}

void esp32_i2c_hal_sim_reset_counters(void) {
    s_sim_raw_writes = 0;
    s_sim_raw_bytes = 0;
    s_sim_mode_writes = 0;
}

int esp32_i2c_hal_init(int sda_pin, int scl_pin, uint32_t clk_speed_hz) {
    (void)sda_pin; (void)scl_pin; (void)clk_speed_hz;
    return I2C_HAL_SUCCESS;
}

static int s_dummy_handle = 1;
esp32_i2c_handle_t* esp32_i2c_hal_get_handle(void) {
    return (esp32_i2c_handle_t*)&s_dummy_handle;
}

int esp32_i2c_hal_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    if (!data) return I2C_HAL_ERROR;
    memset(data, 0, len);
    if (dev_addr == 0x57) { /* MAX30102 */
        if (reg_addr == 0x04 && len >= 3) { /* Burst FIFO pointers WR, OVF, RD */
            data[0] = s_sim_fifo_wr;
            data[1] = s_sim_fifo_ovf;
            data[2] = s_sim_fifo_rd;
            return I2C_HAL_SUCCESS;
        }
        for (uint16_t i = 0; i < len; i++) {
            uint8_t reg = (uint8_t)(reg_addr + i);
            uint8_t val = 0;
            esp32_i2c_hal_read_byte(dev_addr, reg, &val);
            data[i] = val;
        }
    }
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_write(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len) {
    if (dev_addr == 0x57) {
        if (reg_addr == 0x09 && len > 0) {
            s_sim_mode_writes++;
            if (data[0] & 0x40) {
                s_sim_mode = 0x00; /* Self-clearing reset bit */
            } else {
                s_sim_mode = data[0];
            }
        } else if (reg_addr == 0x04 && len > 0) {
            s_sim_fifo_wr = data[0];
        } else if (reg_addr == 0x05 && len > 0) {
            s_sim_fifo_ovf = data[0];
        } else if (reg_addr == 0x06 && len > 0) {
            s_sim_fifo_rd = data[0];
        }
    }
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_write_raw(uint8_t dev_addr, const uint8_t *data, uint16_t len) {
    (void)dev_addr; (void)data;
    s_sim_raw_writes++;
    s_sim_raw_bytes += len;
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val) {
    return esp32_i2c_hal_write(dev_addr, reg_addr, &val, 1);
}

int esp32_i2c_hal_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val) {
    if (!val) return I2C_HAL_ERROR;
    *val = 0;
    if (dev_addr == 0x57) {
        switch (reg_addr) {
            case 0xFF: /* PART_ID */
                *val = 0x15;
                break;
            case 0x09: /* MODE_CONFIG */
                *val = s_sim_mode;
                break;
            case 0x21: /* TEMP_CONFIG */
                *val = 0x00; /* Ready */
                break;
            case 0x1F: /* TEMP_INT */
                *val = (uint8_t)s_sim_temp_int;
                break;
            case 0x20: /* TEMP_FRAC */
                *val = s_sim_temp_frac;
                break;
            case 0x04: /* FIFO_WR_PTR */
                *val = s_sim_fifo_wr;
                break;
            case 0x05: /* OVF_COUNTER */
                *val = s_sim_fifo_ovf;
                break;
            case 0x06: /* FIFO_RD_PTR */
                *val = s_sim_fifo_rd;
                break;
            case 0x0C: /* LED1_PA */
            case 0x0D: /* LED2_PA */
                *val = 0x36;
                break;
            default:
                *val = 0;
                break;
        }
    }
    return I2C_HAL_SUCCESS;
}

int esp32_i2c_hal_probe(uint8_t dev_addr) {
    (void)dev_addr;
    return I2C_HAL_SUCCESS;
}

void esp32_i2c_hal_scan(void) {
    // stub
}

void esp32_i2c_hal_lock(void) {}
void esp32_i2c_hal_unlock(void) {}
int esp32_i2c_hal_recover_bus(void) { return I2C_HAL_SUCCESS; }
#endif
