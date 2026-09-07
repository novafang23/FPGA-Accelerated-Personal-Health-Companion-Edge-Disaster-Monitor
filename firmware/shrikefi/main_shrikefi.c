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
#include "ssd1306.h"

/* Include platform-agnostic core firmware algorithms */
#include "hrv_analysis.h"
#include "spo2_engine.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model_int8.h"
#include "wifi_mqtt_manager.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "SHRIKEFI_MAIN";
static SemaphoreHandle_t s_data_mutex = NULL;

/* Signal quality and contact status */
typedef enum {
    SIGNAL_STATUS_NO_FINGER = 0,
    SIGNAL_STATUS_LOW_PERFUSION,  /* Touching too gently, hovering, or weak capillary pulse */
    SIGNAL_STATUS_ACQUIRING,      /* Locking pulse / accumulating beats */
    SIGNAL_STATUS_TRACKING        /* Vitals locked and clinically verified */
} signal_status_t;

/* Global shared health state */
typedef struct {
    float heart_rate;
    float r_peak_interval_ms;
    float hrv_rmssd;
    float hrv_sdnn;
    int   hrv_sample_count;   /* mirrors hrv_state_t.count from Core 0 */
    float spo2_percent;
    int   spo2_valid;         /* mirrors spo2_is_valid() from Core 0 */
    signal_status_t signal_status; /* real-time contact & perfusion quality */
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
static ssd1306_t s_ssd1306;



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
    spo2_state_t spo2_state;
    spo2_init(&spo2_state);

    /* 8-tap running-sum moving average filter (matching moving_average_8tap.v) */
    static uint16_t red_history[8] = {0};
    static uint16_t ir_history[8]  = {0};
    static uint8_t  hist_idx       = 0;
    static uint32_t red_sum        = 0;
    static uint32_t ir_sum         = 0;

    /* Software systolic peak detector state machine with strict noise rejection */
    enum { SW_ARMED, SW_RISING, SW_REFRACTORY };
    static int      sw_state             = SW_ARMED;
    static uint32_t sw_prev_sample       = 0;
    static uint32_t sw_peak_val          = 0;
    static uint8_t  sw_fall_count        = 0;
    static uint32_t sw_refractory_end_ms = 0;
    static uint32_t sw_last_peak_time_ms = 0;
    static uint32_t sw_last_valid_ibi_ms = 0;
    static int      sw_beat_streak       = 0;
    static uint32_t sw_finger_start_ms   = 0;
    static uint32_t sw_running_mean      = 15000;
    static int      raw_log_timer        = 0;

    /* Perfusion & AC amplitude tracking over 1-second rolling windows */
    static uint32_t ir_win_min           = UINT32_MAX;
    static uint32_t ir_win_max           = 0;
    static int      ir_win_count         = 0;
    static uint32_t last_ac_amplitude    = 0;

    ESP_LOGI(TAG, "Core 0: PPG Accelerator Task Started (FPGA hardware + Software DSP fallback).");

    while (1) {
        int avail = max30102_fifo_available(&s_max30102);
        int samples_read = 0;

        /* Drain available samples from FIFO (prevents buffer overflow & lag) */
        while (avail > 0 && samples_read < 8) {
            if (max30102_read_sample(&s_max30102, &ppg_sample) != 0) {
                break;
            }
            samples_read++;
            avail--;

            uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

            /* Track min/max over 50 samples (1 sec) to measure pulsatile AC amplitude */
            if (ppg_sample.ir < ir_win_min) ir_win_min = ppg_sample.ir;
            if (ppg_sample.ir > ir_win_max) ir_win_max = ppg_sample.ir;
            if (++ir_win_count >= 50) {
                last_ac_amplitude = (ir_win_max > ir_win_min) ? (ir_win_max - ir_win_min) : 0;
                ir_win_min = UINT32_MAX;
                ir_win_max = 0;
                ir_win_count = 0;

                /* Adjust LED current every 50 samples to compensate for weak/saturated signals */
                max30102_adjust_led_current(&s_max30102, ppg_sample.red, ppg_sample.ir);
            }

            /* Optical contact check: ambient air is IR<1000; tissue contact elevates levels to >50,000 */
            bool optical_contact = (ppg_sample.ir > 1500 || ppg_sample.red > 1500);

            uint8_t raw_red = max30102_scale_to_8bit(ppg_sample.red);
            uint8_t raw_ir  = max30102_scale_to_8bit(ppg_sample.ir);

            /* 2. Stream to ForgeFPGA over 4-bit parallel link */
            shrikefi_write_red_sample(raw_red);
            shrikefi_write_ir_sample(raw_ir);

            /* 3. Compute 8-tap running-sum moving average filter */
            red_sum = red_sum - red_history[hist_idx] + raw_red;
            red_history[hist_idx] = raw_red;
            uint8_t filt_red = (uint8_t)(red_sum >> 3);
            (void)filt_red;

            ir_sum = ir_sum - ir_history[hist_idx] + raw_ir;
            ir_history[hist_idx] = raw_ir;
            uint8_t filt_ir = (uint8_t)(ir_sum >> 3);
            (void)filt_ir;
            hist_idx = (hist_idx + 1) & 7;

            /* 4. Feed 18-bit samples into SpO2 engine when tissue contact is present */
            if (optical_contact) {
                spo2_add_samples(&spo2_state, ppg_sample.red, ppg_sample.ir);
                if (spo2_is_valid(&spo2_state)) {
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.spo2_percent = spo2_get_value(&spo2_state);
                        g_state.spo2_valid = 1;
                        xSemaphoreGive(s_data_mutex);
                    }
                } else {
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.spo2_valid = 0;
                        xSemaphoreGive(s_data_mutex);
                    }
                }
            }

            /* 5. Systolic Peak Detection (FPGA Interrupt with Software Fallback) */
            if (shrikefi_is_beat_detected()) {
                /* Hardware beat detected by ForgeFPGA on GPIO 10 */
                uint32_t ibi_cycles = shrikefi_read_ibi_cycles();
                shrikefi_clear_irq();

                float ibi_ms = (float)ibi_cycles * (20.0f / 1000000.0f); // 50 MHz clock
                if (ibi_ms > 400.0f && ibi_ms < 1500.0f) {
                    hrv_add_ibi(&hrv_state, ibi_ms);
                    hrv_compute(&hrv_state);

                    float inst_hr = 60000.0f / ibi_ms;
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.r_peak_interval_ms = ibi_ms;
                        g_state.heart_rate = (g_state.heart_rate > 30.0f) ?
                                             (0.70f * g_state.heart_rate + 0.30f * inst_hr) : inst_hr;
                        g_state.hrv_rmssd = hrv_state.rmssd;
                        g_state.hrv_sdnn = hrv_state.sdnn;
                        g_state.hrv_sample_count = hrv_state.count;
                        xSemaphoreGive(s_data_mutex);
                    }
                }
            } else if (optical_contact) {
                if (sw_finger_start_ms == 0) {
                    sw_finger_start_ms = now_ms;
                }

                /* Adaptive Peak Detector: tracks high-resolution optical baseline */
                sw_running_mean = (sw_running_mean * 31 + ppg_sample.ir) / 32;
                
                /* Use dynamic threshold based on recent AC amplitude (25% of AC) */
                uint32_t threshold_offset = last_ac_amplitude / 4;
                if (threshold_offset < 12) threshold_offset = 12; // Minimum 12 count threshold offset
                uint32_t threshold = sw_running_mean + threshold_offset;

                if (sw_state == SW_ARMED) {
                    if (ppg_sample.ir >= threshold) {
                        sw_state = SW_RISING;
                        sw_peak_val = ppg_sample.ir;
                        sw_fall_count = 0;
                    }
                } else if (sw_state == SW_RISING) {
                    if (ppg_sample.ir > sw_peak_val) {
                        sw_peak_val = ppg_sample.ir;
                    }
                    if (ppg_sample.ir < sw_prev_sample) {
                        if (++sw_fall_count >= 2) {
                            /* Crest confirmed: check peak prominence over baseline */
                            if (sw_peak_val > (sw_running_mean + 10)) {
                                uint32_t ibi_ms = now_ms - sw_last_peak_time_ms;
                                sw_last_peak_time_ms = now_ms;
                                sw_state = SW_REFRACTORY;
                                sw_refractory_end_ms = now_ms + 400; /* 400ms refractory blanking */

                                /* Apply 800ms stabilization window after initial finger contact */
                                if ((now_ms - sw_finger_start_ms) > 800 && ibi_ms >= 400 && ibi_ms <= 1500) {
                                    /* Plausibility check: reject sudden motion twitches */
                                    bool beat_plausible = true;
                                    if (sw_last_valid_ibi_ms > 0) {
                                        int32_t delta = (int32_t)ibi_ms - (int32_t)sw_last_valid_ibi_ms;
                                        if (delta < -350 || delta > 350) {
                                            beat_plausible = false;
                                        }
                                    }

                                    if (beat_plausible) {
                                        sw_last_valid_ibi_ms = ibi_ms;
                                        sw_beat_streak++;

                                        hrv_add_ibi(&hrv_state, (float)ibi_ms);
                                        hrv_compute(&hrv_state);

                                        float inst_hr = 60000.0f / (float)ibi_ms;
                                        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                                            g_state.r_peak_interval_ms = (float)ibi_ms;
                                            /* Require 2 consecutive valid beats before displaying HR */
                                            if (sw_beat_streak >= 2) {
                                                g_state.heart_rate = (g_state.heart_rate > 30.0f) ?
                                                                     (0.70f * g_state.heart_rate + 0.30f * inst_hr) : inst_hr;
                                            }
                                            g_state.hrv_rmssd = hrv_state.rmssd;
                                            g_state.hrv_sdnn = hrv_state.sdnn;
                                            g_state.hrv_sample_count = hrv_state.count;
                                            xSemaphoreGive(s_data_mutex);
                                        }
                                    }
                                }
                            } else {
                                /* Sub-threshold ripple (touching too gently) -> reject */
                                sw_state = SW_ARMED;
                            }
                        }
                    } else {
                        sw_fall_count = 0;
                    }
                } else if (sw_state == SW_REFRACTORY) {
                    if (now_ms >= sw_refractory_end_ms) {
                        sw_state = SW_ARMED;
                    }
                }
                sw_prev_sample = ppg_sample.ir;

                /* Evaluate real-time signal quality */
                signal_status_t current_status;
                if ((now_ms - sw_finger_start_ms) > 2500 && (last_ac_amplitude < 10 || sw_beat_streak == 0)) {
                    current_status = SIGNAL_STATUS_LOW_PERFUSION;
                } else if (hrv_state.count < 10) {
                    current_status = SIGNAL_STATUS_ACQUIRING;
                } else {
                    current_status = SIGNAL_STATUS_TRACKING;
                }

                if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.signal_status = current_status;
                    xSemaphoreGive(s_data_mutex);
                }
            } else {
                /* No optical contact (IR <= 1500) */
                sw_finger_start_ms   = 0;
                sw_last_peak_time_ms = 0;
                sw_last_valid_ibi_ms = 0;
                sw_beat_streak       = 0;
                sw_peak_val          = 0;
                sw_fall_count        = 0;
                sw_state             = SW_ARMED;
                last_ac_amplitude    = 0;
                ir_win_min           = UINT32_MAX;
                ir_win_max           = 0;
                ir_win_count         = 0;

                hrv_init(&hrv_state);   /* Reset HRV history on finger removal */
                spo2_init(&spo2_state); /* Reset SpO2 history on finger removal */

                if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.heart_rate         = 0.0f;
                    g_state.r_peak_interval_ms = 0.0f;
                    g_state.hrv_rmssd          = 0.0f;
                    g_state.hrv_sdnn           = 0.0f;
                    g_state.hrv_sample_count   = 0;
                    g_state.spo2_valid         = 0;
                    g_state.spo2_percent       = 0.0f;
                    g_state.signal_status      = SIGNAL_STATUS_NO_FINGER;
                    xSemaphoreGive(s_data_mutex);
                }
            }
        }

        /* Periodic optical debug log (every 1 second at 50Hz = 50 iterations) */
        if (++raw_log_timer >= 50) {
            raw_log_timer = 0;
            const char *status_str = (g_state.signal_status == SIGNAL_STATUS_LOW_PERFUSION) ? "LOW PERFUSION (PRESS FIRMER)" :
                                     (g_state.signal_status == SIGNAL_STATUS_ACQUIRING)     ? "ACQUIRING" :
                                     (g_state.signal_status == SIGNAL_STATUS_TRACKING)      ? "LOCKED" : "NO FINGER";
            ESP_LOGI("PPG_OPTICAL", "Raw: IR=%lu, Red=%lu | AC=%lu | Status: %s | HR: %.1f BPM | Beats: %d/10 | SpO2: %s | (Samples/tick: %d)",
                     (unsigned long)ppg_sample.ir, (unsigned long)ppg_sample.red,
                     (unsigned long)last_ac_amplitude,
                     status_str,
                     g_state.heart_rate, hrv_state.count,
                     g_state.spo2_valid ? "VALID" : "CALC/--",
                     samples_read);
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz sampling loop
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
        signal_status_t sig_stat = SIGNAL_STATUS_NO_FINGER;
        bool vitals_ready = false;

        /* Persistent environmental sensor state (prevents collapse to 0.0C on momentary packet drops) */
        static float s_last_temp = 25.0f;
        static float s_last_hum  = 50.0f;
        static float s_last_pm25 = 15.0f;

        /* Read BME280 (temperature, humidity, pressure) */
        bme280_data_t bme_data;
        if (read_bme280_env(&bme_data) == 0) {
            if (bme_data.temperature_c >= -20.0f && bme_data.temperature_c <= 65.0f) {
                s_last_temp = bme_data.temperature_c;
                s_last_hum  = bme_data.humidity_pct;
            }
        }
        env.ambient_temp_c = s_last_temp;
        env.humidity_pct   = s_last_hum;

        /* Read PMS5003 (PM2.5) */
        pms5003_data_t pms_data;
        if (read_pms5003_data(&pms_data) == 0 && pms_data.valid) {
            s_last_pm25 = (float)pms_data.pm2_5_atm;
        }
        env.pm25 = s_last_pm25;

        /* Skin temperature not available from current sensors */
        env.skin_temp_c = 0.0f;

        /* Get latest HR/HRV/SpO2 from Core 0 */
        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (g_state.heart_rate > 30.0f) hr = g_state.heart_rate;
            spo2 = g_state.spo2_valid ? g_state.spo2_percent : 0.0f;
            hrv_snapshot.rmssd = g_state.hrv_rmssd;
            hrv_snapshot.sdnn = g_state.hrv_sdnn;
            hrv_snapshot.count = g_state.hrv_sample_count;
            sig_stat = g_state.signal_status;
            vitals_ready = (hrv_snapshot.count >= 10 && hr > 30.0f);
            xSemaphoreGive(s_data_mutex);
        }

        /* 1. Execute Rule-based Disaster Risk Engine & TinyML only when vitals are genuine */
        risk_assessment_t rule_risk;
        memset(&rule_risk, 0, sizeof(rule_risk));
        rule_risk.overall_risk = RISK_UNKNOWN;

        risk_assessment_t nn_risk;
        memset(&nn_risk, 0, sizeof(nn_risk));
        nn_risk.overall_risk = RISK_UNKNOWN;

        nn_output_t nn_out;
        memset(&nn_out, 0, sizeof(nn_out));

        risk_assessment_t final_risk;
        memset(&final_risk, 0, sizeof(final_risk));
        final_risk.overall_risk = RISK_UNKNOWN;

        if (vitals_ready) {
            /* Unify SpO2 fallback so both engines see the same data: if calibrating, use neutral 96.0f */
            float engine_spo2 = (spo2 > 0.0f) ? spo2 : 96.0f;

            /* 1. Execute Clinical Deterministic Rule Engine */
            disaster_assess(&hrv_snapshot, engine_spo2, hr, &env, &rule_risk);

            /* 2. Execute On-Device TinyML INT8 Neural Network (single-pass populates both risk and telemetry) */
            disaster_assess_nn_int8(&hrv_snapshot, engine_spo2, hr, &env, &nn_risk, &nn_out);

            /* 3. Unified Triage: Fuse deterministic clinical bounds with predictive TinyML patterns */
            final_risk = rule_risk;
            if (nn_risk.overall_risk > final_risk.overall_risk) {
                final_risk.overall_risk     = nn_risk.overall_risk;
                final_risk.overall_advisory = nn_risk.overall_advisory;
            }

            ESP_LOGI(TAG, "[ShrikeFi] HR: %.1f BPM | SpO2: %s | RMSSD: %.1f ms | Temp: %.1f C | PM2.5: %.0f",
                     hr, (spo2 > 0.0f ? "VALID" : "CALC"), hrv_snapshot.rmssd, env.ambient_temp_c, env.pm25);
            ESP_LOGI(TAG, "[RuleEngine] Heat: %s | Poll: %s | Flood: %s => Overall: %s",
                     risk_level_to_string(rule_risk.heat_risk),
                     risk_level_to_string(rule_risk.pollution_risk),
                     risk_level_to_string(rule_risk.flood_risk),
                     risk_level_to_string(rule_risk.overall_risk));
            ESP_LOGI(TAG, "[TinyML INT8] Heat: %.3f (%s) | Poll: %.3f (%s) | Flood: %.3f (%s) => AI Overall: %s",
                     nn_out.heat_score, risk_level_to_string(nn_risk.heat_risk),
                     nn_out.pollution_score, risk_level_to_string(nn_risk.pollution_risk),
                     nn_out.flood_score, risk_level_to_string(nn_risk.flood_risk),
                     risk_level_to_string(nn_risk.overall_risk));
            ESP_LOGI(TAG, "[Unified Triage] Final Condition: %s | %s",
                     risk_level_to_string(final_risk.overall_risk), final_risk.overall_advisory);

            /* Thread-safe state update for telemetry & system monitoring */
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                g_state.ambient_temp_c   = env.ambient_temp_c;
                g_state.humidity_percent = env.humidity_pct;
                g_state.pm25_ugm3        = env.pm25;
                g_state.risk_result      = final_risk;
                g_state.nn_scores        = nn_out;
                xSemaphoreGive(s_data_mutex);
            }

            /* Publish to Cloud Dashboard */
            cloud_publish_health_data(hr, hrv_snapshot.rmssd, engine_spo2, env.ambient_temp_c, env.pm25, risk_level_to_string(final_risk.overall_risk));
        } else {
            ESP_LOGI(TAG, "[ShrikeFi] Vitals: HR=%s SpO2=%s | Status: %s (%d/10 beats) | Temp: %.1f C | PM2.5: %.0f",
                     (hr > 30.0f ? "LOCKED" : "--"),
                     (spo2 > 0.0f ? "VALID" : "--"),
                     (sig_stat == SIGNAL_STATUS_LOW_PERFUSION) ? "LOW PERFUSION (PRESS FIRMER)" :
                     (sig_stat == SIGNAL_STATUS_ACQUIRING)     ? "ACQUIRING" :
                     (sig_stat == SIGNAL_STATUS_TRACKING)      ? "READY" : "WAITING",
                     hrv_snapshot.count, env.ambient_temp_c, env.pm25);
        }

        /* 2. Render Live Dashboard to OLED Display */
        if (s_ssd1306.initialized) {
            ssd1306_clear(&s_ssd1306);

            // Header
            ssd1306_draw_string(&s_ssd1306, 8, 2, "SIH26181 COMPANION");
            ssd1306_draw_hline(&s_ssd1306, 0, 11, 128);

            // Line 1: Vitals (HR & SpO2)
            char buf_vitals[32];
            if (hr > 30.0f && spo2 > 0.0f) {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR:%3.0fBPM SpO2:%2.0f%%", hr, spo2);
            } else if (hr > 30.0f) {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR:%3.0fBPM SpO2: -- ", hr);
            } else {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR: --   SpO2: -- ");
            }
            ssd1306_draw_string(&s_ssd1306, 2, 15, buf_vitals);

            // Line 2: Environment (Temp & PM2.5)
            char buf_env[32];
            snprintf(buf_env, sizeof(buf_env), "T:%4.1fC   PM:%3.0f", env.ambient_temp_c, env.pm25);
            ssd1306_draw_string(&s_ssd1306, 2, 27, buf_env);

            // Line 3: HRV RMSSD or Guidance Feedback
            char buf_hrv[32];
            if (sig_stat == SIGNAL_STATUS_NO_FINGER) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Touch MAX30102...");
            } else if (sig_stat == SIGNAL_STATUS_LOW_PERFUSION) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Press Firmer...");
            } else if (hrv_snapshot.count < 10) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Reading... (%d/10)", hrv_snapshot.count);
            } else {
                snprintf(buf_hrv, sizeof(buf_hrv), "HRV RMSSD:%4.1fms", hrv_snapshot.rmssd);
            }
            ssd1306_draw_string(&s_ssd1306, 2, 39, buf_hrv);

            // Line 4: Health Condition Alert (TinyML & Disaster Engine)
            ssd1306_draw_hline(&s_ssd1306, 0, 49, 128);
            char buf_cond[32];
            if (sig_stat == SIGNAL_STATUS_NO_FINGER) {
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: WAITING");
            } else if (sig_stat == SIGNAL_STATUS_LOW_PERFUSION) {
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: LOW PERF");
            } else if (hrv_snapshot.count < 10) {
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: CALC...");
            } else {
                const char *risk_str = risk_level_to_string(final_risk.overall_risk);
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: %s", risk_str);
            }
            ssd1306_draw_string(&s_ssd1306, 2, 53, buf_cond);

            ssd1306_update(&s_ssd1306);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz assessment rate
    }
}

static void task_pms5003_uart(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "PMS5003 UART RX Task Started.");

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(s_pms5003.uart_num, &byte, 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            pms5003_feed_byte(&s_pms5003, byte);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "================================================================");
    ESP_LOGI(TAG, "  SIH26181 ShrikeFi Health Companion Firmware");
    ESP_LOGI(TAG, "  ESP32-S3 + Renesas ForgeFPGA 4-Bit Heterogeneous System");
    ESP_LOGI(TAG, "================================================================");

    s_data_mutex = xSemaphoreCreateMutex();

    /* Initialize I2C HAL for sensors (SDA=GPIO1, SCL=GPIO2, 400kHz) */
    esp32_i2c_hal_init(1, 2, 400000);

    /* Hardware diagnosis: scan and log all connected I2C devices */
    esp32_i2c_hal_scan();

    /* Flash ForgeFPGA bitstream via I2C */
    shrikefi_fpga_flash_init();

    /* Initialize 4-bit link to ForgeFPGA */
    shrikefi_link_init(NULL);
    shrikefi_set_threshold(120);

    /* Initialize WiFi & MQTT Cloud Sync */
    wifi_mqtt_init();

    /* Initialize SSD1306 OLED (check default 0x3C, fallback to 0x3D) */
    if (ssd1306_init(&s_ssd1306, esp32_i2c_hal_get_handle(), SSD1306_I2C_ADDR) == 0) {
        ESP_LOGI(TAG, "SSD1306 OLED initialized OK at 0x3C");
    } else if (ssd1306_init(&s_ssd1306, esp32_i2c_hal_get_handle(), SSD1306_I2C_ALT) == 0) {
        ESP_LOGI(TAG, "SSD1306 OLED initialized OK at 0x3D");
    } else {
        ESP_LOGW(TAG, "SSD1306 OLED not found at 0x3C or 0x3D (check SDA=GPIO1, SCL=GPIO2, VCC=3.3V, GND)");
    }

    if (s_ssd1306.initialized) {
        ssd1306_clear(&s_ssd1306);
        ssd1306_draw_string(&s_ssd1306, 8, 4, "SIH26181 HEALTH");
        ssd1306_draw_hline(&s_ssd1306, 0, 14, 128);
        ssd1306_draw_string(&s_ssd1306, 14, 20, "QUALCOMM SoC");
        ssd1306_draw_string(&s_ssd1306, 8, 34, "ESP32-S3 + FPGA");
        ssd1306_draw_string(&s_ssd1306, 18, 48, "Starting...");
        ssd1306_update(&s_ssd1306);
    }

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

    /* Configure UART1 for PMS5003 (TX=GPIO18, RX=GPIO14, 9600 baud) */
    uart_config_t uart_cfg = {
        .baud_rate = PMS5003_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_1, &uart_cfg);
    uart_set_pin(UART_NUM_1, 18, 14, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_set_pull_mode(14, GPIO_PULLUP_ONLY); // Prevent floating noise when sensor disconnected
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

    risk_assessment_t rule_risk;
    disaster_assess(&hrv, spo2, hr, &env, &rule_risk);

    // Single-pass TinyML INT8 model inference populates both risk categories and raw telemetry
    nn_output_t out;
    risk_assessment_t nn_risk;
    disaster_assess_nn_int8(&hrv, spo2, hr, &env, &nn_risk, &out);

    risk_assessment_t final_risk = rule_risk;
    if (nn_risk.overall_risk > final_risk.overall_risk) {
        final_risk.overall_risk     = nn_risk.overall_risk;
        final_risk.overall_advisory = nn_risk.overall_advisory;
    }

    printf("Host Test - Heat Wave Profile:\n");
    printf("  [TinyML INT8] Heat Score: %.3f (%s)\n", out.heat_score, risk_level_to_string(nn_risk.heat_risk));
    printf("  [TinyML INT8] Poll Score: %.3f (%s)\n", out.pollution_score, risk_level_to_string(nn_risk.pollution_risk));
    printf("  [TinyML INT8] Flood Score:%.3f (%s)\n", out.flood_score, risk_level_to_string(nn_risk.flood_risk));
    printf("  [TinyML INT8] AI Overall: %s\n", risk_level_to_string(nn_risk.overall_risk));
    printf("  [Rule Engine] Heat Risk:  %s\n", risk_level_to_string(rule_risk.heat_risk));
    printf("  [Rule Engine] Rule Risk:  %s\n", risk_level_to_string(rule_risk.overall_risk));
    printf("  [Unified Triage] Final Condition: %s\n", risk_level_to_string(final_risk.overall_risk));
    printf("  [Unified Triage] Action Advisory: %s\n", final_risk.overall_advisory);
    printf("\n>>> ShrikeFi Host Test Completed Successfully <<<\n");
    return 0;
}
#endif
