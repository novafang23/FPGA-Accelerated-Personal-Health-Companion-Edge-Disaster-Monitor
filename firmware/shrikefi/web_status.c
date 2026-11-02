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
#include "skin_calib.h"
#include <math.h>
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_random.h"
#endif

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Snapshot                                                                   */
/* ------------------------------------------------------------------------- */

static valor_status_t s_snap;

/* Per-session core-offset calibration state.
 *
 * Lives here rather than in main_shrikefi.c so the HTTP handler that SETS it
 * and the JSON serializer that REPORTS it share one copy. A fingertip probe
 * cannot give core temperature without a reference measurement; this holds the
 * offset derived from one oral reading and expires it when it stops being
 * trustworthy (see skin_calib.c). */
static skin_calib_t s_temp_calib;
static bool         s_temp_calib_inited = false;

/* -------------------------------------------------------------------------
 * Control PIN — authorisation for state-changing endpoints.
 *
 * The access point is OPEN by design: a rescuer must be able to join it with no
 * password, no app and no laptop, which is the whole point of the local page.
 * But open means anyone within radio range can also POST, and the endpoints
 * that do not merely display data are:
 *
 *     POST /api/emergency/cancel   silences an emergency
 *     POST /location               rewrites the deployment location
 *
 * Without a credential, a bystander (or a malicious neighbour) can cancel an
 * SOS. The same-origin check and the 3-second hold stop accidental taps, but
 * neither is authentication - the code said so and it was right.
 *
 * The fix keeps zero-install: a 4-digit PIN is generated at boot and shown on
 * the OLED and in the serial log, so the person physically holding the device
 * can read it and type it. It is not strong crypto against a determined
 * attacker who can see the screen, but it removes the "anyone on the AP" hole
 * and costs the user four keystrokes.
 *
 * Read-only endpoints (/status.json, /desktop, /ppg.json) stay open so the
 * dashboard still loads with no friction.
 * ------------------------------------------------------------------------- */
static char s_control_pin[5] = {0};

static void control_pin_init(void)
{
    if (s_control_pin[0] != '\0') return;
#ifdef ESP_PLATFORM
    /* esp_random() is seeded from the RF subsystem; at boot it is good enough
     * for a local-only 4-digit code and needs no entropy source of our own. */
    uint32_t r = esp_random() % 10000u;
    snprintf(s_control_pin, sizeof(s_control_pin), "%04u", (unsigned)r);
    /* Logged in web_status_start(), where TAG and the log macros are in scope. */
#else
    /* Host build: no radio entropy, and the value is only exercised by tests.
     * Keep it fixed so the tests are deterministic. */
    snprintf(s_control_pin, sizeof(s_control_pin), "%04u", (unsigned)1234u);
#endif
}

const char *web_status_control_pin(void)
{
    control_pin_init();
    return s_control_pin;
}

/* Accept a PIN from a query string, a form body, or a JSON-ish body.
 * Deliberately permissive about WHERE it appears: the page and any client can
 * send ?pin=, pin=, or "pin": "1234". It is NOT permissive about the value.
 *
 * Public rather than static so the parsing rules are unit-testable on the host,
 * where httpd_req_t does not exist. */
bool web_status_control_pin_matches(const char *haystack)
{
    control_pin_init();
    if (!haystack) return false;
    const char *p = strstr(haystack, "pin");
    if (!p) return false;
    /* Advance to the digits that follow 'pin', skipping =, :, quotes, spaces. */
    p += 3;
    while (*p && (*p == '=' || *p == ':' || *p == '"' || *p == '\'' || *p == ' ' ||
                  *p == '\t' || *p == '&')) {
        if (*p == '&') return false;   /* 'pin' present but no value attached */
        p++;
    }
    if (strlen(p) < 4) return false;
    return strncmp(p, s_control_pin, 4) == 0;
}



/* Spinlock protecting the decimated PPG ring. Declared here because the
 * producer (PPG task) and the consumer (HTTP task) are on different cores. */
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
static portMUX_TYPE s_ppg_mux = portMUX_INITIALIZER_UNLOCKED;
#endif

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
static uint16_t s_accum_pm25_n = 0;   /* how many real PM2.5 readings this hour */
static unsigned s_accum_news2 = 0;
static bool     s_history_initialized = false;

static void history_feed_locked(const valor_status_t *s);

/* NVS-backed persistence (T3.2). These are no-ops on the host build; the
 * persistence decision (which slots to save, and how a restored set is shifted)
 * lives in the pure code so it is observable and unit-tested. */
static void history_nvs_load(void);
static void history_nvs_store(const uint8_t *blob, size_t len);
static void history_persist_if_dirty(void);

/* Monotonic milliseconds for the calibration expiry check.
 *
 * Must exist on host too: web_status.c is compiled into the host test suite,
 * where esp_log_timestamp() is unavailable. Both branches return a monotonic
 * millisecond count, which is all skin_calib needs (it only ever subtracts). */
static uint32_t calib_now_ms(void)
{
#ifdef ESP_PLATFORM
    return (uint32_t)esp_log_timestamp();
#else
    static uint32_t host_ms = 0;
    host_ms += 1000u;   /* one simulated second per call: enough for tests */
    return host_ms;
#endif
}

bool web_status_temp_calib_set(float skin_c, float ref_c, const char *site)
{
    if (!s_temp_calib_inited) {
        skin_calib_init(&s_temp_calib);
        s_temp_calib_inited = true;
    }
    const uint32_t now_ms = calib_now_ms();
    /* +1 so a calibration taken at the same millisecond as the query is not
     * immediately rejected as "stale" by a zero/negative elapsed check. */
    return skin_calib_set(&s_temp_calib, skin_c, ref_c, site, now_ms + 1u);
}

bool web_status_temp_core(const valor_status_t *s, float *out_core_c,
                          const char **out_status, float *out_ref_c,
                          const char **out_site)
{
    if (out_core_c) *out_core_c = 0.0f;
    if (out_ref_c)  *out_ref_c = 0.0f;
    if (out_status) *out_status = "uncalibrated";
    if (out_site)   *out_site = "none";

    if (!s || !s_temp_calib_inited || !(s->skin_temp > 0.0f)) return false;

    const uint32_t now_ms = calib_now_ms();
    bool ok = false;
    const float core = skin_calib_core_c(&s_temp_calib, s->skin_temp, now_ms + 1u, &ok);

    if (out_status) *out_status = skin_calib_status(&s_temp_calib, s->skin_temp, now_ms + 1u);
    if (s_temp_calib.valid) {
        if (out_ref_c) *out_ref_c = s_temp_calib.ref_c;
        if (out_site)  *out_site = s_temp_calib.ref_site;
    }
    if (!ok) return false;
    if (out_core_c) *out_core_c = core;
    return true;
}


#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
static SemaphoreHandle_t s_lock = NULL;

/* The history module is usable before web_status_start() (app_main calls
 * web_status_history_init() independently, and a publish can arrive from the
 * monitor task first). Create the mutex lazily here so those callers never run
 * unlocked merely because the HTTP server has not started yet. */
static bool ensure_lock(void) {
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
    return s_lock != NULL;
}
#define WS_LOCK()   do { (void)ensure_lock(); if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY); } while (0)
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
/* Live PPG waveform ring                                                     */
/* ------------------------------------------------------------------------- */
/* A plain ring of the last WEB_STATUS_PPG_RING IR samples. The producer is the
 * PPG task and the consumer is the HTTP task, so it is guarded by its own
 * short critical section rather than by WS_LOCK(): the producer runs at 25 Hz
 * and must never wait on the status-page lock.
 *
 * Head/tail semantics: `head` is the next write index and `tail` the oldest
 * unread index. A drain takes up to max_points from the tail and advances it.
 * When the writer overtakes a stalled reader the oldest samples are discarded
 * - a late client gets the newest waveform, never an 8-second backlog. */
static uint32_t s_ppg_ring[WEB_STATUS_PPG_RING];
static size_t   s_ppg_ring_head  = 0;
static size_t   s_ppg_ring_tail  = 0;
static size_t   s_ppg_ring_count = 0;

void web_status_ppg_push(uint32_t ir_sample) {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&s_ppg_mux);
#endif
    s_ppg_ring[s_ppg_ring_head] = ir_sample;
    s_ppg_ring_head = (s_ppg_ring_head + 1) % WEB_STATUS_PPG_RING;
    if (s_ppg_ring_count == WEB_STATUS_PPG_RING) {
        /* Full: overwrite the oldest unread point. */
        s_ppg_ring_tail = (s_ppg_ring_tail + 1) % WEB_STATUS_PPG_RING;
    } else {
        s_ppg_ring_count++;
    }
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&s_ppg_mux);
#endif
}

void web_status_ppg_reset(void) {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&s_ppg_mux);
#endif
    s_ppg_ring_head  = 0;
    s_ppg_ring_tail  = 0;
    s_ppg_ring_count = 0;
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&s_ppg_mux);
#endif
}

size_t web_status_ppg_drain(uint32_t *out, size_t max_points) {
    if (!out || max_points == 0) return 0;
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&s_ppg_mux);
#endif
    size_t n = s_ppg_ring_count;
    if (n > max_points) n = max_points;
    for (size_t i = 0; i < n; i++) {
        out[i] = s_ppg_ring[(s_ppg_ring_tail + i) % WEB_STATUS_PPG_RING];
    }
    s_ppg_ring_tail = (s_ppg_ring_tail + n) % WEB_STATUS_PPG_RING;
    s_ppg_ring_count -= n;
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&s_ppg_mux);
#endif
    return n;
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

    if (origin && origin[0] != '\0') {
        if (strcmp(origin, expected) == 0) return true;
        if (strcmp(origin, "null") == 0) return true; /* local browser file:/// */
        return false;
    }
    if (!referer || strncmp(referer, expected, expected_len) != 0) return false;

    /* Require the host to end here or continue with a path. This rejects
     * lookalike hosts such as 192.168.4.1.attacker.example. */
    return referer[expected_len] == '\0' || referer[expected_len] == '/';
}

/* Pure: a struct in, a string out. No ESP-IDF, no locks, no I/O, so the wire
 * format is proved in the host test suite rather than by curling a live board. */
size_t web_status_json(const valor_status_t *s, char *buf, size_t cap) {
    if (!s || !buf || cap == 0) return 0;

    /* Derive the core estimate from the per-session offset HERE, so every caller
     * gets the same answer and the serializer cannot disagree with the page.
     * core_est_c stays 0.0f (serialised as null) unless a calibration exists,
     * is fresh, and the skin has not drifted out from under it. */
    valor_status_t local;
    local = *s;
    {
        float core = 0.0f, ref = 0.0f;
        const char *status = "uncalibrated", *site = "none";
        web_status_temp_core(&local, &core, &status, &ref, &site);
        local.core_est_c = core;
        local.temp_calib_status = status;
        local.temp_calib_ref_c = ref;
        local.temp_calib_site = site;
    }
    s = &local;

    size_t used = 0;
    buf[0] = '\0';
    if (!json_append_format(
        buf, cap, &used,
        "{\"hr\":%.1f,\"spo2\":%.1f,\"rr\":%.1f,\"sqi\":%.2f,\"rmssd\":%.1f,\"prv\":%.1f,\"hrvn\":%d,"
        "\"rrconf\":%.2f,\"rrdepth\":%.1f,\"pi\":%.2f,",
        (double)s->hr, (double)s->spo2, (double)s->rr, (double)s->sqi,
        (double)s->rmssd, (double)s->rmssd, s->hrv_n < 0 ? 0 : s->hrv_n,
        (double)s->rr_conf, (double)s->rr_depth_ms, (double)s->pi)) return 0;

    if (s->temp < -40.0f || s->temp > 85.0f) {
        if (!json_append(buf, cap, &used, "\"temp\":null,", 12U)) return 0;
    } else if (!json_append_format(buf, cap, &used, "\"temp\":%.1f,", (double)s->temp)) {
        return 0;
    }

    /* Clinical skin temperature. skintemp is null when there is no usable
     * reading; skinsrc says whether the number is a measured probe value or a
     * model output, so the page can label it. */
    if (!s->skin_valid || s->skin_temp < 20.0f || s->skin_temp > 50.0f) {
        if (!json_append(buf, cap, &used, "\"skintemp\":null,", 16)) return 0;
    } else if (!json_append_format(buf, cap, &used, "\"skintemp\":%.1f,", (double)s->skin_temp)) {
        return 0;
    }
    if (!json_append(buf, cap, &used, "\"skinsrc\":", sizeof("\"skinsrc\":") - 1) ||
        !json_append_string(buf, cap, &used,
                            s->skin_source ? s->skin_source : SKIN_SRC_NONE) ||
        !json_append_format(buf, cap, &used, "\"skinstale\":%s,", s->skin_stale ? "true" : "false") ||
        !json_append_format(buf, cap, &used,
                            ",\"skincal\":%s,\"skindie\":%.2f,\"skinweak\":%s,",
                            s->skin_calibrated ? "true" : "false",
                            (double)s->skin_die_delta,
                            s->skin_weak ? "true" : "false")) {
        return 0;
    }

    /* Peripheral thermal state. Emitted as null when there is no settled
     * contact so the page cannot mistake "we do not know yet" for "stable".
     * skin_trend_c is a CHANGE, not a temperature: a page that prints it as
     * one would be inventing a reading. */
    if (!json_append(buf, cap, &used, "\"skintherm\":", sizeof("\"skintherm\":") - 1)) return 0;
    if (!s->skin_therm_ok || !s->skin_therm_state) {
        if (!json_append(buf, cap, &used, "null,", 5)) return 0;
    } else if (!json_append_string(buf, cap, &used, s->skin_therm_state) ||
               !json_append_format(buf, cap, &used, ",\"skintrend\":%+.2f,\"skinrate\":%.2f,",
                                   (double)s->skin_trend_c,
                                   (double)s->skin_rate_c_per_min)) {
        return 0;
    }

    /* Core estimate from the per-session offset. null unless the calibration is
     * currently valid: a fingertip cannot give core temperature on its own, so
     * "no calibration" must serialise as absent rather than as a number. */
    if (!json_append(buf, cap, &used, "\"coretemp\":", sizeof("\"coretemp\":") - 1)) return 0;
    if (!(s->core_est_c > 0.0f)) {
        if (!json_append(buf, cap, &used, "null,", 5)) return 0;
    } else if (!json_append_format(buf, cap, &used, "%.2f,", (double)s->core_est_c)) {
        return 0;
    }
    if (!json_append(buf, cap, &used, "\"calstat\":", sizeof("\"calstat\":") - 1) ||
        !json_append_string(buf, cap, &used,
                            s->temp_calib_status ? s->temp_calib_status : "uncalibrated") ||
        !json_append_format(buf, cap, &used, ",\"calref\":%.2f,\"calsite\":",
                            (double)s->temp_calib_ref_c) ||
        !json_append_string(buf, cap, &used,
                            s->temp_calib_site ? s->temp_calib_site : "none") ||
        !json_append(buf, cap, &used, ",", 1)) {
        return 0;
    }

    if (s->hum < 0.0f) {
        if (!json_append(buf, cap, &used, "\"hum\":null,", 11)) return 0;
    } else {
        if (!json_append_format(buf, cap, &used, "\"hum\":%.1f,", (double)s->hum)) return 0;
    }

    /* PM2.5: a negative value means "no reading". It is serialised as null so
     * the page shows "--" and "not assessed" instead of a clean atmosphere. */
    if (s->pm25 < 0.0f) {
        if (!json_append(buf, cap, &used, "\"pm25\":null,", 12)) return 0;
    } else {
        if (!json_append_format(buf, cap, &used, "\"pm25\":%.1f,", (double)s->pm25)) return 0;
    }
    if (s->pm25_raw < 0.0f) {
        if (!json_append(buf, cap, &used, "\"pm25raw\":null,", 15)) return 0;
    } else {
        if (!json_append_format(buf, cap, &used, "\"pm25raw\":%.1f,", (double)s->pm25_raw)) return 0;
    }
    if (!json_append(buf, cap, &used, "\"pm25conf\":", sizeof("\"pm25conf\":") - 1) ||
        !json_append_string(buf, cap, &used, s->pm25_conf ? s->pm25_conf : "NODATA") ||
        !json_append(buf, cap, &used, ",", 1)) {
        return 0;
    }

    /* 24-hour PM2.5 and the standards-based AQI. Both are null / -1 until enough
     * complete hours exist, because a 24-hour AQI cannot be computed from
     * twenty minutes of data and claiming otherwise is exactly the error this
     * field exists to prevent. aqibasis tells the page which basis it is
     * looking at, so a short-term number can never be rendered as a 24-h one. */
    if (!(s->pm25_24h > 0.0f)) {
        if (!json_append(buf, cap, &used, "\"pm2524h\":null,", 13)) return 0;
    } else if (!json_append_format(buf, cap, &used, "\"pm2524h\":%.1f,", (double)s->pm25_24h)) {
        return 0;
    }
    if (s->aqi24 < 0) {
        if (!json_append(buf, cap, &used, "\"aqi24\":null,", 12)) return 0;
    } else if (!json_append_format(buf, cap, &used, "\"aqi24\":%d,", s->aqi24)) {
        return 0;
    }
    if (!json_append(buf, cap, &used, "\"aqibasis\":", sizeof("\"aqibasis\":") - 1) ||
        !json_append_string(buf, cap, &used, s->aqibasis ? s->aqibasis : "60s") ||
        !json_append(buf, cap, &used, ",", 1)) {
        return 0;
    }


    if (!json_append_format(
        buf, cap, &used,
        "\"pressure\":%.1f,\"ptrend\":%.2f,"
        "\"pdrop\":%.1f,\"pspan\":%lu,\"pvalid\":%s,"
        "\"news2\":%u,\"level\":%u,\"flags\":%u,"
        "\"risk\":",
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
        !json_append_format(buf, cap, &used, ",\"aqi\":%d,\"aqicat\":", s->aqi) ||
        !json_append_string(buf, cap, &used, s->aqi_cat ? s->aqi_cat : "") ||
        !json_append_format(buf, cap, &used, ",\"heatc\":%.1f,\"heatcat\":", (double)s->heat_c) ||
        !json_append_string(buf, cap, &used, s->heat_cat ? s->heat_cat : "") ||
        !json_append_format(buf, cap, &used, ",\"motion\":%.2f,\"fall\":", (double)s->motion) ||
        !json_append_string(buf, cap, &used, s->fall ? s->fall : "MONITORING") ||
        !json_append_format(buf, cap, &used,
                            ",\"psi\":%.2f,\"aha\":%.2f,"
                            "\"nnheat\":%.3f,\"nnpoll\":%.3f,\"nnflood\":%.3f,\"aivalid\":%s,"
                            "\"rheat\":",
                            (double)s->psi, (double)s->aha,
                            (double)s->nn_heat, (double)s->nn_pollution, (double)s->nn_flood,
                            s->ai_valid ? "true" : "false") ||
        !json_append_string(buf, cap, &used, s->rule_heat ? s->rule_heat : "UNKNOWN") ||
        !json_append(buf, cap, &used, ",\"rpoll\":", sizeof(",\"rpoll\":") - 1) ||
        !json_append_string(buf, cap, &used, s->rule_pollution ? s->rule_pollution : "UNKNOWN") ||
        !json_append(buf, cap, &used, ",\"rflood\":", sizeof(",\"rflood\":") - 1) ||
        !json_append_string(buf, cap, &used, s->rule_flood ? s->rule_flood : "UNKNOWN") ||
        !json_append_format(buf, cap, &used,
                            ",\"hrpts\":%u,\"spo2pts\":%u,\"rrpts\":%u,\"temppts\":%u",
                            s->news2_hr_pts, s->news2_spo2_pts,
                            s->news2_rr_pts, s->news2_temp_pts) ||
        !json_append(buf, cap, &used, ",\"ssid\":", sizeof(",\"ssid\":") - 1) ||
        !json_append_string(buf, cap, &used, s->ssid ? s->ssid : "") ||
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
    size_t n = 0;
    WS_LOCK();
    if (s_history_dirty) {
        n = web_status_history_encode(s_history, VALOR_HISTORY_HOURS,
                                      s_history_blob, sizeof(s_history_blob));
        if (n > 0) s_history_dirty = false;
    }
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
    s_accum_pm25_n = 0;
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
    while (s->uptime_s >= s_last_hour_s + VALOR_HISTORY_HOUR_SECONDS) {
        for (int i = VALOR_HISTORY_HOURS - 1; i > 0; i--) {
            s_history[i] = s_history[i - 1];
            s_history[i].hour_offset = (uint32_t)i;
        }
        s_history[0].hour_offset = 0;
        /* "valid" means the hour contains patient contact. An hour with only
           environmental samples (no finger on the sensor) is not a patient
           hour; its PM2.5 peak is still kept in the accumulator, and a missing
           PM2.5 reading is reported as -1, never as a measured 0. */
        if (s_accum_count > 0) {
            s_history[0].avg_hr = (s_accum_count > 0)
                                  ? s_accum_hr / (float)s_accum_count : 0.0f;
            s_history[0].avg_spo2 = (s_accum_spo2_n > 0)
                                    ? s_accum_spo2 / (float)s_accum_spo2_n : 0.0f;
            s_history[0].avg_rmssd = (s_accum_rmssd_n > 0)
                                     ? s_accum_rmssd / (float)s_accum_rmssd_n : 0.0f;
            /* -1 means "no PM2.5 reading in this hour", which is not the same
               claim as a peak of 0 (a measured clean atmosphere). */
            s_history[0].peak_pm25 = (s_accum_pm25_n > 0) ? s_accum_pm25 : -1.0f;
            s_history[0].max_news2 = s_accum_news2;
            s_history[0].valid = true;
        } else {
            s_history[0].avg_hr = 0.0f;
            s_history[0].avg_spo2 = 0.0f;
            s_history[0].avg_rmssd = 0.0f;
            s_history[0].peak_pm25 = -1.0f;
            s_history[0].max_news2 = 0;
            s_history[0].valid = false;
        }
        /* A slot just became a completed hour: persist the whole array once the
         * lock is released (history_persist_if_dirty). Even when the hour is
         * invalid this is written, so a genuine gap is preserved as a gap. */
        s_history_dirty = true;
        s_last_hour_s += VALOR_HISTORY_HOUR_SECONDS;
        s_accum_count = 0;
        s_accum_spo2_n = 0;
        s_accum_rmssd_n = 0;
        s_accum_hr = 0.0f;
        s_accum_spo2 = 0.0f;
        s_accum_rmssd = 0.0f;
        s_accum_pm25 = 0.0f;
        s_accum_pm25_n = 0;
        s_accum_news2 = 0;

        /* Prevent infinite loop on massive jumps (e.g. sleep/wake) */
        if (s->uptime_s >= s_last_hour_s + (VALOR_HISTORY_HOUR_SECONDS * VALOR_HISTORY_HOURS)) {
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
    /* Only a real reading counts. A negative pm25 means the sensor produced no
       value, which is not a PM2.5 of zero. */
    if (s->pm25 >= 0.0f) {
        if (s->pm25 > s_accum_pm25) {
            s_accum_pm25 = s->pm25;
        }
        s_accum_pm25_n++;
    }

    /* Update slot 0 live with running averages for the current hour.
       "valid" means the hour contains patient contact - an environmental-only
       hour (no finger on the sensor) is not a patient hour, so it is not
       published as one. The PM2.5 peak of such an hour is still kept in the
       accumulator, and a missing reading is reported as -1, never as 0. */
    s_history[0].hour_offset = 0;
    if (s_accum_count > 0) {
        s_history[0].avg_hr = (s_accum_count > 0)
                              ? s_accum_hr / (float)s_accum_count : 0.0f;
        s_history[0].avg_spo2 = (s_accum_spo2_n > 0)
                                ? s_accum_spo2 / (float)s_accum_spo2_n : 0.0f;
        s_history[0].avg_rmssd = (s_accum_rmssd_n > 0)
                                 ? s_accum_rmssd / (float)s_accum_rmssd_n : 0.0f;
        s_history[0].peak_pm25 = (s_accum_pm25_n > 0) ? s_accum_pm25 : -1.0f;
        s_history[0].max_news2 = s_accum_news2;
        s_history[0].valid = true;
    } else {
        /* No clinical contact in this hour. Clearing this is important: the old
           code left the previous hour's valid flag and values in slot 0, so the
           chart showed a stale current point. */
        s_history[0].avg_hr = 0.0f;
        s_history[0].avg_spo2 = 0.0f;
        s_history[0].avg_rmssd = 0.0f;
        s_history[0].peak_pm25 = -1.0f;
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
        if (p->peak_pm25 < 0.0f) {
            /* No PM2.5 reading in this hour: null, not 0. A zero would be a
               claim of clean air that was never measured. */
            if (!json_append_format(buf, cap, &used,
                                    "{\"hour\":%u,\"hr\":%.1f,\"spo2\":%.1f,\"rmssd\":%.1f,"
                                    "\"pm25\":null,\"news2\":%u,\"valid\":%s}",
                                    (unsigned)p->hour_offset,
                                    (double)p->avg_hr, (double)p->avg_spo2, (double)p->avg_rmssd,
                                    p->max_news2,
                                    p->valid ? "true" : "false")) {
                WS_UNLOCK();
                return 0;
            }
        } else if (!json_append_format(buf, cap, &used,
                                "{\"hour\":%u,\"hr\":%.1f,\"spo2\":%.1f,\"rmssd\":%.1f,"
                                "\"pm25\":%.1f,\"news2\":%u,\"valid\":%s}",
                                (unsigned)p->hour_offset,
                                (double)p->avg_hr, (double)p->avg_spo2, (double)p->avg_rmssd,
                                (double)p->peak_pm25, p->max_news2,
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

#include "valor_dashboard_gz.h"

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
"<meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; script-src 'sha256-cX+16m1WEpuvBN0jbcLrYsbkETqhQ1b+7mSLU/QfueE='; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; base-uri 'none'; object-src 'none'\">"
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
"<div style='display:flex;justify-content:space-between;align-items:center'><h1>VALOR</h1><a href='/desktop' style='display:inline-block;padding:6px 10px;background:#1E293B;color:#38BDF8;border:1px solid #334155;border-radius:6px;font-size:12px;font-weight:700;text-decoration:none'>🖥️ Desktop View</a></div>"
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
/* Skin temperature is a clinical vital, so it sits with the other vitals and
   not with the environmental cards. It is published only once the TMP117 has
   thermally settled on skin; until then the device sends null and this tile
   reads -- rather than showing a number the device does not stand behind. */
"<div class='card'><div class='k'>Skin temp</div>"
"<div class='v'><span id='skin'>--</span><span class='u'>&deg;C</span></div></div>"
"<div class='card'><div class='k'>Signal quality</div>"
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
"['hr','spo2','rmssd','rr','sqi','skin','temp','hum','pm','storm','pmv','heatv'].forEach(function(i){t(i,D)});"
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
"t('hr',positive(s.hr,0));t('spo2',positive(s.spo2,1));"
"t('rmssd',s.rmssd>0?s.rmssd.toFixed(1):(s.contact&&typeof s.hrvn==='number'&&s.hrvn>0&&s.hrvn<30?(s.hrvn+'/30'):D));"
"t('rr',positive(s.rr,0));t('sqi',s.contact&&typeof s.sqi==='number'?(s.sqi*100).toFixed(0):D);"
"t('temp',num(s.temp,1));t('hum',num(s.hum,0));t('pm',num(s.pm25,0));"
/* skintemp is null until the TMP117 has settled on skin. A null value means
   there is no settled skin temperature to show, so the tile stays -- instead of
   implying that the sensor is missing when it is merely still warming up. */
"t('skin',num(s.skintemp,1));"
"var storm=s.storm||'UNKNOWN';"
"if(!s.pvalid){t('storm','WARMING UP')}else{t('storm',storm)}"
"var sc={NORMAL:'#4ADE80',MODERATE:'#FBBF24',HIGH:'#FB923C',CRITICAL:'#F87171',UNKNOWN:'#94A3B8'};"
"document.getElementById('storm').style.color=sc[storm]||'';"
"t('pmv',num(s.pm25,1));"
/* PM2.5: a null value means the sensor produced no reading. That is shown as
   not-assessed - never as a clean atmosphere. The raw PMS5003 value and the
   confidence code of the published estimate are shown next to it, so the
   reader can see both the measurement and the correction.
   NOTE: keep this comment free of double-quote characters - the dashboard
   verification script extracts the embedded page with a simple string-literal
   scan, and a quoted word here would be picked up as page content. */
"var pmKnown=typeof s.pm25==='number'&&isFinite(s.pm25);"
"var pmConf={OK:'',SETTLING:'reading still settling',HUMID:'humidity-corrected, RH >= 80%',DUST:'dust aerosol, correction suppressed',RANGE:'above sensor range',NODATA:'no reading'};"
"var pmNote=pmKnown?(((typeof s.pm25raw==='number')?('raw '+s.pm25raw.toFixed(1)):'')+((s.pm25conf&&s.pm25conf!=='OK')?(' - '+pmConf[s.pm25conf]):'')):('no PM2.5 reading - this is not a clean-air report');"
"if(pmKnown&&typeof s.aqi==='number'&&s.aqi>=0&&s.aqicat){setBadge('aqibadge',s.aqicat+' | AQI '+s.aqi,aqiCol[s.aqicat]||'#94A3B8');"
"t('aqinote',aqiNote[s.aqicat]||'');"
"t('whonote','WHO 2021 24-h 15 ug/m3'+((s.pm25>15)?' - '+((s.pm25/15).toFixed(1))+'x guideline':' - within guideline')+(pmNote?' - '+pmNote:''))}"
"else if(pmKnown){setBadge('aqibadge',D,'');t('aqinote','Air quality not assessed.');t('whonote',pmNote)}"
"else{setBadge('aqibadge','Not assessed','#94A3B8');t('aqinote','PM2.5 sensor unavailable - air quality not assessed.');t('whonote',pmNote)}"
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
"function start(e){if(tm)return;ok=false;if(e&&e.preventDefault)e.preventDefault();t('sos-note','Hold for 3 s to cancel...');"
"tm=setTimeout(function(){tm=null;ok=true;postEmergency('/api/emergency/cancel','SOS cancel',3000)},3000)}"
"function stop(){if(tm){clearTimeout(tm);tm=null;if(!ok)t('sos-note','Cancel aborted - hold for 3 s')}ok=false}"
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
    /* Slot 0 is this power-on's in-progress hour; it is already cleared.
     * Report the slots that actually carry the valid flag, not the slot count
     * the blob always encodes (24), so the log cannot claim a full day's record
     * after a few minutes of uptime. An hour with only environmental samples is
     * still a recorded hour, hence the separate heart-rate figure. */
    size_t populated = 0, with_hr = 0;
    size_t limit = (n < VALOR_HISTORY_HOURS) ? n : VALOR_HISTORY_HOURS;
    for (size_t i = 0; i < limit; i++) {
        if (!stored[i].valid) continue;
        populated++;
        if (stored[i].avg_hr > 0.0f) with_hr++;
    }
    ESP_LOGI(TAG, "Restored NVS history: %zu of %d slots populated, %zu with a heart-rate average",
             populated, VALOR_HISTORY_HOURS, with_hr);
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
        "default-src 'none'; script-src 'sha256-cX+16m1WEpuvBN0jbcLrYsbkETqhQ1b+7mSLU/QfueE='; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; base-uri 'none'; object-src 'none'; frame-ancestors 'none'");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "X-Frame-Options", "DENY");
    httpd_resp_set_hdr(req, "Referrer-Policy", "same-origin");
    return httpd_resp_send(req, PAGE_HTML, HTTPD_RESP_USE_STRLEN);
}

/* CORS headers for wireless desktop dashboard access (file:/// and localhost). */
#define HTTP_CORS_ALLOW_ORIGIN  "Access-Control-" "Allow-Origin"
#define HTTP_CORS_ALLOW_METHODS "Access-Control-" "Allow-Methods"
#define HTTP_CORS_ALLOW_HEADERS "Access-Control-" "Allow-Headers"

static esp_err_t h_desktop(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    /* A one-hour cache on a page served from device flash was serving stale
     * copies after a firmware update - the operator saw the old dashboard and
     * concluded the board was not sending data. Revalidate every load; the
     * page is a single 35 KB gzip stream over SoftAP, so this is cheap. */
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "Referrer-Policy", "same-origin");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    /* Content-Length lets the browser treat the response as a document rather
     * than an open-ended stream some clients offered to download. */
    char lenbuf[16];
    snprintf(lenbuf, sizeof(lenbuf), "%u", (unsigned)VALOR_DASHBOARD_GZ_LEN);
    httpd_resp_set_hdr(req, "Content-Length", lenbuf);

    /* Send the whole payload in one call: with a declared length the web
     * server no longer has to emit chunked framing. */
    return httpd_resp_send(req, (const char *)VALOR_DASHBOARD_GZ,
                           (ssize_t)VALOR_DASHBOARD_GZ_LEN);
}

static esp_err_t h_ppg_json(httpd_req_t *req) {
    /* 25 Hz decimated IR samples since the last poll. Shape:
     *   {"n":N,"fs":25,"samples":[12345,12350,...]}
     * The browser pushes them straight into its 700-point scope ring. */
    static char buf[2048];
    static uint32_t pts[128];
    size_t n = web_status_ppg_drain(pts, sizeof(pts) / sizeof(pts[0]));

    size_t used = (size_t)snprintf(buf, sizeof(buf), "{\"n\":%u,\"fs\":25,\"samples\":[",
                                   (unsigned)n);
    for (size_t i = 0; i < n && used < sizeof(buf); i++) {
        int w = snprintf(buf + used, sizeof(buf) - used, (i == 0) ? "%lu" : ",%lu",
                         (unsigned long)pts[i]);
        if (w < 0) break;
        used += (size_t)w;
    }
    if (used < sizeof(buf) - 2) {
        buf[used++] = ']';
        buf[used++] = '}';
        buf[used]   = '\0';
    } else {
        buf[sizeof(buf) - 1] = '\0';
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "ppg json too large");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_METHODS, "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_HEADERS, "Content-Type");
    return httpd_resp_send(req, buf, (ssize_t)used);
}

static esp_err_t h_options(httpd_req_t *req) {
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_METHODS, "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_HEADERS, "Content-Type");
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

/* ---- /cal : record an oral reference and derive the core offset -----------
 *
 * A fingertip probe reads 3-7 C below core and the gap is vasoconstriction,
 * not sensor error, so there is no fixed skin->core conversion. The published
 * approach for continuous monitoring is an individualised correction factor:
 * take ONE reference reading with the probe in place, then report
 * core = skin + offset for as long as that offset holds.
 *
 * Usage from the SoftAP page or any phone on VALOR-0081:
 *     http://192.168.4.1/cal?ref=36.6
 *
 * The current skin value is read from the last published snapshot, so the
 * caller must have a settled finger on the sensor BEFORE calling this. That is
 * the entire point: the offset is only meaningful if skin and reference
 * describe the same moment.
 *
 * Deliberately GET, not POST: this is a single-purpose demo endpoint on a
 * private AP and a plain URL is what a user can type on a phone. It is
 * idempotent in the sense that repeating it with the same reference simply
 * re-derives the same offset. */
static esp_err_t h_cal(httpd_req_t *req) {
    char q[96];
    char refbuf[24] = {0};
    valor_status_t snap;

    if (!s_temp_calib_inited) {
        skin_calib_init(&s_temp_calib);
        s_temp_calib_inited = true;
    }

    if (httpd_req_get_url_query_str(req, q, sizeof(q)) != ESP_OK ||
        httpd_query_key_value(q, "ref", refbuf, sizeof(refbuf)) != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req,
            "{\"ok\":false,\"error\":\"missing ?ref=<oral_temp_C>\"}");
        return ESP_OK;
    }

    char *endp = NULL;
    const float ref_c = strtof(refbuf, &endp);
    if (endp == refbuf || !isfinite(ref_c)) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"ref is not a number\"}");
        return ESP_OK;
    }

    web_status_snapshot(&snap);
    const float skin_c = snap.skin_temp;

    const bool ok = web_status_temp_calib_set(skin_c, ref_c, "oral");

    static char out[256];
    if (ok) {
        snprintf(out, sizeof(out),
                 "{\"ok\":true,\"ref_c\":%.2f,\"skin_c\":%.2f,\"offset_c\":%.2f,"
                 "\"site\":\"oral\",\"note\":\"core = skin + offset for this session\"}",
                 (double)ref_c, (double)skin_c, (double)(ref_c - skin_c));
    } else {
        snprintf(out, sizeof(out),
                 "{\"ok\":false,\"ref_c\":%.2f,\"skin_c\":%.2f,"
                 "\"error\":\"rejected - need a settled finger on the probe and a "
                 "plausible oral temperature (30-43 C)\"}",
                 (double)ref_c, (double)skin_c);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_sendstr(req, out);
    return ESP_OK;
}

static esp_err_t h_json(httpd_req_t *req) {
    valor_status_t s;
    web_status_snapshot(&s);

    /* SCRATCH BUFFER MUST BE STATIC, NOT ON THE STACK.
     *
     * This handler used to declare `char buf[1536]` locally. The HTTP server
     * task runs with HTTPD_DEFAULT_CONFIG stack_size = 4096 bytes, shared with
     * every httpd call frame (recv, header parse, send). A 1.5 KB local plus
     * those frames leaves almost nothing, and once the AI/TinyML channels,
     * rule-engine strings, PSI/AHA, NEWS2 sub-scores and SSID pushed the JSON
     * past ~750 bytes the handler overflowed the server task stack on every
     * poll. The visible symptom was the board RESETTING under load and the
     * browser reporting ERR_CONNECTION_RESET - which looks exactly like a
     * network/Wi-Fi fault and is not one.
     *
     * Static storage removes the handler from the stack budget entirely. The
     * HTTP server serialises handler invocations for a given server instance,
     * and h_history already used the same pattern for its 4 KB buffer. */
    static char buf[1536];
    size_t n = web_status_json(&s, buf, sizeof(buf));
    if (n == 0) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "status too large");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_METHODS, "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_HEADERS, "Content-Type");
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
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_METHODS, "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_HEADERS, "Content-Type");
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
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_METHODS, "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_HEADERS, "Content-Type");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

/* Look for the PIN in the query string, then in the body already read. */
static bool control_pin_authorised(httpd_req_t *req, const char *body)
{
    char q[96];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK &&
        web_status_control_pin_matches(q)) {
        return true;
    }
    return web_status_control_pin_matches(body);
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
    /* Authorisation BEFORE acting. The hold_ms check alone only stopped
     * accidental taps; this stops anyone on the open AP from silencing an
     * emergency. PIN is visible on the OLED/serial for whoever holds the unit. */
    if (!control_pin_authorised(req, body)) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN,
                                   "PIN required - read it on the device display");
    }
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
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_ORIGIN, "*");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_METHODS, "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, HTTP_CORS_ALLOW_HEADERS, "Content-Type");
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

    /* Authorisation before the location is rewritten: the location is what a
     * rescue team is sent to, so it must not be editable by anyone who merely
     * joined the open AP. */
    if (!control_pin_authorised(req, body)) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN,
                                   "PIN required - read it on the device display");
    }

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
    /* Generate and publish the control PIN first: every state-changing endpoint
     * depends on it, and the boot log is where the operator reads it from. */
    (void)web_status_control_pin();
    ESP_LOGI(TAG, "Control PIN for SOS-cancel / location: %s (shown on OLED)",
             s_control_pin);

    if (!ensure_lock()) {
        ESP_LOGE(TAG, "status mutex allocation failed; status page disabled");
        return ESP_ERR_NO_MEM;
    }

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
    hcfg.max_uri_handlers = 20;
    hcfg.core_id = 1;
    /* 4096 (the IDF default) is not enough for this server: the handlers build
     * JSON strings, run snprintf with a dozen arguments, and one of them used
     * to put a 1.5 KB buffer in the frame. The overflow surfaced as the board
     * resetting whenever a dashboard polled it, which reads as a Wi-Fi fault.
     * 8192 leaves room for the deepest handler plus httpd's own frames. */
    hcfg.stack_size = 8192;
    /* A standalone, battery-powered unit is often polled by a phone that
     * disappears mid-request. Drop dead sockets faster than the 5 s default so
     * a stale connection cannot hold one of the 7 slots for a whole poll
     * interval. */
    hcfg.recv_wait_timeout = 3;
    hcfg.send_wait_timeout = 3;
    err = httpd_start(&s_server, &hcfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed (%s); AP is up but has no page", esp_err_to_name(err));
        s_ap_active = true;
        return err;
    }

    httpd_uri_t uri_root     = { .uri = "/",                      .method = HTTP_GET,     .handler = h_root };
    httpd_uri_t uri_desktop  = { .uri = "/desktop",               .method = HTTP_GET,     .handler = h_desktop };
    httpd_uri_t uri_dash     = { .uri = "/dashboard",             .method = HTTP_GET,     .handler = h_desktop };
    httpd_uri_t uri_json     = { .uri = "/status.json",           .method = HTTP_GET,     .handler = h_json };
    httpd_uri_t uri_loc      = { .uri = "/location",              .method = HTTP_POST,    .handler = h_set_location };
    httpd_uri_t uri_hist     = { .uri = "/history.json",          .method = HTTP_GET,     .handler = h_history };
    httpd_uri_t uri_api      = { .uri = "/api/history",           .method = HTTP_GET,     .handler = h_history };
    httpd_uri_t uri_ppg      = { .uri = "/ppg.json",              .method = HTTP_GET,     .handler = h_ppg_json };
    httpd_uri_t uri_cal      = { .uri = "/cal",                   .method = HTTP_GET,     .handler = h_cal };
    httpd_uri_t uri_sos_trig = { .uri = "/api/emergency/trigger", .method = HTTP_POST,    .handler = h_sos_trigger };
    httpd_uri_t uri_sos_canc = { .uri = "/api/emergency/cancel",  .method = HTTP_POST,    .handler = h_sos_cancel };
    httpd_uri_t uri_opt_trig = { .uri = "/api/emergency/trigger", .method = HTTP_OPTIONS, .handler = h_options };
    httpd_uri_t uri_opt_canc = { .uri = "/api/emergency/cancel",  .method = HTTP_OPTIONS, .handler = h_options };
    httpd_uri_t uri_opt_json = { .uri = "/status.json",           .method = HTTP_OPTIONS, .handler = h_options };
    httpd_uri_t uri_opt_api  = { .uri = "/api/history",           .method = HTTP_OPTIONS, .handler = h_options };
    httpd_register_uri_handler(s_server, &uri_root);
    httpd_register_uri_handler(s_server, &uri_desktop);
    httpd_register_uri_handler(s_server, &uri_dash);
    httpd_register_uri_handler(s_server, &uri_json);
    httpd_register_uri_handler(s_server, &uri_loc);
    httpd_register_uri_handler(s_server, &uri_hist);
    httpd_register_uri_handler(s_server, &uri_api);
    httpd_register_uri_handler(s_server, &uri_ppg);
    httpd_register_uri_handler(s_server, &uri_cal);
    httpd_register_uri_handler(s_server, &uri_sos_trig);
    httpd_register_uri_handler(s_server, &uri_sos_canc);
    httpd_register_uri_handler(s_server, &uri_opt_trig);
    httpd_register_uri_handler(s_server, &uri_opt_canc);
    httpd_register_uri_handler(s_server, &uri_opt_json);
    httpd_register_uri_handler(s_server, &uri_opt_api);
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, h_404);

    s_ap_active = true;
    ESP_LOGI(TAG, "Status page up: join WiFi '%s' (open), then open http://192.168.4.1", s_ssid);
    return ESP_OK;
}

#endif /* ESP_PLATFORM */
