/* test_status_json.c - validates that /status.json is actually parseable JSON.
 *
 * WHY THIS EXISTS
 * ---------------
 * The dashboard and the local status page both do `JSON.parse()` on
 * /status.json. If the payload is malformed the parse throws, the page clears
 * every reading and shows "reconnecting" forever - which looks exactly like a
 * Wi-Fi fault and is not one.
 *
 * That is precisely what happened: three separate serialisation bugs shipped in
 * one afternoon and none was caught, because every existing test checked the
 * TEXT of fields rather than whether the document parses:
 *
 *   1. `"skinsrc":"TMP117""skinstale":false,,` - a new field was spliced into
 *      the middle of an `||` append chain, losing the separator before it and
 *      duplicating the one after.
 *   2. `"skintrend":+6.60` - a printf `%+.2f` emitted a leading '+', which JSON
 *      does not permit on numbers.
 *   3. `"pm2524h":nul` - a hand-counted literal length was 13 where the text was
 *      15, truncating "null," to "nul" and swallowing the delimiter.
 *
 * So this file contains a small recursive-descent JSON parser and runs the real
 * serialiser through it. Anything that would make JSON.parse() throw fails here
 * instead of on a user's phone.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "web_status.h"

static int fails = 0;
static void ck(int cond, const char *what) {
    printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) fails++;
}

/* ---- minimal strict JSON parser ---------------------------------------- */
typedef struct { const char *p; int depth; int keys; } jp_t;

static void jp_ws(jp_t *j) {
    while (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r') j->p++;
}

static int jp_value(jp_t *j, int depth);

static int jp_string(jp_t *j) {
    if (*j->p != '"') return 0;
    j->p++;
    while (*j->p && *j->p != '"') {
        if (*j->p == '\\') {
            j->p++;
            switch (*j->p) {
                case '"': case '\\': case '/': case 'b':
                case 'f': case 'n': case 'r': case 't': break;
                case 'u': {
                    for (int i = 0; i < 4; i++) {
                        j->p++;
                        if (!isxdigit((unsigned char)*j->p)) return 0;
                    }
                    break;
                }
                default: return 0;
            }
        } else if ((unsigned char)*j->p < 0x20) {
            return 0;   /* raw control character */
        }
        j->p++;
    }
    if (*j->p != '"') return 0;
    j->p++;
    return 1;
}

static int jp_number(jp_t *j) {
    const char *start = j->p;
    if (*j->p == '-') j->p++;
    /* A leading '+' is NOT valid JSON. This is the exact bug that shipped. */
    if (*j->p == '+') return 0;
    if (*j->p < '0' || *j->p > '9') return 0;
    while (*j->p >= '0' && *j->p <= '9') j->p++;
    if (*j->p == '.') {
        j->p++;
        if (*j->p < '0' || *j->p > '9') return 0;
        while (*j->p >= '0' && *j->p <= '9') j->p++;
    }
    if (*j->p == 'e' || *j->p == 'E') {
        j->p++;
        if (*j->p == '+' || *j->p == '-') j->p++;
        if (*j->p < '0' || *j->p > '9') return 0;
        while (*j->p >= '0' && *j->p <= '9') j->p++;
    }
    return j->p > start;
}

static int jp_literal(jp_t *j, const char *lit) {
    size_t n = strlen(lit);
    if (strncmp(j->p, lit, n) != 0) return 0;
    j->p += n;
    return 1;
}

static int jp_array(jp_t *j, int depth) {
    if (depth > 8) return 0;
    j->p++;                       /* '[' */
    jp_ws(j);
    if (*j->p == ']') { j->p++; return 1; }
    for (;;) {
        if (!jp_value(j, depth + 1)) return 0;
        jp_ws(j);
        if (*j->p == ',') { j->p++; jp_ws(j); continue; }
        if (*j->p == ']') { j->p++; return 1; }
        return 0;
    }
}

static int jp_object(jp_t *j, int depth) {
    if (depth > 8) return 0;
    j->p++;                       /* '{' */
    jp_ws(j);
    if (*j->p == '}') { j->p++; return 1; }
    for (;;) {
        if (!jp_string(j)) return 0;
        j->keys++;
        jp_ws(j);
        if (*j->p != ':') return 0;
        j->p++;
        jp_ws(j);
        if (!jp_value(j, depth + 1)) return 0;
        jp_ws(j);
        if (*j->p == ',') { j->p++; jp_ws(j); continue; }
        if (*j->p == '}') { j->p++; return 1; }
        return 0;                 /* missing separator -> this is the ".."" bug */
    }
}

static int jp_value(jp_t *j, int depth) {
    jp_ws(j);
    switch (*j->p) {
        case '{': return jp_object(j, depth);
        case '[': return jp_array(j, depth);
        case '"': return jp_string(j);
        case 't': return jp_literal(j, "true");
        case 'f': return jp_literal(j, "false");
        case 'n': return jp_literal(j, "null");
        default:  return jp_number(j);
    }
}

/* Returns 1 when the whole buffer is exactly one valid JSON value. */
static int json_valid(const char *s, int *out_keys) {
    jp_t j = { s, 0, 0 };
    if (!jp_value(&j, 0)) return 0;
    jp_ws(&j);
    if (out_keys) *out_keys = j.keys;
    return *j.p == '\0';
}

int main(void) {
    static char buf[8192];
    valor_status_t s;
    int keys = 0;

    printf("status.json validity tests\n==========================\n");

    /* ---- 1. bare minimum: everything absent --------------------------- */
    memset(&s, 0, sizeof s);
    size_t n = web_status_json(&s, buf, sizeof buf);
    ck(n > 0, "empty snapshot serialises");
    ck(json_valid(buf, &keys), "empty snapshot is valid JSON");
    printf("     (%zu bytes, %d keys)\n", n, keys);

    /* ---- 2. realistic live snapshot ----------------------------------- */
    memset(&s, 0, sizeof s);
    s.hr = 97.2f; s.spo2 = 97.4f; s.sqi = 0.83f; s.rmssd = 46.5f; s.hrv_n = 43;
    s.pi = 1.16f; s.temp = 26.7f; s.hum = 53.3f;
    s.pm25 = 256.8f; s.pm25_raw = 272.6f; s.pm25_conf = "OK";
    s.skin_temp = 30.9f; s.skin_valid = true; s.skin_source = "TMP117";
    s.skin_calibrated = true; s.skin_therm_state = "REWARMING";
    s.skin_therm_ok = true; s.skin_trend_c = 6.60f; s.skin_rate_c_per_min = 1.2f;
    s.aqibasis = "60s"; s.ssid = "VALOR-0081"; s.risk = "CRITICAL";
    s.storm = "UNKNOWN"; s.sos = "IDLE"; s.trigger = "none";
    s.loc = "KIET COLLEGE GHAZIABAD"; s.fall = "MONITORING";
    s.rule_heat = "NORMAL"; s.rule_pollution = "CRITICAL"; s.rule_flood = "NORMAL";
    n = web_status_json(&s, buf, sizeof buf);
    ck(n > 0, "live snapshot serialises");
    ck(json_valid(buf, &keys), "live snapshot is valid JSON");
    printf("     (%zu bytes, %d keys)\n", n, keys);
    ck(strstr(buf, "\"skinstale\":") != NULL, "skinstale present (the field that broke it)");
    ck(strstr(buf, "\"prv\":") != NULL, "prv present");
    ck(strstr(buf, "\"skintherm\":\"REWARMING\"") != NULL, "skintherm present");

    /* ---- 3. NEGATIVE trend: must not emit a leading '+' --------------- */
    memset(&s, 0, sizeof s);
    s.skin_temp = 30.0f; s.skin_valid = true;
    s.skin_therm_state = "COOLING"; s.skin_therm_ok = true;
    s.skin_trend_c = -1.234f; s.skin_rate_c_per_min = -0.5f;
    n = web_status_json(&s, buf, sizeof buf);
    ck(json_valid(buf, &keys), "negative skin trend is valid JSON (no leading '+')");
    ck(strstr(buf, ":-") != NULL, "negative value rendered with '-'");
    ck(strstr(buf, ":+") == NULL, "no value rendered with a leading '+'");

    /* ---- 4. every optional block populated ---------------------------- */
    memset(&s, 0, sizeof s);
    s.hr = 128.4f; s.spo2 = 88.3f; s.rr = 27.1f; s.sqi = 0.93f;
    s.rmssd = 123.4f; s.hrv_n = 128; s.rr_conf = 0.87f; s.rr_depth_ms = 412.3f;
    s.pi = 12.34f; s.temp = 39.7f; s.hum = 98.6f;
    s.skin_temp = 41.23f; s.skin_valid = true; s.skin_source = "ESTIMATED";
    s.skin_die_delta = 3.75f; s.skin_weak = true; s.skin_stale = true;
    s.skin_therm_state = "VASOCONSTRICTED"; s.skin_therm_ok = true;
    s.skin_trend_c = -9.99f; s.skin_rate_c_per_min = -2.0f;
    s.core_est_c = 42.99f; s.temp_calib_status = "uncalibrated";
    s.temp_calib_ref_c = 39.5f; s.temp_calib_site = "axillary";
    s.pm25 = 499.9f; s.pm25_raw = 512.4f; s.pm25_conf = "SETTLING";
    s.pm25_24h = 488.8f; s.aqi24 = 499; s.aqibasis = "24h";
    s.env = "PM2.5 499.9 ug/m3: Hazardous (EPA AQI 500, 2024). Heat index 44.2 C: Danger (NWS).";
    s.aqi = 500; s.aqi_cat = "Hazardous"; s.heat_c = 44.2f; s.heat_cat = "Danger";
    s.pressure_hpa = 1101.7f; s.pressure_trend_hpa_per_hr = -12.34f;
    s.pdrop_hpa = 99.9f; s.pvalid = true;
    s.news2 = 20; s.level = 3; s.flags = 0xFFFF;
    s.risk = "CRITICAL"; s.storm = "SEVERE"; s.sos = "COUNTDOWN";
    s.trigger = "MANUAL_BUTTON"; s.loc = "KIET COLLEGE GHAZIABAD";
    s.contact = true; s.motion = 9.87f; s.fall = "FALL_CONFIRMED";
    s.nn_heat = 1.0f; s.nn_pollution = 1.0f; s.nn_flood = 1.0f; s.ai_valid = true;
    s.rule_heat = "CRITICAL"; s.rule_pollution = "CRITICAL"; s.rule_flood = "CRITICAL";
    s.psi = 9.87f; s.aha = 9.87f; s.ssid = "VALOR-0081";
    s.news2_hr_pts = 3; s.news2_spo2_pts = 3; s.news2_rr_pts = 3; s.news2_temp_pts = 2;
    s.uptime_s = 999999;
    n = web_status_json(&s, buf, sizeof buf);
    ck(n > 0, "fully populated snapshot serialises");
    ck(json_valid(buf, &keys), "fully populated snapshot is valid JSON");
    printf("     (%zu bytes, %d keys)\n", n, keys);

    /* ---- 5. the specific malformations that shipped -------------------- */
    ck(strstr(buf, ",,") == NULL, "no doubled comma");
    ck(strstr(buf, "nul\"") == NULL, "no truncated 'null'");
    ck(strstr(buf, "nul,") == NULL, "no truncated 'null,'");

    /* ---- 6. fits the device buffer ------------------------------------ */
    ck(n < 1536, "worst-case payload fits the 1536-byte device buffer");
    printf("     headroom: %d bytes\n", (int)(1536 - (int)n));

    /* ---- 7. the parser itself rejects bad input ----------------------- */
    {
        int k;
        ck(!json_valid("{\"a\":1\"b\":2}", &k), "parser rejects a missing comma");
        ck(!json_valid("{\"a\":+1}", &k), "parser rejects a leading '+'");
        ck(!json_valid("{\"a\":nul}", &k), "parser rejects truncated null");
        ck(!json_valid("{\"a\":1,}", &k), "parser rejects a trailing comma");
        ck(json_valid("{\"a\":1}", &k), "parser accepts valid input");
    }

    printf("\n%s (%d failure%s)\n",
           fails ? "FAILED" : "ALL STATUS-JSON CHECKS PASSED",
           fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
