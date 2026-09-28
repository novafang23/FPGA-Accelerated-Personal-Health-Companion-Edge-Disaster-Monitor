/*
 * spo2_engine.h
 * Pulse Oximetry (SpO2) Estimation Module
 */
#ifndef SPO2_ENGINE_H
#define SPO2_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Samples per measurement window. The optical link delivers ~100 samples/s
 * (measured ~105/s), so this is ~0.5 s per entry, not the 1.0 s an earlier
 * 50 Hz assumption gave. */
#define SPO2_WINDOW_SIZE 50
#define SPO2_MA_FILTER_SIZE 8 /* Moving average history size (8-second smoothing window) */

/* Clinical validity thresholds calibrated for MAX30102 18-bit optical levels */
#define SPO2_MIN_AC_IR              10.0f  /* Minimum pulsatile AC amplitude for IR (counts) */
#define SPO2_MIN_AC_RED              8.0f  /* Minimum pulsatile AC amplitude for Red (counts) */
#define SPO2_MIN_DC_IR            1000.0f  /* Minimum optical DC level for IR (counts, ambient air is <600) */
#define SPO2_MIN_DC_RED            800.0f  /* Minimum optical DC level for Red (counts) */
#define SPO2_MIN_PERFUSION_INDEX     0.15f /* Minimum Perfusion Index (0.15%) */
#define SPO2_MAX_PERFUSION_INDEX    15.0f  /* Maximum Perfusion Index to reject motion (15.0%) */
#define SPO2_MIN_RATIO_R             0.35f /* Physiological lower bound for R (approx 100% SpO2) */
#define SPO2_MAX_RATIO_R             1.65f /* Physiological upper bound for R (approx 68% SpO2) */

/* Acquisition gate.
 *
 * This used to be 1: a single valid one-second window latched `valid` and
 * published the value. Because SPO2_MA_FILTER_SIZE is 8, the moving average is
 * not even fully primed after one window - and the underlying R ratio keeps
 * drifting for far longer than that, while max30102_adjust_led_current()
 * settles the DC baseline. Measured on hardware, the reported value climbed
 * from 78% to 96% over about 30 seconds, all of it published as VALID. A real
 * 78% is a life-threatening desaturation, so that is not a cosmetic problem.
 *
 * A fixed window count cannot fix it either: the settling time depends on the
 * finger and the LED current, not on a constant. Too low publishes an artefact
 * as an emergency; too high hides genuine hypoxia, which is the worse failure
 * for SpO2. So the gate asks the question that actually matters - has the
 * smoothed estimate stopped moving? - and adapts to however long convergence
 * takes.
 *
 * The comparison must span the FULL moving-average depth, not the last few
 * windows. Against a synthetic re-acquisition ramp, a three-window comparison
 * latched at window 5 at 87.45% - better than the 78% it replaced, but 87% is
 * still significant hypoxaemia. The drift decelerates as it converges, so any
 * short baseline eventually looks flat while the absolute value is still far
 * from settled. Eight windows at a 2% tolerance clears the real transient
 * (about 0.55%/s on hardware, so roughly 4.4% across the window) and still
 * latches on a genuinely settled signal. */
#define SPO2_REQUIRED_VALID_WINDOWS  8     /* Consecutive valid windows before latching */
#define SPO2_STABLE_MIN_WINDOWS      8     /* Windows compared for flatness (= MA depth) */
#define SPO2_STABLE_SPREAD_PCT       2.0f  /* Max spread across those windows, in SpO2 % */

typedef struct {
    /* Per-window accumulators for the mean (DC) and standard deviation (AC).
     *
     * These replaced min/max tracking. Peak-to-peak is decided by exactly TWO of
     * the 50 samples - the single highest and the single lowest - so it is the
     * noisiest possible amplitude estimate, and the ratio of two noisy channels is
     * what SpO2 is computed from. Measured on hardware: with max-min the raw
     * reading wobbled about +/-2.5% window to window, giving an 8-window spread of
     * 4-5% against the 2% settle bar - LARGER than the 2.4% drift that bar exists
     * to catch, so the gate could never pass and acquisition took 20+ s despite 46
     * consecutive valid windows.
     *
     * The AC is taken as 2*sqrt(2)*sigma, the equivalent peak-to-peak of a
     * sinusoid. For a clean pulse that equals max-min exactly, so R and the
     * calibration curve below are unchanged; the difference is that it is computed
     * from all 50 samples instead of two. The same formula is already used for RSA
     * depth in ppg_respiratory_rate.c, for the same reason. */
    double   red_sum, red_sumsq;
    double   ir_sum,  ir_sumsq;
    int      sample_count;

    float    ratio_r;
    float    perfusion_index;      /* Perfusion Index (%): (AC_ir / DC_ir) * 100 */
    int      consecutive_valid;    /* Number of consecutive valid measurement windows */
    float    spo2_history[SPO2_MA_FILTER_SIZE];
    int      spo2_hist_idx;
    int      spo2_hist_count;
    float    spo2;
    int      valid;

    /* Diagnostics - why acquisition is taking as long as it is.
     *
     * The gate is deliberately conservative (8 consecutive valid windows AND a
     * flat 8-window history), which is 4 s at 0.5 s per window, but the measured
     * time to first reading was 22 s. Nothing in the published value can say
     * which gate was failing for the intervening 18 s, so this records the
     * running totals and the last window's measurements. */
    uint32_t windows_total;        /* measurement windows completed */
    uint32_t windows_valid;        /* of those, how many passed every gate */
    uint8_t  last_reject;          /* 0 = accepted, 1 = DC, 2 = AC, 3 = PI, 4 = ratio R */
    float    last_ir_dc, last_ir_ac, last_red_ac;
    float    last_red_dc;          /* SpO2 is a ratio of the two channels: if this
                                    * and ir_dc diverge, the ratio is biased by the
                                    * light level rather than by the blood */
    float    last_spread;          /* SpO2 spread across the compared history */
    float    last_spread_raw;      /* same, without discarding the extremes */
    int      last_settled;
} spo2_state_t;

/* Initialize / reset SpO2 state */
void  spo2_init(spo2_state_t *state);

/* Feed a pair of filtered Red and IR samples (18-bit); SpO2 is recomputed at end of each window */
void  spo2_add_samples(spo2_state_t *state, uint32_t red_filtered, uint32_t ir_filtered);

/* Get the latest SpO2 percentage (0-100) */
float spo2_get_value(const spo2_state_t *state);

/* Get current Perfusion Index (PI %) */
float spo2_get_perfusion_index(const spo2_state_t *state);

/* Returns 1 if the SpO2 reading is currently valid */
int   spo2_is_valid(const spo2_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* SPO2_ENGINE_H */
