/*
 * web_status.c — Local status page over a self-hosted access point (T1.3)
 * SIH26181 / VALOR
 *
 * See web_status.h for why this exists and why the JSON formatter is kept
 * separate from the server.
 */

#include "web_status.h"
#include "location.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Snapshot                                                                   */
/* ------------------------------------------------------------------------- */

static valor_status_t s_snap;

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
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

/* Pure: a struct in, a string out. No ESP-IDF, no locks, no I/O, so the wire
 * format is proved in the host test suite rather than by curling a live board. */
size_t web_status_json(const valor_status_t *s, char *buf, size_t cap) {
    if (!s || !buf || cap == 0) return 0;

    int n = snprintf(
        buf, cap,
        "{\"hr\":%.1f,\"spo2\":%.1f,\"rr\":%.1f,\"sqi\":%.2f,\"rmssd\":%.1f,"
        "\"temp\":%.1f,\"hum\":%.1f,\"pm25\":%.1f,"
        "\"pressure\":%.1f,\"ptrend\":%.2f,"
        "\"news2\":%u,\"level\":%u,\"flags\":%u,"
        "\"risk\":\"%s\",\"storm\":\"%s\",\"sos\":\"%s\",\"trigger\":\"%s\",\"loc\":\"%s\","
        "\"contact\":%s,\"up\":%lu}",
        (double)s->hr, (double)s->spo2, (double)s->rr, (double)s->sqi,
        (double)s->rmssd, (double)s->temp, (double)s->hum, (double)s->pm25,
        (double)s->pressure_hpa, (double)s->pressure_trend_hpa_per_hr,
        s->news2, s->level, s->flags,
        s->risk ? s->risk : "UNKNOWN",
        s->storm ? s->storm : "UNKNOWN",
        s->sos ? s->sos : "IDLE",
        s->trigger ? s->trigger : "none",
        s->loc ? s->loc : "UNSET",
        s->contact ? "true" : "false",
        (unsigned long)s->uptime_s);

    if (n < 0) return 0;
    if ((size_t)n >= cap) return 0;   /* truncated: report failure, never a partial document */
    return (size_t)n;
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
"<div class='card wide'><div class='k'>Deployment location</div>"
"<form class='setloc' method='post' action='/location'>"
"<input id='locin' name='loc' maxlength='31' placeholder='Village / Block / District'>"
"<button type='submit'>Save</button></form>"
"<div id='locnote'>Stored on the device and shown on the emergency card.</div></div>"
"</div>"
"<div class='env'>"
"<div class='card'><div class='k'>Temp</div><div class='v'><span id='temp'>--</span></div></div>"
"<div class='card'><div class='k'>Humidity</div><div class='v'><span id='hum'>--</span></div></div>"
"<div class='card'><div class='k'>PM2.5</div><div class='v'><span id='pm'>--</span></div></div>"
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
"function num(v,d){return v>0?v.toFixed(d):D}"
"function poll(){"
"fetch('/status.json',{cache:'no-store'}).then(function(r){return r.json()})"
".then(function(s){"
"document.body.className=s.sos==='ACTIVE'?'sos':'';"
"t('risk',s.sos==='ACTIVE'?'MEDICAL EMERGENCY':s.risk);"
"t('msg',s.sos==='ACTIVE'?'SOS active ('+s.trigger+') - call emergency services':"
"'Triage verdict computed on the device. Updates every second.');"
"t('hr',num(s.hr,0));t('spo2',num(s.spo2,1));t('rmssd',num(s.rmssd,1));"
"t('rr',num(s.rr,0));t('sqi',s.sqi>0?(s.sqi*100).toFixed(0):D);"
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
"}).catch(function(){t('link','reconnecting')});"
"}"
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

/* POST /location — the caregiver records where this device is deployed.
 *
 * A phone on the access point is the only input device this system has. There
 * is no keypad, and a serial console needs a laptop and a cable, which is the
 * exact dependency the local page exists to remove. */
static esp_err_t h_set_location(httpd_req_t *req) {
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
    /* Open by design. This is an emergency status page: requiring a passphrase
     * would be a credential a bystander does not have. Nothing confidential
     * leaves the device over it - the cloud path stays off (requirement 5). */
    ap.ap.authmode       = WIFI_AUTH_OPEN;

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) { ESP_LOGE(TAG, "set_mode failed (%s)", esp_err_to_name(err)); return err; }
    err = esp_wifi_set_config(WIFI_IF_AP, &ap);
    if (err != ESP_OK) { ESP_LOGE(TAG, "set_config failed (%s)", esp_err_to_name(err)); return err; }
    err = esp_wifi_start();
    if (err != ESP_OK) { ESP_LOGE(TAG, "wifi_start failed (%s)", esp_err_to_name(err)); return err; }

    httpd_config_t hcfg = HTTPD_DEFAULT_CONFIG();
    hcfg.lru_purge_enable = true;
    hcfg.max_uri_handlers = 6;
    err = httpd_start(&s_server, &hcfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed (%s); AP is up but has no page", esp_err_to_name(err));
        s_ap_active = true;
        return err;
    }

    httpd_uri_t uri_root = { .uri = "/",            .method = HTTP_GET, .handler = h_root };
    httpd_uri_t uri_json = { .uri = "/status.json", .method = HTTP_GET, .handler = h_json };
    httpd_uri_t uri_loc  = { .uri = "/location",    .method = HTTP_POST, .handler = h_set_location };
    httpd_register_uri_handler(s_server, &uri_root);
    httpd_register_uri_handler(s_server, &uri_json);
    httpd_register_uri_handler(s_server, &uri_loc);
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, h_404);

    s_ap_active = true;
    ESP_LOGI(TAG, "Status page up: join WiFi '%s' (open), then open http://192.168.4.1", s_ssid);
    return ESP_OK;
}

#endif /* ESP_PLATFORM */
