#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "disaster_risk_engine.h"

/* Helper: Initialize HRV with synthetic data to make it "ready" */
static void init_hrv_ready(hrv_state_t *hrv, float bpm) {
    hrv_init(hrv);
    float ibi_ms = 60000.0f / bpm;
    for (int i = 0; i < HRV_MIN_SAMPLES; i++) {
        /* Add jitter to achieve realistic RMSSD ~20-50ms */
        float jitter = (float)((i % 11) - 5) * 10.0f;  /* +/- 50ms pseudo-jitter */
        hrv_add_ibi(hrv, ibi_ms + jitter);
    }
    hrv_compute(hrv);
}

static void test_heat_risk() {
    hrv_state_t hrv;
    init_hrv_ready(&hrv, 75.0f);  // Normal HRV
    env_sensors_t env = { .ambient_temp_c = 25.0f, .humidity_pct = 30.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    /* Test Normal */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    printf("  Heat test 1: heat_risk=%d (expected %d), RMSSD=%.1f\n", result.heat_risk, RISK_NORMAL, hrv.rmssd);
    fflush(stdout);
    assert(result.heat_risk == RISK_NORMAL);

    /* Test Critical Heat Stroke */
    init_hrv_ready(&hrv, 135.0f);
    env.ambient_temp_c = 46.0f;
    env.humidity_pct = 70.0f;
    /* Manually set low RMSSD for critical test */
    hrv.rmssd = 8.0f;
    disaster_assess(&hrv, 98.0f, 135.0f, &env, &result);
    assert(result.heat_risk == RISK_CRITICAL);

    printf("test_heat_risk: PASS\n");
}

static void test_pollution_risk() {
    hrv_state_t hrv;
    init_hrv_ready(&hrv, 75.0f);
    env_sensors_t env = { .ambient_temp_c = 25.0f, .humidity_pct = 30.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    /* Test Normal */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.pollution_risk == RISK_NORMAL);

    /* Test Severe Smog */
    init_hrv_ready(&hrv, 125.0f);
    env.pm25 = 350.0f;
    hrv.rmssd = 10.0f;  /* Low HRV for pollution stress */
    disaster_assess(&hrv, 85.0f, 125.0f, &env, &result);
    assert(result.pollution_risk == RISK_CRITICAL);

    printf("test_pollution_risk: PASS\n");
}

static void test_flood_risk() {
    hrv_state_t hrv;
    init_hrv_ready(&hrv, 75.0f);
    env_sensors_t env = { .ambient_temp_c = 15.0f, .humidity_pct = 80.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    /* Test Normal */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.flood_risk == RISK_NORMAL);

    /* Test Hypothermia */
    init_hrv_ready(&hrv, 45.0f);  /* Bradycardia */
    env.skin_temp_c = 25.0f;
    hrv.rmssd = 5.0f;  /* Low HRV for autonomic collapse */
    disaster_assess(&hrv, 98.0f, 45.0f, &env, &result);
    assert(result.flood_risk == RISK_CRITICAL);

    printf("test_flood_risk: PASS\n");
}

/* Test RISK_UNKNOWN when HRV not ready */
static void test_hrv_not_ready() {
    hrv_state_t hrv;
    hrv_init(&hrv);  /* Only 0 samples - not ready */
    env_sensors_t env = { .ambient_temp_c = 25.0f, .humidity_pct = 30.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.heat_risk == RISK_UNKNOWN);
    assert(result.pollution_risk == RISK_UNKNOWN);
    assert(result.flood_risk == RISK_UNKNOWN);
    assert(result.overall_risk == RISK_UNKNOWN);

    printf("test_hrv_not_ready: PASS\n");
}

/* Test RISK_UNKNOWN when skin_temp missing */
static void test_flood_unknown_skin_temp() {
    hrv_state_t hrv;
    init_hrv_ready(&hrv, 75.0f);
    env_sensors_t env = { .ambient_temp_c = 15.0f, .humidity_pct = 80.0f, .pm25 = 10.0f, .skin_temp_c = 0.0f };
    risk_assessment_t result;

    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.flood_risk == RISK_UNKNOWN);

    printf("test_flood_unknown_skin_temp: PASS\n");
}

int main() {
    printf("Running unit tests for disaster_risk_engine...\n");
    test_heat_risk();
    test_pollution_risk();
    test_flood_risk();
    test_hrv_not_ready();
    test_flood_unknown_skin_temp();
    printf("ALL TESTS PASSED.\n");
    return 0;
}