#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disaster_risk_engine.h"

int main(void) {
    risk_assessment_t res;
    hrv_state_t hrv;

    /* Test 1: Normal conditions */
    hrv.rmssd = 50.0f;
    env_sensors_t env = {25.0f, 40.0f, 10.0f, 33.0f};
    disaster_assess(&hrv, 98.0f, 70.0f, &env, &res);
    if (res.overall_risk != RISK_NORMAL) {
        fprintf(stderr, "Test1 failed: expected NORMAL, got %d\n", res.overall_risk);
        return 1;
    }

    /* Test 2: Heat critical */
    env.ambient_temp_c = 55.0f;
    env.humidity_pct = 60.0f;
    disaster_assess(&hrv, 98.0f, 140.0f, &env, &res);
    if (res.heat_risk != RISK_CRITICAL) {
        fprintf(stderr, "Test2 failed: expected HEAT CRITICAL, got %d\n", res.heat_risk);
        return 2;
    }

    /* Test 3: Pollution high */
    env.ambient_temp_c = 25.0f; env.humidity_pct = 50.0f;
    env.pm25 = 200.0f;
    disaster_assess(&hrv, 90.0f, 80.0f, &env, &res);
    if (res.pollution_risk != RISK_HIGH) {
        fprintf(stderr, "Test3 failed: expected POLLUTION HIGH, got %d\n", res.pollution_risk);
        return 3;
    }

    printf("All tests passed\n");
    return 0;
}
