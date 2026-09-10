/*
 * ppg_respiratory_rate.c
 * Photoplethysmography-Derived Respiratory Rate (EDR) Implementation
 * Based on Charlton et al. (2018) & Addison (2014)
 * Smart India Hackathon 2026 - Project SIH26181
 */

#include "ppg_respiratory_rate.h"
#include <math.h>

static float clamp_rr(float x) {
    if (x < PPG_RR_MIN_BPM) return PPG_RR_MIN_BPM;
    if (x > PPG_RR_MAX_BPM) return PPG_RR_MAX_BPM;
    return x;
}

void ppg_estimate_respiratory_rate(
    const float *ibi_ms,
    const float *pulse_amplitudes,
    size_t beat_count,
    ppg_respiratory_result_t *result
) {
    if (!result) return;

    /* Fallback default */
    result->respiratory_rate_bpm = PPG_RR_DEFAULT_BPM;
    result->confidence = 0.30f;
    result->rsa_depth_ms = 0.0f;
    result->is_reliable = false;

    if (!ibi_ms || beat_count < 8) {
        return;
    }

    /* 1. Calculate mean IBI and detrend */
    float ibi_sum = 0.0f;
    for (size_t i = 0; i < beat_count; i++) {
        ibi_sum += ibi_ms[i];
    }
    float mean_ibi = ibi_sum / (float)beat_count;
    if (mean_ibi <= 100.0f || mean_ibi >= 2500.0f) {
        return; /* Unrealistic physiological IBI */
    }

    /* Detrended IBI array */
    #define MAX_BEATS_BUF 64
    size_t N = (beat_count > MAX_BEATS_BUF) ? MAX_BEATS_BUF : beat_count;
    float y[MAX_BEATS_BUF];
    float min_y = 1e6f, max_y = -1e6f;
    float var_sum = 0.0f;

    for (size_t i = 0; i < N; i++) {
        y[i] = ibi_ms[i] - mean_ibi;
        if (y[i] < min_y) min_y = y[i];
        if (y[i] > max_y) max_y = y[i];
        var_sum += y[i] * y[i];
    }

    result->rsa_depth_ms = (max_y - min_y);

    if (var_sum < 1e-3f) {
        /* Zero variance in IBI */
        return;
    }

    /* 2. Autocorrelation of IBI sequence (Frequency Modulation / RSA) */
    size_t max_lag = N / 2;
    if (max_lag > 24) max_lag = 24;
    if (max_lag < 3) max_lag = 3;

    int best_lag = -1;
    float best_r = -1.0f;

    /* Search lags corresponding to 6 to 36 breaths/min.
     * With mean_ibi in ms, lag k beats corresponds to period = k * mean_ibi / 1000 sec.
     * RR = 60 / period = 60000 / (k * mean_ibi).
     * So k = 60000 / (RR * mean_ibi).
     */
    for (size_t k = 2; k <= max_lag; k++) {
        float cross = 0.0f;
        for (size_t i = 0; i < N - k; i++) {
            cross += y[i] * y[i + k];
        }
        float r = cross / var_sum;

        /* Check for local peak */
        if (r > best_r && r > 0.15f) {
            best_r = r;
            best_lag = (int)k;
        }
    }

    float rr_from_fm = PPG_RR_DEFAULT_BPM;
    float conf_fm = 0.0f;

    if (best_lag > 0) {
        float breath_period_sec = ((float)best_lag * mean_ibi) / 1000.0f;
        if (breath_period_sec > 1.2f && breath_period_sec < 12.0f) {
            rr_from_fm = 60.0f / breath_period_sec;
            conf_fm = (best_r > 0.90f) ? 0.90f : best_r;
        }
    }

    /* 3. Amplitude Modulation (AM) analysis if amplitudes are provided */
    float rr_from_am = rr_from_fm;
    float conf_am = 0.0f;

    if (pulse_amplitudes) {
        float amp_sum = 0.0f;
        for (size_t i = 0; i < N; i++) amp_sum += pulse_amplitudes[i];
        float mean_amp = amp_sum / (float)N;

        float amp_y[MAX_BEATS_BUF];
        float amp_var = 0.0f;
        for (size_t i = 0; i < N; i++) {
            amp_y[i] = pulse_amplitudes[i] - mean_amp;
            amp_var += amp_y[i] * amp_y[i];
        }

        if (amp_var > 1e-3f) {
            int best_am_lag = -1;
            float best_am_r = -1.0f;
            for (size_t k = 2; k <= max_lag; k++) {
                float cross = 0.0f;
                for (size_t i = 0; i < N - k; i++) {
                    cross += amp_y[i] * amp_y[i + k];
                }
                float r = cross / amp_var;
                if (r > best_am_r && r > 0.20f) {
                    best_am_r = r;
                    best_am_lag = (int)k;
                }
            }
            if (best_am_lag > 0) {
                float period_am = ((float)best_am_lag * mean_ibi) / 1000.0f;
                if (period_am > 1.2f && period_am < 12.0f) {
                    rr_from_am = 60.0f / period_am;
                    conf_am = best_am_r;
                }
            }
        }
    }

    /* 4. Fusion of FM and AM estimations */
    float final_rr;
    float final_conf;

    if (conf_am > 0.20f && conf_fm > 0.20f) {
        /* Weighted combination */
        float total_w = conf_fm + conf_am;
        final_rr = (rr_from_fm * conf_fm + rr_from_am * conf_am) / total_w;
        final_conf = (conf_fm + conf_am) * 0.55f;
    } else if (conf_fm > 0.20f) {
        final_rr = rr_from_fm;
        final_conf = conf_fm;
    } else if (conf_am > 0.20f) {
        final_rr = rr_from_am;
        final_conf = conf_am;
    } else {
        final_rr = PPG_RR_DEFAULT_BPM;
        final_conf = 0.25f;
    }

    /* Adjust confidence based on RSA amplitude depth */
    if (result->rsa_depth_ms < 15.0f && final_conf > 0.50f) {
        final_conf *= 0.85f; /* Shallow autonomic modulation */
    }

    if (final_conf > 1.0f) final_conf = 1.0f;

    result->respiratory_rate_bpm = clamp_rr(final_rr);
    result->confidence = final_conf;
    result->is_reliable = (final_conf >= 0.60f);
}
