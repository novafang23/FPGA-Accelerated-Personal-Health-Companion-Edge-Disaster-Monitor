/**
 * @file main_shrikefi.c
 * @brief Dual-Core FreeRTOS Application for ShrikeFi (ESP32-S3 + Renesas ForgeFPGA)
 * @project SIH26181 Personal Health Companion & Edge Disaster Monitor
 *
 * Demonstrates:
 *   - Core 0: High-speed optical acquisition (~100 Hz) & SPI link driver to the FPGA
 *   - Core 1: Environmental sensor fusion, INT8 TinyML inference, and OLED UI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif
#include "shrikefi_pinmap.h"
#include "shrikefi_link_driver.h"
#include "esp32_i2c_hal.h"
#include "max30102.h"
#include "bme280.h"
#include "pms5003.h"
#include "ssd1306.h"

/* Include platform-agnostic core firmware algorithms */
#include "hrv_analysis.h"
#include "spo2_engine.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model_int8.h"
#include "pm25_calibration_int8.h"
#include "clinical_vitals_engine.h"
#include "ppg_sqi.h"
#include "ppg_respiratory_rate.h"
#include "ppg_baseline_rr.h"
#include "wifi_mqtt_manager.h"
#include "sos.h"
#include "web_status.h"
#include "location.h"
#include "gnss_nmea.h"
#include "gps_l89hs.h"
#include "mpu6050.h"
#include "fall_detect.h"

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "7963e41"
#endif

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"   /* microsecond beat timestamps - see the PPG task */
#include "driver/uart.h"
#else
#define pdMS_TO_TICKS(ms) (ms)
#define pdTRUE 1
typedef void* SemaphoreHandle_t;
static inline int xSemaphoreTake(SemaphoreHandle_t s, int t) { (void)s; (void)t; return 1; }
static inline void xSemaphoreGive(SemaphoreHandle_t s) { (void)s; }
#ifndef ESP_LOGI
#define ESP_LOGI(tag, fmt, ...) do { } while(0)
#endif
#ifndef ESP_LOGW
#define ESP_LOGW(tag, fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef ESP_LOGE
#define ESP_LOGE(tag, fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif
#endif

static const char *TAG __attribute__((unused)) = "SHRIKEFI_MAIN";
static SemaphoreHandle_t s_data_mutex __attribute__((unused)) = NULL;

/* Signal quality and contact status */
typedef enum {
    SIGNAL_STATUS_NO_FINGER = 0,
    SIGNAL_STATUS_LOW_PERFUSION,  /* Touching too gently, hovering, or weak capillary pulse */
    SIGNAL_STATUS_ACQUIRING,      /* Locking pulse / accumulating beats */
    SIGNAL_STATUS_TRACKING        /* Vitals locked and clinically verified */
} signal_status_t;

/* Global shared health state */
typedef struct {
    float heart_rate;
    float r_peak_interval_ms;
    float hrv_rmssd;
    float hrv_sdnn;
    int   hrv_sample_count;   /* mirrors hrv_state_t.count from Core 0 */
    float spo2_percent;
    int   spo2_valid;         /* mirrors spo2_is_valid() from Core 0 */
    signal_status_t signal_status; /* real-time contact & perfusion quality */
    float ambient_temp_c;
    float humidity_percent;
    float pm25_ugm3;
    float respiratory_rate_bpm; /* PPG-derived (Charlton 2018 RSA); 0 = unavailable */
    float ppg_sqi;              /* composite signal quality index [0,1]        */
    /* The estimator's own inputs for the respiratory rate, published so the
     * thresholds can be set from measurements of real data rather than from
     * first principles. On hardware RR published in 4 of 118 frames in one
     * sitting and 0 of 84 in the next, and there is no way to tell from the
     * published rate alone whether that is little RSA or a bar set too high. */
    float rr_confidence;        /* estimator confidence [0,1]                  */
    float rr_rsa_depth_ms;      /* equivalent sinusoid RSA peak-to-peak (ms)   */
    float imu_motion_energy;    /* dynamic motion energy in g from MPU6050     */
    fall_state_t fall_state;    /* biomechanical fall detection state          */
    risk_assessment_t risk_result;
    nn_output_t nn_scores;
} health_system_state_t;

static health_system_state_t g_state;

#ifdef ESP_PLATFORM
/* Sensor driver instances */
static max30102_t s_max30102;
static bme280_t s_bme280;
static pms5003_t s_pms5003;
static ssd1306_t s_ssd1306;
static gnss_nmea_t s_gnss;
#define s_gps s_gnss
static mpu6050_t s_mpu6050;
static fall_detector_t s_fall_detector;
static char s_last_gps_coord[GNSS_COORD_STR_LEN] = "NO FIX";
static bool s_has_gps_fix = false;

/* Barometric pressure history for the storm advisory (T3.1).
 *
 * File scope rather than function-local because it is device state with a
 * lifetime longer than one task iteration: the BME280 reports pressure and this
 * firmware used to read it and discard it, and a falling-barometer signature
 * needs three hours of it. One sample is stored per minute (see pressure_trend.h
 * for the window sizing), so it fills slowly and reports UNKNOWN until it has
 * enough span to mean anything. */
static pressure_trend_t s_pressure_trend;
static uint32_t         s_pressure_next_due_ms = 0;
#endif

/* ---------------------------------------------------------------------------
 * IBI acceptance pipeline - the single door every heartbeat interval passes
 * through, whichever detector measured it.
 * ---------------------------------------------------------------------------
 * Two independent peak detectors run on this board and they do not agree:
 *
 *   * the ForgeFPGA crest detector, which answers once per SPI transaction and
 *     therefore timestamps a beat to within ~1 ms, and
 *   * the software fallback FSM in this task, which runs on the 18-bit IR
 *     stream and has its own threshold, refractory and confirmation delay.
 *
 * Whichever one is authoritative changes mid-session: at the start of every
 * finger contact the FPGA has not yet produced a beat, so the software FSM is
 * authoritative and fills the HRV window; a second or two later the FPGA takes
 * over. Measured consequence on hardware: intervals recorded by the software
 * detector sat near 500 ms (HR ~119) and intervals recorded by the FPGA sat near
 * 880 ms (HR ~70), all in one rolling window. The device therefore reported an
 * HR that ramped 119 -> 98 -> 84 -> 72 over ~20 s as the faster intervals were
 * diluted, and an RMSSD of 190 ms that was mostly the one 400 ms step between
 * the two groups rather than anything physiological.
 *
 * So: a change of detector flushes the window. Intervals measured by two
 * different detectors are never mixed in one statistic.
 */
#define IBI_SRC_NONE      0
#define IBI_SRC_SOFTWARE  1
#define IBI_SRC_FPGA      2

/* Absolute plausibility bounds, and the relative window around the local
 * rhythm. Everything here is expressed against ONE reference: a median over the
 * last few DETECTED intervals.
 *
 * THE REFERENCE IS FED BY EVERY INTERVAL THAT PASSES THE ABSOLUTE BOUNDS,
 * ACCEPTED OR NOT. That is the whole trick, and it is what makes a lower bound
 * safe. An earlier version derived its floor from a median over ACCEPTED
 * intervals only, and it latched: rejecting short intervals removed exactly the
 * values that would have pulled the median down, so the floor ratcheted upward
 * until it excluded the subject's real rhythm permanently. Measured on
 * hardware - four intervals in (1470, 720, 759, 1400 ms), median 1400 ms, a
 * 0.80x floor of 1120 ms, and every subsequent ~740 ms beat rejected. The
 * sample counter froze at 4, took 12.5 s to reach 5, 49 s to reach 6 and 62 s
 * more to reach 7 while the FPGA detected ~80 BPM throughout.
 *
 * When the reference is fed by everything plausible, a rejection cannot change
 * it at all, so no bound derived from it can ratchet. There is no feedback path
 * left to latch. This is why the ratio bounds below are allowed to be
 * two-sided, which they must be: the detector's errors come in PAIRS.
 *
 * WHY TWO-SIDED IS NECESSARY - measured, not assumed
 * --------------------------------------------------
 * From the 01:34 capture, the raw IR waveform was autocorrelated independently
 * of the FPGA (hardware/shrikefi/tools/analyse_capture.py plus the raw [PPG]
 * stream). The true rhythm was 730-770 ms, 78-82 BPM, with no competitor at
 * half that lag. The FPGA detector's own interval distribution over the same
 * 116 s was bimodal:
 *
 *      700-899 ms : 74 intervals   <- real
 *      340-599 ms : 15 intervals   <- artefact
 *     1000-1300 ms: 33 intervals   <- artefact, 23% of all intervals
 *
 * 23% of intervals at 1000-1300 ms is impossible at a 78-82 BPM rhythm. The
 * short and long clusters are two halves of one event: a short interval plus
 * its long partner sum to two cardiac cycles (e.g. 340 + 1200 = 1540 = 2 x 770).
 * The FPGA's crest detector therefore has TWO fiducial points about 350 ms
 * apart, and picks between them beat by beat.
 *
 * Rejecting only the long half would leave the short half in the series, and
 * vice versa; either one alone still injects a ~350 ms successive difference.
 * Both halves have to go, so the window is symmetric.
 *
 * A +/-30% window was too permissive on the attached hardware: a live capture
 * at a stable reference of 810 ms admitted a 669/1020 ms split pair and
 * reported RMSSD=136.8 ms. A +/-15% local window rejects both halves while
 * still allowing gradual rate changes; sustained genuine changes reach the
 * rejection escape and reseed the reference rather than ratcheting it. */
#define IBI_MIN_MS    272.0f   /* hard floor (220 BPM): allows tachycardia >= 150 BPM; above 250 ms FPGA blanking */
#define IBI_MAX_MS   1500.0f   /* hard ceiling: above this a whole cycle was lost  */
#define IBI_LOW_RATIO  0.83f   /* reject below this x the reference median (rejects split-beats <=669ms) */
#define IBI_HIGH_RATIO 1.20f   /* reject above this x the reference median (rejects split-beats >=1020ms) */
#define PPG_CONTACT_THRESHOLD 7000u /* raw MAX30102 counts; rejects ambient LED leakage while accepting gentle finger contact */

/* Escape hatch. Any rejection rule can in principle reject a run of real beats
 * (a genuine rate change, a stretch of poor perfusion). A filter that keeps
 * rejecting is itself producing an artifact, so after this many consecutive
 * rejections the reference window is discarded and rebuilt from fresh data. */
#define IBI_REJECT_ESCAPE 12

/* Reference priming. The first few intervals after a detector handover cannot
 * be validated - there is no rhythm estimate to validate them against yet - so
 * they seed the reference and are not measured with.
 *
 * Measured why this matters, from the 02:49 capture. Three of its four contact
 * sessions settled cleanly (RMSSD 39.4, 41.6 and 27.3 ms, no jumps). The fourth
 * started noisily - a 420 ms interval, a 959 ms pair, a 1559 ms gap and a
 * 1390 ms interval inside its first three accepted beats - and once past that,
 * a 540 ms interval was accepted at a reference of about 771 ms. Its ratio was
 * 0.7004, a hair inside the 0.70 floor. Against the 839 ms beat before it that
 * is a 299 ms successive difference, and at n=6 it took RMSSD from 41.6 to
 * 127.7 ms.
 *
 * Priming removes the whole class: the junk that a fresh contact produces is
 * spent building the reference instead of entering the series. It costs
 * IBI_PRIME_N intervals per contact - about 4 s at 75 BPM, once. */
#define IBI_PRIME_N 5

/* Per-interval audit trail. Costs ~1 console line per second at 75 BPM. Set to
 * 0 once the pipeline is trusted; it is the only way to see WHY an interval was
 * dropped, as opposed to inferring it from the surviving HRV statistics. */
#ifndef SHRIKEFI_IBI_TRACE
#define SHRIKEFI_IBI_TRACE 1
#endif

typedef struct {
    int          source;        /* IBI_SRC_* currently feeding the buffer */
    hrv_median_t ref;           /* median of recent DETECTED intervals, not accepted ones */
    bool         skip_next;     /* previous interval was rejected as an artifact */
    int          reject_streak; /* consecutive rejections, triggers the escape */
    int          prime_count;   /* plausible intervals seen since handover */
} ibi_pipeline_t;

static bool ppg_has_optical_contact(uint32_t ir, uint32_t red) {
    return ir > PPG_CONTACT_THRESHOLD || red > PPG_CONTACT_THRESHOLD;
}

static void ibi_pipeline_reset(ibi_pipeline_t *p) {
    p->source = IBI_SRC_NONE;
    hrv_median_init(&p->ref);
    p->skip_next = false;
    p->reject_streak = 0;
    p->prime_count = 0;
}

/* ---------------------------------------------------------------------------
 * PPG-derived respiration and signal quality
 * ---------------------------------------------------------------------------
 * ppg_respiratory_rate.c (Charlton 2018, RSA) and ppg_sqi.c (Elgendi 2016 /
 * Karlen 2012) were both written and both exercised - but only in the host
 * harness. Neither was ever added to the ESP build, so on the device NEWS2 ran
 * with its respiratory rate defaulted to a normal 14
 * (clinical_vitals_engine.c:106) and could not flag tachypnoea or bradypnoea,
 * which is the earliest and most sensitive term in the score. The sensor path
 * was there; only the wiring was missing.
 *
 * Both estimators take a contiguous window, so these are shift buffers rather
 * than rings. They are read about once a second, so the memmove is free.
 */
#define PPG_RR_HISTORY   40    /* accepted beats fed to the RR estimator        */
/* Minimum beats before an RSA estimate is attempted at all.
 *
 * Respiration is 0.2-0.3 Hz, a 3-5 s cycle, so resolving it needs several full
 * cycles - not ten beats. At a resting 78 BPM the old floor of 10 was 7.7
 * seconds, under two cycles, and the estimator still answered: it railed at its
 * 36 br/min ceiling and the triage reported CRITICAL tachypnoea on a healthy
 * resting subject, on every other assessment. 30 beats is about 23 s at rest,
 * roughly five cycles - enough for the confidence figure to mean something. */
#define PPG_RR_MIN_IBIS  30
#define PPG_SQI_RAW_WIN (3 * PPG_FIFO_RATE_HZ) /* raw IR samples fed to the SQI (3 s at 25 Hz FIFO rate) */

static float    s_rr_ibis[PPG_RR_HISTORY];
static float    s_rr_amps[PPG_RR_HISTORY]; /* pulse height per beat, aligned with s_rr_ibis */
static int      s_rr_ibi_n = 0;
#ifdef ESP_PLATFORM
static uint32_t s_sqi_raw[PPG_SQI_RAW_WIN];
static int      s_sqi_raw_n = 0;
/* Low-quality beats still feed the SQI estimator, but never publish HRV. */
static bool     s_hrv_sqi_valid = false;

/* Published-rate stabiliser. The raw estimate re-decides every second and on
 * hardware crossed its own confidence bar in 4 frames out of 118, which
 * flickers the display and flips the triage; see ppg_rr_tracker_update(). */
static ppg_rr_tracker_t s_rr_tracker;

/* Baseline (DC) respiratory path: 25 Hz raw IR -> 1 s baseline -> 5 Hz ->
 * 0.1-0.5 Hz -> ACF. Independent of the beat rate; see ppg_baseline_rr.h. */
static ppg_baseline_rr_t s_base_rr;
#endif

/* (The single-series float push helper was removed when the IBI history gained
 * its parallel pulse-amplitude series; see ppg_history_push_pair below.) */

/* Keep the IBI and pulse-amplitude series in lockstep: the AM corroboration
 * path in ppg_estimate_respiratory_rate() correlates the amplitude series at
 * the respiratory lag, so element i of each array must describe the same beat. */
static void ppg_history_push_pair(float *ibis, float *amps, int *n, int cap,
                                  float ibi, float amp) {
    if (*n < cap) {
        ibis[*n] = ibi;
        amps[*n] = amp;
        (*n)++;
    } else {
        memmove(ibis, ibis + 1, sizeof(float) * (size_t)(cap - 1));
        memmove(amps, amps + 1, sizeof(float) * (size_t)(cap - 1));
        ibis[cap - 1] = ibi;
        amps[cap - 1] = amp;
    }
}

#ifdef ESP_PLATFORM
static void ppg_history_push_u32(uint32_t *buf, int *n, int cap, uint32_t v) {
    if (*n < cap) {
        buf[(*n)++] = v;
    } else {
        memmove(buf, buf + 1, sizeof(uint32_t) * (size_t)(cap - 1));
        buf[cap - 1] = v;
    }
}

static void ppg_history_reset(void) {
    s_rr_ibi_n  = 0;
    s_sqi_raw_n = 0;
    memset(s_rr_amps, 0, sizeof(s_rr_amps));
    s_hrv_sqi_valid = false;
    /* A new contact starts with no respiratory evidence, so it must not inherit
     * a rate - or a hold - from the previous one. */
    ppg_rr_tracker_init(&s_rr_tracker);
    ppg_baseline_rr_init(&s_base_rr);
}

/* Recompute respiration and SQI from the rolling windows. Cheap enough to run
 * at 1 Hz on a 160 MHz core. */
static void ppg_update_respiration_and_sqi(hrv_state_t *hrv,
                                           ibi_pipeline_t *ibi_pipe) {
    ppg_respiratory_result_t rr;
    ppg_sqi_result_t         sqi;
    memset(&rr, 0, sizeof(rr));
    memset(&sqi, 0, sizeof(sqi));

    /* Charlton-style RSA needs a reasonable run of beats to resolve a
     * respiratory modulation; below that the estimate is noise. */
    if (s_rr_ibi_n >= PPG_RR_MIN_IBIS) {
        ppg_estimate_respiratory_rate(s_rr_ibis, s_rr_amps, (size_t)s_rr_ibi_n, &rr);
    }

    /* What the estimator actually found, not just what it published.
     *
     * A breath hold - no respiration at all - produced a RELIABLE 27.4 br/min at
     * an 82 bpm heart rate, which is a 3-beat rhythm, not breathing. The rate
     * alone cannot show that: the published confidence of 0.66 was mostly the
     * depth term (saturated at 273 ms) while the periodicity behind it was only
     * 0.36. Printing the winning lag and its correlation makes that visible, and
     * a rate without them cannot be argued with. */
    ESP_LOGI(TAG, "[RRDIAG] beats=%d mean_ibi=%.0fms lag=%d(%.2f) r=%.3f r1=%.3f depth=%.1fms conf=%.2f rel=%d am=%.1f amc=%.2f",
             rr.diag_beats, rr.diag_mean_ibi_ms, rr.diag_peak_lag, rr.diag_peak_lag_f,
             rr.diag_acf_peak_r, rr.diag_lag1_r, rr.rsa_depth_ms, rr.confidence,
             (int)rr.is_reliable, rr.diag_am_bpm, rr.diag_am_conf);

    /* Baseline (DC) respiratory path: independent of the beat rate, so it can
     * publish when the IBI/AM estimator cannot. Consulted only when the primary
     * estimator is not reliable, and only at the same 0.60 confidence bar the
     * primary path uses - a weak ACF peak is not a measurement. */
    float base_rr = 0.0f, base_conf = 0.0f;
    bool base_ok = ppg_baseline_rr_estimate(&s_base_rr, &base_rr, &base_conf);
    if (base_ok) {
        ESP_LOGI(TAG, "[RRBASE] rr=%.1f conf=%.2f win=%d lag=%d acf=%.2f", base_rr,
                 base_conf, s_base_rr.win_n, s_base_rr.last_lag, s_base_rr.last_acf);
    }
    if (!rr.is_reliable && base_ok && base_conf >= PPG_BASE_RR_CONF_MIN &&
        s_base_rr.win_n >= 100) {
        rr.respiratory_rate_bpm = base_rr;
        rr.confidence           = base_conf;
        rr.is_reliable          = true;
    }

    /* Stabilise before publishing. A fresh contact, or one that has just lost
     * its evidence, must not inherit a rate from before. */
    ppg_rr_tracker_update(&s_rr_tracker, &rr);

    float motion_energy = 0.0f;
    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        motion_energy = g_state.imu_motion_energy;
        xSemaphoreGive(s_data_mutex);
    }

    if (s_sqi_raw_n >= PPG_FIFO_RATE_HZ) {
        ppg_calculate_sqi_with_motion(s_sqi_raw, (size_t)s_sqi_raw_n,
                                      s_rr_ibis, (size_t)s_rr_ibi_n,
                                      motion_energy, &sqi);
    }

    bool hrv_quality_ok = false;
    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        /* Honour the estimator's own reliability verdict. It computes
         * confidence and sets is_reliable at >= 0.60, and the caller used to
         * ignore both - so a low-confidence estimate was published as though it
         * were a measurement. Publish 0 instead: clinical_vitals_assess_full()
         * treats rr <= 0 as "unavailable" and holds the NEWS2 respiratory term
         * neutral, which is the honest answer. Something confidently wrong is
         * worse here than something absent, because tachypnoea is the most
         * sensitive term in the score. */
        g_state.respiratory_rate_bpm = rr.is_reliable ? rr.respiratory_rate_bpm : 0.0f;
        if (g_state.ppg_sqi < PPG_SQI_THRESHOLD_VALID && sqi.overall_sqi >= PPG_SQI_THRESHOLD_VALID) {
            /* Fast-lock when SQI crosses the validity threshold (e.g., once 3+ beats arrive)
             * so EMA inertia from the 0-beat cold start does not stall HRV at 10/10 beats. */
            g_state.ppg_sqi = sqi.overall_sqi;
        } else if (g_state.ppg_sqi > 0.0f && sqi.overall_sqi > 0.0f) {
            g_state.ppg_sqi = 0.80f * g_state.ppg_sqi + 0.20f * sqi.overall_sqi;
        } else {
            g_state.ppg_sqi = sqi.overall_sqi;
        }
        hrv_quality_ok = (sqi.overall_sqi >= PPG_SQI_THRESHOLD_VALID) &&
                         (g_state.ppg_sqi >= PPG_SQI_THRESHOLD_VALID);
        s_hrv_sqi_valid = hrv_quality_ok;
        if (hrv_quality_ok) {
            if (hrv != NULL && hrv->count >= HRV_MIN_SAMPLES && hrv->rmssd > 0.0f && g_state.hrv_rmssd <= 0.0f) {
                g_state.hrv_rmssd = hrv->rmssd;
                g_state.hrv_sdnn  = hrv->sdnn;
            }
        } else {
            /* An RMSSD from a pulse stream below the project's validity gate is
             * not a meaningful measurement. Clear the published RMSSD/SDNN so
             * low-quality beats are not scored for clinical triage. HR and SpO2
             * remain available. Sample count is preserved so the 10-beat acquisition
             * progress is visible and uninterrupted. */
            g_state.hrv_rmssd        = 0.0f;
            g_state.hrv_sdnn         = 0.0f;
        }
        /* Published even when unreliable, so the gate can be judged against what
         * the estimator actually saw. */
        g_state.rr_confidence         = rr.confidence;
        g_state.rr_rsa_depth_ms       = rr.rsa_depth_ms;
        xSemaphoreGive(s_data_mutex);
    }

    (void)ibi_pipe;
}
#endif

/**
 * @brief Validate, artifact-filter and record one heartbeat interval.
 *
 * @param p        Pipeline state (one per contact session).
 * @param hrv      HRV accumulator this interval belongs to.
 * @param source   IBI_SRC_SOFTWARE or IBI_SRC_FPGA.
 * @param ibi_ms   Interval in milliseconds, as measured by that detector.
 * @return true if the interval was accepted into the HRV window, in which case
 *         the caller may publish hrv->rmssd / hrv->count.
 */
static bool ibi_pipeline_submit(ibi_pipeline_t *p, hrv_state_t *hrv,
                                int source, float ibi_ms, float peak_amp) {
    if (!(ibi_ms > 0.0f)) {
        hrv_break_sequence(hrv);
        return false;
    }

    if (p->source != source) {
        int old_source = p->source;
        p->source = source;
        /* Do NOT wipe accumulated beats with hrv_init() on detector handover!
         * Wiping beats caused the 10-beat registration counter to reset every time
         * a beat arrived from the alternate detector. Only reset reference rhythm. */
        hrv_median_init(&p->ref);
        p->skip_next = false;
        p->reject_streak = 0;
        p->prime_count = 0;

        if (old_source != IBI_SRC_NONE) {
            ESP_LOGI(TAG, "IBI detector handover -> %s (current progress: %d/10)",
                     (source == IBI_SRC_FPGA) ? "ForgeFPGA" : "software fallback",
                     hrv->count);
        }
    }

    bool accepted = false;

    /* Absolute plausibility first. Only intervals that clear it may enter the
     * reference window, so one absurd reading cannot corrupt the local rhythm
     * estimate. */
    bool plausible = (ibi_ms >= IBI_MIN_MS && ibi_ms <= IBI_MAX_MS);

    /* Reference rhythm, or 0.0f until its window is full. The full-window
     * requirement matters: a median over a partly-filled window can be
     * dominated by its own outliers - which is exactly how the old latch was
     * seeded (4 entries, 2 of them artefacts). */
    float ref = hrv_median_value(&p->ref);

    if (plausible) {
        /* When a rhythm reference is already locked (ref > 0), protect the median
         * from being poisoned by dicrotic notch false-triggers (< 0.55 * ref, e.g. ~350ms)
         * or double-period missed beats (> 1.80 * ref). This prevents the reference median
         * from ratcheting down to ~450ms and rejecting all subsequent legitimate beats. */
        bool allow_ref_feed = true;
        if (ref > 0.0f && (ibi_ms < (0.55f * ref) || ibi_ms > (1.80f * ref))) {
            allow_ref_feed = false;
        }
        if (allow_ref_feed) {
            hrv_median_push(&p->ref, ibi_ms);
            ref = hrv_median_value(&p->ref);
        }
        /* Feed plausible intervals and their pulse heights to the respiration/
         * SQI history. The amplitudes let the estimator corroborate the
         * frequency-modulation (RSA) rate with an independent amplitude-
         * modulation rate, which is what rejects IBI-only artifacts. */
        ppg_history_push_pair(s_rr_ibis, s_rr_amps, &s_rr_ibi_n, PPG_RR_HISTORY,
                              ibi_ms, peak_amp);
    }
    const char *verdict;

    if (plausible && p->prime_count < IBI_PRIME_N) {
        /* A fresh contact produces junk while the finger settles and while the
         * pulse amplitude stabilises, and there is no rhythm estimate yet to
         * catch it with. Spend these beats on the reference instead of on the
         * measurement. See IBI_PRIME_N. */
        p->prime_count++;
        p->skip_next = false;
        verdict = "prime";
    } else if (!plausible && ibi_ms > IBI_MAX_MS) {
        /* A whole cardiac cycle was lost. Not a beat-to-beat measurement, and
         * not the remainder of a split either, so clear skip_next. */
        p->skip_next = false;
        verdict = "gap";
    } else if (ref > 0.0f &&
               (ibi_ms < (IBI_LOW_RATIO * ref) || ibi_ms > (IBI_HIGH_RATIO * ref))) {
        /* Outside this subject's local rhythm.
         * Only an extreme short interval (< 0.65 * ref, e.g. true dicrotic notch split)
         * is the first half of a split pair whose compensating partner must be dropped.
         * Mild short intervals (between 0.65 and 0.83 * ref, e.g. sinus variation) do not
         * poison the following beat, so skip_next is only set on true split halves. */
        bool short_side = (ibi_ms < (0.65f * ref));
        p->skip_next = short_side;
        verdict = (ibi_ms < (IBI_LOW_RATIO * ref)) ? "low" : "high";
    } else if (ibi_ms < IBI_MIN_MS) {
        p->skip_next = true;    /* detector double-fired, partner follows */
        verdict = "abs-low";
    } else if (p->skip_next) {
        p->skip_next = false;   /* the compensating half of that same cycle */
        verdict = "pair";
    } else {
        hrv_add_ibi(hrv, ibi_ms);
        hrv_compute(hrv);
        accepted = true;
        verdict = "ok";
    }

#if SHRIKEFI_IBI_TRACE
    /* One line per detected interval: what the filter decided, and why.
     *
     * Without this the ONLY view of the filter is the HRV sample count, which
     * updates once per second and says nothing about which intervals were
     * dropped or on what grounds - so every diagnosis has to be inferred from
     * arithmetic on the surviving statistics. That is how the two earlier
     * mistakes in this pipeline survived as long as they did. At ~75 BPM this
     * costs about one line per second.
     *
     *   IBI <src> <ms> <verdict> ref=<rhythm> n=<count> rmssd=<ms>
     *   verdict: ok | low | high | gap | abs-low | pair
     */
    printf("IBI %d %lu %s ref=%.0f n=%d rmssd=%.1f\n",
           source, (unsigned long)ibi_ms, verdict, ref,
           hrv->count, hrv->rmssd);
    fflush(stdout);
#endif

    /* A rejected, priming, or dropped interval breaks NN adjacency. Without
     * this, hrv_compute() would compare accepted intervals on opposite sides
     * of the gap and inject a false successive difference into RMSSD. */
    if (!accepted) hrv_break_sequence(hrv);

    if (accepted) {
        p->reject_streak = 0;
    } else if (++p->reject_streak >= IBI_REJECT_ESCAPE) {
        ESP_LOGW(TAG, "IBI filter rejected %d intervals in a row; discarding the "
                      "rhythm reference and re-seeding from live beats",
                 p->reject_streak);
        hrv_median_init(&p->ref);
        p->skip_next = false;
        p->reject_streak = 0;
        p->prime_count = 0;
    }

    return accepted;
}

#ifdef ESP_PLATFORM
static int read_bme280_env(bme280_data_t *data) {
    return bme280_read(&s_bme280, data);
}

static int read_pms5003_data(pms5003_data_t *data) {
    return pms5003_get_data(&s_pms5003, data);
}

/**
 * @brief Core 0 Task: FPGA SPI link transceiver & high-frequency PPG processing
 */
static void task_ppg_accelerator(void *pvParameters) {
    (void)pvParameters;
    hrv_state_t hrv_state;
    hrv_init(&hrv_state);
    max30102_sample_t ppg_sample;
    spo2_state_t spo2_state;
    spo2_init(&spo2_state);

    /* 8-tap running-sum moving average filter (matching moving_average_8tap.v) */
    static uint16_t red_history[8] = {0};
    static uint16_t ir_history[8]  = {0};
    static uint8_t  hist_idx       = 0;
    static uint32_t red_sum        = 0;
    static uint32_t ir_sum         = 0;

    /* Software systolic peak detector state machine with strict noise rejection */
    enum { SW_ARMED, SW_RISING, SW_REFRACTORY };
    static int      sw_state             = SW_ARMED;
    static uint32_t sw_prev_sample       = 0;
    static uint32_t sw_peak_val          = 0;
    static uint8_t  sw_fall_count        = 0;
    static uint32_t sw_refractory_end_ms = 0;
    static uint64_t sw_last_peak_sample  = 0;
    static uint64_t s_total_samples      = 0;
    static uint32_t sw_last_valid_ibi_ms = 0;
    static int      sw_beat_streak       = 0;
    static uint32_t sw_finger_start_ms   = 0;
    static uint32_t sw_running_mean      = 15000;

    /* Timestamp of the most recent beat reported by the ForgeFPGA. This records
     * whether the FPGA has ever reported a beat during the current contact. */
    static uint32_t s_last_fpga_beat_ms  = 0;

    /* Split-beat guard + artifact filter + detector-handover flushing. Every
     * interval from either detector goes through this one pipeline; see
     * ibi_pipeline_submit() for why. */
    ibi_pipeline_t  ibi_pipe;
    ibi_pipeline_reset(&ibi_pipe);

    /* Optical baseline railing counters (see the re-seed below) */
    static uint8_t  s_rail_red = 0;
    static uint8_t  s_rail_ir  = 0;

    static int      raw_log_timer        = 0;
    /* Counts the ~2 Hz debug-log ticks so the MISO line heuristic runs about
     * every 5 s rather than on every one. */
    static int      s_fpga_check_counter = 0;
    static bool     s_miso_warning_logged = false;

    /* Perfusion & AC amplitude tracking over 1-second rolling windows */
    static uint32_t ir_win_min           = UINT32_MAX;
    static uint32_t ir_win_max           = 0;
    static int      ir_win_count         = 0;
    static uint32_t last_ac_amplitude    = 0;
    static uint32_t next_max30102_retry_ms = 0;
    static uint16_t max30102_i2c_error_streak = 0;
    static uint8_t max30102_init_failures = 0;
    static int      s_disconnect_samples = 0;
    /* Highest raw IR seen since the last accepted beat - the pulse height used
     * for amplitude-modulation (AM) corroboration of the respiratory rate. */
    static uint32_t s_ir_peak_since_beat = 0;

    ESP_LOGI(TAG, "Core 0: PPG Accelerator Task Started (FPGA hardware + Software DSP fallback).");

    while (1) {
        if (!s_max30102.initialized) {
            uint32_t retry_now_ms = (uint32_t)(esp_timer_get_time() / 1000);
            if ((int32_t)(retry_now_ms - next_max30102_retry_ms) >= 0) {
                next_max30102_retry_ms = retry_now_ms + 1000;
                if (max30102_init(&s_max30102, esp32_i2c_hal_get_handle()) == 0) {
                    max30102_init_failures = 0;
                    ESP_LOGI(TAG, "MAX30102 initialized after a runtime I2C retry");
                } else {
                    if (max30102_init_failures < UINT8_MAX) max30102_init_failures++;
                    if (max30102_init_failures == 3) {
                        ESP_LOGW(TAG, "MAX30102 init failed 3 times; attempting safe I2C bus recovery");
                        (void)esp32_i2c_hal_recover_bus();
                    }
                    ESP_LOGW(TAG, "MAX30102 still unavailable; retrying in 1 second");
                }
            }
            if (!s_max30102.initialized) {
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
        }

        int dropped = 0;
        int saturated = 0;
        int avail = max30102_fifo_available(&s_max30102, &dropped, &saturated);
        int samples_read = 0;
        bool i2c_read_failed = (avail < 0);

        if (saturated) {
            s_total_samples = 0;
        } else if (dropped > 0) {
            s_total_samples += dropped;
        }

        /* Drain available samples from FIFO (prevents buffer overflow & lag) */
        while (avail > 0 && samples_read < 8) {
            if (max30102_read_sample(&s_max30102, &ppg_sample) != 0) {
                i2c_read_failed = true;
                break;
            }
            samples_read++;
            avail--;
            s_total_samples++;
            if (ppg_sample.ir > s_ir_peak_since_beat) s_ir_peak_since_beat = ppg_sample.ir;

            /* Timestamps must not come from the RTOS tick OR esp_timer, because
             * FreeRTOS scheduling jitter makes timer-based intervals noisy. RMSSD
             * is highly sensitive to per-sample error. We use OVF_COUNTER and 
             * the sample count to generate perfect sample-synchronous timestamps. */
            uint32_t now_ms = (uint32_t)((s_total_samples * 1000) / PPG_FIFO_RATE_HZ);

            /* Track min/max over 0.5 sec to measure pulsatile AC amplitude */
            if (ppg_sample.ir < ir_win_min) ir_win_min = ppg_sample.ir;
            if (ppg_sample.ir > ir_win_max) ir_win_max = ppg_sample.ir;
            if (++ir_win_count >= (PPG_FIFO_RATE_HZ / 2)) {
                uint32_t ac = (ir_win_max > ir_win_min) ? (ir_win_max - ir_win_min) : 0;
                /* Plausibility gate. The pulsatile (AC) component of a real PPG is
                 * the perfusion index - roughly 0.2-10% of the DC level. Lifting or
                 * re-seating the finger mid-window makes max-min span the entire DC
                 * step, giving AC >= DC, which is physiologically impossible.
                 * Ungated, that single frame sets threshold_offset = AC/4 (line 225)
                 * to tens of thousands of counts and blinds the peak detector for
                 * the whole of the next second. */
                if (ir_win_max > 0 && ac > (ir_win_max / 2)) {
                    ac = 0;   /* transient, not perfusion - discard */
                }
                last_ac_amplitude = ac;
                ir_win_min = UINT32_MAX;
                ir_win_max = 0;
                ir_win_count = 0;
            }

            /* Optical contact check with 100-sample (debounced) tolerance:
             * Ambient air is IR < 1000; tissue contact elevates levels to > 50,000.
             * This board also produced 3,800-4,400 counts without tissue while the
             * LED current was high; a 7,000-count threshold separates that measured
             * ambient level from tissue.
             * A momentary 1-sample optical glitch or movement must not wipe the 300-beat
             * HRV and SpO2 history buffers immediately. We require 100 consecutive missing
             * samples before declaring contact lost. */
            bool raw_contact = ppg_has_optical_contact(ppg_sample.ir, ppg_sample.red);
            bool optical_contact;
            if (raw_contact) {
                s_disconnect_samples = 0;
                optical_contact = true;
            } else {
                if (s_disconnect_samples < 100) {
                    s_disconnect_samples++;
                }
                optical_contact = (s_disconnect_samples < 100);
            }

            /* Keep LED pulse amplitude fixed during acquisition. Runtime
             * current writes are not safe recovery actions: a failed I2C mode
             * read can make the sensor re-enter/configure SpO2 and visibly
             * blink the emitters. LED current is set during initialization;
             * change it only through an explicit calibration procedure. */

            /* Per-channel DC trackers. These MUST be separate: RED and IR sit
             * ~50-70k counts apart, and a shared baseline saturates the IR byte
             * handed to the FPGA to 0/255 on alternating samples. */
            static uint32_t s_base_red = 0;
            static uint32_t s_base_ir  = 0;

            uint8_t raw_red = max30102_scale_to_8bit_ch(ppg_sample.red, &s_base_red);
            uint8_t raw_ir  = max30102_scale_to_8bit_ch(ppg_sample.ir,  &s_base_ir);

            /* --- Re-seed a baseline that has stopped tracking --------------
             * max30102_scale_to_8bit_ch() centres its output on 120 and clamps
             * to [0,255], so at DIV=8 it can express only +/-120 counts of AC.
             * The MAX30100's DC output ramps from 0 to ~127000 counts over the
             * ~1.5 s after a finger is seated. The 640 ms baseline EMA lags a
             * ramp by (slope x tau) - tens of thousands of counts here - so the
             * byte handed to the FPGA pins at a rail instead of carrying the
             * pulse.
             *
             * Measured cost of that, from the SPI capture: tx sat at 255 for
             * 4.6 s, the FPGA's 8-tap average railed with it, the peak detector
             * could not see a crest for 7.7 s after contact, and the one beat it
             * did report in that window was a phantom emitted when the signal
             * finally came off the rail. That phantom is what produced the
             * 7740 ms first interval.
             *
             * A genuine pulse is periodic, so it never sits on a rail for 300 ms
             * in a row. Counting consecutive railed samples separates "the
             * tracker has lost the DC level" from "the pulse is large", and
             * re-seeds the tracker (setting the baseline to 0 makes the scaler
             * adopt the current sample) instead of integrating for seconds. */
            s_rail_red = (raw_red == 0 || raw_red == 255) ? (uint8_t)(s_rail_red + 1) : 0;
            s_rail_ir  = (raw_ir  == 0 || raw_ir  == 255) ? (uint8_t)(s_rail_ir  + 1) : 0;
            const uint8_t rail_limit = (uint8_t)(PPG_FIFO_RATE_HZ * 3 / 10); /* 300 ms at 25 Hz */
            if (s_rail_red > rail_limit) { s_base_red = 0; s_rail_red = 0; }
            if (s_rail_ir  > rail_limit) { s_base_ir  = 0; s_rail_ir  = 0; }

            /* Raw 18-bit IR window for the signal-quality index. Fed only while
             * there is direct optical contact, so a momentary glitch cannot poison it. */
            if (raw_contact) {
                ppg_history_push_u32(s_sqi_raw, &s_sqi_raw_n, PPG_SQI_RAW_WIN,
                                     ppg_sample.ir);
            }

/* ESP-IDF omits an unset bool from sdkconfig.h entirely, so an undefined
 * CONFIG_SHRIKEFI_STREAM_PPG and a "not set" one are indistinguishable here -
 * and an undefined identifier in #if evaluates to 0. That is how this option
 * silently compiled the whole [PPG]/FG stream out of a build whose
 * sdkconfig.defaults asked for it, flattening the dashboard oscilloscope with
 * no diagnostic. Spell the test out so the dependency is explicit rather than
 * relying on the reader knowing the #if rule. The option is default y in
 * main/Kconfig.projbuild and y in sdkconfig.defaults. */
#if defined(CONFIG_SHRIKEFI_STREAM_PPG) && CONFIG_SHRIKEFI_STREAM_PPG
            /* Stream raw PPG sample for the dashboard oscilloscope.
             *
             * Gated on CONFIG_SHRIKEFI_STREAM_PPG (default y). At the ~100 Hz optical
 * rate this one
             * line is most of the console output, and it buries the lines that
             * actually answer bring-up questions - the FPGA link handshake, the
             * detector handover, and [TRIAGE]. Turn the option off for a
             * readable log; the web dashboard keeps every number and only loses
             * the waveform. */
            printf("[PPG] %lu\n", (unsigned long)ppg_sample.ir);
#endif

            /* 2. Stream to the ForgeFPGA over the SPI link */
            if (raw_contact) {
                shrikefi_write_red_sample(raw_red);
                shrikefi_write_ir_sample(raw_ir);
            } else {
                shrikefi_write_red_sample(0);
                shrikefi_write_ir_sample(0);
            }

            /* 3. Compute 8-tap running-sum moving average filter */
            red_sum = red_sum - red_history[hist_idx] + raw_red;
            red_history[hist_idx] = raw_red;
            uint8_t filt_red = (uint8_t)(red_sum >> 3);
            (void)filt_red;

            ir_sum = ir_sum - ir_history[hist_idx] + raw_ir;
            ir_history[hist_idx] = raw_ir;
            uint8_t filt_ir = (uint8_t)(ir_sum >> 3);
            (void)filt_ir;
            hist_idx = (hist_idx + 1) & 7;

            /* 4. Feed 18-bit samples into SpO2 engine when tissue contact is present */
            if (raw_contact) {
                spo2_add_samples(&spo2_state, ppg_sample.red, ppg_sample.ir);
                ppg_baseline_rr_add(&s_base_rr, (float)ppg_sample.ir);
                if (spo2_is_valid(&spo2_state)) {
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.spo2_percent = spo2_get_value(&spo2_state);
                        g_state.spo2_valid = 1;
                        xSemaphoreGive(s_data_mutex);
                    }
                } else {
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.spo2_valid = 0;
                        /* Never publish pre-settling transient values (e.g. 80s ramp) */
                        g_state.spo2_percent = 0.0f;
                        xSemaphoreGive(s_data_mutex);
                    }
                }
            }

            /* 5. Systolic Peak Detection (FPGA Interrupt with Software Fallback) */
            bool fpga_beat = shrikefi_is_beat_detected();
            if (fpga_beat) {
                shrikefi_clear_irq();
            }

            if (fpga_beat && optical_contact) {
                /* Hardware beat detected by ForgeFPGA on MISO Bit 7 with verified optical tissue contact */
                uint32_t ibi_cycles = shrikefi_read_ibi_cycles();
                /* Latch liveness from the RAW beat flag, here, before the
                 * plausibility pipeline runs.
                 *
                 * This must NOT be moved inside the ibi_pipeline_submit() block
                 * below, however much more careful that looks. It was tried, and
                 * it livelocks: submit() returns false for every "prime"
                 * interval, priming needs IBI_PRIME_N consecutive plausible
                 * beats, and the first interval after any handover resets
                 * prime_count to 0. So liveness would never latch, the software
                 * fallback would keep running for want of it, and every software
                 * beat would hand the source back and flush the window - which
                 * resets priming again.
                 *
                 * Measured on hardware in that state: the two detectors
                 * alternated on every single beat, "IBI detector handover" fired
                 * several times a second, hrv_state.count stayed pinned at 0
                 * forever, and the device sat on ACQUIRING with a perfect signal
                 * (raw IR ~110000, Red ~130000, crests every second) and never
                 * published an HR, an SpO2 or a triage. The gate below depends on
                 * liveness being sticky for the whole contact session, so a
                 * spurious beat flag is the far lesser risk. */
                s_last_fpga_beat_ms = now_ms;

                float ibi_ms = (float)ibi_cycles * (20.0f / 1000000.0f); // 50 MHz clock

                /* Split-beat guard - CAUSE NOW ESTABLISHED.
                 * The detector fires twice on roughly one beat in six. The
                 * waveform the FPGA sees was captured at full rate and replayed
                 * through the RTL (hardware/shrikefi/tools/replay_fpga_link_log.py);
                 * the trace shows WHY. On the affected beats the 8-tap filtered
                 * pulse is not one hump but two: a first systolic maximum at
                 * ~187 counts, a dicrotic notch down to ~171, then a second and
                 * larger maximum at ~199. The FSM confirms a crest as soon as it
                 * sees two consecutive falling samples, so it latches onto the
                 * FIRST maximum. The second one lands ~100 ms later, inside the
                 * 250 ms refractory, so it is ignored and the next detection is
                 * a whole cardiac cycle late.
                 *
                 * That is the dicrotic notch, on finger PPG, ~100-210 ms after
                 * the first systolic peak. An earlier revision of this comment
                 * retracted the notch as the cause on the grounds that the
                 * notch sits inside the refractory; that retraction was wrong,
                 * because the notch does not need to be detected itself - it
                 * only has to make the waveform fall, which is all the FSM
                 * needs to declare the preceding sample a crest.
                 *
                 * Fixing this properly means changing the RTL (confirm a crest
                 * only after a fall of, say, 8% of the peak, or widen the
                 * averaging window), which requires re-synthesising the
                 * bitstream in the Renesas ForgeFPGA GUI. Until that happens
                 * this guard removes the symptom, and ibi_pipeline_submit()
                 * removes the residual one-in-N split with a median filter. */
                float beat_amp = (float)s_ir_peak_since_beat;
                s_ir_peak_since_beat = 0;
                if (ibi_pipeline_submit(&ibi_pipe, &hrv_state, IBI_SRC_FPGA, ibi_ms, beat_amp)) {
                    float inst_hr = 60000.0f / ibi_ms;
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.r_peak_interval_ms = ibi_ms;
                        g_state.heart_rate = (g_state.heart_rate > 30.0f) ?
                                             (0.70f * g_state.heart_rate + 0.30f * inst_hr) : inst_hr;
                        bool hrv_ready = hrv_is_ready(&hrv_state) ||
                                         (hrv_state.count >= HRV_MIN_SAMPLES && hrv_state.rmssd > 0.0f);
                        if (s_hrv_sqi_valid && hrv_ready) {
                            g_state.hrv_rmssd = (g_state.hrv_rmssd > 0.0f) ?
                                                (0.85f * g_state.hrv_rmssd + 0.15f * hrv_state.rmssd) : hrv_state.rmssd;
                            g_state.hrv_sdnn  = (g_state.hrv_sdnn > 0.0f) ?
                                                (0.85f * g_state.hrv_sdnn + 0.15f * hrv_state.sdnn) : hrv_state.sdnn;
                        } else {
                            g_state.hrv_rmssd = 0.0f;
                            g_state.hrv_sdnn  = 0.0f;
                        }
                        g_state.hrv_sample_count = hrv_state.count;
                        xSemaphoreGive(s_data_mutex);
                    }
                }
            } else if (optical_contact) {
                /* FPGA-liveness gate.
                 * The two detectors are mutually exclusive per SAMPLE, but not
                 * per HEARTBEAT: the FPGA branch above runs only on the single
                 * sample where the beat flag is set, so this software FSM still
                 * runs across every other sample and independently finds the
                 * SAME systolic peaks. Both then called hrv_add_ibi(), so each
                 * heartbeat entered the HRV buffer twice - measured on hardware
                 * as 51 intervals added for 34 FPGA-detected beats.
                 * The two detectors timestamp a beat at slightly different
                 * instants (the FPGA answers within one SPI transaction; this
                 * FSM only confirms after two falling samples), so the
                 * interleaved pairs roughly doubled RMSSD, from ~60 ms of
                 * genuine variability to ~100 ms - which in turn made the
                 * autonomic-strain terms in the risk engines read too low.
                 * While the FPGA is healthy it is the authoritative detector and
                 * the fallback must not contribute. If the FPGA goes quiet for
                 * SW_FALLBACK_ARM_MS, the fallback resumes by itself. */
                /* Sticky for the whole contact session: once the ForgeFPGA has
                 * produced a beat, it stays the authoritative detector until
                 * the finger comes off. It is NOT a rolling window.
                 *
                 * It used to be `(now_ms - s_last_fpga_beat_ms) <
                 * SW_FALLBACK_ARM_MS`, i.e. 3 s. Measured cost of that, from the
                 * 02:49 capture: the FPGA's crest detector went silent for
                 * 3.38 s four times in three minutes while the optical signal
                 * was perfectly healthy (raw IR mean 106,800, peak-to-peak
                 * ~1300, uninterrupted across the gaps). Each silence tripped
                 * the 3 s gate, flipped the source to the software fallback for
                 * a few hundred milliseconds - not even one beat at 78 BPM -
                 * and each flip flushed the entire 300-interval HRV window.
                 * The measurement restarted four times, which is why the
                 * reported RMSSD appeared to "settle" at four different values.
                 *
                 * A brief detector dropout should cost the intervals it spans,
                 * not the whole session. Swapping in a different detector
                 * mid-measurement is worse than briefly showing no data. */
                /* If ForgeFPGA detects beats, it is authoritative. If it goes quiet for >2.5s,
                 * software fallback resumes so patient vitals always register and reach 10 beats. */
                bool fpga_alive = (s_last_fpga_beat_ms != 0) && ((now_ms - s_last_fpga_beat_ms) < 2500);

                if (sw_finger_start_ms == 0) {
                    sw_finger_start_ms = now_ms;
                }

                /* Adaptive Peak Detector: tracks high-resolution optical baseline */
                sw_running_mean = (sw_running_mean * 31 + ppg_sample.ir) / 32;
                
                /* Use dynamic threshold based on recent AC amplitude (25% of AC) */
                uint32_t threshold_offset = last_ac_amplitude / 4;
                if (threshold_offset < 12) threshold_offset = 12; // Minimum 12 count threshold offset
                uint32_t threshold = sw_running_mean + threshold_offset;

                if (sw_state == SW_ARMED) {
                    if (ppg_sample.ir >= threshold) {
                        sw_state = SW_RISING;
                        sw_peak_val = ppg_sample.ir;
                        sw_fall_count = 0;
                    }
                } else if (sw_state == SW_RISING) {
                    if (ppg_sample.ir > sw_peak_val) {
                        sw_peak_val = ppg_sample.ir;
                    }
                    if (ppg_sample.ir < sw_prev_sample) {
                        if (++sw_fall_count >= 2) {
                            /* Crest confirmed: check peak prominence over baseline */
                            if (sw_peak_val > (sw_running_mean + 10)) {
                                float ibi_ms = (float)(s_total_samples - sw_last_peak_sample) * (1000.0f / PPG_FIFO_RATE_HZ);
                                sw_last_peak_sample = s_total_samples;
                                sw_state = SW_REFRACTORY;
                                sw_refractory_end_ms = now_ms + 400; /* 400ms refractory blanking */

                                /* Apply 800ms stabilization window after initial finger contact */
                                if ((now_ms - sw_finger_start_ms) > 800 && ibi_ms >= 300.0f && ibi_ms <= 2000.0f) {
                                    /* Plausibility check: reject sudden motion twitches */
                                    bool beat_plausible = true;
                                    if (sw_last_valid_ibi_ms > 0) {
                                        int32_t delta = (int32_t)ibi_ms - (int32_t)sw_last_valid_ibi_ms;
                                        if (delta < -350 || delta > 350) {
                                            beat_plausible = false;
                                        }
                                    }

                                    if (beat_plausible) {
                                        sw_last_valid_ibi_ms = ibi_ms;
                                        sw_beat_streak++;

                                        /* Keep the FSM warm for handover, but
                                         * do not let it double-count a heartbeat
                                         * the FPGA has already reported. */
                                        float beat_amp = (float)s_ir_peak_since_beat;
                                        s_ir_peak_since_beat = 0;
                                        if (!fpga_alive &&
                                            ibi_pipeline_submit(&ibi_pipe, &hrv_state,
                                                                IBI_SRC_SOFTWARE, (float)ibi_ms, beat_amp)) {
                                            float inst_hr = 60000.0f / (float)ibi_ms;
                                            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                                                g_state.r_peak_interval_ms = (float)ibi_ms;
                                                /* Require 2 consecutive valid beats before displaying HR */
                                                if (sw_beat_streak >= 2) {
                                                    g_state.heart_rate = (g_state.heart_rate > 30.0f) ?
                                                                         (0.70f * g_state.heart_rate + 0.30f * inst_hr) : inst_hr;
                                                }
                                                bool hrv_ready = hrv_is_ready(&hrv_state) ||
                                                                 (hrv_state.count >= HRV_MIN_SAMPLES && hrv_state.rmssd > 0.0f);
                                                if (s_hrv_sqi_valid && hrv_ready) {
                                                    g_state.hrv_rmssd = (g_state.hrv_rmssd > 0.0f) ?
                                                                        (0.85f * g_state.hrv_rmssd + 0.15f * hrv_state.rmssd) : hrv_state.rmssd;
                                                    g_state.hrv_sdnn  = (g_state.hrv_sdnn > 0.0f) ?
                                                                        (0.85f * g_state.hrv_sdnn + 0.15f * hrv_state.sdnn) : hrv_state.sdnn;
                                                } else {
                                                    g_state.hrv_rmssd = 0.0f;
                                                    g_state.hrv_sdnn  = 0.0f;
                                                }
                                                g_state.hrv_sample_count = hrv_state.count;
                                                xSemaphoreGive(s_data_mutex);
                                            }
                                        }
                                    } else {
                                        /* This software-only plausibility gate
                                         * rejects before the shared pipeline.
                                         * Preserve that missing-interval boundary
                                         * for RMSSD just like pipeline rejects. */
                                        hrv_break_sequence(&hrv_state);
                                    }
                                } else {
                                    /* Stabilization/range rejection also drops
                                     * this detected interval from the NN series. */
                                    hrv_break_sequence(&hrv_state);
                                }
                            } else {
                                /* Sub-threshold ripple (touching too gently) -> reject */
                                sw_state = SW_ARMED;
                            }
                        }
                    } else {
                        sw_fall_count = 0;
                    }
                } else if (sw_state == SW_REFRACTORY) {
                    if (now_ms >= sw_refractory_end_ms) {
                        sw_state = SW_ARMED;
                    }
                }
                sw_prev_sample = ppg_sample.ir;

                /* Evaluate real-time signal quality */
                bool beats_detected = (fpga_alive || g_state.heart_rate > 30.0f || sw_beat_streak > 0 || hrv_state.count > 0);
                signal_status_t current_status;
                if ((now_ms - sw_finger_start_ms) > 2500 && !beats_detected && last_ac_amplitude < 10) {
                    current_status = SIGNAL_STATUS_LOW_PERFUSION;
                } else if (hrv_state.count < 10 || !s_hrv_sqi_valid) {
                    current_status = SIGNAL_STATUS_ACQUIRING;
                } else {
                    current_status = SIGNAL_STATUS_TRACKING;
                }

                if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.signal_status = current_status;
                    xSemaphoreGive(s_data_mutex);
                }
            } else {
                /* No optical contact (debounced: 100 consecutive samples below the contact threshold) */
                s_disconnect_samples = 100;
                s_last_fpga_beat_ms  = 0;   /* FPGA beat history is scoped to this contact session */
                sw_finger_start_ms   = 0;
                sw_last_peak_sample  = 0;
                sw_last_valid_ibi_ms = 0;
                sw_beat_streak       = 0;
                sw_peak_val          = 0;
                sw_fall_count        = 0;
                sw_state             = SW_ARMED;
                last_ac_amplitude    = 0;
                ir_win_min           = UINT32_MAX;
                ir_win_max           = 0;
                ir_win_count         = 0;

                hrv_init(&hrv_state);   /* Reset HRV history on finger removal */
                ibi_pipeline_reset(&ibi_pipe); /* ...and the detector-handover state */
                shrikefi_link_reset_beat_tracking(); /* Clear inter-beat timing baseline */
                ppg_history_reset();  /* ...and the respiration/SQI windows */
                spo2_init(&spo2_state); /* Reset SpO2 history on finger removal */
                s_rail_red = 0;
                s_rail_ir  = 0;

                if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.heart_rate         = 0.0f;
                    g_state.r_peak_interval_ms = 0.0f;
                    g_state.hrv_rmssd          = 0.0f;
                    g_state.hrv_sdnn           = 0.0f;
                    g_state.hrv_sample_count   = 0;
                    g_state.spo2_valid         = 0;
                    g_state.spo2_percent       = 0.0f;
                    g_state.respiratory_rate_bpm = 0.0f;
                    g_state.ppg_sqi            = 0.0f;
                    g_state.signal_status      = SIGNAL_STATUS_NO_FINGER;
                    xSemaphoreGive(s_data_mutex);
                }
            }
        }

        if (i2c_read_failed) {
            if (max30102_i2c_error_streak < UINT16_MAX) {
                max30102_i2c_error_streak++;
            }
            if (max30102_i2c_error_streak == 3) {
                /* Back off on transient errors. Do not issue recovery writes
                 * while the bus is already failing: the old pointer/mode
                 * recovery loop multiplied I2C traffic and could make the
                 * OLED and MAX30102 appear to reset together. */
                ESP_LOGW(TAG, "MAX30102 I2C errors continuing; suppressing recovery writes until the bus is stable");
            } else if (max30102_i2c_error_streak >= 300) {
                ESP_LOGE(TAG, "MAX30102 persistent I2C failure (300 polls / ~3s); checking device and scheduling re-init");
                if (max30102_recover_fifo(&s_max30102) == 0) {
                    ESP_LOGI(TAG, "MAX30102 recovered; resuming acquisition");
                    max30102_i2c_error_streak = 0;
                } else {
                    ESP_LOGE(TAG, "MAX30102 unrecoverable; clearing vitals and scheduling re-init");
                    s_max30102.initialized = 0;
                    max30102_i2c_error_streak = 0;
                    next_max30102_retry_ms = (uint32_t)(esp_timer_get_time() / 1000) + 1000;

                    hrv_init(&hrv_state);
                    spo2_init(&spo2_state);
                    ibi_pipeline_reset(&ibi_pipe);
                    ppg_history_reset();
                    shrikefi_link_reset_beat_tracking();
                    memset(red_history, 0, sizeof(red_history));
                    memset(ir_history, 0, sizeof(ir_history));
                    hist_idx = 0;
                    red_sum = 0;
                    ir_sum = 0;
                    s_total_samples = 0;
                    s_disconnect_samples = 0;
                    sw_prev_sample = 0;
                    sw_peak_val = 0;
                    sw_last_peak_sample = 0;
                    sw_last_valid_ibi_ms = 0;
                    sw_beat_streak = 0;
                    sw_finger_start_ms = 0;
                    sw_state = SW_ARMED;
                    sw_fall_count = 0;
                    sw_running_mean = 15000;
                    s_last_fpga_beat_ms = 0;
                    ir_win_min = UINT32_MAX;
                    ir_win_max = 0;
                    ir_win_count = 0;
                    last_ac_amplitude = 0;
                    s_rail_red = 0;
                    s_rail_ir = 0;

                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        g_state.heart_rate           = 0.0f;
                        g_state.r_peak_interval_ms   = 0.0f;
                        g_state.hrv_rmssd            = 0.0f;
                        g_state.hrv_sdnn             = 0.0f;
                        g_state.hrv_sample_count     = 0;
                        g_state.spo2_valid           = 0;
                        g_state.spo2_percent         = 0.0f;
                        g_state.respiratory_rate_bpm = 0.0f;
                        g_state.rr_confidence        = 0.0f;
                        g_state.rr_rsa_depth_ms      = 0.0f;
                        g_state.ppg_sqi              = 0.0f;
                        g_state.signal_status        = SIGNAL_STATUS_NO_FINGER;
                        xSemaphoreGive(s_data_mutex);
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        max30102_i2c_error_streak = 0;

        /* Periodic optical debug log (about once a second at the ~100 Hz optical rate) */
        if (++raw_log_timer >= (PPG_SAMPLE_RATE_HZ / 2)) {
            raw_log_timer = 0;
            ppg_update_respiration_and_sqi(&hrv_state, &ibi_pipe);

            /* MISO line-level heuristic, every ~5 s. This detects a level that
             * follows the ESP32's weak pulls, but cannot detect a frozen FPGA or
             * prove that a stable level is driven by the FPGA. Do not trigger a
             * reflash from this ambiguous check. */
            s_fpga_check_counter++;
            if (s_fpga_check_counter >= 10) {
                s_fpga_check_counter = 0;
                bool miso_stable = shrikefi_link_miso_stable_under_pulls();
                if (!miso_stable && !s_miso_warning_logged) {
                    ESP_LOGW(TAG, "[FPGA] MISO level follows the weak pull test; the line may be floating. "
                                  "This check cannot determine FPGA execution state.");
                    s_miso_warning_logged = true;
                } else if (miso_stable && s_miso_warning_logged) {
                    ESP_LOGI(TAG, "[FPGA] MISO level is now stable across weak pull settings; "
                                  "FPGA execution is still unverified by this check.");
                    s_miso_warning_logged = false;
                }
            }

            signal_status_t log_sig_stat = SIGNAL_STATUS_NO_FINGER;
            float log_hr = 0.0f;
            int log_spo2_valid = 0;
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                log_sig_stat   = g_state.signal_status;
                log_hr         = g_state.heart_rate;
                log_spo2_valid = g_state.spo2_valid;
                xSemaphoreGive(s_data_mutex);
            }

            const char *status_str = (log_sig_stat == SIGNAL_STATUS_LOW_PERFUSION) ? "LOW PERFUSION (PRESS FIRMER)" :
                                     (log_sig_stat == SIGNAL_STATUS_ACQUIRING)     ? "ACQUIRING" :
                                     (log_sig_stat == SIGNAL_STATUS_TRACKING)      ? "LOCKED" : "NO FINGER";
            if (samples_read > 0) {
                ESP_LOGI("PPG_OPTICAL", "Raw: IR=%lu, Red=%lu | AC=%lu | Status: %s | HR: %.1f BPM | IBI: %d (need 10 to arm risk engines) | SpO2: %s | (Samples/tick: %d)",
                         (unsigned long)ppg_sample.ir, (unsigned long)ppg_sample.red,
                         (unsigned long)last_ac_amplitude,
                         status_str,
                         log_hr, hrv_state.count,
                         log_spo2_valid ? "VALID" : "CALC/--",
                         samples_read);
            } else {
                ESP_LOGI("PPG_OPTICAL", "No fresh MAX30102 FIFO sample; status: %s, HR: %.1f BPM, SpO2: %s",
                         status_str, log_hr, log_spo2_valid ? "VALID" : "CALC/--");
            }

            /* Why SpO2 is or is not showing yet.
             *
             * The acquisition gate needs 8 consecutive valid windows (4 s) plus a
             * flat history, but the measured time to first reading was 22 s. The
             * published value cannot say which gate was failing for the other
             * 18 s, so log the running totals and the last window's measurements.
             * reject: 0 accepted, 1 DC too low, 2 AC too small, 3 perfusion index
             * out of range, 4 ratio R out of range. */
            if (!spo2_is_valid(&spo2_state)) {
                uint8_t led_r = 0, led_ir = 0;
                if (max30102_get_led_current(&s_max30102, &led_r, &led_ir) != 0) {
                    led_r = 0; led_ir = 0;
                }
                ESP_LOGI("PPG_OPTICAL",
                         "[SPO2DIAG] windows=%lu valid=%lu consec=%d reject=%d "
                         "ir_dc=%.0f ir_ac=%.0f red_dc=%.0f red_ac=%.0f pi=%.3f R=%.3f "
                         "led_r=%u led_ir=%u hist=%d spread=%.2f raw=%.2f settled=%d",
                         (unsigned long)spo2_state.windows_total,
                         (unsigned long)spo2_state.windows_valid,
                         spo2_state.consecutive_valid, (int)spo2_state.last_reject,
                         spo2_state.last_ir_dc, spo2_state.last_ir_ac,
                         spo2_state.last_red_dc, spo2_state.last_red_ac,
                         spo2_get_perfusion_index(&spo2_state),
                         spo2_state.ratio_r,
                         (unsigned)led_r, (unsigned)led_ir,
                         spo2_state.spo2_hist_count,
                         spo2_state.last_spread,
                         spo2_state.last_spread_raw,
                         spo2_state.last_settled);
            }

            /* Calibration aid (2026-10-17): log the un-gated ratio R and the
             * engine's SpO2 every ~2 s so R can be paired with a reference
             * oximeter. Diagnostic only; remove once the curve is fitted. */
            static int s_spo2_cal_counter = 0;
            if (++s_spo2_cal_counter >= 4) { /* 4 * 0.5 s = ~2 s */
                s_spo2_cal_counter = 0;
                ESP_LOGI("PPG_CAL",
                         "[SPO2CAL] R=%.3f spo2=%.1f valid=%d pi=%.3f rej=%d "
                         "red_dc=%.0f red_ac=%.0f ir_dc=%.0f ir_ac=%.0f",
                         spo2_state.ratio_r, spo2_state.spo2,
                         spo2_is_valid(&spo2_state),
                         spo2_get_perfusion_index(&spo2_state),
                         (int)spo2_state.last_reject,
                         spo2_state.last_red_dc, spo2_state.last_red_ac,
                         spo2_state.last_ir_dc, spo2_state.last_ir_ac);
            }

            static int s_10s_diag_counter = 0;
            if (++s_10s_diag_counter >= 20) { /* 20 * 0.5s = 10s */
                s_10s_diag_counter = 0;
                float d_hr = 0.0f, d_sqi = 0.0f, d_spo2 = 0.0f, d_rr = 0.0f, d_rrconf = 0.0f;
                int d_spo2_val = 0;
                if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    d_hr       = g_state.heart_rate;
                    d_sqi      = g_state.ppg_sqi;
                    d_spo2     = g_state.spo2_percent;
                    d_spo2_val = g_state.spo2_valid;
                    d_rr       = g_state.respiratory_rate_bpm;
                    d_rrconf   = g_state.rr_confidence;
                    xSemaphoreGive(s_data_mutex);
                }
                ESP_LOGI("PPG_DIAG", "[10s Summary] HR: %.1f BPM (Conf: %.2f), SpO2: %.1f%% (Valid: %d), RR: %.1f br/min (Conf: %.2f), SDNN: %.1f ms",
                         d_hr, d_sqi, d_spo2, d_spo2_val, d_rr, d_rrconf, hrv_state.sdnn);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz poll loop to match sensor output, preventing 20ms IBI quantization jitter
    }
}

/**
 * @brief Core 1 Task: Environmental sensor acquisition, TinyML inference & OLED UI
 */
/* ---------------------------------------------------------------------------
 * Emergency assist (requirement 6)
 * ------------------------------------------------------------------------ */

/* Where the patient is, for the emergency card. Held in NVS and set once by the
 * caregiver from the local status page - see location.h. */

/* Render the framebuffer as ASCII, so the card can be verified by reading the
 * log instead of by looking at the panel.
 *
 * There is no other way to check what the display actually shows: the SSD1306 is
 * write-only over I2C, nothing in the loop can see the screen, and a card that
 * silently clips at the 128 px edge looks perfectly correct to the code that
 * drew it. Printed only on the transition into an emergency. */
static void sos_dump_framebuffer(const ssd1306_t *d) {
    /* Built up whole and emitted in ONE printf.
     *
     * Emitting it line by line looked fine in the source and was useless in
     * practice: the PPG task on the other core writes to the same console, and
     * newlib only locks per call, so its lines landed between the rows of the
     * card and shredded it. One call is atomic against the other core. */
    static char buf[SSD1306_HEIGHT * (SSD1306_WIDTH + 2) + 16];
    size_t n = 0;
    memcpy(buf, "[SOS CARD]\n", 11);
    n = 11;
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        for (int x = 0; x < SSD1306_WIDTH; x++) {
            int on = (d->framebuf[x + (y / 8) * SSD1306_WIDTH] >> (y % 8)) & 1;
            buf[n++] = on ? '#' : '.';
        }
        buf[n++] = '\n';
    }
    buf[n] = '\0';
    printf("%s", buf);
    fflush(stdout);
}

/* Full-screen emergency card.
 *
 * Replaces the dashboard entirely while an emergency is latched: whoever finds
 * the patient reads this off the screen, with no phone, network or instruction
 * needed. Every line is at most 21 characters, because the 5x7 font advances
 * 6 px per glyph on a 128 px panel - anything longer is clipped silently.
 *
 * There is no time of day to show. The board has no battery-backed RTC, so
 * offline there is no wall clock; elapsed-since-activation is printed instead,
 * which is the number that matters to whoever is standing there. */
static void sos_render_card(ssd1306_t *d, uint32_t active_ms, sos_trigger_t trigger,
                            float hr, float spo2, float rr, float sqi,
                            const char *condition) {
    /* Sized for the worst case, not the typical one: "%s" against the 20-byte
     * location plus the "LOC: " prefix is already past a 24-byte buffer, and
     * gcc's -Wformat-truncation cannot bound the float fields either. */
    char line[48];

    ssd1306_draw_string(d, 0, 0, "* MEDICAL EMERGENCY *");

    uint32_t secs = active_ms / 1000u;
    if (trigger == SOS_TRIGGER_FALL) {
        snprintf(line, sizeof(line), "FALL DETECT T+%02u:%02u",
                 (unsigned)(secs / 60u), (unsigned)(secs % 60u));
    } else {
        snprintf(line, sizeof(line), "SOS ACTIVE  T+%02u:%02u",
                 (unsigned)(secs / 60u), (unsigned)(secs % 60u));
    }
    ssd1306_draw_string(d, 0, 9, line);

    if (hr > 0.0f && spo2 > 0.0f) {
        snprintf(line, sizeof(line), "HR %3.0f bpm SpO2 %2.0f%%", hr, spo2);
    } else if (hr > 0.0f) {
        snprintf(line, sizeof(line), "HR %3.0f bpm SpO2 --", hr);
    } else {
        snprintf(line, sizeof(line), "HR -- bpm   SpO2 --");
    }
    ssd1306_draw_string(d, 0, 18, line);

    if (rr > 0.0f) {
        snprintf(line, sizeof(line), "RR %2.0f/min  SQI %.2f", rr, sqi);
    } else {
        snprintf(line, sizeof(line), "RR --/min   SQI %.2f", sqi);
    }
    ssd1306_draw_string(d, 0, 27, line);

    snprintf(line, sizeof(line), "CONDITION: %s", condition);
    ssd1306_draw_string(d, 0, 36, line);

    /* LIVE GNSS TAKES PRECEDENCE over the stored location when a fix exists:
     * a rescuer needs where the patient is NOW, not where the device was
     * deployed. The stored NVS location is only a fallback for when there is no
     * fix. */
    char loc[LOCATION_MAX_LEN];
    if (s_has_gps_fix) {
        char temp_coord[GNSS_COORD_STR_LEN];
        temp_coord[0] = '\0';
        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            strncpy(temp_coord, s_last_gps_coord, sizeof(temp_coord));
            temp_coord[sizeof(temp_coord) - 1] = '\0';
            xSemaphoreGive(s_data_mutex);
        } else {
            strncpy(temp_coord, s_last_gps_coord, sizeof(temp_coord));
            temp_coord[sizeof(temp_coord) - 1] = '\0';
        }
        location_card_text(temp_coord, loc, sizeof(loc));
    } else {
        location_card_text(location_get(), loc, sizeof(loc));
    }
    snprintf(line, sizeof(line), "LOC: %s", loc);
    ssd1306_draw_string(d, 0, 45, line);

    ssd1306_draw_string(d, 0, 54, "CALL 108 DO NOT MOVE");
}

static void task_disaster_monitor(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "Core 1: Disaster Risk Engine & TinyML Task Started.");

    hrv_state_t hrv_snapshot;
    hrv_init(&hrv_snapshot);
    static bool oled_frame_transfer_logged = false;
    static uint32_t next_oled_write_error_log_ms = 0;

    while (1) {
        env_sensors_t env = {0};
        float hr = 0.0f;
        float spo2 = 0.0f;
        signal_status_t sig_stat = SIGNAL_STATUS_NO_FINGER;
        bool vitals_ready = false;

        /* Respiration/SQI are computed every second from the beat history, which
         * exists long before the SpO2 gate opens. They used to be printed only on
         * the triage line, so a paced-breathing run that spent 95 s in ACQUIRING
         * showed 92 frames of HR and not one RR value - the estimator had been
         * producing them the whole time and the capture simply could not see it.
         * Snapshotted here so the ACQUIRING line can carry them. */
        float rr_pub      = 0.0f;
        float sqi_pub     = 0.0f;
        float rrdepth_pub = 0.0f;
        float rrconf_pub  = 0.0f;

        /* Persistent environmental sensor state (prevents collapse to 0.0C on momentary packet drops) */
        static float s_last_temp = -100.0f; /* invalid until a real BME280 sample */
        static float s_last_hum  = -1.0f;
        static float s_last_pm25 = 15.0f;
        static uint32_t s_pm25_warn_last_ms = 0;
        static uint32_t s_next_bme_retry_ms = 0;
        static uint8_t s_bme_read_errors = 0;

        uint32_t env_now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (!s_bme280.initialized &&
            (int32_t)(env_now_ms - s_next_bme_retry_ms) >= 0) {
            s_next_bme_retry_ms = env_now_ms + 5000;
            if (bme280_init(&s_bme280, esp32_i2c_hal_get_handle(), BME280_I2C_ADDR) == 0) {
                ESP_LOGI(TAG, "BME280 initialized after a runtime I2C retry");
                s_bme_read_errors = 0;
            }
        }

        /* Read BME280 (temperature, humidity, pressure) */
        bme280_data_t bme_data;
        if (s_bme280.initialized && read_bme280_env(&bme_data) == 0) {
            s_bme_read_errors = 0;
            if (bme_data.temperature_c >= 5.0f && bme_data.temperature_c <= 65.0f &&
                bme_data.humidity_pct >= 0.0f && bme_data.humidity_pct <= 100.0f) {
                s_last_temp = bme_data.temperature_c;
                s_last_hum  = bme_data.humidity_pct;
            }
            /* Pressure is independent of the temperature sanity gate above, so
             * it is fed even when the temperature reading is rejected. */
            pressure_trend_add_paced(&s_pressure_trend,
                                     (uint32_t)(esp_timer_get_time() / 1000),
                                     bme_data.pressure_hpa, &s_pressure_next_due_ms);
        } else if (s_bme280.initialized && ++s_bme_read_errors >= 3) {
            ESP_LOGW(TAG, "BME280 read failed three times; retrying sensor initialization");
            s_bme280.initialized = 0;
            s_bme_read_errors = 0;
            s_next_bme_retry_ms = env_now_ms + 5000;
            s_last_temp = -100.0f;
            s_last_hum = -1.0f;
        }
        const bool ambient_valid = (s_last_temp >= 5.0f && s_last_hum >= 0.0f);
        env.ambient_temp_c = s_last_temp;
        env.humidity_pct   = s_last_hum;

        /* Read PMS5003 (PM2.5) */
        pms5003_data_t pms_data;
        if (read_pms5003_data(&pms_data) == 0 && pms_data.valid) {
            s_last_pm25 = (float)pms_data.pm2_5_atm;
        }

        /* 3-Input INT8 Neural Network Calibration (Si et al., AMT 2019):
         * Fuses PM2.5 with BME280 temperature and relative humidity to eliminate
         * humidity-induced laser scattering bias (slashing error by 60%) */
        float calibrated_pm25 = ambient_valid
            ? pm25_calibrate_nn_int8(s_last_pm25, env.ambient_temp_c, env.humidity_pct)
            : s_last_pm25;
        env.pm25 = calibrated_pm25;

        /* Unconditionally update shared system state with live environmental telemetry */
        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            g_state.ambient_temp_c   = env.ambient_temp_c;
            g_state.humidity_percent = env.humidity_pct;
            g_state.pm25_ugm3        = env.pm25;
            xSemaphoreGive(s_data_mutex);
        }

        if (ambient_valid && pm25_is_humidity_distorted(s_last_pm25, env.humidity_pct)) {
            /* Rate-limited. High ambient RH is persistent in this environment, so
             * logging unconditionally produced ~69 identical lines per minute and
             * buried the vitals output the operator actually needs. */
            uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            if (now_ms - s_pm25_warn_last_ms >= 60000u) {
                s_pm25_warn_last_ms = now_ms;
                ESP_LOGW(TAG, "[PM2.5 Calibration] Humidity spike (RH: %.1f%%) - correction applied. Raw: %.1f -> Calibrated: %.1f ug/m3",
                         env.humidity_pct, s_last_pm25, calibrated_pm25);
            }
        } else {
            s_pm25_warn_last_ms = 0;   /* re-arm once conditions recover */
        }

        /* Get latest HR/HRV/SpO2 from Core 0 */
        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (g_state.heart_rate > 30.0f) hr = g_state.heart_rate;
            spo2 = g_state.spo2_valid ? g_state.spo2_percent : 0.0f;
            hrv_snapshot.rmssd = g_state.hrv_rmssd;
            hrv_snapshot.sdnn = g_state.hrv_sdnn;
            hrv_snapshot.count = g_state.hrv_sample_count;
            sig_stat = g_state.signal_status;
            /* SpO2 belongs in the gate, not just in the payload. All three
             * engines below are handed the same saturation and the triage line
             * publishes what they scored, so scoring a session whose SpO2 has
             * not latched yet means feeding the rule engine, the TinyML model
             * and NEWS2 a normal-looking oxygen saturation that was never
             * measured - a reassuring verdict printed beside "--" on screen.
             * The SpO2 engine latches after ~17 s of steady contact; until then
             * the ACQUIRING line reports the true state and nothing is scored. */
            vitals_ready = (hrv_snapshot.count >= 10 && hr > 30.0f && spo2 > 0.0f);
            rr_pub      = g_state.respiratory_rate_bpm;
            sqi_pub     = g_state.ppg_sqi;
            rrdepth_pub = g_state.rr_rsa_depth_ms;
            rrconf_pub  = g_state.rr_confidence;
            xSemaphoreGive(s_data_mutex);
        }

        /* Skin temperature is intentionally NOT instrumented (2026-10-17).
         *
         * The only on-board candidate was the MAX30102 die-temperature register,
         * but that is the sensor's own silicon temperature: LED self-heating puts
         * it at 40-41 C while the finger is normal, which NEWS2 then scored as a
         * high fever and turned into false CRITICAL alarms. The optional MAX30205
         * was never populated on this build.
         *
         * env.skin_temp_c therefore stays 0.0f ("unavailable"), so the disaster
         * engine uses its documented ambient-air cold-stress proxy and body
         * temperature stays uninstrumented in NEWS2. Re-enable when a real contact
         * sensor (e.g. TMP117) is fitted. */
        env.skin_temp_c = 0.0f;

        /* 1. Execute Rule-based Disaster Risk Engine & TinyML only when vitals are genuine */
        risk_assessment_t rule_risk;
        memset(&rule_risk, 0, sizeof(rule_risk));
        rule_risk.overall_risk = RISK_UNKNOWN;

        risk_assessment_t nn_risk;
        memset(&nn_risk, 0, sizeof(nn_risk));
        nn_risk.overall_risk = RISK_UNKNOWN;

        nn_output_t nn_out;
        memset(&nn_out, 0, sizeof(nn_out));

        /* Emergency assist (requirement 6) needs the clinical level outside the
         * vitals_ready branch, so the emergency state machine can be updated
         * every second whether or not a full assessment ran. -1 = none yet. */
        int triage_level = -1;
        /* Hoisted out of the vitals_ready branch: the emergency card shows the
         * same respiratory rate and signal quality the triage used, and it has
         * to be drawable whatever state the assessment is in. */
        float rr_for_news2 = 0.0f;
        float sqi_for_news2 = 0.0f;
        /* The estimator's raw inputs, for judging the gate against real data. */
        float rr_conf_dbg = 0.0f;
        float rr_depth_dbg = 0.0f;
        /* Also hoisted for the same reason: the status page shows the triage
         * the device actually produced, not a re-derivation of it. */
        unsigned clin_news2 = 0;
        unsigned clin_flags = 0;

        /* Barometric trend, evaluated and ASSESSED every second, deliberately
         * outside the assessment branch below.
         *
         * The weather does not need a finger on the sensor, and the window has to
         * keep filling while the device sits idle. Putting the assessment inside
         * the vitals_ready branch (which requires ten accepted beats) made the
         * storm advisory read UNKNOWN forever on an untouched device - verified
         * on hardware, where the fitted trend was visibly moving while the
         * published level stayed UNKNOWN - and an untouched device is exactly
         * the case where a storm warning matters. */
        pressure_trend_result_t pressure_trend;
        pressure_trend_evaluate(&s_pressure_trend, &pressure_trend);

        risk_level_t storm_risk = RISK_UNKNOWN;
        const char *storm_advisory = NULL;
        assess_cyclone_risk(&pressure_trend, &storm_risk, &storm_advisory);

        risk_assessment_t final_risk;
        memset(&final_risk, 0, sizeof(final_risk));
        final_risk.overall_risk = RISK_UNKNOWN;

        if (vitals_ready) {
            /* vitals_ready guarantees spo2 > 0, so every engine here scores a
             * measured saturation. This line was previously a "neutral 96.0f"
             * fallback for the calibrating case - but that is precisely the case
             * the gate above now excludes, and a fabricated normal SpO2 is not a
             * neutral input to a triage score, it is an optimistic one. */
            const float engine_spo2 = spo2;

            /* No skin-temperature sensor is fitted, so env.skin_temp_c is always
             * 0.0f (unavailable): the disaster engine uses its documented BME280
             * ambient cold-stress proxy, and NEWS2 has no body temperature. */
            env_sensors_t env_for_assessment = env;

            /* 1. Execute Clinical Deterministic Rule Engine */
            disaster_assess(&hrv_snapshot, engine_spo2, hr, &env_for_assessment, &rule_risk);

            /* Fold in the cyclone/storm advisory, already assessed above every
             * second so that it does not depend on a finger being present.
             *
             * It cannot come from disaster_assess() because that is a pure
             * function of one instant and a trend needs a history. It is applied
             * here, before env_fused is built, so the fusion sees it. It may only
             * ever RAISE the verdict: a steady barometer is not evidence that
             * anything else is fine, so RISK_NORMAL and RISK_UNKNOWN from the
             * trend change nothing. */
            rule_risk.cyclone_risk = storm_risk;
            rule_risk.cyclone_advisory = storm_advisory;
            if (storm_risk > RISK_NORMAL && storm_risk > rule_risk.overall_risk) {
                rule_risk.overall_risk = storm_risk;
                snprintf(rule_risk.overall_advisory, sizeof(rule_risk.overall_advisory), "%s",
                          storm_advisory ? storm_advisory : "");
            }

            /* This build has no skin-temperature sensor: the BME280 measures
             * ambient air, so env.skin_temp_c is 0 and assess_flood_risk() uses
             * its ambient cold-stress proxy instead of the clinical skin-temp
             * thresholds. This block is therefore now a FALLBACK for the case
             * where even the ambient reading is unusable (BME280 absent or out
             * of range): finalize_overall_risk() deliberately propagates a
             * single UNKNOWN, which would otherwise bury a normal heat/pollution
             * picture behind "Overall: UNKNOWN". Report the blind spot honestly
             * rather than letting it mask the modalities that did run. */
            if (rule_risk.flood_risk == RISK_UNKNOWN && rule_risk.heat_risk != RISK_UNKNOWN) {
                if (rule_risk.pollution_risk > rule_risk.heat_risk) {
                    rule_risk.overall_risk = rule_risk.pollution_risk;
                    snprintf(rule_risk.overall_advisory, sizeof(rule_risk.overall_advisory), "%s",
                             rule_risk.pollution_advisory ? rule_risk.pollution_advisory : "");
                } else {
                    rule_risk.overall_risk = rule_risk.heat_risk;
                    snprintf(rule_risk.overall_advisory, sizeof(rule_risk.overall_advisory), "%s",
                             rule_risk.heat_advisory ? rule_risk.heat_advisory : "");
                }
                rule_risk.flood_advisory =
                    "Not instrumented: neither skin nor ambient temperature available";
            }

            /* 2. Execute On-Device TinyML INT8 Neural Network (single-pass populates both risk and telemetry) */
            if (ambient_valid) {
                disaster_assess_nn_int8(&hrv_snapshot, engine_spo2, hr, &env_for_assessment, &nn_risk, &nn_out);
            }

            /* 3. Execute NEWS2 Clinical Physiological Triage Engine.
             *
             * Uses the _full form so respiratory rate and signal quality come
             * from the measurement rather than from a default. The 4-argument
             * form pins RR at a normal 14, which silently disables the
             * tachypnoea and bradypnoea terms - the most sensitive part of
             * NEWS2. RR is only trusted once the estimator reports it reliable;
             * SQI gates the whole score. */
            clinical_assessment_t clin_assess;
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                rr_for_news2  = g_state.respiratory_rate_bpm;
                sqi_for_news2 = g_state.ppg_sqi;
                rr_conf_dbg   = g_state.rr_confidence;
                rr_depth_dbg  = g_state.rr_rsa_depth_ms;
                xSemaphoreGive(s_data_mutex);
            }
            /* No body-temperature sensor is fitted (see the skin-temp note in the
             * acquisition path). Temperature therefore stays the NEWS2
             * "uninstrumented" sentinel (0.0f) and contributes no score. */
            const float news2_temp = 0.0f;
            clinical_vitals_assess_ext(hr, engine_spo2, hrv_snapshot.rmssd,
                                       rr_for_news2, sqi_for_news2, news2_temp, &clin_assess);
            triage_level = (int)clin_assess.level;
            clin_news2 = (unsigned)clin_assess.news2_score;
            clin_flags = (unsigned)clin_assess.alert_flags;

            /* 4. Unified Triage: Fuse deterministic bounds, TinyML patterns, and clinical vitals */
            risk_assessment_t env_fused = rule_risk;
            if (nn_risk.overall_risk > env_fused.overall_risk) {
                env_fused.overall_risk = nn_risk.overall_risk;
                snprintf(env_fused.overall_advisory, sizeof(env_fused.overall_advisory), "%s",
                         nn_risk.overall_advisory);
            }
            clinical_fuse_triage(&clin_assess, &env_fused, &final_risk);
            if (!ambient_valid && final_risk.overall_risk == RISK_NORMAL) {
                final_risk.overall_risk = RISK_UNKNOWN;
                snprintf(final_risk.overall_advisory, sizeof(final_risk.overall_advisory),
                         "Ambient sensor unavailable; environmental risk incomplete");
            }

            ESP_LOGI(TAG, "[ShrikeFi] HR: %.1f BPM | SpO2: %s | RMSSD: %.1f ms | RR: %.1f br/min | SQI: %.2f | AmbTemp: %.1f C | PM2.5: %.0f",
                     hr, (spo2 > 0.0f ? "VALID" : "CALC"), hrv_snapshot.rmssd,
                     rr_for_news2, sqi_for_news2, env.ambient_temp_c, env.pm25);
            /* Clean telemetry broadcast line for PC Dashboard.
             * RR and SQI are APPENDED, not inserted: the dashboard's sscanf
             * matches the six original fields and returns on the last one, so
             * trailing additions stay backward compatible with older builds of
             * the dashboard binary. */
            printf("[TELEMETRY] HR=%.1f,SPO2=%.1f,RMSSD=%.1f,TEMP=%.1f,HUM=%.1f,PM25=%.1f,RR=%.1f,SQI=%.2f,"
                   /* RRDEPTH and RRCONF are the estimator's own inputs, appended
                    * so the existing parsers are unaffected. They are published
                    * whether or not RR is, because the question they answer is
                    * why the gate did or did not open. */
                   "RRDEPTH=%.1f,RRCONF=%.2f\n",
                   hr, engine_spo2, hrv_snapshot.rmssd, env.ambient_temp_c,
                   env.humidity_pct, env.pm25, rr_for_news2, sqi_for_news2,
                   rr_depth_dbg, rr_conf_dbg);
            fflush(stdout);
            /* Label which cold-risk path produced the flood figure, so the
             * ambient proxy is never mistaken for a measured skin temperature. */
            char flood_str[40];
            snprintf(flood_str, sizeof(flood_str), "%s%s",
                     risk_level_to_string(rule_risk.flood_risk),
                     (env.skin_temp_c > 0.0f) ? "" : " (ambient proxy)");
            ESP_LOGI(TAG, "[RuleEngine] Heat: %s | Poll: %s | Flood: %s | Storm: %s (%.2f hPa/hr) => Overall: %s",
                     risk_level_to_string(rule_risk.heat_risk),
                     risk_level_to_string(rule_risk.pollution_risk),
                     flood_str,
                     risk_level_to_string(rule_risk.cyclone_risk),
                     pressure_trend.slope_hpa_per_hr,
                     risk_level_to_string(rule_risk.overall_risk));
            ESP_LOGI(TAG, "[TinyML INT8] Heat: %.3f (%s) | Poll: %.3f (%s) | Flood: %.3f (%s) => AI Overall: %s",
                     nn_out.heat_score, risk_level_to_string(nn_risk.heat_risk),
                     nn_out.pollution_score, risk_level_to_string(nn_risk.pollution_risk),
                     nn_out.flood_score, risk_level_to_string(nn_risk.flood_risk),
                     risk_level_to_string(nn_risk.overall_risk));
            ESP_LOGI(TAG, "[Unified Triage] Final Condition: %s | %s",
                     risk_level_to_string(final_risk.overall_risk), final_risk.overall_advisory);

            /* Structured verdict line for any dashboard that is not this device.
             *
             * [TELEMETRY] above carries raw vitals only. That meant a dashboard
             * had to re-derive NEWS2, the three hazard risks and the TinyML
             * scores from the raw numbers - a second implementation of the
             * clinical logic, running on the PC, in a project whose whole claim
             * is edge-first. This line publishes what the device already
             * computed, so a dashboard is a pure view and the device stays the
             * single source of truth.
             *
             * LEVEL is the clinical_risk_level_t ordinal (0 normal, 1 elevated,
             * 2 high, 3 critical) and FLAGS is the clinical_alert_flags_t
             * bitmask - both are enum mirrors, not derived logic, so a consumer
             * does not have to interpret anything.
             *
             * Moran PSI and the AHA autonomic strain were previously computed
             * only inside the host test harness and never on the live path, so
             * the device had no value for them at all. Both are cited indices
             * (Moran 1998; Brook et al., AHA 2010) and the functions were
             * already linked in, so they are now computed for real. */
            float moran_psi   = disaster_calculate_moran_psi(hr, env.ambient_temp_c, env.humidity_pct);
            float aha_strain  = disaster_calculate_aha_autonomic_strain(env.pm25, hrv_snapshot.rmssd);

            printf("[TRIAGE] NEWS2=%u,LEVEL=%u,FLAGS=0x%02X,RISK=%s,"
                   "NNHEAT=%.3f,NNPOLL=%.3f,NNFLOOD=%.3f,"
                   "RHEAT=%s,RPOLL=%s,RFLOOD=%s,PSI=%.2f,AHA=%.2f,"
                   /* CYCLONE is the storm level from the barometer TREND and
                    * PTREND is the fitted rate in hPa/hr (negative = falling).
                    * Appended, not inserted, so existing parsers keep working. */
                   "CYCLONE=%s,PTREND=%.2f\n",
                   (unsigned)clin_assess.news2_score,
                   (unsigned)clin_assess.level,
                   (unsigned)clin_assess.alert_flags,
                   risk_level_to_string(final_risk.overall_risk),
                   nn_out.heat_score, nn_out.pollution_score, nn_out.flood_score,
                   risk_level_to_string(rule_risk.heat_risk),
                   risk_level_to_string(rule_risk.pollution_risk),
                   risk_level_to_string(rule_risk.flood_risk),
                   moran_psi, aha_strain,
                   risk_level_to_string(rule_risk.cyclone_risk),
                   pressure_trend.slope_hpa_per_hr);
            fflush(stdout);

            /* Thread-safe state update for telemetry & system monitoring */
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                g_state.risk_result      = final_risk;
                g_state.nn_scores        = nn_out;
                xSemaphoreGive(s_data_mutex);
            }

            /* Publish to Cloud Dashboard */
            cloud_publish_health_data(hr, hrv_snapshot.rmssd, engine_spo2, env.ambient_temp_c, env.pm25, risk_level_to_string(final_risk.overall_risk));
        } else {
            /* Print the numeric HR while acquiring. This branch previously
             * printed the literal string "LOCKED" into the HR field whenever
             * hr > 30, producing lines such as "HR=LOCKED SpO2=VALID" that read
             * like a value and sent hardware bring-up down the wrong path. The
             * HRV window simply is not full yet; the HR itself is already known. */
            char hr_str[12];
            if (hr > 30.0f) {
                snprintf(hr_str, sizeof(hr_str), "%.1f", hr);
            } else {
                snprintf(hr_str, sizeof(hr_str), "--");
            }
            ESP_LOGI(TAG, "[ShrikeFi] Vitals: HR=%s BPM SpO2=%s | Status: %s | IBI samples: %d (need %d for risk engines) | Temp: %.1f C | PM2.5: %.0f",
                     hr_str,
                     (spo2 > 0.0f ? "VALID" : "--"),
                     (sig_stat == SIGNAL_STATUS_LOW_PERFUSION) ? "LOW PERFUSION (PRESS FIRMER)" :
                     (sig_stat == SIGNAL_STATUS_ACQUIRING)     ? "ACQUIRING" :
                     (sig_stat == SIGNAL_STATUS_TRACKING)      ? "READY" : "WAITING",
                     hrv_snapshot.count, 10, env.ambient_temp_c, env.pm25);
            /* This branch runs while the HRV window is not yet full - but that
             * is NOT the same thing as "no finger". It previously printed
             * "[TELEMETRY] NO_FINGER" unconditionally, so for the first ~8 s of
             * every contact the host dashboard was told there was no finger
             * while the device was in fact locked on and reporting a heart rate
             * and a valid SpO2. The dashboard dropped those HR/SpO2 values on
             * the floor (it only accepts them from the "HR=..." packet) and
             * flickered its contact indicator. Report the real state instead. */
            if (sig_stat == SIGNAL_STATUS_NO_FINGER) {
                printf("[TELEMETRY] NO_FINGER,TEMP=%.1f,HUM=%.1f,PM25=%.1f\n",
                       env.ambient_temp_c, env.humidity_pct, env.pm25);
            } else {
                float tele_spo2 = (spo2 > 0.0f) ? spo2 : g_state.spo2_percent;
                printf("[TELEMETRY] ACQUIRING,HR=%.1f,SPO2=%.1f,RMSSD=%.1f,TEMP=%.1f,HUM=%.1f,PM25=%.1f,RR=%.1f,SQI=%.2f,RRDEPTH=%.1f,RRCONF=%.2f\n",
                       hr, tele_spo2, hrv_snapshot.rmssd,
                       env.ambient_temp_c, env.humidity_pct, env.pm25,
                       rr_pub, sqi_pub, rrdepth_pub, rrconf_pub);
            }
            fflush(stdout);
        }

        /* 2. Emergency assist (requirement 6).
         *
         * Updated before the display, so the state machine and the card can
         * never disagree about whether an emergency is on: whatever
         * sos_update() decides is what gets drawn immediately below. */
        uint32_t sos_now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        bool contact_present = (sig_stat != SIGNAL_STATUS_NO_FINGER);
        sos_state_t sos_was = SOS_IDLE;
        sos_state_t sos_now = SOS_IDLE;
        sos_trigger_t sos_trig = SOS_TRIGGER_NONE;
        uint32_t sos_rem_ms = 0;
        uint32_t sos_act_ms = 0;

        if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            sos_lock();
            sos_was = sos_get_state();
            sos_update(sos_now_ms, triage_level == (int)CLINICAL_CRITICAL, contact_present);
            sos_now = sos_get_state();
            sos_trig = sos_get_trigger();
            if (sos_now == SOS_ACTIVE) {
                sos_act_ms = sos_active_ms(sos_now_ms);
            } else if (sos_now == SOS_ARMED && sos_trig == SOS_TRIGGER_FALL) {
                sos_rem_ms = sos_armed_remaining_ms(sos_now_ms);
            }
            sos_unlock();
            xSemaphoreGive(s_data_mutex);
        }

        if (sos_was != SOS_ACTIVE && sos_now == SOS_ACTIVE) {
            ESP_LOGW(TAG, "[SOS] EMERGENCY LATCHED - trigger: %s (level %d, contact %d)",
                     sos_trigger_name(sos_trig), triage_level,
                     (int)contact_present);
        } else if (sos_was == SOS_ACTIVE && sos_now != SOS_ACTIVE) {
            ESP_LOGI(TAG, "[SOS] stood down -> %s", sos_state_name(sos_now));
        }

        /* Publish the status-page snapshot.
         *
         * The page renders and derives nothing: every number here was computed
         * on the device, which is the same rule the desktop dashboard follows.
         * Publishing unconditionally - not only during an emergency - is what
         * makes the page useful to a rescuer who arrives before any alarm. */
        {
            valor_status_t st;
            memset(&st, 0, sizeof(st));
            st.hr       = hr;
            st.spo2     = (spo2 > 0.0f) ? spo2 : 0.0f;
            st.rr       = rr_for_news2;
            st.sqi      = sqi_for_news2;
            st.rmssd    = hrv_snapshot.rmssd;
            st.temp     = env.ambient_temp_c;
             
            st.hum      = env.humidity_pct;
            st.pm25     = env.pm25;
            st.pressure_hpa = pressure_trend.pressure_hpa;
            st.pressure_trend_hpa_per_hr = pressure_trend.slope_hpa_per_hr;
            st.news2    = clin_news2;
            st.storm    = risk_level_to_string(storm_risk);
            st.level    = (triage_level > 0) ? (unsigned)triage_level : 0u;
            st.flags    = clin_flags;
            st.risk     = risk_level_to_string(final_risk.overall_risk);
            st.sos      = sos_state_name(sos_now);
            st.trigger  = sos_trigger_name(sos_trig);
            const char *loc_str = NULL;
            /* Static, not automatic: the published status snapshot stores this
             * pointer and the HTTP task dereferences it after this task's stack
             * frame is gone. A stack buffer here was a use-after-scope (the
             * snapshot's loc pointer outlived the block). Static storage makes
             * the pointer valid for the program's lifetime. */
            static char live_gps_buf[GNSS_COORD_STR_LEN];
            live_gps_buf[0] = '\0';
            st.uptime_s = sos_now_ms / 1000u;
            st.contact  = contact_present;
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                st.motion = g_state.imu_motion_energy;
                st.fall   = fall_state_name(g_state.fall_state);
                /* Live GNSS takes precedence over the stored location. */
                if (s_has_gps_fix && s_last_gps_coord[0] != '\0') {
                    strncpy(live_gps_buf, s_last_gps_coord, sizeof(live_gps_buf));
                    live_gps_buf[sizeof(live_gps_buf) - 1] = '\0';
                    loc_str = live_gps_buf;
                }
                xSemaphoreGive(s_data_mutex);
            }
            if (loc_str == NULL) loc_str = location_get(); /* stored / "UNSET" */
            st.loc      = loc_str;
            web_status_publish(&st);

            /* 1 Hz location line for the USB dashboard: live GNSS when fixed,
             * otherwise the stored location. */
            printf("[LOC] %s\n", (st.loc && st.loc[0]) ? st.loc : "UNSET");
            fflush(stdout);

            /* Periodically publish hourly history summary for WebSerial (every 30s) */
            static uint32_t s_hist_print_sec = 0;
            if (++s_hist_print_sec >= 30) {
                s_hist_print_sec = 0;
                valor_history_point_t all_pts[VALOR_HISTORY_HOURS];
                web_status_history_snapshot(all_pts, VALOR_HISTORY_HOURS);
                for (size_t h = 0; h < VALOR_HISTORY_HOURS; h++) {
                    if (all_pts[h].valid) {
                        printf("[HISTORY] H%u,HR=%.1f,SPO2=%.1f,RMSSD=%.1f,NEWS2=%u,PM25=%.1f\n",
                               (unsigned)all_pts[h].hour_offset,
                               all_pts[h].avg_hr, all_pts[h].avg_spo2, all_pts[h].avg_rmssd,
                               all_pts[h].max_news2, all_pts[h].peak_pm25);
                    }
                }
                fflush(stdout);
            }
        }

        /* 3. Render the OLED: the emergency card replaces the dashboard
         * entirely while an emergency is latched. */
        /* A failed boot-time OLED probe used to disable the display for the
         * entire run. Retry at a low rate so a transient bus/device startup
         * failure does not permanently leave the vitals screen blank. */
        static uint32_t next_oled_retry_ms = 0;
        static uint32_t next_oled_retry_log_ms = 0;
        uint32_t oled_now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (!s_ssd1306.initialized &&
            (int32_t)(oled_now_ms - next_oled_retry_ms) >= 0) {
            next_oled_retry_ms = oled_now_ms + 5000;
            int oled_rc = ssd1306_init(&s_ssd1306, esp32_i2c_hal_get_handle(),
                                       SSD1306_I2C_ADDR);
            if (oled_rc != 0) {
                oled_rc = ssd1306_init(&s_ssd1306, esp32_i2c_hal_get_handle(),
                                       SSD1306_I2C_ALT);
            }
            if (oled_rc == 0) {
                ESP_LOGI(TAG, "SSD1306 OLED initialized after a runtime I2C retry at 0x%02X",
                         s_ssd1306.addr);
            } else if ((int32_t)(oled_now_ms - next_oled_retry_log_ms) >= 0) {
                ESP_LOGW(TAG, "OLED is still unavailable; retrying in 5 seconds");
                next_oled_retry_log_ms = oled_now_ms + 30000;
            }
        }

        if (s_ssd1306.initialized) {
            ssd1306_clear(&s_ssd1306);

            if (sos_now == SOS_ACTIVE) {
                sos_render_card(&s_ssd1306, sos_act_ms, sos_trig, hr, spo2,
                                rr_for_news2, sqi_for_news2,
                                risk_level_to_string(final_risk.overall_risk));
                if (sos_was != SOS_ACTIVE) {
                    /* First frame of the emergency: verify what was drawn. */
                    sos_dump_framebuffer(&s_ssd1306);
                }
            } else if (sos_now == SOS_ARMED && sos_trig == SOS_TRIGGER_FALL) {
                /* Render fall countdown warning card: alerts patient to press cancel if false alarm */
                unsigned rem_sec = (sos_rem_ms + 999u) / 1000u;
                ssd1306_draw_string(&s_ssd1306, 4, 2, "! FALL DETECTED !");
                ssd1306_draw_hline(&s_ssd1306, 0, 11, 128);
                char warn[32];
                snprintf(warn, sizeof(warn), "ALARM IN: %u SEC", rem_sec);
                ssd1306_draw_string(&s_ssd1306, 8, 16, warn);
                ssd1306_draw_string(&s_ssd1306, 8, 28, "HOLD CANCEL TO STOP");
                ssd1306_draw_string(&s_ssd1306, 8, 40, "RESTORING IF ACTIVE");
                ssd1306_draw_hline(&s_ssd1306, 0, 52, 128);
                ssd1306_draw_string(&s_ssd1306, 8, 55, "EMERGENCY COUNTDOWN");
            } else {

            // Header: Page 0 (rows 0-7)
            ssd1306_draw_string(&s_ssd1306, 19, 0, "VALOR COMPANION");
            // Divider 1: Page 1 (row 8)
            ssd1306_draw_hline(&s_ssd1306, 0, 8, 128);

            // Line 1: Vitals (HR & SpO2) - Page 2 (rows 16-23)
            char buf_vitals[32];
            float disp_spo2 = (spo2 > 0.0f) ? spo2 : 0.0f;
            if (hr > 30.0f && disp_spo2 > 0.0f) {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR:%3.0fBPM SpO2:%2.0f%%", hr, disp_spo2);
            } else if (hr > 30.0f) {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR:%3.0fBPM SpO2: -- ", hr);
            } else if (disp_spo2 > 0.0f) {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR: --   SpO2:%2.0f%%", disp_spo2);
            } else {
                snprintf(buf_vitals, sizeof(buf_vitals), "HR: --   SpO2: -- ");
            }
            ssd1306_draw_string(&s_ssd1306, 2, 16, buf_vitals);

            // Line 2: Ambient / PM2.5 - Page 3 (rows 24-31). No skin temperature
            // is instrumented (see the acquisition-path note).
            char buf_env[32];
            if (!ambient_valid) {
                snprintf(buf_env, sizeof(buf_env), "Ta:--   PM:%3.0f", env.pm25);
            } else {
                snprintf(buf_env, sizeof(buf_env), "T:%4.1fC   PM:%3.0f",
                         env.ambient_temp_c, env.pm25);
            }
            ssd1306_draw_string(&s_ssd1306, 2, 24, buf_env);

            // Line 3: HRV RMSSD or Guidance Feedback - Page 4 (rows 32-39)
            char buf_hrv[32];
            if (sig_stat == SIGNAL_STATUS_NO_FINGER) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Touch MAX30102...");
            } else if (sig_stat == SIGNAL_STATUS_LOW_PERFUSION) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Press Firmer...");
            } else if (hrv_snapshot.count < 10) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Reading... (%d/10)", hrv_snapshot.count);
            } else if (hrv_snapshot.rmssd > 0.0f) {
                snprintf(buf_hrv, sizeof(buf_hrv), "HRV RMSSD:%4.1fms", hrv_snapshot.rmssd);
            } else if (sqi_pub < 0.50f) {
                snprintf(buf_hrv, sizeof(buf_hrv), "Hold Finger Still");
            } else {
                snprintf(buf_hrv, sizeof(buf_hrv), "HRV: Stabilizing");
            }
            ssd1306_draw_string(&s_ssd1306, 2, 32, buf_hrv);

            // Divider 2: Page 5 (row 40)
            ssd1306_draw_hline(&s_ssd1306, 0, 40, 128);

            // Line 4: Health Condition Alert (TinyML & Disaster Engine) - Page 6 (rows 48-55)
            char buf_cond[32];
            if (sig_stat == SIGNAL_STATUS_NO_FINGER) {
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: WAITING");
            } else if (sig_stat == SIGNAL_STATUS_LOW_PERFUSION) {
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: LOW PERF");
            } else if (hrv_snapshot.count < 10) {
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: CALC...");
            } else {
                const char *risk_str = risk_level_to_string(final_risk.overall_risk);
                snprintf(buf_cond, sizeof(buf_cond), "CONDITION: %s", risk_str);
            }
            ssd1306_draw_string(&s_ssd1306, 2, 48, buf_cond);
            }   /* end normal dashboard */

            static uint8_t oled_write_failures = 0;
            int oled_update_rc = ssd1306_update(&s_ssd1306);
            uint32_t oled_update_now_ms = (uint32_t)(esp_timer_get_time() / 1000);
            if (oled_update_rc == 0) {
                oled_write_failures = 0;
                if (!oled_frame_transfer_logged) {
                    ESP_LOGI(TAG, "OLED dashboard framebuffer transferred successfully over I2C");
                    oled_frame_transfer_logged = true;
                }
            } else {
                oled_frame_transfer_logged = false;
                if (oled_write_failures < UINT8_MAX) oled_write_failures++;
                if ((int32_t)(oled_update_now_ms - next_oled_write_error_log_ms) >= 0) {
                    ESP_LOGW(TAG, "OLED framebuffer write failed (%u/3); preserving display until repeated failures",
                             (unsigned)oled_write_failures);
                    next_oled_write_error_log_ms = oled_update_now_ms + 30000;
                }
                /* Do not reinitialize on one transient NACK. Three consecutive
                 * failed frame transfers mean the display is no longer usable;
                 * mark it offline so the existing 5 s retry path can perform a
                 * clean initialization. */
                if (oled_write_failures >= 3) {
                    s_ssd1306.initialized = 0;
                    oled_write_failures = 0;
                    next_oled_retry_ms = oled_update_now_ms + 5000;
                }
            }
        }

#if defined(CONFIG_SHRIKEFI_SOS_CANCEL_GPIO) && CONFIG_SHRIKEFI_SOS_CANCEL_GPIO >= 0
        for (int i = 0; i < 20; i++) {
            uint32_t tick_ms = (uint32_t)(esp_timer_get_time() / 1000);
            /* Active low. 30ms debounce implicitly handled by polling at 50ms intervals. */
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                sos_lock();
                if (gpio_get_level(CONFIG_SHRIKEFI_SOS_CANCEL_GPIO) == 0) {
                    sos_cancel_press(tick_ms);
                    sos_update(tick_ms, triage_level == (int)CLINICAL_CRITICAL, contact_present);
                } else {
                    sos_cancel_release();
                }
                sos_unlock();
                xSemaphoreGive(s_data_mutex);
            }

#if defined(PIN_BUZZER) && PIN_BUZZER >= 0
            /* Drive Active Buzzer on GPIO 4:
             * - SOS_ACTIVE: Pulsing loud emergency siren (100ms ON / 100ms OFF)
             * - SOS_ARMED (Fall countdown): 100ms alert chirp at start of each second
             * - Normal: Silenced (LOW) */
            int buzzer_val = 0;
            if (sos_now == SOS_ACTIVE) {
                buzzer_val = (i % 4 < 2) ? 1 : 0;
            } else if (sos_now == SOS_ARMED && sos_trig == SOS_TRIGGER_FALL) {
                buzzer_val = (i < 2) ? 1 : 0;
            }
            gpio_set_level((gpio_num_t)PIN_BUZZER, buzzer_val);
#endif

            vTaskDelay(pdMS_TO_TICKS(50));
        }
#else
#if defined(PIN_BUZZER) && PIN_BUZZER >= 0
        gpio_set_level((gpio_num_t)PIN_BUZZER, (sos_now == SOS_ACTIVE) ? 1 : 0);
#endif
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz assessment rate
#endif
#if defined(PIN_BUZZER) && PIN_BUZZER >= 0
        if (sos_now != SOS_ACTIVE && sos_now != SOS_ARMED) {
            gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
        }
#endif
    }
}

static void task_pms5003_uart(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "PMS5003 UART RX Task Started.");

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(s_pms5003.uart_num, &byte, 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            pms5003_feed_byte(&s_pms5003, byte);
        }
    }
}

static void task_gps_uart(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "GNSS (u-blox NEO-M8N / Quectel L89HS) UART RX Task Started on UART%d (RX=GPIO%d, TX=GPIO%d, 9600 8N1).",
             GPS_UART_NUM, PIN_GPS_RX, PIN_GPS_TX);

    uint8_t rx_buf[64];
    while (1) {
        int len = uart_read_bytes(s_gnss.uart_num, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
        for (int i = 0; i < len; i++) {
            if (gnss_nmea_feed_byte(&s_gnss, rx_buf[i]) == 1) {
                gnss_data_t gdata;
                if (gnss_nmea_get_data(&s_gnss, &gdata) == 0 && gdata.fix_valid) {
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        s_has_gps_fix = true;
                        gnss_nmea_format_coords(&gdata, s_last_gps_coord, sizeof(s_last_gps_coord));
                        xSemaphoreGive(s_data_mutex);
                    }
                } else if (!gnss_nmea_has_fix(&s_gnss)) {
                    if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        s_has_gps_fix = false;
                        xSemaphoreGive(s_data_mutex);
                    }
                }
            }
        }

        /* Check for fix timeout (5 seconds without valid fix de-asserts live fix) */
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if (gnss_nmea_expire_fix(&s_gnss, now_ms, 5000)) {
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                s_has_gps_fix = false;
                xSemaphoreGive(s_data_mutex);
            }
        }

        /* Periodic GNSS diagnostic report (every 5 seconds) */
        static uint32_t s_last_gps_log_ms = 0;
        if (now_ms - s_last_gps_log_ms >= 5000) {
            s_last_gps_log_ms = now_ms;
            gnss_data_t gdata;
            gnss_nmea_get_data(&s_gnss, &gdata);
            if (gdata.fix_valid) {
                char coord_str[GNSS_COORD_STR_LEN];
                gnss_nmea_format_coords(&gdata, coord_str, sizeof(coord_str));
                ESP_LOGI("GNSS", "[3D FIX LOCKED] Coords: %s | Sats: %u | Alt: %.1fm | HDOP: %.2f",
                         coord_str, gdata.satellites, gdata.altitude_m, gdata.hdop);
            } else if (gdata.sentences_received > 0) {
                ESP_LOGI("GNSS", "[SEARCHING SATELLITES] Sats: %u | NMEA parsed: %lu | Fix: NO (PPS LED steady; place antenna near window)",
                         gdata.satellites, (unsigned long)gdata.sentences_parsed);
            } else {
                ESP_LOGW("GNSS", "[NO UART DATA] Waiting for NMEA packets on GPIO%d (Wiring: NEO-M8N TX->GPIO16, VCC=3.3V)",
                         PIN_GPS_RX);
            }
        }
    }
}

static void task_imu_monitor(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "Core 1: IMU & Fall Detection Task Started (MPU6050 at 40 Hz).");
    mpu6050_data_t imu_data;
    static bool s_pending_fall_arm = false;
    static uint32_t s_next_imu_retry_ms = 0;
    static uint8_t s_imu_read_errors = 0;

    while (1) {
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (!s_mpu6050.initialized &&
            (int32_t)(now_ms - s_next_imu_retry_ms) >= 0) {
            s_next_imu_retry_ms = now_ms + 5000;
            if (mpu6050_init(&s_mpu6050, esp32_i2c_hal_get_handle(), MPU6050_I2C_ADDR) == 0) {
                fall_detect_init(&s_fall_detector, NULL);
                s_pending_fall_arm = false;
                s_imu_read_errors = 0;
                ESP_LOGI(TAG, "MPU6050 initialized after a runtime I2C retry");
            }
        }
        if (s_mpu6050.initialized && mpu6050_read(&s_mpu6050, &imu_data) == 0) {
            s_imu_read_errors = 0;
            bool fall_occurred = fall_detect_update(&s_fall_detector, now_ms,
                                                    imu_data.ax, imu_data.ay, imu_data.az,
                                                    imu_data.gx, imu_data.gy, imu_data.gz);
            if (fall_occurred) {
                s_pending_fall_arm = true;
            }
            if (xSemaphoreTake(s_data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                if (s_pending_fall_arm) {
                    ESP_LOGW(TAG, "[IMU] FALL DETECTED! Arming emergency assist with 10s grace...");
                    sos_arm_fall(now_ms);
                    s_pending_fall_arm = false;
                }

                /* Clear latched fall detector alarm if operator/patient cancelled SOS or SOS stood down */
                if ((sos_get_state() == SOS_CANCELLED || sos_get_state() == SOS_IDLE) &&
                    fall_detect_is_confirmed(&s_fall_detector)) {
                    fall_detect_clear_alarm(&s_fall_detector);
                }

                g_state.imu_motion_energy = fall_detect_get_motion_energy(&s_fall_detector);
                g_state.fall_state        = fall_detect_get_state(&s_fall_detector);
                xSemaphoreGive(s_data_mutex);
            }
        } else if (s_mpu6050.initialized && ++s_imu_read_errors >= 5) {
            ESP_LOGW(TAG, "MPU6050 read failed five times; retrying sensor initialization");
            s_mpu6050.initialized = 0;
            s_imu_read_errors = 0;
            s_next_imu_retry_ms = now_ms + 5000;
        }
        vTaskDelay(pdMS_TO_TICKS(25)); /* 40 Hz kinematics sampling */
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "================================================================");
    ESP_LOGI(TAG, "  SIH26181 ShrikeFi Health Companion Firmware");
    ESP_LOGI(TAG, "  ESP32-S3 + Renesas ForgeFPGA SPI-Linked System");
    ESP_LOGI(TAG, "  Build: " __DATE__ " " __TIME__ " | Commit: %s", GIT_COMMIT_HASH);
    ESP_LOGI(TAG, "================================================================");

    s_data_mutex = xSemaphoreCreateMutex();

    /* Initialize I2C HAL for sensors. The return value used to be discarded, so a
     * failed peripheral init produced no diagnostic at all: every sensor then read
     * back garbage, which looks identical to a wiring fault. Report it here. */
    if (esp32_i2c_hal_init(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_SPEED_HZ) != I2C_HAL_SUCCESS) {
        ESP_LOGE(TAG, "I2C HAL init FAILED for SDA=GPIO%d SCL=GPIO%d. No sensor will "
                      "respond; the readings below will be all-ones/garbage. This is a "
                      "software/peripheral fault, NOT wiring.",
                 PIN_I2C_SDA, PIN_I2C_SCL);
    } else {
        ESP_LOGI(TAG, "I2C HAL ready: SDA=GPIO%d SCL=GPIO%d at %d Hz; probing sensors.",
                 PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_SPEED_HZ);
        esp32_i2c_hal_scan();
    }

    /* ForgeFPGA bitstream delivery over SPI2, using the Vicharak
     * Web_FPGA_programmer sequence. On success the boot log reads
     * "configuration COMPLETE! (46408 bytes loaded)".
     *
     * The transfer is open-loop - it cannot confirm the FPGA received anything -
     * so this result is advisory. Non-OK is NOT fatal: the SPI link is the
     * runtime bus and does not depend on this call, so boot continues either way.
     *
     * The 0x55 handshake further down is not the confirmation either: the RTL
     * overwrites its response register with {beat_latched, filt_sample[6:0]} on
     * the clock after reset, so the reply is 0x00 and the probe byte is never
     * echoed. A stable MISO level across weak pull settings is only a line-level
     * heuristic; a beat response after sensor contact is required to confirm
     * that the configured FPGA design is executing. */
    if (shrikefi_fpga_flash_init() == SHRIKEFI_OK) {
        ESP_LOGI(TAG, "ForgeFPGA SLG47910 bitstream programmed successfully over SPI!");
    } else {
        ESP_LOGW(TAG, "ForgeFPGA SPI programming did not complete. Falling back to dual-core MCU DSP.");
    }

    /* Initialize the runtime SPI link to the ForgeFPGA */
    shrikefi_link_init();
    shrikefi_set_threshold(120);

    /* Radio.
     *
     * The SoftAP status page REPLACES the station path rather than joining it.
     * Cloud publishing is off by default, so the station connection had no
     * function beyond its own failure modes - and an AP+STA radio forces the
     * access point onto whatever channel the station lands on, which would drop
     * a connected phone mid-demonstration. The access point depends on nothing
     * external: no router, no credentials, nothing to associate with, which is
     * the whole point of the requirement it serves. */
#if defined(CONFIG_SHRIKEFI_SOFTAP) && CONFIG_SHRIKEFI_SOFTAP
    if (web_status_start() != ESP_OK) {
        ESP_LOGW(TAG, "Status page unavailable - vitals monitoring is unaffected");
    }
#else
    wifi_mqtt_init();
#endif

    /* Initialize SSD1306 OLED (check default 0x3C, fallback to 0x3D) */
    if (ssd1306_init(&s_ssd1306, esp32_i2c_hal_get_handle(), SSD1306_I2C_ADDR) == 0) {
        ESP_LOGI(TAG, "SSD1306 OLED initialized OK at 0x3C");
    } else if (ssd1306_init(&s_ssd1306, esp32_i2c_hal_get_handle(), SSD1306_I2C_ALT) == 0) {
        ESP_LOGI(TAG, "SSD1306 OLED initialized OK at 0x3D");
    } else {
        ESP_LOGW(TAG, "SSD1306 OLED not found at 0x3C or 0x3D (check SDA=GPIO%d, SCL=GPIO%d, VCC=3.3V, GND)", PIN_I2C_SDA, PIN_I2C_SCL);
    }

    if (s_ssd1306.initialized) {
        ssd1306_clear(&s_ssd1306);
        ssd1306_draw_string(&s_ssd1306, 19, 4, "VALOR COMPANION");
        ssd1306_draw_hline(&s_ssd1306, 0, 14, 128);
        ssd1306_draw_string(&s_ssd1306, 14, 20, "QUALCOMM SoC");
        ssd1306_draw_string(&s_ssd1306, 8, 34, "ESP32-S3 + FPGA");
        ssd1306_draw_string(&s_ssd1306, 18, 48, "Starting...");
        ssd1306_update(&s_ssd1306);
    }

    /* Emergency assist (requirement 6). Defaults: 3 s CRITICAL confirm, 5 s
     * cancel hold, 30 s recovery stand-down, and the contact-loss trigger OFF
     * because the finger leaves this device constantly in normal use. See sos.h
     * for why each of those is the value it is. */
    sos_init(NULL);
    /* Load the deployed location before anything can display it, so the card
     * and the page never briefly show UNSET on a device that has one stored. */
    location_init();
    /* The pressure window starts empty and fills at one sample a minute, so the
     * storm advisory reports UNKNOWN for the first half hour rather than
     * inventing a trend from two readings. */
    pressure_trend_init(&s_pressure_trend);
    /* The respiratory-rate stabiliser starts empty too. */
    ppg_rr_tracker_init(&s_rr_tracker);
    ESP_LOGI(TAG, "Emergency assist ready (state: %s), location '%s'",
             sos_state_name(sos_get_state()), location_get());

    /* Initialize MAX30102 (PPG sensor) */
    if (max30102_init(&s_max30102, esp32_i2c_hal_get_handle()) != 0) {
        ESP_LOGE(TAG, "MAX30102 initialization failed!");
    } else {
        ESP_LOGI(TAG, "MAX30102 initialized OK");
    }

    /* No skin-temperature sensor is probed: the MAX30205 was never populated and
     * the MAX30102 die-temperature proxy is not a body temperature. Add a TMP117
     * probe here when that sensor is fitted. */

    /* Initialize BME280 (environmental sensor) */
    if (bme280_init(&s_bme280, esp32_i2c_hal_get_handle(), BME280_I2C_ADDR) != 0) {
        ESP_LOGE(TAG, "BME280 initialization failed!");
    } else {
        ESP_LOGI(TAG, "BME280 initialized OK");
    }

    /* Initialize PMS5003 (PM2.5 sensor) on UART1 */
    if (pms5003_init(&s_pms5003, UART_NUM_1) != 0) {
        ESP_LOGE(TAG, "PMS5003 initialization failed!");
    } else {
        ESP_LOGI(TAG, "PMS5003 initialized OK");
    }

    /* Configure UART1 for PMS5003 (TX=GPIO18, RX=GPIO14, 9600 baud) */
    uart_config_t uart_cfg = {
        .baud_rate = PMS5003_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_1, &uart_cfg);
    uart_set_pin(UART_NUM_1, PIN_PMS5003_TX, PIN_PMS5003_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_set_pull_mode((gpio_num_t)PIN_PMS5003_RX, GPIO_PULLUP_ONLY); // Prevent floating noise when sensor disconnected
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);

    /* Initialize u-blox NEO-M8N / Quectel GNSS on UART2 */
    if (gnss_nmea_init(&s_gnss, GPS_UART_NUM) != 0) {
        ESP_LOGE(TAG, "u-blox NEO-M8N GNSS initialization failed!");
    } else {
        ESP_LOGI(TAG, "u-blox NEO-M8N GNSS (NMEA-0183) initialized OK");
    }

    /* Configure UART2 for u-blox NEO-M8N GNSS (TX=GPIO17, RX=GPIO16, 9600 baud, 8N1) */
    uart_config_t gps_uart_cfg = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(GPS_UART_NUM, &gps_uart_cfg);
    uart_set_pin(GPS_UART_NUM, PIN_GPS_TX, PIN_GPS_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_set_pull_mode((gpio_num_t)PIN_GPS_RX, GPIO_PULLUP_ONLY); // Prevent floating noise when sensor disconnected
    uart_driver_install(GPS_UART_NUM, 1024, 0, 0, NULL, 0);

    /* Initialize MPU6050 6-Axis IMU & 3-Stage Fall Detection Engine */
    fall_detect_init(&s_fall_detector, NULL);
    if (mpu6050_init(&s_mpu6050, esp32_i2c_hal_get_handle(), MPU6050_I2C_ADDR) != 0) {
        ESP_LOGW(TAG, "MPU6050 IMU not responding at 0x%02X (check SDA=GPIO%d, SCL=GPIO%d, VCC=3.3V)",
                 MPU6050_I2C_ADDR, PIN_I2C_SDA, PIN_I2C_SCL);
    } else {
        ESP_LOGI(TAG, "MPU6050 6-Axis IMU initialized OK (fall detection active)");
    }

    /* Scan after device initialization so slow-starting sensors get the same
     * settled-bus opportunity as their driver probes. */
    esp32_i2c_hal_scan();

#if defined(CONFIG_SHRIKEFI_SOS_CANCEL_GPIO) && CONFIG_SHRIKEFI_SOS_CANCEL_GPIO >= 0
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_SHRIKEFI_SOS_CANCEL_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_cfg);
    ESP_LOGI(TAG, "SOS hardware cancel button configured on GPIO%d", CONFIG_SHRIKEFI_SOS_CANCEL_GPIO);
#endif

#if defined(PIN_BUZZER) && PIN_BUZZER >= 0
    gpio_config_t buzzer_cfg = {
        .pin_bit_mask = (1ULL << PIN_BUZZER),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&buzzer_cfg);
    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
    ESP_LOGI(TAG, "Active Buzzer configured on GPIO%d (audible alarm sounder)", PIN_BUZZER);
    /* Power-on self-test: 80ms audible beep to verify buzzer wiring and acoustic health */
    gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
    vTaskDelay(pdMS_TO_TICKS(80));
    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
#endif

    /* Spawn Dual-Core FreeRTOS Tasks.
     *
     * Stack sizes are not arbitrary. task_disaster_monitor alone puts roughly
     * 4 KB of structs on its stack:
     *
     *   hrv_state_t           1528 B (IBI window plus adjacency markers)
     *   risk_assessment_t  x4 2208 B   (552 B each after overall_advisory became
     *                                   an embedded char[512] instead of a
     *                                   const char* - the C-02 fix)
     *   clinical_assessment_t  284 B
     *   nn_output_t             12 B
     *
     * That is ~3.7 KB before any call frame, and its ESP_LOGI/printf lines with
     * a dozen arguments (four of them doubles) need several hundred more. At the
     * old 4096-byte stack it overflowed on every boot and corrupted the heap,
     * which surfaced as a StoreProhibited panic inside the I2C ISR rather than
     * as anything recognisable. Enlarging the buffers without enlarging the
     * stack is what broke it, so the sizes below carry real headroom.
     *
     * CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK is enabled in sdkconfig.defaults so
     * the next overflow of this kind panics immediately and names the task,
     * instead of corrupting memory somewhere unrelated. */
    xTaskCreatePinnedToCore(task_ppg_accelerator, "PPG_Accel",    7168, NULL, 5, NULL, 0); // Core 0
    xTaskCreatePinnedToCore(task_disaster_monitor, "Risk_Monitor", 10240, NULL, 2, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(task_pms5003_uart, "PMS5003_UART",     4096, NULL, 3, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(task_gps_uart,     "GPS_UART",         4096, NULL, 3, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(task_imu_monitor,  "IMU_Monitor",      4096, NULL, 3, NULL, 1); // Core 1

    ESP_LOGI(TAG, "Task Affinities Configured:");
    ESP_LOGI(TAG, " - PPG_Accel (Acquisition): Core 0");
    ESP_LOGI(TAG, " - Risk_Monitor (Processing): Core 1");
    ESP_LOGI(TAG, " - PMS5003_UART: Core 1");
    ESP_LOGI(TAG, " - GPS_UART: Core 1");
    ESP_LOGI(TAG, " - IMU_Monitor: Core 1");
#if defined(CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1) || (defined(CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE) && CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE == 1)
    ESP_LOGI(TAG, " - Wi-Fi Driver: Core 1");
#else
    ESP_LOGI(TAG, " - Wi-Fi Driver: Core 0 (WARNING: May jitter PPG timing)");
#endif
    ESP_LOGI(TAG, " - TCP/IP (lwIP): Core 1");
    ESP_LOGI(TAG, " - HTTPD Web Server: Core 1");
}
#endif

#ifndef ESP_PLATFORM
int main(void) {
    printf("================================================================\n");
    printf("  SIH26181 ShrikeFi (ESP32-S3 + Renesas ForgeFPGA) Host Test\n");
    printf("  Qualcomm Hardware Challenge — Smart India Hackathon 2026\n");
    printf("================================================================\n\n");

    // Initialize link driver (stub)
    shrikefi_link_init();
    shrikefi_set_threshold(120);

    // Prepare simulated inputs
    float hr = 135.0f;
    float rmssd = 10.0f;
    float spo2 = 97.0f;
    env_sensors_t env = {
        .ambient_temp_c = 46.0f,
        .humidity_pct   = 65.0f,
        .pm25           = 25.0f,
        .skin_temp_c    = 0.0f  /* reserved for a future TMP117 */
    };

    hrv_state_t hrv;
    hrv_init(&hrv);
    hrv.rmssd = rmssd;
    hrv.sdnn = 12.0f;
    hrv.count = 50;

    risk_assessment_t rule_risk;
    disaster_assess(&hrv, spo2, hr, &env, &rule_risk);

    // Single-pass TinyML INT8 model inference populates both risk categories and raw telemetry
    nn_output_t out;
    risk_assessment_t nn_risk;
    disaster_assess_nn_int8(&hrv, spo2, hr, &env, &nn_risk, &out);

    risk_assessment_t final_risk = rule_risk;
    if (nn_risk.overall_risk > final_risk.overall_risk) {
        final_risk.overall_risk = nn_risk.overall_risk;
        snprintf(final_risk.overall_advisory, sizeof(final_risk.overall_advisory), "%s",
                 nn_risk.overall_advisory);
    }

    printf("Host Test - Heat Wave Profile:\n");
    printf("  [TinyML INT8] Heat Score: %.3f (%s)\n", out.heat_score, risk_level_to_string(nn_risk.heat_risk));
    printf("  [TinyML INT8] Poll Score: %.3f (%s)\n", out.pollution_score, risk_level_to_string(nn_risk.pollution_risk));
    printf("  [TinyML INT8] Flood Score:%.3f (%s)\n", out.flood_score, risk_level_to_string(nn_risk.flood_risk));
    printf("  [TinyML INT8] AI Overall: %s\n", risk_level_to_string(nn_risk.overall_risk));
    printf("  [Rule Engine] Heat Risk:  %s\n", risk_level_to_string(rule_risk.heat_risk));
    printf("  [Rule Engine] Rule Risk:  %s\n", risk_level_to_string(rule_risk.overall_risk));
    printf("  [Unified Triage] Final Condition: %s\n", risk_level_to_string(final_risk.overall_risk));
    printf("  [Unified Triage] Action Advisory: %s\n\n", final_risk.overall_advisory);

    /* Profile 2: High-Humidity Storm / Fog (Testing Si et al. 2019 Neural Calibration) */
    printf("Host Test - High Humidity / Flood Fog Profile (Paper Neural Calibration):\n");
    float raw_fog_pm = 85.0f;
    float fog_temp   = 20.0f;
    float fog_hum    = 85.0f;
    float cal_fog_pm = pm25_calibrate_nn_int8(raw_fog_pm, fog_temp, fog_hum);
    bool  is_distorted = pm25_is_humidity_distorted(raw_fog_pm, fog_hum);

    printf("  [Sensor Input] Raw PM2.5: %.1f ug/m3 | Temp: %.1f C | Humidity: %.1f%%\n", raw_fog_pm, fog_temp, fog_hum);
    printf("  [Neural Net]   Calibrated PM2.5: %.1f ug/m3 (Humidity bias suppressed: %s)\n",
           cal_fog_pm, is_distorted ? "YES (-60% error correction)" : "NO");

    env_sensors_t fog_env = {
        .ambient_temp_c = fog_temp,
        .humidity_pct   = fog_hum,
        .pm25           = cal_fog_pm, /* Use neural-calibrated PM2.5 */
        .skin_temp_c    = 0.0f  /* reserved for a future TMP117 */
    };
    risk_assessment_t fog_risk;
    disaster_assess(&hrv, 98.0f, 75.0f, &fog_env, &fog_risk);
    /* Profile 3: Clinical ICU Emergency (MIMIC-III Sepsis & Severe Hypoxia in Normal Room) */
    printf("\nHost Test - Clinical ICU Emergency Profile (MIMIC-III Benchmark):\n");
    float icu_hr    = 142.0f; /* Tachycardia */
    float icu_spo2  = 82.0f;  /* Severe Hypoxia */
    float icu_rmssd = 9.0f;   /* Autonomic Shock Collapse */
    env_sensors_t normal_room = {
        .ambient_temp_c = 22.0f,
        .humidity_pct   = 45.0f,
        .pm25           = 12.0f,
        .skin_temp_c    = 0.0f  /* reserved for a future TMP117 */
    };
    risk_assessment_t env_baseline;
    disaster_assess(&hrv, icu_spo2, icu_hr, &normal_room, &env_baseline);

    clinical_assessment_t clin_icu;
    clinical_vitals_assess(icu_hr, icu_spo2, icu_rmssd, &clin_icu);

    risk_assessment_t fused_icu;
    clinical_fuse_triage(&clin_icu, &env_baseline, &fused_icu);

    printf("  [Sensor Input] HR: %.0f BPM | SpO2: %.0f%% | RMSSD: %.1f ms | Room Temp: %.1f C\n",
           icu_hr, icu_spo2, icu_rmssd, normal_room.ambient_temp_c);
    printf("  [Disaster Engine Alone] Risk: %s (Misses clinical crisis because room air is clean!)\n",
           risk_level_to_string(env_baseline.overall_risk));
    printf("  [NEWS2 Clinical Engine] Risk: %s (Flags: 0x%02X | NEWS2 Score: %u)\n",
           risk_level_to_string((risk_level_t)clin_icu.level), clin_icu.alert_flags, clin_icu.news2_score);
    printf("  [Unified Clinical Triage] Final Condition: %s\n", risk_level_to_string(fused_icu.overall_risk));
    printf("  [Unified Action Advisory] %s\n", fused_icu.overall_advisory);

    /* Profile 4: Motion Artifact & Noise Rejection (Elgendi 2016 SQI Benchmark) */
    printf("\nHost Test - Motion Artifact Rejection Profile (Elgendi 2016 SQI Benchmark):\n");
    // Clean resting pulse simulation: 32 samples with strong pulsatile waveform
    uint32_t clean_ppg[32];
    for (int i = 0; i < 32; i++) {
        clean_ppg[i] = 120000 + (uint32_t)(2500.0f * sinf((float)i * 0.4f) + 1200.0f * sinf((float)i * 0.8f));
    }
    float clean_ibis[8] = { 800.0f, 810.0f, 795.0f, 805.0f, 800.0f, 815.0f, 790.0f, 805.0f };
    ppg_sqi_result_t clean_sqi;
    ppg_calculate_sqi(clean_ppg, 32, clean_ibis, 8, &clean_sqi);
    printf("  [Clean Pulse]  SQI: %.2f | Perfusion: %.2f%% | Regularity: %.2f | Motion Noise: %s\n",
           clean_sqi.overall_sqi, clean_sqi.perfusion_index, clean_sqi.interval_regularity,
           clean_sqi.is_motion_artifact ? "YES" : "NO (Hospital Grade)");

    // Motion artifact simulation: sensor displaced during running/evacuation
    uint32_t noisy_ppg[32];
    for (int i = 0; i < 32; i++) {
        noisy_ppg[i] = 110000 + (uint32_t)((rand() % 15000));
    }
    float noisy_ibis[8] = { 450.0f, 980.0f, 320.0f, 1100.0f, 410.0f, 890.0f, 350.0f, 1020.0f };
    ppg_sqi_result_t noisy_sqi;
    ppg_calculate_sqi(noisy_ppg, 32, noisy_ibis, 8, &noisy_sqi);
    printf("  [Motion Noise] SQI: %.2f | Perfusion: %.2f%% | Regularity: %.2f | Motion Noise: %s\n",
           noisy_sqi.overall_sqi, noisy_sqi.perfusion_index, noisy_sqi.interval_regularity,
           noisy_sqi.is_motion_artifact ? "YES (Artifact Detected)" : "NO");

    // Test SQI Gating in clinical triage
    // Case A: Non-crisis vitals with noise (HR=95, SpO2=96) -> correctly held / suppressed
    clinical_assessment_t gated_clin;
    clinical_vitals_assess_full(95.0f, 96.0f, 25.0f, 16.0f, noisy_sqi.overall_sqi, &gated_clin);
    printf("  [SQI Gating]   Non-Crisis Noise Suppressed: %s | Status: %s\n",
           (gated_clin.alert_flags & ALERT_SIGNAL_NOISE) ? "YES" : "NO", gated_clin.advisory);

    // Case B: Genuine life-threatening emergency with motion (HR=165, SpO2=82) -> Crisis override fires!
    clinical_assessment_t crisis_clin;
    clinical_vitals_assess_full(165.0f, 82.0f, 8.0f, 32.0f, noisy_sqi.overall_sqi, &crisis_clin);
    printf("  [Crisis Override] Alarm Active Despite Noise: %s | Level: %s\n",
           (crisis_clin.level == CLINICAL_CRITICAL) ? "YES (EMERGENCY ALARM FIRED)" : "NO",
           risk_level_to_string((risk_level_t)crisis_clin.level));
    printf("  [Emergency Text]  %s\n", crisis_clin.advisory);

    /* Profile 5: Cardiopulmonary Respiratory Distress (Charlton 2018 PPG-RR & mNEWS2) */
    printf("\nHost Test - Cardiopulmonary Distress Profile (Charlton 2018 PPG-RR & mNEWS2):\n");
    // Tachypneic IBI sequence (breathing 28 breaths/min at HR=120 bpm -> 1 breath cycle every ~4.3 beats)
    float tachy_ibis[24];
    for (int i = 0; i < 24; i++) {
        tachy_ibis[i] = 500.0f + 40.0f * sinf((float)i * (2.0f * 3.14159f / 4.3f));
    }
    ppg_respiratory_result_t rr_result;
    ppg_estimate_respiratory_rate(tachy_ibis, NULL, 24, &rr_result);
    printf("  [PPG Extraction] Derived Breathing Rate: %.1f Breaths/min (Confidence: %.2f | RSA Depth: %.1f ms)\n",
           rr_result.respiratory_rate_bpm, rr_result.confidence, rr_result.rsa_depth_ms);

    clinical_assessment_t pneumonia_triage;
    clinical_vitals_assess_full(120.0f, 88.0f, 15.0f, rr_result.respiratory_rate_bpm, 0.92f, &pneumonia_triage);
    printf("  [mNEWS2 Engine]  Score: %u (HR=120: +2, SpO2=88%%: +3, RR=%.0f: +3) | Risk: %s\n",
           pneumonia_triage.news2_score, rr_result.respiratory_rate_bpm,
           risk_level_to_string((risk_level_t)pneumonia_triage.level));
    printf("  [Action Alert]   %s\n", pneumonia_triage.advisory);

    /* Profile 6: Peer-Reviewed Clinical Biomarkers (Moran 1998 PSI & AHA 2010 PM2.5-HRV) */
    printf("\nHost Test - Peer-Reviewed Clinical Biomarkers (Moran 1998 PSI & AHA 2010):\n");
    float moran_psi = disaster_calculate_moran_psi(145.0f, 44.0f, 60.0f);
    printf("  [Moran PSI] Heat Strain Index: %.2f / 10.0 (T_ambient: 44C, HR: 145 bpm) -> %s\n",
           moran_psi, (moran_psi >= 7.0f) ? "HIGH HEAT EXHAUSTION STRAIN (Compulsory Cooling)" : "MODERATE");

    float aha_strain = disaster_calculate_aha_autonomic_strain(125.0f, 14.0f);
    printf("  [AHA Statement] PM2.5-HRV Autonomic Strain: %.2f / 1.0 (PM2.5: 125 ug/m3, RMSSD: 14 ms) -> %s\n",
           aha_strain, (aha_strain >= 0.70f) ? "SEVERE AUTONOMIC DEPRESSION ALERT" : "NORMAL");

    /* Profile 7: SOS Cancel via Button */
    printf("\nHost Test - SOS Cancel via Button:\n");
    sos_init(NULL);
    sos_manual_trigger(1000);
    printf("  [SOS Button] Triggered manual emergency. State: %s\n", sos_state_name(sos_get_state()));
    sos_cancel_press(2000);
    sos_update(2000 + 2500, false, true);
    printf("  [SOS Button] Held for 2.5s (Progress: %u%%). State: %s\n", sos_cancel_progress_pct(4500), sos_state_name(sos_get_state()));
    sos_update(2000 + 5000, false, true);
    printf("  [SOS Button] Held for 5.0s (Progress: %u%%). State: %s\n", sos_cancel_progress_pct(7000), sos_state_name(sos_get_state()));

    /* Profile 8: Requirement 7 — 24-Hour Rolling Health History and Trends */
    printf("\nHost Test - Requirement 7: 24-Hour Rolling Health History & Trends:\n");
    web_status_history_init();
    valor_status_t sim_stat = {
        .hr = 74.0f,
        .spo2 = 98.0f,
        .rmssd = 45.0f,
        .pm25 = 18.0f,
        .news2 = 1,
        .contact = true,
        .uptime_s = 100
    };
    char missing_env_json[768];
    sim_stat.temp = -100.0f;
    sim_stat.hum = -1.0f;
    if (web_status_json(&sim_stat, missing_env_json, sizeof(missing_env_json)) == 0 ||
        strstr(missing_env_json, "\"temp\":null") == NULL ||
        strstr(missing_env_json, "\"hum\":null") == NULL) {
        fprintf(stderr, "[FAIL] Missing BME280 readings must serialize as null\n");
        return 1;
    }
    sim_stat.temp = 24.5f;
    sim_stat.hum = 48.0f;
    web_status_publish(&sim_stat);

    /* Simulate 1 hour later with moderate hypoxia & smoke spike */
    sim_stat.hr = 105.0f;
    sim_stat.spo2 = 91.0f;
    sim_stat.rmssd = 22.0f;
    sim_stat.pm25 = 85.0f;
    sim_stat.news2 = 4;
    sim_stat.uptime_s = 3705;
    web_status_publish(&sim_stat);

    char hist_json[2048];
    size_t hist_len = web_status_history_json(hist_json, sizeof(hist_json));
    printf("  [History JSON] Serialized %zu bytes across 24 hourly slots\n", hist_len);
    printf("  [Sample Trend] %.*s ...]\n", 130, hist_json);

    /* Profile 9: Sustained Tachycardia Acceptance & IBI Pipeline Verification */
    printf("\nHost Test - Profile 9: Sustained Tachycardia Acceptance & IBI Pipeline:\n");
    ibi_pipeline_t tachy_pipe;
    ibi_pipeline_reset(&tachy_pipe);
    hrv_state_t tachy_hrv;
    hrv_init(&tachy_hrv);

    /* Sequence of 350 ms intervals (171.4 BPM) */
    float tachy_ibi = 350.0f;
    int tachy_accepted = 0;
    for (int i = 0; i < 12; i++) {
        bool ok = ibi_pipeline_submit(&tachy_pipe, &tachy_hrv, IBI_SRC_FPGA, tachy_ibi, 0.0f);
        if (ok) {
            tachy_accepted++;
            float inst_hr = 60000.0f / tachy_ibi;
            g_state.r_peak_interval_ms = tachy_ibi;
            g_state.heart_rate = (g_state.heart_rate > 30.0f) ?
                                 (0.70f * g_state.heart_rate + 0.30f * inst_hr) : inst_hr;
        }
    }
    printf("  [Tachycardia Stimulus] 12 beats @ %.0f ms (%.1f BPM) -> Accepted: %d (after %d priming beats)\n",
           tachy_ibi, 60000.0f / tachy_ibi, tachy_accepted, IBI_PRIME_N);
    printf("  [Pipeline State]       g_state.heart_rate = %.1f BPM (Ceiling unblocked: %s)\n",
           g_state.heart_rate, (g_state.heart_rate > 150.0f) ? "YES" : "NO");

    /* Clinical assessment for this tachycardia */
    clinical_assessment_t tachy_triage;
    clinical_vitals_assess(g_state.heart_rate, 96.0f, 15.0f, &tachy_triage);
    printf("  [Clinical Assessment]  Risk Level: %s | Flags: 0x%02X (%s) | NEWS2 Score: %u\n",
           risk_level_to_string((risk_level_t)tachy_triage.level),
           tachy_triage.alert_flags,
           (tachy_triage.alert_flags & ALERT_TACHYCARDIA) ? "TACHYCARDIA" : "NONE",
           tachy_triage.news2_score);

    /* Also verify zero-SQI warmup holding in clinical triage */
    clinical_assessment_t warmup_triage;
    clinical_vitals_assess_full(165.0f, 82.0f, 8.0f, 32.0f, 0.0f, &warmup_triage);
    printf("  [Sensor Warmup Check]  SQI=0.0 Holds Crisis Triage: %s | Status: %s\n",
           (warmup_triage.alert_flags & ALERT_SIGNAL_NOISE) ? "YES" : "NO", warmup_triage.advisory);

    if (tachy_accepted == 0 || g_state.heart_rate <= 150.0f) {
        printf("  [FAIL] Tachycardia failed acceptance or HR did not exceed 150 BPM!\n");
        return 1;
    }
    if ((warmup_triage.alert_flags & ALERT_SIGNAL_NOISE) == 0) {
        printf("  [FAIL] Zero-SQI failed to hold triage!\n");
        return 1;
    }
    printf("  [PASS] Profile 9 Tachycardia Acceptance & Zero-SQI Warmup Succeeded!\n");

    /* Regression: reject both halves of the split-peak pair observed live on
     * hardware (669 ms + 1020 ms around an 810 ms rhythm) so neither can inflate
     * RMSSD. */
    ibi_pipeline_reset(&tachy_pipe);
    hrv_init(&tachy_hrv);
    for (int i = 0; i < IBI_PRIME_N; i++) {
        (void)ibi_pipeline_submit(&tachy_pipe, &tachy_hrv, IBI_SRC_FPGA, 810.0f, 0.0f);
    }
    for (int i = 0; i < 10; i++) {
        if (!ibi_pipeline_submit(&tachy_pipe, &tachy_hrv, IBI_SRC_FPGA, 810.0f, 0.0f)) {
            printf("  [FAIL] Stable IBI setup rejected a valid interval!\n");
            return 1;
        }
    }
    float stable_rmssd = tachy_hrv.rmssd;
    int stable_count = tachy_hrv.count;
    bool split_short_accepted = ibi_pipeline_submit(&tachy_pipe, &tachy_hrv,
                                                     IBI_SRC_FPGA, 669.0f, 0.0f);
    bool split_long_accepted = ibi_pipeline_submit(&tachy_pipe, &tachy_hrv,
                                                    IBI_SRC_FPGA, 1020.0f, 0.0f);
    if (split_short_accepted || split_long_accepted ||
        tachy_hrv.count != stable_count || fabsf(tachy_hrv.rmssd - stable_rmssd) > 0.1f) {
        printf("  [FAIL] Split-peak pair entered the HRV window!\n");
        return 1;
    }
    printf("  [PASS] 669/1020 ms split pair rejected; RMSSD stayed %.1f ms\n",
           tachy_hrv.rmssd);
    if (ppg_has_optical_contact(4400u, 5400u) ||
        !ppg_has_optical_contact(50001u, 0u) ||
        !ppg_has_optical_contact(0u, 10001u)) {
        printf("  [FAIL] Optical contact threshold admitted ambient data or rejected tissue data!\n");
        return 1;
    }
    printf("  [PASS] Optical contact gate rejects IR=4400/Red=5400 and accepts tissue-level counts\n");

    /* Profile 10: MPU6050 6-Axis IMU, 3-Stage Fall Detection & Motion Artifact Rejection */
    printf("\nHost Test - Profile 10: MPU6050 6-Axis IMU & 3-Stage Fall Detection:\n");
    mpu6050_t test_imu;
    mpu6050_init(&test_imu, NULL, MPU6050_I2C_ADDR);
    mpu6050_data_t imu_reading;

    // Test 1: Normal resting baseline (1.0g on Z-axis, low gyro)
    mpu6050_set_mock_raw(0, 0, 4096, 0, 0, 0, 0); // 4096 LSB = 1.0g in ±8g mode
    mpu6050_read(&test_imu, &imu_reading);
    printf("  [IMU Resting]   Ax=%.2fg Ay=%.2fg Az=%.2fg (A_mag=%.2fg) Gyro=%.1fdps\n",
           imu_reading.ax, imu_reading.ay, imu_reading.az, imu_reading.a_mag, imu_reading.g_mag);
    if (fabsf(imu_reading.a_mag - 1.0f) > 0.05f) {
        printf("  [FAIL] Expected 1.0g resting acceleration!\n");
        return 1;
    }

    fall_detector_t test_fall;
    fall_detect_init(&test_fall, NULL);
    uint32_t t_sim = 1000;
    fall_detect_update(&test_fall, t_sim, imu_reading.ax, imu_reading.ay, imu_reading.az, 0, 0, 0);
    printf("  [Fall FSM Base] State: %s | Motion Energy: %.3fg\n",
           fall_state_name(fall_detect_get_state(&test_fall)), fall_detect_get_motion_energy(&test_fall));
    if (fall_detect_get_state(&test_fall) != FALL_STATE_MONITORING) {
        printf("  [FAIL] Expected MONITORING state!\n");
        return 1;
    }

    // Test 2: Motion Artifact Rejection on PPG SQI
    // Simulate high running/evacuation arm swing (motion_energy = 0.40g)
    ppg_sqi_result_t motion_sqi;
    ppg_calculate_sqi_with_motion(clean_ppg, 32, clean_ibis, 8, 0.40f, &motion_sqi);
    printf("  [SQI Gating]    Motion Energy 0.40g: Clean Optical SQI 0.88 -> Motion-Gated SQI: %.2f (Artifact: %s)\n",
           motion_sqi.overall_sqi, motion_sqi.is_motion_artifact ? "YES (Gated)" : "NO");
    if (!motion_sqi.is_motion_artifact || motion_sqi.overall_sqi >= PPG_SQI_THRESHOLD_VALID) {
        printf("  [FAIL] High motion energy failed to gate SQI!\n");
        return 1;
    }

    // Test 3: False Alarm Rejection - High impact jumping/sitting with immediate recovery/walking
    printf("  [False Alarm]   Simulating jump / sit-down (Impact followed by immediate walking movement)...\n");
    fall_detect_reset(&test_fall);
    // Free-fall dip: 0.3g for 100ms
    t_sim += 100;
    fall_detect_update(&test_fall, t_sim, 0.1f, 0.1f, 0.25f, 50.0f, 20.0f, 10.0f);
    if (fall_detect_get_state(&test_fall) != FALL_STATE_FREE_FALL) {
        printf("  [FAIL] Expected FREE_FALL state!\n");
        return 1;
    }
    // Impact spike: 3.5g
    t_sim += 100;
    fall_detect_update(&test_fall, t_sim, 1.5f, 2.0f, 2.5f, 150.0f, 80.0f, 40.0f);
    if (fall_detect_get_state(&test_fall) != FALL_STATE_INACTIVITY) {
        printf("  [FAIL] Expected INACTIVITY state after impact!\n");
        return 1;
    }
    // User gets up and walks: dynamic accel 0.8g, high gyro 140 dps
    t_sim += 500;
    fall_detect_update(&test_fall, t_sim, 0.8f, 0.6f, 1.5f, 140.0f, 90.0f, 60.0f);
    printf("  [False Alarm]   User moved vigorously -> State recovered to: %s\n",
           fall_state_name(fall_detect_get_state(&test_fall)));
    if (fall_detect_get_state(&test_fall) != FALL_STATE_MONITORING) {
        printf("  [FAIL] Expected return to MONITORING after vigorous movement!\n");
        return 1;
    }

    // Test 4: Genuine 3-Stage Human Fall with Post-Impact Incapacitation
    printf("  [Fall Sequence] Simulating genuine human fall (Free-fall -> Impact -> 1.5s Immobility)...\n");
    fall_detect_reset(&test_fall);
    // Stage 1: Free fall weightlessness (0.33g)
    t_sim += 100;
    fall_detect_update(&test_fall, t_sim, 0.1f, 0.1f, 0.3f, 30.0f, 40.0f, 20.0f);
    printf("    Stage 1: A_mag=0.33g -> State: %s\n", fall_state_name(fall_detect_get_state(&test_fall)));
    if (fall_detect_get_state(&test_fall) != FALL_STATE_FREE_FALL) {
        printf("  [FAIL] Expected FREE_FALL state in fall sequence!\n");
        return 1;
    }

    // Stage 2: Violent Impact deceleration spike (3.8g, 220 dps)
    t_sim += 150;
    fall_detect_update(&test_fall, t_sim, 1.8f, 2.2f, 2.5f, 180.0f, 110.0f, 70.0f);
    printf("    Stage 2: A_mag=3.80g -> State: %s\n", fall_state_name(fall_detect_get_state(&test_fall)));
    if (fall_detect_get_state(&test_fall) != FALL_STATE_INACTIVITY) {
        printf("  [FAIL] Expected INACTIVITY state in fall sequence!\n");
        return 1;
    }

    // Stage 3: Settling and post-impact stillness (1.5 seconds of immobility on the floor)
    t_sim += 450; // past 400ms rebound settling
    bool fall_latched = false;
    for (int step = 0; step < 16; step++) { // 16 * 100ms = 1.6s
        t_sim += 100;
        // Subject lying horizontal: Ax=0.98g, Ay=0.05g, Az=0.08g (A_mag~0.98g), Gyro~0 dps
        bool res = fall_detect_update(&test_fall, t_sim, 0.98f, 0.05f, 0.08f, 1.0f, 2.0f, 1.0f);
        if (res) fall_latched = true;
    }
    printf("    Stage 3: 1.5s Stillness elapsed -> State: %s (Confirmed Flag: %d)\n",
           fall_state_name(fall_detect_get_state(&test_fall)), (int)fall_latched);
    if (!fall_latched || fall_detect_get_state(&test_fall) != FALL_STATE_CONFIRMED) {
        printf("  [FAIL] Expected fall to be confirmed after 1.5s stillness!\n");
        return 1;
    }
    float posture_angle = fall_detect_get_posture_angle_deg(&test_fall);
    printf("    Stage 3: Posture Angle Change: %.1f deg | Peak Impact: %.2fg / %.1fdps\n",
           posture_angle, fall_detect_get_impact_accel(&test_fall), fall_detect_get_impact_gyro(&test_fall));
    if (posture_angle < 60.0f) {
        printf("  [FAIL] Expected posture angle change > 60 deg (got %.1f deg)!\n", posture_angle);
        return 1;
    }
    if (!fall_detect_is_confirmed(&test_fall)) {
        printf("  [FAIL] Expected fall_detect_is_confirmed to be true!\n");
        return 1;
    }
    if (fall_detect_get_impact_accel(&test_fall) < 3.5f) {
        printf("  [FAIL] Expected peak impact accel >= 3.5g!\n");
        return 1;
    }

    // Test 5: SOS State Machine Fall Arming and Triggering
    sos_init(NULL);
    sos_arm_fall(t_sim);
    printf("  [SOS Assist]    sos_arm_fall() -> State: %s | Pending Trigger: %s\n",
           sos_state_name(sos_get_state()), sos_trigger_name(sos_get_trigger()));
    if (sos_get_state() != SOS_ARMED || sos_get_trigger() != SOS_TRIGGER_FALL) {
        printf("  [FAIL] Expected SOS_ARMED state with trigger 'fall' after sos_arm_fall!\n");
        return 1;
    }

    // Countdown / grace period (10 seconds)
    sos_update(t_sim + 5000, false, true); // 5s < 10s confirm
    if (sos_get_state() != SOS_ARMED) {
        printf("  [FAIL] Expected SOS_ARMED to persist during 10s grace period!\n");
        return 1;
    }
    sos_update(t_sim + 10001, false, true); // 10s elapsed
    printf("  [SOS Assist]    10s Grace Expired -> State: %s | Latch Trigger: %s\n",
           sos_state_name(sos_get_state()), sos_trigger_name(sos_get_trigger()));
    if (sos_get_state() != SOS_ACTIVE || sos_get_trigger() != SOS_TRIGGER_FALL) {
        printf("  [FAIL] Expected SOS_ACTIVE with trigger 'fall'!\n");
        return 1;
    }
    if (strcmp(sos_trigger_name(sos_get_trigger()), "fall") != 0) {
        printf("  [FAIL] Expected trigger name 'fall', got '%s'\n", sos_trigger_name(sos_get_trigger()));
        return 1;
    }

    // Test 6: Cancel during emergency
    sos_cancel_now(t_sim + 11000);
    if (sos_get_state() != SOS_CANCELLED) {
        printf("  [FAIL] Expected SOS_CANCELLED after sos_cancel_now!\n");
        return 1;
    }
    sos_update(t_sim + 12000, false, true);
    if (sos_get_state() != SOS_IDLE) {
        printf("  [FAIL] Expected SOS_IDLE after cancel cleared!\n");
        return 1;
    }

    // Test 7: Button Hold-Cancel during SOS_ARMED Grace Window
    sos_arm_fall(t_sim + 12500);
    if (sos_get_state() != SOS_ARMED || sos_get_trigger() != SOS_TRIGGER_FALL) {
        printf("  [FAIL] Expected SOS_ARMED with trigger 'fall' on re-arming!\n");
        return 1;
    }
    // Patient presses and holds cancel button for 5 seconds during the 10s grace window
    sos_cancel_press(t_sim + 13000);
    sos_update(t_sim + 15500, false, true); // 2.5s held (50%)
    if (sos_get_state() != SOS_ARMED || sos_cancel_progress_pct(t_sim + 15500) != 50) {
        printf("  [FAIL] Expected SOS_ARMED with 50%% cancel progress during grace hold!\n");
        return 1;
    }
    sos_update(t_sim + 18000, false, true); // 5.0s held -> stands down
    if (sos_get_state() != SOS_CANCELLED) {
        printf("  [FAIL] Expected SOS_CANCELLED after 5s cancel hold during grace window!\n");
        return 1;
    }
    sos_update(t_sim + 19000, false, true);
    if (sos_get_state() != SOS_IDLE) {
        printf("  [FAIL] Expected SOS_IDLE after cancel cleared!\n");
        return 1;
    }

    // Test 8: Direct Fall Trigger (immediate latch without countdown)
    sos_fall_trigger(t_sim + 20000);
    if (sos_get_state() != SOS_ACTIVE || sos_get_trigger() != SOS_TRIGGER_FALL) {
        printf("  [FAIL] Expected immediate SOS_ACTIVE on sos_fall_trigger!\n");
        return 1;
    }
    printf("  [PASS] Profile 10 MPU6050 & 3-Stage Fall Detection Succeeded!\n");

    /* Profile 11: GNSS NMEA Telemetry & u-blox NEO-M8N Hardware Integration */
    printf("\nHost Test - Profile 11: GNSS NMEA & u-blox NEO-M8N Hardware Integration:\n");
    gnss_nmea_t sim_gnss;
    if (gnss_nmea_init(&sim_gnss, GPS_UART_NUM) != 0) {
        printf("  [FAIL] Failed to initialize GNSS driver!\n");
        return 1;
    }

    /* Test 1: Cold start state */
    if (gnss_nmea_has_fix(&sim_gnss)) {
        printf("  [FAIL] Expected no fix on fresh initialization!\n");
        return 1;
    }
    char cold_coords[GNSS_COORD_STR_LEN];
    gnss_nmea_format_coords(&sim_gnss.data, cold_coords, sizeof(cold_coords));
    if (strcmp(cold_coords, "NO FIX") != 0) {
        printf("  [FAIL] Expected 'NO FIX' before satellite acquisition!\n");
        return 1;
    }

    /* Test 2: Stream live u-blox NEO-M8N burst stream byte-by-byte */
    const char *sim_burst =
        "$GNRMC,083559.00,A,2017.7635,N,08549.4723,E,0.05,120.50,121026,,,A*46\r\n"
        "$GNVTG,120.50,T,,M,0.05,N,0.09,K,A*29\r\n"
        "$GNGGA,083559.00,2017.7635,N,08549.4723,E,1,10,0.95,45.3,M,0.0,M,,*4E\r\n";

    int parsed_sentences = 0;
    for (size_t i = 0; i < strlen(sim_burst); i++) {
        if (gnss_nmea_feed_byte(&sim_gnss, (uint8_t)sim_burst[i]) == 1) {
            parsed_sentences++;
        }
    }
    if (parsed_sentences != 3 || !gnss_nmea_has_fix(&sim_gnss)) {
        printf("  [FAIL] Expected 3 parsed sentences and active fix! (parsed=%d, fix=%d)\n",
               parsed_sentences, (int)gnss_nmea_has_fix(&sim_gnss));
        return 1;
    }

    /* Test 3: Validate parsed coordinates and telemetry */
    gnss_data_t sim_data;
    if (gnss_nmea_get_data(&sim_gnss, &sim_data) != 0 || !sim_data.fix_valid) {
        printf("  [FAIL] Failed to retrieve valid GNSS data!\n");
        return 1;
    }
    char sim_coord_str[GNSS_COORD_STR_LEN];
    gnss_nmea_format_coords(&sim_data, sim_coord_str, sizeof(sim_coord_str));
    printf("  [GNSS Fix]     u-blox NEO-M8N Locked (%u Sats | Alt: %.1fm | HDOP: %.2f)\n",
           sim_data.satellites, sim_data.altitude_m, sim_data.hdop);
    printf("  [Coordinates]  %s (Lat: %.5f%c, Lon: %.5f%c, Speed: %.2f km/h)\n",
           sim_coord_str, (double)sim_data.latitude, sim_data.lat_dir,
           (double)sim_data.longitude, sim_data.lon_dir, (double)sim_data.speed_kmh);

    if (strstr(sim_coord_str, "20.29606N") == NULL || strstr(sim_coord_str, "85.82454E") == NULL) {
        printf("  [FAIL] Coordinate format mismatch! Got '%s'\n", sim_coord_str);
        return 1;
    }

    /* Test 4: OLED Location card fallback */
    char sim_oled_card[LOCATION_MAX_LEN];
    location_card_text(sim_coord_str, sim_oled_card, sizeof(sim_oled_card));
    printf("  [OLED Card]    %s\n", sim_oled_card);
    if (strstr(sim_oled_card, "20.29606N") == NULL) {
        printf("  [FAIL] Expected live GPS coordinates in OLED location card!\n");
        return 1;
    }

    /* Test 5: Command builders & u-blox binary protocol */
    int ubx_bytes = gnss_nmea_ublox_set_rate(&sim_gnss, 1000);
    int pubx_bytes = gnss_nmea_ublox_set_baud(&sim_gnss, 9600);
    printf("  [UBX/PUBX TX]  UBX-CFG-RATE: %d bytes | PUBX-CFG-PRT: %d bytes\n", ubx_bytes, pubx_bytes);
    if (ubx_bytes != 14 || pubx_bytes <= 0) {
        printf("  [FAIL] Expected valid UBX (14B) and PUBX transmit byte counts!\n");
        return 1;
    }

    /* Test 6: Satellite fix loss handling */
    const char *sim_fix_lost = "$GNGGA,083600.00,0000.0000,N,00000.0000,E,0,02,99.9,0.0,M,0.0,M,,*75\r\n";
    for (size_t i = 0; i < strlen(sim_fix_lost); i++) {
        gnss_nmea_feed_byte(&sim_gnss, (uint8_t)sim_fix_lost[i]);
    }
    if (gnss_nmea_has_fix(&sim_gnss)) {
        printf("  [FAIL] Expected fix to be cleared after Quality 0 GGA!\n");
        return 1;
    }
    gnss_nmea_format_coords(&sim_gnss.data, sim_coord_str, sizeof(sim_coord_str));
    printf("  [Fix Lost]     Simulated Signal Loss -> Status: %s\n", sim_coord_str);
    if (strcmp(sim_coord_str, "NO FIX") != 0) {
        printf("  [FAIL] Expected 'NO FIX' after signal loss!\n");
        return 1;
    }
    printf("  [PASS] Profile 11 GNSS NMEA & u-blox NEO-M8N Succeeded!\n");

    /* Profile 12 intentionally removed with the skin-temperature feature
     * (2026-10-17). Re-add a temperature-scoring profile when a TMP117 is fitted. */

    /* Profile 13: SSD1306 Dirty-Page Differential Update & MAX30102 Non-Blinking FIFO Recovery */
    printf("\nHost Test - Profile 13: SSD1306 Differential Update & MAX30102 Burst/Recovery:\n");

    /* Test 1: SSD1306 Differential Dirty-Page Updating */
    ssd1306_t test_oled;
    if (ssd1306_init(&test_oled, esp32_i2c_hal_get_handle(), SSD1306_I2C_ADDR) != 0) {
        printf("  [FAIL] Failed to initialize SSD1306 OLED in simulation!\n");
        return 1;
    }

    /* Subtest 1A: Idle refresh with identical framebuffer produces 0 I2C write transactions */
    esp32_i2c_hal_sim_reset_counters();
    int update_rc = ssd1306_update(&test_oled);
    uint32_t idle_writes = esp32_i2c_hal_sim_get_raw_writes();
    printf("  [OLED Idle Sync] Clean framebuffer -> %u I2C write transactions (100%% idle refresh flicker eliminated)\n",
           idle_writes);
    if (update_rc != 0 || idle_writes != 0) {
        printf("  [FAIL] Expected 0 write transactions on clean framebuffer, got %u!\n", idle_writes);
        return 1;
    }

    /* Subtest 1B: Single page modification transmits ONLY that modified page */
    ssd1306_draw_string(&test_oled, 0, 16, "VALOR OK"); /* Row 16 = Page 2 (rows 16-23) */
    esp32_i2c_hal_sim_reset_counters();
    update_rc = ssd1306_update(&test_oled);
    uint32_t page_writes = esp32_i2c_hal_sim_get_raw_writes();
    uint32_t bytes_written = esp32_i2c_hal_sim_get_raw_bytes();
    printf("  [OLED Page Diff] Modified Page 2 only -> Transmitted 1 page (%u writes, %u bytes vs full 1024B frame)\n",
           page_writes, bytes_written);
    if (update_rc != 0 || page_writes != 2 || bytes_written != 136) {
        printf("  [FAIL] Expected exactly 2 write transactions (1 page window + 1 data burst) and 136 bytes, got %u writes / %u bytes!\n",
               page_writes, bytes_written);
        return 1;
    }

    /* Subtest 1C: Second update with no further changes immediately returns 0 writes */
    esp32_i2c_hal_sim_reset_counters();
    update_rc = ssd1306_update(&test_oled);
    if (update_rc != 0 || esp32_i2c_hal_sim_get_raw_writes() != 0) {
        printf("  [FAIL] Expected 0 write transactions on subsequent unchanged frame!\n");
        return 1;
    }
    printf("  [OLED Steady State] Unmodified frame -> 0 transactions (bus stays clear for optical sampling)\n");

    /* Test 2: MAX30102 Burst FIFO Pointer Read (Single 3-byte transaction).
     * test_max used to be created by the skin-temperature profile that was
     * removed on 2026-10-17, so it is created here now. */
    max30102_t test_max;
    if (max30102_init(&test_max, esp32_i2c_hal_get_handle()) != 0) {
        printf("  [FAIL] Failed to initialize MAX30102 in simulation!\n");
        return 1;
    }
    int dropped = 0, saturated = 0;
    esp32_i2c_hal_sim_set_max30102_fifo_ptrs(12, 0, 4);
    int avail = max30102_fifo_available(&test_max, &dropped, &saturated);
    printf("  [FIFO Burst Read] Available samples: %d (WR=12, RD=4) via single 3-byte burst read\n", avail);
    if (avail != 8 || dropped != 0 || saturated != 0) {
        printf("  [FAIL] Expected 8 available samples, got %d!\n", avail);
        return 1;
    }

    /* Test 3: Non-Blinking FIFO Recovery when already in SpO2 mode */
    esp32_i2c_hal_sim_set_max30102_mode(MAX30102_MODE_SPO2); /* 0x03 */
    esp32_i2c_hal_sim_reset_counters();
    int rec_rc = max30102_recover_fifo(&test_max);
    uint32_t mode_writes = esp32_i2c_hal_sim_get_max30102_mode_writes();
    printf("  [PPG Recovery] Running in SpO2 mode -> FIFO cleared without writing MODE_CONFIG (%u mode writes, 0 LED blinks)\n",
           mode_writes);
    if (rec_rc != 0 || mode_writes != 0) {
        printf("  [FAIL] Expected 0 MODE_CONFIG writes during normal FIFO recovery, got %u!\n", mode_writes);
        return 1;
    }

    /* Test 4: Shutdown/brownout state fails closed without toggling LEDs.
     * Full reconfiguration is reserved for the persistent-failure init path. */
    esp32_i2c_hal_sim_set_max30102_mode(0x80); /* Shutdown after brownout */
    esp32_i2c_hal_sim_reset_counters();
    rec_rc = max30102_recover_fifo(&test_max);
    mode_writes = esp32_i2c_hal_sim_get_max30102_mode_writes();
    uint8_t retained_mode = esp32_i2c_hal_sim_get_max30102_mode();
    printf("  [PPG Brownout] Shutdown detected -> recovery deferred (mode 0x%02X, %u mode writes, no LED blink)\n",
           retained_mode, mode_writes);
    if (rec_rc == 0 || mode_writes != 0 || retained_mode != 0x80) {
        printf("  [FAIL] Brownout recovery unexpectedly rewrote MODE_CONFIG or reported success!\n");
        return 1;
    }
    printf("  [PASS] Profile 13 SSD1306 Differential Update & MAX30102 Recovery Succeeded!\n");

    printf("\n>>> ShrikeFi Host Test Completed Successfully <<<\n");
    return 0;
}
#endif
