/* Host tests for the peripheral thermal state machine (SKIN_THERM_*).
 *
 * The point of this feature is that it must NOT pretend to be a core
 * temperature, so these tests check three things:
 *   1. the state is driven by the subject's OWN baseline and by trend direction;
 *   2. it degrades to UNKNOWN rather than guessing;
 *   3. it still reports cold stress when the reading falls BELOW the display
 *      range -- the case where `is_valid` is false but the subject is coldest.
 *
 * Drivers are stepped the way real hardware steps them: by the MAX30102 die
 * temperature, not by a fabricated target.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "skin_estimate.h"

static int fails = 0;
static void ck(int cond, const char *what) {
    printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) fails++;
}

#define AMB 24.0f

/* Establish an idle die baseline at `idle_die` for 10 s (no finger). */
static void idle(skin_estimate_t *st, float idle_die, uint32_t *t) {
    for (int i = 0; i < 10; i++) {
        *t += 1000u;
        skin_estimate_update(st, idle_die, AMB, false, *t);
    }
}

/* Hold a finger on for n seconds with the die at `die_c`. */
static void contact(skin_estimate_t *st, float die_c, int n, uint32_t *t) {
    for (int i = 0; i < n; i++) {
        *t += 1000u;
        skin_estimate_update(st, die_c, AMB, true, *t);
    }
}

int main(void) {
    skin_estimate_t st;
    uint32_t t;
    printf("Peripheral thermal state tests\n=============================\n");

    /* 1. Unknown before any contact --------------------------------------- */
    skin_estimate_init(&st, SKIN_EST_DEFAULT_B0, SKIN_EST_DEFAULT_B1,
                       SKIN_EST_DEFAULT_B2, false);
    ck(st.pt_state == SKIN_THERM_UNKNOWN, "fresh estimator reports UNKNOWN");
    ck(strcmp(skin_therm_state_name(st.pt_state), "UNKNOWN") == 0,
       "name() of UNKNOWN is \"UNKNOWN\"");

    /* 2. No contact -> stays UNKNOWN -------------------------------------- */
    t = 0;
    idle(&st, 28.0f, &t);
    contact(&st, 28.0f, 0, &t);
    ck(st.pt_state == SKIN_THERM_UNKNOWN, "no finger -> still UNKNOWN");
    ck(!st.pt_valid, "no finger -> pt_valid false");

    /* 3. Warm steady finger -> STABLE ------------------------------------- */
    skin_estimate_init(&st, SKIN_EST_DEFAULT_B0, SKIN_EST_DEFAULT_B1,
                       SKIN_EST_DEFAULT_B2, false);
    t = 0;
    idle(&st, 28.0f, &t);
    contact(&st, 31.5f, 90, &t);        /* die warms: finger present, settled */
    ck(st.is_settled, "warm finger settled");
    ck(st.pt_valid, "ring deep enough -> pt_valid true");
    ck(st.pt_state == SKIN_THERM_STABLE, "steady warm finger -> STABLE");
    ck(fabsf(st.pt_trend_c) < 0.5f, "steady finger -> trend near zero");

    /* 4. Cold finger below idle die -> cooling path -> VASOCONSTRICTED ---- */
    skin_estimate_init(&st, SKIN_EST_DEFAULT_B0, SKIN_EST_DEFAULT_B1,
                       SKIN_EST_DEFAULT_B2, false);
    t = 0;
    idle(&st, 28.0f, &t);
    contact(&st, 24.0f, 80, &t);        /* finger cools the die below idle */
    printf("     (cold case: est=%.2f below_range=%d state=%s)\n",
           (double)st.estimated_skin_c, st.is_below_range,
           skin_therm_state_name(st.pt_state));
    ck(st.estimated_skin_c < SKIN_EST_PT_COLD_ABS_C,
       "cold finger drives the estimate below the 29 C floor");
    ck(st.is_below_range, "cold reading is flagged below display range");
    ck(!st.is_valid, "...and is_valid is false (display bound, not liveness)");
    ck(st.pt_state == SKIN_THERM_VASOCONSTRICTED,
       "cold finger still reports VASOCONSTRICTED, not UNKNOWN");
    ck(st.pt_valid, "state machine stayed live through the cold reading");

    /* 5. Rewarming after cold -> REWARMING -------------------------------- */
    /* Gentle ramp (0.15 C/s die), then hold, so the estimator is not chasing
     * its slew limit -- real skin warming is slow. */
    for (int i = 0; i < 60; i++) {
        t += 1000u;
        skin_estimate_update(&st, 24.0f + 9.0f * (float)i / 59.0f, AMB, true, t);
    }
    for (int i = 0; i < 20; i++) { t += 1000u; skin_estimate_update(&st, 33.0f, AMB, true, t); }
    printf("     (rewarm: est=%.2f trend=%+.2f rate=%+.2f state=%s)\n",
           (double)st.estimated_skin_c, (double)st.pt_trend_c,
           (double)st.pt_rate_c_per_min, skin_therm_state_name(st.pt_state));
    ck(st.pt_trend_c > 0.0f, "warming back up produces an upward trend");
    ck(st.pt_state == SKIN_THERM_REWARMING, "recovered periphery -> REWARMING");
    ck(fabsf(st.pt_rate_c_per_min) <= SKIN_EST_PT_RATE_MAX_C_MIN + 1e-3f,
       "reported rate is clamped to a physically possible skin rate");
    /* The state machine must not have dropped out mid-transition: the whole
     * point is that it keeps tracking while the estimate is moving. */
    ck(st.pt_ring_n == SKIN_EST_PT_RING, "ring stayed full across the transition");

    /* 6. Finger lift resets everything ------------------------------------ */
    skin_estimate_reset(&st);
    ck(st.pt_state == SKIN_THERM_UNKNOWN, "reset -> UNKNOWN");
    ck(!st.pt_valid, "reset -> pt_valid false");
    ck(st.pt_ring_n == 0u, "reset -> ring emptied");

    /* 7. A new contact must not inherit the old baseline ------------------ */
    t = 0;
    idle(&st, 28.0f, &t);
    contact(&st, 31.5f, 90, &t);
    ck(st.pt_ring_n > 0u, "new contact refills the ring");
    ck(fabsf(st.pt_trend_c) < 1.0f,
       "new contact trends from ITS OWN baseline, not the old one");

    /* 8. Names are stable and non-NULL ------------------------------------ */
    int all_named = 1;
    for (int s = SKIN_THERM_UNKNOWN; s <= SKIN_THERM_VASOCONSTRICTED; s++) {
        if (!skin_therm_state_name((skin_therm_state_t)s)) { all_named = 0; break; }
    }
    ck(all_named, "all state names are non-NULL");
    ck(strcmp(skin_therm_state_name(SKIN_THERM_VASOCONSTRICTED),
              "VASOCONSTRICTED") == 0, "VASOCONSTRICTED name is exact");
    ck(strcmp(skin_therm_state_name((skin_therm_state_t)99), "UNKNOWN") == 0,
       "out-of-range enum falls back to UNKNOWN");

    /* 9. NULL safety ----------------------------------------------------- */
    skin_estimate_update_thermal(NULL, 0);
    ck(1, "update_thermal(NULL) is safe");

    /* 10. Ambient common-mode rejection -----------------------------------
     * A room that cools drags the finger with it. Both the finger AND ambient
     * fall together, so that must NOT be reported as cold stress: it is the
     * environment changing, not the subject constricting. */
    skin_estimate_init(&st, SKIN_EST_DEFAULT_B0, SKIN_EST_DEFAULT_B1,
                       SKIN_EST_DEFAULT_B2, false);
    t = 0;
    idle(&st, 28.0f, &t);
    contact(&st, 31.5f, 90, &t);          /* settle at 24 C ambient */
    ck(st.pt_state == SKIN_THERM_STABLE, "baseline established at 24 C ambient");
    /* Now the WHOLE room cools 2 C: ambient 24 -> 22 and the die follows it
     * down by the same amount (the finger tracks the room). */
    for (int i = 0; i < 40; i++) {
        t += 1000u;
        float drop = 2.0f * (float)i / 39.0f;
        skin_estimate_update(&st, 31.5f - drop, AMB - drop, true, t);
    }
    printf("     (room cooled 2 C: raw would be %.2f, ambient explains %.2f, trend=%+.2f -> %s)\n",
           (double)(st.estimated_skin_c - st.pt_baseline_c), (double)st.pt_trend_amb_c,
           (double)st.pt_trend_c, skin_therm_state_name(st.pt_state));
    ck(st.pt_trend_amb_c < -0.5f, "ambient component detected and quantified");
    ck(st.pt_state != SKIN_THERM_COOLING,
       "room cooling alone does NOT raise a cold-stress alert");

    /* 11. Hysteresis: one noisy DIE sample must not flip the label -------- */
    skin_estimate_init(&st, SKIN_EST_DEFAULT_B0, SKIN_EST_DEFAULT_B1,
                       SKIN_EST_DEFAULT_B2, false);
    t = 0;
    idle(&st, 28.0f, &t);
    contact(&st, 31.5f, 120, &t);          /* long enough to be firmly settled */
    skin_therm_state_t before = st.pt_state;
    ck(before == SKIN_THERM_STABLE, "settled and stable before the spike");
    /* A single wild die reading (sensor glitch / momentary lift), then back. */
    t += 1000u; skin_estimate_update(&st, 34.0f, AMB, true, t);
    t += 1000u; skin_estimate_update(&st, 31.5f, AMB, true, t);
    ck(st.pt_state == before,
       "a single glitch sample does not change the published state");

    /* 12. A sustained cooling DOES publish COOLING -----------------------
     * This is the case the old peak-hold estimator could not produce at all:
     * a warm finger that starts cooling while still above the idle baseline. */
    for (int i = 0; i < 90; i++) {
        t += 1000u;
        float die = 31.5f - 1.8f * (float)i / 89.0f;   /* gentle 1.8 C drop */
        skin_estimate_update(&st, die, AMB, true, t);
    }
    printf("     (sustained cooling: est=%.2f trend=%+.2f rate=%+.2f -> %s)\n",
           (double)st.estimated_skin_c, (double)st.pt_trend_c,
           (double)st.pt_rate_c_per_min, skin_therm_state_name(st.pt_state));
    ck(st.pt_state == SKIN_THERM_COOLING,
       "a warm finger that then cools IS detected as COOLING");

    printf("\n%s (%d failure%s)\n",
           fails ? "FAILED" : "ALL PERIPHERAL THERMAL CHECKS PASSED",
           fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
