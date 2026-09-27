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

/* --- Confidence calibration (see ppg_respiratory_rate.c) -------------------
 *
 * Two independent things have to be true before a respiratory rate is worth
 * publishing: the IBI series has to be PERIODIC at the winning lag, and the
 * modulation has to be BIG enough to be respiratory rather than noise. The
 * earlier implementation scored only the first, using the raw autocorrelation
 * coefficient as the confidence. On real finger PPG that peaks around 0.3-0.5,
 * so it never cleared the 0.60 reliability bar and RR was published as 0
 * (unavailable) on every real contact while the estimator was working correctly.
 *
 * These thresholds are a first calibration from first principles and the
 * literature. They have NOT been validated against paced breathing; that is the
 * outstanding half of T3.5. Do not treat them as measured.
 */

/* Autocorrelation coefficient at the winning lag. Below MIN there is no
 * periodicity to find; at STRONG the peak is unambiguous. Random jitter of
 * similar magnitude peaks around 1/sqrt(N), which for a 60-beat window is about
 * 0.13, so MIN sits deliberately above that. */
#define PPG_RR_ACF_MIN       0.25f
#define PPG_RR_ACF_STRONG    0.60f

/* Respiratory sinus arrhythmia, as an equivalent sinusoid peak-to-peak in ms.
 * Adult RSA at rest is typically 20-60 ms peak-to-peak; below about 10 ms there
 * is effectively no modulation to detect. */
#define PPG_RSA_DEPTH_NONE_MS  10.0f
#define PPG_RSA_DEPTH_CLEAR_MS 35.0f

/* When the frequency-modulation and amplitude-modulation paths - independent
 * measurements of the same physiology - land within this many breaths per minute
 * of each other, that agreement is evidence in its own right and is worth more
 * than either coefficient alone. */
#define PPG_RR_AGREE_BPM     3.0f
#define PPG_RR_AGREE_BONUS   0.15f

/* Reliability bar. UNCHANGED: the point of this work is to make a genuine
 * respiratory component reach it, not to lower it. Lowering it would reinstate
 * the bug where a railed 36 br/min was published as a reliable measurement. */
#define PPG_RR_CONF_MIN      0.60f

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
