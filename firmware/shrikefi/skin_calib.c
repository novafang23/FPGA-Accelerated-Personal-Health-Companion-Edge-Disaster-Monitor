/* skin_calib.c - per-session core-offset calibration. See skin_calib.h. */
#include "skin_calib.h"
#include <math.h>

#define SKIN_CALIB_MIN_SKIN_C   20.0f   /* below this nothing is in contact */
#define SKIN_CALIB_MAX_SKIN_C   42.0f
#define SKIN_CALIB_MIN_REF_C    30.0f   /* survivable reference window */
#define SKIN_CALIB_MAX_REF_C    43.0f

void skin_calib_init(skin_calib_t *c)
{
    if (!c) return;
    c->valid = false;
    c->offset_c = 0.0f;
    c->ref_c = 0.0f;
    c->skin_at_cal_c = 0.0f;
    c->cal_ms = 0u;
    c->ref_site = "none";
}

bool skin_calib_set(skin_calib_t *c, float skin_c, float ref_c,
                    const char *site, uint32_t now_ms)
{
    if (!c) return false;

    /* Both values must be plausible before an offset can mean anything. A
     * calibration taken with no finger on the probe would bake in the room
     * temperature as a "core offset"; rejecting it is the whole point. */
    if (!isfinite(skin_c) || !isfinite(ref_c)) return false;
    if (skin_c < SKIN_CALIB_MIN_SKIN_C || skin_c > SKIN_CALIB_MAX_SKIN_C) return false;
    if (ref_c  < SKIN_CALIB_MIN_REF_C  || ref_c  > SKIN_CALIB_MAX_REF_C)  return false;

    const float offset = ref_c - skin_c;
    if (offset < SKIN_CALIB_MIN_OFFSET_C || offset > SKIN_CALIB_MAX_OFFSET_C) {
        /* e.g. a fingertip at 26 C with an oral reference at 36.8 C gives a
         * 10.8 C offset: physiologically real (the subject is vasoconstricted)
         * but far too large to extrapolate an absolute core from. Refuse. */
        return false;
    }

    c->offset_c = offset;
    c->ref_c = ref_c;
    c->skin_at_cal_c = skin_c;
    c->cal_ms = now_ms;
    c->ref_site = (site && *site) ? site : "unknown";
    c->valid = true;
    return true;
}

/* Return the reason the calibration is unusable, or NULL when it is fine. */
static const char *invalid_reason(const skin_calib_t *c, float current_skin_c, uint32_t now_ms)
{
    if (!c) return "uncalibrated";
    if (!c->valid) return "uncalibrated";
    if (c->cal_ms == 0u) return "uncalibrated";

    /* Wrapping-safe elapsed time. */
    const uint32_t elapsed = now_ms - c->cal_ms;
    if (elapsed > SKIN_CALIB_VALID_MS) return "stale";

    if (current_skin_c < SKIN_CALIB_MIN_SKIN_C || current_skin_c > SKIN_CALIB_MAX_SKIN_C) {
        return "no contact";
    }
    /* The frozen offset only holds while the skin stays near where it was
     * measured. A large drift means the subject's vasomotor state changed, and
     * applying the old offset would report a core temperature that follows
     * neither the old state nor the new one. */
    if (fabsf(current_skin_c - c->skin_at_cal_c) > SKIN_CALIB_MAX_SKIN_DRIFT_C) {
        return "drifted";
    }
    return NULL;
}

bool skin_calib_is_valid(const skin_calib_t *c, float current_skin_c, uint32_t now_ms)
{
    return invalid_reason(c, current_skin_c, now_ms) == NULL;
}

float skin_calib_core_c(const skin_calib_t *c, float current_skin_c,
                        uint32_t now_ms, bool *ok)
{
    if (ok) *ok = false;
    if (invalid_reason(c, current_skin_c, now_ms) != NULL) return 0.0f;

    const float core = current_skin_c + c->offset_c;
    /* A core estimate outside survival bounds is a contradiction, not a
     * finding: the offset no longer describes reality. Refuse rather than
     * publish an impossible number. */
    if (core < 30.0f || core > 43.0f) return 0.0f;

    if (ok) *ok = true;
    return core;
}

const char *skin_calib_status(const skin_calib_t *c, float current_skin_c, uint32_t now_ms)
{
    const char *why = invalid_reason(c, current_skin_c, now_ms);
    return why ? why : "calibrated";
}
