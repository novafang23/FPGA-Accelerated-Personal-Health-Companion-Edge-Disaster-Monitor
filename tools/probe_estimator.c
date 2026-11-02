/* Adversarial test of the supplied estimator. Nothing is modified: this is a
 * separate harness that links their .c and probes the edge cases their
 * description does not cover. */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "valor_temp_estimator.h"

static int fails = 0;
static void ck(int cond, const char *what) {
    printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) fails++;
}

static const vtemp_cal_t CAL_OK  = { 30.0f, 0.3f, 0.1f, 0.2f, 0.3f, true  };
static const vtemp_cal_t CAL_BAD = { 0, 0, 0, 0, 0, false };

static void feed(vtemp_state_t *s, vtemp_out_t *o, float tsk, float amb, float hr, float pi, int n) {
    vtemp_in_t in = { tsk, amb, hr, pi };
    for (int i = 0; i < n; i++) vtemp_update(s, &in, o);
}

int main(void) {
    vtemp_state_t s; vtemp_out_t o;
    printf("Supplied estimator - adversarial checks\n========================================\n");

    /* 1. NEWS2 band edges vs the project's existing engine */
    printf("\n[1] NEWS2 band edges\n");
    struct { float t; int want; } cases[] = {
        {35.0f,3},{35.04f,3},{35.05f,1},   /* rounding at 35.05 */
        {35.1f,1},{36.0f,1},{36.04f,1},{36.05f,0},
        {38.0f,0},{38.04f,0},{38.05f,1},
        {39.0f,1},{39.04f,1},{39.05f,2},
        {34.9f,3},{39.5f,2},{36.5f,0},
    };
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        int got = vtemp_news2_score(cases[i].t);
        char buf[80];
        snprintf(buf, sizeof buf, "%.2f C -> %d (want %d)", cases[i].t, got, cases[i].want);
        ck(got == cases[i].want, buf);
    }

    /* 2. Uncalibrated must be -1, never 0 */
    printf("\n[2] Uncalibrated gate\n");
    vtemp_init(&s, &CAL_BAD);
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 2.0f, 120);
    ck(o.status == VT_UNCALIBRATED, "placeholder coefficients -> VT_UNCALIBRATED");
    ck(o.news2_temp == -1, "news2_temp == -1 (not 0)");

    /* 3. No contact */
    printf("\n[3] Contact gate\n");
    vtemp_init(&s, &CAL_OK);
    feed(&s, &o, 26.0f, 24.0f, 70.0f, 2.0f, 120);   /* only +2 C above amb */
    ck(o.status == VT_NO_CONTACT, "+2 C above ambient -> VT_NO_CONTACT");
    ck(o.news2_temp == -1, "no-contact -> -1");

    /* 4. Settling time */
    printf("\n[4] Settling gate\n");
    vtemp_init(&s, &CAL_OK);
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 2.0f, 44);
    ck(o.status == VT_SETTLING, "44 s contact -> still VT_SETTLING");
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 2.0f, 30);
    ck(o.status == VT_OK, "74 s contact -> VT_OK");

    /* 5. Vasoconstriction */
    printf("\n[5] Perfusion gate\n");
    vtemp_init(&s, &CAL_OK);
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 0.3f, 120);
    ck(o.status == VT_VASOCONSTRICTED, "PI 0.3%% -> VT_VASOCONSTRICTED");
    ck(o.news2_temp == -1, "vasoconstricted -> -1");

    /* 6. PI <= 0 means UNKNOWN. Does the code treat it as "pass"? */
    printf("\n[6] PI unknown (<=0) path\n");
    vtemp_init(&s, &CAL_OK);
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 0.0f, 120);
    printf("  PI=0.0 (unknown) -> status=%d news2=%d\n", (int)o.status, (int)o.news2_temp);
    ck(o.status == VT_OK, "PI unknown is treated as 'do not gate' and scores anyway");

    /* 7. Temperature clamp then scoring */
    printf("\n[7] Range clamp\n");
    vtemp_init(&s, &CAL_OK);
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 2.0f, 120);
    printf("  normal case -> tcore=%.2f news2=%d\n", o.tcore_est_c, (int)o.news2_temp);

    /* 8. Borderline behaviour */
    printf("\n[8] Borderline rule\n");
    const vtemp_cal_t NEAR_EDGE = { 30.0f, 0.3f, 0.1f, 0.0f, 0.5f, true };
    vtemp_init(&s, &NEAR_EDGE);
    float tsk_solved = (36.0f - 30.0f - 0.1f * 24.0f) / 0.3f;  /* aim tcore ~36.0 */
    feed(&s, &o, tsk_solved, 24.0f, 70.0f, 2.0f, 120);
    printf("  tuned to sit at %.2f C, sigma 0.5 -> news2=%d borderline=%d\n",
           o.tcore_est_c, (int)o.news2_temp, (int)o.borderline);
    ck(o.borderline == true, "straddling a band edge sets borderline");

    /* 9. HR term magnitude */
    printf("\n[9] HR term contribution\n");
    vtemp_init(&s, &CAL_OK); feed(&s, &o, 33.0f, 24.0f, 70.0f,  2.0f, 120);
    float t70 = o.tcore_est_c;
    vtemp_init(&s, &CAL_OK); feed(&s, &o, 33.0f, 24.0f, 120.0f, 2.0f, 120);
    float t120 = o.tcore_est_c;
    printf("  HR70 -> %.3f   HR120 -> %.3f   delta=%.3f C\n", t70, t120, t120 - t70);

    /* 10. Contact counter saturation / reset */
    printf("\n[10] Contact reset\n");
    vtemp_init(&s, &CAL_OK);
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 2.0f, 120);
    ck(o.status == VT_OK, "was OK");
    feed(&s, &o, 25.0f, 24.0f, 72.0f, 2.0f, 1);   /* finger off */
    ck(o.status == VT_NO_CONTACT, "finger off -> NO_CONTACT");
    feed(&s, &o, 33.0f, 24.0f, 72.0f, 2.0f, 50);  /* finger back on */
    ck(o.status == VT_SETTLING, "re-contact restarts the 45 s settle");

    printf("\n%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
