/**
 * @file wifi_mqtt_manager.h
 * @brief WiFi and MQTT Cloud Sync Manager for ESP32
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 */

#ifndef WIFI_MQTT_MANAGER_H
#define WIFI_MQTT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi (Station Mode) and connect to the configured AP.
 *        Also initializes the MQTT client.
 */
void wifi_mqtt_init(void);

/**
 * @brief Check if currently connected to the MQTT broker.
 * @return true if connected
 */
bool mqtt_is_connected(void);

/**
 * @brief Publish health and environmental data as a JSON payload to the cloud.
 * 
 * @param hr Heart Rate in BPM
 * @param rmssd HRV RMSSD in ms
 * @param spo2 SpO2 percentage
 * @param temp Ambient Temperature in Celsius
 * @param pm25 PM2.5 in ug/m3
 * @param risk_level String representation of overall risk (e.g., "CRITICAL")
 */
void cloud_publish_health_data(float hr, float rmssd, float spo2, float temp, float pm25, const char* risk_level);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MQTT_MANAGER_H
