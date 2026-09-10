# ==============================================================================
# Main Component for ShrikeFi Health Companion
# ==============================================================================

set(CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../core")
set(SHRIKEFI_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

idf_component_register(
    SRCS 
        "${SHRIKEFI_DIR}/main_shrikefi.c"
        "${SHRIKEFI_DIR}/shrikefi_link_driver.c"
        "${SHRIKEFI_DIR}/esp32_i2c_hal.c"
        "${SHRIKEFI_DIR}/max30102.c"
        "${SHRIKEFI_DIR}/bme280.c"
        "${SHRIKEFI_DIR}/pms5003.c"
        "${SHRIKEFI_DIR}/ssd1306.c"
        "${CORE_DIR}/hrv_analysis.c"
        "${CORE_DIR}/spo2_engine.c"
        "${CORE_DIR}/disaster_risk_engine.c"
        "${CORE_DIR}/nn_risk_model.c"
        "${CORE_DIR}/nn_risk_model_int8.c"
        "${CORE_DIR}/pm25_calibration_int8.c"
        "${CORE_DIR}/clinical_vitals_engine.c"
        "${SHRIKEFI_DIR}/wifi_mqtt_manager.c"
    INCLUDE_DIRS 
        "${SHRIKEFI_DIR}"
        "${CORE_DIR}"
    REQUIRES 
        driver
        freertos
        log
        esp_wifi
        esp_netif
        nvs_flash
        mqtt
)
