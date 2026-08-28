/*
 * pms5003.h
 * PMS5003 Particulate Matter (PM2.5) Sensor Driver
 */

#ifndef PMS5003_H
#define PMS5003_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PMS5003 UART Configuration */
#define PMS5003_BAUD_RATE     9600
#define PMS5003_FRAME_LEN     32   /* Total frame length in bytes */
#define PMS5003_START_BYTE_1  0x42
#define PMS5003_START_BYTE_2  0x4D

/* Particulate Matter Data */
typedef struct {
    /* Standard particle concentrations (CF=1, factory environment) */
    uint16_t pm1_0_cf1;    /* PM1.0 µg/m³ (CF=1)  */
    uint16_t pm2_5_cf1;    /* PM2.5 µg/m³ (CF=1)  */
    uint16_t pm10_cf1;     /* PM10  µg/m³ (CF=1)  */

    /* Atmospheric environment concentrations (use these for monitoring) */
    uint16_t pm1_0_atm;    /* PM1.0 µg/m³ (atmospheric) */
    uint16_t pm2_5_atm;    /* PM2.5 µg/m³ (atmospheric) — primary metric */
    uint16_t pm10_atm;     /* PM10  µg/m³ (atmospheric) */

    /* Particle counts per 0.1L of air */
    uint16_t count_0_3um;  /* Particles > 0.3µm  */
    uint16_t count_0_5um;  /* Particles > 0.5µm  */
    uint16_t count_1_0um;  /* Particles > 1.0µm  */
    uint16_t count_2_5um;  /* Particles > 2.5µm  */
    uint16_t count_5_0um;  /* Particles > 5.0µm  */
    uint16_t count_10um;   /* Particles > 10µm   */

    int valid;             /* 1 if checksum passed */
} pms5003_data_t;

/* Driver State */
typedef struct {
    uint8_t         rx_buf[PMS5003_FRAME_LEN]; /* Frame receive buffer     */
    int             rx_pos;                     /* Current position in buffer */
    int             synced;                     /* 1 if start bytes detected */
    uint32_t        uart_base;                  /* UART controller base addr */
    int             initialized;
    pms5003_data_t  last_data;                  /* Last valid parsed frame    */
    int             has_valid_data;             /* 1 if last_data is valid    */
} pms5003_t;

/* Zynq PS UART base addresses */
#define ZYNQ_UART0_BASEADDR  0xE0000000U
#define ZYNQ_UART1_BASEADDR  0xE0001000U

/*
 * Initialize the PMS5003 driver.
 *   dev       — pointer to driver state
 *   uart_base — UART controller base address
 * Returns 0 on success.
 */
int pms5003_init(pms5003_t *dev, uint32_t uart_base);

/*
 * Feed a single received UART byte into the parser.
 * Call this from your UART RX interrupt handler or polling loop.
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
 * Try to read a complete frame by polling the UART.
 * Blocks until a valid frame is received or timeout.
 *   dev     — initialized PMS5003 driver
 *   data    — output structure
 *   timeout — approximate loop iterations before giving up
 * Returns 0 on success, -1 on timeout.
 */
int pms5003_read_blocking(pms5003_t *dev, pms5003_data_t *data, int timeout);

/*
 * Get the AQI category string for a given PM2.5 value.
 * Based on India's National Air Quality Index (NAQI) breakpoints.
 */
const char* pms5003_aqi_category(uint16_t pm25);

#ifdef __cplusplus
}
#endif

#endif /* PMS5003_H */
