/**
 * @file wifi_mqtt_manager.c
 * @brief WiFi and MQTT Cloud Sync Manager for ESP32
 */

#include "wifi_mqtt_manager.h"

#ifdef ESP_PLATFORM
#include <string.h>
#include <stdio.h>
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
            if (s_wifi_retries == 3) {
                ESP_LOGW(TAG, "Still failing. Check: SSID is 2.4GHz (ESP32-S3 has no 5GHz radio), "
                              "password is correct, and the AP is in range. Further retries logged every 10th.");
            }
        }
        esp_wifi_connect();
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

    ESP_LOGI(TAG, "Initializing WiFi (SSID: %s)...", SHRIKEFI_WIFI_SSID);

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
