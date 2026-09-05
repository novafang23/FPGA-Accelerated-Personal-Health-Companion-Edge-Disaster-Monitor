/*
 * spo2_engine.h
 * Pulse Oximetry (SpO2) Estimation Module
 */
#ifndef SPO2_ENGINE_H
#define SPO2_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPO2_WINDOW_SIZE 50  /* Samples per measurement window (1.0 sec at 50Hz) */
#define SPO2_MA_FILTER_SIZE 8 /* Moving average history size (8-second smoothing window) */

/* Clinical validity thresholds calibrated for MAX30102 18-bit optical levels */
#define SPO2_MIN_AC_IR              10.0f  /* Minimum pulsatile AC amplitude for IR (counts) */
#define SPO2_MIN_AC_RED              8.0f  /* Minimum pulsatile AC amplitude for Red (counts) */
#define SPO2_MIN_DC_IR            1000.0f  /* Minimum optical DC level for IR (counts, ambient air is <600) */
#define SPO2_MIN_DC_RED            800.0f  /* Minimum optical DC level for Red (counts) */
#define SPO2_MIN_PERFUSION_INDEX     0.15f /* Minimum Perfusion Index (0.15%) */
#define SPO2_MAX_PERFUSION_INDEX    15.0f  /* Maximum Perfusion Index to reject motion (15.0%) */
#define SPO2_MIN_RATIO_R             0.35f /* Physiological lower bound for R (approx 100% SpO2) */
#define SPO2_MAX_RATIO_R             1.65f /* Physiological upper bound for R (approx 68% SpO2) */
#define SPO2_REQUIRED_VALID_WINDOWS  1     /* Single 1-second valid window to lock in with 8-tap smoothing */

typedef struct {
    uint32_t red_min, red_max;
    uint32_t ir_min, ir_max;
    int      sample_count;

    float    ratio_r;
    float    perfusion_index;      /* Perfusion Index (%): (AC_ir / DC_ir) * 100 */
    int      consecutive_valid;    /* Number of consecutive valid measurement windows */
    float    spo2_history[SPO2_MA_FILTER_SIZE];
    int      spo2_hist_idx;
    int      spo2_hist_count;
    float    spo2;
    int      valid;
} spo2_state_t;

/* Initialize / reset SpO2 state */
void  spo2_init(spo2_state_t *state);

/* Feed a pair of filtered Red and IR samples (18-bit); SpO2 is recomputed at end of each window */
void  spo2_add_samples(spo2_state_t *state, uint32_t red_filtered, uint32_t ir_filtered);

/* Get the latest SpO2 percentage (0-100) */
float spo2_get_value(const spo2_state_t *state);

/* Get current Perfusion Index (PI %) */
float spo2_get_perfusion_index(const spo2_state_t *state);

/* Returns 1 if the SpO2 reading is currently valid */
int   spo2_is_valid(const spo2_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* SPO2_ENGINE_H */
