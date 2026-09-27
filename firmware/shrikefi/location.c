/*
 * location.c — Stored deployment location (T1.4)
 * SIH26181 / VALOR
 *
 * See location.h for why the location is text, and why the parsing is kept
 * free of ESP-IDF.
 */

#include "location.h"

#include <stdio.h>
#include <string.h>

#define LOCATION_NVS_NAMESPACE "valor"
#define LOCATION_NVS_KEY       "loc"

static char s_location[LOCATION_MAX_LEN] = "UNSET";
static bool s_is_set = false;

/* ------------------------------------------------------------------------- */
/* Pure: sanitising                                                           */
/* ------------------------------------------------------------------------- */

bool location_sanitize(const char *in, char *out, size_t cap) {
    if (!out || cap == 0) return false;
    out[0] = '\0';
    if (!in) return false;

    size_t n = 0;
    bool pending_space = false;

    for (const unsigned char *p = (const unsigned char *)in; *p; p++) {
        unsigned char c = *p;

        /* Whitespace collapses to a single space and never leads. Dropping it
         * outright would weld words together - "Ward 3\tKolar" would become
         * "Ward3Kolar" - which is a worse failure than a slightly longer line. */
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (n > 0) pending_space = true;
            continue;
        }

        /* Printable ASCII only. Anything else is a control byte from a terminal
         * or a UTF-8 continuation byte whose lead byte was already dropped;
         * either would corrupt the display or split a glyph mid-sequence. */
        if (c < 0x20 || c > 0x7E) continue;

        if (pending_space) {
            if (n + 2 >= cap) break;   /* space + at least one char + NUL */
            out[n++] = ' ';
            pending_space = false;
        }
        if (n + 1 >= cap) break;       /* leave room for the terminator */
        out[n++] = (char)c;
    }

    out[n] = '\0';
    return n > 0;
}

/* ------------------------------------------------------------------------- */
/* Pure: form decoding                                                        */
/* ------------------------------------------------------------------------- */

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool location_parse_form(const char *body, char *out, size_t cap) {
    if (!body || !out || cap == 0) return false;
    out[0] = '\0';

    /* Find the loc field. It is not assumed to be first or only: a browser
     * submits fields in document order, and a form that grows a second input
     * later must not silently stop working. */
    const char *p = body;
    size_t keylen = 4; /* strlen("loc=") - getting this wrong silently never matches */
    while (*p) {
        if (strncmp(p, "loc=", keylen) == 0 && (p == body || p[-1] == '&')) break;
        p++;
    }
    if (!*p) return false;
    p += keylen;

    /* Decode into a scratch buffer, then sanitise. Decoding and sanitising are
     * separate because a %00 in the body must not terminate the value early,
     * and sanitising is what decides which decoded bytes are printable. */
    char decoded[LOCATION_MAX_LEN * 4];
    size_t d = 0;
    while (*p && *p != '&' && d + 1 < sizeof(decoded)) {
        if (*p == '+') {
            decoded[d++] = ' ';
            p++;
        } else if (*p == '%' && hex_val(p[1]) >= 0 && hex_val(p[2]) >= 0) {
            decoded[d++] = (char)((hex_val(p[1]) << 4) | hex_val(p[2]));
            p += 3;
        } else {
            decoded[d++] = *p++;
        }
    }
    decoded[d] = '\0';

    return location_sanitize(decoded, out, cap);
}

/* ------------------------------------------------------------------------- */
/* Pure: card rendering                                                       */
/* ------------------------------------------------------------------------- */

void location_card_text(const char *loc, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!loc) return;

    size_t len = strlen(loc);
    if (len <= LOCATION_CARD_COLS || LOCATION_CARD_COLS + 4 > cap) {
        /* Fits, or there is no room for a marker either - copy what fits and
         * let the caller's buffer bound it. */
        size_t n = (len < cap - 1) ? len : cap - 1;
        memcpy(out, loc, n);
        out[n] = '\0';
        return;
    }

    /* Too long for the panel. Show as much as fits minus the marker and say so
     * rather than cutting a district name in half with no indication. */
    const char *marker = "...";
    size_t keep = LOCATION_CARD_COLS - strlen(marker);
    memcpy(out, loc, keep);
    memcpy(out + keep, marker, strlen(marker));
    out[keep + strlen(marker)] = '\0';
}

/* ------------------------------------------------------------------------- */
/* Device-facing                                                              */
/* ------------------------------------------------------------------------- */

#ifndef ESP_PLATFORM

/* Host build: an in-memory store, so set/get/round-trip is testable. */
void location_init(void) {
    s_is_set = false;
    strncpy(s_location, "UNSET", sizeof(s_location) - 1);
    s_location[sizeof(s_location) - 1] = '\0';
}

bool location_set(const char *raw) {
    char clean[LOCATION_MAX_LEN];
    if (!location_sanitize(raw, clean, sizeof(clean))) return false;
    strncpy(s_location, clean, sizeof(s_location) - 1);
    s_location[sizeof(s_location) - 1] = '\0';
    s_is_set = true;
    return true;
}

const char *location_get(void) { return s_location; }
bool location_is_set(void) { return s_is_set; }

#else  /* ESP_PLATFORM */

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "LOCATION";

void location_init(void) {
    nvs_handle_t h;
    if (nvs_open(LOCATION_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        s_is_set = false;
        strncpy(s_location, "UNSET", sizeof(s_location) - 1);
        s_location[sizeof(s_location) - 1] = '\0';
        return;
    }

    size_t len = sizeof(s_location);
    esp_err_t err = nvs_get_str(h, LOCATION_NVS_KEY, s_location, &len);
    nvs_close(h);

    char clean[LOCATION_MAX_LEN];
    if (err == ESP_OK && location_sanitize(s_location, clean, sizeof(clean))) {
        strncpy(s_location, clean, sizeof(s_location) - 1);
        s_location[sizeof(s_location) - 1] = '\0';
        s_is_set = true;
        ESP_LOGI(TAG, "Stored location: '%s'", s_location);
    } else {
        /* Nothing stored yet, or what was stored no longer sanitises. Say so
         * rather than showing a stale or malformed string. */
        s_is_set = false;
        strncpy(s_location, "UNSET", sizeof(s_location) - 1);
        s_location[sizeof(s_location) - 1] = '\0';
        ESP_LOGI(TAG, "No stored location; the card and page will show UNSET");
    }
}

bool location_set(const char *raw) {
    char clean[LOCATION_MAX_LEN];
    if (!location_sanitize(raw, clean, sizeof(clean))) return false;

    /* Update memory first. The value has to be visible for the rest of this
     * session even if NVS is full or unwritable - a caregiver setting the
     * location during an emergency should not lose it because a flash write
     * failed. */
    strncpy(s_location, clean, sizeof(s_location) - 1);
    s_location[sizeof(s_location) - 1] = '\0';
    s_is_set = true;

    nvs_handle_t h;
    esp_err_t err = nvs_open(LOCATION_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed (%s); location kept in RAM only", esp_err_to_name(err));
        return true;
    }
    err = nvs_set_str(h, LOCATION_NVS_KEY, clean);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs write failed (%s); location kept in RAM only", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Location set to '%s' (persisted)", s_location);
    }
    return true;
}

const char *location_get(void) { return s_location; }
bool location_is_set(void) { return s_is_set; }

#endif /* ESP_PLATFORM */
