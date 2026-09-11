# ==============================================================================
# Main Component for ShrikeFi Health Companion
# ==============================================================================

set(CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../core")
set(SHRIKEFI_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

# ── Optional local WiFi credentials ──────────────────────────────────────────
# wifi_credentials.h is gitignored, so a fresh clone does not have it. Resolve
# the WiFi mode HERE and report it, rather than probing in C source: an in-source
# __has_include() probe was found to evaluate false under this build's include
# flags even when the header existed, silently dropping the credentials and
# compiling the offline path with no error. Configure-time detection also means
# the mode is visible in the build log instead of being a silent surprise.
#
# NOTE: this is evaluated at configure time. After creating or deleting
# wifi_credentials.h, run `idf.py reconfigure` (or `idf.py fullclean`) so the
# mode is re-read. The firmware also logs the mode it actually booted with, so a
# stale configure can never be mistaken for a working WiFi setup.
# (file(GLOB ... CONFIGURE_DEPENDS) would re-check automatically but is rejected
#  in ESP-IDF's script-mode component pass.)
if(EXISTS "${SHRIKEFI_DIR}/wifi_credentials.h")
    set(SHRIKEFI_WIFI_MODE "WiFi + MQTT ENABLED (credentials from wifi_credentials.h)")
    set(SHRIKEFI_WIFI_MODE_DEF "SHRIKEFI_HAVE_WIFI_CREDENTIALS=1")
elseif(DEFINED CONFIG_SHRIKEFI_WIFI_SSID AND NOT CONFIG_SHRIKEFI_WIFI_SSID STREQUAL "")
    set(SHRIKEFI_WIFI_MODE "WiFi + MQTT ENABLED (credentials from menuconfig/sdkconfig)")
    set(SHRIKEFI_WIFI_MODE_DEF "SHRIKEFI_WIFI_FROM_KCONFIG=1")
else()
    set(SHRIKEFI_WIFI_MODE "OFFLINE edge mode - WiFi/MQTT disabled (no credentials configured)")
    set(SHRIKEFI_WIFI_MODE_DEF "SHRIKEFI_WIFI_OFFLINE=1")
endif()
message(STATUS "ShrikeFi: ${SHRIKEFI_WIFI_MODE}")

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

target_compile_definitions(${COMPONENT_LIB} PRIVATE ${SHRIKEFI_WIFI_MODE_DEF})
