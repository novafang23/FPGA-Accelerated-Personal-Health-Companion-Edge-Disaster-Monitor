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
#define HRV_MIN_SAMPLES 10   /* Minimum accepted IBIs before publishing HRV */
/* Window fill required before RMSSD may be published.
 *
 * Ultra-Short-Term (UST) HRV Settling Gate (Munoz 2015, Shaffer 2017):
 * RMSSD is a time-domain successive-difference metric reflecting parasympathetic
 * vagal tone. At resting breathing rates (12-18 breaths/min), one full respiratory
 * cycle lasts ~3.3 to 5.0 s (4-5 beats). A 20-beat window spans 4-5 full
 * respiratory sinus arrhythmia (RSA) cycles (~15-20 s), achieving Pearson r >= 0.88
 * correlation against standard 5-minute recordings and relative standard error
 * ~16% (sufficient to distinguish healthy autonomic reserve >= 35 ms from
 * autonomic shock < 8 ms by >6 sigma).
 *
 * Once 20 beats / 15 s are reached, hrv_is_settled() opens publication and unlocks
 * TinyML AI inference. As finger contact continues, the 300-beat buffer keeps
 * accumulating and refining via exponential moving average. HRV_MIN_SAMPLES (10)
 * remains the absolute floor used by hrv_is_ready() for structural validity.
 *
 * The two predicates are deliberately separate:
 *   hrv_is_ready()   - structural: enough contiguous pairs to compute a value.
 *                      Used by the risk engines and by callers that treat the
 *                      struct as a summary (they check the value itself).
 *   hrv_is_settled() - publication: the window holds at least 30 beats covering
 *                      at least 20 s of contact so RSA phase bias is stabilized.
 *                      Used by the publish paths that write g_state.hrv_rmssd. */
#define HRV_SETTLE_MIN_SAMPLES 30
/* Minimum time span the window must cover before RMSSD may be published.
 * Expressed in milliseconds of summed IBI, not wall-clock time: 20.0 s ensures
 * that even under extreme tachycardia (e.g. 120+ BPM where 20 beats take <10 s),
 * at least 3-4 respiratory cycles must elapse before publication. */
#define HRV_SETTLE_MIN_SPAN_MS 20000.0f

/* Maximum physiological beat-to-beat difference (Task Force 1996, Kamath 2012).
 * Sinus rhythm cannot accelerate/decelerate by >120 ms in a single beat without
 * an ectopic beat, missed peak, or optical motion artifact. Intervals exceeding
 * this jump are retained for Heart Rate but excluded from RMSSD squaring. */
#define HRV_MAX_DELTA_IBI_MS 120.0f

typedef struct {
    float ibi_ms[HRV_BUFFER_SIZE];
    /* True when this interval is contiguous with the previous accepted IBI.
     * RMSSD must not bridge an interval rejected by the beat-quality filter. */
    uint8_t pair_valid[HRV_BUFFER_SIZE];
    int   head;
    int   count;
    uint8_t has_previous_ibi;

    float rmssd;
    float sdnn;
    float mean_hr;
    float mean_ibi;
} hrv_state_t;

void  hrv_init(hrv_state_t *state);
void  hrv_add_ibi(hrv_state_t *state, float ibi_ms);
/* Mark a missing/rejected beat interval so RMSSD won't pair across the gap. */
void  hrv_break_sequence(hrv_state_t *state);
void  hrv_compute(hrv_state_t *state);

/* ---------------------------------------------------------------------------
 * IBI rhythm reference: causal median filter
 * ---------------------------------------------------------------------------
 * This filter estimates the subject's current rhythm from the last few
 * detected intervals. It is NOT applied to the intervals that enter the HRV
 * statistics: hrv_add_ibi() still receives the raw accepted interval. Artifact
 * rejection is done by the ratio/pair rules in ibi_pipeline_submit()
 * (main_shrikefi.c), using this median as the local reference.
 *
 * Measured on hardware: when the ForgeFPGA crest detector latches onto the
 * first of the two systolic maxima that a dicrotic notch separates
 * (~100-210 ms before the larger one), it emits one extra beat. That one
 * mis-detection produces a SHORT interval followed by a LONG one (measured:
 * 600 ms then 1010 ms against a ~800 ms rhythm). RMSSD squares successive
 * differences, so that pair alone drove RMSSD from ~40 ms of genuine
 * variability to ~116 ms; mixed with software-fallback intervals at the start
 * of a session it reached ~190 ms. The ratio/pair gate removes both halves
 * before they reach hrv_add_ibi().
 *
 * The median is deliberately causal and robust: a lone short/long pair cannot
 * move it, while a sustained genuine rate change can. It is fed by plausible
 * intervals that are not extreme outliers; see the reference-feed guard in
 * ibi_pipeline_submit(). The accepted interval is still published raw as
 * g_state.r_peak_interval_ms, so no raw data is lost.
 *
 * CAVEAT to state in any write-up: the ratio/pair gate removes some beats, so
 * the RMSSD is a slightly conservative estimate of the raw pulse series. The
 * median itself does not smooth the accepted intervals; it only sets the
 * reference used by that gate.
 *
 * WINDOW SIZE IS A ROBUSTNESS CHOICE, NOT ARBITRARY. It was 5, and on hardware
 * a run of five consecutive missed beats captured it: the median became an
 * artefact value, the caller's 0.83x floor then rejected the subject's real
 * beats and accepted the artefacts, and RMSSD went from 26 ms to 100+ ms.
 * A burst can only capture a window of N by supplying more than half of it, so
 * N=11 tolerates runs up to 5 - which covers what the detector actually
 * produces. Lengthening it further would track a genuine rate change more
 * slowly; at 78 BPM, 11 intervals is about 8 s.
 *
 * A partly-filled window is safe here ONLY because the caller feeds it with
 * every plausible interval (apart from the extreme-outlier guard), accepted or
 * rejected. Rejections cannot move it, so a wrong estimate self-corrects
 * instead of latching.
 *
 * WARNING - safe to use as an UPPER or LOWER bound only under that condition.
 * See the comment above IBI_MIN_MS in main_shrikefi.c for what happens when a
 * bound is derived from a window that rejections can shrink.
 */
#define HRV_MEDIAN_WINDOW 11   /* see below: 5 was not robust to a burst */
#define HRV_MEDIAN_MIN 3       /* pass the value through below this */
#define HRV_MEDIAN_MIN_REF 5   /* hrv_median_value() returns 0 below this */

typedef struct {
    float buf[HRV_MEDIAN_WINDOW];
    int   count;   /* valid entries, saturates at HRV_MEDIAN_WINDOW */
    int   head;    /* next write index */
} hrv_median_t;

void  hrv_median_init(hrv_median_t *m);
/* Insert one interval and return the median of the window including it.
 * Values are passed through unchanged until HRV_MEDIAN_MIN entries exist. */
float hrv_median_push(hrv_median_t *m, float ibi_ms);
/* Median of the reference window, or 0.0f until HRV_MEDIAN_MIN_REF entries
 * exist. Below that a caller falls back to absolute bounds.
 *
 * The caller feeds this from every plausible interval (apart from the
 * extreme-outlier guard in ibi_pipeline_submit()), accepted or rejected, so a
 * rejection cannot shrink the window and ratchet a bound. See the long comment
 * above HRV_MEDIAN_WINDOW for the window-size rationale and the measured
 * failure that motivated it. */
float hrv_median_value(const hrv_median_t *m);

float hrv_get_rmssd(const hrv_state_t *state);
float hrv_get_sdnn(const hrv_state_t *state);
float hrv_get_mean_hr(const hrv_state_t *state);
int   hrv_is_ready(const hrv_state_t *state);
/* Publication gate: non-zero only when the window holds HRV_SETTLE_MIN_SAMPLES
 * contiguous beats spanning HRV_SETTLE_MIN_SPAN_MS, i.e. the RMSSD has stopped
 * converging. See the HRV_SETTLE_* block above for why this is separate from
 * hrv_is_ready(). */
int   hrv_is_settled(const hrv_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* HRV_ANALYSIS_H */
