/**
 * @file wifi_mqtt_manager.c
 * @brief WiFi and MQTT Cloud Sync Manager for ESP32
 */

#include "wifi_mqtt_manager.h"

#ifdef ESP_PLATFORM
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

/* ── WiFi credentials ────────────────────────────────────────────────────────
 * Credentials are deliberately NOT hardcoded here: this file is tracked in a
 * public repository. Resolved in this order:
 *
 *   1. firmware/shrikefi/wifi_credentials.h   (gitignored, see the .example)
 *   2. idf.py menuconfig -> "ShrikeFi configuration" -> WiFi SSID / Password
 *   3. Neither set  -> WiFi/MQTT is skipped entirely (offline edge mode).
 */
/* Detection of wifi_credentials.h happens in main/CMakeLists.txt, NOT with an
 * in-source __has_include probe. An earlier revision used
 *     #if __has_include("wifi_credentials.h")
 * and it silently evaluated false under this build's include flags even when
 * the header was present on disk, so the firmware compiled the offline path and
 * dropped the credentials without any error. Detecting in CMake is explicit, is
 * reported at configure time, and cannot fail quietly.
 */
#if defined(SHRIKEFI_HAVE_WIFI_CREDENTIALS)
#  include "wifi_credentials.h"
#endif

#ifndef SHRIKEFI_WIFI_SSID
#  ifdef CONFIG_SHRIKEFI_WIFI_SSID
#    define SHRIKEFI_WIFI_SSID CONFIG_SHRIKEFI_WIFI_SSID
#  else
#    define SHRIKEFI_WIFI_SSID ""
#  endif
#endif

#ifndef SHRIKEFI_WIFI_PASS
#  ifdef CONFIG_SHRIKEFI_WIFI_PASS
#    define SHRIKEFI_WIFI_PASS CONFIG_SHRIKEFI_WIFI_PASS
#  else
#    define SHRIKEFI_WIFI_PASS ""
#  endif
#endif

#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"  // Free public test broker
#define MQTT_TOPIC     "sih26181/shrikefi/health"

static const char *TAG = "WIFI_MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool s_mqtt_connected = false;
static int  s_wifi_retries   = 0;
static bool s_scan_dumped    = false;   /* scan diagnostic runs at most once */
static volatile bool s_scanning = false; /* suppress auto-reconnect during a scan */

/* Human-readable form of the common esp_wifi disconnect reasons. Without this
 * a failing association is indistinguishable from a wrong password, a 5 GHz-only
 * SSID, or an out-of-range AP. */
static const char *wifi_reason_to_string(int reason) {
    switch (reason) {
        case WIFI_REASON_UNSPECIFIED:            return "unspecified";
        case WIFI_REASON_AUTH_EXPIRE:            return "auth expired";
        case WIFI_REASON_AUTH_LEAVE:             return "auth leave";
        case WIFI_REASON_ASSOC_EXPIRE:           return "association expired";
        case WIFI_REASON_ASSOC_TOOMANY:          return "AP has too many stations";
        case WIFI_REASON_NOT_AUTHED:             return "not authenticated";
        case WIFI_REASON_NOT_ASSOCED:            return "not associated";
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "WRONG PASSWORD (4-way handshake timeout)";
        case WIFI_REASON_HANDSHAKE_TIMEOUT:      return "handshake timeout - WRONG PASSWORD";
        case WIFI_REASON_NO_AP_FOUND:            return "SSID NOT FOUND (wrong name, or 5GHz-only AP)";
        case WIFI_REASON_CONNECTION_FAIL:        return "connection fail";
        case WIFI_REASON_BEACON_TIMEOUT:         return "beacon timeout (out of range)";
        case WIFI_REASON_AUTH_FAIL:              return "authentication failed - WRONG PASSWORD";
        case WIFI_REASON_ASSOC_FAIL:             return "association failed";
        default:                                 return "see esp_wifi_types.h";
    }
}

/* ── WiFi scan diagnostic ────────────────────────────────────────────────────
 * WIFI_REASON_NO_AP_FOUND (201) says only that the driver's scan did not return
 * the configured SSID. Four very different faults produce it:
 *
 *   1. a typo, a wrong suffix, or a trailing space in the SSID
 *   2. an AP that advertises only on 5 GHz
 *   3. the AP is out of range
 *   4. a radio or antenna fault (on this board, a U.FL connector with no
 *      antenna fitted behaves exactly like "no networks exist")
 *
 * The reason code cannot distinguish them, and guessing costs a flash cycle
 * each time. So after the third failure the firmware scans and prints what is
 * ACTUALLY on the air - every visible SSID with channel, RSSI and auth mode -
 * plus an explicit comparison against the configured name. One boot answers it.
 *
 * Because the ESP32-S3 has no 5 GHz radio, every AP in this list is by
 * definition a 2.4 GHz network. That makes the test decisive: if the target SSID
 * appears here, it IS on 2.4 GHz and the problem is elsewhere; if it does not
 * appear while a phone can see it, the router is not broadcasting that name on
 * 2.4 GHz.
 *
 * Runs in its own task because esp_wifi_scan_start(..., true) blocks, and the
 * caller is the system event task - blocking there would stall every other
 * event in the system.
 *
 * The FIRST version of this did not disconnect before scanning, and on hardware
 * it failed with:
 *     wifi:sta_scan: STA is connecting, scan are not allowed!
 *     esp_wifi_scan_start failed: ESP_ERR_WIFI_STATE
 * The driver refuses to scan while a connect attempt is in flight, which is
 * precisely the state the third disconnect leaves it in. The wrapper task now
 * disconnects first, holds off the auto-reconnect for the duration, and
 * reconnects afterwards.
 */
static void wifi_scan_and_report(void)
{
    const char *want     = SHRIKEFI_WIFI_SSID;
    const size_t want_len = strlen(want);

    ESP_LOGW(TAG, "================ WiFi scan diagnostic ================");
    ESP_LOGW(TAG, "Configured SSID: \"%s\"  (%u bytes)", want, (unsigned)want_len);
    if (want_len > 0 && (want[0] == ' ' || want[want_len - 1] == ' ')) {
        ESP_LOGE(TAG, "  ^ starts or ends with a SPACE. SSIDs may legally contain "
                      "spaces, and a stray one is invisible in every editor and in "
                      "every log line above. Check the bytes, not the picture.");
    }

    wifi_scan_config_t scan_cfg = { 0 };
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true /* block until done */);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_start failed: %s", esp_err_to_name(err));
        return;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0) {
        ESP_LOGE(TAG, "Scan found ZERO access points.");
        ESP_LOGE(TAG, "  This is NOT an SSID problem - the radio is receiving nothing.");
        ESP_LOGE(TAG, "  Check the antenna. On ShrikeFi the WiFi antenna is a U.FL/IPEX "
                      "connector: a board with no antenna fitted scans identically to a "
                      "board in a Faraday cage. A metal enclosure or a hand wrapped "
                      "around the module does the same thing.");
        ESP_LOGW(TAG, "======================================================");
        return;
    }
    if (ap_count > 24) ap_count = 24;   /* bound the console output */

    wifi_ap_record_t *aps = calloc(ap_count, sizeof(wifi_ap_record_t));
    if (aps == NULL) {
        ESP_LOGE(TAG, "out of memory listing %u APs", (unsigned)ap_count);
        return;
    }
    err = esp_wifi_scan_get_ap_records(&ap_count, aps);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records failed: %s", esp_err_to_name(err));
        free(aps);
        return;
    }

    static const char *auth_names[] = {
        "OPEN", "WEP", "WPA_PSK", "WPA2_PSK",
        "WPA_WPA2_PSK", "WPA2_ENTERPRISE", "WPA3_PSK", "WPA2_WPA3_PSK"
    };
    ESP_LOGW(TAG, "%u AP(s) visible on 2.4 GHz (the ESP32-S3 radio has no 5 GHz):",
             (unsigned)ap_count);

    bool exact = false, ci_match = false, substr = false;
    const char *closest = NULL;
    for (int i = 0; i < (int)ap_count; i++) {
        const char *ssid = (const char *)aps[i].ssid;
        const char *auth = ((int)aps[i].authmode <
                            (int)(sizeof(auth_names) / sizeof(auth_names[0])))
                           ? auth_names[aps[i].authmode] : "UNKNOWN";
        ESP_LOGW(TAG, "  %2d. \"%s\"  ch=%2u  %4d dBm  %s",
                 i + 1, ssid, aps[i].primary, aps[i].rssi, auth);

        if (strcmp(ssid, want) == 0) {
            exact = true;
        } else if (strcasecmp(ssid, want) == 0) {
            ci_match = true; closest = ssid;
        } else if (strstr(ssid, want) != NULL || strstr(want, ssid) != NULL) {
            substr = true; closest = ssid;
        }
    }

    if (exact) {
        ESP_LOGW(TAG, "Verdict: the configured SSID IS broadcast and visible, yet "
                      "association failed. This is NOT a name or band problem - look "
                      "at password, auth mode and PMF instead.");
    } else if (ci_match) {
        ESP_LOGE(TAG, "Verdict: found \"%s\" - same letters, DIFFERENT CASE. SSIDs are "
                      "case-sensitive. Fix wifi_credentials.h.", closest);
    } else if (substr) {
        ESP_LOGE(TAG, "Verdict: closest name on the air is \"%s\". The configured SSID "
                      "is a typo, or is missing/extra a suffix.", closest);
    } else {
        ESP_LOGE(TAG, "Verdict: nothing resembling \"%s\" is being broadcast on "
                      "2.4 GHz.", want);
        ESP_LOGE(TAG, "  Most likely: the router advertises this name on 5 GHz only, or "
                      "uses a different name for its 2.4 GHz radio (common on Airtel/Jio "
                      "routers: e.g. \"..._5G\" and \"..._2.4G\" as separate SSIDs). Log "
                      "into the router and read the 2.4 GHz SSID.");
    }
    ESP_LOGW(TAG, "======================================================");

    free(aps);
}

/* Disconnect, scan, reconnect. The driver will not scan while connecting, and
 * disconnecting fires WIFI_EVENT_STA_DISCONNECTED whose handler would
 * immediately reconnect and re-block the scan - hence s_scanning. */
static void wifi_scan_diagnostic_task(void *arg)
{
    (void)arg;

    s_scanning = true;
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(300));

    wifi_scan_and_report();

    s_scanning = false;
    esp_wifi_connect();
    vTaskDelete(NULL);
}

/* Event handler for WiFi and IP events */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t *)event_data;
        int reason = d ? d->reason : -1;
        s_wifi_retries++;

        /* Log the first few attempts in full, then only every 10th, so a
         * permanently unreachable AP cannot flood the console. */
        if (s_wifi_retries <= 3 || (s_wifi_retries % 10) == 0) {
            ESP_LOGW(TAG, "WiFi disconnected (reason %d: %s). Attempt #%d.",
                     reason, wifi_reason_to_string(reason), s_wifi_retries);
        }
        /* Once, on the third failure, stop guessing and look at the airwaves. */
        if (s_wifi_retries == 3 && !s_scan_dumped) {
            s_scan_dumped = true;
            if (xTaskCreate(wifi_scan_diagnostic_task, "wifi_scan", 4096, NULL, 4, NULL)
                != pdPASS) {
                ESP_LOGE(TAG, "could not start the scan diagnostic task");
            }
        }
        /* Skip the automatic retry while the scan diagnostic owns the radio. */
        if (!s_scanning) {
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        s_wifi_retries = 0;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Connect to MQTT broker now that we have WiFi
        if (mqtt_client != NULL) {
            esp_mqtt_client_start(mqtt_client);
        }
    }
}

/* Event handler for MQTT events */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    (void)event;
    (void)handler_args;
    (void)base;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected to %s", MQTT_BROKER_URI);
            s_mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            s_mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error");
            break;
        default:
            break;
    }
}

void wifi_mqtt_init(void) {
    /* Compile-time check: an empty SSID string literal has size 1. */
    if (sizeof(SHRIKEFI_WIFI_SSID) <= 1) {
        ESP_LOGW(TAG, "No WiFi credentials configured - running in OFFLINE edge mode. "
                      "Set them in firmware/shrikefi/wifi_credentials.h (see the .example) "
                      "or via 'idf.py menuconfig' -> ShrikeFi configuration.");
        return;
    }

    /* Printed with quotes and an explicit byte count: a trailing space in the
     * SSID is legal, invisible here, and produces exactly the same
     * WIFI_REASON_NO_AP_FOUND (201) as a wrong name. */
    ESP_LOGI(TAG, "Initializing WiFi (SSID: \"%s\", %u bytes)...",
             SHRIKEFI_WIFI_SSID, (unsigned)strlen(SHRIKEFI_WIFI_SSID));

    // Initialize NVS (needed by WiFi driver)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SHRIKEFI_WIFI_SSID,
            .password = SHRIKEFI_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize MQTT
    ESP_LOGI(TAG, "Initializing MQTT Client...");
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    // Note: mqtt_client_start is called after we get an IP address
}

bool mqtt_is_connected(void) {
    return s_mqtt_connected;
}

void cloud_publish_health_data(float hr, float rmssd, float spo2, float temp, float pm25, const char* risk_level) {
    if (!s_mqtt_connected || mqtt_client == NULL) {
        return;
    }

    char payload[256];
    snprintf(payload, sizeof(payload), 
             "{\"hr\":%.1f, \"hrv_rmssd\":%.1f, \"spo2\":%.1f, \"temp\":%.1f, \"pm25\":%.1f, \"risk\":\"%s\"}",
             hr, rmssd, spo2, temp, pm25, risk_level);

    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 1, 0);
    ESP_LOGD(TAG, "Published msg_id=%d: %s", msg_id, payload);
}

#else

// Stubs for non-ESP compilation (e.g. Host testing)
void wifi_mqtt_init(void) {}
bool mqtt_is_connected(void) { return false; }
void cloud_publish_health_data(float hr, float rmssd, float spo2, float temp, float pm25, const char* risk_level) {
    (void)hr; (void)rmssd; (void)spo2; (void)temp; (void)pm25; (void)risk_level;
}

#endif // ESP_PLATFORM
