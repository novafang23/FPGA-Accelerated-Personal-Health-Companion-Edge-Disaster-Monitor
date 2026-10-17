#include "ppg_baseline_rr.h"

#include <math.h>
#include <string.h>

/* At 5 Hz these give roughly a 0.06 Hz high-pass and a 0.65 Hz low-pass. The
 * low-pass used to sit at 0.48 Hz, which attenuated 30 br/min (0.5 Hz) by ~67%
 * across the three stages and made the top of the range unreachable. */
#define PPG_BASE_HP_A  0.92f
#define PPG_BASE_LP_A  0.55f

void ppg_baseline_rr_init(ppg_baseline_rr_t *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
}

void ppg_baseline_rr_add(ppg_baseline_rr_t *s, float ir_sample) {
    if (!s) return;

    /* 1 s moving average at 25 Hz: removes the cardiac pulse and leaves the
     * baseline, which is what carries the respiratory modulation. */
    if (s->ma_n < PPG_BASE_RR_MA_LEN) {
        s->ma_buf[s->ma_n++] = ir_sample;
        s->ma_sum += ir_sample;
    } else {
        s->ma_sum += ir_sample - s->ma_buf[s->ma_idx];
        s->ma_buf[s->ma_idx] = ir_sample;
        s->ma_idx = (s->ma_idx + 1) % PPG_BASE_RR_MA_LEN;
    }
    if (s->ma_n < PPG_BASE_RR_MA_LEN) return;

    float baseline = s->ma_sum / (float)PPG_BASE_RR_MA_LEN;

    /* Track the DC level for the absolute-modulation gate. */
    if (s->base_mean <= 0.0f) {
        s->base_mean = baseline;
    } else {
        s->base_mean += 0.01f * (baseline - s->base_mean);
    }

    /* Decimate 25 Hz -> 5 Hz. */
    if (++s->decim < PPG_BASE_RR_DECIM) return;
    s->decim = 0;

    /* High-pass (one-pole DC blocker), then three cascaded one-pole low-passes.
     * Three stages keep the real baseline modulation above the amplitude gate;
     * six stages rejected the cardiac residual better but also attenuated the
     * genuine 6-12 br/min signal, dropping its confidence from ~0.55 to ~0.3 on
     * hardware. The subharmonic that motivated six stages is handled by the
     * harmonic guard below instead. */
    float hp = baseline - s->hp_x1 + PPG_BASE_HP_A * s->hp_y1;
    s->hp_x1 = baseline;
    s->hp_y1 = hp;

    s->lp_y1 += PPG_BASE_LP_A * (hp    - s->lp_y1);
    s->lp_y2 += PPG_BASE_LP_A * (s->lp_y1 - s->lp_y2);
    s->lp_y3 += PPG_BASE_LP_A * (s->lp_y2 - s->lp_y3);
    float lp = s->lp_y3;

    s->win[s->win_head] = lp;
    s->win_head = (s->win_head + 1) % PPG_BASE_RR_WIN;
    if (s->win_n < PPG_BASE_RR_WIN) s->win_n++;
}

bool ppg_baseline_rr_estimate(ppg_baseline_rr_t *s, float *rr_bpm, float *conf) {
    if (rr_bpm) *rr_bpm = 0.0f;
    if (conf) *conf = 0.0f;
    if (!s || s->win_n < 50) return false;   /* need >= 10 s of 5 Hz history */

    /* Linearise the ring, oldest first. */
    float y[PPG_BASE_RR_WIN];
    float mean = 0.0f;
    for (int i = 0; i < s->win_n; i++) {
        int idx = (s->win_head - s->win_n + i + PPG_BASE_RR_WIN) % PPG_BASE_RR_WIN;
        y[i] = s->win[idx];
        mean += y[i];
    }
    mean /= (float)s->win_n;

    float var = 0.0f;
    for (int i = 0; i < s->win_n; i++) {
        y[i] -= mean;
        var += y[i] * y[i];
    }
    if (var < 1e-6f) return false;

    /* Absolute modulation gate: the band-passed signal must be a real
     * modulation of the baseline, not the filter's residual cardiac pulse. At
     * 180 000 counts, 0.02% is ~36 counts; a 2000-count 1.2 Hz pulse leaves
     * ~15 counts after the 1 s average and the three low-pass stages, while a
     * real 0.1-0.5% respiratory modulation is 180-900 counts. */
    float rms = sqrtf(var / (float)s->win_n);
    if (s->base_mean > 0.0f && rms < 0.0002f * s->base_mean) return false;

    int lag_min = (int)(PPG_BASE_RR_FS * 60.0f / PPG_BASE_RR_MAX_BPM + 0.5f);
    int lag_max = (int)(PPG_BASE_RR_FS * 60.0f / PPG_BASE_RR_MIN_BPM + 0.5f);
    if (lag_min < 2) lag_min = 2;
    if (lag_max > s->win_n / 2) lag_max = s->win_n / 2;
    if (lag_max <= lag_min) return false;

    float acf[PPG_BASE_RR_WIN];
    float best_r = -2.0f;
    for (int lag = lag_min; lag <= lag_max; lag++) {
        float cross = 0.0f;
        for (int i = 0; i + lag < s->win_n; i++) cross += y[i] * y[i + lag];
        /* Normalise by the number of products, so a perfect sinusoid scores ~1
         * at every lag instead of penalising long lags. */
        float norm = var * (float)(s->win_n - lag) / (float)s->win_n;
        acf[lag] = (norm > 0.0f) ? (cross / norm) : 0.0f;
        if (acf[lag] > best_r) best_r = acf[lag];
    }
    if (best_r < 0.2f) return false;

    /* A periodic signal correlates equally well at every multiple of its
     * period, so the global maximum can land on a subharmonic: an 18 br/min
     * signal has equally strong ACF peaks at 3, 6 and 9 breaths per minute, and
     * the longest lag can win by a hair. Take the SHORTEST lag that is a local
     * maximum within 10% of the best - that is the fundamental. */
    int best_lag = -1;
    for (int lag = lag_min + 1; lag < lag_max; lag++) {
        if (acf[lag] >= 0.9f * best_r &&
            acf[lag] >= acf[lag - 1] &&
            acf[lag] >= acf[lag + 1]) {
            best_lag = lag;
            break;
        }
    }
    if (best_lag < 0) {
        for (int lag = lag_min; lag <= lag_max; lag++) {
            if (best_lag < 0 || acf[lag] > acf[best_lag]) best_lag = lag;
        }
    }
    if (best_lag < 0) return false;
    best_r = acf[best_lag];

    /* Harmonic guard. At 30 br/min the cardiac-pulse residual makes lag 2L
     * correlate higher than lag L, so the winner is the subharmonic (15 br/min).
     * If half the winner is itself a positive local maximum with at least half
     * the correlation, the shorter lag is the fundamental. A genuine slow rate
     * has a negative ACF at L/2, so it is not affected.
     *
     * Check the nearest half in BOTH directions: a 12 br/min signal at lag 25
     * was once read as lag 49 (6.1 br/min), and 49 is odd, so an even-only
     * check missed it. */
    {
        int half_lo = best_lag / 2;
        int half_hi = half_lo + 1;
        for (int attempt = 0; attempt < 2; attempt++) {
            int half = (attempt == 0) ? half_lo : half_hi;
            if (half >= lag_min && half < best_lag &&
                acf[half] > 0.0f &&
                acf[half] >= 0.5f * best_r &&
                acf[half] >= acf[half - 1] &&
                acf[half] >= acf[half + 1]) {
                best_lag = half;
                best_r   = acf[half];
                break;
            }
        }
    }

    /* A peak pinned to either edge of the search range is not a measurement:
     * it means the real periodicity is outside the band (or the loop fell back
     * to the global maximum at the boundary). Reporting it would publish the
     * boundary value - on hardware that showed up as a solid 30.0 br/min. */
    if (best_lag <= lag_min || best_lag >= lag_max) return false;

    s->last_lag = best_lag;
    s->last_acf = best_r;

    /* Parabolic interpolation around the winning lag. */
    float lag_f = (float)best_lag;
    if (best_lag > lag_min && best_lag < lag_max) {
        float rm = 0.0f, rp = 0.0f;

        float cross = 0.0f;
        for (int i = 0; i + (best_lag - 1) < s->win_n; i++) cross += y[i] * y[i + (best_lag - 1)];
        float norm = var * (float)(s->win_n - (best_lag - 1)) / (float)s->win_n;
        rm = (norm > 0.0f) ? (cross / norm) : 0.0f;

        cross = 0.0f;
        for (int i = 0; i + (best_lag + 1) < s->win_n; i++) cross += y[i] * y[i + (best_lag + 1)];
        norm = var * (float)(s->win_n - (best_lag + 1)) / (float)s->win_n;
        rp = (norm > 0.0f) ? (cross / norm) : 0.0f;

        float denom = rm - 2.0f * best_r + rp;
        if (fabsf(denom) > 1e-6f) {
            float delta = 0.5f * (rm - rp) / denom;
            if (delta > 0.5f) delta = 0.5f;
            if (delta < -0.5f) delta = -0.5f;
            lag_f = (float)best_lag + delta;
        }
    }
    if (lag_f <= 0.0f) return false;

    float rr = 60.0f * (float)PPG_BASE_RR_FS / lag_f;
    if (rr < PPG_BASE_RR_MIN_BPM || rr > PPG_BASE_RR_MAX_BPM) return false;

    if (rr_bpm) *rr_bpm = rr;
    if (conf) *conf = best_r;
    return true;
}
