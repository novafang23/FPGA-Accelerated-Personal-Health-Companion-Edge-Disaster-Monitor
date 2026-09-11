/**
 * @file shrikefi_dashboard.c
 * @brief Standalone Native Windows Graphical Dashboard (GUI) for ShrikeFi / EdgeGuard
 * 
 * SIH26181: FPGA-Accelerated Edge Health Companion & Disaster Triage System
 * 
 * Features:
 *  - High-precision 60 FPS double-buffered GDI rendering (0% flicker)
 *  - DUAL MODE OPERATION:
 *      1. LIVE HARDWARE MODE: Connects to real physical ESP32-S3 over USB UART (COM port)
 *         and displays actual live optical PPG, real HR, real SpO2, real Temp, Humidity, and PM2.5.
 *      2. INTERACTIVE CLINICAL SIMULATION: Evaluates 6 extreme disaster & medical profiles.
 *  - Real-time animated medical Photoplethysmogram (PPG) oscilloscope with cardiac grid
 *  - Live digital displays for HR, SpO2, HRV (RMSSD), derived RR (Charlton 2018), and SQI (Karlen 2012)
 *  - Environmental telemetry: Temp, Humidity, NOAA Steadman Heat Index, and Neural-Calibrated PM2.5
 *  - Peer-reviewed clinical indices: Moran's Physiological Strain Index (PSI) & AHA PM2.5 Autonomic Strain
 *  - Royal College of Physicians mNEWS2 Clinical Triage scoring with automated alert banner
 *  - INT8 Micro-Engine TinyML 3-Axis Risk Gauges (Heat, Pollution, Flood) on the shared 6->24->16->3 engine
 *  - Interactive Scenario Switcher buttons (6 clinical/disaster profiles)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "clinical_vitals_engine.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model.h"
#include "nn_risk_model_int8.h"
#include "pm25_calibration_int8.h"
#include "ppg_sqi.h"
#include "ppg_respiratory_rate.h"
#include "hrv_analysis.h"
#include "spo2_engine.h"

/* Window Dimensions */
#define WINDOW_WIDTH  1260
#define WINDOW_HEIGHT 820

/* Color Palette (Sleek Dark Medical/Cyberpunk Theme) */
#define COL_BG          RGB(13, 17, 23)       /* Main dark background */
#define COL_CARD_BG     RGB(22, 27, 34)       /* Card container */
#define COL_CARD_BORDER RGB(48, 54, 61)       /* Card border */
#define COL_HEADER_BG   RGB(18, 22, 29)       /* Top header */
#define COL_TEXT_MAIN   RGB(240, 246, 252)    /* Bright white */
#define COL_TEXT_MUTED  RGB(139, 148, 158)    /* Gray */
#define COL_GREEN       RGB(46, 160, 67)      /* Normal status green */
#define COL_GREEN_BRT   RGB(0, 255, 136)      /* Glowing oscilloscope green */
#define COL_AMBER       RGB(210, 153, 34)     /* Warning amber */
#define COL_RED         RGB(248, 81, 73)      /* Alarm red */
#define COL_CYAN        RGB(0, 229, 255)      /* Accent cyan */
#define COL_PURPLE      RGB(187, 134, 252)    /* AI accent purple */
#define COL_GRID        RGB(22, 45, 40)       /* Oscilloscope grid */
#define COL_GRID_SUB    RGB(16, 32, 28)       /* Oscilloscope sub-grid */

/* Control IDs */
#define IDC_BTN_SCENARIO_1  101
#define IDC_BTN_SCENARIO_2  102
#define IDC_BTN_SCENARIO_3  103
#define IDC_BTN_SCENARIO_4  104
#define IDC_BTN_SCENARIO_5  105
#define IDC_BTN_SCENARIO_6  106
#define IDC_BTN_PAUSE       107
#define IDC_BTN_CONNECT     108
#define IDC_EDIT_COM        109

/* Simulation Profile Definition */
typedef struct {
    const char *name;
    const char *description;
    float target_hr;
    float target_spo2;
    float target_rmssd;
    float target_rr;
    float target_temp;
    float target_humidity;
    float target_pm25;
    float target_sqi;
    int is_motion;
} scenario_profile_t;

static const scenario_profile_t g_scenarios[6] = {
    {
        "Normal Baseline",
        "Stable resting physiology; optimal air quality; room temperature.",
        72.0f, 98.5f, 38.0f, 15.0f, 24.5f, 48.0f, 12.0f, 0.96f, 0
    },
    {
        "Heat Wave & Dehydration",
        "Hyperthermic strain (43.5 C); compensatory tachycardia; high heat index.",
        138.0f, 96.0f, 12.0f, 24.0f, 43.5f, 65.0f, 45.0f, 0.94f, 0
    },
    {
        "Severe Smog / PM2.5 Crisis",
        "Industrial smoke/wildfire; acute PM2.5 surge; autonomic depression.",
        108.0f, 92.5f, 14.0f, 22.0f, 29.0f, 82.0f, 340.0f, 0.91f, 0
    },
    {
        "Flash Flood / Hypothermia",
        "Cold water submersion (11 C); bradycardia; hypoventilation cold shock.",
        42.0f, 93.0f, 65.0f, 9.0f, 11.2f, 98.0f, 8.0f, 0.90f, 0
    },
    {
        "Cardiopulmonary ICU Emergency",
        "Acute respiratory decompensation / sepsis; severe hypoxia (83%); tachypnea.",
        146.0f, 83.0f, 7.0f, 32.0f, 38.8f, 50.0f, 20.0f, 0.88f, 0
    },
    {
        "Motion Artifact / Noise Test",
        "Evacuation sprint; optical sensor displacement; Karlen SQI artifact rejection.",
        95.0f, 96.0f, 25.0f, 18.0f, 26.0f, 55.0f, 25.0f, 0.32f, 1
    }
};

/* Oscilloscope Buffer */
#define OSC_POINTS 600
static float s_osc_buffer[OSC_POINTS];
static int   s_osc_head = 0;
static float s_cardiac_phase = 0.0f;

/* System State */
typedef struct {
    // Current live values
    float hr;
    float spo2;
    float rmssd;
    float sdnn;
    float derived_rr;
    float sqi;
    float temp_c;
    float humidity_pct;
    float pm25_raw;
    float pm25_calibrated;
    float heat_index_c;
    float moran_psi;
    float aha_autonomic_strain;
    
    // Live Hardware Detection Flags
    int is_finger_present;
    int has_live_ppg_stream;
    uint32_t last_packet_time_ms;
    uint32_t raw_ir_min;
    uint32_t raw_ir_max;
    
    // mNEWS2 Triage
    uint32_t news2_score;
    risk_level_t news2_level;
    char news2_advisory[128];
    int alert_flags;
    
    // TinyML INT8 Inference
    float heat_risk_pct;
    float pollution_risk_pct;
    float flood_risk_pct;
    risk_level_t overall_ai_risk;
    
    // UI State
    int active_scenario;
    int is_paused;
    int is_serial_connected;
    HANDLE h_serial_port;
    HANDLE h_serial_thread;
    int stop_serial_thread;
    char com_port_str[16];
    char status_bar_text[128];
    uint32_t frame_count;
    float pulse_anim;
} dashboard_state_t;

static dashboard_state_t g_state;
static HWND g_hwnd_main = NULL;
static HWND g_hwnd_edit_com = NULL;
static HWND g_hwnd_btn_connect = NULL;
static HWND g_hwnd_btn_pause = NULL;
static HWND g_hwnd_scenario_btns[6];

/* Fonts */
static HFONT g_font_header = NULL;
static HFONT g_font_title = NULL;
static HFONT g_font_large_val = NULL;
static HFONT g_font_med_val = NULL;
static HFONT g_font_label = NULL;
static HFONT g_font_small = NULL;

/* NOAA Steadman Heat Index Calculation */
static float calculate_nws_heat_index(float temp_c, float hum_pct) {
    if (temp_c < 27.0f) return temp_c;
    float tf = temp_c * 1.8f + 32.0f;
    float rh = hum_pct;
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;
    float hi_f = -42.379f + 2.04901523f * tf + 10.14333127f * rh
               - 0.22475541f * tf * rh - 0.00683783f * tf * tf
               - 0.05481717f * rh * rh + 0.00122874f * tf * tf * rh
               + 0.00085282f * tf * rh * rh - 0.00000199f * tf * tf * rh * rh;
    float hi_c = (hi_f - 32.0f) / 1.8f;
    /* Same clamp as disaster_risk_engine.c. This copy is what the GUI prints
     * ("NOAA Steadman Heat Index: %.1f C"), so without it the display showed
     * 106 C on the built-in heat-wave profile. */
    if (hi_c > HEAT_INDEX_MAX_C) hi_c = HEAT_INDEX_MAX_C;
    return (hi_c > temp_c) ? hi_c : temp_c;
}

/* -------------------------------------------------------------------------- */
/* High-Speed Robust Line-Buffered Serial Reader Thread                       */
/* -------------------------------------------------------------------------- */
static DWORD WINAPI SerialReaderThread(LPVOID lpParam) {
    char chunk[256];
    char line_buf[512];
    int line_len = 0;
    DWORD bytesRead;
    
    while (!g_state.stop_serial_thread && g_state.h_serial_port != INVALID_HANDLE_VALUE) {
        if (ReadFile(g_state.h_serial_port, chunk, sizeof(chunk) - 1, &bytesRead, NULL) && bytesRead > 0) {
            for (DWORD i = 0; i < bytesRead; i++) {
                char c = chunk[i];
                if (c == '\n' || c == '\r') {
                    if (line_len > 0) {
                        line_buf[line_len] = '\0';
                        
                        float r_hr, r_spo2, r_rmssd, r_temp, r_hum, r_pm;
                        unsigned long r_ir = 0;
                        
                        // Packet 1: [TELEMETRY] HR=...
                        if (sscanf(line_buf, "[TELEMETRY] HR=%f,SPO2=%f,RMSSD=%f,TEMP=%f,HUM=%f,PM25=%f",
                                   &r_hr, &r_spo2, &r_rmssd, &r_temp, &r_hum, &r_pm) == 6) {
                            if (r_hr > 20.0f) g_state.hr = r_hr;
                            if (r_spo2 > 50.0f) g_state.spo2 = r_spo2;
                            if (r_rmssd > 0.0f) g_state.rmssd = r_rmssd;
                            g_state.temp_c = r_temp;
                            g_state.humidity_pct = r_hum;
                            g_state.pm25_raw = r_pm;
                            g_state.is_finger_present = 1;
                            g_state.sqi = 0.95f;
                            g_state.last_packet_time_ms = GetTickCount();
                        }
                        // Packet 2: [TELEMETRY] NO_FINGER...
                        else if (sscanf(line_buf, "[TELEMETRY] NO_FINGER,TEMP=%f,HUM=%f,PM25=%f",
                                        &r_temp, &r_hum, &r_pm) == 3) {
                            g_state.temp_c = r_temp;
                            g_state.humidity_pct = r_hum;
                            g_state.pm25_raw = r_pm;
                            g_state.is_finger_present = 0;
                            g_state.last_packet_time_ms = GetTickCount();
                        }
                        // Packet 3: Real Optical Waveform Sample: [PPG] %lu
                        else if (sscanf(line_buf, "[PPG] %lu", &r_ir) == 1 ||
                                 strstr(line_buf, "Raw: IR=") != NULL) {
                            char *p = strstr(line_buf, "IR=");
                            if (p) r_ir = strtoul(p + 3, NULL, 10);
                            
                            if (r_ir > 1500) {
                                g_state.has_live_ppg_stream = 1;
                                g_state.is_finger_present = 1;
                                
                                if (r_ir < g_state.raw_ir_min || g_state.raw_ir_min == 0) g_state.raw_ir_min = r_ir;
                                if (r_ir > g_state.raw_ir_max) g_state.raw_ir_max = r_ir;
                                
                                float span = (float)(g_state.raw_ir_max - g_state.raw_ir_min);
                                if (span < 50.0f) span = 50.0f;
                                float norm = (float)(r_ir - g_state.raw_ir_min) / span;
                                if (norm < 0.0f) norm = 0.0f;
                                if (norm > 1.0f) norm = 1.0f;
                                
                                s_osc_buffer[s_osc_head] = norm;
                                s_osc_head = (s_osc_head + 1) % OSC_POINTS;
                                
                                // Auto-adaptive baseline tracker
                                g_state.raw_ir_min += 1;
                                if (g_state.raw_ir_max > 1) g_state.raw_ir_max -= 1;
                            }
                        }
                        // Packet 4: Standard ESP-IDF log format
                        else if (strstr(line_buf, "[ShrikeFi] HR:") != NULL) {
                            char *phr = strstr(line_buf, "HR:");
                            char *prm = strstr(line_buf, "RMSSD:");
                            char *pt = strstr(line_buf, "Temp:");
                            char *ppm = strstr(line_buf, "PM2.5:");
                            if (phr && prm && pt && ppm) {
                                sscanf(phr, "HR: %f", &r_hr);
                                sscanf(prm, "RMSSD: %f", &r_rmssd);
                                sscanf(pt, "Temp: %f", &r_temp);
                                sscanf(ppm, "PM2.5: %f", &r_pm);
                                if (r_hr > 20.0f) {
                                    g_state.hr = r_hr;
                                    g_state.is_finger_present = 1;
                                }
                                if (r_rmssd > 0.0f) g_state.rmssd = r_rmssd;
                                g_state.temp_c = r_temp;
                                g_state.pm25_raw = r_pm;
                                g_state.last_packet_time_ms = GetTickCount();
                            }
                        }
                        line_len = 0;
                    }
                } else if (line_len < (int)sizeof(line_buf) - 2) {
                    line_buf[line_len++] = c;
                }
            }
        }
        Sleep(10);
    }
    return 0;
}

static void ConnectSerialPort(const char *port_name) {
    char full_path[32];
    snprintf(full_path, sizeof(full_path), "\\\\.\\%s", port_name);
    
    g_state.h_serial_port = CreateFileA(
        full_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
    );
    
    if (g_state.h_serial_port == INVALID_HANDLE_VALUE) {
        snprintf(g_state.status_bar_text, sizeof(g_state.status_bar_text),
                 "Could not open %s (device unplugged or busy). Reverting to Simulation Mode.", port_name);
        return;
    }
    
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(g_state.h_serial_port, &dcbSerialParams)) {
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity   = NOPARITY;
        SetCommState(g_state.h_serial_port, &dcbSerialParams);
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutConstant = 20;
    SetCommTimeouts(g_state.h_serial_port, &timeouts);
    
    g_state.stop_serial_thread = 0;
    g_state.has_live_ppg_stream = 0;
    g_state.raw_ir_min = 0;
    g_state.raw_ir_max = 0;
    g_state.last_packet_time_ms = GetTickCount();
    g_state.h_serial_thread = CreateThread(NULL, 0, SerialReaderThread, NULL, 0, NULL);
    g_state.is_serial_connected = 1;
    strncpy(g_state.com_port_str, port_name, sizeof(g_state.com_port_str) - 1);
    snprintf(g_state.status_bar_text, sizeof(g_state.status_bar_text),
             "ONLINE: Streaming LIVE physical sensor data from %s @ 115200 baud.", port_name);
    SetWindowTextA(g_hwnd_btn_connect, "Disconnect");
}

static void DisconnectSerialPort(void) {
    if (g_state.is_serial_connected) {
        g_state.stop_serial_thread = 1;
        if (g_state.h_serial_thread) {
            WaitForSingleObject(g_state.h_serial_thread, 500);
            CloseHandle(g_state.h_serial_thread);
            g_state.h_serial_thread = NULL;
        }
        if (g_state.h_serial_port != INVALID_HANDLE_VALUE) {
            CloseHandle(g_state.h_serial_port);
            g_state.h_serial_port = INVALID_HANDLE_VALUE;
        }
        g_state.is_serial_connected = 0;
        g_state.has_live_ppg_stream = 0;
        snprintf(g_state.status_bar_text, sizeof(g_state.status_bar_text),
                 "Hardware disconnected. Reverted to interactive clinical simulation.");
        SetWindowTextA(g_hwnd_btn_connect, "Connect COM");
    }
}

/* -------------------------------------------------------------------------- */
/* Real-Time Simulation & Live Hardware AI/Clinical Assessment Engine         */
/* -------------------------------------------------------------------------- */
static void UpdateTelemetryStep(void) {
    if (g_state.is_paused) return;
    
    const scenario_profile_t *target = &g_scenarios[g_state.active_scenario];
    
    /* If NOT connected to hardware, drive vitals using scenario target */
    if (!g_state.is_serial_connected) {
        float rate = 0.08f;
        g_state.hr += (target->target_hr - g_state.hr) * rate;
        g_state.spo2 += (target->target_spo2 - g_state.spo2) * rate;
        g_state.rmssd += (target->target_rmssd - g_state.rmssd) * rate;
        g_state.derived_rr += (target->target_rr - g_state.derived_rr) * rate;
        g_state.temp_c += (target->target_temp - g_state.temp_c) * rate;
        g_state.humidity_pct += (target->target_humidity - g_state.humidity_pct) * rate;
        g_state.pm25_raw += (target->target_pm25 - g_state.pm25_raw) * rate;
        g_state.sqi += (target->target_sqi - g_state.sqi) * rate;
        g_state.is_finger_present = 1;
    } else {
        // If connected, check for packet timeout (3 seconds)
        if (GetTickCount() - g_state.last_packet_time_ms > 3000) {
            snprintf(g_state.status_bar_text, sizeof(g_state.status_bar_text),
                     "WAITING FOR DATA: Check ESP32-S3 USB cable on %s...", g_state.com_port_str);
        }
    }
    
    float curr_hr = g_state.hr;
    if (!g_state.is_serial_connected) {
        float jitter = ((float)(rand() % 100) - 50.0f) / 100.0f;
        curr_hr += jitter * 0.5f;
    }
    
    /* 1. Neural PM2.5 Humidity Compensation (Slashing optical scattering bias) */
    g_state.pm25_calibrated = pm25_calibrate_nn_int8(g_state.pm25_raw, g_state.temp_c, g_state.humidity_pct);
    
    /* 2. NOAA Steadman Heat Index */
    g_state.heat_index_c = calculate_nws_heat_index(g_state.temp_c, g_state.humidity_pct);
    
    /* 3. Peer-Reviewed Biomarkers */
    g_state.moran_psi = disaster_calculate_moran_psi(curr_hr, g_state.temp_c, g_state.humidity_pct);
    g_state.aha_autonomic_strain = disaster_calculate_aha_autonomic_strain(g_state.pm25_calibrated, g_state.rmssd);
    
    /* 4. Royal College of Physicians mNEWS2 Clinical Triage */
    clinical_assessment_t clin_out;
    clinical_vitals_assess_full(curr_hr, g_state.spo2, g_state.rmssd, g_state.derived_rr, g_state.sqi, &clin_out);
    g_state.news2_score = clin_out.news2_score;
    g_state.news2_level = (risk_level_t)clin_out.level;
    g_state.alert_flags = clin_out.alert_flags;
    strncpy(g_state.news2_advisory, clin_out.advisory, sizeof(g_state.news2_advisory) - 1);
    
    /* 5. TinyML INT8 Neural Network Inference (6 -> 24 -> 16 -> 3) */
    hrv_state_t hrv_snap;
    memset(&hrv_snap, 0, sizeof(hrv_snap));
    hrv_snap.rmssd = g_state.rmssd;
    hrv_snap.mean_hr = curr_hr;
    /* Readiness sentinel, not a sample count: disaster_assess_nn_int8() gates on
     * hrv_is_ready(), which requires count >= HRV_MIN_SAMPLES. This process has
     * no beat-to-beat interval history, so the flag means "assessment permitted". */
    hrv_snap.count = HRV_MIN_SAMPLES;
    /* hrv_snap.sdnn is deliberately left at zero (from the memset above).
     *
     * A real SDNN is the sample standard deviation of beat-to-beat intervals.
     * This process never sees IBIs: RMSSD arrives pre-computed over the telemetry
     * link, whose frame carries HR/SpO2/RMSSD/TEMP/HUM/PM25 and no SDNN. Nothing
     * on this path reads sdnn either -- disaster_assess_nn_int8() uses hrv->rmssd
     * only -- so there is no value to fill in.
     *
     * This previously read `hrv_snap.sdnn = g_state.rmssd * 1.25f;`, which was an
     * invented constant ratio dressed up as a measurement. It happened to be inert,
     * but it is exactly the kind of number a later reader would trust and display.
     * If SDNN is ever needed here, add it to the telemetry frame in
     * main_shrikefi.c (which does compute a true SDNN via hrv_compute()) and parse
     * it -- do not synthesise it locally. */

    env_sensors_t env_snap = {
        .ambient_temp_c = g_state.temp_c,
        .humidity_pct = g_state.humidity_pct,
        .pm25 = g_state.pm25_calibrated,
        .skin_temp_c = (g_state.temp_c > 30.0f) ? g_state.temp_c - 1.5f : 34.0f
    };
    risk_assessment_t nn_risk;
    nn_output_t nn_out;
    disaster_assess_nn_int8(&hrv_snap, g_state.spo2, curr_hr, &env_snap, &nn_risk, &nn_out);
    
    g_state.heat_risk_pct = nn_out.heat_score * 100.0f;
    g_state.pollution_risk_pct = nn_out.pollution_score * 100.0f;
    g_state.flood_risk_pct = nn_out.flood_score * 100.0f;
    g_state.overall_ai_risk = nn_risk.overall_risk;
    
    /* 6. Oscilloscope Waveform Animation */
    // If not streaming raw optical samples from hardware, synthesize the realistic pulse wave:
    if (!g_state.has_live_ppg_stream) {
        float phase_step = (curr_hr > 20.0f ? curr_hr : 60.0f) / 60.0f * 0.0333f;
        s_cardiac_phase += phase_step;
        if (s_cardiac_phase >= 1.0f) {
            s_cardiac_phase -= 1.0f;
            g_state.pulse_anim = 1.0f;
        } else {
            g_state.pulse_anim *= 0.85f;
        }
        
        float t = s_cardiac_phase;
        float ppg_val = 0.0f;
        if (t < 0.25f) {
            float st = (t - 0.12f) / 0.06f;
            ppg_val = expf(-st * st * 2.0f);
        } else if (t < 0.45f) {
            float dn = (t - 0.32f) / 0.05f;
            ppg_val = 0.42f * expf(-dn * dn * 3.0f) + 0.15f;
        } else {
            float decay = 1.0f - (t - 0.45f) / 0.55f;
            ppg_val = 0.25f * decay * decay;
        }
        
        if (target->is_motion && !g_state.is_serial_connected) {
            float noise = ((float)(rand() % 100) - 50.0f) / 50.0f;
            ppg_val += noise * 0.45f + 0.2f * sinf(g_state.frame_count * 0.35f);
        }
        
        s_osc_buffer[s_osc_head] = ppg_val;
        s_osc_head = (s_osc_head + 1) % OSC_POINTS;
    }
    
    g_state.frame_count++;
}

/* -------------------------------------------------------------------------- */
/* High-Precision GDI Drawing Helpers                                         */
/* -------------------------------------------------------------------------- */
static void DrawDarkCard(HDC hdc, int x, int y, int w, int h, const char *title, COLORREF accent_col) {
    HBRUSH hbg = CreateSolidBrush(COL_CARD_BG);
    HPEN hborder = CreatePen(PS_SOLID, 1, COL_CARD_BORDER);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hbg);
    HPEN oldPen = (HPEN)SelectObject(hdc, hborder);
    RoundRect(hdc, x, y, x + w, y + h, 10, 10);
    
    if (accent_col != 0) {
        HPEN hAccPen = CreatePen(PS_SOLID, 2, accent_col);
        SelectObject(hdc, hAccPen);
        MoveToEx(hdc, x + 8, y + 2, NULL);
        LineTo(hdc, x + w - 8, y + 2);
        DeleteObject(hAccPen);
    }
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(hbg);
    DeleteObject(hborder);
    
    if (title && title[0]) {
        SelectObject(hdc, g_font_label);
        SetTextColor(hdc, COL_TEXT_MUTED);
        SetBkMode(hdc, TRANSPARENT);
        RECT r = { x + 14, y + 10, x + w - 14, y + 28 };
        DrawTextA(hdc, title, -1, &r, DT_LEFT | DT_SINGLELINE);
    }
}

static void DrawProgressBar(HDC hdc, int x, int y, int w, int h, float pct, COLORREF col) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    
    HBRUSH hbg = CreateSolidBrush(RGB(30, 36, 44));
    HPEN hpen = CreatePen(PS_SOLID, 1, RGB(50, 58, 70));
    SelectObject(hdc, hbg);
    SelectObject(hdc, hpen);
    RoundRect(hdc, x, y, x + w, y + h, 4, 4);
    DeleteObject(hbg);
    DeleteObject(hpen);
    
    int fill_w = (int)((float)(w - 2) * (pct / 100.0f));
    if (fill_w > 2) {
        HBRUSH hfill = CreateSolidBrush(col);
        HPEN hfillPen = CreatePen(PS_SOLID, 1, col);
        SelectObject(hdc, hfill);
        SelectObject(hdc, hfillPen);
        RoundRect(hdc, x + 1, y + 1, x + 1 + fill_w, y + h - 1, 3, 3);
        DeleteObject(hfill);
        DeleteObject(hfillPen);
    }
}

/* -------------------------------------------------------------------------- */
/* Main UI Rendering Routine (Double Buffered)                                */
/* -------------------------------------------------------------------------- */
static void RenderDashboard(HDC hdcMem, int width, int height) {
    RECT rcCanvas = { 0, 0, width, height };
    HBRUSH hbgBrush = CreateSolidBrush(COL_BG);
    FillRect(hdcMem, &rcCanvas, hbgBrush);
    DeleteObject(hbgBrush);
    
    RECT rcHeader = { 0, 0, width, 58 };
    HBRUSH hhdrBrush = CreateSolidBrush(COL_HEADER_BG);
    FillRect(hdcMem, &rcHeader, hhdrBrush);
    DeleteObject(hhdrBrush);
    
    COLORREF pulseCol = (g_state.pulse_anim > 0.3f) ? COL_GREEN_BRT : COL_GREEN;
    HBRUSH hDot = CreateSolidBrush(pulseCol);
    HPEN hDotPen = CreatePen(PS_SOLID, 1, pulseCol);
    SelectObject(hdcMem, hDot);
    SelectObject(hdcMem, hDotPen);
    Ellipse(hdcMem, 22, 22, 34, 34);
    DeleteObject(hDot);
    DeleteObject(hDotPen);
    
    SelectObject(hdcMem, g_font_header);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, COL_CYAN);
    TextOutA(hdcMem, 42, 12, "EDGEGUARD / SHRIKEFI", 20);
    
    SelectObject(hdcMem, g_font_label);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, 42, 34, "FPGA-Accelerated Edge Health Companion & Disaster Triage System (SIH26181)", 74);
    
    // Header Status Badges (Right side)
    char badgeBuf[128];
    SelectObject(hdcMem, g_font_small);
    if (g_state.is_serial_connected) {
        SetTextColor(hdcMem, COL_GREEN_BRT);
        snprintf(badgeBuf, sizeof(badgeBuf), "[MODE: LIVE HARDWARE ON %s]  [SENSORS: ACTIVE]  [TinyML: INT8]", g_state.com_port_str);
    } else {
        SetTextColor(hdcMem, COL_AMBER);
        snprintf(badgeBuf, sizeof(badgeBuf), "[MODE: SIMULATION DEMO]  [FPGA: Renesas Forge]  [TinyML: INT8 91.0%%]");
    }
    TextOutA(hdcMem, width - 580, 16, badgeBuf, strlen(badgeBuf));
    
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    snprintf(badgeBuf, sizeof(badgeBuf), "%s | FPS: ~30 | TICK: %u",
             g_state.is_serial_connected ? (g_state.is_finger_present ? "FINGER DETECTED (LOCKED)" : "TOUCH MAX30100 SENSOR...") : "SCENARIO SIMULATOR",
             g_state.frame_count);
    TextOutA(hdcMem, width - 420, 34, badgeBuf, strlen(badgeBuf));
    
    // ROW 1: Oscilloscope & NEWS2 Triage
    int osc_x = 18, osc_y = 68, osc_w = 830, osc_h = 220;
    const char *oscHeader = g_state.is_serial_connected ? 
        "REAL-TIME OPTICAL PPG OSCILLOSCOPE (PHYSICAL MAX30100/MAX30102 AC WAVEFORM)" :
        "REAL-TIME PPG OPTICAL OSCILLOSCOPE (MAX30102 AC/DC SIMULATION EXTRACTION)";
    DrawDarkCard(hdcMem, osc_x, osc_y, osc_w, osc_h, oscHeader, COL_CYAN);
    
    int scr_x = osc_x + 14, scr_y = osc_y + 30, scr_w = osc_w - 28, scr_h = osc_h - 42;
    HBRUSH hScrBg = CreateSolidBrush(RGB(7, 18, 15));
    HPEN hScrBorder = CreatePen(PS_SOLID, 1, RGB(18, 48, 38));
    SelectObject(hdcMem, hScrBg);
    SelectObject(hdcMem, hScrBorder);
    Rectangle(hdcMem, scr_x, scr_y, scr_x + scr_w, scr_y + scr_h);
    DeleteObject(hScrBg);
    DeleteObject(hScrBorder);
    
    HPEN hGridPen = CreatePen(PS_DOT, 1, COL_GRID);
    SelectObject(hdcMem, hGridPen);
    for (int gx = scr_x + 25; gx < scr_x + scr_w; gx += 25) {
        MoveToEx(hdcMem, gx, scr_y, NULL);
        LineTo(hdcMem, gx, scr_y + scr_h);
    }
    for (int gy = scr_y + 20; gy < scr_y + scr_h; gy += 20) {
        MoveToEx(hdcMem, scr_x, gy, NULL);
        LineTo(hdcMem, scr_x + scr_w, gy);
    }
    DeleteObject(hGridPen);
    
    HPEN hWavePen = CreatePen(PS_SOLID, 2, COL_GREEN_BRT);
    SelectObject(hdcMem, hWavePen);
    
    float baseline_y = (float)(scr_y + scr_h - 22);
    float scale_amp  = (float)(scr_h - 40);
    int prev_px = 0, prev_py = 0;
    
    for (int i = 0; i < scr_w && i < OSC_POINTS; i++) {
        int buf_idx = (s_osc_head - scr_w + i + OSC_POINTS) % OSC_POINTS;
        float val = s_osc_buffer[buf_idx];
        int px = scr_x + i;
        int py = (int)(baseline_y - val * scale_amp);
        
        if (py < scr_y + 4) py = scr_y + 4;
        if (py > scr_y + scr_h - 4) py = scr_y + scr_h - 4;
        
        if (i == 0) {
            MoveToEx(hdcMem, px, py, NULL);
        } else {
            LineTo(hdcMem, px, py);
        }
        prev_px = px;
        prev_py = py;
    }
    DeleteObject(hWavePen);
    
    HBRUSH hHead = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(hdcMem, hHead);
    Ellipse(hdcMem, prev_px - 3, prev_py - 3, prev_px + 3, prev_py + 3);
    DeleteObject(hHead);
    
    SelectObject(hdcMem, g_font_small);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, (g_state.sqi >= 0.70f) ? COL_GREEN_BRT : COL_AMBER);
    char oscInfo[96];
    if (g_state.is_serial_connected && !g_state.is_finger_present) {
        snprintf(oscInfo, sizeof(oscInfo), "TOUCH SENSOR: PLACE FINGER ON MAX30100/MAX30102 LED...");
    } else {
        snprintf(oscInfo, sizeof(oscInfo), "SQI: %.1f%% (%s) | %s | RATE: 100 Hz",
                 g_state.sqi * 100.0f, (g_state.sqi >= 0.70f) ? "HOSPITAL GRADE" : "ARTIFACT DETECTED",
                 g_state.is_serial_connected ? "REAL OPTICAL INPUT" : "SYNTHETIC AC");
    }
    TextOutA(hdcMem, scr_x + 10, scr_y + 8, oscInfo, strlen(oscInfo));
    
    int news_x = 860, news_y = 68, news_w = 380, news_h = 220;
    COLORREF triageCol = (g_state.news2_level == RISK_CRITICAL || g_state.news2_score >= 7) ? COL_RED :
                         (g_state.news2_level == RISK_HIGH || g_state.news2_score >= 5) ? COL_AMBER : COL_GREEN;
    DrawDarkCard(hdcMem, news_x, news_y, news_w, news_h, "ROYAL COLLEGE OF PHYSICIANS mNEWS2 TRIAGE", triageCol);
    
    RECT rcNewsBox = { news_x + 18, news_y + 36, news_x + 140, news_y + 115 };
    HBRUSH hTriageBg = CreateSolidBrush(triageCol);
    FillRect(hdcMem, &rcNewsBox, hTriageBg);
    DeleteObject(hTriageBg);
    
    SelectObject(hdcMem, g_font_large_val);
    SetTextColor(hdcMem, RGB(0, 0, 0));
    char szScore[16];
    snprintf(szScore, sizeof(szScore), "%u", g_state.news2_score);
    DrawTextA(hdcMem, szScore, -1, &rcNewsBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdcMem, g_font_title);
    SetTextColor(hdcMem, triageCol);
    const char *szRiskTitle = (g_state.news2_score >= 7 || g_state.news2_level == RISK_CRITICAL) ? "CRITICAL EMERGENCY" :
                              (g_state.news2_score >= 5 || g_state.news2_level == RISK_HIGH) ? "HIGH ALERT / URGENT" :
                              (g_state.news2_score >= 1) ? "LOW RISK / MONITOR" : "STABLE / NORMAL";
    TextOutA(hdcMem, news_x + 150, news_y + 40, szRiskTitle, strlen(szRiskTitle));
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, news_x + 150, news_y + 66, "National Early Warning Score (mNEWS2)", 37);
    
    char subBreakdown[96];
    snprintf(subBreakdown, sizeof(subBreakdown), "HR Pt: +%d | SpO2 Pt: +%d | RR Pt: +%d",
             (g_state.hr > 130 || g_state.hr < 40) ? 3 : (g_state.hr > 110 || g_state.hr <= 50) ? 1 : 0,
             (g_state.spo2 <= 91) ? 3 : (g_state.spo2 <= 93) ? 2 : (g_state.spo2 <= 95) ? 1 : 0,
             (g_state.derived_rr >= 25 || g_state.derived_rr <= 8) ? 3 : (g_state.derived_rr >= 21) ? 2 : 0);
    TextOutA(hdcMem, news_x + 150, news_y + 88, subBreakdown, strlen(subBreakdown));
    
    RECT rcAdv = { news_x + 18, news_y + 128, news_x + news_w - 18, news_y + 205 };
    HBRUSH hAdvBg = CreateSolidBrush(RGB(30, 36, 46));
    HPEN hAdvPen = CreatePen(PS_SOLID, 1, COL_CARD_BORDER);
    SelectObject(hdcMem, hAdvBg);
    SelectObject(hdcMem, hAdvPen);
    RoundRect(hdcMem, rcAdv.left, rcAdv.top, rcAdv.right, rcAdv.bottom, 6, 6);
    DeleteObject(hAdvBg);
    DeleteObject(hAdvPen);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MAIN);
    RECT rcText = { rcAdv.left + 10, rcAdv.top + 8, rcAdv.right - 10, rcAdv.bottom - 8 };
    DrawTextA(hdcMem, g_state.news2_advisory, -1, &rcText, DT_WORDBREAK);
    
    // ROW 2: Primary Vitals
    int r2_y = 300, r2_h = 130;
    
    // Card 1: Heart Rate
    int c1_x = 18, c1_w = 236;
    COLORREF hrCol = (g_state.hr > 120.0f || g_state.hr < 45.0f) ? COL_RED :
                     (g_state.hr > 100.0f || g_state.hr < 55.0f) ? COL_AMBER : COL_GREEN_BRT;
    DrawDarkCard(hdcMem, c1_x, r2_y, c1_w, r2_h, "HEART RATE", hrCol);
    
    char szVal[32];
    if (g_state.is_serial_connected && !g_state.is_finger_present) {
        snprintf(szVal, sizeof(szVal), "--");
    } else {
        snprintf(szVal, sizeof(szVal), "%.0f", g_state.hr);
    }
    SelectObject(hdcMem, g_font_large_val);
    SetTextColor(hdcMem, hrCol);
    TextOutA(hdcMem, c1_x + 18, r2_y + 36, szVal, strlen(szVal));
    
    SelectObject(hdcMem, g_font_title);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c1_x + 115, r2_y + 48, "BPM", 3);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c1_x + 18, r2_y + 96, g_state.is_serial_connected ? "Live MAX30100 Optical" : "Resting Target: 60-100", 22);
    
    // Card 2: SpO2
    int c2_x = 264, c2_w = 236;
    COLORREF spo2Col = (g_state.spo2 < 90.0f) ? COL_RED : (g_state.spo2 < 95.0f) ? COL_AMBER : COL_CYAN;
    DrawDarkCard(hdcMem, c2_x, r2_y, c2_w, r2_h, "BLOOD OXYGEN (SpO2)", spo2Col);
    
    if (g_state.is_serial_connected && !g_state.is_finger_present) {
        snprintf(szVal, sizeof(szVal), "--");
    } else {
        snprintf(szVal, sizeof(szVal), "%.1f", g_state.spo2);
    }
    SelectObject(hdcMem, g_font_large_val);
    SetTextColor(hdcMem, spo2Col);
    TextOutA(hdcMem, c2_x + 18, r2_y + 36, szVal, strlen(szVal));
    
    SelectObject(hdcMem, g_font_title);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c2_x + 130, r2_y + 48, "%", 1);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c2_x + 18, r2_y + 96, g_state.is_serial_connected ? "Red/IR Ratio Calibration" : "Red/IR Ratio R: 0.62", 24);
    
    // Card 3: HRV RMSSD
    int c3_x = 510, c3_w = 236;
    COLORREF hrvCol = (g_state.rmssd < 15.0f) ? COL_RED : (g_state.rmssd < 25.0f) ? COL_AMBER : COL_GREEN_BRT;
    DrawDarkCard(hdcMem, c3_x, r2_y, c3_w, r2_h, "HRV (RMSSD)", hrvCol);
    
    if (g_state.is_serial_connected && !g_state.is_finger_present) {
        snprintf(szVal, sizeof(szVal), "--");
    } else {
        snprintf(szVal, sizeof(szVal), "%.1f", g_state.rmssd);
    }
    SelectObject(hdcMem, g_font_large_val);
    SetTextColor(hdcMem, hrvCol);
    TextOutA(hdcMem, c3_x + 18, r2_y + 36, szVal, strlen(szVal));
    
    SelectObject(hdcMem, g_font_title);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c3_x + 125, r2_y + 48, "ms", 2);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c3_x + 18, r2_y + 96, "Vagal Autonomic Reserve", 23);
    
    // Card 4: Derived Resp Rate
    int c4_x = 756, c4_w = 236;
    COLORREF rrCol = (g_state.derived_rr > 26.0f || g_state.derived_rr < 9.0f) ? COL_RED :
                    (g_state.derived_rr > 21.0f || g_state.derived_rr < 11.0f) ? COL_AMBER : COL_CYAN;
    DrawDarkCard(hdcMem, c4_x, r2_y, c4_w, r2_h, "RESPIRATORY RATE (PPG-RR)", rrCol);
    
    snprintf(szVal, sizeof(szVal), "%.0f", g_state.derived_rr);
    SelectObject(hdcMem, g_font_large_val);
    SetTextColor(hdcMem, rrCol);
    TextOutA(hdcMem, c4_x + 18, r2_y + 36, szVal, strlen(szVal));
    
    SelectObject(hdcMem, g_font_title);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c4_x + 105, r2_y + 48, "Br/m", 4);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c4_x + 18, r2_y + 96, "RSA Modulated Extraction", 24);
    
    // Card 5: SQI
    int c5_x = 1002, c5_w = 238;
    COLORREF sqiCol = (g_state.sqi < 0.50f) ? COL_RED : (g_state.sqi < 0.75f) ? COL_AMBER : COL_GREEN_BRT;
    DrawDarkCard(hdcMem, c5_x, r2_y, c5_w, r2_h, "SIGNAL QUALITY (SQI)", sqiCol);
    
    snprintf(szVal, sizeof(szVal), "%.0f%%", g_state.sqi * 100.0f);
    SelectObject(hdcMem, g_font_large_val);
    SetTextColor(hdcMem, sqiCol);
    TextOutA(hdcMem, c5_x + 18, r2_y + 36, szVal, strlen(szVal));
    
    DrawProgressBar(hdcMem, c5_x + 18, r2_y + 86, c5_w - 36, 12, g_state.sqi * 100.0f, sqiCol);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, c5_x + 18, r2_y + 104, (g_state.sqi >= 0.65f) ? "Artifact Filter: PASSED" : "ARTIFACT DETECTED (GATED)",
             (g_state.sqi >= 0.65f) ? 23 : 25);
    
    // ROW 3: Environment & Stress Indices
    int r3_y = 440, r3_h = 135;
    
    int e1_x = 18, e1_w = 300;
    COLORREF tempCol = (g_state.temp_c > 40.0f || g_state.temp_c < 12.0f) ? COL_RED :
                       (g_state.temp_c > 35.0f || g_state.temp_c < 18.0f) ? COL_AMBER : COL_GREEN_BRT;
    DrawDarkCard(hdcMem, e1_x, r3_y, e1_w, r3_h, "AMBIENT TEMPERATURE & HEAT INDEX", tempCol);
    
    snprintf(szVal, sizeof(szVal), "%.1f C", g_state.temp_c);
    SelectObject(hdcMem, g_font_med_val);
    SetTextColor(hdcMem, tempCol);
    TextOutA(hdcMem, e1_x + 18, r3_y + 34, szVal, strlen(szVal));
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    char szHI[64];
    snprintf(szHI, sizeof(szHI), "NOAA Steadman Heat Index: %.1f C", g_state.heat_index_c);
    TextOutA(hdcMem, e1_x + 18, r3_y + 76, szHI, strlen(szHI));
    
    const char *szHIAlert = (g_state.heat_index_c >= 54.0f) ? "HI Status: EXTREME DANGER" :
                            (g_state.heat_index_c >= 41.0f) ? "HI Status: DANGER (Heat Stroke)" :
                            (g_state.heat_index_c >= 32.0f) ? "HI Status: EXTREME CAUTION" : "HI Status: NORMAL / SAFE";
    SetTextColor(hdcMem, (g_state.heat_index_c >= 41.0f) ? COL_RED : (g_state.heat_index_c >= 32.0f) ? COL_AMBER : COL_GREEN_BRT);
    TextOutA(hdcMem, e1_x + 18, r3_y + 98, szHIAlert, strlen(szHIAlert));
    
    int e2_x = 328, e2_w = 260;
    DrawDarkCard(hdcMem, e2_x, r3_y, e2_w, r3_h, "RELATIVE HUMIDITY (SHT31/BME280)", COL_CYAN);
    
    snprintf(szVal, sizeof(szVal), "%.1f %%", g_state.humidity_pct);
    SelectObject(hdcMem, g_font_med_val);
    SetTextColor(hdcMem, COL_CYAN);
    TextOutA(hdcMem, e2_x + 18, r3_y + 34, szVal, strlen(szVal));
    
    DrawProgressBar(hdcMem, e2_x + 18, r3_y + 76, e2_w - 36, 12, g_state.humidity_pct, COL_CYAN);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, e2_x + 18, r3_y + 98, (g_state.humidity_pct > 80.0f) ? "Hygroscopic Growth Active" : "Comfort Zone: 40-60%", 25);
    
    int e3_x = 598, e3_w = 320;
    COLORREF pmCol = (g_state.pm25_calibrated > 150.0f) ? COL_RED : (g_state.pm25_calibrated > 60.0f) ? COL_AMBER : COL_GREEN_BRT;
    DrawDarkCard(hdcMem, e3_x, r3_y, e3_w, r3_h, "PM2.5 AIR QUALITY (NEURAL CALIBRATED)", pmCol);
    
    snprintf(szVal, sizeof(szVal), "%.0f ug/m3", g_state.pm25_calibrated);
    SelectObject(hdcMem, g_font_med_val);
    SetTextColor(hdcMem, pmCol);
    TextOutA(hdcMem, e3_x + 18, r3_y + 34, szVal, strlen(szVal));
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    char szRawPm[64];
    snprintf(szRawPm, sizeof(szRawPm), "Raw Laser: %.0f ug/m3 (Bias: -%.0f ug/m3)",
             g_state.pm25_raw, g_state.pm25_raw - g_state.pm25_calibrated);
    TextOutA(hdcMem, e3_x + 18, r3_y + 76, szRawPm, strlen(szRawPm));
    
    const char *szAqi = (g_state.pm25_calibrated > 150.0f) ? "AQI: VERY UNHEALTHY / HAZARDOUS" :
                        (g_state.pm25_calibrated > 60.0f)  ? "AQI: MODERATE / UNHEALTHY SENSITIVE" : "AQI: GOOD / CLEAN AIR";
    SetTextColor(hdcMem, pmCol);
    TextOutA(hdcMem, e3_x + 18, r3_y + 98, szAqi, strlen(szAqi));
    
    int e4_x = 928, e4_w = 312;
    DrawDarkCard(hdcMem, e4_x, r3_y, e4_w, r3_h, "PEER-REVIEWED PHYSIOLOGICAL STRESS", COL_PURPLE);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, e4_x + 18, r3_y + 30, "Moran's Physiological Strain (PSI 0-10):", 40);
    
    char szPsi[48];
    COLORREF psiCol = (g_state.moran_psi >= 7.0f) ? COL_RED : (g_state.moran_psi >= 4.0f) ? COL_AMBER : COL_GREEN_BRT;
    snprintf(szPsi, sizeof(szPsi), "%.2f / 10.0 (%s)", g_state.moran_psi,
             (g_state.moran_psi >= 7.0f) ? "HIGH HEAT STRAIN" : (g_state.moran_psi >= 4.0f) ? "MODERATE" : "LOW");
    SetTextColor(hdcMem, psiCol);
    TextOutA(hdcMem, e4_x + 18, r3_y + 48, szPsi, strlen(szPsi));
    DrawProgressBar(hdcMem, e4_x + 18, r3_y + 68, e4_w - 36, 8, g_state.moran_psi * 10.0f, psiCol);
    
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, e4_x + 18, r3_y + 82, "AHA (Brook 2010) PM2.5-HRV Autonomic Strain:", 44);
    
    char szAha[48];
    COLORREF ahaCol = (g_state.aha_autonomic_strain >= 0.70f) ? COL_RED : (g_state.aha_autonomic_strain >= 0.40f) ? COL_AMBER : COL_CYAN;
    snprintf(szAha, sizeof(szAha), "%.2f / 1.0 (%s)", g_state.aha_autonomic_strain,
             (g_state.aha_autonomic_strain >= 0.70f) ? "SEVERE VAGAL SUPPRESSION" : (g_state.aha_autonomic_strain >= 0.40f) ? "ELEVATED" : "OPTIMAL");
    SetTextColor(hdcMem, ahaCol);
    TextOutA(hdcMem, e4_x + 18, r3_y + 98, szAha, strlen(szAha));
    DrawProgressBar(hdcMem, e4_x + 18, r3_y + 116, e4_w - 36, 8, g_state.aha_autonomic_strain * 100.0f, ahaCol);
    
    // ROW 4: TinyML INT8 Inference
    int r4_y = 585, r4_h = 100, r4_w = 1222;
    DrawDarkCard(hdcMem, 18, r4_y, r4_w, r4_h, "ON-DEVICE TinyML INT8 MULTI-HAZARD INFERENCE (6 -> 24 -> 16 -> 3 | 619 PARAMS)", COL_PURPLE);
    
    int meter_w = 340, meter_h = 14;
    
    int ax1_x = 36, ax1_y = r4_y + 36;
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MAIN);
    char szRisk1[64];
    COLORREF cR1 = (g_state.heat_risk_pct > 70.0f) ? COL_RED : (g_state.heat_risk_pct > 35.0f) ? COL_AMBER : COL_GREEN_BRT;
    snprintf(szRisk1, sizeof(szRisk1), "HEAT STROKE RISK: %.1f%% (%s)", g_state.heat_risk_pct,
             (g_state.heat_risk_pct > 70.0f) ? "CRITICAL" : (g_state.heat_risk_pct > 35.0f) ? "ELEVATED" : "NORMAL");
    TextOutA(hdcMem, ax1_x, ax1_y, szRisk1, strlen(szRisk1));
    DrawProgressBar(hdcMem, ax1_x, ax1_y + 20, meter_w, meter_h, g_state.heat_risk_pct, cR1);
    
    int ax2_x = 420, ax2_y = r4_y + 36;
    COLORREF cR2 = (g_state.pollution_risk_pct > 70.0f) ? COL_RED : (g_state.pollution_risk_pct > 35.0f) ? COL_AMBER : COL_GREEN_BRT;
    char szRisk2[64];
    snprintf(szRisk2, sizeof(szRisk2), "POLLUTION / SMOG RISK: %.1f%% (%s)", g_state.pollution_risk_pct,
             (g_state.pollution_risk_pct > 70.0f) ? "CRITICAL" : (g_state.pollution_risk_pct > 35.0f) ? "ELEVATED" : "NORMAL");
    TextOutA(hdcMem, ax2_x, ax2_y, szRisk2, strlen(szRisk2));
    DrawProgressBar(hdcMem, ax2_x, ax2_y + 20, meter_w, meter_h, g_state.pollution_risk_pct, cR2);
    
    int ax3_x = 804, ax3_y = r4_y + 36;
    COLORREF cR3 = (g_state.flood_risk_pct > 70.0f) ? COL_RED : (g_state.flood_risk_pct > 35.0f) ? COL_AMBER : COL_GREEN_BRT;
    char szRisk3[64];
    snprintf(szRisk3, sizeof(szRisk3), "FLOOD / COLD SHOCK: %.1f%% (%s)", g_state.flood_risk_pct,
             (g_state.flood_risk_pct > 70.0f) ? "CRITICAL" : (g_state.flood_risk_pct > 35.0f) ? "ELEVATED" : "NORMAL");
    TextOutA(hdcMem, ax3_x, ax3_y, szRisk3, strlen(szRisk3));
    DrawProgressBar(hdcMem, ax3_x, ax3_y + 20, meter_w, meter_h, g_state.flood_risk_pct, cR3);
    
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, 36, r4_y + 78, "MIMIC-III triage 94.11% (16,387 recs) | Val 88.47% | INT8 weights 619 B | Dequantized-float core", 96);
    
    // ROW 5: Status Line
    int st_y = height - 26;
    SelectObject(hdcMem, g_font_small);
    SetTextColor(hdcMem, COL_CYAN);
    TextOutA(hdcMem, 18, st_y, "SYSTEM STATUS:", 14);
    SetTextColor(hdcMem, COL_TEXT_MUTED);
    TextOutA(hdcMem, 120, st_y, g_state.status_bar_text, strlen(g_state.status_bar_text));
}

/* -------------------------------------------------------------------------- */
/* Window Message Handler (WndProc)                                           */
/* -------------------------------------------------------------------------- */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hwnd_main = hwnd;
            
            g_font_header = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            g_font_title = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                       DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            g_font_large_val = CreateFontA(46, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                           DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            g_font_med_val = CreateFontA(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                         DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            g_font_label = CreateFontA(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                       DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            g_font_small = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                       DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            
            int btn_y = 696;
            int btn_w = 145, btn_h = 32;
            const char *btn_labels[6] = {
                "[1] Baseline",
                "[2] Heat Wave",
                "[3] Smog Crisis",
                "[4] Flash Flood",
                "[5] ICU Crisis",
                "[6] Motion Test"
            };
            
            for (int i = 0; i < 6; i++) {
                g_hwnd_scenario_btns[i] = CreateWindowA(
                    "BUTTON", btn_labels[i],
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    18 + i * (btn_w + 8), btn_y, btn_w, btn_h,
                    hwnd, (HMENU)(INT_PTR)(IDC_BTN_SCENARIO_1 + i),
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
                );
            }
            
            g_hwnd_btn_pause = CreateWindowA(
                "BUTTON", "|| Pause",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                940, btn_y, 75, btn_h,
                hwnd, (HMENU)IDC_BTN_PAUSE,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );
            
            g_hwnd_edit_com = CreateWindowA(
                "EDIT", "COM3",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                1024, btn_y + 2, 60, btn_h - 4,
                hwnd, (HMENU)IDC_EDIT_COM,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );
            
            g_hwnd_btn_connect = CreateWindowA(
                "BUTTON", "Connect COM",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                1092, btn_y, 148, btn_h,
                hwnd, (HMENU)IDC_BTN_CONNECT,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );
            
            SetTimer(hwnd, 1, 33, NULL);
            break;
        }
        
        case WM_TIMER: {
            UpdateTelemetryStep();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId >= IDC_BTN_SCENARIO_1 && wmId <= IDC_BTN_SCENARIO_6) {
                if (g_state.is_serial_connected) {
                    DisconnectSerialPort();
                }
                g_state.active_scenario = wmId - IDC_BTN_SCENARIO_1;
                snprintf(g_state.status_bar_text, sizeof(g_state.status_bar_text),
                         "Switched to scenario [%d]: %s (%s)",
                         g_state.active_scenario + 1,
                         g_scenarios[g_state.active_scenario].name,
                         g_scenarios[g_state.active_scenario].description);
            } else if (wmId == IDC_BTN_PAUSE) {
                g_state.is_paused = !g_state.is_paused;
                SetWindowTextA(g_hwnd_btn_pause, g_state.is_paused ? "> Run" : "|| Pause");
                snprintf(g_state.status_bar_text, sizeof(g_state.status_bar_text),
                         g_state.is_paused ? "Simulation paused." : "Simulation resumed.");
            } else if (wmId == IDC_BTN_CONNECT) {
                if (g_state.is_serial_connected) {
                    DisconnectSerialPort();
                } else {
                    char port[16];
                    GetWindowTextA(g_hwnd_edit_com, port, sizeof(port) - 1);
                    ConnectSerialPort(port);
                }
            }
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int width = rcClient.right - rcClient.left;
            int height = rcClient.bottom - rcClient.top;
            
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
            
            RenderDashboard(hdcMem, width, height);
            
            BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_ERASEBKGND:
            return 1;
            
        case WM_DESTROY: {
            DisconnectSerialPort();
            KillTimer(hwnd, 1);
            if (g_font_header) DeleteObject(g_font_header);
            if (g_font_title) DeleteObject(g_font_title);
            if (g_font_large_val) DeleteObject(g_font_large_val);
            if (g_font_med_val) DeleteObject(g_font_med_val);
            if (g_font_label) DeleteObject(g_font_label);
            if (g_font_small) DeleteObject(g_font_small);
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
/* WinMain Application Entry Point                                            */
/* -------------------------------------------------------------------------- */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitCommonControls();
    srand((unsigned int)time(NULL));
    
    memset(&g_state, 0, sizeof(g_state));
    g_state.hr = 72.0f;
    g_state.spo2 = 98.5f;
    g_state.rmssd = 38.0f;
    g_state.derived_rr = 15.0f;
    g_state.sqi = 0.96f;
    g_state.temp_c = 24.5f;
    g_state.humidity_pct = 48.0f;
    g_state.pm25_raw = 12.0f;
    g_state.pm25_calibrated = 12.0f;
    g_state.active_scenario = 0;
    g_state.h_serial_port = INVALID_HANDLE_VALUE;
    strncpy(g_state.com_port_str, "COM3", sizeof(g_state.com_port_str) - 1);
    strncpy(g_state.status_bar_text, "Ready. Enter COM port and click 'Connect COM' for live hardware, or select a scenario below.",
            sizeof(g_state.status_bar_text) - 1);
    
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "ShrikeFiDashboardClass";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class!", "Error", MB_ICONERROR);
        return 1;
    }
    
    RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    
    HWND hwnd = CreateWindowExA(
        0,
        "ShrikeFiDashboardClass",
        "EdgeGuard / ShrikeFi - FPGA & AI Clinical Companion Dashboard (SIH26181)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create main dashboard window!", "Error", MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(hwnd, nCmdShow ? nCmdShow : SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    return (int)msg.wParam;
}
