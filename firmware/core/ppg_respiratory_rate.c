/*
 * ppg_respiratory_rate.c
 * Photoplethysmography-Derived Respiratory Rate (EDR) Implementation
 * Based on Charlton et al. (2018) & Addison (2014)
 * Smart India Hackathon 2026 - Project SIH26181
 */

#include "ppg_respiratory_rate.h"
#include <math.h>
#include <string.h>

/* Upper bound on the autocorrelation lag search (see max_lag below). Named
 * because the lag-correlation buffer has to be sized off it. */
#define MAX_ACF_LAGS 24

static float clamp_rr(float x) {
    if (x < PPG_RR_MIN_BPM) return PPG_RR_MIN_BPM;
    if (x > PPG_RR_MAX_BPM) return PPG_RR_MAX_BPM;
    return x;
}

/* Quadratic (parabolic) interpolation of an autocorrelation peak.
 *
 * The ACF is sampled at INTEGER beat lags and the period was formed as
 * `lag * mean_ibi`, so the reported rate could only ever land on the grid
 * 60000 / (k * mean_ibi). At the 880 ms IBI measured on real finger PPG that
 * grid is 22.7, 17.0, 13.6, 11.3, 9.7 br/min: steps of 3.4 br/min, with a
 * worst-case error of half a step at the midpoint between two lags.
 *
 * That is a clinical problem, not a cosmetic one. NEWS2's first abnormal
 * respiratory band starts at 21 br/min, and this file's own FM/AM agreement
 * test allows 3 br/min of disagreement between two independent estimates -
 * both finer than the instrument's own resolution. It is also why a paced run
 * at 15 br/min could not be told apart from a spontaneous 17: at 880 ms, 15
 * sits almost exactly between lag 4 (17.0) and lag 5 (13.6).
 *
 * The three correlations around the winner sample a parabola whose vertex lies
 * between them; its offset recovers the sub-lag peak. Clamped to +/-0.5 lag so
 * the refinement can sharpen the chosen peak but never migrate to another one.
 * A flat or inverted neighbourhood has no vertex to find and is left alone. */
static float interp_peak_lag(const float *r, int k, size_t max_lag) {
    if (k < 1 || (size_t)(k + 1) > max_lag) return (float)k;
    float rm = r[k - 1], r0 = r[k], rp = r[k + 1];
    float denom = rm - 2.0f * r0 + rp;
    if (fabsf(denom) < 1e-6f) return (float)k;
    float d = 0.5f * (rm - rp) / denom;
    if (d >  0.5f) d =  0.5f;
    if (d < -0.5f) d = -0.5f;
    return (float)k + d;
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
    if (max_lag > MAX_ACF_LAGS) max_lag = MAX_ACF_LAGS;
    if (max_lag < 3) max_lag = 3;

    int best_lag = -1;
    float best_r = -1.0f;

    /* The correlation at every lag, kept so the winner's peak can be
     * interpolated afterwards. A scalar running maximum cannot be refined once
     * the loop has moved past the neighbours it would need.
     *
     * Lag 1 is computed purely as a neighbour for the case best_lag == 2. Its
     * own implied rate (~68 br/min at rest) is far outside the band and it can
     * never win - the band gate below still excludes it from the search. */
    float r_lag[MAX_ACF_LAGS + 2];
    memset(r_lag, 0, sizeof(r_lag));

    if (max_lag >= 1) {
        float cross = 0.0f;
        for (size_t i = 0; i + 1 < N; i++) cross += y[i] * y[i + 1];
        r_lag[1] = cross / var_sum;
    }

    /* Search every lag up to max_lag, then let the band decide afterwards.
     *
     * The band used to gate the search, which was wrong in a way that mattered.
     * Excluding out-of-band lags does not make the estimator refuse a rate
     * outside the band - it makes it snap to the nearest admissible one and
     * report that. A genuine 8 br/min at a 880 ms IBI peaks at lag 8.5, which
     * was excluded, so the winner became lag 7 (9.74 br/min) and the parabola
     * clamped to +0.5, publishing 9.09 as a RELIABLE measurement. NEWS2 scores
     * <=8 as +3 and 9-11 as +1, so that is a severe bradypnoea reported as a
     * mild one - and 9.09 sits just inside the band, so the `clipped` guard
     * below never fired. The header claimed "below 9 br/min this now reports
     * unavailable"; it actually reported a confident wrong number, which is the
     * one outcome this file argues against everywhere else.
     *
     * Finding the real peak first and testing the band afterwards makes the
     * existing `clipped` check do what it was written to do. */
    for (size_t k = 2; k <= max_lag; k++) {
        float cross = 0.0f;
        for (size_t i = 0; i < N - k; i++) {
            cross += y[i] * y[i + k];
        }
        float r = cross / var_sum;
        r_lag[k] = r;

        if (r > best_r) {
            best_r = r;
            best_lag = (int)k;
        }
    }

    /* No admissible lag cleared the floor, so there is no peak to report. */
    if (best_r < 0.15f) {
        best_lag = -1;
    }

    /* Prefer the fundamental over its harmonics - see PPG_RR_HARMONIC_FRAC.
     * `best_lag` is the strongest admissible lag; this walks up from the
     * shortest and takes the first local maximum that is within the fraction. */
    int   peak_lag = best_lag;
    float peak_r   = best_r;
    if (best_lag > 0) {
        float frac_r = PPG_RR_HARMONIC_FRAC * best_r;
        for (size_t k = 2; k <= max_lag; k++) {
            if (r_lag[k] < frac_r) continue;
            bool local_max = (r_lag[k] >= r_lag[k - 1]) &&
                             (k + 1 > max_lag || r_lag[k] >= r_lag[k + 1]);
            if (local_max) {
                peak_lag = (int)k;
                peak_r   = r_lag[k];
                break;
            }
        }
    }

    float rr_from_fm = PPG_RR_DEFAULT_BPM;
    float conf_fm = 0.0f;

    if (peak_lag > 0) {
        float lag_f = interp_peak_lag(r_lag, peak_lag, max_lag);
        float breath_period_sec = (lag_f * mean_ibi) / 1000.0f;
        if (breath_period_sec > 1.2f && breath_period_sec < 12.0f) {
            rr_from_fm = 60.0f / breath_period_sec;
            conf_fm = (peak_r > 0.90f) ? 0.90f : peak_r;
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
            float ar_lag[MAX_ACF_LAGS + 2];
            memset(ar_lag, 0, sizeof(ar_lag));
            if (max_lag >= 1) {
                float cross = 0.0f;
                for (size_t i = 0; i + 1 < N; i++) cross += amp_y[i] * amp_y[i + 1];
                ar_lag[1] = cross / amp_var;
            }
            for (size_t k = 2; k <= max_lag; k++) {
                float cross = 0.0f;
                for (size_t i = 0; i < N - k; i++) {
                    cross += amp_y[i] * amp_y[i + k];
                }
                float r = cross / amp_var;
                ar_lag[k] = r;

                if (r > best_am_r) {
                    best_am_r = r;
                    best_am_lag = (int)k;
                }
            }
            /* Same 0.20 floor the search applied before it was restructured to
             * store every lag. */
            if (best_am_r < 0.20f) {
                best_am_lag = -1;
            }
            /* Same fundamental-over-harmonic preference as the FM path: this
             * estimate is fused with that one, so an octave error here would go
             * straight back into the published rate. */
            if (best_am_lag > 0) {
                float frac_am = PPG_RR_HARMONIC_FRAC * best_am_r;
                for (size_t k = 2; k <= max_lag; k++) {
                    if (ar_lag[k] < frac_am) continue;
                    bool am_local_max = (ar_lag[k] >= ar_lag[k - 1]) &&
                                        (k + 1 > max_lag || ar_lag[k] >= ar_lag[k + 1]);
                    if (am_local_max) {
                        best_am_lag = (int)k;
                        best_am_r   = ar_lag[k];
                        break;
                    }
                }
            }
            if (best_am_lag > 0) {
                /* Interpolated on the same reasoning as the FM path - this is the
                 * estimate that is fused with it, so leaving it quantised would
                 * put the grid straight back into the fused value. */
                float lag_am = interp_peak_lag(ar_lag, best_am_lag, max_lag);
                float period_am = (lag_am * mean_ibi) / 1000.0f;
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
