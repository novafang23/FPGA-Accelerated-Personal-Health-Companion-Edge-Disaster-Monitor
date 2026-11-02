/*
 * skin_estimate.h
 * Dual-Sensor Skin Temperature Estimation (MAX30102 Die Temp + BME280 Ambient)
 *
 * Physical Model (Median-of-5 Peak-Holding Monotonic Thermal Conduction):
 *   T_target = b0 + b1 * delta_die + b2 * (T_amb - 27.5)
 *   Strictly non-decreasing first-order thermal convergence during warm contact.
 *
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#ifndef SKIN_ESTIMATE_H
#define SKIN_ESTIMATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Clinical / Sensor Range Constraints */
#define SKIN_EST_DEFAULT_B0          34.10f  /* Calibrated peripheral skin baseline at 27.5C amb */
#define SKIN_EST_DEFAULT_B1          0.55f   /* Gain on bounded contact delta (T_die_peak - T_base) */
#define SKIN_EST_DEFAULT_B2          0.12f   /* Ambient room correction gain */
#define SKIN_EST_DEFAULT_TAU_S       0.00f   /* Deprecated derivative lead (0 = disabled for monotonic stability) */

#define SKIN_EST_INITIAL_TOUCH_C    32.20f   /* Starting anchor when finger first makes optical contact */
#define SKIN_EST_MIN_REF_C          31.0f    /* Lower physiological skin display bound (<31C = COLD) */
#define SKIN_EST_MAX_REF_C          36.8f    /* Upper peripheral skin display bound */
#define SKIN_EST_EQUIL_MAX_C        36.35f   /* Hard upper equilibrium ceiling against LED self-heating */
#define SKIN_EST_MAX_RISE_PER_SEC   0.28f    /* Maximum upward slew rate per 1 Hz update (C/s) */
/* Downward tracking. Skin sheds heat more slowly than the model converges, so
 * the fall is deliberately gentler than the rise: an eager fall would let
 * ordinary sensor noise read as vasoconstriction. */
#define SKIN_EST_FALL_FRACTION      0.12f    /* Fraction of the remaining gap per tick */
#define SKIN_EST_MAX_FALL_PER_SEC   0.08f    /* Hard cap on the downward slew (C/s) */
#define SKIN_EST_VALID_MIN_MS       3000u    /* Publish monotonic estimate after 3s continuous contact */
#define SKIN_EST_SETTLE_MIN_MS      12000u   /* Minimum 12 seconds contact before thermal equilibrium lock */
#define SKIN_EST_MAX_SLOPE_C_PER_S  0.050f   /* Max residual slope for settled flag */
#define SKIN_EST_HIST_LEN           16       /* Rolling slope window */
#define SKIN_EST_MED_LEN            5        /* 5-sample median filter window for spike rejection */

/* ---- Peripheral thermal state ------------------------------------------
 * A fingertip is NOT a core thermometer. It reads 30-34 C while core sits at
 * 36.5-37.5 C, and that gap is set by vasoconstriction and room temperature,
 * not by sensor error. Converting skin -> core requires a paired reference
 * (tympanic/oesophageal) and ~100 paired readings across several sessions; we
 * have neither, so this code deliberately does not attempt it. A two-point
 * fit on one degenerate dataset would produce coefficients that look precise
 * and are meaningless.
 *
 * What IS valid at a peripheral site is the DIRECTION the periphery is being
 * driven: a finger that cools 1.5 C below its own settled baseline while HR
 * climbs is showing cold stress, and that statement needs no core reference.
 * So we report state + trend + rate, all relative to the subject's own
 * baseline, and never an absolute core estimate.
 */
#define SKIN_EST_PT_RING            30       /* 30 s at 1 Hz for the rate fit     */
#define SKIN_EST_PT_MIN_SAMPLES     12       /* Below this the rate is not trusted */
#define SKIN_EST_PT_RATE_MIN_C_MIN  0.15f    /* |dT/dt| below this = STABLE        */
/* Physical plausibility bound. The ESTIMATOR can slew at 0.35 C/s (21 C/min)
 * because it is chasing a moving target, but fingertip skin under a ~6 mm
 * probe cannot change faster than roughly 2 C/min. Anything above that is the
 * model catching up, not the subject, so it is clamped rather than published
 * as if it were a physiological rate. */
#define SKIN_EST_PT_RATE_MAX_C_MIN  2.00f
/* Trend thresholds are measured on the ESTIMATE, which attenuates the die
 * signal by b1=0.55. On hardware the finger-on die sits only ~1.0 C above the
 * idle baseline, so a physically possible drop can move the skin estimate by
 * about 0.5 C at most before the finger is colder than the room. A -1.00 C
 * threshold was therefore mathematically unreachable, and a real finger could
 * never be reported as COOLING. These are set to what the model can actually
 * produce while staying well clear of the noise floor. */
#define SKIN_EST_PT_COOLING_C      (-0.35f)  /* Trend at/below this = COOLING      */
#define SKIN_EST_PT_REWARMING_C     0.50f    /* Trend at/above this = REWARMING    */
#define SKIN_EST_PT_COLD_ABS_C      29.00f   /* Absolute floor: vasoconstricted    */

typedef enum {
    SKIN_THERM_UNKNOWN = 0,   /* no settled contact yet - say nothing          */
    SKIN_THERM_STABLE,        /* periphery steady: no thermal drive             */
    SKIN_THERM_COOLING,       /* falling trend: vasoconstriction / cold exposure*/
    SKIN_THERM_REWARMING,     /* rising trend: rewarming / recovery             */
    SKIN_THERM_VASOCONSTRICTED/* trend-agnostic absolute floor: finger shut down */
} skin_therm_state_t;

/* ---- Ambient common-mode rejection -------------------------------------
 * A single peripheral site cannot separate two causes of the same signal:
 *
 *   room cools 1 C  -> finger cools ~0.3-0.5 C   -> looks like COOLING
 *   subject constricts -> finger cools 0.3-0.5 C -> IS cold stress
 *
 * Both move the same number in the same direction. Reporting the raw trend
 * therefore produces false cold-stress alerts whenever the environment
 * drifts, which is exactly what happens in a field deployment.
 *
 * The ambient channel (BME280) observes the common-mode part: whatever the
 * room does, it does to the air too. Subtracting the ambient component leaves
 * the differential part, which is the only part that is a property of the
 * subject. This is not a correction factor pulled from thin air -- it is the
 * standard common-mode rejection idea, and it is bounded so a bad ambient
 * reading cannot manufacture a trend on its own.
 */
#define SKIN_EST_PT_COUPLING        0.35f  /* finger tracks ~35% of ambient drift */
#define SKIN_EST_PT_AMB_MAX_C       3.00f  /* Ignore ambient swing beyond this   */

/* ---- State hysteresis ---------------------------------------------------
 * Without a minimum dwell the published state chatters on a single noisy
 * sample, which makes the OLED and the web tile flicker between COOLING and
 * STABLE during a perfectly ordinary contact. A new state must persist for
 * this many consecutive 1 Hz evaluations before it is published. */
#define SKIN_EST_PT_DWELL           3u

/* LED self-heating, MEASURED not assumed.
 *
 * The MAX30102's emitters heat their own die. The baseline against which the
 * finger's warming is judged must therefore exclude that self-heating, or the
 * finger looks cooler than it is.
 *
 * This used to be the constant `amb + 5.5`. On hardware that guard sat HIGHER
 * than the die ever reached with a finger on it:
 *
 *     ambient 25.40, idle die 29.06  (self-heating +3.66, not +5.5)
 *     guard  = 25.40 + 5.5 = 30.90
 *     peak   = die with finger pressed = 30.07
 *     delta  = 30.07 - 30.90 = -0.83  -> clamped to 0.00
 *
 * so the die thermometer contributed exactly nothing and the published value
 * was the constant b0 plus an ambient term. Every unit test passed because
 * their fixtures sit +6 to +9 C above ambient, above the guard; the real board
 * sits at +3.7 and fell straight through the gap.
 *
 * The offset is now measured at boot (skin_estimate_measure_led_offset) as the
 * die-temperature step across enabling the emitters. When no measurement has
 * been taken, the estimator falls back to the die's own idle reading rather
 * than to a fabricated ambient offset. */
#define SKIN_EST_LED_OFFSET_DEFAULT_C  4.00f  /* Fallback ONLY, until measured    */
/* Lower bound is strictly positive. An offset of exactly 0.0 is what a FAILED
 * die read produces (both reads return 0), so allowing 0 here let a broken
 * measurement masquerade as "the LEDs heat the die by nothing". Real
 * self-heating on this board is ~3.7 C; anything under 0.5 C means the reads
 * did not both succeed. */
#define SKIN_EST_LED_OFFSET_MIN_C      0.50f
#define SKIN_EST_LED_OFFSET_MAX_C     12.00f  /* A larger step means the probe moved */

typedef struct {
    /* Calibration Coefficients */
    float b0;           /* Skin baseline at 27.5C amb (nominal 34.10C) */
    float b1;           /* Gain on contact delta (nominal 0.55) */
    float b2;           /* Ambient room correction gain (nominal 0.12) */
    float tau_lead_s;   /* Retained for struct compatibility (0.0f) */
    float cal_min_c;
    float cal_max_c;
    bool  is_calibrated;

    /* Live State */
    float last_raw_die_c;
    float last_filtered_die_c;
    float last_amb_c;
    float idle_die_c;               /* Pre-contact steady die baseline */
    float contact_baseline_die_c;   /* Effective baseline at moment finger touched */
    float contact_peak_die_c;       /* Peak median-filtered die temp during current contact */
    float estimated_skin_c;
    float slope_c_per_sec;

    /* ---- Peripheral thermal state (SKIN_THERM_*) -------------------------
     * A fingertip NEVER reads core temperature: it sits 3-7 C below core and
     * the gap is driven by vasoconstriction, not by sensor error. So instead
     * of pretending to convert to core, this tracks the thing that IS
     * measurable from a peripheral site -- the direction and rate the
     * periphery is being driven. That is what a cold-exposure decision needs
     * and it needs no core reference to be valid.
     *
     * baseline_c is frozen when contact is established, so trend_c is always
     * "how far has this finger drifted since it settled", never an absolute
     * claim about the body. */
    float   pt_baseline_c;          /* Settled skin temp at end of settle window */
    float   pt_trend_c;             /* last - baseline (negative = cooling)     */
    float   pt_rate_c_per_min;      /* least-squares slope over the ring        */
    float   pt_ring[SKIN_EST_PT_RING];   /* 1 Hz ring of settled samples        */
    uint32_t pt_ring_ms[SKIN_EST_PT_RING];
    uint8_t pt_ring_n;
    uint8_t pt_ring_head;
    skin_therm_state_t pt_state;
    bool    pt_valid;               /* ring deep enough for a trustworthy rate  */
    /* Hysteresis: candidate state + how many consecutive ticks it has held.
     * pt_state only changes once a candidate survives SKIN_EST_PT_DWELL. */
    skin_therm_state_t pt_pending;
    uint8_t pt_pending_n;
    /* Common-mode rejection: how much of the trend the AMBIENT accounts for.
     * pt_trend_amb_c is subtracted from the raw trend to give pt_trend_c, the
     * differential part that is a property of the subject rather than the room. */
    float   pt_amb_baseline_c;
    float   pt_trend_amb_c;

    /* Measured LED self-heating offset (die step across enabling the emitters).
     * 0 with led_offset_valid == false means "not measured yet": the estimator
     * then uses the die's own idle reading as the baseline and flags the result
     * as weak-signal, rather than substituting a fabricated ambient offset. */
    float led_offset_c;
    bool  led_offset_valid;

    /* Diagnostics: how much of the published value the die thermometer
     * actually contributed this contact. die_delta is peak - baseline (C) and
     * is what b1 multiplies; when it is ~0 the reading is effectively the
     * constant b0 plus the ambient term, and the UI says so. */
    float die_delta_c;
    bool  weak_signal;

    /* 5-sample median filter state */
    float   med_buf[SKIN_EST_MED_LEN];
    uint8_t med_count;
    uint8_t med_idx;

    uint32_t contact_start_ms;
    uint32_t last_sample_ms;
    bool     contact_active;
    bool     is_settled;
    bool     is_valid;
    bool     is_below_range;

    /* Rolling History for Slope Calculation */
    float    die_history[SKIN_EST_HIST_LEN];
    uint32_t time_history_ms[SKIN_EST_HIST_LEN];
    uint8_t  hist_count;
    uint8_t  hist_idx;
} skin_estimate_t;

/**
 * @brief Initialize skin temperature estimator with default or stored coefficients.
 */
void skin_estimate_init(skin_estimate_t *st, float b0, float b1, float b2, bool is_calibrated);

/**
 * @brief Update estimator with 1 Hz die and ambient temperature samples.
 * @param st Pointer to estimator state
 * @param raw_die_c MAX30102 die temperature reading in Celsius
 * @param amb_c BME280 ambient temperature reading in Celsius
 * @param finger_present True if optical PPG contact is confirmed
 * @param now_ms System time in milliseconds
 */
void skin_estimate_update(skin_estimate_t *st, float raw_die_c, float amb_c,
                          bool finger_present, uint32_t now_ms);

/**
 * @brief Reset contact and settling state (e.g. when finger is removed).
 */
void skin_estimate_reset(skin_estimate_t *st);

/**
 * @brief Update calibration parameters dynamically.
 */
void skin_estimate_set_calibration(skin_estimate_t *st, float b0, float b1, float b2,
                                  float min_c, float max_c);

/**
 * @brief Record the MEASURED LED self-heating offset.
 *
 * Call once at boot after measuring the die temperature with the emitters off
 * and again with them running. The difference is how much the MAX30102 heats
 * itself, and it must be removed from the baseline or the finger's own warming
 * is understated. A value outside [MIN, MAX] is rejected as a bad measurement
 * (the probe almost certainly moved between the two reads).
 *
 * @param st       Estimator state
 * @param offset_c die_on - die_off, in Celsius
 * @return true if accepted
 */
bool skin_estimate_set_led_offset(skin_estimate_t *st, float offset_c);

/**
 * @brief Human-readable peripheral thermal state, for the UI and logs.
 *
 * Returns a stable static string ("UNKNOWN", "STABLE", "COOLING",
 * "REWARMING", "VASOCONSTRICTED"). Never returns NULL.
 */
const char *skin_therm_state_name(skin_therm_state_t s);

/**
 * @brief Recompute the peripheral thermal state from the current estimate.
 *
 * Called internally by skin_estimate_update() once the estimate is settled and
 * valid; exposed so tests and the status publisher can drive it directly.
 * Trend and rate are relative to the baseline frozen at contact settle, so the
 * result is valid on any subject without a core reference.
 *
 * @param st     Estimator state
 * @param now_ms System time in milliseconds
 */
void skin_estimate_update_thermal(skin_estimate_t *st, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* SKIN_ESTIMATE_H */
