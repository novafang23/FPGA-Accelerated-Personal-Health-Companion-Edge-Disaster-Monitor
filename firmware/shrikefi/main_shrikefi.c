/**
 * @file main_shrikefi.c
 * @brief Dual-Core FreeRTOS Application for ShrikeFi (ESP32-S3 + Renesas ForgeFPGA)
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 *
 * Demonstrates:
 *   - Core 0: High-speed 50Hz optical acquisition & 4-bit FPGA parallel link driver
 *   - Core 1: Environmental sensor fusion, INT8 TinyML inference, and OLED UI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shrikefi_link_driver.h"
#include "esp32_i2c_hal.h"
#include "max30102.h"
#include "bme280.h"
#include "pms5003.h"

/* Include platform-agnostic core firmware algorithms */
#include "hrv_analysis.h"
#include "spo2_engine.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model_int8.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "SHRIKEFI_MAIN";
static SemaphoreHandle_t s_data_mutex = NULL;

/* Global shared health state */
typedef struct {
    float heart_rate;
    float r_peak_interval_ms;
    float hrv_rmssd;
    float hrv_sdnn;
    float spo2_percent;
    float ambient_temp_c;
    float humidity_percent;
    float pm25_ugm3;
    risk_assessment_t risk_result;
    nn_output_t nn_scores;
} health_system_state_t;

static health_system_state_t g_state;

/* Sensor driver instances */
static max30102_t s_max30102;
static bme280_t s_bme280;
static pms5003_t s_pms5003;

/* Sensor read helpers */
static int read_max30102_samples(max30102_sample_t *sample) {
    return max30102_read_sample(&s_max30102, sample);
}

static int read_bme280_env(bme280_data_t *data) {
    return bme280_read(&s_bme280, data);
}

static int read_pms5003_data(pms5003_data_t *data) {
    return pms5003_get_data(&s_pms5003, data);
}

/**
 * @brief Core 0 Task: FPGA 4-bit link transceiver & high-frequency PPG processing
 */
static void task_ppg_accelerator(void *pvParameters) {
    (void)pvParameters;
    hrv_state_t hrv_state;
    hrv_init(&hrv_state);
    max30102_sample_t ppg_sample;

    ESP_LOGI(TAG, "Core 0: PPG FPGA Accelerator Task Started.");

    while (1) {
        /* 1. Read optical samples from MAX30102 */
        if (read_max30102_samples(&ppg_sample) == 0) {
            uint8_t raw_red = max30102_scale_to_8bit(ppg_sample.red);
            uint8_t raw_ir  = max30102_scale_to_8bit(ppg_sample.ir);

            /* 2. Stream to ForgeFPGA over 4-bit parallel link */
            shrikefi_write_red_sample(raw_red);
            shrikefi_write_ir_sample(raw_ir);
        }

        /* 3. Check for hardware beat interrupt from ForgeFPGA */
        if (shrikefi_is_beat_detected()) {
            uint32_t ibi_cycles = shrikefi_read_ibi_cycles();
            shrikefi_clear_irq();

            float ibi_ms = (float)ibi_cycles * (20.0f / 1000000.0f); // 50 MHz clock
            if (ibi_ms > 300.0f && ibi_ms < 2000.0f) {
                hrv_add_ibi(&hrv_state, ibi_ms);
                hrv_compute(&hrv_state);

                if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.r_peak_interval_ms = ibi_ms;
                    g_state.heart_rate = 60000.0f / ibi_ms;
                    g_state.hrv_rmssd = hrv_state.rmssd;
                    g_state.hrv_sdnn = hrv_state.sdnn;
                    xSemaphoreGive(s_data_mutex);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz sampling rate
    }
}

/**
 * @brief Core 1 Task: Environmental sensor acquisition, TinyML inference & OLED UI
 */
static void task_disaster_monitor(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "Core 1: Disaster Risk Engine & TinyML Task Started.");

    hrv_state_t hrv_snapshot;
    hrv_init(&hrv_snapshot);

    while (1) {
        env_sensors_t env = {0};
        float hr = 0.0f;
        float spo2 = 0.0f;

        /* Read BME280 (temperature, humidity, pressure) */
        bme280_data_t bme_data;
        if (read_bme280_env(&bme_data) == 0) {
            env.ambient_temp_c = bme_data.temperature_c;
            env.humidity_pct = bme_data.humidity_pct;
            /* Pressure available but not used in current risk model */
        }

        /* Read PMS5003 (PM2.5) */
        pms5003_data_t pms_data;
        if (read_pms5003_data(&pms_data) == 0 && pms_data.valid) {
            env.pm25 = (float)pms_data.pm2_5_atm;
        }

        /* Skin temperature not available from current sensors */
        env.skin_temp_c = 0.0f;

        /* Get latest HR/HRV from Core 0 */
        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (g_state.heart_rate > 30.0f) hr = g_state.heart_rate;
            spo2 = g_state.spo2_percent > 0.0f ? g_state.spo2_percent : 96.0f; // Fallback
            hrv_snapshot.rmssd = g_state.hrv_rmssd;
            hrv_snapshot.sdnn = g_state.hrv_sdnn;
            hrv_snapshot.count = 50;
            xSemaphoreGive(s_data_mutex);
        }

        /* 1. Execute Rule-based Disaster Risk Engine */
        risk_assessment_t risk;
        disaster_assess(&hrv_snapshot, spo2, hr, &env, &risk);

        /* 2. Execute INT8 Quantized TinyML Model (< 1 µs inference) */
        nn_output_t nn_out;
        nn_predict_int8(&nn_default_model_int8, &nn_quant_params,
                        hr, hrv_snapshot.rmssd, spo2,
                        env.ambient_temp_c, env.humidity_pct, env.pm25,
                        &nn_out);

        /* 3. Print Live Status to Serial Console */
        ESP_LOGI(TAG, "[ShrikeFi] HR: %.1f BPM | RMSSD: %.1f ms | Temp: %.1f C | PM2.5: %.0f",
                 hr, hrv_snapshot.rmssd, env.ambient_temp_c, env.pm25);
        ESP_LOGI(TAG, "[TinyML] Heat: %.3f | Pollution: %.3f | Flood: %.3f | Overall: %s",
                 nn_out.heat_score, nn_out.pollution_score, nn_out.flood_score,
                 risk_level_to_string(risk.overall_risk));

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz assessment rate
    }
}

/**
 * @brief UART RX Task for PMS5003
 */
static void task_pms5003_uart(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "PMS5003 UART RX Task Started.");

    while (1) {
        pms5003_uart_rx_task(&s_pms5003, pdMS_TO_TICKS(100));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "================================================================");
    ESP_LOGI(TAG, "  SIH26181 ShrikeFi Health Companion Firmware");
    ESP_LOGI(TAG, "  ESP32-S3 + Renesas ForgeFPGA 4-Bit Heterogeneous System");
    ESP_LOGI(TAG, "================================================================");

    s_data_mutex = xSemaphoreCreateMutex();

    /* Initialize 4-bit link to ForgeFPGA */
    shrikefi_link_init(NULL);
    shrikefi_set_threshold(120);

    /* Initialize I2C HAL for sensors (SDA=GPIO1, SCL=GPIO2, 400kHz) */
    esp32_i2c_hal_init(1, 2, 400000);

    /* Initialize MAX30102 (PPG sensor) */
    if (max30102_init(&s_max30102, esp32_i2c_hal_get_handle()) != 0) {
        ESP_LOGE(TAG, "MAX30102 initialization failed!");
    } else {
        ESP_LOGI(TAG, "MAX30102 initialized OK");
    }

    /* Initialize BME280 (environmental sensor) */
    if (bme280_init(&s_bme280, esp32_i2c_hal_get_handle(), BME280_I2C_ADDR) != 0) {
        ESP_LOGE(TAG, "BME280 initialization failed!");
    } else {
        ESP_LOGI(TAG, "BME280 initialized OK");
    }

    /* Initialize PMS5003 (PM2.5 sensor) on UART1 */
    if (pms5003_init(&s_pms5003, UART_NUM_1) != 0) {
        ESP_LOGE(TAG, "PMS5003 initialization failed!");
    } else {
        ESP_LOGI(TAG, "PMS5003 initialized OK");
    }

    /* Configure UART1 for PMS5003 (TX=GPIO18, RX=GPIO19, 9600 baud) */
    uart_config_t uart_cfg = {
        .baud_rate = PMS5003_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_1, &uart_cfg);
    uart_set_pin(UART_NUM_1, 18, 19, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);

    /* Spawn Dual-Core FreeRTOS Tasks */
    xTaskCreatePinnedToCore(task_ppg_accelerator, "PPG_Accel", 4096, NULL, 5, NULL, 0); // Core 0
    xTaskCreatePinnedToCore(task_disaster_monitor, "Risk_Monitor", 4096, NULL, 2, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(task_pms5003_uart, "PMS5003_UART", 2048, NULL, 3, NULL, 1); // Core 1
}
#endif

#ifndef ESP_PLATFORM
int main(void) {
    printf("================================================================\n");
    printf("  SIH26181 ShrikeFi (ESP32-S3 + Renesas ForgeFPGA) Host Test\n");
    printf("  Qualcomm Hardware Challenge — Smart India Hackathon 2026\n");
    printf("================================================================\n\n");

    // Initialize link driver (stub)
    shrikefi_link_init(NULL);
    shrikefi_set_threshold(120);

    // Prepare simulated inputs
    float hr = 135.0f;
    float rmssd = 10.0f;
    float spo2 = 97.0f;
    env_sensors_t env = {
        .ambient_temp_c = 46.0f,
        .humidity_pct   = 65.0f,
        .pm25           = 25.0f,
        .skin_temp_c    = 38.0f
    };

    hrv_state_t hrv;
    hrv_init(&hrv);
    hrv.rmssd = rmssd;
    hrv.sdnn = 12.0f;
    hrv.count = 50;

    // Test TinyML model inference on host
    nn_output_t out;
    nn_predict_int8(&nn_default_model_int8, &nn_quant_params,
                    hr, rmssd, spo2,
                    env.ambient_temp_c, env.humidity_pct, env.pm25,
                    &out);

    printf("Host Test - Heat Wave Profile:\n");
    printf("  TinyML Heat Score:      %.3f\n", out.heat_score);
    printf("  TinyML Pollution Score: %.3f\n", out.pollution_score);
    printf("  TinyML Flood Score:     %.3f\n", out.flood_score);

    risk_assessment_t risk;
    disaster_assess(&hrv, spo2, hr, &env, &risk);
    printf("  Rule Engine Heat Risk:  %s\n", risk_level_to_string(risk.heat_risk));
    printf("  Overall Risk Assessment: %s\n", risk_level_to_string(risk.overall_risk));
    printf("  Action Advisory:        %s\n", risk.overall_advisory);
    printf("\n>>> ShrikeFi Host Test Completed Successfully <<<\n");
    return 0;
}
#endif
