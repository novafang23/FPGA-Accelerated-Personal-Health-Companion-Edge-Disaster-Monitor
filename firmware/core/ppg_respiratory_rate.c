/*
 * ppg_respiratory_rate.c
 * Photoplethysmography-Derived Respiratory Rate (EDR) Implementation
 * Based on Charlton et al. (2018) & Addison (2014)
 * Smart India Hackathon 2026 - Project SIH26181
 */

#include "ppg_respiratory_rate.h"
#include <math.h>
#include <string.h>

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

    for (size_t i = 0; i < N; i++) {
        y[i] = ibi_ms[i] - mean_ibi;
    }

    /* Remove the best-fit straight line, not just the mean.
     *
     * A slow drift in heart rate is not respiration, but it correlates strongly
     * with itself at short lags and therefore wins the autocorrelation search.
     * On the measured reproduction - a healthy 78 bpm subject with 55 ms of drift
     * and NO respiratory modulation at all - mean removal alone let lag 3 win and
     * the estimator published 26 br/min. Adding a depth term then made that
     * confident (0.97) rather than merely plausible, which is worse: a confident
     * wrong number in NEWS2's most sensitive term gets acted on.
     *
     * Detrending first is what separates a trend from an oscillation, and it is
     * also what makes the depth measure meaningful - respiration is the residual
     * ripple, not the drift underneath it. */
    float idx_mean = 0.0f;
    for (size_t i = 0; i < N; i++) idx_mean += (float)i;
    idx_mean /= (float)N;

    float stt = 0.0f, sty = 0.0f;
    for (size_t i = 0; i < N; i++) {
        float dt = (float)i - idx_mean;
        stt += dt * dt;
        sty += dt * y[i];
    }
    float slope = (stt > 1e-6f) ? (sty / stt) : 0.0f;

    float var_sum = 0.0f;
    for (size_t i = 0; i < N; i++) {
        y[i] -= slope * ((float)i - idx_mean);
        var_sum += y[i] * y[i];
    }

    /* RSA depth as an EQUIVALENT SINUSOID peak-to-peak over the DETRENDED series,
     * from the standard deviation: pp = 2*sqrt(2)*sigma.
     *
     * This used to be a raw max-minus-min. A range is set by the single worst
     * pair of beats in the window and grows with sample count, so thirty beats
     * of pure jitter produced the same "30 ms of RSA" as a genuine 30 ms
     * respiratory modulation - exactly the confusion the depth term exists to
     * prevent. The standard deviation uses every sample and is stable in N.
     *
     * For a true sinusoid this recovers the peak-to-peak exactly, since
     * sigma = pp / (2*sqrt(2)); the constant is algebra, not a fudge factor. */
    float sigma = sqrtf(var_sum / (float)N);
    result->rsa_depth_ms = 2.0f * 1.41421356f * sigma;

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
        /* Reject a lag whose implied rate is outside the physiological band
         * BEFORE it can win the search.
         *
         * The band was previously applied after the winner was chosen, and to
         * the breath *period*, which is far too permissive. Lag 2 at a 770 ms
         * mean IBI is a 1.54 s period - inside the accepted 1.2-12 s window -
         * but it is 38.96 br/min, above PPG_RR_MAX_BPM. clamp_rr() then pinned
         * that to exactly 36.00 and the result was published as reliable, so a
         * healthy resting subject with no respiratory modulation could be
         * reported as breathing at the tachypnoea ceiling, which sets
         * is_absolute_crisis and bypasses the SQI hold on the way to a CRITICAL
         * triage. A rate that only exists because we clamped it is not a rate
         * we measured. */
        float rr_k = 60000.0f / ((float)k * mean_ibi);
        if (rr_k < PPG_RR_MIN_BPM || rr_k > PPG_RR_MAX_BPM) {
            continue;
        }

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
                /* Same band rejection as the FM search above. */
                float rr_k = 60000.0f / ((float)k * mean_ibi);
                if (rr_k < PPG_RR_MIN_BPM || rr_k > PPG_RR_MAX_BPM) {
                    continue;
                }
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

    /* 4. Fusion of the FM and AM estimates.
     *
     * The point estimate still prefers the sharper evidence, but the CONFIDENCE
     * is no longer taken from here - see step 5. */
    float final_rr;
    float acf_conf;   /* periodicity evidence only; one input to the confidence */

    if (conf_am > 0.20f && conf_fm > 0.20f) {
        float total_w = conf_fm + conf_am;
        final_rr = (rr_from_fm * conf_fm + rr_from_am * conf_am) / total_w;
        acf_conf = (conf_fm + conf_am) * 0.55f;
    } else if (conf_fm > 0.20f) {
        final_rr = rr_from_fm;
        acf_conf = conf_fm;
    } else if (conf_am > 0.20f) {
        final_rr = rr_from_am;
        acf_conf = conf_am;
    } else {
        final_rr = PPG_RR_DEFAULT_BPM;
        acf_conf = 0.0f;
    }

    /* 5. Confidence, rebuilt around the two things that actually indicate a real
     * respiratory component rather than one weak proxy for both.
     *
     * The old confidence WAS the autocorrelation coefficient. On real finger PPG
     * that sits around 0.3-0.5 - RSA modulates the IBI series by tens of
     * milliseconds on top of beat-to-beat variation of similar size - so it never
     * reached the 0.60 bar and RR was published as unavailable on every real
     * contact, even with 94 accepted intervals behind it.
     *
     * Periodicity alone is also not enough in the other direction: random jitter
     * produces a modest ACF peak at some lag with nothing periodic behind it. So
     * the two questions are scored separately and both must be answered:
     *
     *   acf_conf   - is the series periodic at the winning lag?
     *   depth_conf - is the modulation big enough to be respiratory?
     *
     * and agreement between the independent FM and AM paths is credited on top. */

    /* Scale the raw coefficient onto the range that carries information: below
     * ACF_MIN there is nothing to find, at ACF_STRONG the peak is unambiguous. */
    float acf_scaled = (acf_conf - PPG_RR_ACF_MIN) / (PPG_RR_ACF_STRONG - PPG_RR_ACF_MIN);
    if (acf_scaled < 0.0f) acf_scaled = 0.0f;
    if (acf_scaled > 1.0f) acf_scaled = 1.0f;

    float depth_scaled =
        (result->rsa_depth_ms - PPG_RSA_DEPTH_NONE_MS) /
        (PPG_RSA_DEPTH_CLEAR_MS - PPG_RSA_DEPTH_NONE_MS);
    if (depth_scaled < 0.0f) depth_scaled = 0.0f;
    if (depth_scaled > 1.0f) depth_scaled = 1.0f;

    float final_conf = 0.5f * acf_scaled + 0.5f * depth_scaled;

    /* Corroboration: two independent measurements of the same physiology landing
     * on the same rate is evidence that neither coefficient captures. */
    bool fm_am_agree = (conf_am > 0.20f && conf_fm > 0.20f &&
                        fabsf(rr_from_fm - rr_from_am) <= PPG_RR_AGREE_BPM);
    if (fm_am_agree) {
        final_conf += PPG_RR_AGREE_BONUS;
    }

    if (final_conf > 1.0f) final_conf = 1.0f;

    /* Belt and braces: the AM path and the FM/AM fusion can still land outside
     * the band, so refuse to certify any value clamp_rr() had to move. */
    bool clipped = (final_rr < PPG_RR_MIN_BPM || final_rr > PPG_RR_MAX_BPM);

    result->respiratory_rate_bpm = clamp_rr(final_rr);
    result->confidence = final_conf;
    result->is_reliable = (final_conf >= PPG_RR_CONF_MIN) && !clipped;
}

/* ------------------------------------------------------------------------- */
/* Published-rate stabilisation                                               */
/* ------------------------------------------------------------------------- */

void ppg_rr_tracker_init(ppg_rr_tracker_t *t) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
}

static void rr_hist_push(ppg_rr_tracker_t *t, float v) {
    t->hist[t->head] = v;
    t->head = (t->head + 1) % PPG_RR_TRACK_HISTORY;
    if (t->n < PPG_RR_TRACK_HISTORY) t->n++;
}

static float rr_hist_median(const ppg_rr_tracker_t *t) {
    if (t->n <= 0) return 0.0f;

    float s[PPG_RR_TRACK_HISTORY];
    memcpy(s, t->hist, sizeof(s));
    /* Insertion sort: five elements, and it keeps the function free of any
     * allocation or qsort callback. */
    for (int i = 1; i < t->n; i++) {
        float key = s[i];
        int j = i - 1;
        while (j >= 0 && s[j] > key) { s[j + 1] = s[j]; j--; }
        s[j + 1] = key;
    }
    return s[t->n / 2];
}

void ppg_rr_tracker_update(ppg_rr_tracker_t *t, ppg_respiratory_result_t *rr) {
    if (!t || !rr) return;

    if (rr->is_reliable) {
        /* A fresh estimate above the bar: accept it and restart the hold. */
        rr_hist_push(t, rr->respiratory_rate_bpm);
        t->last_conf = rr->confidence;
        t->holding = true;
        t->hold = 0;
    } else if (t->holding && t->n > 0 && t->hold < PPG_RR_HOLD_MAX &&
               rr->confidence >= PPG_RR_REL_OFF) {
        /* Between the hysteresis floor and the bar. Weak, but not decisively
         * bad, so hold: report the smoothed value rather than blinking off. */
        t->hold++;
    } else {
        /* Decisively below the floor, or the hold has run out. Retract, and
         * drop the history so the next reliable episode starts from its own
         * evidence rather than inheriting a stale rate. */
        t->holding = false;
        t->hold = 0;
        t->n = 0;
        t->head = 0;
        rr->is_reliable = false;
        return;
    }

    /* Published value is the median of accepted estimates, so one noisy frame
     * cannot move the number. */
    rr->respiratory_rate_bpm = rr_hist_median(t);
    /* Report the confidence the published value rests on, not this frame's, so
     * "is_reliable implies confidence >= PPG_RR_CONF_MIN" stays true. */
    rr->confidence = t->last_conf;
    rr->is_reliable = true;
}
