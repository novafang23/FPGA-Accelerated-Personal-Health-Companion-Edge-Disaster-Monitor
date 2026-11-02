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
 * *demonstrated* rather than asserted. Because the AP is open, anyone in range
 * can read that status; this is a deliberate emergency-access/privacy tradeoff.
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
    float    sqi;          /* [0, 1]                                            */
    float    rmssd;        /* ms, 0 when withheld by the SQI / 9-pair gate      */
    int      hrv_n;        /* beats currently in the HRV window (0..30+)        */
    /* Respiratory-rate evidence and perfusion index, so the page can explain a
     * missing RR instead of guessing. rr_conf is [0,1]; rr_depth_ms is the RSA
     * envelope depth. pi is the SpO2 engine's perfusion index (%); 0 = not yet
     * measured, which the page must render as absent rather than as a value. */
    float    rr_conf;
    float    rr_depth_ms;
    float    pi;
    float    temp;         /* deg C, < -40 when BME280 has no reading           */
    /* Clinical skin temperature from TMP117 (0.0f = unavailable / settling).
     * skin_valid is true only when active optical contact is confirmed and
     * the sensor has thermally equilibrated (|dT/dt| <= 0.05 C/s over 3 samples). */
    float    skin_temp;
    bool     skin_valid;
    /* Where skin_temp came from. This exists so the page can label the number
     * honestly instead of implying a clinical contact thermometer:
     *   SKIN_SRC_NONE       no reading (sensor absent / not settled)
     *   SKIN_SRC_ESTIMATED  modelled from MAX30102 die temp + BME280 ambient
     *   SKIN_SRC_TMP117     measured by the dedicated clinical probe
     * An estimate must never be rendered with the same authority as a
     * measurement; it is also excluded from the NEWS2 temperature term, whose
     * temperature sub-score stays unpopulated unless a real probe is fitted. */
    const char *skin_source;
    /* True only when the model coefficients were fitted against a reference
     * thermometer. With the nominal defaults this is false, and the page must
     * render the value as "uncalibrated estimate" rather than as a
     * measurement. The TMP117 calibration run sets it true. */
    bool     skin_calibrated;
    /* How much the MAX30102 die thermometer actually contributed this contact:
     * skin_die_delta is (peak die - no-finger baseline) in Celsius, and
     * skin_weak is true when that delta is too small for the die to be driving
     * the published value. Without this the page cannot distinguish "the model
     * followed the sensor" from "the model returned its constant" - which is
     * exactly the failure that went unnoticed on hardware. */
    float    skin_die_delta;
    bool     skin_weak;
    /* Peripheral thermal state: "UNKNOWN" / "STABLE" / "COOLING" /
     * "REWARMING" / "VASOCONSTRICTED" (skin_therm_state_t as text).
     *
     * This is the answer to "what is the skin temperature telling us" that a
     * fingertip can actually support. It is a TREND relative to the subject's
     * own settled baseline, never a core-temperature estimate: skin sits 3-7 C
     * below core and that gap is vasoconstriction, not a calibratable offset.
     * A page must not present skin_trend_c as a temperature in its own right. */
    const char *skin_therm_state;    /* Signed change since the baseline frozen at contact settle (C). Negative
     * means the periphery is cooling. Valid only when skin_therm_ok is true. */
    float    skin_trend_c;
    /* Least-squares drift rate (C/min), clamped to a physically possible skin
     * rate so the estimator's own slew cannot masquerade as physiology. */
    float    skin_rate_c_per_min;
    bool     skin_therm_ok;
    /* Per-session core-offset calibration.
     *
     * A fingertip probe cannot report core temperature on its own: the skin sits
     * 3-7 C below core and that gap is vasoconstriction, not sensor error. The
     * published approach for continuous monitoring is an INDIVIDUALISED
     * correction factor - one reference reading (oral), then core = skin +
     * offset, revalidated as the session goes on.
     *
     * core_est_c is 0.0f whenever the calibration is not usable, and
     * temp_calib_status then explains why ("uncalibrated", "stale", "drifted",
     * "no contact"). A page must render 0.0f as "--", never as 0 C.
     */
    float    core_est_c;
    const char *temp_calib_status;
    /* The reference reading and site that produced the current calibration, so
     * the audit trail is visible on the page rather than implied. */
    float    temp_calib_ref_c;
    const char *temp_calib_site;
    float    hum;
    float    pm25;
    /* Raw PMS5003 reading behind the published PM2.5 estimate, and the
     * confidence code of that estimate ("OK", "SETTLING", "HUMID", "DUST",
     * "RANGE", "NODATA"). pm25 < 0 means "no reading" - the page must not
     * render that as a clean atmosphere. */
    float    pm25_raw;
    const char *pm25_conf;
    /* 24-hour PM2.5 mean and the standards-based AQI derived from it.
     *
     * EPA's PM2.5 AQI is defined on a 24-HOUR average. The live tile shows a
     * 60 s mean, which is far noisier and peaks higher, so an AQI computed from
     * it overstates the hazard. pm25_24h is -1 until enough complete hours
     * exist; aqi24 is -1 in the same condition. When they ARE available, they
     * are the defensible numbers; when they are not, aqibasis says 60s so no
     * page can present a one-minute mean as a 24-hour AQI. */
    float    pm25_24h;
    int      aqi24;
    const char *aqibasis;

    /* Literature-based environmental condition sentence (EPA AQI / WHO 2021 /
     * NWS heat index). Pointer to static storage owned by the publisher. */
    const char *env;
    /* Structured form of the same verdict, so the page can render tiles and
     * colour-coded badges without re-deriving the EPA/NWS categories.
     * aqi < 0 means "not assessed" (no PM2.5 reading). */
    int         aqi;
    const char *aqi_cat;
    float       heat_c;
    const char *heat_cat;

    /* Barometric pressure and its fitted trend (T3.1). The trend is the useful
     * number: absolute pressure varies with altitude and season, so the page
     * shows the rate of change alongside it and labels which one drives the
     * storm advisory. */
    float    pressure_hpa;
    float    pressure_trend_hpa_per_hr;
    float    pdrop_hpa;      /* fall over the trend window (+ = pressure fell) */
    uint32_t pspan_s;        /* seconds covered by the trend window */
    bool     pvalid;         /* window long enough to judge (>= 30 min) */

    unsigned news2;
    unsigned level;      /* clinical_risk_level_t ordinal */
    unsigned flags;      /* clinical_alert_flags_t bitmask */

    const char *risk;    /* fused triage verdict, e.g. "NORMAL" */
    const char *storm;   /* cyclone risk level, from the pressure trend */
    const char *sos;     /* sos_state_name() */
    const char *trigger; /* sos_trigger_name() */
    const char *loc;     /* stored location, "UNSET" until T1.4 */

    uint32_t uptime_s;
    bool     contact;    /* finger on the sensor */

    /* IMU motion & fall status */
    float       motion;  /* Instantaneous IMU motion energy in g */
    const char *fall;    /* Fall state name: "MONITORING", "FALL_CONFIRMED", etc. */

    /* On-device TinyML outputs. These were previously only printed to the
     * serial [TRIAGE] line, so the Wi-Fi dashboard had no way to show the AI
     * tiles at all - it silently fell back to invented values. ai_valid is
     * false when the INT8 model declined to score this cycle (no published
     * RMSSD, no environment, non-finite input); the page must then render
     * "declined", never 0 %. */
    float nn_heat;       /* [0,1] */
    float nn_pollution;  /* [0,1] */
    float nn_flood;      /* [0,1] */
    bool  ai_valid;

    /* Fused rule-engine hazard channels (RHEAT/RPOLL/RFLOOD on the serial
     * line). Shown as text so the page does not have to infer them. */
    const char *rule_heat;
    const char *rule_pollution;
    const char *rule_flood;

    /* Moran physiological strain index (0-10) and the AHA PM2.5-HRV
     * autonomic strain (0-1). Both are device-derived; 0 with no contact is
     * absence, not calm - the page gates them on contact. */
    float psi;
    float aha;

    /* Device identity, so a phone on the SoftAP can confirm which unit it is
     * talking to when more than one VALOR is in range. */
    const char *ssid;

    /* NEWS2 sub-scores the device actually computed (0..3 each). Kept separate
     * from the total so the page renders the board's own breakdown instead of
     * re-deriving it. */
    unsigned news2_hr_pts;
    unsigned news2_spo2_pts;
    unsigned news2_rr_pts;
    unsigned news2_temp_pts;
} valor_status_t;

/* Record a reference temperature and derive the per-session core offset.
 *
 * Caller supplies the skin value that was current WHEN the reference was
 * taken, so the two describe one moment. Returns false if the pair is not
 * usable (no contact, implausible reference, offset too large to
 * extrapolate) - in which case the previous calibration is left untouched.
 *
 * Exposed as public API rather than living only inside the /cal handler so
 * the behaviour is testable, and so a future hardware button could drive it
 * without duplicating the offset logic. */
bool web_status_temp_calib_set(float skin_c, float ref_c, const char *site);

/* Report the current calibration state for the given skin reading.
 * Fills *out_core_c with 0.0f and returns false when it is not usable. */
bool web_status_temp_core(const valor_status_t *s, float *out_core_c,
                          const char **out_status, float *out_ref_c,
                          const char **out_site);

/* Publish a new snapshot. Called at 1 Hz from the monitor task; the HTTP server
 * reads it from its own task, so the copy is internally locked. The pointed-to
 * strings must outlive the call - all three come from static name tables. */
void web_status_publish(const valor_status_t *s);

/* Copy the current snapshot out. Safe to call from any task. */
void web_status_snapshot(valor_status_t *out);

/* ------------------------------------------------------------------------- */
/* Skin-temperature provenance                                                */
/* ------------------------------------------------------------------------- */
#define SKIN_SRC_NONE      "NONE"
#define SKIN_SRC_ESTIMATED "ESTIMATED"
#define SKIN_SRC_TMP117    "TMP117"

/* ------------------------------------------------------------------------- */
/* Live PPG waveform transport (Wi-Fi dashboard oscilloscope)                 */
/* ------------------------------------------------------------------------- */

/* The desktop dashboard's scope draws real samples over USB serial, but the
 * SoftAP link carries only JSON. Without a waveform channel the Wi-Fi scope
 * can only show a synthesised pulse, which is why it was labelled "Synthetic
 * PPG". These calls feed a small decimated ring that /ppg.json drains, so the
 * standalone dashboard draws the same IR samples the board is actually
 * reading.
 *
 * Decimation: the FIFO runs at 100 Hz. Pushing every sample would be 100
 * points/s of JSON for a 700-point scope that redraws at most once per poll.
 * 25 Hz keeps ~4 s of waveform, which is what the serial scope shows too. */
#define WEB_STATUS_PPG_RING 200   /* 8 s at 25 Hz */

/* Append one 18-bit IR sample. Called from the PPG task; drop-in, never
 * blocks, and discards the oldest point when the ring is full. */
void web_status_ppg_push(uint32_t ir_sample);

/* Drain up to max_points samples into out[], oldest first, and return how many
 * were copied. The ring is emptied by a successful drain so each client sees
 * every sample exactly once. */
size_t web_status_ppg_drain(uint32_t *out, size_t max_points);

/* Reset the ring (called when contact is lost, so a new finger does not
 * inherit the previous one's tail). */
void web_status_ppg_reset(void);

/* Serialise to JSON. Pure and deterministic - strings are JSON-escaped, output
 * is bounded and NUL-terminated, and the function returns the length written
 * excluding the terminator (0 if it did not fit).
 * Kept in the JSON shape the page polls:
 *   {"hr":72.1,...,"sos":"IDLE","trigger":"none","loc":"UNSET","contact":true} */
size_t web_status_json(const valor_status_t *s, char *buf, size_t cap);

/* Pure origin check shared by SoftAP handlers and host regression tests.
 * Accepts same-origin http://192.168.4.1 and local browser file:/// origin ("null"). */
bool web_status_request_is_same_origin(const char *origin, const char *referer);

#define VALOR_HISTORY_HOURS 24

/* Length of one history "hour" in seconds of device uptime. This is not a
 * calendar hour and there is no wall clock: the ring fills at this rate from
 * boot. The default is a real hour. A bench build may override it (for example
 * -DVALOR_HISTORY_HOUR_SECONDS=60) to exercise ring wrap and oldest-slot
 * eviction in minutes instead of waiting a day; nothing else depends on the
 * value, and the on-flash format does not store it. */
#ifndef VALOR_HISTORY_HOUR_SECONDS
#define VALOR_HISTORY_HOUR_SECONDS 3600u
#endif

typedef struct {
    uint32_t hour_offset; /* 0 = current hour, 1 = 1 hour ago, ... up to 23 */
    float    avg_hr;
    float    avg_spo2;
    float    avg_rmssd;
    float    peak_pm25;
    unsigned max_news2;
    bool     valid;
} valor_history_point_t;

/* Canonical on-flash encoding for the 24-hour history (T3.2).
 *
 * This is NOT a memcpy of the struct. Each field is written explicitly in
 * little-endian order so the blob cannot be silently invalidated by compiler
 * padding, a bool whose size differs, or a future field added to the struct. A
 * 12-byte header carries a magic, a version, the point count and an FNV-1a
 * checksum over the payload; the decoder rejects a short buffer, a bad
 * magic/version/checksum, an out-of-range hour index, or a non-boolean valid
 * byte. Any of those means "unreadable", and the caller keeps the empty RAM
 * buffer rather than showing half-decoded history as real.
 *
 * These are pure so the round-trip and every corruption path are unit-tested on
 * the host. Only web_status_history_init() (load) and an hourly rollover (save)
 * touch NVS, and only on the device.
 *
 * Layout: 12-byte header + 24 * 25-byte points = 612 bytes. */
#define VALOR_HISTORY_BLOB_BYTES 612

/* Encode n points into buf. Returns the bytes written, or 0 if the arguments are
 * invalid or cap is too small. */
size_t web_status_history_encode(const valor_history_point_t *pts, size_t n,
                                 uint8_t *buf, size_t cap);

/* Decode a blob into pts. Returns the number of points decoded (<= n), or 0 if
 * the blob is unreadable for any reason. */
size_t web_status_history_decode(const uint8_t *buf, size_t len,
                                 valor_history_point_t *pts, size_t n);

/* Initialize the 24-hour history tracker.
 *
 * On the device this resets the RAM buffer, then restores the last saved set of
 * *completed* hours from NVS and shifts it so slot 0 is free for the hour that
 * is starting now. A power loss mid-hour therefore loses that partial hour
 * rather than promoting it to a completed one, and there is no calendar time
 * source: the slots are "hours of device uptime", not clock hours. On the host
 * the NVS step is a no-op. */
void web_status_history_init(void);

/* Copy 24-hour history points out thread-safely */
void web_status_history_snapshot(valor_history_point_t *out, size_t max_points);

/* Serialise 24-hour history points to JSON array:
 * [{"hour":0,"hr":72.5,"spo2":98.0,"rmssd":42.1,"pm25":14.2,"news2":0,"valid":true},...] */
size_t web_status_history_json(char *buf, size_t cap);

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


/* ---- Control PIN (available on host too, for tests) ----------------------
 *
 * The access point is OPEN by design so a rescuer can join with no app. That
 * means the endpoints which CHANGE state must be authorised, or anyone in radio
 * range could silence an SOS:
 *
 *     POST /api/emergency/cancel
 *     POST /location
 *
 * A four-digit PIN is generated at boot and shown on the OLED and in the serial
 * log. Read-only endpoints stay open, so the dashboard still loads with no
 * friction. Both functions are available on the host build so the gate is
 * unit-testable. */

/* Never NULL. Four characters. */
const char *web_status_control_pin(void);

/* True when `haystack` (query string, form body or JSON fragment) carries the
 * current PIN. Permissive about WHERE it appears, strict about its value. */
bool web_status_control_pin_matches(const char *haystack);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_WEB_STATUS_H */
