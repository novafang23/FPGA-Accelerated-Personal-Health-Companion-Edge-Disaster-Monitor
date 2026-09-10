/*
 * pmsa003.c
 * Plantower PMSA003 / PMSA003C Particulate Matter Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 *
 * Protocol Notes:
 *   - Frame format: identical to PMS5003 (32 bytes, 0x42 0x4D start, big-endian)
 *   - UART: 9600 baud, 8N1, no flow control
 *   - The PMSA003 always outputs in passive mode at ~1 Hz after power-up
 *   - Hardware wiring: VCC=5V (fan+laser), TX/RX logic = 3.3V (safe for ESP32-S3)
 *   - Warm-up time: ~30 seconds after power-on for stable readings
 */

#include "pmsa003.h"

#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "esp_log.h"
static const char *TAG = "PMSA003";
#else
#include <stdio.h>
#define ESP_LOGI(tag, ...) do {} while(0)
#define ESP_LOGE(tag, ...) do {} while(0)
#define ESP_LOGW(tag, ...) do {} while(0)
static const char *TAG __attribute__((unused)) = "PMSA003";
#endif

/* -------------------------------------------------------------------------
 * Internal: Extract 16-bit big-endian value from buffer at given offset
 * ---------------------------------------------------------------------- */
static uint16_t extract_u16(const uint8_t *buf, int offset) {
    return (uint16_t)((buf[offset] << 8) | buf[offset + 1]);
}

/* -------------------------------------------------------------------------
 * Internal: Validate checksum and decode a complete 32-byte frame
 *
 * PMSA003 Frame Layout (identical to PMS5003):
 *   [0]    = 0x42 (START1)
 *   [1]    = 0x4D (START2)
 *   [2-3]  = Frame length (always 0x001C = 28)
 *   [4-5]  = PM1.0 CF=1
 *   [6-7]  = PM2.5 CF=1
 *   [8-9]  = PM10  CF=1
 *   [10-11]= PM1.0 atmospheric
 *   [12-13]= PM2.5 atmospheric  <-- primary disaster monitor metric
 *   [14-15]= PM10  atmospheric
 *   [16-17]= Particle count >0.3um per 0.1L
 *   [18-19]= Particle count >0.5um per 0.1L
 *   [20-21]= Particle count >1.0um per 0.1L
 *   [22-23]= Particle count >2.5um per 0.1L
 *   [24-25]= Particle count >5.0um per 0.1L
 *   [26-27]= Particle count >10um per 0.1L
 *   [28-29]= Reserved (PMSA003 = 0x0000)
 *   [30-31]= Checksum (sum of bytes [0..29])
 * ---------------------------------------------------------------------- */
static int pmsa003_parse_frame(pmsa003_t *dev) {
    const uint8_t *buf = dev->rx_buf;

    /* Verify start bytes */
    if (buf[0] != PMSA003_START_BYTE_1 || buf[1] != PMSA003_START_BYTE_2) {
        return -1;
    }

    /* Validate checksum: sum of bytes [0..29] must equal bytes [30..31] */
    uint16_t checksum = 0;
    for (int i = 0; i < 30; i++) {
        checksum += buf[i];
    }
    uint16_t expected = extract_u16(buf, 30);
    if (checksum != expected) {
        ESP_LOGW(TAG, "Checksum mismatch: calc=0x%04X, frame=0x%04X", checksum, expected);
        return -1;
    }

    /* Decode particulate matter data */
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
    dev->last_data.reserved    = extract_u16(buf, 28); /* Always 0 on PMSA003 */

    dev->last_data.valid = 1;
    dev->has_valid_data  = 1;

    ESP_LOGI(TAG, "PM2.5(atm)=%u µg/m³  PM10(atm)=%u µg/m³  PM1.0(atm)=%u µg/m³",
             dev->last_data.pm2_5_atm,
             dev->last_data.pm10_atm,
             dev->last_data.pm1_0_atm);

    return 0;
}

/* =========================================================================
 * Public API Implementation
 * ====================================================================== */

int pmsa003_init(pmsa003_t *dev, int uart_num) {
    if (!dev) return -1;

    dev->uart_num      = uart_num;
    dev->rx_pos        = 0;
    dev->synced        = 0;
    dev->initialized   = 1;
    dev->has_valid_data = 0;
    dev->last_data.valid = 0;

    ESP_LOGI(TAG, "PMSA003 driver initialized on UART%d (9600 baud, GPIO14=RX)", uart_num);
    return 0;
}

int pmsa003_feed_byte(pmsa003_t *dev, uint8_t byte) {
    if (!dev || !dev->initialized) return 0;

    /* State 1: Waiting for first start byte 0x42 */
    if (!dev->synced) {
        if (byte == PMSA003_START_BYTE_1) {
            dev->rx_buf[0] = byte;
            dev->rx_pos    = 1;
            dev->synced    = 1;
        }
        return 0;
    }

    /* State 2: Waiting for second start byte 0x4D */
    if (dev->rx_pos == 1) {
        if (byte == PMSA003_START_BYTE_2) {
            dev->rx_buf[1] = byte;
            dev->rx_pos    = 2;
        } else {
            /* Re-sync: check if this byte is itself 0x42 */
            dev->synced = 0;
            dev->rx_pos = 0;
            if (byte == PMSA003_START_BYTE_1) {
                dev->rx_buf[0] = byte;
                dev->rx_pos    = 1;
                dev->synced    = 1;
            }
        }
        return 0;
    }

    /* State 3: Accumulating frame bytes */
    dev->rx_buf[dev->rx_pos++] = byte;

    if (dev->rx_pos >= PMSA003_FRAME_LEN) {
        /* Frame complete — reset sync for next frame before parsing */
        dev->synced = 0;
        dev->rx_pos = 0;

        if (pmsa003_parse_frame(dev) == 0) {
            return 1; /* Signal: valid frame received */
        }
    }

    return 0;
}

int pmsa003_get_data(const pmsa003_t *dev, pmsa003_data_t *data) {
    if (!dev || !dev->has_valid_data || !data) return -1;
    *data = dev->last_data;
    return 0;
}

int pmsa003_uart_rx_task(pmsa003_t *dev, TickType_t timeout) {
#ifdef ESP_PLATFORM
    if (!dev || !dev->initialized) return -1;

    uint8_t byte;
    int len = uart_read_bytes(dev->uart_num, &byte, 1, timeout);
    if (len > 0) {
        return pmsa003_feed_byte(dev, byte);
    }
    return 0;
#else
    (void)dev; (void)timeout;
    return 0;
#endif
}

const char* pmsa003_aqi_category(uint16_t pm25) {
    /* India CPCB National AQI (NAQI) PM2.5 breakpoints */
    if (pm25 <= 30)  return "Good";
    if (pm25 <= 60)  return "Satisfactory";
    if (pm25 <= 90)  return "Moderate";
    if (pm25 <= 120) return "Poor";
    if (pm25 <= 250) return "Very Poor";
    return "Severe";
}
