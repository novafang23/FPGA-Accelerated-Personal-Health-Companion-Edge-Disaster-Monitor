#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hrv_analysis.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model.h"

typedef struct {
    const char *name;
    float hr, spo2, rmssd_target, temp, hum, pm25, skin_temp;
} scenario_t;

int main(void) {
    srand((unsigned int)time(NULL));
    
    scenario_t scenarios[4] = {
        {"Normal Resting",         72.0f, 98.0f, 45.0f, 25.0f, 45.0f,  15.0f, 36.5f},
        {"Heat Wave (Delhi 47C)", 140.0f, 95.0f,  8.0f, 50.0f, 65.0f,  25.0f, 39.0f},
        {"Severe Smog (AQI500+)", 123.0f, 86.0f, 12.0f, 12.0f, 85.0f, 400.0f, 33.0f},
        {"Flash Flood/Hypothermia",140.0f, 93.0f,  6.0f,  6.0f, 98.0f,  20.0f, 24.5f},
    };

    const nn_model_t *model = nn_get_default_model();
    if (model == NULL) {
        fprintf(stderr, "Failed to load NN model\n");
        return 1;
    }

    printf("%-25s | %-9s %-9s %-9s | %-9s %-9s %-9s\n",
           "Scenario", "RuleHeat", "RulePoll", "RuleFlood", "NN-Heat", "NN-Poll", "NN-Flood");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < 4; i++) {
        scenario_t *s = &scenarios[i];

        hrv_state_t hrv;
        hrv_init(&hrv);

        /* Jitter amplitude is derived from s->rmssd_target so the seeded
         * hrv.rmssd actually lands near the scenario's intended value
         * instead of a fixed pattern that ignores rmssd_target entirely
         * (previously every scenario produced a similar RMSSD regardless
         * of the authored target -- see derivation in main_simulation.c). */
        float ibi_ms = 60000.0f / s->hr;
        float jitter_amplitude = s->rmssd_target * 1.224745f; /* sqrt(1.5) */
        for (int j = 0; j < HRV_MIN_SAMPLES; j++) {
            float jitter = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * jitter_amplitude;
            hrv_add_ibi(&hrv, ibi_ms + jitter);
        }
        hrv_compute(&hrv);

        env_sensors_t env = {0};
        env.ambient_temp_c = s->temp;
        env.humidity_pct   = s->hum;
        env.pm25           = s->pm25;
        env.skin_temp_c    = s->skin_temp;

        risk_assessment_t rule_result;
        disaster_assess(&hrv, s->spo2, s->hr, &env, &rule_result);

        nn_output_t nn_out;
        nn_predict(model, s->hr, hrv.rmssd, s->spo2, s->temp, s->hum, s->pm25, &nn_out);

        printf("%-25s | %-9s %-9s %-9s | %-9.3f %-9.3f %-9.3f\n",
               s->name,
               risk_level_to_string(rule_result.heat_risk),
               risk_level_to_string(rule_result.pollution_risk),
               risk_level_to_string(rule_result.flood_risk),
               nn_out.heat_score, nn_out.pollution_score, nn_out.flood_score);
    }
    return 0;
}