/*
 * ppg_sqi.c
 * Photoplethysmography Signal Quality Index (SQI) Implementation
 * Based on Elgendi (2016) & Karlen et al. (2012)
 * Smart India Hackathon 2026 - Project SIH26181
 */

#include "ppg_sqi.h"
#include <math.h>

static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

void ppg_calculate_sqi(
    const uint32_t *raw_ir_samples,
    size_t sample_count,
    const float *ibi_ms,
    size_t ibi_count,
    ppg_sqi_result_t *result
) {
    if (!result) return;

    /* Default fallback for insufficient data */
    result->overall_sqi = 0.50f;
    result->perfusion_index = 1.0f;
    result->interval_regularity = 0.50f;
    result->waveform_skewness = 0.0f;
    result->is_motion_artifact = false;

    float pi_score = 0.50f;
    float skew_score = 0.50f;
    float reg_score = 0.50f;

    /* 1. Waveform Morphology & Perfusion Index (AC/DC) */
    if (raw_ir_samples && sample_count >= 16) {
        uint32_t min_v = raw_ir_samples[0];
        uint32_t max_v = raw_ir_samples[0];
        double sum = 0.0;

        for (size_t i = 0; i < sample_count; i++) {
            uint32_t v = raw_ir_samples[i];
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
            sum += (double)v;
        }

        double mean = sum / (double)sample_count;
        double ac = (double)(max_v - min_v);
        double dc = (mean > 1.0) ? mean : 1.0;
        float pi = (float)((ac / dc) * 100.0);
        result->perfusion_index = pi;

        /* Score Perfusion Index: typical healthy range 0.4% - 10.0% */
        if (pi < 0.15f) {
            pi_score = 0.1f; /* Sensor off-finger or very weak perfusion */
        } else if (pi > 15.0f) {
            pi_score = 0.2f; /* Ambient light saturation / severe clip */
        } else if (pi >= 0.5f && pi <= 7.0f) {
            pi_score = 1.0f; /* Optimal clinical pulsatile amplitude */
        } else {
            pi_score = 0.75f;
        }

        /* Compute Skewness m3 / sigma^3 */
        double var_sum = 0.0;
        for (size_t i = 0; i < sample_count; i++) {
            double diff = (double)raw_ir_samples[i] - mean;
            var_sum += diff * diff;
        }
        double std_dev = sqrt(var_sum / (double)sample_count);

        if (std_dev > 1e-4) {
            double m3_sum = 0.0;
            for (size_t i = 0; i < sample_count; i++) {
                double diff = (double)raw_ir_samples[i] - mean;
                m3_sum += diff * diff * diff;
            }
            double skewness = (m3_sum / (double)sample_count) / (std_dev * std_dev * std_dev);
            result->waveform_skewness = (float)skewness;

            /* Clean PPG has steep systolic rise and gentle runoff: positive skewness */
            if (skewness > 0.1) {
                skew_score = clamp01(0.5f + (float)(0.4 * skewness));
            } else {
                /* Negative or zero skewness indicates motion baseline wandering or noise */
                skew_score = clamp01(0.5f + (float)(0.5 * skewness));
            }
        }
    }

    /* 2. Beat-to-Beat Interval Regularity */
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
        }
        result->interval_regularity = reg_score;
    }

    /* 3. Weighted Composite SQI */
    float composite = 0.35f * pi_score + 0.40f * reg_score + 0.25f * skew_score;
    result->overall_sqi = clamp01(composite);
    result->is_motion_artifact = (result->overall_sqi < PPG_SQI_THRESHOLD_VALID);
}

bool ppg_is_quality_acceptable(float sqi) {
    return (sqi >= PPG_SQI_THRESHOLD_VALID);
}
