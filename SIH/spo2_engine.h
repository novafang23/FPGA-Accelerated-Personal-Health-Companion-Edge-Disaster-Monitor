/*
 * spo2_engine.h — Pulse Oximetry (SpO2) Estimation Module
 * SIH26181: AI-Powered Personal Health Companion
 *
 * Computes blood oxygen saturation from dual-wavelength PPG signals
 * (Red 660nm + Infrared 940nm) using the Ratio-of-Ratios method:
 *
 *   R = (AC_Red / DC_Red) / (AC_IR / DC_IR)
 *   SpO2 = 110 - 25 * R
 *
 * AC component = peak-to-valley pulsatile amplitude
 * DC component = average (baseline) signal level
 */

#ifndef SPO2_ENGINE_H
#define SPO2_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPO2_WINDOW_SIZE 50  /* Samples per measurement window */

typedef struct {
    /* Running min/max trackers for AC/DC extraction within current window */
    uint8_t red_min, red_max;
    uint8_t ir_min,  ir_max;
    int     sample_count;

    /* Computed values */
    float   ratio_r;    /* Ratio of ratios (R)       */
    float   spo2;       /* Estimated SpO2 percentage  */
    int     valid;      /* 1 if current reading valid */
} spo2_state_t;

/* Initialize / reset SpO2 state */
void  spo2_init(spo2_state_t *state);

/* Feed a pair of filtered Red and IR samples; SpO2 is recomputed at end of each window */
void  spo2_add_samples(spo2_state_t *state, uint8_t red_filtered, uint8_t ir_filtered);

/* Get the latest SpO2 percentage (0-100) */
float spo2_get_value(const spo2_state_t *state);

/* Returns 1 if the SpO2 reading is currently valid */
int   spo2_is_valid(const spo2_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* SPO2_ENGINE_H */
