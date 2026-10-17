/*
 * gps_l89hs.h
 * Backward Compatibility Wrapper for Quectel L89HS GNSS Driver
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 *
 * NOTE: The primary GNSS NMEA driver is now gnss_nmea.h / gnss_nmea.c,
 * which supports u-blox NEO-M8N (GY-GPSV3-NEO / GY-GPSU3-NEO) as well as Quectel L89-HS.
 * This header provides 100% backward compatibility for existing code.
 */

#ifndef GPS_L89HS_H
#define GPS_L89HS_H

#include "gnss_nmea.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Backward compatibility type aliases */
#define GPS_NMEA_MAX_SENTENCE_LEN GNSS_NMEA_MAX_SENTENCE_LEN
#define GPS_COORD_STR_LEN         GNSS_COORD_STR_LEN

typedef gnss_data_t gps_data_t;
typedef gnss_nmea_t gps_l89hs_t;

/* Backward compatibility function prototypes */
uint8_t gps_nmea_checksum(const char *sentence);
int gps_l89hs_init(gps_l89hs_t *dev, int uart_num);
int gps_l89hs_feed_byte(gps_l89hs_t *dev, uint8_t byte);
int gps_l89hs_parse_line(gps_l89hs_t *dev, const char *line);
int gps_l89hs_get_data(const gps_l89hs_t *dev, gps_data_t *out);
bool gps_l89hs_has_fix(const gps_l89hs_t *dev);
void gps_l89hs_format_coords(const gps_data_t *data, char *out, size_t cap);
int gps_l89hs_uart_poll(gps_l89hs_t *dev, TickType_t timeout);
int gps_l89hs_send_raw(gps_l89hs_t *dev, const char *raw_str);
int gps_l89hs_send_command(gps_l89hs_t *dev, const char *cmd_body);
int gps_l89hs_cmd_set_rate(gps_l89hs_t *dev, uint16_t interval_ms);
int gps_l89hs_cmd_set_baud(gps_l89hs_t *dev, uint32_t baud);
int gps_l89hs_cmd_standby(gps_l89hs_t *dev);
int gps_l89hs_cmd_hot_restart(gps_l89hs_t *dev);
int gps_l89hs_ublox_set_rate(gps_l89hs_t *dev, uint16_t interval_ms);
int gps_l89hs_ublox_set_baud(gps_l89hs_t *dev, uint32_t baud);

#ifdef __cplusplus
}
#endif

#endif /* GPS_L89HS_H */
