#include <stdio.h>
#include "hrv_analysis.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model.h"

typedef struct {
    const char *name;
    float hr, spo2, rmssd, temp, hum, pm25, skin_temp;
} scenario_t;

int main(void) {
    /* End-of-drift steady-state values, taken directly from main_simulation.c's
       four built-in scenarios (base + drift). skin_temp is the dedicated sensor
       channel the rule engine's flood assessment actually uses -- NOT the same
       as ambient temp. */
    scenario_t scenarios[4] = {
        {"Normal Resting",         72.0f, 98.0f, 45.0f, 25.0f, 45.0f,  15.0f, 36.5f},
        {"Heat Wave (Delhi 47C)", 140.0f, 95.0f,  8.0f, 50.0f, 65.0f,  25.0f, 39.0f},
        {"Severe Smog (AQI500+)", 123.0f, 86.0f, 12.0f, 12.0f, 85.0f, 400.0f, 33.0f},
        {"Flash Flood/Hypothermia",140.0f, 93.0f,  6.0f,  6.0f, 98.0f,  20.0f, 24.5f},
    };

    const nn_model_t *model = nn_get_default_model();

    printf("%-25s | %-9s %-9s %-9s | %-9s %-9s %-9s\n",
           "Scenario", "RuleHeat", "RulePoll", "RuleFlood", "NN-Heat", "NN-Poll", "NN-Flood");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < 4; i++) {
        scenario_t *s = &scenarios[i];

        hrv_state_t hrv;
        hrv_init(&hrv);
        hrv.rmssd = s->rmssd;  /* inject directly for this steady-state snapshot */

        env_sensors_t env = {0};
        env.ambient_temp_c = s->temp;
        env.humidity_pct   = s->hum;
        env.pm25           = s->pm25;
        env.skin_temp_c    = s->skin_temp;

        risk_assessment_t rule_result;
        disaster_assess(&hrv, s->spo2, s->hr, &env, &rule_result);

        nn_output_t nn_out;
        nn_predict(model, s->hr, s->rmssd, s->spo2, s->temp, s->hum, s->pm25, &nn_out);

        printf("%-25s | %-9s %-9s %-9s | %-9.3f %-9.3f %-9.3f\n",
               s->name,
               risk_level_to_string(rule_result.heat_risk),
               risk_level_to_string(rule_result.pollution_risk),
               risk_level_to_string(rule_result.flood_risk),
               nn_out.heat_score, nn_out.pollution_score, nn_out.flood_score);
    }
    return 0;
}
