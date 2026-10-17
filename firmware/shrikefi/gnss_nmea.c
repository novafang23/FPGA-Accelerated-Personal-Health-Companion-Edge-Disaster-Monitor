/*
 * gnss_nmea.c
 * Generic GNSS (GPS / GLONASS / Galileo / BeiDou) Driver & NMEA-0183 Parser
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 */

#include "gnss_nmea.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "GNSS_NMEA";
static portMUX_TYPE s_gnss_mux = portMUX_INITIALIZER_UNLOCKED;
#define GNSS_LOCK()   portENTER_CRITICAL(&s_gnss_mux)
#define GNSS_UNLOCK() portEXIT_CRITICAL(&s_gnss_mux)

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

#else
/* Host test environment */
#define ESP_LOGI(tag, ...) do {} while(0)
#define ESP_LOGW(tag, ...) do {} while(0)
#define ESP_LOGE(tag, ...) do {} while(0)
static const char *TAG __attribute__((unused)) = "GNSS_NMEA";
#define GNSS_LOCK()
#define GNSS_UNLOCK()

static uint32_t s_mock_time_ms = 1000;
static uint32_t get_time_ms(void) {
    return s_mock_time_ms;
}
#endif

/* ------------------------------------------------------------------------- */
/* Pure: NMEA Checksum Calculation                                           */
/* ------------------------------------------------------------------------- */

uint8_t gnss_nmea_checksum(const char *sentence) {
    if (!sentence) return 0;
    const char *p = sentence;
    if (*p == '$' || *p == '!') p++;

    uint8_t sum = 0;
    while (*p && *p != '*' && *p != '\r' && *p != '\n') {
        sum ^= (uint8_t)(*p);
        p++;
    }
    return sum;
}

static int hex_char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* ------------------------------------------------------------------------- */
/* Pure: Coordinate Conversion (DDMM.MMMM -> Signed Decimal Degrees)         */
/* ------------------------------------------------------------------------- */

static float nmea_to_decimal_degrees(const char *raw, char dir) {
    if (!raw || raw[0] == '\0') return 0.0f;

    /* Trim leading whitespace */
    while (*raw == ' ' || *raw == '\t') raw++;
    if (*raw == '\0') return 0.0f;

    const char *dot = strchr(raw, '.');
    if (!dot || (dot - raw) < 2) return 0.0f;

    /* Minutes are the 2 digits preceding the decimal point plus fractional part */
    const char *min_start = dot - 2;
    float minutes = strtof(min_start, NULL);

    /* Degrees are the leading digits preceding the minutes */
    char deg_buf[8];
    size_t deg_len = (size_t)(min_start - raw);
    if (deg_len >= sizeof(deg_buf)) return 0.0f;

    memcpy(deg_buf, raw, deg_len);
    deg_buf[deg_len] = '\0';
    float degrees = strtof(deg_buf, NULL);

    float decimal = degrees + (minutes / 60.0f);
    if (dir == 'S' || dir == 's' || dir == 'W' || dir == 'w') {
        decimal = -decimal;
    }
    return decimal;
}

/* ------------------------------------------------------------------------- */
/* NMEA Sentence Parsers: RMC, GGA, and VTG                                  */
/* ------------------------------------------------------------------------- */

static void parse_rmc(gnss_nmea_t *dev, char *fields[], int num_fields) {
    /* Minimum fields for valid RMC: 0:Talker, 1:Time, 2:Status, 3:Lat, 4:N/S, 5:Lon, 6:E/W */
    if (num_fields < 7) return;

    char status = fields[2][0];
    bool valid = (status == 'A' || status == 'a');

    GNSS_LOCK();
    dev->data.fix_valid = valid;
    if (!valid) {
        dev->data.fix_quality = 0;
    }

    if (num_fields > 1 && fields[1][0] != '\0') {
        snprintf(dev->data.utc_time, sizeof(dev->data.utc_time), "%s", fields[1]);
    }

    if (valid) {
        if (num_fields > 4 && fields[3][0] != '\0' && fields[4][0] != '\0') {
            dev->data.lat_dir = (char)toupper((unsigned char)fields[4][0]);
            dev->data.latitude = nmea_to_decimal_degrees(fields[3], dev->data.lat_dir);
        }
        if (num_fields > 6 && fields[5][0] != '\0' && fields[6][0] != '\0') {
            dev->data.lon_dir = (char)toupper((unsigned char)fields[6][0]);
            dev->data.longitude = nmea_to_decimal_degrees(fields[5], dev->data.lon_dir);
        }
        if (num_fields > 7 && fields[7][0] != '\0') {
            float knots = strtof(fields[7], NULL);
            dev->data.speed_kmh = knots * 1.852f;
        }
        if (num_fields > 8 && fields[8][0] != '\0') {
            dev->data.course_deg = strtof(fields[8], NULL);
        }
        if (num_fields > 9 && fields[9][0] != '\0') {
            snprintf(dev->data.utc_date, sizeof(dev->data.utc_date), "%s", fields[9]);
        }
        dev->data.last_fix_time_ms = get_time_ms();
    }
    dev->data.sentences_parsed++;
    GNSS_UNLOCK();
}

static void parse_gga(gnss_nmea_t *dev, char *fields[], int num_fields) {
    /* Minimum fields: 0:Talker, 1:Time, 2:Lat, 3:N/S, 4:Lon, 5:E/W, 6:Quality, 7:Sats */
    if (num_fields < 8) return;

    int quality = fields[6][0] ? (fields[6][0] - '0') : 0;
    /* A GGA that claims fix quality but reports zero satellites is not a usable
     * fix. This was seen right after boot, before the receiver re-acquired: the
     * device briefly reported a 3-D fix with Sats 0 / Alt 0 / HDOP 0 and would
     * have published a bogus coordinate. Require a real satellite count too. */
    int sats = (num_fields > 7 && fields[7][0] != '\0') ? atoi(fields[7]) : 0;
    bool valid = (quality > 0) && (sats > 0);

    GNSS_LOCK();
    dev->data.fix_quality = (uint8_t)(quality > 0 ? quality : 0);
    dev->data.fix_valid = valid;

    if (num_fields > 1 && fields[1][0] != '\0') {
        snprintf(dev->data.utc_time, sizeof(dev->data.utc_time), "%s", fields[1]);
    }
    if (valid) {
        if (num_fields > 5 && fields[2][0] != '\0' && fields[3][0] != '\0' &&
            fields[4][0] != '\0' && fields[5][0] != '\0') {
            dev->data.lat_dir = (char)toupper((unsigned char)fields[3][0]);
            dev->data.latitude = nmea_to_decimal_degrees(fields[2], dev->data.lat_dir);
            dev->data.lon_dir = (char)toupper((unsigned char)fields[5][0]);
            dev->data.longitude = nmea_to_decimal_degrees(fields[4], dev->data.lon_dir);
        }
        if (num_fields > 8 && fields[8][0] != '\0') {
            dev->data.hdop = strtof(fields[8], NULL);
        }
        if (num_fields > 9 && fields[9][0] != '\0') {
            dev->data.altitude_m = strtof(fields[9], NULL);
        }
        dev->data.last_fix_time_ms = get_time_ms();
    }
    if (num_fields > 7 && fields[7][0] != '\0') {
        dev->data.satellites = (uint8_t)atoi(fields[7]);
    }
    dev->data.sentences_parsed++;
    GNSS_UNLOCK();
}

static void parse_vtg(gnss_nmea_t *dev, char *fields[], int num_fields) {
    /* Minimum fields for VTG: talker, true course, T, mag course, M, speed knots, N, speed kmh, K */
    if (num_fields < 8) return;

    GNSS_LOCK();
    /* If mode indicator present and explicitly 'N' (Data Not Valid), do not update motion metrics */
    bool valid = true;
    if (num_fields > 9 && (fields[9][0] == 'N' || fields[9][0] == 'n')) {
        valid = false;
    }

    if (valid) {
        if (num_fields > 1 && fields[1][0] != '\0') {
            dev->data.course_deg = strtof(fields[1], NULL);
        }
        if (num_fields > 7 && fields[7][0] != '\0') {
            dev->data.speed_kmh = strtof(fields[7], NULL);
        } else if (num_fields > 5 && fields[5][0] != '\0') {
            float knots = strtof(fields[5], NULL);
            dev->data.speed_kmh = knots * 1.852f;
        }
    }
    dev->data.sentences_parsed++;
    GNSS_UNLOCK();
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                */
/* ------------------------------------------------------------------------- */

int gnss_nmea_init(gnss_nmea_t *dev, int uart_num) {
    if (!dev) return -1;

    memset(dev, 0, sizeof(*dev));
    dev->uart_num = uart_num;
    dev->initialized = 1;
    dev->line_idx = 0;
    dev->in_sentence = false;
    dev->data.lat_dir = 'N';
    dev->data.lon_dir = 'E';

    return 0;
}

int gnss_nmea_parse_line(gnss_nmea_t *dev, const char *line) {
    if (!dev || !dev->initialized || !line) return -1;

    /* Trim leading whitespace */
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
    if (*line != '$' && *line != '!') return -1;

    /* Verify Checksum */
    const char *star = strchr(line, '*');
    if (!star) return -1;

    if (star[1] == '\0' || star[2] == '\0') {
        GNSS_LOCK();
        dev->data.checksum_errors++;
        GNSS_UNLOCK();
        return -1;
    }

    int h1 = hex_char_to_val(star[1]);
    int h2 = hex_char_to_val(star[2]);
    if (h1 < 0 || h2 < 0) {
        GNSS_LOCK();
        dev->data.checksum_errors++;
        GNSS_UNLOCK();
        return -1;
    }
    uint8_t expected_sum = (uint8_t)((h1 << 4) | h2);
    uint8_t actual_sum = gnss_nmea_checksum(line);

    if (expected_sum != actual_sum) {
        GNSS_LOCK();
        dev->data.checksum_errors++;
        GNSS_UNLOCK();
        static uint32_t s_last_csum_warn_ms = 0;
        uint32_t now = get_time_ms();
        if ((now - s_last_csum_warn_ms) >= 5000) {
            s_last_csum_warn_ms = now;
            ESP_LOGW(TAG, "NMEA Checksum error (rate-limited 5s): exp 0x%02X vs got 0x%02X", expected_sum, actual_sum);
        }
        return -1;
    }

    GNSS_LOCK();
    dev->data.sentences_received++;
    GNSS_UNLOCK();

    /* Copy sentence content between '$' and '*' for CSV splitting */
    char scratch[GNSS_NMEA_MAX_SENTENCE_LEN];
    size_t body_len = (size_t)(star - (line + 1));
    if (body_len >= sizeof(scratch)) return -1;

    memcpy(scratch, line + 1, body_len);
    scratch[body_len] = '\0';

    /* Tokenize by ',' */
    char *fields[24];
    int num_fields = 0;
    char *p = scratch;
    fields[num_fields++] = p;

    while (*p && num_fields < 24) {
        if (*p == ',') {
            *p = '\0';
            fields[num_fields++] = p + 1;
        }
        p++;
    }

    const char *talker = fields[0];
    size_t tlen = strlen(talker);
    if (tlen < 3) return 0;

    /* Match sentence type ignoring 2-letter talker prefix (GP, GN, GL, GA, BD, QZ) */
    const char *type = (tlen >= 3) ? (talker + (tlen - 3)) : talker;

    if (strcmp(type, "RMC") == 0) {
        parse_rmc(dev, fields, num_fields);
        return 1;
    } else if (strcmp(type, "GGA") == 0) {
        parse_gga(dev, fields, num_fields);
        return 1;
    } else if (strcmp(type, "VTG") == 0) {
        parse_vtg(dev, fields, num_fields);
        return 1;
    }

    return 0;
}

int gnss_nmea_feed_byte(gnss_nmea_t *dev, uint8_t byte) {
    if (!dev || !dev->initialized) return 0;

    char c = (char)byte;

    /* Start of sentence */
    if (c == '$' || c == '!') {
        dev->in_sentence = true;
        dev->line_idx = 0;
        dev->line_buf[dev->line_idx++] = c;
        dev->line_buf[dev->line_idx] = '\0';
        return 0;
    }

    if (!dev->in_sentence) return 0;

    /* End of sentence */
    if (c == '\r' || c == '\n') {
        if (dev->line_idx > 4) {
            dev->line_buf[dev->line_idx] = '\0';
            dev->in_sentence = false;
            return gnss_nmea_parse_line(dev, dev->line_buf) == 1 ? 1 : 0;
        }
        dev->in_sentence = false;
        dev->line_idx = 0;
        return 0;
    }

    /* Append character if buffer space permits */
    if (dev->line_idx + 1 < sizeof(dev->line_buf)) {
        dev->line_buf[dev->line_idx++] = c;
        dev->line_buf[dev->line_idx] = '\0';
    } else {
        /* Line exceeded maximum length without terminator; reset buffer */
        dev->in_sentence = false;
        dev->line_idx = 0;
    }

    return 0;
}

int gnss_nmea_get_data(const gnss_nmea_t *dev, gnss_data_t *out) {
    if (!dev || !out) return -1;
    GNSS_LOCK();
    *out = dev->data;
    GNSS_UNLOCK();
    return out->fix_valid ? 0 : -1;
}

bool gnss_nmea_has_fix(const gnss_nmea_t *dev) {
    if (!dev) return false;
    GNSS_LOCK();
    bool fix = dev->data.fix_valid;
    GNSS_UNLOCK();
    return fix;
}

bool gnss_nmea_expire_fix(gnss_nmea_t *dev, uint32_t now_ms,
                          uint32_t max_age_ms) {
    if (!dev) return false;
    GNSS_LOCK();
    bool expired = dev->data.fix_valid &&
                   (uint32_t)(now_ms - dev->data.last_fix_time_ms) > max_age_ms;
    if (expired) {
        dev->data.fix_valid = false;
        dev->data.fix_quality = 0;
    }
    GNSS_UNLOCK();
    return expired;
}

void gnss_nmea_format_coords(const gnss_data_t *data, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = '\0';

    if (!data || !data->fix_valid) {
        snprintf(out, cap, "NO FIX");
        return;
    }

    float abs_lat = fabsf(data->latitude);
    float abs_lon = fabsf(data->longitude);
    char lat_c = (data->lat_dir == 'S' || data->lat_dir == 's' || (data->lat_dir == '\0' && data->latitude < 0.0f)) ? 'S' : 'N';
    char lon_c = (data->lon_dir == 'W' || data->lon_dir == 'w' || (data->lon_dir == '\0' && data->longitude < 0.0f)) ? 'W' : 'E';

    snprintf(out, cap, "%.5f%c, %.5f%c", (double)abs_lat, lat_c, (double)abs_lon, lon_c);
}

int gnss_nmea_uart_poll(gnss_nmea_t *dev, TickType_t timeout) {
#ifdef ESP_PLATFORM
    if (!dev || !dev->initialized) return -1;

    uint8_t byte;
    int sentences_found = 0;
    while (1) {
        int len = uart_read_bytes(dev->uart_num, &byte, 1, timeout);
        if (len <= 0) break;
        if (gnss_nmea_feed_byte(dev, byte) == 1) {
            sentences_found++;
        }
        /* Only block on the first byte; subsequent bytes in buffer are drained immediately */
        timeout = 0;
    }
    return sentences_found;
#else
    (void)dev; (void)timeout;
    return 0;
#endif
}

/* ------------------------------------------------------------------------- */
/* Command Transmission Helpers                                              */
/* ------------------------------------------------------------------------- */

int gnss_nmea_send_raw(gnss_nmea_t *dev, const char *raw_str) {
    if (!dev || !dev->initialized || !raw_str) return -1;
#ifdef ESP_PLATFORM
    size_t len = strlen(raw_str);
    return uart_write_bytes(dev->uart_num, raw_str, len);
#else
    return (int)strlen(raw_str);
#endif
}

int gnss_nmea_send_command(gnss_nmea_t *dev, const char *cmd_body) {
    if (!dev || !dev->initialized || !cmd_body) return -1;
    if (cmd_body[0] == '$') cmd_body++;

    uint8_t sum = gnss_nmea_checksum(cmd_body);
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "$%s*%02X\r\n", cmd_body, sum);
    if (n < 0 || (size_t)n >= sizeof(buf)) return -1;

    ESP_LOGI(TAG, "TX Command: %s", buf);
    return gnss_nmea_send_raw(dev, buf);
}

int gnss_nmea_cmd_set_rate(gnss_nmea_t *dev, uint16_t interval_ms) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "PMTK220,%u", (unsigned)interval_ms);
    return gnss_nmea_send_command(dev, cmd);
}

int gnss_nmea_cmd_set_baud(gnss_nmea_t *dev, uint32_t baud) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "PMTK251,%lu", (unsigned long)baud);
    return gnss_nmea_send_command(dev, cmd);
}

int gnss_nmea_cmd_standby(gnss_nmea_t *dev) {
    return gnss_nmea_send_command(dev, "PMTK161,0");
}

int gnss_nmea_cmd_hot_restart(gnss_nmea_t *dev) {
    return gnss_nmea_send_command(dev, "PMTK101");
}

/* ------------------------------------------------------------------------- */
/* u-blox Binary Protocol (UBX) Transmission Helpers                         */
/* ------------------------------------------------------------------------- */

static void ublox_calc_checksum(const uint8_t *data, size_t len, uint8_t *ck_a, uint8_t *ck_b) {
    uint8_t a = 0, b = 0;
    for (size_t i = 0; i < len; i++) {
        a = (uint8_t)(a + data[i]);
        b = (uint8_t)(b + a);
    }
    *ck_a = a;
    *ck_b = b;
}

int gnss_nmea_send_ubx(gnss_nmea_t *dev, uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len) {
    if (!dev || !dev->initialized) return -1;

    /* UBX frame: Sync1(0xB5), Sync2(0x62), Class, ID, Length(2B), Payload(len), CK_A, CK_B */
    uint8_t packet[128];
    size_t total_len = 8 + len;
    if (total_len > sizeof(packet)) return -1;

    packet[0] = 0xB5;
    packet[1] = 0x62;
    packet[2] = cls;
    packet[3] = id;
    packet[4] = (uint8_t)(len & 0xFF);
    packet[5] = (uint8_t)((len >> 8) & 0xFF);

    if (payload && len > 0) {
        memcpy(&packet[6], payload, len);
    }

    uint8_t ck_a = 0, ck_b = 0;
    ublox_calc_checksum(&packet[2], 4 + len, &ck_a, &ck_b);
    packet[6 + len] = ck_a;
    packet[7 + len] = ck_b;

#ifdef ESP_PLATFORM
    return uart_write_bytes(dev->uart_num, (const char *)packet, total_len);
#else
    return (int)total_len;
#endif
}

int gnss_nmea_ublox_set_rate(gnss_nmea_t *dev, uint16_t interval_ms) {
    /* UBX-CFG-RATE (Class 0x06, ID 0x08): measRate(2B), navRate(2B), timeRef(2B) */
    uint8_t payload[6];
    payload[0] = (uint8_t)(interval_ms & 0xFF);
    payload[1] = (uint8_t)((interval_ms >> 8) & 0xFF);
    payload[2] = 0x01; /* navRate = 1 cycle */
    payload[3] = 0x00;
    payload[4] = 0x01; /* timeRef = 1 (GPS time) */
    payload[5] = 0x00;

    return gnss_nmea_send_ubx(dev, 0x06, 0x08, payload, sizeof(payload));
}

int gnss_nmea_ublox_set_baud(gnss_nmea_t *dev, uint32_t baud) {
    char cmd[48];
    snprintf(cmd, sizeof(cmd), "PUBX,41,1,0007,0003,%lu,0", (unsigned long)baud);
    return gnss_nmea_send_command(dev, cmd);
}
