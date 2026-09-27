/*
 * web_status.h — Local status page over a self-hosted access point (T1.3)
 * SIH26181 / VALOR: Personal Health Companion & Edge Disaster Monitor
 *
 * Requirement 5 asks the device to "operate effectively with intermittent or no
 * internet", and requirement 6 to assist in an emergency. A laptop tethered by
 * USB does neither: the whole premise is a field device, and the dashboard that
 * exists today needs a PC, a cable and a browser gesture to show anything.
 *
 * So the device broadcasts its own network and serves its own page. No router,
 * no credentials, no association step, and nothing to fail: a phone joins
 * VALOR-xxxx and reads the patient's status. That is the requirement
 * *demonstrated* rather than asserted.
 *
 * WHY THE JSON FORMATTER IS SEPARATE FROM THE SERVER
 * web_status_json() is pure - a struct in, a string out - so the wire format is
 * unit-tested on the host. Only the SoftAP and the HTTP handlers need ESP-IDF,
 * and they are behind #ifdef ESP_PLATFORM so this file still compiles into the
 * test harness.
 */

#ifndef SHRIKEFI_WEB_STATUS_H
#define SHRIKEFI_WEB_STATUS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One complete picture of the device, as the page consumes it. Everything here
 * is already computed on the device - the page renders, it does not derive. */
typedef struct {
    float    hr;
    float    spo2;
    float    rr;
    float    sqi;
    float    rmssd;
    float    temp;
    float    hum;
    float    pm25;

    unsigned news2;
    unsigned level;      /* clinical_risk_level_t ordinal */
    unsigned flags;      /* clinical_alert_flags_t bitmask */

    const char *risk;    /* fused triage verdict, e.g. "NORMAL" */
    const char *sos;     /* sos_state_name() */
    const char *trigger; /* sos_trigger_name() */
    const char *loc;     /* stored location, "UNSET" until T1.4 */

    uint32_t uptime_s;
    bool     contact;    /* finger on the sensor */
} valor_status_t;

/* Publish a new snapshot. Called at 1 Hz from the monitor task; the HTTP server
 * reads it from its own task, so the copy is internally locked. The pointed-to
 * strings must outlive the call - all three come from static name tables. */
void web_status_publish(const valor_status_t *s);

/* Copy the current snapshot out. Safe to call from any task. */
void web_status_snapshot(valor_status_t *out);

/* Serialise to JSON. Pure and deterministic - bounded, NUL-terminated, and
 * returns the length written excluding the terminator (0 if it did not fit).
 * Kept in the JSON shape the page polls:
 *   {"hr":72.1,...,"sos":"IDLE","trigger":"none","loc":"UNSET","contact":true} */
size_t web_status_json(const valor_status_t *s, char *buf, size_t cap);

#ifdef ESP_PLATFORM
#include "esp_err.h"

/* Raise the access point and start the HTTP server. Returns 0 on success.
 *
 * Failure is NOT fatal and must never be: the health monitor is the product and
 * the status page is a convenience, so a radio that will not come up must leave
 * the device measuring. Every step below is logged and returned rather than
 * ESP_ERROR_CHECK'd, because an abort here would take the vitals with it. */
esp_err_t web_status_start(void);

/* SSID actually in use, for the boot log and the OLED, or "" if not up. */
const char *web_status_ap_ssid(void);
bool web_status_ap_active(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_WEB_STATUS_H */
