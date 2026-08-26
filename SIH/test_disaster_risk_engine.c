#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "disaster_risk_engine.h"

static void test_heat_risk() {
    hrv_state_t hrv = { .rmssd = 50.0f }; // Normal HRV
    env_sensors_t env = { .ambient_temp_c = 25.0f, .humidity_pct = 30.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    /* Test Normal */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.heat_risk == RISK_NORMAL);

    /* Test Critical Heat Stroke */
    env.ambient_temp_c = 46.0f;
    env.humidity_pct = 70.0f;
    hrv.rmssd = 8.0f; /* Critical */
    disaster_assess(&hrv, 98.0f, 135.0f /* High BPM */, &env, &result);
    assert(result.heat_risk == RISK_CRITICAL);

    printf("test_heat_risk: PASS\n");
}

static void test_pollution_risk() {
    hrv_state_t hrv = { .rmssd = 50.0f };
    env_sensors_t env = { .ambient_temp_c = 25.0f, .humidity_pct = 30.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    /* Test Normal */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.pollution_risk == RISK_NORMAL);

    /* Test Severe Smog */
    env.pm25 = 350.0f;
    disaster_assess(&hrv, 85.0f /* Critical SpO2 */, 125.0f /* Elevated BPM */, &env, &result);
    assert(result.pollution_risk == RISK_CRITICAL);

    printf("test_pollution_risk: PASS\n");
}

static void test_flood_risk() {
    hrv_state_t hrv = { .rmssd = 50.0f };
    env_sensors_t env = { .ambient_temp_c = 15.0f, .humidity_pct = 80.0f, .pm25 = 10.0f, .skin_temp_c = 36.0f };
    risk_assessment_t result;

    /* Test Normal */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.flood_risk == RISK_NORMAL);

    /* Test Hypothermia */
    env.skin_temp_c = 25.0f;
    disaster_assess(&hrv, 98.0f, 45.0f /* Bradycardia */, &env, &result);
    assert(result.flood_risk == RISK_CRITICAL);

    printf("test_flood_risk: PASS\n");
}

int main() {
    printf("Running unit tests for disaster_risk_engine...\n");
    test_heat_risk();
    test_pollution_risk();
    test_flood_risk();
    printf("ALL TESTS PASSED.\n");
    return 0;
}
