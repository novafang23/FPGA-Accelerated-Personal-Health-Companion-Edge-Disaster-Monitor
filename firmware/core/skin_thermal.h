/*
 * skin_thermal.h - peripheral thermal state from a MEASURED skin temperature.
 *
 * WHAT CHANGED AND WHY
 * --------------------
 * This state machine used to live inside skin_estimate.c and was driven by the
 * MAX30102 die thermometer. That whole die-based skin-temperature ESTIMATE has
 * been removed: the die is a junction sensor heated by its own emitters, its
 * LED self-heating had to be measured and subtracted at boot, and the resulting
 * number was a model output wearing a measurement's clothes. With a real TMP117
 * contact probe on the board there is no reason to publish an estimate at all,
 * so the fallback is gone.
 *
 * What survives is the part that was genuinely useful and does NOT depend on the
 * die: tracking which way the periphery is being driven. It now takes a REAL
 * temperature reading, which is a strictly better input than the model output
 * it used to consume.
 *
 * WHAT THIS DOES NOT CLAIM
 * ------------------------
 * A fingertip is not a core site. It reads 30-34 C while core sits at
 * 36.5-37.5 C, and that gap is vasoconstriction, not a calibratable offset.
 * This reports DIRECTION relative to the subject's own settled baseline, never
 * an absolute core temperature, and it never feeds the NEWS2 temperature row.
 */
#ifndef SKIN_THERMAL_H
#define SKIN_THERMAL_H

#include <stdint.h>
#include <stdbool.h>

#define SKIN_THERM_RING_SIZE        30      /* 30 s at 1 Hz for the rate fit      */
#define SKIN_THERM_MIN_SAMPLES      12      /* below this the rate is not trusted */
#define SKIN_THERM_RATE_MIN_C_MIN   0.15f   /* |dT/dt| below this = STABLE        */
#define SKIN_THERM_RATE_MAX_C_MIN   2.00f   /* beyond this is the model, not skin */
#define SKIN_THERM_COOLING_C      (-0.35f)  /* trend at/below this = COOLING      */
#define SKIN_THERM_REWARMING_C      0.50f   /* trend at/above this = REWARMING    */
#define SKIN_THERM_COLD_ABS_C       29.00f  /* absolute floor: vasoconstricted    */
#define SKIN_THERM_SETTLE_MS        12000u  /* ignore the first 12 s of contact   */
#define SKIN_THERM_AMB_COUPLING     0.35f   /* finger tracks ~35% of ambient drift*/
#define SKIN_THERM_AMB_MAX_C        3.00f   /* ignore ambient swing beyond this   */
#define SKIN_THERM_DWELL            3u      /* ticks a new state must hold        */

typedef enum {
    SKIN_THERM_UNKNOWN = 0,
    SKIN_THERM_STABLE,
    SKIN_THERM_COOLING,
    SKIN_THERM_REWARMING,
    SKIN_THERM_VASOCONSTRICTED
} skin_therm_state_t;

typedef struct {
    float    baseline_c;                /* settled skin temp at contact start   */
    float    trend_c;                   /* last - baseline (negative = cooling) */
    float    rate_c_per_min;            /* least-squares slope over the ring    */
    float    ring[SKIN_THERM_RING_SIZE];
    uint32_t ring_ms[SKIN_THERM_RING_SIZE];
    uint8_t  ring_n;
    uint8_t  ring_head;
    float    amb_baseline_c;
    float    trend_amb_c;               /* part of the trend the ROOM explains  */
    skin_therm_state_t state;
    skin_therm_state_t pending;
    uint8_t  pending_n;
    bool     valid;
    uint32_t contact_start_ms;
    bool     contact_active;
} skin_thermal_t;

void skin_thermal_init(skin_thermal_t *t);

/* Reset the contact session. Call when contact is lost. */
void skin_thermal_reset(skin_thermal_t *t);

/*
 * Feed one sample at ~1 Hz.
 *   skin_c   - MEASURED skin temperature (TMP117). <= 0 means "no reading".
 *   amb_c    - ambient temperature, used for common-mode rejection
 *   contact  - true while a finger is on the probe
 *   now_ms   - monotonic milliseconds
 */
void skin_thermal_update(skin_thermal_t *t, float skin_c, float amb_c,
                         bool contact, uint32_t now_ms);

const char *skin_therm_state_name(skin_therm_state_t s);

#endif /* SKIN_THERMAL_H */
