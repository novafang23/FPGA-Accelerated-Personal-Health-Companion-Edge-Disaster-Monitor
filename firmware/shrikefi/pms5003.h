/*
 * pms5003.h
 * PMS5003 Particulate Matter Sensor Driver (ESP-IDF Port)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#ifndef PMS5003_H
#define PMS5003_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PMS5003 UART Configuration */
#define PMS5003_BAUD_RATE     9600
#define PMS5003_FRAME_LEN     32
#define PMS5003_START_BYTE_1  0x42
#define PMS5003_START_BYTE_2  0x4D

/* Particulate Matter Data */
typedef struct {
    uint16_t pm1_0_cf1;
    uint16_t pm2_5_cf1;
    uint16_t pm10_cf1;

    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;
    uint16_t pm10_atm;

    uint16_t count_0_3um;
    uint16_t count_0_5um;
    uint16_t count_1_0um;
    uint16_t count_2_5um;
    uint16_t count_5_0um;
    uint16_t count_10um;

    int valid;
} pms5003_data_t;

/* Driver State */
typedef struct {
    uint8_t rx_buf[PMS5003_FRAME_LEN];
    int rx_pos;
    int synced;
    int uart_num;
    int initialized;
    pms5003_data_t last_data;
    int has_valid_data;
} pms5003_t;

/*
 * Initialize the PMS5003 driver.
 *   dev       — pointer to driver state
 *   uart_num  — UART port number (e.g., UART_NUM_1 or UART_NUM_2)
 * Returns 0 on success.
 */
int pms5003_init(pms5003_t *dev, int uart_num);

/*
 * Feed a single received UART byte into the parser.
 * Call this from your UART RX task or interrupt handler.
 *   dev  — initialized PMS5003 driver
 *   byte — the received byte
 * Returns 1 if a complete valid frame was decoded, 0 otherwise.
 */
int pms5003_feed_byte(pms5003_t *dev, uint8_t byte);

/*
 * Get the latest parsed data (valid after pms5003_feed_byte returns 1).
 *   dev  — initialized PMS5003 driver
 *   data — output structure
 * Returns 0 on success, -1 if no valid data available.
 */
int pms5003_get_data(const pms5003_t *dev, pms5003_data_t *data);

/*
 * UART RX Task: reads from UART and feeds bytes to parser.
 * This should be run as a FreeRTOS task.
 *   dev     — initialized PMS5003 driver
 *   timeout — blocking timeout for uart_read_bytes (ticks)
 * Returns 0 on success, -1 on timeout/error.
 */
int pms5003_uart_rx_task(pms5003_t *dev, TickType_t timeout);

/*
 * Get the AQI category string for a given PM2.5 value.
 * Based on India's National Air Quality Index (NAQI) breakpoints.
 */
const char* pms5003_aqi_category(uint16_t pm25);

#ifdef __cplusplus
}
#endif

#endif /* PMS5003_H */