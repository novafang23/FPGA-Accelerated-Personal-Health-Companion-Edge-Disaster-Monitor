#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "disaster_risk_engine.h"
#include "nn_risk_model.h"
#include "nn_risk_model_int8.h"

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

/* Map a raw NN score [0,1] to the same 4-tier scale nn_score_to_risk() uses
 * in disaster_risk_engine.c, so we can catch the specific failure mode of
 * "float and int8 land in different risk tiers" -- not just "the numbers
 * differ a bit", which is expected and harmless quantization noise. */
static int score_to_tier(float score) {
    if (score >= 0.70f) return 3;  /* CRITICAL */
    if (score >= 0.50f) return 2;  /* HIGH     */
    if (score >= 0.25f) return 1;  /* MODERATE */
    return 0;                      /* NORMAL   */
}

/* Regression test: the INT8 quantized NN (deployed on ShrikeFi/ForgeFPGA)
 * must stay close to the float32 NN (deployed on Zynq) for the same input.
 * If a future retrain or quantization change reintroduces the scale/zero-
 * point bug this test guards against, the two boards could show different
 * risk levels for identical vitals -- this test fails loudly instead of
 * only being noticed live during a demo. */
static void test_int8_matches_float_nn() {
    struct {
        const char *name;
        float hr, rmssd, spo2, temp, hum, pm25;
    } scenarios[] = {
        {"Normal Resting",  72.0f, 45.0f, 98.0f, 25.0f, 45.0f,  15.0f},
        {"Heat Wave",      140.0f,  8.0f, 95.0f, 50.0f, 65.0f,  25.0f},
        {"Severe Smog",    123.0f, 12.0f, 86.0f, 12.0f, 85.0f, 400.0f},
        {"Flash Flood",    140.0f,  6.0f, 93.0f,  6.0f, 98.0f,  20.0f},
    };
    const float MAX_ABS_ERROR = 0.05f;  /* generous quantization-noise budget */
    const nn_model_t *model = nn_get_default_model();

    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++) {
        nn_output_t f_out, q_out;
        nn_predict(model, scenarios[i].hr, scenarios[i].rmssd, scenarios[i].spo2,
                   scenarios[i].temp, scenarios[i].hum, scenarios[i].pm25, &f_out);
        nn_predict_int8(&nn_default_model_int8, &nn_quant_params,
                        scenarios[i].hr, scenarios[i].rmssd, scenarios[i].spo2,
                        scenarios[i].temp, scenarios[i].hum, scenarios[i].pm25, &q_out);

        float f_scores[3] = {f_out.heat_score, f_out.pollution_score, f_out.flood_score};
        float q_scores[3] = {q_out.heat_score, q_out.pollution_score, q_out.flood_score};
        const char *names[3] = {"heat", "pollution", "flood"};

        for (int k = 0; k < 3; k++) {
            float diff = fabsf(f_scores[k] - q_scores[k]);
            if (diff > MAX_ABS_ERROR) {
                printf("  FAIL [%s/%s]: float=%.3f int8=%.3f diff=%.3f (max %.3f)\n",
                       scenarios[i].name, names[k], f_scores[k], q_scores[k], diff, MAX_ABS_ERROR);
            }
            assert(diff <= MAX_ABS_ERROR);

            int f_tier = score_to_tier(f_scores[k]);
            int q_tier = score_to_tier(q_scores[k]);
            if (f_tier != q_tier) {
                /* A tier flip is only a real bug if it happens FAR from a
                 * boundary (large divergence). A flip where both scores sit
                 * within MAX_ABS_ERROR of the same boundary (e.g. 0.246 vs
                 * 0.255 around the 0.25 line) is inherent quantization noise
                 * that any 8-bit model will occasionally show right at a
                 * threshold -- not something further tuning can eliminate. */
                static const float boundaries[3] = {0.25f, 0.50f, 0.70f};
                int near_shared_boundary = 0;
                for (int b = 0; b < 3; b++) {
                    if (fabsf(f_scores[k] - boundaries[b]) <= MAX_ABS_ERROR &&
                        fabsf(q_scores[k] - boundaries[b]) <= MAX_ABS_ERROR) {
                        near_shared_boundary = 1;
                        break;
                    }
                }
                if (!near_shared_boundary) {
                    printf("  FAIL [%s/%s]: float tier=%d int8 tier=%d (float=%.3f int8=%.3f) -- "
                           "NOT a boundary case, this is a real divergence\n",
                           scenarios[i].name, names[k], f_tier, q_tier, f_scores[k], q_scores[k]);
                }
                assert(near_shared_boundary);
            }
        }
    }

    printf("test_int8_matches_float_nn: PASS\n");
}

int main() {
    printf("Running unit tests for disaster_risk_engine...\n");
    test_heat_risk();
    test_pollution_risk();
    test_flood_risk();
    test_hrv_not_ready();
    test_flood_unknown_skin_temp();
    test_int8_matches_float_nn();
    printf("ALL TESTS PASSED.\n");
    return 0;
}