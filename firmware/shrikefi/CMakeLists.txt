# ==============================================================================
# ESP-IDF CMakeLists.txt for ShrikeFi Health Companion
# Project: SIH26181 Personal Health Companion & Edge Disaster Monitor
# Target: ESP32-S3 (Xtensa Dual-Core)
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# Include core platform-agnostic algorithms
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../core")

set(COMPONENT_SRCS
    "main_shrikefi.c"
    "shrikefi_link_driver.c"
    "esp32_i2c_hal.c"
    "${CORE_DIR}/hrv_analysis.c"
    "${CORE_DIR}/spo2_engine.c"
    "${CORE_DIR}/disaster_risk_engine.c"
    "${CORE_DIR}/nn_risk_model.c"
    "${CORE_DIR}/nn_risk_model_int8.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
    "${CORE_DIR}"
)

set(COMPONENT_REQUIRES
    driver
    freertos
    log
)

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES ${COMPONENT_REQUIRES}
)
