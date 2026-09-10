/*
 * pmsa003.h
 * Plantower PMSA003 / PMSA003C Particulate Matter Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 *
 * Drop-in replacement for pms5003.h
 * UART: 9600 baud, 8N1, passive mode output
 * Frame: 32 bytes, same start bytes (0x42 0x4D) as PMS5003
 * Key difference from PMS5003: Reserved bytes at offset 28-29 are always 0x00
 */

#ifndef PMSA003_H
#define PMSA003_H

#include <stdint.h>
#include <stddef.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
typedef uint32_t TickType_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * UART Configuration — identical to PMS5003, no changes needed on the
 * ESP32-S3 UART1 peripheral side (GPIO14=RX, GPIO18=TX, 9600 baud)
 * ---------------------------------------------------------------------- */
#define PMSA003_BAUD_RATE     9600
#define PMSA003_FRAME_LEN     32
#define PMSA003_START_BYTE_1  0x42
#define PMSA003_START_BYTE_2  0x4D

/* -------------------------------------------------------------------------
 * Particulate Matter Data
 * All concentrations in µg/m³ (standard conditions)
 * ---------------------------------------------------------------------- */
typedef struct {
    /* CF=1 (factory calibration) — use for comparison against factory specs */
    uint16_t pm1_0_cf1;
    uint16_t pm2_5_cf1;
    uint16_t pm10_cf1;

    /* Atmospheric environment — use these for actual air quality assessment */
    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;   /* Primary metric for PRSI (Pollution Respiratory Strain Index) */
    uint16_t pm10_atm;

    /* Particle count per 0.1L air by size */
    uint16_t count_0_3um;
    uint16_t count_0_5um;
    uint16_t count_1_0um;
    uint16_t count_2_5um;
    uint16_t count_5_0um;
    uint16_t count_10um;

    /* PMSA003C only: Reserved field (always 0 in PMSA003/PMSA003C) */
    uint16_t reserved;

    int valid;
} pmsa003_data_t;

/* -------------------------------------------------------------------------
 * Driver State (byte-level stream parser — same architecture as pms5003_t)
 * ---------------------------------------------------------------------- */
typedef struct {
    uint8_t rx_buf[PMSA003_FRAME_LEN];
    int     rx_pos;
    int     synced;
    int     uart_num;
    int     initialized;
    pmsa003_data_t last_data;
    int     has_valid_data;
} pmsa003_t;

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialize the PMSA003 driver state.
 * @param dev       Pointer to driver state struct
 * @param uart_num  UART port number (UART_NUM_1, etc.)
 * @return 0 on success, -1 on invalid parameter
 */
int pmsa003_init(pmsa003_t *dev, int uart_num);

/**
 * @brief Feed a single received UART byte into the frame parser.
 * Call this from your UART RX FreeRTOS task on every received byte.
 * @param dev  Initialized PMSA003 driver
 * @param byte Received byte
 * @return 1 if a complete valid frame was decoded, 0 otherwise
 */
int pmsa003_feed_byte(pmsa003_t *dev, uint8_t byte);

/**
 * @brief Retrieve the latest successfully parsed sensor data.
 * @param dev  Initialized PMSA003 driver
 * @param data Output data structure
 * @return 0 on success, -1 if no valid data has been received yet
 */
int pmsa003_get_data(const pmsa003_t *dev, pmsa003_data_t *data);

/**
 * @brief Single UART RX service call — reads one byte and feeds it.
 * Designed to be called in a tight FreeRTOS task loop.
 * @param dev     Initialized PMSA003 driver
 * @param timeout Blocking timeout for uart_read_bytes (FreeRTOS ticks)
 * @return 1 if a complete frame was received, 0 otherwise
 */
int pmsa003_uart_rx_task(pmsa003_t *dev, TickType_t timeout);

/**
 * @brief Get India NAQI AQI category string from a PM2.5 value.
 * Thresholds per CPCB (Central Pollution Control Board) NAQI standard.
 * @param pm25 PM2.5 concentration in µg/m³ (use pm2_5_atm field)
 * @return Static string: "Good", "Satisfactory", "Moderate", "Poor",
 *         "Very Poor", or "Severe"
 */
const char* pmsa003_aqi_category(uint16_t pm25);

#ifdef __cplusplus
}
#endif

#endif /* PMSA003_H */
