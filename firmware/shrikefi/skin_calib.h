/*
 * skin_calib.h - per-session core-offset calibration for VALOR's temperature
 * channel.
 *
 * WHY THIS EXISTS
 * ---------------
 * A fingertip thermistor reads 3-7 C below core, and that gap is set by
 * vasoconstriction and ambient temperature, not by a sensor error. It is not a
 * fixed offset, so a global skin->core regression fitted on one subject fails
 * to transfer and, worse, looks precise while being meaningless.
 *
 * The published approach for continuous monitoring is an INDIVIDUALISED
 * CORRECTION FACTOR: take one reference reading (oral, tympanic) with the skin
 * probe in place, compute the offset for THIS subject at THIS moment, then
 * report core = skin + offset and track changes from there. See e.g.
 * "Estimating changes in volume-weighted mean body temperature using
 * thermometry with an individualized correction factor" (J Appl Physiol 2010).
 *
 * That is what this module does. It is a calibration in the honest sense: it
 * uses a real reference measurement, it is per-session, and it expires.
 *
 * WHAT IT DOES NOT DO
 * -------------------
 * It never claims to work without a reference reading. Until skin_calib_set()
 * has been called with a real reference, every query returns "not calibrated"
 * and the caller must publish nothing rather than a number.
 */
#ifndef SKIN_CALIB_H
#define SKIN_CALIB_H

#include <stdint.h>
#include <stdbool.h>

/* A per-session offset is only valid while the subject's vasomotor state is
 * stable. Ambient swings, exercise and cold exposure all invalidate it. After
 * this long the calibration is treated as stale and must be redone. */
#define SKIN_CALIB_VALID_MS        (4u * 60u * 60u * 1000u)   /* 4 hours */

/* How far the skin reading may drift from the value it had at calibration
 * before the frozen offset is considered invalid. Beyond this the relationship
 * has almost certainly changed (the subject moved, or the room did) and
 * continuing to apply the old offset would report a core temperature that
 * follows neither. */
#define SKIN_CALIB_MAX_SKIN_DRIFT_C  3.00f

/* Reject absurd offsets: a fingertip offset larger than this means the
 * reference was taken at the wrong site or the probe was not in contact. */
#define SKIN_CALIB_MIN_OFFSET_C     (-2.0f)
#define SKIN_CALIB_MAX_OFFSET_C      12.0f

typedef struct {
    bool     valid;             /* set() has succeeded at least once          */
    float    offset_c;          /* T_ref - T_skin at calibration              */
    float    ref_c;             /* the reference reading that produced it     */
    float    skin_at_cal_c;     /* skin value at calibration                  */
    uint32_t cal_ms;            /* when it was taken                          */
    const char *ref_site;       /* "oral", "tympanic", "axillary", ...        */
} skin_calib_t;

/* Initialise to "no calibration". */
void skin_calib_init(skin_calib_t *c);

/*
 * Record a reference measurement.
 *
 *   c        - calibration state
 *   skin_c   - the skin reading AT THE MOMENT of the reference (required)
 *   ref_c    - the reference thermometer reading
 *   site     - reference site, static string ("oral" etc.)
 *   now_ms   - timestamp
 *
 * Returns true if accepted. Rejected when: the skin reading is not a plausible
 * contact value, the reference is outside a survivable range, or the implied
 * offset is not physically sensible (see the MIN/MAX above).
 */
bool skin_calib_set(skin_calib_t *c, float skin_c, float ref_c,
                    const char *site, uint32_t now_ms);

/* True when a calibration exists AND is still fresh AND the skin has not
 * drifted far enough to invalidate it. */
bool skin_calib_is_valid(const skin_calib_t *c, float current_skin_c, uint32_t now_ms);

/*
 * Apply the calibration.
 *
 * Returns the estimated core temperature, or 0.0f with *ok = false when the
 * calibration is not valid for the current reading. Callers MUST check *ok and
 * publish "--" rather than 0.0f.
 */
float skin_calib_core_c(const skin_calib_t *c, float current_skin_c,
                        uint32_t now_ms, bool *ok);

/* Why the calibration is not currently usable, for the UI/log.
 * Returns a static string; never NULL. */
const char *skin_calib_status(const skin_calib_t *c, float current_skin_c, uint32_t now_ms);

#endif /* SKIN_CALIB_H */
