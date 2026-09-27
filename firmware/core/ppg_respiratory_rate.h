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

/* Physiological floor. Currently 6.0 - TEMPORARY, for the breath-hold test at the
 * end of this block; it was raised from 6.0 to 9.0 on the evidence described below.
 * Rates outside the band are rejected, and because a value that clamp_rr() has to
 * move is never certified, they publish as unavailable rather than as a
 * bradypnoea finding.
 *
 * WHY 9 AND NOT 6
 * A capture with a finger on the sensor published 6.6-7.3 br/min with a median
 * RSA depth of 158 ms peak-to-peak. Adult RESPIRATORY RSA is 20-60 ms. Three to
 * five times that, with the rate pinned at the very bottom of the band, is the
 * signature of the ~0.1 Hz MAYER WAVE - a genuine, strong, periodic
 * blood-pressure oscillation in the inter-beat interval that is not breathing.
 * A floor of 6 put Mayer waves inside the accepted range instead of outside it.
 *
 * The floor is set at 9 rather than tuned to the observation because 9 is where
 * NEWS2's respiratory term stops being the severe band (<=8 scores +3, 9-11
 * scores +1). The device cannot distinguish slow breathing from a vasomotor
 * oscillation using the inter-beat interval alone - that is a known limitation of
 * respiration-from-IBI, and it is worst at exactly the rates where the clinical
 * penalty is largest. So the rule is: do not score in the band where you cannot
 * discriminate. Below 9 br/min this now reports "unavailable", and NEWS2 holds
 * the respiratory term neutral rather than claiming bradypnoea.
 *
 * THE COST, STATED PLAINLY: genuine breathing at 6-9 br/min will no longer be
 * reported. That was not confirmed by paced breathing - the one test that would
 * settle it (15/min for 60 s, to see whether the rate follows) could not be
 * completed, because repeated captures failed on finger contact. This is a
 * judgement from the depth evidence, not a measurement, and it is reversible in
 * one line if paced breathing later shows the estimator does track respiration.
 *
 * TEMPORARILY 6.0 FOR THE BREATH-HOLD TEST (2026-09-28). Paced breathing has since
 * PROVEN that the estimator tracks respiration (cued 10 -> reported 11.3, cued 24 ->
 * 26.9 in one run), which removes the main reason for the floor and removes the need
 * for the judgement above. What is still unknown is whether the Mayer wave that
 * justified it is real at all - the original evidence is equally consistent with an
 * octave error that has since been fixed.
 *
 * The floor is temporarily at 6 so that a 6-7 br/min oscillation CAN be reported. At
 * a floor of 9 the estimator would refuse it and the test could not tell the two
 * apart. After the test: if a steady ~6-7 br/min appears while the subject is holding
 * their breath (no respiration, so no respiratory sinus arrhythmia) it is a Mayer wave
 * and the floor goes back to 9. If the output goes quiet or erratic during the hold,
 * there is no Mayer wave to guard against and 6.0 becomes the permanent value.
 *
 * The original reasoning, retained because it is what the floor encodes: */
#define PPG_RR_MIN_BPM       6.0f    /* TEMPORARY - breath-hold test; was 9.0f */
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

/* Harmonic preference.
 *
 * A periodic series correlates at every MULTIPLE of its period, so the
 * autocorrelation has a peak at the fundamental lag L and again at 2L, 3L, ...
 * In the continuous limit those peaks are all the same height and the choice
 * between them is free. Sampled at integer beat lags they are not: whenever L
 * is not close to an integer, the sampled value at L is pulled down while a
 * harmonic can still land near an integer and score close to full height.
 *
 * Concretely, at the 800 ms IBI used in the tests a clean 21.9 br/min
 * modulation has L = 3.43 beats, which scores 0.68 at lag 3 but 0.89 at lag 7 -
 * so a subject breathing nearly 22 times a minute was reported as breathing
 * 10.9. Taking the global maximum is not a choice between equal evidence; it is
 * a systematic bias toward reporting half the true rate.
 *
 * The autocorrelation cannot separate them, so this needs a prior, which is the
 * same kind PPG_RR_MIN_BPM and PPG_RR_MAX_BPM already encode. Of the lags that
 * are local maxima and reach this fraction of the strongest admissible one, take
 * the LOWEST - the fundamental. Below it a genuine fast rate is missed; too low
 * and noise peaks start winning, so it is set from the measured ratio above
 * (0.68/0.89 = 0.77) with margin for a noisier series. */
#define PPG_RR_HARMONIC_FRAC 0.60f

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

/* --- Published-rate stabilisation -----------------------------------------
 *
 * ppg_estimate_respiratory_rate() is stateless and re-decides on every call. On
 * hardware that showed up as the rate appearing in 4 frames out of 118 -
 * confidence sitting right on the 0.60 bar and crossing it occasionally rather
 * than settling. That flickers the display and, because NEWS2 scores the band
 * edges most steeply, flips the whole triage between NORMAL and MODERATE frame
 * to frame. A rate that appears one second and vanishes the next is worse than
 * no rate at all.
 *
 * This tracker applies two things the raw estimate cannot:
 *
 *   hysteresis  - a published rate is HELD until confidence falls decisively
 *                 below the bar (PPG_RR_REL_OFF), not merely back to it, so a
 *                 value hovering at the boundary does not blink;
 *   a median    - the published value is the median of the recent accepted
 *                 estimates, so one noisy frame cannot move the number.
 *
 * The hold is BOUNDED (PPG_RR_HOLD_MAX). An unbounded hold would be the same
 * stale-value-published-as-valid defect this codebase has had to remove from
 * SpO2: a rate must not keep being reported as current once the evidence for it
 * has gone. */
#define PPG_RR_TRACK_HISTORY 5      /* odd, so the median is a real sample */
#define PPG_RR_REL_OFF       0.45f  /* hysteresis floor: below this, retract */
#define PPG_RR_HOLD_MAX      10     /* frames; at the 1 Hz call site, 10 s */

typedef struct {
    float hist[PPG_RR_TRACK_HISTORY];
    float last_conf;   /* confidence at the last accepted estimate */
    int   n;           /* entries in hist */
    int   head;        /* next write index */
    int   hold;        /* consecutive frames published without a fresh accept */
    bool  holding;     /* a rate is currently being published */
} ppg_rr_tracker_t;

void ppg_rr_tracker_init(ppg_rr_tracker_t *t);

/* Apply hysteresis and smoothing to a fresh estimate, in place.
 *
 * On return, respiratory_rate_bpm and is_reliable hold the PUBLISHED verdict,
 * which may be a held value from an earlier frame. confidence is set to the
 * confidence that verdict rests on, so "is_reliable implies confidence >=
 * PPG_RR_CONF_MIN" stays true for consumers. Call once per estimator run. */
void ppg_rr_tracker_update(ppg_rr_tracker_t *t, ppg_respiratory_result_t *rr);

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
