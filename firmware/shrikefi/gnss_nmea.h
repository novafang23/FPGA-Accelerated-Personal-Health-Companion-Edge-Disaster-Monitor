/*
 * gnss_nmea.h
 * Generic GNSS (GPS / GLONASS / Galileo / BeiDou) Driver & NMEA-0183 Parser
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 *
 * Supported Hardware Modules:
 *   - u-blox NEO-M8N (GY-GPSV3-NEO / GY-GPSU3-NEO breakout) [Primary Hardware]
 *   - Quectel L89-HS Multi-Constellation GNSS [Fully Compatible]
 *
 * Hardware Interface:
 *   - GNSS TXD -> ESP32-S3 GPIO 16 (UART2 RX, NMEA data stream)
 *   - GNSS RXD <- ESP32-S3 GPIO 17 (UART2 TX, module configuration & UBX/NMEA commands)
 *   - VCC: 3.3V ONLY (Strict LVCMOS limit; DO NOT connect to 5V VBUS!)
 *   - GND: System Common Ground
 *   - Default Serial: 9600 bps, 8 data bits, no parity, 1 stop bit (8-N-1)
 *
 * Supported NMEA Sentences:
 *   - $GNRMC / $GPRMC: Recommended Minimum Specific GNSS Data (Lat, Lon, Speed, Course, Date, Time)
 *   - $GNGGA / $GPGGA: Fix Data (Quality, Satellites in view, HDOP, Altitude)
 *   - $GNVTG / $GPVTG: Course Over Ground & Ground Speed (km/h and knots)
 *
 * Thread-safety & Portability:
 *   - ESP-IDF: Thread-safe state protected by portMUX critical section spinlock.
 *   - Host Builds: 100% dependency-free pure C parser for CI & host test harnesses.
 */

#ifndef GNSS_NMEA_H
#define GNSS_NMEA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
typedef uint32_t TickType_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of a single NMEA sentence (standard is 82, extra headroom for vendor extensions) */
#define GNSS_NMEA_MAX_SENTENCE_LEN 128

/* Formatted coordinate string capacity (e.g. "20.29606N, 85.82454E" or "NO FIX") */
#define GNSS_COORD_STR_LEN         32

/* GNSS Telemetry & Navigation State */
typedef struct {
    bool     fix_valid;          /* true if 2D/3D navigation fix is active and valid */
    uint8_t  fix_quality;        /* 0=Invalid, 1=GPS SPS, 2=DGPS/SBAS, 6=Dead Reckoning */
    float    latitude;           /* Signed decimal degrees: positive=North, negative=South */
    float    longitude;          /* Signed decimal degrees: positive=East, negative=West */
    char     lat_dir;            /* 'N' or 'S' */
    char     lon_dir;            /* 'E' or 'W' */
    float    altitude_m;         /* Altitude in meters above mean sea level */
    float    speed_kmh;          /* Ground speed in km/h */
    float    course_deg;         /* Heading / course over ground in degrees */
    float    hdop;               /* Horizontal Dilution of Precision */
    uint8_t  satellites;         /* Satellites in view/use */
    char     utc_time[11];       /* "HHMMSS.sss" NUL-terminated */
    char     utc_date[7];        /* "DDMMYY" NUL-terminated */
    uint32_t last_fix_time_ms;   /* System timestamp of last valid fix */
    uint32_t sentences_received; /* Total NMEA lines received */
    uint32_t sentences_parsed;   /* Recognized and successfully parsed sentences */
    uint32_t checksum_errors;    /* Lines dropped due to checksum mismatch */
} gnss_data_t;

/* Driver State */
typedef struct {
    int         uart_num;
    int         initialized;
    char        line_buf[GNSS_NMEA_MAX_SENTENCE_LEN];
    size_t      line_idx;
    bool        in_sentence;
    gnss_data_t data;
} gnss_nmea_t;

/*
 * Calculate standard NMEA XOR checksum for a sentence body.
 * Expects characters between '$' and '*' (or terminating NUL).
 */
uint8_t gnss_nmea_checksum(const char *sentence);

/*
 * Initialize the GNSS NMEA driver state.
 *   dev      - Pointer to driver state struct
 *   uart_num - UART port number (e.g. UART_NUM_2)
 * Returns 0 on success, negative on error.
 */
int gnss_nmea_init(gnss_nmea_t *dev, int uart_num);

/*
 * Feed a single received UART byte into the line assembler.
 * When a complete line terminates ('\n' or '\r'), it validates the checksum
 * and parses supported sentences ($GNRMC, $GPRMC, $GNGGA, $GPGGA, $GNVTG, $GPVTG).
 *   dev  - Driver state struct
 *   byte - Received byte from UART
 * Returns 1 if a complete valid sentence was parsed, 0 otherwise.
 */
int gnss_nmea_feed_byte(gnss_nmea_t *dev, uint8_t byte);

/*
 * Directly parse a null-terminated NMEA sentence string.
 * Used for host testing or pre-buffered UART reads.
 *   dev  - Driver state struct
 *   line - Full NMEA sentence, e.g. "$GNRMC,123519,A,2017.7635,N,08549.4723,E,0.0,0.0,230394,,,A*6D"
 * Returns 1 if parsed successfully, 0 if ignored, -1 on checksum error or invalid format.
 */
int gnss_nmea_parse_line(gnss_nmea_t *dev, const char *line);

/*
 * Safely copy the latest parsed GNSS data.
 *   dev  - Driver state struct
 *   out  - Destination struct
 * Returns 0 on success, -1 if no valid fix has been acquired yet.
 */
int gnss_nmea_get_data(const gnss_nmea_t *dev, gnss_data_t *out);

/*
 * Check if the driver currently holds a valid satellite navigation fix.
 */
bool gnss_nmea_has_fix(const gnss_nmea_t *dev);

/* Expire a fix whose last valid RMC/GGA update is older than max_age_ms.
 * The age comparison is wrap-safe for intervals shorter than 2^31 ms.
 * Returns true only when this call invalidated a previously valid fix. */
bool gnss_nmea_expire_fix(gnss_nmea_t *dev, uint32_t now_ms,
                          uint32_t max_age_ms);

/*
 * Format decimal coordinates into human-readable compact string for OLED / Web UI display.
 * E.g., "20.29606N, 85.82454E" or "NO FIX".
 *   data - Parsed GNSS data
 *   out  - Target buffer
 *   cap  - Capacity of target buffer
 */
void gnss_nmea_format_coords(const gnss_data_t *data, char *out, size_t cap);

/*
 * Read available bytes from UART and feed to parser.
 * Designed to be called from a FreeRTOS background task.
 *   dev     - Initialized driver struct
 *   timeout - Blocking timeout in FreeRTOS ticks
 * Returns number of valid sentences parsed during this poll.
 */
int gnss_nmea_uart_poll(gnss_nmea_t *dev, TickType_t timeout);

/* =========================================================================
 * Command Transmission & Configuration Helpers (via GPIO 17)
 * ========================================================================= */

/*
 * Send a raw string or byte buffer directly to the GNSS module over UART TX.
 * Returns bytes transmitted, or negative on error.
 */
int gnss_nmea_send_raw(gnss_nmea_t *dev, const char *raw_str);

/*
 * Formats an NMEA command body with '$', computed checksum '*hh', and "\r\n", then transmits.
 * Example: gnss_nmea_send_command(dev, "PMTK220,1000") -> sends "$PMTK220,1000*1F\r\n"
 */
int gnss_nmea_send_command(gnss_nmea_t *dev, const char *cmd_body);

/*
 * Set GNSS NMEA fix update rate (PMTK protocol - Quectel compatible).
 *   interval_ms: 1000 = 1 Hz, 200 = 5 Hz, 100 = 10 Hz
 */
int gnss_nmea_cmd_set_rate(gnss_nmea_t *dev, uint16_t interval_ms);

/*
 * Set GNSS module baud rate (PMTK protocol - Quectel compatible).
 */
int gnss_nmea_cmd_set_baud(gnss_nmea_t *dev, uint32_t baud);

/*
 * Put module into low-power standby mode (PMTK protocol).
 */
int gnss_nmea_cmd_standby(gnss_nmea_t *dev);

/*
 * Trigger a hot restart (PMTK protocol).
 */
int gnss_nmea_cmd_hot_restart(gnss_nmea_t *dev);

/*
 * Send a binary UBX protocol frame to a u-blox module (e.g. NEO-M8N).
 * Automatically calculates the 2-byte Fletcher checksum (CK_A, CK_B)
 * and formats the 0xB5 0x62 synchronization header.
 *   dev     - Driver state struct
 *   cls     - UBX Class ID (e.g., 0x06 for CFG)
 *   id      - UBX Message ID (e.g., 0x08 for CFG-RATE)
 *   payload - Message payload bytes (can be NULL if len == 0)
 *   len     - Length of payload in bytes
 * Returns bytes transmitted, or negative on error.
 */
int gnss_nmea_send_ubx(gnss_nmea_t *dev, uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len);

/*
 * Configure update rate on u-blox NEO-M8N using native UBX-CFG-RATE.
 *   interval_ms: 1000 = 1 Hz, 200 = 5 Hz, 100 = 10 Hz
 */
int gnss_nmea_ublox_set_rate(gnss_nmea_t *dev, uint16_t interval_ms);

/*
 * Configure baud rate on u-blox NEO-M8N using standard $PUBX,41 command.
 */
int gnss_nmea_ublox_set_baud(gnss_nmea_t *dev, uint32_t baud);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_NMEA_H */
