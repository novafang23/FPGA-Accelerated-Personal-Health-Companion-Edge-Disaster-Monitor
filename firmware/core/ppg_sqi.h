/*
 * ppg_sqi.h
 * Photoplethysmography Signal Quality Index (SQI) Module
 * Based on Elgendi (2016) & Karlen et al. (2012)
 * Smart India Hackathon 2026 - Project SIH26181
 */

#ifndef PPG_SQI_H
#define PPG_SQI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_SQI_THRESHOLD_VALID     0.70f   /* Minimum SQI for clinical triage */
#define PPG_SQI_THRESHOLD_EXCELLENT 0.85f   /* Clean, hospital-grade pulse */

typedef struct {
    float overall_sqi;          /* Composite SQI [0.0, 1.0] */
    float perfusion_index;      /* AC/DC ratio percentage (%) */
    float interval_regularity;  /* Beat interval consistency [0.0, 1.0] */
    float waveform_skewness;    /* Pulse skewness (positive = clean systolic upstroke) */
    bool  is_motion_artifact;   /* True if motion corrupted (SQI < 0.70) */
    float motion_energy;        /* Dynamic acceleration motion energy in g */
} ppg_sqi_result_t;

/*
 * Calculate comprehensive PPG Signal Quality Index.
 * raw_ir_samples: recent raw IR ADC readings (e.g. 50-100 samples at 25-100 Hz)
 * sample_count: number of raw samples
 * ibi_ms: array of recent inter-beat intervals in ms
 * ibi_count: number of IBIs (typically 5 to 20 beats)
 */
void ppg_calculate_sqi(
    const uint32_t *raw_ir_samples,
    size_t sample_count,
    const float *ibi_ms,
    size_t ibi_count,
    ppg_sqi_result_t *result
);

/*
 * Calculate PPG Signal Quality Index with IMU motion energy gating.
 * motion_energy_g: dynamic acceleration magnitude |A - 1.0g| in g.
 * When motion_energy_g > 0.10g, motion artifact penalty scales down SQI.
 * When motion_energy_g >= 0.35g, signal is gated as corrupted (SQI < 0.70).
 */
void ppg_calculate_sqi_with_motion(
    const uint32_t *raw_ir_samples,
    size_t sample_count,
    const float *ibi_ms,
    size_t ibi_count,
    float motion_energy_g,
    ppg_sqi_result_t *result
);

/* Quick check if current pulse window is trustworthy for clinical decision */
bool ppg_is_quality_acceptable(float sqi);

#ifdef __cplusplus
}
#endif

#endif /* PPG_SQI_H */
