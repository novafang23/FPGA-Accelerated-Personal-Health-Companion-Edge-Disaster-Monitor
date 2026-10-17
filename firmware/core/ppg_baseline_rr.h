/*
 * ppg_baseline_rr.h — Respiratory rate from the PPG BASELINE (DC) modulation.
 *
 * Why this exists
 * ---------------
 * The IBI/RSA estimator and the beat-amplitude (AM) estimator are both
 * constrained by the beat rate: the amplitude is sampled once per beat (~1 Hz)
 * and the IBI is quantised by the SPI poll cadence (~10 ms). The PPG baseline,
 * by contrast, is available at the full 25 Hz FIFO rate and is modulated by
 * respiration (intrathoracic pressure, venous return, peripheral resistance)
 * at 0.1-0.5 Hz. That is comfortably inside its bandwidth.
 *
 * This module:
 *   1. removes the cardiac pulse with a 1 s moving average (the baseline),
 *   2. decimates to 5 Hz,
 *   3. high-pass (DC blocker) then low-pass to 0.1-0.5 Hz,
 *   4. autocorrelates a 30 s window to find the respiratory period.
 *
 * It is a third, independent estimate. The caller publishes it when the
 * IBI/AM estimator is unavailable or weak.
 */

#ifndef PPG_BASELINE_RR_H
#define PPG_BASELINE_RR_H

#include <stdbool.h>
#include <stdint.h>

#define PPG_BASE_RR_SAMPLE_HZ  100                                 /* FIFO rate */
#define PPG_BASE_RR_FS         5                                   /* decimated rate */
#define PPG_BASE_RR_DECIM      (PPG_BASE_RR_SAMPLE_HZ / PPG_BASE_RR_FS) /* 20 */
#define PPG_BASE_RR_MA_LEN     PPG_BASE_RR_SAMPLE_HZ               /* 1 s baseline */
#define PPG_BASE_RR_WIN        150                                 /* 30 s at 5 Hz */

#define PPG_BASE_RR_MIN_BPM    6.0f
/* Same publication ceiling as the IBI/AM estimator: a rate above 24 br/min is
 * the range where the false-alarm history lives, so the baseline path must not
 * be the one that breaks that policy. */
#define PPG_BASE_RR_MAX_BPM    24.0f
/* Publication bar for the ACF confidence. This is NOT the IBI estimator's 0.60:
 * that number is a blend of periodicity and RSA depth, while this is the raw
 * autocorrelation peak of a band-passed signal. A paced 12 br/min test produced
 * a stable 13.0-13.4 br/min at ACF 0.48-0.57, so 0.50 is the calibrated value
 * for THIS metric. It is only applied to estimates that already passed the
 * modulation gate and the boundary check. */
#define PPG_BASE_RR_CONF_MIN   0.50f

typedef struct {
    /* 1 s moving average over the 25 Hz input */
    float   ma_buf[PPG_BASE_RR_MA_LEN];
    int     ma_idx;
    int     ma_n;
    float   ma_sum;

    int     decim;
    /* high-pass (DC blocker) then a 3-stage low-pass, all at 5 Hz */
    float   hp_x1, hp_y1;
    float   lp_y1, lp_y2, lp_y3;

    /* Slow mean of the baseline, used to reject a signal whose only content is
     * the residual cardiac pulse (see ppg_baseline_rr_estimate). */
    float   base_mean;

    float   win[PPG_BASE_RR_WIN];
    int     win_n;
    int     win_head;

    /* Diagnostics from the last estimate, so the caller can log what the
     * autocorrelation actually locked onto. */
    int     last_lag;
    float   last_acf;
} ppg_baseline_rr_t;

void ppg_baseline_rr_init(ppg_baseline_rr_t *s);

/* Feed one raw IR sample at PPG_BASE_RR_SAMPLE_HZ. */
void ppg_baseline_rr_add(ppg_baseline_rr_t *s, float ir_sample);

/* Estimate the respiratory rate. Returns false when there is not enough
 * history or no credible periodicity. On true, *rr_bpm is in
 * [PPG_BASE_RR_MIN_BPM, PPG_BASE_RR_MAX_BPM] and *conf is the ACF peak. */
bool ppg_baseline_rr_estimate(ppg_baseline_rr_t *s, float *rr_bpm, float *conf);

#endif /* PPG_BASELINE_RR_H */
