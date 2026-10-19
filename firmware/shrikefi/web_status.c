/*
 * web_status.c — Local status page over a self-hosted access point (T1.3)
 * SIH26181 / VALOR
 *
 * See web_status.h for why this exists and why the JSON formatter is kept
 * separate from the server.
 */

#include "web_status.h"
#include "location.h"
#include "sos.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Snapshot                                                                   */
/* ------------------------------------------------------------------------- */

static valor_status_t s_snap;

/* ------------------------------------------------------------------------- */
/* 24-Hour Rolling History (Requirement 7 / T3.2)                            */
/* ------------------------------------------------------------------------- */
/* 24 completed-hour slots, persisted to NVS as a canonical 612-byte blob and
 * restored on boot (see web_status.h). There is no calendar time source, so a
 * slot is "an hour of device uptime", not a clock hour; the UI must label it
 * that way and must not call it a calendar day. A partial hour in progress at
 * power loss is discarded rather than promoted to a completed one. */
static valor_history_point_t s_history[VALOR_HISTORY_HOURS];
static uint32_t s_last_hour_s = 0;
static bool     s_last_hour_init = false;
/* Scratch for encode/decode. Static because the monitor task stack is not sized
 * for a 612-byte blob on top of its existing frame, and both uses are
 * serialised by WS_LOCK() or by single-task startup. */
static uint8_t  s_history_blob[VALOR_HISTORY_BLOB_BYTES];
static bool     s_history_dirty = false;
static uint32_t s_accum_count = 0;      /* HR samples                                  */
static uint32_t s_accum_spo2_n = 0;     /* samples with a valid SpO2                    */
static uint32_t s_accum_rmssd_n = 0;    /* samples with a valid RMSSD                   */
static float    s_accum_hr = 0.0f;
static float    s_accum_spo2 = 0.0f;
static float    s_accum_rmssd = 0.0f;
static float    s_accum_pm25 = 0.0f;
static unsigned s_accum_news2 = 0;
static bool     s_history_initialized = false;

static void history_feed_locked(const valor_status_t *s);

/* NVS-backed persistence (T3.2). These are no-ops on the host build; the
 * persistence decision (which slots to save, and how a restored set is shifted)
 * lives in the pure code so it is observable and unit-tested. */
static void history_nvs_load(void);
static void history_nvs_store(const uint8_t *blob, size_t len);
static void history_persist_if_dirty(void);

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
static SemaphoreHandle_t s_lock = NULL;
#define WS_LOCK()   do { if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY); } while (0)
#define WS_UNLOCK() do { if (s_lock) xSemaphoreGive(s_lock); } while (0)
#else
#define WS_LOCK()   do { } while (0)
#define WS_UNLOCK() do { } while (0)
static void history_nvs_load(void) { }
static void history_nvs_store(const uint8_t *blob, size_t len) {
    (void)blob;
    (void)len;
}
#endif

void web_status_publish(const valor_status_t *s) {
    if (!s) return;
    WS_LOCK();
    s_snap = *s;
    history_feed_locked(s);
    WS_UNLOCK();
    /* Do the flash write outside the lock so a slow NVS commit never stalls the
     * HTTP task reading the snapshot. This only does work once an hour, when a
     * rollover has just finalised a slot. */
    history_persist_if_dirty();
}

void web_status_snapshot(valor_status_t *out) {
    if (!out) return;
    WS_LOCK();
    *out = s_snap;
    WS_UNLOCK();
}

/* ------------------------------------------------------------------------- */
/* JSON                                                                       */
/* ------------------------------------------------------------------------- */

static bool json_append(char *buf, size_t cap, size_t *used,
                        const char *text, size_t len) {
    if (*used >= cap || len >= cap - *used) return false;
    memcpy(buf + *used, text, len);
    *used += len;
    buf[*used] = '\0';
    return true;
}

static bool json_append_format(char *buf, size_t cap, size_t *used,
                               const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf + *used, cap - *used, fmt, args);
    va_end(args);
    if (n < 0 || (size_t)n >= cap - *used) {
        buf[cap - 1] = '\0';
        return false;
    }
    *used += (size_t)n;
    return true;
}

static bool json_append_string(char *buf, size_t cap, size_t *used,
                               const char *value) {
    static const char hex[] = "0123456789abcdef";
    if (!json_append(buf, cap, used, "\"", 1)) return false;

    for (const unsigned char *p = (const unsigned char *)value; *p; p++) {
        char escaped[6];
        const char *text = (const char *)p;
        size_t len = 1;
        switch (*p) {
            case '"':  text = "\\\""; len = 2; break;
            case '\\': text = "\\\\"; len = 2; break;
            case '\b': text = "\\b";  len = 2; break;
            case '\f': text = "\\f";  len = 2; break;
            case '\n': text = "\\n";  len = 2; break;
            case '\r': text = "\\r";  len = 2; break;
            case '\t': text = "\\t";  len = 2; break;
            default:
                if (*p < 0x20) {
                    escaped[0] = '\\';
                    escaped[1] = 'u';
                    escaped[2] = '0';
                    escaped[3] = '0';
                    escaped[4] = hex[*p >> 4];
                    escaped[5] = hex[*p & 0x0f];
                    text = escaped;
                    len = sizeof(escaped);
                }
                break;
        }
        if (!json_append(buf, cap, used, text, len)) return false;
    }
    return json_append(buf, cap, used, "\"", 1);
}

bool web_status_request_is_same_origin(const char *origin, const char *referer) {
    static const char expected[] = "http://192.168.4.1";
    const size_t expected_len = sizeof(expected) - 1;

    /* If Origin is present, it is authoritative; do not let a matching
     * Referer override a cross-origin request. */
    if (origin && origin[0] != '\0') return strcmp(origin, expected) == 0;
    if (!referer || strncmp(referer, expected, expected_len) != 0) return false;

    /* Require the host to end here or continue with a path. This rejects
     * lookalike hosts such as 192.168.4.1.attacker.example. */
    return referer[expected_len] == '\0' || referer[expected_len] == '/';
}

/* Pure: a struct in, a string out. No ESP-IDF, no locks, no I/O, so the wire
 * format is proved in the host test suite rather than by curling a live board. */
size_t web_status_json(const valor_status_t *s, char *buf, size_t cap) {
    if (!s || !buf || cap == 0) return 0;

    size_t used = 0;
    buf[0] = '\0';
    if (!json_append_format(
        buf, cap, &used,
        "{\"hr\":%.1f,\"spo2\":%.1f,\"rr\":%.1f,\"sqi\":%.2f,\"rmssd\":%.1f,"
        "\"temp\":",
        (double)s->hr, (double)s->spo2, (double)s->rr, (double)s->sqi,
        (double)s->rmssd)) return 0;

    if (s->temp < -40.0f || s->temp > 85.0f) {
        if (!json_append(buf, cap, &used, "null,", 5)) return 0;
    } else if (!json_append_format(buf, cap, &used, "%.1f,", (double)s->temp)) {
        return 0;
    }

    if (s->hum < 0.0f) {
        if (!json_append(buf, cap, &used, "\"hum\":null,", 11)) return 0;
    } else {
        if (!json_append_format(buf, cap, &used, "\"hum\":%.1f,", (double)s->hum)) return 0;
    }

    if (!json_append_format(
        buf, cap, &used,
        "\"pm25\":%.1f,"
        "\"pressure\":%.1f,\"ptrend\":%.2f,"
        "\"pdrop\":%.1f,\"pspan\":%lu,\"pvalid\":%s,"
        "\"news2\":%u,\"level\":%u,\"flags\":%u,"
        "\"risk\":",
        (double)s->pm25,
        (double)s->pressure_hpa, (double)s->pressure_trend_hpa_per_hr,
        (double)s->pdrop_hpa, (unsigned long)s->pspan_s,
        s->pvalid ? "true" : "false",
        s->news2, s->level, s->flags)) return 0;
    if (!json_append_string(buf, cap, &used, s->risk ? s->risk : "UNKNOWN") ||
        !json_append(buf, cap, &used, ",\"storm\":", sizeof(",\"storm\":") - 1) ||
        !json_append_string(buf, cap, &used, s->storm ? s->storm : "UNKNOWN") ||
        !json_append(buf, cap, &used, ",\"sos\":", sizeof(",\"sos\":") - 1) ||
        !json_append_string(buf, cap, &used, s->sos ? s->sos : "IDLE") ||
        !json_append(buf, cap, &used, ",\"trigger\":", sizeof(",\"trigger\":") - 1) ||
        !json_append_string(buf, cap, &used, s->trigger ? s->trigger : "none") ||
        !json_append(buf, cap, &used, ",\"loc\":", sizeof(",\"loc\":") - 1) ||
        !json_append_string(buf, cap, &used, s->loc ? s->loc : "UNSET") ||
        !json_append(buf, cap, &used, ",\"env\":", sizeof(",\"env\":") - 1) ||
        !json_append_string(buf, cap, &used, s->env ? s->env : "") ||
        !json_append_format(buf, cap, &used, ",\"aqi\":%u,\"aqicat\":", s->aqi) ||
        !json_append_string(buf, cap, &used, s->aqi_cat ? s->aqi_cat : "") ||
        !json_append_format(buf, cap, &used, ",\"heatc\":%.1f,\"heatcat\":", (double)s->heat_c) ||
        !json_append_string(buf, cap, &used, s->heat_cat ? s->heat_cat : "") ||
        !json_append_format(buf, cap, &used, ",\"motion\":%.2f,\"fall\":", (double)s->motion) ||
        !json_append_string(buf, cap, &used, s->fall ? s->fall : "MONITORING") ||
        !json_append_format(buf, cap, &used, ",\"contact\":%s,\"up\":%lu}",
                            s->contact ? "true" : "false", (unsigned long)s->uptime_s)) {
        return 0;
    }
    return used;
}

/* ------------------------------------------------------------------------- */
/* 24-Hour History Implementation                                             */
/* ------------------------------------------------------------------------- */

/* --- canonical on-flash encoding (see web_status.h) ---------------------- */

#define VALOR_HISTORY_BLOB_MAGIC   0x31564C48u /* "HLV1" little-endian */
#define VALOR_HISTORY_BLOB_VERSION 1u
#define VALOR_HISTORY_POINT_BYTES  25u
#define VALOR_HISTORY_HEADER_BYTES 12u

static void hist_put_u32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint32_t hist_get_u32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void hist_put_f32le(uint8_t *p, float v) {
    uint32_t bits;
    memcpy(&bits, &v, sizeof(bits));
    hist_put_u32le(p, bits);
}

static float hist_get_f32le(const uint8_t *p) {
    uint32_t bits = hist_get_u32le(p);
    float v;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

/* FNV-1a, 32-bit. Chosen because it is tiny and dependency-free, not because it
 * is a cryptographic digest: it catches the half-written or bit-rotted page this
 * guard exists for. */
static uint32_t hist_checksum(const uint8_t *p, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h ^= (uint32_t)p[i];
        h *= 16777619u;
    }
    return h;
}

size_t web_status_history_encode(const valor_history_point_t *pts, size_t n,
                                 uint8_t *buf, size_t cap) {
    if (!pts || !buf || n == 0 || n > VALOR_HISTORY_HOURS) return 0;
    size_t need = VALOR_HISTORY_HEADER_BYTES + n * VALOR_HISTORY_POINT_BYTES;
    if (cap < need) return 0;

    hist_put_u32le(buf + 0, VALOR_HISTORY_BLOB_MAGIC);
    buf[4] = (uint8_t)VALOR_HISTORY_BLOB_VERSION;
    buf[5] = 0;
    buf[6] = (uint8_t)(n & 0xFFu);
    buf[7] = (uint8_t)((n >> 8) & 0xFFu);

    size_t off = VALOR_HISTORY_HEADER_BYTES;
    for (size_t i = 0; i < n; i++) {
        hist_put_u32le(buf + off, pts[i].hour_offset); off += 4;
        hist_put_f32le(buf + off, pts[i].avg_hr);      off += 4;
        hist_put_f32le(buf + off, pts[i].avg_spo2);    off += 4;
        hist_put_f32le(buf + off, pts[i].avg_rmssd);   off += 4;
        hist_put_f32le(buf + off, pts[i].peak_pm25);   off += 4;
        hist_put_u32le(buf + off, (uint32_t)pts[i].max_news2); off += 4;
        buf[off++] = pts[i].valid ? 1u : 0u;
    }

    hist_put_u32le(buf + 8, hist_checksum(buf + VALOR_HISTORY_HEADER_BYTES,
                                          n * VALOR_HISTORY_POINT_BYTES));
    return off;
}

size_t web_status_history_decode(const uint8_t *buf, size_t len,
                                 valor_history_point_t *pts, size_t n) {
    if (!buf || !pts || n == 0) return 0;
    if (len < VALOR_HISTORY_HEADER_BYTES) return 0;
    if (hist_get_u32le(buf) != VALOR_HISTORY_BLOB_MAGIC) return 0;
    if (buf[4] != (uint8_t)VALOR_HISTORY_BLOB_VERSION) return 0;

    size_t count = (size_t)buf[6] | ((size_t)buf[7] << 8);
    if (count == 0 || count > VALOR_HISTORY_HOURS) return 0;
    size_t need = VALOR_HISTORY_HEADER_BYTES + count * VALOR_HISTORY_POINT_BYTES;
    if (len < need) return 0;
    if (hist_get_u32le(buf + 8) !=
        hist_checksum(buf + VALOR_HISTORY_HEADER_BYTES,
                      count * VALOR_HISTORY_POINT_BYTES)) {
        return 0;
    }

    size_t out_n = (count < n) ? count : n;
    for (size_t i = 0; i < out_n; i++) {
        size_t off = VALOR_HISTORY_HEADER_BYTES + i * VALOR_HISTORY_POINT_BYTES;
        uint32_t hour = hist_get_u32le(buf + off);
        if (hour >= VALOR_HISTORY_HOURS) return 0;
        uint8_t valid = buf[off + 24];
        if (valid > 1u) return 0;

        pts[i].hour_offset = hour;
        pts[i].avg_hr      = hist_get_f32le(buf + off + 4);
        pts[i].avg_spo2    = hist_get_f32le(buf + off + 8);
        pts[i].avg_rmssd   = hist_get_f32le(buf + off + 12);
        pts[i].peak_pm25   = hist_get_f32le(buf + off + 16);
        pts[i].max_news2   = (unsigned)hist_get_u32le(buf + off + 20);
        pts[i].valid       = (valid != 0u);
    }
    return out_n;
}

/* Encode a finished hour under the lock, then write it to flash with the lock
 * released. If the encode fails the dirty flag stays set and the next publish
 * retries, so a transient failure does not silently drop the data. */
static void history_persist_if_dirty(void) {
    if (!s_history_dirty) return;
    size_t n;
    WS_LOCK();
    n = web_status_history_encode(s_history, VALOR_HISTORY_HOURS,
                                  s_history_blob, sizeof(s_history_blob));
    if (n > 0) s_history_dirty = false;
    WS_UNLOCK();
    if (n > 0) history_nvs_store(s_history_blob, n);
}

static void history_reset_locked(void) {
    for (size_t i = 0; i < VALOR_HISTORY_HOURS; i++) {
        s_history[i].hour_offset = (uint32_t)i;
        s_history[i].avg_hr = 0.0f;
        s_history[i].avg_spo2 = 0.0f;
        s_history[i].avg_rmssd = 0.0f;
        s_history[i].peak_pm25 = 0.0f;
        s_history[i].max_news2 = 0;
        s_history[i].valid = false;
    }
    s_history_initialized = true;
    s_history_dirty = false;
}

void web_status_history_init(void) {
    WS_LOCK();
    history_reset_locked();
    history_nvs_load();
    s_last_hour_s = 0;
    s_last_hour_init = false;
    s_accum_count = 0;
    s_accum_spo2_n = 0;
    s_accum_rmssd_n = 0;
    s_accum_hr = 0.0f;
    s_accum_spo2 = 0.0f;
    s_accum_rmssd = 0.0f;
    s_accum_pm25 = 0.0f;
    s_accum_news2 = 0;
    WS_UNLOCK();
}

static void history_feed_locked(const valor_status_t *s) {
    if (!s_history_initialized) {
        /* The device calls web_status_history_init() explicitly; this lazy path
         * is the safety net for a publish that arrives first, and it restores
         * the saved history exactly the same way. */
        history_reset_locked();
        history_nvs_load();
    }

    if (!s_last_hour_init) {
        s_last_hour_s = s->uptime_s;
        s_last_hour_init = true;
    }

    /* Finalise every hour that has fully elapsed BEFORE this sample is added, so
     * the sample that crosses the boundary counts toward the hour that is
     * starting, not the hour that just ended. */
    while (s->uptime_s >= s_last_hour_s + 3600) {
        for (int i = VALOR_HISTORY_HOURS - 1; i > 0; i--) {
            s_history[i] = s_history[i - 1];
            s_history[i].hour_offset = (uint32_t)i;
        }
        s_history[0].hour_offset = 0;
        if (s_accum_count > 0 || s_accum_pm25 > 0.0f) {
            s_history[0].avg_hr = (s_accum_count > 0)
                                  ? s_accum_hr / (float)s_accum_count : 0.0f;
            s_history[0].avg_spo2 = (s_accum_spo2_n > 0)
                                    ? s_accum_spo2 / (float)s_accum_spo2_n : 0.0f;
            s_history[0].avg_rmssd = (s_accum_rmssd_n > 0)
                                     ? s_accum_rmssd / (float)s_accum_rmssd_n : 0.0f;
            s_history[0].peak_pm25 = s_accum_pm25;
            s_history[0].max_news2 = s_accum_news2;
            s_history[0].valid = true;
        } else {
            s_history[0].avg_hr = 0.0f;
            s_history[0].avg_spo2 = 0.0f;
            s_history[0].avg_rmssd = 0.0f;
            s_history[0].peak_pm25 = 0.0f;
            s_history[0].max_news2 = 0;
            s_history[0].valid = false;
        }
        /* A slot just became a completed hour: persist the whole array once the
         * lock is released (history_persist_if_dirty). Even when the hour is
         * invalid this is written, so a genuine gap is preserved as a gap. */
        s_history_dirty = true;
        s_last_hour_s += 3600;
        s_accum_count = 0;
        s_accum_spo2_n = 0;
        s_accum_rmssd_n = 0;
        s_accum_hr = 0.0f;
        s_accum_spo2 = 0.0f;
        s_accum_rmssd = 0.0f;
        s_accum_pm25 = 0.0f;
        s_accum_news2 = 0;

        /* Prevent infinite loop on massive jumps (e.g. sleep/wake) */
        if (s->uptime_s >= s_last_hour_s + (3600 * VALOR_HISTORY_HOURS)) {
            s_last_hour_s = s->uptime_s;
            break;
        }
    }

    /* Accumulate this sample into the hour that is now current. */
    if (s->contact && s->hr >= 30.0f && s->hr <= 240.0f) {
        s_accum_hr += s->hr;
        s_accum_count++;
        /* Average each clinical field over its own valid samples. SpO2 is 0
         * while the engine is calibrating, and RMSSD is 0 until the SQI/9-pair
         * gate opens; including those zeros would drag the hourly averages down
         * and make a normal hour look hypoxic or low-HRV. */
        if (s->spo2 > 0.0f) {
            s_accum_spo2 += s->spo2;
            s_accum_spo2_n++;
        }
        if (s->rmssd > 0.0f) {
            s_accum_rmssd += s->rmssd;
            s_accum_rmssd_n++;
        }
        if (s->news2 > s_accum_news2) {
            s_accum_news2 = s->news2;
        }
    }
    if (s->pm25 > s_accum_pm25) {
        s_accum_pm25 = s->pm25;
    }

    /* Update slot 0 live with running averages for the current hour */
    s_history[0].hour_offset = 0;
    if (s_accum_count > 0 || s_accum_pm25 > 0.0f) {
        s_history[0].avg_hr = (s_accum_count > 0)
                              ? s_accum_hr / (float)s_accum_count : 0.0f;
        s_history[0].avg_spo2 = (s_accum_spo2_n > 0)
                                ? s_accum_spo2 / (float)s_accum_spo2_n : 0.0f;
        s_history[0].avg_rmssd = (s_accum_rmssd_n > 0)
                                 ? s_accum_rmssd / (float)s_accum_rmssd_n : 0.0f;
        s_history[0].peak_pm25 = s_accum_pm25;
        s_history[0].max_news2 = s_accum_news2;
        s_history[0].valid = true;
    } else {
        /* No clinical contact and no environmental sample this hour. Clearing
         * this is important: the old code left the previous hour's valid flag
         * and values in slot 0, so the chart showed a stale current point. */
        s_history[0].avg_hr = 0.0f;
        s_history[0].avg_spo2 = 0.0f;
        s_history[0].avg_rmssd = 0.0f;
        s_history[0].peak_pm25 = 0.0f;
        s_history[0].max_news2 = 0;
        s_history[0].valid = false;
    }
}

void web_status_history_snapshot(valor_history_point_t *out, size_t max_points) {
    if (!out || max_points == 0) return;
    WS_LOCK();
    size_t n = (max_points < VALOR_HISTORY_HOURS) ? max_points : VALOR_HISTORY_HOURS;
    for (size_t i = 0; i < n; i++) {
        out[i] = s_history[i];
    }
    WS_UNLOCK();
}

size_t web_status_history_json(char *buf, size_t cap) {
    if (!buf || cap < 4) return 0;
    size_t used = 0;

    WS_LOCK();
    if (!json_append(buf, cap, &used, "[", 1)) { WS_UNLOCK(); return 0; }

    for (size_t i = 0; i < VALOR_HISTORY_HOURS; i++) {
        const valor_history_point_t *p = &s_history[i];
        if (i > 0) {
            if (!json_append(buf, cap, &used, ",", 1)) { WS_UNLOCK(); return 0; }
        }
        if (!json_append_format(buf, cap, &used,
                                "{\"hour\":%u,\"hr\":%.1f,\"spo2\":%.1f,\"rmssd\":%.1f,"
                                "\"pm25\":%.1f,\"news2\":%u,\"valid\":%s}",
                                (unsigned)p->hour_offset,
                                p->avg_hr, p->avg_spo2, p->avg_rmssd,
                                p->peak_pm25, p->max_news2,
                                p->valid ? "true" : "false")) {
            WS_UNLOCK();
            return 0;
        }
    }

    if (!json_append(buf, cap, &used, "]", 1)) { WS_UNLOCK(); return 0; }
    WS_UNLOCK();
    return used;
}

#ifdef ESP_PLATFORM

/* ------------------------------------------------------------------------- */
/* The page                                                                   */
/* ------------------------------------------------------------------------- */

/* Deliberately attribute-quoted with single quotes so the whole document drops
 * into a C string with no escapes, and units live in their own elements so the
 * script only ever sets textContent - no nested quoting to get wrong.
 *
 * It polls /status.json once a second and renders. It derives nothing: NEWS2,
 * the risk level and the hazard verdicts all arrive already computed, which is
 * the same rule the desktop dashboard follows. */
static const char PAGE_HTML[] =
"<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; script-src 'sha256-2uU/83cXyZm/fR5I19lxos2zxsaJv/Qq7/dXeNK+Am0='; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; base-uri 'none'; object-src 'none'\">"
"<title>VALOR - local status</title><style>"
":root{--bg:#070B14;--card:#0E1626;--line:#1C2740;--text:#E6EDF7;--dim:#8A9AB5;"
"--cyan:#00E5FF;--emerald:#00FFAA;--amber:#FFB020;--crimson:#FF3B5C}"
"*{box-sizing:border-box}"
"body{margin:0;padding:14px;background:var(--bg);color:var(--text);"
"font:15px/1.45 system-ui,-apple-system,'Segoe UI',Roboto,sans-serif}"
"h1{font-size:17px;letter-spacing:.15em;margin:0;font-weight:700}"
".sub{color:var(--dim);font-size:12px;margin:2px 0 12px}"
"#banner{padding:12px 14px;border-radius:12px;border:1px solid var(--line);"
"background:var(--card);margin-bottom:12px}"
"#banner .lvl{font-size:20px;font-weight:700;letter-spacing:.05em}"
"#banner .msg{font-size:12.5px;color:var(--dim);margin-top:3px}"
".grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
".card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:11px 12px}"
".card .k{font-size:10.5px;letter-spacing:.13em;color:var(--dim);text-transform:uppercase}"
".card .v{font-size:27px;font-weight:650;margin-top:2px;font-variant-numeric:tabular-nums}"
".card .u{font-size:12px;color:var(--dim);font-weight:400;margin-left:3px}"
".wide{grid-column:1/-1}"
".env{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:10px}"
".ec{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}"
".ec-tile{border:1px solid var(--line);border-radius:8px;padding:8px}"
".ec-k{font-size:10.5px;letter-spacing:.06em;text-transform:uppercase;color:var(--dim)}"
".ec-v{font-size:22px;font-weight:700;margin-top:2px}"
".ec-v .u{font-size:11px;font-weight:400;color:var(--dim)}"
".ec-badge{display:inline-block;margin-top:5px;padding:2px 8px;border:1px solid;border-radius:999px;font-size:10.5px;font-weight:700;line-height:1.35}"
".ec-note{font-size:11.5px;margin-top:5px;line-height:1.35}"
".ec-sub{font-size:10.5px;color:var(--dim);margin-top:4px}"
".env .v{font-size:18px}"
".foot{margin-top:12px;color:var(--dim);font-size:11.5px;display:flex;"
"justify-content:space-between;gap:8px;flex-wrap:wrap}"
".dot{display:inline-block;width:7px;height:7px;border-radius:50%;"
"background:var(--emerald);margin-right:6px;vertical-align:middle}"
".sos #banner{background:#2A0A12;border-color:var(--crimson)}"
".sos h1{color:var(--crimson)}"
".sos .dot{background:var(--crimson)}"
".armed #banner{background:#261A05;border-color:var(--amber)}"
".armed h1{color:var(--amber)}"
".armed .dot{background:var(--amber)}"
"form.setloc{display:flex;gap:8px;margin-top:7px}"
"form.setloc input{flex:1;min-width:0;background:#0A1120;border:1px solid var(--line);"
"color:var(--text);border-radius:8px;padding:9px 10px;font-size:14px}"
"form.setloc button{background:var(--cyan);color:#04121A;border:0;border-radius:8px;"
"padding:9px 13px;font-weight:700;font-size:13px}"
"#locnote{font-size:11.5px;color:var(--dim);margin-top:6px}"
"</style></head><body>"
"<h1>VALOR</h1>"
"<div class='sub'>Vital and Atmospheric Logic for Offline Rescue</div>"
"<div id='banner'><div class='lvl' id='risk'>waiting</div>"
"<div class='msg' id='msg'>connecting to the device</div></div>"
"<div class='grid'>"
"<div class='card'><div class='k'>Heart rate</div>"
"<div class='v'><span id='hr'>--</span><span class='u'>bpm</span></div></div>"
"<div class='card'><div class='k'>SpO2</div>"
"<div class='v'><span id='spo2'>--</span><span class='u'>%</span></div></div>"
"<div class='card'><div class='k'>HRV RMSSD</div>"
"<div class='v'><span id='rmssd'>--</span><span class='u'>ms</span></div></div>"
"<div class='card'><div class='k'>Respiratory</div>"
"<div class='v'><span id='rr'>--</span><span class='u'>/min</span></div></div>"
"<div class='card wide'><div class='k'>Signal quality</div>"
"<div class='v'><span id='sqi'>--</span><span class='u'>%</span></div></div>"
"<div class='card wide'><label class='k' for='locin'>Deployment location</label>"
"<form class='setloc' method='post' action='/location'>"
"<input id='locin' name='loc' maxlength='31' placeholder='Village / Block / District'>"
"<button type='submit'>Save</button></form>"
"<div id='locnote'>Stored on the device and shown on the emergency card.</div></div>"
"<div class='card wide'><div class='k'>Emergency Assist Control</div>"
"<div style='display:flex;gap:8px;margin-top:7px'>"
"<button id='sos-trigger' type='button' style='flex:1;background:var(--crimson);color:#fff;border:0;border-radius:8px;padding:9px;font-weight:700;cursor:pointer'>TRIGGER SOS</button>"
"<button id='sos-cancel' type='button' style='flex:1;background:#1E293B;color:var(--text);border:1px solid var(--line);border-radius:8px;padding:9px;font-weight:700;cursor:pointer'>CANCEL SOS</button>"
"</div><div id='sos-note' role='status' aria-live='polite' style='font-size:11.5px;color:var(--dim);margin-top:6px'>Manual emergency actions require a working device connection.</div></div>"
"</div>"
"<div class='env'>"
"<div class='card'><div class='k'>Ambient temp</div><div class='v'><span id='temp'>--</span><span class='u'>&deg;C</span></div></div>"

"<div class='card'><div class='k'>Humidity</div><div class='v'><span id='hum'>--</span><span class='u'>% RH</span></div></div>"
"<div class='card'><div class='k'>PM2.5</div><div class='v'><span id='pm'>--</span><span class='u'>&micro;g/m&sup3;</span></div></div>"
"<div class='card wide'><div class='k'>Storm watch &middot; barometric trend</div>"
"<div class='v' style='font-size:19px'><span id='storm'>--</span></div>"
"<div id='pnote'>&nbsp;</div></div>"
"<div class='card wide'><div class='k'>Environmental condition &middot; EPA AQI 2024 / NWS heat index</div>"
"<div class='ec'>"
"<div class='ec-tile'><div class='ec-k'>PM2.5</div><div class='ec-v'><span id='pmv'>--</span><span class='u'> &micro;g/m&sup3;</span></div>"
"<div class='ec-badge' id='aqibadge'>--</div><div class='ec-note' id='aqinote'>--</div><div class='ec-sub' id='whonote'>--</div></div>"
"<div class='ec-tile'><div class='ec-k'>Heat index</div><div class='ec-v'><span id='heatv'>--</span><span class='u'> &deg;C</span></div>"
"<div class='ec-badge' id='heatbadge'>--</div><div class='ec-note' id='heatnote'>--</div><div class='ec-sub'>NWS heat-index band</div></div>"
"</div></div>"
"</div>"
"<div class='foot'>"
"<span><span class='dot'></span><span id='link'>live</span></span>"
"<span id='loc'>LOC --</span><span id='news2'>NEWS2 --</span>"
"</div>"
"<script>"
"var D='--';var locFilled=false;"
"function t(i,v){document.getElementById(i).textContent=v}"
"function num(v,d){return typeof v==='number'&&isFinite(v)?v.toFixed(d):D}"
"function positive(v,d){return typeof v==='number'&&isFinite(v)&&v>0?v.toFixed(d):D}"
"var aqiNote={Good:'Air quality is satisfactory.',Moderate:'Acceptable; unusually sensitive people may be affected.','Unhealthy for Sensitive Groups':'Sensitive groups: reduce prolonged outdoor exertion.',Unhealthy:'Everyone: reduce outdoor exertion; sensitive groups avoid it.','Very Unhealthy':'Health alert: avoid outdoor exertion.',Hazardous:'Emergency: stay indoors.'};"
"var aqiCol={Good:'#4ADE80',Moderate:'#FBBF24','Unhealthy for Sensitive Groups':'#FB923C',Unhealthy:'#F87171','Very Unhealthy':'#C084FC',Hazardous:'#E11D48'};"
"var heatNote={Normal:'No heat stress.',Caution:'Fatigue possible with prolonged exposure.','Extreme Caution':'Heat cramps / exhaustion possible.',Danger:'Cramps or exhaustion likely; heat stroke possible.','Extreme Danger':'Heat stroke imminent - avoid exposure.'};"
"var heatCol={Normal:'#4ADE80',Caution:'#FBBF24','Extreme Caution':'#FB923C',Danger:'#F87171','Extreme Danger':'#E11D48'};"
"function setBadge(i,v,c){var e=document.getElementById(i);if(!e)return;e.textContent=v;e.style.color=c||'';e.style.borderColor=c||''}"
"function clearReadings(){document.body.className='';t('risk','NO CONNECTION');"
"t('msg','Live status unavailable; current readings cleared.');"
"['hr','spo2','rmssd','rr','sqi','temp','hum','pm','storm','pmv','heatv'].forEach(function(i){t(i,D)});"
"t('aqinote',D);t('whonote',D);t('heatnote',D);"
"setBadge('aqibadge',D,'');setBadge('heatbadge',D,'');"
"document.getElementById('storm').style.color='';"
"t('news2','NEWS2 --');t('pnote','');t('link','reconnecting');"
"t('sos-note','Device connection lost; check the WiFi link')}"
"function poll(){"
"fetch('/status.json',{cache:'no-store'}).then(function(r){if(!r.ok)throw new Error('HTTP '+r.status);return r.json()})"
".then(function(s){"
"document.body.className=s.sos==='ACTIVE'?'sos':(s.sos==='ARMED'?'armed':'');"
"t('risk',s.sos==='ACTIVE'?'MEDICAL EMERGENCY':(s.sos==='ARMED'?'FALL DETECTED':s.risk));"
"t('msg',s.sos==='ACTIVE'?'SOS active ('+s.trigger+') - call emergency services':"
"(s.sos==='ARMED'?'WARNING: Fall detected! SOS countdown active ('+s.trigger+'). Cancel if safe.':"
"'Triage verdict computed on the device. Updates every second.'));"
"t('hr',positive(s.hr,0));t('spo2',positive(s.spo2,1));t('rmssd',positive(s.rmssd,1));"
"t('rr',positive(s.rr,0));t('sqi',s.contact&&typeof s.sqi==='number'?(s.sqi*100).toFixed(0):D);"
"t('temp',num(s.temp,1));t('hum',num(s.hum,0));t('pm',num(s.pm25,0));"
"var storm=s.storm||'UNKNOWN';"
"if(!s.pvalid){t('storm','WARMING UP')}else{t('storm',storm)}"
"var sc={NORMAL:'#4ADE80',MODERATE:'#FBBF24',HIGH:'#FB923C',CRITICAL:'#F87171',UNKNOWN:'#94A3B8'};"
"document.getElementById('storm').style.color=sc[storm]||'';"
"t('pmv',num(s.pm25,1));"
"if(typeof s.aqi==='number'&&s.aqicat){setBadge('aqibadge',s.aqicat+' | AQI '+s.aqi,aqiCol[s.aqicat]||'#94A3B8');"
"t('aqinote',aqiNote[s.aqicat]||'');"
"t('whonote','WHO 2021 24-h 15 ug/m3'+((typeof s.pm25==='number'&&s.pm25>15)?' - '+((s.pm25/15).toFixed(1))+'x guideline':' - within guideline'))}"
"else{setBadge('aqibadge',D,'');t('aqinote',D);t('whonote',D)}"
"if(typeof s.heatc==='number'&&s.heatcat){t('heatv',s.heatc.toFixed(1));setBadge('heatbadge',s.heatcat,heatCol[s.heatcat]||'#94A3B8');t('heatnote',heatNote[s.heatcat]||'')}"
"else{t('heatv',D);setBadge('heatbadge',D,'');t('heatnote',D)}"
/* The trend is the number that carries the signal; the absolute pressure is
   shown beside it only so a reader can see the barometer they are reading. */
"var spanMin=Math.round((typeof s.pspan==='number'?s.pspan:0)/60);"
"if(!s.pvalid){t('pnote','Collecting pressure trend: '+Math.min(spanMin,30)+'/30 min. Indoor barometer - a trend is a proxy, not an official cyclone warning.')}"
"else{var sh=Math.floor(spanMin/60),sm=spanMin%60;var spantxt=sh>0?(sh+'h '+sm+'m'):(sm+' min');"
"var pd=(typeof s.pdrop==='number')?s.pdrop:0;var pc=(pd>0?'-':'+')+Math.abs(pd).toFixed(1)+' hPa';"
"t('pnote','Pressure '+(s.pressure>0?s.pressure.toFixed(1):D)+' hPa; '+pc+' over '+spantxt+'; trend '+((s.ptrend<0?'':'+')+s.ptrend.toFixed(2))+' hPa/hr. Indoor barometer - proxy only, not an official warning.')}"
"t('news2','NEWS2 '+s.news2);t('loc','LOC '+s.loc);"
"t('link',s.contact?'finger detected':'no finger');"
/* Seed the location box once. Doing it on every poll would overwrite whatever
   the caregiver is halfway through typing. */
"if(!locFilled){var el=document.getElementById('locin');"
"if(el&&s.loc!=='UNSET'){el.value=s.loc}locFilled=true}"
"}).catch(clearReadings);"
"}"
"function postEmergency(path,action,holdMs){t('sos-note','Sending '+action+' request...');"
"var o={method:'POST'};if(holdMs){o.headers={'Content-Type':'application/x-www-form-urlencoded'};o.body='hold_ms='+holdMs;}"
"fetch(path,o).then(function(r){if(!r.ok)throw new Error('HTTP '+r.status);return r.json()})"
".then(function(){t('sos-note',action+' request sent');poll()})"
".catch(function(){t('sos-note',action+' request failed; check the connection')})}"
"document.getElementById('sos-trigger').addEventListener('click',function(){postEmergency('/api/emergency/trigger','SOS trigger')});"
"(function(){var b=document.getElementById('sos-cancel'),tm=null,ok=false;"
"function start(e){if(ok)return;if(e&&e.preventDefault)e.preventDefault();t('sos-note','Hold for 3 s to cancel...');"
"tm=setTimeout(function(){ok=true;postEmergency('/api/emergency/cancel','SOS cancel',3000)},3000)}"
"function stop(){if(tm){clearTimeout(tm);tm=null}if(!ok)t('sos-note','Cancel aborted - hold for 3 s')}"
"b.addEventListener('pointerdown',start);b.addEventListener('pointerup',stop);"
"b.addEventListener('pointerleave',stop);b.addEventListener('pointercancel',stop);"
"b.addEventListener('keydown',function(e){if(e.key===' '||e.key==='Enter')start(e)});"
"b.addEventListener('keyup',stop);})();"
"poll();setInterval(poll,1000);"
"</script></body></html>";

/* ------------------------------------------------------------------------- */
/* SoftAP + HTTP server                                                       */
/* ------------------------------------------------------------------------- */

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "WEB_STATUS";

/* ------------------------------------------------------------------------- */
/* History persistence (T3.2)                                                 */
/* ------------------------------------------------------------------------- */

#define VALOR_HISTORY_NVS_NAMESPACE "valor_hist"
#define VALOR_HISTORY_NVS_KEY       "hist"

/* Restore the last saved set of completed hours and shift it one slot older so
 * slot 0 is free for the hour that starts now. A partial hour that was in
 * progress when power was lost is therefore dropped rather than promoted to a
 * completed one. Called with WS_LOCK() held, from web_status_history_init(). */
static void history_nvs_load(void) {
    nvs_handle_t h;
    if (nvs_open(VALOR_HISTORY_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "No stored history; the chart starts empty");
        return;
    }
    size_t len = sizeof(s_history_blob);
    esp_err_t err = nvs_get_blob(h, VALOR_HISTORY_NVS_KEY, s_history_blob, &len);
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No stored history (%s); the chart starts empty", esp_err_to_name(err));
        return;
    }

    static valor_history_point_t stored[VALOR_HISTORY_HOURS];
    size_t n = web_status_history_decode(s_history_blob, len, stored, VALOR_HISTORY_HOURS);
    if (n == 0) {
        /* Bad magic/version/checksum or a short blob. Keeping the empty buffer is
         * better than graphing half-decoded values as if they were measured. */
        ESP_LOGW(TAG, "Stored history is unreadable; ignoring it");
        return;
    }

    for (int i = VALOR_HISTORY_HOURS - 1; i > 0; i--) {
        s_history[i] = stored[i - 1];
        s_history[i].hour_offset = (uint32_t)i;
    }
    /* Slot 0 is this power-on's in-progress hour; it is already cleared. */
    ESP_LOGI(TAG, "Restored %zu completed hourly summaries from NVS", n);
}

/* One write per completed hour. The blob is encoded under the lock by
 * history_persist_if_dirty() and written here with the lock released. */
static void history_nvs_store(const uint8_t *blob, size_t len) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(VALOR_HISTORY_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed (%s); history kept in RAM only", esp_err_to_name(err));
        return;
    }
    err = nvs_set_blob(h, VALOR_HISTORY_NVS_KEY, blob, len);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "history write failed (%s); history kept in RAM only", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Persisted %zu-byte hourly-history blob", len);
    }
}

static char s_ssid[33];
static bool s_ap_active = false;
static httpd_handle_t s_server = NULL;

const char *web_status_ap_ssid(void) { return s_ssid; }
bool web_status_ap_active(void) { return s_ap_active; }

static esp_err_t h_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    /* No cache: a stale page after an emergency clears would be worse than a
     * re-fetch every second on a network with one client. */
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Content-Security-Policy",
        "default-src 'none'; script-src 'sha256-2uU/83cXyZm/fR5I19lxos2zxsaJv/Qq7/dXeNK+Am0='; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; base-uri 'none'; object-src 'none'; frame-ancestors 'none'");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "X-Frame-Options", "DENY");
    httpd_resp_set_hdr(req, "Referrer-Policy", "same-origin");
    return httpd_resp_send(req, PAGE_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t h_json(httpd_req_t *req) {
    valor_status_t s;
    web_status_snapshot(&s);

    /* 512 bytes was enough before the environmental-condition sentence (env,
     * ~170 bytes) and the AQI/heat fields were added. The JSON now overflows
     * it, web_status_json() returns 0, and the handler answered HTTP 500,
     * which the page reports as "NO CONNECTION" even though the Wi-Fi is up. */
    char buf[1024];
    size_t n = web_status_json(&s, buf, sizeof(buf));
    if (n == 0) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "status too large");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, buf, (ssize_t)n);
}

static esp_err_t h_history(httpd_req_t *req) {
    static char buf[4096];
    size_t n = web_status_history_json(buf, sizeof(buf));
    if (n == 0) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "history json format failed");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, buf, (ssize_t)n);
}

static bool location_request_is_same_origin(httpd_req_t *req) {
    char origin[48] = {0};
    char referer[96] = {0};
    const bool has_origin = httpd_req_get_hdr_value_str(req, "Origin", origin, sizeof(origin)) == ESP_OK;
    const bool has_referer = httpd_req_get_hdr_value_str(req, "Referer", referer, sizeof(referer)) == ESP_OK;
    return web_status_request_is_same_origin(has_origin ? origin : NULL,
                                             has_referer ? referer : NULL);
}

/* POST /api/emergency/trigger and POST /api/emergency/cancel */
static esp_err_t h_sos_trigger(httpd_req_t *req) {
    if (!location_request_is_same_origin(req)) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "same-origin request required");
    }
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    /* The SOS state has its own lock: the monitor task also mutates it, and the
     * status-page lock does not cover that. */
    sos_lock();
    sos_manual_trigger(now_ms);
    const char *sos_name  = sos_state_name(sos_get_state());
    const char *trig_name = sos_trigger_name(sos_get_trigger());
    sos_unlock();
    WS_LOCK();
    s_snap.sos = sos_name;
    s_snap.trigger = trig_name;
    WS_UNLOCK();
    const char *resp = "{\"status\":\"ok\",\"sos\":\"ACTIVE\"}\n";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t h_sos_cancel(httpd_req_t *req) {
    if (!location_request_is_same_origin(req)) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "same-origin request required");
    }
    /* A single forged request must not be able to silence an emergency. The page
     * sends hold_ms=3000 only after a 3-second held press; anything shorter (or a
     * bare POST) is refused. This is not authentication - a determined client on
     * the open AP can still hold the button - but it stops accidental taps and
     * trivial drive-by cancels. */
    char body[32];
    int len = req->content_len;
    if (len <= 0 || len >= (int)sizeof(body)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "hold_ms required");
    }
    int got = httpd_req_recv(req, body, (size_t)len);
    if (got != len) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "incomplete body");
    }
    body[len] = '\0';
    char *eq = strchr(body, '=');
    int hold_ms = atoi(eq ? eq + 1 : body);
    if (hold_ms < 3000) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "hold for 3 s to cancel");
    }
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    sos_lock();
    sos_cancel_now(now_ms);
    const char *sos_name  = sos_state_name(sos_get_state());
    const char *trig_name = sos_trigger_name(sos_get_trigger());
    sos_unlock();
    WS_LOCK();
    s_snap.sos = sos_name;
    s_snap.trigger = trig_name;
    WS_UNLOCK();
    const char *resp = "{\"status\":\"ok\",\"sos\":\"CANCELLED\"}\n";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

/* POST /location — the caregiver records where this device is deployed.
 *
 * A phone on the access point is the only input device this system has. There
 * is no keypad, and a serial console needs a laptop and a cable, which is the
 * exact dependency the local page exists to remove. */
static esp_err_t h_set_location(httpd_req_t *req) {
    /* The AP stays open for emergency access. Requiring the page's same-origin
     * Origin header blocks cross-site browser form submissions, but is not
     * authentication: a client deliberately connected to the AP can still post. */
    if (!location_request_is_same_origin(req)) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "same-origin request required");
    }

    char body[160];
    int total = req->content_len;

    /* Reading the whole body is required, not optional: httpd will otherwise
     * leave the remainder in the socket and the next request on that connection
     * parses garbage. */
    if (total <= 0 || total >= (int)sizeof(body)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body missing or too long");
    }
    int got = 0;
    while (got < total) {
        int r = httpd_req_recv(req, body + got, total - got);
        if (r <= 0) return ESP_FAIL;
        got += r;
    }
    body[got] = '\0';

    char clean[LOCATION_MAX_LEN];
    if (!location_parse_form(body, clean, sizeof(clean)) || !location_set(clean)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                   "location must contain printable text");
    }

    /* 303 so the browser re-fetches / with a GET. Answering the POST directly
     * would leave a refresh re-submitting the form. */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t h_404(httpd_req_t *req, httpd_err_code_t err) {    (void)err;
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "not found\n", HTTPD_RESP_USE_STRLEN);
}

esp_err_t web_status_start(void) {
    esp_err_t err;

    /* Every failure below is returned, never ESP_ERROR_CHECK'd. The health
     * monitor is the product; the status page is a convenience. A radio that
     * will not come up must leave the device measuring. */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed (%s); status page disabled", esp_err_to_name(err));
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed (%s); status page disabled", esp_err_to_name(err));
        return err;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop failed (%s); status page disabled", esp_err_to_name(err));
        return err;
    }
    if (esp_netif_create_default_wifi_ap() == NULL) {
        ESP_LOGE(TAG, "AP netif creation failed; status page disabled");
        return ESP_FAIL;
    }

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wcfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed (%s); status page disabled", esp_err_to_name(err));
        return err;
    }

#ifdef ESP_PLATFORM
    if (s_lock == NULL) s_lock = xSemaphoreCreateMutex();
#endif

    /* SSID carries the last two MAC bytes so two units in one room are
     * distinguishable on a phone's network list. */
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(s_ssid, sizeof(s_ssid), "VALOR-%02X%02X", mac[4], mac[5]);

    wifi_config_t ap = {0};
    memcpy(ap.ap.ssid, s_ssid, strlen(s_ssid));
    ap.ap.ssid_len       = (uint8_t)strlen(s_ssid);
    ap.ap.channel        = 6;
    ap.ap.max_connection = 4;
    /* Open by design so a bystander needs no credential in an emergency.
     * This exposes live status to anyone in WiFi range; cloud publishing is
     * independently disabled by default. */
    ap.ap.authmode       = WIFI_AUTH_OPEN;

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) { ESP_LOGE(TAG, "set_mode failed (%s)", esp_err_to_name(err)); return err; }
    err = esp_wifi_set_config(WIFI_IF_AP, &ap);
    if (err != ESP_OK) { ESP_LOGE(TAG, "set_config failed (%s)", esp_err_to_name(err)); return err; }
    err = esp_wifi_start();
    if (err != ESP_OK) { ESP_LOGE(TAG, "wifi_start failed (%s)", esp_err_to_name(err)); return err; }

    httpd_config_t hcfg = HTTPD_DEFAULT_CONFIG();
    hcfg.lru_purge_enable = true;
    hcfg.max_uri_handlers = 12;
    hcfg.core_id = 1;
    err = httpd_start(&s_server, &hcfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed (%s); AP is up but has no page", esp_err_to_name(err));
        s_ap_active = true;
        return err;
    }

    httpd_uri_t uri_root     = { .uri = "/",                      .method = HTTP_GET,  .handler = h_root };
    httpd_uri_t uri_json     = { .uri = "/status.json",           .method = HTTP_GET,  .handler = h_json };
    httpd_uri_t uri_loc      = { .uri = "/location",              .method = HTTP_POST, .handler = h_set_location };
    httpd_uri_t uri_hist     = { .uri = "/history.json",          .method = HTTP_GET,  .handler = h_history };
    httpd_uri_t uri_api      = { .uri = "/api/history",           .method = HTTP_GET,  .handler = h_history };
    httpd_uri_t uri_sos_trig = { .uri = "/api/emergency/trigger", .method = HTTP_POST, .handler = h_sos_trigger };
    httpd_uri_t uri_sos_canc = { .uri = "/api/emergency/cancel",  .method = HTTP_POST, .handler = h_sos_cancel };
    httpd_register_uri_handler(s_server, &uri_root);
    httpd_register_uri_handler(s_server, &uri_json);
    httpd_register_uri_handler(s_server, &uri_loc);
    httpd_register_uri_handler(s_server, &uri_hist);
    httpd_register_uri_handler(s_server, &uri_api);
    httpd_register_uri_handler(s_server, &uri_sos_trig);
    httpd_register_uri_handler(s_server, &uri_sos_canc);
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, h_404);

    s_ap_active = true;
    ESP_LOGI(TAG, "Status page up: join WiFi '%s' (open), then open http://192.168.4.1", s_ssid);
    return ESP_OK;
}

#endif /* ESP_PLATFORM */
