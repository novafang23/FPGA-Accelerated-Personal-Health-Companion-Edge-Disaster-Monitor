/*
 * test_gps_l89hs.c
 * Unit Tests for GNSS NMEA-0183 Driver & Parser (u-blox NEO-M8N & Quectel L89HS)
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "gnss_nmea.h"
#include "gps_l89hs.h"

static void test_nmea_checksum(void) {
    printf("Running test_nmea_checksum...\n");

    /* Standard PMTK update rate command */
    uint8_t c1 = gps_nmea_checksum("PMTK220,1000");
    assert(c1 == 0x1F);

    /* Hot restart command */
    uint8_t c2 = gps_nmea_checksum("PMTK101");
    assert(c2 == 0x32);

    /* Standby command */
    uint8_t c3 = gps_nmea_checksum("PMTK161,0");
    assert(c3 == 0x28);

    /* Sentence starting with '$' and ending with '*' */
    uint8_t c4 = gps_nmea_checksum("$GNGGA,000000.000,0000.0000,N,00000.0000,E,0,00,0.0,0.0,M,0.0,M,,*");
    assert(c4 != 0);

    /* Direct generic GNSS checksum API */
    assert(gnss_nmea_checksum("PMTK220,1000") == 0x1F);

    printf("  [PASS] NMEA checksum calculations verified.\n");
}

static void test_rmc_parsing(void) {
    printf("Running test_rmc_parsing...\n");
    gps_l89hs_t dev;
    assert(gps_l89hs_init(&dev, 2) == 0);

    /* Real Quectel L89HS sample NMEA stream: Bhubaneswar, Odisha (20.296058 N, 85.824538 E) */
    /* 20 deg 17.7635 min N = 20 + 17.7635/60 = 20.296058 */
    /* 085 deg 49.4723 min E = 85 + 49.4723/60 = 85.824538 */
    const char *rmc_valid = "$GNRMC,092750.000,A,2017.7635,N,08549.4723,E,0.05,120.50,121026,,,A*7D\r\n";
    int ret = gps_l89hs_parse_line(&dev, rmc_valid);
    assert(ret == 1);

    gps_data_t data;
    assert(gps_l89hs_get_data(&dev, &data) == 0);
    assert(data.fix_valid == true);
    assert(fabsf(data.latitude - 20.296058f) < 0.0001f);
    assert(fabsf(data.longitude - 85.824538f) < 0.0001f);
    assert(data.lat_dir == 'N');
    assert(data.lon_dir == 'E');
    assert(fabsf(data.speed_kmh - (0.05f * 1.852f)) < 0.01f);
    assert(fabsf(data.course_deg - 120.50f) < 0.01f);
    assert(strcmp(data.utc_date, "121026") == 0);
    assert(strcmp(data.utc_time, "092750.000") == 0);

    char coord_str[GPS_COORD_STR_LEN];
    gps_l89hs_format_coords(&data, coord_str, sizeof(coord_str));
    printf("  Formatted coords: %s\n", coord_str);
    assert(strstr(coord_str, "20.29606N") != NULL);
    assert(strstr(coord_str, "85.82454E") != NULL);

    /* Now test RMC invalid / searching fix ('V' status) */
    const char *rmc_invalid = "$GNRMC,092751.000,V,2017.7635,N,08549.4723,E,0.00,0.00,121026,,,N*67\r\n";
    ret = gps_l89hs_parse_line(&dev, rmc_invalid);
    assert(ret == 1);
    assert(gps_l89hs_has_fix(&dev) == false);

    gps_l89hs_format_coords(&dev.data, coord_str, sizeof(coord_str));
    assert(strcmp(coord_str, "NO FIX") == 0);

    printf("  [PASS] RMC sentences parsed and validated.\n");
}

static void test_gga_parsing(void) {
    printf("Running test_gga_parsing...\n");
    gps_l89hs_t dev;
    assert(gps_l89hs_init(&dev, 2) == 0);

    /* Real Quectel L89HS sample GGA line */
    const char *gga_valid = "$GNGGA,092750.000,2017.7635,N,08549.4723,E,1,09,1.1,45.3,M,0.0,M,,*41\r\n";
    int ret = gps_l89hs_parse_line(&dev, gga_valid);
    assert(ret == 1);

    gps_data_t data;
    assert(gps_l89hs_get_data(&dev, &data) == 0);
    assert(data.fix_valid == true);
    assert(data.fix_quality == 1);
    assert(data.satellites == 9);
    assert(fabsf(data.hdop - 1.1f) < 0.01f);
    assert(fabsf(data.altitude_m - 45.3f) < 0.01f);

    printf("  [PASS] GGA sentences parsed (Satellites: %u, Altitude: %.1fm, HDOP: %.1f).\n",
           data.satellites, data.altitude_m, data.hdop);
}

static void test_vtg_parsing(void) {
    printf("Running test_vtg_parsing...\n");
    gnss_nmea_t dev;
    assert(gnss_nmea_init(&dev, 2) == 0);

    /* Test $GNVTG sentence with true heading, knots, and km/h */
    const char *vtg_gn = "$GNVTG,77.52,T,,M,0.004,N,0.008,K,A*18\r\n";
    int ret = gnss_nmea_parse_line(&dev, vtg_gn);
    assert(ret == 1);
    assert(fabsf(dev.data.course_deg - 77.52f) < 0.01f);
    assert(fabsf(dev.data.speed_kmh - 0.008f) < 0.001f);

    /* Test $GPVTG with faster motion (10.2 km/h, 54.7 deg true course) */
    const char *vtg_gp = "$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K,A*25\r\n";
    ret = gnss_nmea_parse_line(&dev, vtg_gp);
    assert(ret == 1);
    assert(fabsf(dev.data.course_deg - 54.7f) < 0.01f);
    assert(fabsf(dev.data.speed_kmh - 10.2f) < 0.01f);

    /* Test VTG with mode 'N' (invalid) - should not overwrite previous valid values */
    const char *vtg_invalid = "$GNVTG,,,,,,,,,N*2E\r\n";
    ret = gnss_nmea_parse_line(&dev, vtg_invalid);
    assert(ret == 1);
    assert(fabsf(dev.data.course_deg - 54.7f) < 0.01f);
    assert(fabsf(dev.data.speed_kmh - 10.2f) < 0.01f);

    printf("  [PASS] VTG course & speed parsing verified.\n");
}

static void test_southern_western_hemispheres(void) {
    printf("Running test_southern_western_hemispheres...\n");
    gps_l89hs_t dev;
    assert(gps_l89hs_init(&dev, 2) == 0);

    /* Santiago, Chile: 33 deg 27.0000 S, 70 deg 40.0000 W -> -33.4500, -70.6667 */
    /* 33 deg 27.0 min = 33 + 0.45 = -33.45 */
    /* 70 deg 40.0 min = 70 + 0.666667 = -70.666667 */
    const char *rmc_south_west = "$GNRMC,120000.000,A,3327.0000,S,07040.0000,W,0.0,0.0,121026,,,A*7C\r\n";
    int ret = gps_l89hs_parse_line(&dev, rmc_south_west);
    assert(ret == 1);
    assert(dev.data.fix_valid == true);
    assert(fabsf(dev.data.latitude - (-33.45f)) < 0.001f);
    assert(fabsf(dev.data.longitude - (-70.6667f)) < 0.001f);
    assert(dev.data.lat_dir == 'S');
    assert(dev.data.lon_dir == 'W');

    printf("  [PASS] S/W signed coordinates verified (Lat: %.4f, Lon: %.4f).\n",
           dev.data.latitude, dev.data.longitude);
}

static void test_checksum_rejection(void) {
    printf("Running test_checksum_rejection...\n");
    gps_l89hs_t dev;
    assert(gps_l89hs_init(&dev, 2) == 0);

    /* Valid sentence has checksum 7C; pass corrupt checksum 00 */
    const char *rmc_corrupt = "$GNRMC,092750.000,A,2017.7635,N,08549.4723,E,0.05,120.50,121026,,,A*00\r\n";
    int ret = gps_l89hs_parse_line(&dev, rmc_corrupt);
    assert(ret == -1);
    assert(dev.data.checksum_errors == 1);
    assert(dev.data.fix_valid == false);

    printf("  [PASS] Corrupted sentence safely rejected.\n");
}

static void test_byte_stream_feeder(void) {
    printf("Running test_byte_stream_feeder...\n");
    gps_l89hs_t dev;
    assert(gps_l89hs_init(&dev, 2) == 0);

    const char *stream =
        "garbage prefix before dollar"
        "$GNRMC,092750.000,A,2017.7635,N,08549.4723,E,0.05,120.50,121026,,,A*7D\r\n"
        "$GNGGA,092750.000,2017.7635,N,08549.4723,E,1,11,0.9,45.3,M,0.0,M,,*41\r\n"
        "trailing line without dollar";

    int valid_frames = 0;
    for (size_t i = 0; i < strlen(stream); i++) {
        if (gps_l89hs_feed_byte(&dev, (uint8_t)stream[i]) == 1) {
            valid_frames++;
        }
    }

    assert(valid_frames == 2);
    assert(dev.data.fix_valid == true);
    assert(dev.data.satellites == 11);
    assert(dev.data.sentences_parsed == 2);

    printf("  [PASS] Byte-by-byte UART stream assembled and parsed (%d sentences).\n", valid_frames);
}

static void test_ublox_neom8n_stream(void) {
    printf("Running test_ublox_neom8n_stream...\n");
    gnss_nmea_t dev;
    assert(gnss_nmea_init(&dev, 2) == 0);

    /*
     * Real u-blox NEO-M8N burst stream captured at Thalwil, Switzerland:
     * Lat: 47 deg 17.11437 N = 47.28524
     * Lon: 08 deg 33.91522 E = 8.56525
     * Alt: 445.4 m
     * Speed: 0.008 km/h, Course: 77.52 deg
     */
    const char *ublox_stream =
        "$GNRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091218,,,A*42\r\n"
        "$GNVTG,77.52,T,,M,0.004,N,0.008,K,A*18\r\n"
        "$GNGGA,083559.00,4717.11437,N,00833.91522,E,1,08,1.01,445.4,M,46.7,M,,*4C\r\n";

    int frames = 0;
    for (size_t i = 0; i < strlen(ublox_stream); i++) {
        if (gnss_nmea_feed_byte(&dev, (uint8_t)ublox_stream[i]) == 1) {
            frames++;
        }
    }

    assert(frames == 3);
    assert(dev.data.fix_valid == true);
    assert(dev.data.fix_quality == 1);
    assert(dev.data.satellites == 8);
    assert(fabsf(dev.data.latitude - 47.28524f) < 0.0001f);
    assert(fabsf(dev.data.longitude - 8.56525f) < 0.0001f);
    assert(fabsf(dev.data.altitude_m - 445.4f) < 0.1f);
    assert(fabsf(dev.data.course_deg - 77.52f) < 0.01f);
    assert(fabsf(dev.data.speed_kmh - 0.008f) < 0.001f);

    char coords[GNSS_COORD_STR_LEN];
    gnss_nmea_format_coords(&dev.data, coords, sizeof(coords));
    printf("  u-blox NEO-M8N Formatted Coords: %s\n", coords);
    assert(strstr(coords, "47.28524N") != NULL);
    assert(strstr(coords, "8.56525E") != NULL);

    printf("  [PASS] u-blox NEO-M8N multi-sentence burst parsed successfully.\n");
}

static void test_gp_talker_sentences(void) {
    printf("Running test_gp_talker_sentences...\n");
    gnss_nmea_t dev;
    assert(gnss_nmea_init(&dev, 2) == 0);

    /* Test $GPRMC and $GPGGA legacy talker format (e.g. GPS-only or older receivers) */
    const char *gp_rmc = "$GPRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W,A*29\r\n";
    assert(gnss_nmea_parse_line(&dev, gp_rmc) == 1);
    assert(dev.data.fix_valid == true);
    assert(fabsf(dev.data.latitude - 48.11730f) < 0.0001f);
    assert(fabsf(dev.data.longitude - 11.51667f) < 0.0001f);
    assert(fabsf(dev.data.course_deg - 84.4f) < 0.01f);

    const char *gp_gga = "$GPGGA,123519.00,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*69\r\n";
    assert(gnss_nmea_parse_line(&dev, gp_gga) == 1);
    assert(dev.data.satellites == 8);
    assert(fabsf(dev.data.altitude_m - 545.4f) < 0.1f);
    assert(fabsf(dev.data.hdop - 0.9f) < 0.01f);

    printf("  [PASS] GP talker sentences parsed successfully.\n");
}

static void test_command_builders(void) {
    printf("Running test_command_builders...\n");
    gnss_nmea_t dev;
    assert(gnss_nmea_init(&dev, 2) == 0);

    /* PMTK commands (Quectel) */
    assert(gnss_nmea_cmd_set_rate(&dev, 1000) > 0);
    assert(gnss_nmea_cmd_set_baud(&dev, 115200) > 0);
    assert(gnss_nmea_cmd_standby(&dev) > 0);
    assert(gnss_nmea_cmd_hot_restart(&dev) > 0);

    /* UBX binary protocol commands (u-blox NEO-M8N) */
    assert(gnss_nmea_ublox_set_rate(&dev, 1000) == 14); /* 8 overhead + 6 payload = 14 bytes */
    uint8_t dummy_payload[4] = {1, 2, 3, 4};
    assert(gnss_nmea_send_ubx(&dev, 0x06, 0x01, dummy_payload, 4) == 12);
    assert(gnss_nmea_ublox_set_baud(&dev, 115200) > 0);

    /* Compatibility wrappers */
    assert(gps_l89hs_cmd_set_rate(&dev, 1000) > 0);
    assert(gps_l89hs_cmd_set_baud(&dev, 9600) > 0);
    assert(gps_l89hs_ublox_set_rate(&dev, 1000) == 14);
    assert(gps_l89hs_ublox_set_baud(&dev, 115200) > 0);

    /* Command with leading '$' should not generate double '$$' */
    assert(gnss_nmea_send_command(&dev, "$PMTK220,1000") > 0);

    printf("  [PASS] Command and UBX protocol generators verified.\n");
}

static void test_gga_fix_loss_and_recovery(void) {
    printf("Running test_gga_fix_loss_and_recovery...\n");
    gnss_nmea_t dev;
    assert(gnss_nmea_init(&dev, 2) == 0);

    /* 1. Acquire fix via valid GGA */
    const char *gga_valid = "$GNGGA,092750.000,2017.7635,N,08549.4723,E,1,09,1.1,45.3,M,0.0,M,,*41\r\n";
    assert(gnss_nmea_parse_line(&dev, gga_valid) == 1);
    assert(dev.data.fix_valid == true);
    assert(dev.data.fix_quality == 1);
    assert(fabsf(dev.data.latitude - 20.296058f) < 0.0001f);

    /* 2. Fix lost (Quality drops to 0) */
    const char *gga_lost = "$GNGGA,092751.000,0000.0000,N,00000.0000,E,0,00,0.0,0.0,M,0.0,M,,*7B\r\n";
    assert(gnss_nmea_parse_line(&dev, gga_lost) == 1);
    assert(dev.data.fix_valid == false);
    assert(dev.data.fix_quality == 0);
    assert(gnss_nmea_has_fix(&dev) == false);
    /* Latitude should NOT be overwritten by zero/invalid coordinates during lost fix */
    assert(fabsf(dev.data.latitude - 20.296058f) < 0.0001f);

    char coord_str[GNSS_COORD_STR_LEN];
    gnss_nmea_format_coords(&dev.data, coord_str, sizeof(coord_str));
    assert(strcmp(coord_str, "NO FIX") == 0);

    /* 3. Fix re-acquired */
    const char *gga_rec = "$GNGGA,092752.000,2017.7635,N,08549.4723,E,1,10,0.9,45.3,M,0.0,M,,*42\r\n";
    assert(gnss_nmea_parse_line(&dev, gga_rec) == 1);
    assert(dev.data.fix_valid == true);
    assert(dev.data.fix_quality == 1);
    assert(dev.data.satellites == 10);
    assert(gnss_nmea_has_fix(&dev) == true);

    /* Expiry uses wrap-safe uint32 millisecond arithmetic and invalidates every
     * API view of the fix, not only the application's display flag. */
    dev.data.last_fix_time_ms = UINT32_MAX - 2500u;
    assert(gnss_nmea_expire_fix(&dev, 2499u, 5000u) == false);
    assert(gnss_nmea_has_fix(&dev) == true);
    assert(gnss_nmea_expire_fix(&dev, 2500u, 5000u) == true);
    assert(gnss_nmea_has_fix(&dev) == false);
    gnss_data_t snapshot;
    assert(gnss_nmea_get_data(&dev, &snapshot) == -1);
    assert(snapshot.fix_valid == false);

    /* A fresh valid report re-establishes the fix after expiry. */
    assert(gnss_nmea_parse_line(&dev, gga_rec) == 1);
    assert(gnss_nmea_has_fix(&dev) == true);

    /* 4. RMC fix loss also clears fix_quality */
    const char *rmc_lost = "$GNRMC,092753.000,V,2017.7635,N,08549.4723,E,0.00,0.00,121026,,,N*65\r\n";
    assert(gnss_nmea_parse_line(&dev, rmc_lost) == 1);
    assert(dev.data.fix_valid == false);
    assert(dev.data.fix_quality == 0);

    printf("  [PASS] GGA fix loss and recovery state machine verified.\n");
}

static void test_malformed_and_truncated_safety(void) {
    printf("Running test_malformed_and_truncated_safety...\n");
    gnss_nmea_t dev;
    assert(gnss_nmea_init(&dev, 2) == 0);

    /* Truncated at asterisk */
    assert(gnss_nmea_parse_line(&dev, "$GNRMC,123*") == -1);

    /* Truncated with single char after asterisk */
    assert(gnss_nmea_parse_line(&dev, "$GNRMC,123*A") == -1);

    /* Non-hex characters in checksum */
    assert(gnss_nmea_parse_line(&dev, "$GNRMC,123*ZZ") == -1);

    /* Whitespace prefix tolerance */
    const char *rmc_spaces = "   \t$GNRMC,092750.000,A,2017.7635,N,08549.4723,E,0.05,120.50,121026,,,A*7D\r\n";
    assert(gnss_nmea_parse_line(&dev, rmc_spaces) == 1);
    assert(dev.data.fix_valid == true);

    /* Coordinate formatting with negative lat/lon and missing direction indicators */
    gnss_data_t neg_data;
    memset(&neg_data, 0, sizeof(neg_data));
    neg_data.fix_valid = true;
    neg_data.latitude = -33.45f;
    neg_data.longitude = -70.6667f;
    neg_data.lat_dir = '\0';
    neg_data.lon_dir = '\0';
    char buf[GNSS_COORD_STR_LEN];
    gnss_nmea_format_coords(&neg_data, buf, sizeof(buf));
    assert(strstr(buf, "33.45000S") != NULL);
    assert(strstr(buf, "70.66670W") != NULL);

    printf("  [PASS] Truncated and malformed sentence memory safety verified.\n");
}

int main(void) {
    printf("================================================================\n");
    printf("  Unit Tests: GNSS NMEA Driver & Parser (u-blox M8N & Quectel)\n");
    printf("================================================================\n\n");

    test_nmea_checksum();
    test_rmc_parsing();
    test_gga_parsing();
    test_vtg_parsing();
    test_southern_western_hemispheres();
    test_checksum_rejection();
    test_byte_stream_feeder();
    test_ublox_neom8n_stream();
    test_gp_talker_sentences();
    test_command_builders();
    test_gga_fix_loss_and_recovery();
    test_malformed_and_truncated_safety();

    printf("\n>>> ALL GNSS NMEA TESTS PASSED (12/12) <<<\n");
    return 0;
}
