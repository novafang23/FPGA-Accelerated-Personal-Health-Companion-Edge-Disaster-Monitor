/*
 * test_fall_detect.c
 * Comprehensive Unit Test Suite for 3-Stage Biomechanical Fall Detection FSM
 * SIH26181 Health Companion & Edge Disaster Monitor
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "fall_detect.h"

static void test_default_config_and_init(void) {
    printf("Running test_default_config_and_init...\n");
    fall_config_t cfg;
    fall_detect_default_config(&cfg);

    assert(fabsf(cfg.free_fall_thresh_g - 0.60f) < 0.001f);
    assert(fabsf(cfg.impact_thresh_g - 2.50f) < 0.001f);
    assert(fabsf(cfg.gyro_impact_thresh_dps - 80.0f) < 0.001f);
    assert(fabsf(cfg.stillness_accel_tol_g - 0.30f) < 0.001f);
    assert(fabsf(cfg.stillness_gyro_thresh_dps - 40.0f) < 0.001f);
    assert(cfg.free_fall_max_ms == 800u);
    assert(cfg.post_impact_settle_ms == 400u);
    assert(cfg.inactivity_duration_ms == 1500u);

    fall_detector_t det;
    fall_detect_init(&det, NULL);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fall_detect_is_confirmed(&det) == false);
    assert(fabsf(det.baseline_az - 1.0f) < 0.001f);
    assert(fall_detect_is_motion_quiet(&det) == true);
    assert(strcmp(fall_state_name(FALL_STATE_MONITORING), "MONITORING") == 0);
    assert(strcmp(fall_state_name(FALL_STATE_FREE_FALL), "FREE_FALL") == 0);
    assert(strcmp(fall_state_name(FALL_STATE_IMPACT), "IMPACT") == 0);
    assert(strcmp(fall_state_name(FALL_STATE_INACTIVITY), "INACTIVITY") == 0);
    assert(strcmp(fall_state_name(FALL_STATE_CONFIRMED), "FALL_CONFIRMED") == 0);

    printf("  [PASS] Default config and initialization verified.\n");
}

static void test_free_fall_timeout_returns_to_monitoring(void) {
    printf("Running test_free_fall_timeout_returns_to_monitoring...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 1000;
    /* Normal gravity */
    fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    /* Step into weightlessness: free fall (< 0.60g) */
    t += 50;
    bool res = fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 10.0f, 10.0f, 10.0f);
    assert(res == false);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* Remain in free-fall for 850 ms (exceeds free_fall_max_ms of 800 ms) with no impact */
    t += 850;
    res = fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 10.0f, 10.0f, 10.0f);
    assert(res == false);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    printf("  [PASS] Free fall timeout correctly returned to MONITORING.\n");
}

static void test_impact_without_free_fall_rejected(void) {
    printf("Running test_impact_without_free_fall_rejected...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 2000;
    /* Device bumped or tapped on table: acceleration spikes to 3.5g without prior weightlessness */
    fall_detect_update(&det, t, 2.0f, 2.0f, 2.0f, 120.0f, 90.0f, 50.0f);
    assert(fall_detect_is_confirmed(&det) == false);

    /* Orientation unchanged afterwards (upright): soft path must stand down, never confirm */
    for (int i = 0; i < 160; i++) {
        t += 25;
        fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
        assert(fall_detect_is_confirmed(&det) == false);
    }
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fabsf(fall_detect_get_impact_accel(&det)) < 0.001f);

    printf("  [PASS] Sudden shock without weightlessness and no posture change is not a fall.\n");
}

static void test_soft_fall_slump_confirmed(void) {
    printf("Running test_soft_fall_slump_confirmed...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 1000;
    /* Upright baseline (gravity on Z) */
    for (int i = 0; i < 80; i++) {
        t += 25;
        fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    }
    /* Slump: moderate 2.0g impact, no free-fall */
    t += 25;
    fall_detect_update(&det, t, 1.4f, 1.4f, 0.0f, 20.0f, 20.0f, 20.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    /* Lying on side: gravity now on X, still */
    bool confirmed = false;
    for (int i = 0; i < 160; i++) {
        t += 25;
        if (fall_detect_update(&det, t, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f)) confirmed = true;
    }
    assert(confirmed);
    assert(fall_detect_is_confirmed(&det));
    assert(fall_detect_get_posture_angle_deg(&det) > 55.0f);

    /* Clearing leaves a clean detector and clears the soft flag */
    fall_detect_clear_alarm(&det);
    assert(det.soft_path == false);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    printf("  [PASS] Slump without free-fall + posture change confirmed as fall.\n");
}

static void test_soft_fall_disabled(void) {
    printf("Running test_soft_fall_disabled...\n");
    fall_config_t cfg;
    fall_detect_default_config(&cfg);
    cfg.soft_impact_thresh_g = 0.0f;
    fall_detector_t det;
    fall_detect_init(&det, &cfg);

    uint32_t t = 1000;
    for (int i = 0; i < 80; i++) { t += 25; fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0, 0, 0); }
    t += 25;
    fall_detect_update(&det, t, 1.4f, 1.4f, 0.0f, 20.0f, 20.0f, 20.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    printf("  [PASS] Soft path can be disabled via config.\n");
}

static void test_impact_gyro_threshold_gating(void) {
    printf("Running test_impact_gyro_threshold_gating...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 3000;
    /* Free fall weightlessness */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 10.0f, 10.0f, 10.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* High acceleration impact (3.2g), but 0 dps gyro (no rotational tumble) */
    t += 100;
    fall_detect_update(&det, t, 1.8f, 1.8f, 1.8f, 0.0f, 0.0f, 0.0f);
    /* Should remain in FREE_FALL, rejecting impact without rotation */
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* After timeout, should abort to MONITORING */
    t += 800;
    fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    printf("  [PASS] Impact without rotational jerk rejected by gyro gate.\n");
}

static void test_rebound_settling_window(void) {
    printf("Running test_rebound_settling_window...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 4000;
    /* Free fall */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 30.0f, 40.0f, 20.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* Impact deceleration shock (3.8g, 200 dps) */
    t += 100;
    fall_detect_update(&det, t, 1.8f, 2.2f, 2.5f, 160.0f, 100.0f, 60.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    /* Physical rebound settling window (400 ms): samples during first 300 ms should NOT start stillness */
    t += 300;
    bool res = fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
    assert(res == false);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);
    assert(fall_detect_is_confirmed(&det) == false);

    printf("  [PASS] Post-impact rebound settling period enforced.\n");
}

static void test_vigorous_recovery_rejects_false_alarm(void) {
    printf("Running test_vigorous_recovery_rejects_false_alarm...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 5000;
    /* Free fall */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 30.0f, 40.0f, 20.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* Impact */
    t += 100;
    fall_detect_update(&det, t, 2.0f, 2.0f, 2.0f, 150.0f, 80.0f, 40.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    /* User immediately jumps back up and starts walking (dynamic accel 0.8g, high gyro 150 dps) */
    t += 450;
    fall_detect_update(&det, t, 0.8f, 0.6f, 1.5f, 150.0f, 90.0f, 60.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fall_detect_is_confirmed(&det) == false);
    assert(fall_detect_get_impact_accel(&det) == 0.0f);
    assert(fall_detect_get_impact_gyro(&det) == 0.0f);
    assert(fall_detect_get_posture_angle_deg(&det) == 0.0f);

    printf("  [PASS] Vigorous recovery motion immediately aborts false alarm.\n");
}

static void test_continuous_stillness_reset_on_motion(void) {
    printf("Running test_continuous_stillness_reset_on_motion...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 6000;
    /* Stage 1: Free fall */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 30.0f, 40.0f, 20.0f);
    /* Stage 2: Impact */
    t += 100;
    fall_detect_update(&det, t, 2.0f, 2.0f, 2.0f, 150.0f, 80.0f, 40.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    /* Settle 450 ms */
    t += 450;

    /* Still for 1.0s (10 steps of 100 ms) */
    for (int i = 0; i < 10; i++) {
        t += 100;
        bool res = fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
        assert(res == false);
        assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);
    }

    /* Interruption: User moves arm or twitches (g_mag = 60 dps > 40 dps threshold) */
    t += 100;
    bool res = fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 60.0f, 30.0f, 20.0f);
    assert(res == false);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    /* Now still again for only 800 ms: total time since impact is large, but CONTINUOUS stillness is only 800ms! */
    for (int i = 0; i < 8; i++) {
        t += 100;
        res = fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
        assert(res == false);
        assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);
    }

    /* Complete full 1500 ms continuous stillness (need 8 more steps of 100ms: 700ms + 800ms = 1500ms) */
    for (int i = 0; i < 8; i++) {
        t += 100;
        res = fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
    }
    assert(res == true);
    assert(fall_detect_get_state(&det) == FALL_STATE_CONFIRMED);
    assert(fall_detect_is_confirmed(&det) == true);

    printf("  [PASS] Interrupted stillness resets timer; uninterrupted 1.5s enforced.\n");
}

static void test_complete_biomechanical_fall_and_posture(void) {
    printf("Running test_complete_biomechanical_fall_and_posture...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 8000;
    /* Normal baseline standing: Az = 1.0g, Ax = 0.0g, Ay = 0.0g */
    for (int i = 0; i < 20; i++) {
        t += 25;
        fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    }
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    /* Stage 1: Free fall weightlessness (0.33g) */
    t += 50;
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.3f, 30.0f, 40.0f, 20.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* Stage 2: High deceleration impact shock (3.8g, 222 dps) */
    t += 150;
    fall_detect_update(&det, t, 1.8f, 2.2f, 2.5f, 180.0f, 110.0f, 70.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);
    assert(fall_detect_get_impact_accel(&det) >= 3.7f);
    assert(fall_detect_get_impact_gyro(&det) >= 220.0f);

    /* Settle 450 ms */
    t += 450;

    /* Stage 3: Post-impact stillness horizontal (Ax = 0.98g, Ay = 0.05g, Az = 0.08g) */
    bool latched = false;
    for (int i = 0; i < 16; i++) {
        t += 100;
        bool res = fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 2.0f, 1.0f);
        if (res) latched = true;
    }
    assert(latched == true);
    assert(fall_detect_get_state(&det) == FALL_STATE_CONFIRMED);
    assert(fall_detect_is_confirmed(&det) == true);

    /* Posture angle calculation */
    float angle = fall_detect_get_posture_angle_deg(&det);
    printf("  Calculated posture angle change: %.1f deg\n", angle);
    assert(angle > 75.0f && angle < 95.0f);

    /* Alarm clearing */
    fall_detect_clear_alarm(&det);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fall_detect_is_confirmed(&det) == false);
    assert(fall_detect_get_posture_angle_deg(&det) == 0.0f);
    assert(fall_detect_get_impact_accel(&det) == 0.0f);
    assert(fall_detect_get_impact_gyro(&det) == 0.0f);

    printf("  [PASS] 3-stage fall detection and posture angle change validated.\n");
}

static void test_auto_standby_timeout(void) {
    printf("Running test_auto_standby_timeout...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 10000;
    /* Trigger a fall */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 30.0f, 40.0f, 20.0f);
    t += 100;
    fall_detect_update(&det, t, 2.0f, 2.0f, 2.0f, 150.0f, 80.0f, 40.0f);
    t += 450;
    for (int i = 0; i < 16; i++) {
        t += 100;
        fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
    }
    assert(fall_detect_get_state(&det) == FALL_STATE_CONFIRMED);

    /* 59 seconds later: still CONFIRMED */
    t += 59000;
    fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_CONFIRMED);

    /* 61 seconds later: auto-standby resets to MONITORING */
    t += 2000;
    fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 1.0f, 1.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fall_detect_is_confirmed(&det) == false);

    printf("  [PASS] Auto-standby clears alarm after 60s without manual reset.\n");
}

static void test_null_pointer_robustness(void) {
    printf("Running test_null_pointer_robustness...\n");
    fall_detect_default_config(NULL);
    fall_detect_init(NULL, NULL);
    fall_detect_reset(NULL);
    fall_detect_clear_alarm(NULL);

    assert(fall_detect_get_state(NULL) == FALL_STATE_MONITORING);
    assert(strcmp(fall_state_name((fall_state_t)999), "UNKNOWN") == 0);
    assert(fall_detect_get_motion_energy(NULL) == 0.0f);
    assert(fall_detect_is_motion_quiet(NULL) == true);
    assert(fall_detect_is_confirmed(NULL) == false);
    assert(fall_detect_get_impact_accel(NULL) == 0.0f);
    assert(fall_detect_get_impact_gyro(NULL) == 0.0f);
    /* Timestamp backwards / underflow robustness */
    fall_detector_t tdet;
    fall_detect_init(&tdet, NULL);
    fall_detect_update(&tdet, 5000, 3.0f, 3.0f, 3.0f, 100.0f, 100.0f, 100.0f);
    assert(fall_detect_get_state(&tdet) == FALL_STATE_INACTIVITY);
    fall_detect_update(&tdet, 2000, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    assert(fall_detect_get_state(&tdet) == FALL_STATE_INACTIVITY);

    printf("  [PASS] Null pointers handled safely without faults.\n");
}

static void test_inactivity_timeout_clears_metrics(void) {
    printf("Running test_inactivity_timeout_clears_metrics...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 15000;
    /* Stage 1: Free fall */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 30.0f, 40.0f, 20.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);

    /* Stage 2: Impact (3.8g shock) */
    t += 100;
    fall_detect_update(&det, t, 2.0f, 2.0f, 2.0f, 150.0f, 80.0f, 40.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    /* Post-impact: Settle 450 ms */
    t += 450;

    /* Intermittent movement preventing 1.5s stillness until 4.5s elapses (> 4.4s timeout) */
    for (int i = 0; i < 45; i++) {
        t += 100;
        /* Slight movement that fails stillness (g_mag = 50 dps > 40 dps, dyn_a = 0.4g > 0.3g)
         * but is NOT vigorous enough for immediate recovery (dyn_a < 0.65g, g_mag < 120 dps) */
        fall_detect_update(&det, t, 0.7f, 0.7f, 0.8f, 30.0f, 30.0f, 25.0f);
    }

    /* Should stand down to MONITORING */
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fall_detect_is_confirmed(&det) == false);
    assert(fall_detect_get_impact_accel(&det) == 0.0f);
    assert(fall_detect_get_impact_gyro(&det) == 0.0f);
    assert(fall_detect_get_posture_angle_deg(&det) == 0.0f);

    printf("  [PASS] Inactivity timeout returns to MONITORING and clears metrics.\n");
}

static void test_consecutive_falls_independent_lifecycle(void) {
    printf("Running test_consecutive_falls_independent_lifecycle...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 20000;
    /* Baseline standing */
    for (int i = 0; i < 20; i++) {
        t += 25;
        fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    }

    /* Fall 1: Free fall -> Impact -> 1.5s Stillness horizontal (Ax~1.0g) */
    t += 50;
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.3f, 30.0f, 40.0f, 20.0f);
    t += 150;
    fall_detect_update(&det, t, 1.8f, 2.2f, 2.5f, 180.0f, 110.0f, 70.0f);
    t += 450;
    for (int i = 0; i < 16; i++) {
        t += 100;
        fall_detect_update(&det, t, 0.98f, 0.05f, 0.08f, 1.0f, 2.0f, 1.0f);
    }
    assert(fall_detect_get_state(&det) == FALL_STATE_CONFIRMED);
    assert(fall_detect_is_confirmed(&det) == true);
    float angle1 = fall_detect_get_posture_angle_deg(&det);
    assert(angle1 > 75.0f && angle1 < 95.0f);

    /* Patient receives help and clears alarm */
    fall_detect_clear_alarm(&det);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(fall_detect_is_confirmed(&det) == false);
    assert(fall_detect_get_posture_angle_deg(&det) == 0.0f);
    assert(fall_detect_get_impact_accel(&det) == 0.0f);

    /* Subject walks upright for 2 seconds (re-establishes vertical Az~1.0g) */
    for (int i = 0; i < 40; i++) {
        t += 50;
        fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 5.0f, 5.0f, 5.0f);
    }

    /* Fall 2: Free fall -> Impact -> 1.5s Stillness on side (Ay~1.0g) */
    t += 50;
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.2f, 30.0f, 40.0f, 20.0f);
    t += 150;
    fall_detect_update(&det, t, 2.0f, 1.5f, 2.0f, 170.0f, 90.0f, 80.0f);
    t += 450;
    for (int i = 0; i < 16; i++) {
        t += 100;
        fall_detect_update(&det, t, 0.05f, 0.98f, 0.08f, 1.0f, 1.0f, 2.0f);
    }
    assert(fall_detect_get_state(&det) == FALL_STATE_CONFIRMED);
    assert(fall_detect_is_confirmed(&det) == true);
    float angle2 = fall_detect_get_posture_angle_deg(&det);
    assert(angle2 > 75.0f && angle2 < 95.0f);

    fall_detect_clear_alarm(&det);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);

    printf("  [PASS] Consecutive falls operate with independent lifecycles.\n");
}

static void test_bed_rest_does_not_drift_baseline(void) {
    printf("Running test_bed_rest_does_not_drift_baseline...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 1000;
    for (int i = 0; i < 80; i++) { t += 25; fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0, 0, 0); }

    /* 10 minutes lying still on the side (gravity on X) */
    for (int i = 0; i < 24000; i++) { t += 25; fall_detect_update(&det, t, 1.0f, 0.0f, 0.0f, 0, 0, 0); }
    assert(det.baseline_az > 0.8f);
    assert(det.baseline_ax < 0.2f);

    printf("  [PASS] Baseline stays upright through prolonged bed rest.\n");
}

static void test_soft_path_cleared_on_all_exits(void) {
    printf("Running test_soft_path_cleared_on_all_exits...\n");
    fall_detector_t det;
    fall_detect_init(&det, NULL);

    uint32_t t = 1000;
    /* Establish upright baseline */
    for (int i = 0; i < 80; i++) { t += 25; fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0, 0, 0); }

    /* 1. Enter soft path, then vigorous recovery */
    t += 25;
    fall_detect_update(&det, t, 1.4f, 1.4f, 0.0f, 20.0f, 20.0f, 20.0f);
    assert(det.soft_path == true);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);

    t += 500;
    fall_detect_update(&det, t, 0.8f, 0.6f, 1.5f, 150.0f, 90.0f, 60.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(det.soft_path == false);

    /* 2. Enter soft path, then stillness timeout (lack of stillness within 4.4s) */
    t += 1000;
    fall_detect_update(&det, t, 1.4f, 1.4f, 0.0f, 20.0f, 20.0f, 20.0f);
    assert(det.soft_path == true);
    /* Intermittent micro-movement exceeding stillness tolerance */
    for (int i = 0; i < 200; i++) {
        t += 25;
        fall_detect_update(&det, t, 0.5f, 0.5f, 0.5f, 30.0f, 30.0f, 30.0f);
    }
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(det.soft_path == false);

    /* 3. Enter soft path, confirm fall with posture change, then 60s auto-standby */
    t += 1000;
    fall_detect_update(&det, t, 1.4f, 1.4f, 0.0f, 20.0f, 20.0f, 20.0f);
    assert(det.soft_path == true);
    for (int i = 0; i < 160; i++) {
        t += 25;
        fall_detect_update(&det, t, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    assert(fall_detect_is_confirmed(&det) == true);
    assert(det.soft_path == false);

    /* Advance 65s for auto-standby */
    t += 65000;
    fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_MONITORING);
    assert(det.soft_path == false);

    /* 4. Subsequent hard fall without posture change (e.g. falling into upright wedged position)
     * must confirm cleanly because soft_path is NOT set! */
    t += 1000;
    /* Free-fall */
    fall_detect_update(&det, t, 0.1f, 0.1f, 0.1f, 10.0f, 10.0f, 10.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_FREE_FALL);
    assert(det.soft_path == false);

    /* Hard impact */
    t += 100;
    fall_detect_update(&det, t, 2.5f, 2.0f, 1.0f, 120.0f, 90.0f, 60.0f);
    assert(fall_detect_get_state(&det) == FALL_STATE_INACTIVITY);
    assert(det.soft_path == false);

    /* Stillness without posture angle change */
    bool hard_confirmed = false;
    for (int i = 0; i < 160; i++) {
        t += 25;
        if (fall_detect_update(&det, t, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f)) {
            hard_confirmed = true;
        }
    }
    assert(hard_confirmed == true);
    assert(fall_detect_is_confirmed(&det) == true);
    assert(det.soft_path == false);

    printf("  [PASS] soft_path cleared on all exits; subsequent hard fall not posture-gated.\n");
}

int main(void) {
    printf("================================================================\n");
    printf("  SIH26181 Biomechanical Fall Detection FSM Unit Test Suite\n");
    printf("================================================================\n\n");

    test_default_config_and_init();
    test_free_fall_timeout_returns_to_monitoring();
    test_impact_without_free_fall_rejected();
    test_impact_gyro_threshold_gating();
    test_rebound_settling_window();
    test_vigorous_recovery_rejects_false_alarm();
    test_continuous_stillness_reset_on_motion();
    test_complete_biomechanical_fall_and_posture();
    test_auto_standby_timeout();
    test_inactivity_timeout_clears_metrics();
    test_consecutive_falls_independent_lifecycle();
    test_soft_fall_slump_confirmed();
    test_soft_fall_disabled();
    test_bed_rest_does_not_drift_baseline();
    test_soft_path_cleared_on_all_exits();
    test_null_pointer_robustness();

    printf("\n>>> ALL 16 FALL DETECTION FSM UNIT TESTS PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
