/*
 * gps_l89hs.c
 * Backward Compatibility Wrapper Implementation for Quectel L89HS
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 */

#include "gps_l89hs.h"

uint8_t gps_nmea_checksum(const char *sentence) {
    return gnss_nmea_checksum(sentence);
}

int gps_l89hs_init(gps_l89hs_t *dev, int uart_num) {
    return gnss_nmea_init(dev, uart_num);
}

int gps_l89hs_feed_byte(gps_l89hs_t *dev, uint8_t byte) {
    return gnss_nmea_feed_byte(dev, byte);
}

int gps_l89hs_parse_line(gps_l89hs_t *dev, const char *line) {
    return gnss_nmea_parse_line(dev, line);
}

int gps_l89hs_get_data(const gps_l89hs_t *dev, gps_data_t *out) {
    return gnss_nmea_get_data(dev, out);
}

bool gps_l89hs_has_fix(const gps_l89hs_t *dev) {
    return gnss_nmea_has_fix(dev);
}

void gps_l89hs_format_coords(const gps_data_t *data, char *out, size_t cap) {
    gnss_nmea_format_coords(data, out, cap);
}

int gps_l89hs_uart_poll(gps_l89hs_t *dev, TickType_t timeout) {
    return gnss_nmea_uart_poll(dev, timeout);
}

int gps_l89hs_send_raw(gps_l89hs_t *dev, const char *raw_str) {
    return gnss_nmea_send_raw(dev, raw_str);
}

int gps_l89hs_send_command(gps_l89hs_t *dev, const char *cmd_body) {
    return gnss_nmea_send_command(dev, cmd_body);
}

int gps_l89hs_cmd_set_rate(gps_l89hs_t *dev, uint16_t interval_ms) {
    return gnss_nmea_cmd_set_rate(dev, interval_ms);
}

int gps_l89hs_cmd_set_baud(gps_l89hs_t *dev, uint32_t baud) {
    return gnss_nmea_cmd_set_baud(dev, baud);
}

int gps_l89hs_cmd_standby(gps_l89hs_t *dev) {
    return gnss_nmea_cmd_standby(dev);
}

int gps_l89hs_cmd_hot_restart(gps_l89hs_t *dev) {
    return gnss_nmea_cmd_hot_restart(dev);
}

int gps_l89hs_ublox_set_rate(gps_l89hs_t *dev, uint16_t interval_ms) {
    return gnss_nmea_ublox_set_rate(dev, interval_ms);
}

int gps_l89hs_ublox_set_baud(gps_l89hs_t *dev, uint32_t baud) {
    return gnss_nmea_ublox_set_baud(dev, baud);
}
