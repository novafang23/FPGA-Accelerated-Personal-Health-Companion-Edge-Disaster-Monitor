/*
 * ppg_sqi.c
 * Photoplethysmography Signal Quality Index (SQI) Implementation
 * Elgendi (2016) skewness-based SQI, with a perfusion-index term and a
 * beat-interval regularity term added here. The Karlen (2012) template-
 * correlation SQI is NOT implemented.
 * Smart India Hackathon 2026 - Project SIH26181
 */

#include "ppg_sqi.h"
#include <math.h>

static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

void ppg_calculate_sqi_with_motion(
    const uint32_t *raw_ir_samples,
    size_t sample_count,
    const float *ibi_ms,
    size_t ibi_count,
    float motion_energy_g,
    ppg_sqi_result_t *result
) {
    if (!result) return;

    /* Default fallback for insufficient data */
    result->overall_sqi = 0.50f;
    result->perfusion_index = 1.0f;
    result->interval_regularity = 0.50f;
    result->waveform_skewness = 0.0f;
    result->is_motion_artifact = false;
    result->motion_energy = motion_energy_g;

    float pi_score = 0.50f;
    float skew_score = 0.50f;
    float reg_score = 0.50f;

    /* 1. Waveform Morphology & Perfusion Index (AC/DC) */
    if (raw_ir_samples && sample_count >= 16) {
        float sum = 0.0f;

        for (size_t i = 0; i < sample_count; i++) {
            sum += (float)raw_ir_samples[i];
        }

        float mean = sum / (float)sample_count;
        float var_sum = 0.0f;
        for (size_t i = 0; i < sample_count; i++) {
            float diff = (float)raw_ir_samples[i] - mean;
            var_sum += diff * diff;
        }
        float std_dev = sqrtf(var_sum / (float)sample_count);
        /* Use the peak-to-peak equivalent of a sinusoid rather than raw
         * max-min. One MAX30102 spike or a single clipped sample must not
         * define perfusion for the entire three-second SQI window. */
        float ac = 2.0f * 1.41421356f * std_dev;
        float dc = (mean > 1.0f) ? mean : 1.0f;
        float pi = (ac / dc) * 100.0f;
        result->perfusion_index = pi;

        /* Score Perfusion Index calibrated for 18-bit MAX30102 DC levels (~100k-185k) */
        if (pi < 0.02f) {
            pi_score = 0.1f; /* Sensor off-finger or negligible pulsatile signal */
        } else if (pi > 15.0f) {
            pi_score = 0.2f; /* Ambient light saturation / severe clip */
        } else if (pi >= 0.08f && pi <= 7.0f) {
            pi_score = 1.0f; /* Optimal clinical pulsatile amplitude */
        } else {
            pi_score = 0.75f;
        }

        /* Compute skewness m3 / sigma^3 after clipping extreme samples.
         * This preserves pulse morphology while preventing one optical spike
         * from turning the whole SQI window into a motion artifact. */
        if (std_dev > 1e-4f) {
            float m3_sum = 0.0f;
            float clip = 3.0f * std_dev;
            for (size_t i = 0; i < sample_count; i++) {
                float diff = (float)raw_ir_samples[i] - mean;
                if (diff > clip) diff = clip;
                if (diff < -clip) diff = -clip;
                m3_sum += diff * diff * diff;
            }
            float skewness = (m3_sum / (float)sample_count) / (std_dev * std_dev * std_dev);
            result->waveform_skewness = skewness;

            /* Clean PPG has steep systolic rise and gentle runoff: positive skewness */
            if (skewness > 0.1f) {
                skew_score = clamp01(0.5f + 0.4f * skewness);
            } else {
                /* Negative or zero skewness indicates motion baseline wandering or noise */
                skew_score = clamp01(0.5f + 0.5f * skewness);
            }
        }
    }

    /* 2. Beat-to-Beat Interval Regularity */
    bool reg_measured = false;
    if (ibi_ms && ibi_count >= 3) {
        float ibi_sum = 0.0f;
        for (size_t i = 0; i < ibi_count; i++) {
            ibi_sum += ibi_ms[i];
        }
        float ibi_mean = ibi_sum / (float)ibi_count;

        if (ibi_mean > 200.0f) {
            float ibi_var_sum = 0.0f;
            for (size_t i = 0; i < ibi_count; i++) {
                float diff = ibi_ms[i] - ibi_mean;
                ibi_var_sum += diff * diff;
            }
            float ibi_std = sqrtf(ibi_var_sum / (float)ibi_count);
            float cv = ibi_std / ibi_mean; /* Coefficient of variation */

            /* Healthy physiological rhythm CV is typically 0.03 - 0.15 */
            if (cv <= 0.12f) {
                reg_score = 1.0f;
            } else if (cv <= 0.25f) {
                reg_score = 1.0f - (cv - 0.12f) * 3.0f;
            } else {
                /* Excessive jitter > 0.25 strongly indicates motion artifact */
                reg_score = clamp01(0.6f - (cv - 0.25f) * 2.0f);
            }
            result->interval_regularity = reg_score;
            reg_measured = true;
        }
    }

    /* 3. Weighted Composite SQI
     *
     * reg_score starts at 0.50 and is only overwritten above, so when no
     * beat-to-beat intervals have been accepted that initialiser used to be
     * mixed in at its full 40% weight. A clean-looking waveform with NO
     * detected beats therefore scored 0.35*1.0 + 0.40*0.50 + 0.25*1.0 = 0.80
     * and cleared the 0.70 gate that certifies a pulse for triage. That state
     * is reachable: the FPGA crest detector can fall silent while the optics
     * stay healthy, and the software fallback is suppressed while the FPGA is
     * considered alive - so the device reported hospital-grade signal quality
     * with zero beats behind it.
     *
     * A neutral substitute is not a measurement. Waveform shape alone cannot
     * certify a pulsatile signal: regularity is the component that separates a
     * pulse train from a plausible-looking curve. If it never ran, hold the
     * composite below the validity threshold instead of inventing it. This does
     * not affect normal operation - four accepted beats are enough to measure
     * it, and the triage gate needs ten. */
    float composite = 0.35f * pi_score + 0.40f * reg_score + 0.25f * skew_score;
    if (!reg_measured && composite >= PPG_SQI_THRESHOLD_VALID) {
        composite = PPG_SQI_THRESHOLD_VALID - 0.01f;
    }

    /* 4. IMU Motion Artifact Rejection & Gating
     * Optical photoplethysmography is highly susceptible to sensor-tissue displacement
     * and venous pool movement during physical motion. If the IMU reports dynamic motion:
     * - motion_energy_g > 0.08g: penalizes composite SQI proportionally
     * - motion_energy_g >= 0.35g: gates signal as corrupted (SQI < 0.70) */
    if (motion_energy_g > 0.08f) {
        float penalty = (motion_energy_g - 0.08f) * 1.5f;
        if (penalty > 0.45f) penalty = 0.45f;
        composite -= penalty;
    }
    if (motion_energy_g >= 0.35f && composite >= PPG_SQI_THRESHOLD_VALID) {
        composite = PPG_SQI_THRESHOLD_VALID - 0.05f;
    }

    result->overall_sqi = clamp01(composite);
    result->is_motion_artifact = (result->overall_sqi < PPG_SQI_THRESHOLD_VALID);
}

void ppg_calculate_sqi(
    const uint32_t *raw_ir_samples,
    size_t sample_count,
    const float *ibi_ms,
    size_t ibi_count,
    ppg_sqi_result_t *result
) {
    ppg_calculate_sqi_with_motion(raw_ir_samples, sample_count, ibi_ms, ibi_count, 0.0f, result);
}

bool ppg_is_quality_acceptable(float sqi) {
    return (sqi >= PPG_SQI_THRESHOLD_VALID);
}
