/* Host tests for the per-session core-offset calibration (skin_calib.c).
 *
 * The point of this module is that it must NEVER invent a core temperature.
 * These tests therefore focus as much on the refusal paths as on the maths:
 * a calibration that silently applies a stale or implausible offset is worse
 * than no calibration at all.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "skin_calib.h"

static int fails = 0;
static void ck(int cond, const char *what) {
    printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) fails++;
}

int main(void) {
    skin_calib_t c;
    bool ok;

    printf("Per-session core-offset calibration tests\n=========================================\n");

    /* 1. Uncalibrated never produces a number ----------------------------- */
    skin_calib_init(&c);
    ck(!c.valid, "fresh init is invalid");
    ck(strcmp(skin_calib_status(&c, 32.0f, 1000), "uncalibrated") == 0,
       "status says uncalibrated");
    float v = skin_calib_core_c(&c, 32.0f, 1000, &ok);
    ck(!ok && v == 0.0f, "uncalibrated returns 0.0 with ok=false");
    ck(!skin_calib_is_valid(&c, 32.0f, 1000), "is_valid false when uncalibrated");

    /* 2. A normal fingertip calibration ----------------------------------- */
    /* finger 32.0, oral reference 36.6 -> offset 4.6 */
    ck(skin_calib_set(&c, 32.0f, 36.6f, "oral", 10000), "normal calibration accepted");
    ck(c.valid, "valid after set");
    ck(fabsf(c.offset_c - 4.6f) < 1e-3f, "offset = ref - skin = 4.6 C");
    ck(skin_calib_is_valid(&c, 32.0f, 20000), "still valid 10 s later");
    v = skin_calib_core_c(&c, 32.0f, 20000, &ok);
    ck(ok && fabsf(v - 36.6f) < 1e-3f, "same skin in -> same core out");
    v = skin_calib_core_c(&c, 33.0f, 20000, &ok);
    ck(ok && fabsf(v - 37.6f) < 1e-3f, "skin +1 C -> core +1 C (offset preserved)");

    /* 3. Rejects implausible inputs --------------------------------------- */
    skin_calib_t bad;
    skin_calib_init(&bad);
    ck(!skin_calib_set(&bad, 0.0f, 36.6f, "oral", 1000), "rejects skin = 0 (no contact)");
    ck(!skin_calib_set(&bad, 15.0f, 36.6f, "oral", 1000), "rejects skin below 20 C");
    ck(!skin_calib_set(&bad, 32.0f, 20.0f, "oral", 1000), "rejects reference 20 C (not a body temp)");
    ck(!skin_calib_set(&bad, 32.0f, 50.0f, "oral", 1000), "rejects reference 50 C");
    ck(!bad.valid, "rejected attempts leave it invalid");
    /* offset 12.0 exceeds the sensible maximum: a 26 C finger in a cold room */
    ck(!skin_calib_set(&bad, 24.0f, 36.5f, "oral", 1000),
       "rejects a 12.5 C offset (too large to extrapolate)");
    /* NaN / inf must not slip through */
    ck(!skin_calib_set(&bad, NAN, 36.6f, "oral", 1000), "rejects NaN skin");
    ck(!skin_calib_set(&bad, 32.0f, NAN, "oral", 1000), "rejects NaN reference");
    ck(!skin_calib_set(NULL, 32.0f, 36.6f, "oral", 1000), "NULL state is safe");

    /* 4. Expiry ------------------------------------------------------------ */
    skin_calib_init(&c);
    skin_calib_set(&c, 32.0f, 36.6f, "oral", 5000);
    ck(skin_calib_is_valid(&c, 32.0f, 5000 + SKIN_CALIB_VALID_MS - 1000),
       "valid just before the expiry window");
    ck(!skin_calib_is_valid(&c, 32.0f, 5000 + SKIN_CALIB_VALID_MS + 1000),
       "INVALID just after the expiry window");
    ck(strcmp(skin_calib_status(&c, 32.0f, 5000 + SKIN_CALIB_VALID_MS + 1000), "stale") == 0,
       "status explains it is stale");
    v = skin_calib_core_c(&c, 32.0f, 5000 + SKIN_CALIB_VALID_MS + 1000, &ok);
    ck(!ok && v == 0.0f, "stale calibration yields no number");

    /* 5. Drift invalidation ------------------------------------------------ */
    skin_calib_init(&c);
    skin_calib_set(&c, 32.0f, 36.6f, "oral", 1000);
    ck(skin_calib_is_valid(&c, 32.0f + SKIN_CALIB_MAX_SKIN_DRIFT_C - 0.5f, 2000),
       "small drift still valid");
    ck(!skin_calib_is_valid(&c, 32.0f + SKIN_CALIB_MAX_SKIN_DRIFT_C + 0.5f, 2000),
       "large drift invalidates the frozen offset");
    ck(strcmp(skin_calib_status(&c, 32.0f + SKIN_CALIB_MAX_SKIN_DRIFT_C + 0.5f, 2000),
              "drifted") == 0, "status explains the drift");

    /* 6. Impossible core estimate is refused -------------------------------- */
    /* Craft an offset that puts core above 43 C for a warm reading. */
    skin_calib_init(&c);
    ck(skin_calib_set(&c, 31.0f, 42.5f, "oral", 1000), "calibrate at the edge (offset 11.5)");
    v = skin_calib_core_c(&c, 33.0f, 2000, &ok);   /* would be 44.5 C */
    ck(!ok, "core > 43 C is refused rather than published");

    /* 7. Recalibration replaces cleanly ------------------------------------ */
    skin_calib_init(&c);
    skin_calib_set(&c, 32.0f, 36.6f, "oral", 1000);
    skin_calib_set(&c, 34.0f, 37.0f, "oral", 2000);   /* offset 3.0 */
    v = skin_calib_core_c(&c, 34.0f, 3000, &ok);
    ck(ok && fabsf(v - 37.0f) < 1e-3f, "recalibration replaces the old offset");
    ck(fabsf(c.offset_c - 3.0f) < 1e-3f, "new offset is 3.0 C");

    /* 8. Site is recorded for the audit trail ------------------------------ */
    skin_calib_init(&c);
    skin_calib_set(&c, 32.0f, 36.6f, "oral", 1000);
    ck(strcmp(c.ref_site, "oral") == 0, "reference site recorded as oral");
    skin_calib_set(&c, 32.0f, 36.6f, NULL, 1000);
    ck(c.ref_site && strcmp(c.ref_site, "unknown") == 0, "NULL site becomes \"unknown\", never NULL");

    /* 9. NULL safety on all queries ---------------------------------------- */
    ck(!skin_calib_is_valid(NULL, 32.0f, 1000), "is_valid(NULL) is false");
    ck(skin_calib_core_c(NULL, 32.0f, 1000, &ok) == 0.0f && !ok, "core_c(NULL) is safe");
    ck(strcmp(skin_calib_status(NULL, 32.0f, 1000), "uncalibrated") == 0, "status(NULL) safe");
    ck(skin_calib_core_c(&c, 32.0f, 1000, NULL) >= 0.0f, "core_c with NULL ok is safe");

    printf("\n%s (%d failure%s)\n",
           fails ? "FAILED" : "ALL SKIN CALIBRATION CHECKS PASSED",
           fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
