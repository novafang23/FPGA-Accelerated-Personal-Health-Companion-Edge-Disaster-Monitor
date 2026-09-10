/*
 * ppg_respiratory_rate.h
 * Photoplethysmography-Derived Respiratory Rate (EDR) Engine
 * Based on Charlton et al. (2018) & Addison (2014)
 * Smart India Hackathon 2026 - Project SIH26181
 */

#ifndef PPG_RESPIRATORY_RATE_H
#define PPG_RESPIRATORY_RATE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_RR_MIN_BPM       6.0f    /* Physiological floor: severe bradypnea */
#define PPG_RR_MAX_BPM       36.0f   /* Physiological ceiling: severe tachypnea */
#define PPG_RR_DEFAULT_BPM   14.0f   /* Normal resting adult breathing rate */

typedef struct {
    float respiratory_rate_bpm; /* Estimated breaths per minute [6.0, 36.0] */
    float confidence;           /* Respiration estimation confidence [0.0, 1.0] */
    float rsa_depth_ms;         /* Respiratory Sinus Arrhythmia peak-to-peak amplitude (ms) */
    bool  is_reliable;          /* True if confidence >= 0.60 */
} ppg_respiratory_result_t;

/*
 * Estimate Respiratory Rate from pulse Inter-Beat Intervals (IBIs)
 * and pulse peak amplitudes (AM + FM fusion).
 * ibi_ms: array of consecutive beat-to-beat intervals in ms (e.g. 10 to 40 beats)
 * pulse_amplitudes: optional array of corresponding pulse peak heights (can be NULL)
 * beat_count: number of beats in history
 * result: pointer to output structure
 */
void ppg_estimate_respiratory_rate(
    const float *ibi_ms,
    const float *pulse_amplitudes,
    size_t beat_count,
    ppg_respiratory_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* PPG_RESPIRATORY_RATE_H */
