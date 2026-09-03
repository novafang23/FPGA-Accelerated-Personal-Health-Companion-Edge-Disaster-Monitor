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

#define WIFI_SSID      "Airtel_Abhi-506"     // NOTE: ESP32-S3 is 2.4GHz ONLY
#define WIFI_PASS      "Abhinav@2006"
#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"  // Free public test broker
#define MQTT_TOPIC     "sih26181/shrikefi/health"

static const char *TAG = "WIFI_MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool s_mqtt_connected = false;

/* Event handler for WiFi and IP events */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from WiFi. Reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
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
    ESP_LOGI(TAG, "Initializing WiFi...");

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
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
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
void cloud_publish_health_data(float hr, float rmssd, float spo2, float temp, float pm25, const char* risk_level) {}

#endif // ESP_PLATFORM
