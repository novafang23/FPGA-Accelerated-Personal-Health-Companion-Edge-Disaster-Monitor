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
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Snapshot                                                                   */
/* ------------------------------------------------------------------------- */

static valor_status_t s_snap;

/* ------------------------------------------------------------------------- */
/* 24-Hour Rolling History (Requirement 7 / T3.2)                            */
/* ------------------------------------------------------------------------- */

static valor_history_point_t s_history[VALOR_HISTORY_HOURS];
static uint32_t s_last_hour_s = 0;
static bool     s_last_hour_init = false;
static uint32_t s_accum_count = 0;
static float    s_accum_hr = 0.0f;
static float    s_accum_spo2 = 0.0f;
static float    s_accum_rmssd = 0.0f;
static float    s_accum_pm25 = 0.0f;
static unsigned s_accum_news2 = 0;
static bool     s_history_initialized = false;

static void history_feed_locked(const valor_status_t *s);

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
#endif

void web_status_publish(const valor_status_t *s) {
    if (!s) return;
    WS_LOCK();
    s_snap = *s;
    history_feed_locked(s);
    WS_UNLOCK();
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
        "\"news2\":%u,\"level\":%u,\"flags\":%u,"
        "\"risk\":",
        (double)s->pm25,
        (double)s->pressure_hpa, (double)s->pressure_trend_hpa_per_hr,
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

void web_status_history_init(void) {
    WS_LOCK();
    for (size_t i = 0; i < VALOR_HISTORY_HOURS; i++) {
        s_history[i].hour_offset = (uint32_t)i;
        s_history[i].avg_hr = 0.0f;
        s_history[i].avg_spo2 = 0.0f;
        s_history[i].avg_rmssd = 0.0f;
        s_history[i].peak_pm25 = 0.0f;
        s_history[i].max_news2 = 0;
        s_history[i].valid = false;
    }
    s_last_hour_s = 0;
    s_last_hour_init = false;
    s_accum_count = 0;
    s_accum_hr = 0.0f;
    s_accum_spo2 = 0.0f;
    s_accum_rmssd = 0.0f;
    s_accum_pm25 = 0.0f;
    s_accum_news2 = 0;
    s_history_initialized = true;
    WS_UNLOCK();
}

static void history_feed_locked(const valor_status_t *s) {
    if (!s_history_initialized) {
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
    }

    if (!s_last_hour_init) {
        s_last_hour_s = s->uptime_s;
        s_last_hour_init = true;
    }

    /* Accumulate vitals when tissue contact is present */
    if (s->contact && s->hr >= 30.0f && s->hr <= 240.0f) {
        s_accum_hr += s->hr;
        s_accum_spo2 += s->spo2;
        s_accum_rmssd += s->rmssd;
        s_accum_count++;
        if (s->news2 > s_accum_news2) {
            s_accum_news2 = s->news2;
        }
    }
    if (s->pm25 > s_accum_pm25) {
        s_accum_pm25 = s->pm25;
    }

    /* Check if 1 hour (3600 seconds) has elapsed to shift the rolling window */
    while (s->uptime_s >= s_last_hour_s + 3600) {
        for (int i = VALOR_HISTORY_HOURS - 1; i > 0; i--) {
            s_history[i] = s_history[i - 1];
            s_history[i].hour_offset = (uint32_t)i;
        }
        s_history[0].hour_offset = 0;
        if (s_accum_count > 0) {
            s_history[0].avg_hr = s_accum_hr / (float)s_accum_count;
            s_history[0].avg_spo2 = s_accum_spo2 / (float)s_accum_count;
            s_history[0].avg_rmssd = s_accum_rmssd / (float)s_accum_count;
            s_history[0].peak_pm25 = s_accum_pm25;
            s_history[0].max_news2 = s_accum_news2;
            s_history[0].valid = true;
        } else {
            s_history[0].avg_hr = 0.0f;
            s_history[0].avg_spo2 = 0.0f;
            s_history[0].avg_rmssd = 0.0f;
            s_history[0].peak_pm25 = s_accum_pm25;
            s_history[0].max_news2 = 0;
            s_history[0].valid = (s_accum_pm25 > 0.0f);
        }
        s_last_hour_s += 3600;
        s_accum_count = 0;
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

    /* Update slot 0 live with running averages for the current hour */
    s_history[0].hour_offset = 0;
    if (s_accum_count > 0) {
        s_history[0].avg_hr = s_accum_hr / (float)s_accum_count;
        s_history[0].avg_spo2 = s_accum_spo2 / (float)s_accum_count;
        s_history[0].avg_rmssd = s_accum_rmssd / (float)s_accum_count;
        s_history[0].peak_pm25 = s_accum_pm25;
        s_history[0].max_news2 = s_accum_news2;
        s_history[0].valid = true;
    } else if (s->contact && s->hr >= 30.0f) {
        s_history[0].avg_hr = s->hr;
        s_history[0].avg_spo2 = s->spo2;
        s_history[0].avg_rmssd = s->rmssd;
        s_history[0].peak_pm25 = s->pm25;
        s_history[0].max_news2 = s->news2;
        s_history[0].valid = true;
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
"<meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; script-src 'sha256-Xvs0iA3Eui62afmq1ENjhb7IPyFMNHudlEFFYxdk+uY='; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; base-uri 'none'; object-src 'none'\">"
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
"function clearReadings(){document.body.className='';t('risk','NO CONNECTION');"
"t('msg','Live status unavailable; current readings cleared.');"
"['hr','spo2','rmssd','rr','sqi','temp','hum','pm','storm'].forEach(function(i){t(i,D)});"
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
"t('storm',s.storm);"
/* The trend is the number that carries the signal; the absolute pressure is
   shown beside it only so a reader can see the barometer they are reading. */
"t('pnote','Pressure '+(s.pressure>0?s.pressure.toFixed(1):D)+' hPa, trend '"
"+((s.ptrend<0?'':'+')+s.ptrend.toFixed(2))+' hPa/hr');"
"t('news2','NEWS2 '+s.news2);t('loc','LOC '+s.loc);"
"t('link',s.contact?'finger detected':'no finger');"
/* Seed the location box once. Doing it on every poll would overwrite whatever
   the caregiver is halfway through typing. */
"if(!locFilled){var el=document.getElementById('locin');"
"if(el&&s.loc!=='UNSET'){el.value=s.loc}locFilled=true}"
"}).catch(clearReadings);"
"}"
"function postEmergency(path,action){t('sos-note','Sending '+action+' request...');"
"fetch(path,{method:'POST'}).then(function(r){if(!r.ok)throw new Error('HTTP '+r.status);return r.json()})"
".then(function(){t('sos-note',action+' request sent');poll()})"
".catch(function(){t('sos-note',action+' request failed; check the connection')})}"
"document.getElementById('sos-trigger').addEventListener('click',function(){postEmergency('/api/emergency/trigger','SOS trigger')});"
"document.getElementById('sos-cancel').addEventListener('click',function(){postEmergency('/api/emergency/cancel','SOS cancel')});"
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
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "WEB_STATUS";

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
        "default-src 'none'; script-src 'sha256-Xvs0iA3Eui62afmq1ENjhb7IPyFMNHudlEFFYxdk+uY='; style-src 'unsafe-inline'; connect-src 'self'; form-action 'self'; base-uri 'none'; object-src 'none'; frame-ancestors 'none'");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(req, "X-Frame-Options", "DENY");
    httpd_resp_set_hdr(req, "Referrer-Policy", "same-origin");
    return httpd_resp_send(req, PAGE_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t h_json(httpd_req_t *req) {
    valor_status_t s;
    web_status_snapshot(&s);

    char buf[512];
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
    sos_manual_trigger(now_ms);
    WS_LOCK();
    s_snap.sos = sos_state_name(sos_get_state());
    s_snap.trigger = sos_trigger_name(sos_get_trigger());
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
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    sos_cancel_now(now_ms);
    WS_LOCK();
    s_snap.sos = sos_state_name(sos_get_state());
    s_snap.trigger = sos_trigger_name(sos_get_trigger());
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
