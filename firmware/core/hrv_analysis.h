/*
 * hrv_analysis.h
 * Heart Rate Variability Analysis Module
 */

#ifndef HRV_ANALYSIS_H
#define HRV_ANALYSIS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HRV_BUFFER_SIZE 300  /* Rolling window of last 300 IBI intervals (~5 min at 60 BPM) */
#define HRV_MIN_SAMPLES 10   /* Minimum samples for reliable RMSSD/SDNN (responsive warm-up) */

typedef struct {
    float ibi_ms[HRV_BUFFER_SIZE];
    int   head;
    int   count;

    float rmssd;
    float sdnn;
    float mean_hr;
    float mean_ibi;
} hrv_state_t;

void  hrv_init(hrv_state_t *state);
void  hrv_add_ibi(hrv_state_t *state, float ibi_ms);
void  hrv_compute(hrv_state_t *state);

/* ---------------------------------------------------------------------------
 * IBI artifact correction: causal 5-point median filter
 * ---------------------------------------------------------------------------
 * Measured on real hardware: when the ForgeFPGA's crest detector latches onto
 * the first of the two systolic local maxima that a dicrotic notch separates
 * (~100-210 ms before the larger second one), it emits one extra beat. That one
 * mis-detection produces a SHORT interval followed by a LONG one (measured:
 * 600 ms then 1010 ms against a ~800 ms rhythm). RMSSD squares successive
 * differences, so that single pair alone drove RMSSD from ~40 ms of genuine
 * variability to ~116 ms; mixed with software-fallback intervals at the start of
 * the session it reached ~190 ms.
 *
 * Median filtering of the interval series before HRV metrics are computed is
 * standard practice for exactly this reason (it is the usual "remove ectopic
 * beats and artifacts" step in HRV preprocessing). It is a rank filter, so a
 * lone short/long pair cannot survive it, while a run of genuinely short or
 * genuinely long beats can.
 *
 * CAVEAT to state in any write-up: median filtering also attenuates real
 * beat-to-beat variability, so the RMSSD it produces is a slightly conservative
 * (low-biased) estimate. The unfiltered interval is still published separately
 * as g_state.r_peak_interval_ms, so no raw data is lost.
 */
#define HRV_MEDIAN_WINDOW 5

typedef struct {
    float buf[HRV_MEDIAN_WINDOW];
    int   count;   /* valid entries, saturates at HRV_MEDIAN_WINDOW */
    int   head;    /* next write index */
} hrv_median_t;

void  hrv_median_init(hrv_median_t *m);
/* Insert one interval and return the median of the window including it.
 * Values are passed through unchanged until HRV_MEDIAN_MIN entries exist. */
float hrv_median_push(hrv_median_t *m, float ibi_ms);

#define HRV_MEDIAN_MIN 3

float hrv_get_rmssd(const hrv_state_t *state);
float hrv_get_sdnn(const hrv_state_t *state);
float hrv_get_mean_hr(const hrv_state_t *state);
int   hrv_is_ready(const hrv_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* HRV_ANALYSIS_H */
