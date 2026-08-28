/*
 * pms5003.c
 * PMS5003 Particulate Matter Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include "pms5003.h"

#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "PMS5003";

#else
#include <stdio.h>
#define ESP_LOGI(tag, ...) do {} while(0)
#define ESP_LOGE(tag, ...) do {} while(0)
#define ESP_LOGW(tag, ...) do {} while(0)
static const char *TAG __attribute__((unused)) = "PMS5003";
#endif

/* Internal: Extract 16-bit big-endian value from buffer */
static uint16_t extract_u16(const uint8_t *buf, int offset) {
    return (uint16_t)((buf[offset] << 8) | buf[offset + 1]);
}

/* Internal: Parse a complete frame */
static int pms5003_parse_frame(pms5003_t *dev) {
    const uint8_t *buf = dev->rx_buf;

    if (buf[0] != PMS5003_START_BYTE_1 || buf[1] != PMS5003_START_BYTE_2) {
        return -1;
    }

    uint16_t checksum = 0;
    for (int i = 0; i < 30; i++) {
        checksum += buf[i];
    }
    uint16_t expected = extract_u16(buf, 30);
    if (checksum != expected) {
        ESP_LOGW(TAG, "Checksum mismatch: 0x%04X vs 0x%04X", checksum, expected);
        return -1;
    }

    dev->last_data.pm1_0_cf1   = extract_u16(buf, 4);
    dev->last_data.pm2_5_cf1   = extract_u16(buf, 6);
    dev->last_data.pm10_cf1    = extract_u16(buf, 8);

    dev->last_data.pm1_0_atm   = extract_u16(buf, 10);
    dev->last_data.pm2_5_atm   = extract_u16(buf, 12);
    dev->last_data.pm10_atm    = extract_u16(buf, 14);

    dev->last_data.count_0_3um = extract_u16(buf, 16);
    dev->last_data.count_0_5um = extract_u16(buf, 18);
    dev->last_data.count_1_0um = extract_u16(buf, 20);
    dev->last_data.count_2_5um = extract_u16(buf, 22);
    dev->last_data.count_5_0um = extract_u16(buf, 24);
    dev->last_data.count_10um  = extract_u16(buf, 26);

    dev->last_data.valid = 1;
    dev->has_valid_data = 1;

    return 0;
}

/* Public API */
int pms5003_init(pms5003_t *dev, int uart_num) {
    if (!dev) return -1;

    dev->uart_num = uart_num;
    dev->rx_pos = 0;
    dev->synced = 0;
    dev->initialized = 1;
    dev->last_data.valid = 0;
    dev->has_valid_data = 0;

    return 0;
}

int pms5003_feed_byte(pms5003_t *dev, uint8_t byte) {
    if (!dev || !dev->initialized) return 0;

    if (!dev->synced) {
        if (byte == PMS5003_START_BYTE_1) {
            dev->rx_buf[0] = byte;
            dev->rx_pos = 1;
            dev->synced = 1;
        }
        return 0;
    }

    if (dev->rx_pos == 1) {
        if (byte == PMS5003_START_BYTE_2) {
            dev->rx_buf[1] = byte;
            dev->rx_pos = 2;
        } else {
            dev->synced = 0;
            dev->rx_pos = 0;
        }
        return 0;
    }

    dev->rx_buf[dev->rx_pos++] = byte;

    if (dev->rx_pos >= PMS5003_FRAME_LEN) {
        dev->synced = 0;
        dev->rx_pos = 0;

        if (pms5003_parse_frame(dev) == 0) {
            return 1;
        }
    }

    return 0;
}

int pms5003_get_data(const pms5003_t *dev, pms5003_data_t *data) {
    if (!dev->has_valid_data || !data) return -1;
    *data = dev->last_data;
    return 0;
}

int pms5003_uart_rx_task(pms5003_t *dev, TickType_t timeout) {
#ifdef ESP_PLATFORM
    if (!dev || !dev->initialized) return -1;

    uint8_t byte;
    int len = uart_read_bytes(dev->uart_num, &byte, 1, timeout);
    if (len > 0) {
        if (pms5003_feed_byte(dev, byte) == 1) {
            return 1;
        }
    }
    return 0;
#else
    (void)dev; (void)timeout;
    return 0;
#endif
}

const char* pms5003_aqi_category(uint16_t pm25) {
    if (pm25 <= 30)  return "Good";
    if (pm25 <= 60)  return "Satisfactory";
    if (pm25 <= 90)  return "Moderate";
    if (pm25 <= 120) return "Poor";
    if (pm25 <= 250) return "Very Poor";
    return "Severe";
}