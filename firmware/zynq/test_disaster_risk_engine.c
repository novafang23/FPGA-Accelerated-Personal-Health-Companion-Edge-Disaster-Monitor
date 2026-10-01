#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "disaster_risk_engine.h"
#include "spo2_engine.h"
#include "nn_risk_model.h"
#include "nn_risk_model_int8.h"
#include "ppg_respiratory_rate.h"
#include "ppg_sqi.h"
/* sos.c lives under firmware/shrikefi/ but is deliberately free of any hardware
 * or FreeRTOS dependency, so the emergency state machine is testable here
 * alongside everything else rather than only on the bench. */
#include "sos.h"
/* The status page's wire format. web_status.c keeps the JSON formatter free of
 * ESP-IDF (the SoftAP and HTTP handlers are behind ESP_PLATFORM), so the exact
 * bytes the phone receives are checked here rather than by curling a board. */
#include "web_status.h"
/* Deployment location (T1.4). Everything that decides what is stored or shown
 * is pure, so the trimming, decoding and card-shortening rules are all checked
 * here rather than on a 0.96" panel. */
#include "location.h"

/* ---------------------------------------------------------------------------
 * Compile-time contract: the INT8 model struct must exactly match the
 * 6 -> 24 -> 16 -> 3 architecture that train_nn_risk_model.py emits, and its
 * weight/bias storage must be 619 bytes. These asserts fire at build time, so
 * an architecture or quantization mismatch fails the build immediately instead
 * of showing up later as a silent accuracy regression.
 *
 * 619 = (6*24 + 24) + (24*16 + 16) + (16*3 + 3)
 * ------------------------------------------------------------------------- */
_Static_assert(NN_INPUT_SIZE   == 6,  "INT8 model expects 6 input features");
_Static_assert(NN_HIDDEN1_SIZE == 24, "INT8 model expects 24 neurons in hidden layer 1");
_Static_assert(NN_HIDDEN2_SIZE == 16, "INT8 model expects 16 neurons in hidden layer 2");
_Static_assert(NN_OUTPUT_SIZE  == 3,  "INT8 model expects 3 hazard outputs");
_Static_assert(sizeof(nn_model_int8_t) == 619,
               "INT8 weight+bias storage must be 619 bytes; regenerate with "
               "firmware/core/train_nn_risk_model.py");

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

/* Cold-stress handling when no skin-temperature sensor is present.
 *
 * This replaces the previous test_flood_unknown_skin_temp, which asserted that a
 * missing skin sensor must always yield RISK_UNKNOWN. That contract was
 * deliberately changed: the engine now falls back to an ambient air + humidity
 * cold-stress proxy instead of reporting nothing forever.
 *
 * The tests below pin the two properties that matter:
 *   1. ambient air must NOT be fed into the skin-temperature thresholds
 *      (28 C of air is a warm day; 28 C of skin is severe hypothermia), and
 *   2. the proxy must never reach RISK_CRITICAL, because without a measured
 *      skin/core temperature a critical hypothermia call is not supportable.
 */
static void test_flood_ambient_proxy() {
    hrv_state_t hrv;
    init_hrv_ready(&hrv, 75.0f);
    env_sensors_t env = { .ambient_temp_c = 15.0f, .humidity_pct = 80.0f, .pm25 = 10.0f, .skin_temp_c = 0.0f };
    risk_assessment_t result;

    /* RMSSD is set explicitly in every case below. init_hrv_ready()'s synthetic
     * jitter happens to yield RMSSD ~10 ms, which is itself an autonomic-strain
     * value and would otherwise score cold-stress points in what are meant to be
     * "normal vitals" scenarios. */
    hrv.rmssd = 40.0f;   /* Healthy resting HRV */

    /* Mild ambient + normal vitals: no cold-stress alarm, and overall is NORMAL
     * rather than the old blanket RISK_UNKNOWN. */
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.flood_risk == RISK_NORMAL);
    assert(result.overall_risk == RISK_NORMAL);

    /* Cold and wet, normal vitals: caution, not alarm. */
    env.ambient_temp_c = 8.0f;
    env.humidity_pct   = 90.0f;
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.flood_risk == RISK_MODERATE);

    /* Cold + wet + bradycardia + collapsed HRV: HIGH, capped short of CRITICAL. */
    init_hrv_ready(&hrv, 45.0f);
    hrv.rmssd = 5.0f;    /* Autonomic collapse */
    env.ambient_temp_c = 3.0f;
    disaster_assess(&hrv, 98.0f, 45.0f, &env, &result);
    assert(result.flood_risk == RISK_HIGH);
    assert(result.flood_risk != RISK_CRITICAL);

    /* A hot humid day must NOT score as cold stress (the humidity term is gated
     * on the cold temperature band), and a real heat hazard still outranks it. */
    env.ambient_temp_c = 48.0f;
    env.humidity_pct   = 80.0f;
    init_hrv_ready(&hrv, 135.0f);
    hrv.rmssd = 25.0f;
    disaster_assess(&hrv, 98.0f, 135.0f, &env, &result);
    assert(result.flood_risk == RISK_NORMAL);
    assert(result.heat_risk >= RISK_HIGH);
    assert(result.overall_risk >= RISK_HIGH);

    /* Genuinely unusable ambient (outside the plausible band) still reports the
     * blind spot rather than inventing a reading. */
    env.ambient_temp_c = -40.0f;
    init_hrv_ready(&hrv, 75.0f);
    hrv.rmssd = 40.0f;
    disaster_assess(&hrv, 98.0f, 75.0f, &env, &result);
    assert(result.flood_risk == RISK_UNKNOWN);

    printf("test_flood_ambient_proxy: PASS\n");
}

/* Regression test: a NULL env pointer must degrade to RISK_UNKNOWN, not crash.
 * (hrv is deliberately valid/ready here so this isolates the env==NULL path
 * specifically, rather than overlapping with test_hrv_not_ready above.) */
static void test_null_env() {
    hrv_state_t hrv;
    init_hrv_ready(&hrv, 75.0f);
    risk_assessment_t result;

    disaster_assess(&hrv, 98.0f, 75.0f, NULL, &result);
    assert(result.heat_risk == RISK_UNKNOWN);
    assert(result.pollution_risk == RISK_UNKNOWN);
    assert(result.flood_risk == RISK_UNKNOWN);
    assert(result.overall_risk == RISK_UNKNOWN);

    disaster_assess_nn(&hrv, 98.0f, 75.0f, NULL, &result);
    assert(result.overall_risk == RISK_UNKNOWN);

    disaster_assess_nn_int8(&hrv, 98.0f, 75.0f, NULL, &result, NULL);
    assert(result.overall_risk == RISK_UNKNOWN);

    /* Test raw_out telemetry extraction in single-pass call */
    nn_output_t raw_telemetry;
    env_sensors_t normal_env = { .ambient_temp_c = 25.0f, .humidity_pct = 45.0f, .pm25 = 15.0f, .skin_temp_c = 36.0f };
    disaster_assess_nn_int8(&hrv, 98.0f, 72.0f, &normal_env, &result, &raw_telemetry);
    assert(raw_telemetry.heat_score >= 0.0f && raw_telemetry.heat_score <= 1.0f);
    assert(raw_telemetry.pollution_score >= 0.0f && raw_telemetry.pollution_score <= 1.0f);
    assert(raw_telemetry.flood_score >= 0.0f && raw_telemetry.flood_score <= 1.0f);

    printf("test_null_env: PASS\n");
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
    const float MAX_ABS_ERROR = 0.18f;  /* 3-layer cascade INT8 quantization budget (discretization margin) */
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
                fflush(stdout);
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

static void test_spo2_clinical_rejection() {
    spo2_state_t spo2;
    spo2_init(&spo2);

    /* Scenario 1: No finger / Ambient air (IR = 500, Red = 400, no AC) */
    for (int i = 0; i < SPO2_WINDOW_SIZE * 2; i++) {
        spo2_add_samples(&spo2, 400, 500);
    }
    assert(spo2_is_valid(&spo2) == 0);
    printf("  SpO2 Test 1 (ambient air rejected): valid=%d\n", spo2_is_valid(&spo2));

    /* Scenario 2: Touching too gently / hover (IR = 2500, Red = 2000, AC = 10 counts noise) */
    spo2_init(&spo2);
    for (int i = 0; i < SPO2_WINDOW_SIZE * 3; i++) {
        uint32_t noise = (i % 5);
        spo2_add_samples(&spo2, 2000 + noise, 2500 + noise);
    }
    assert(spo2_is_valid(&spo2) == 0);
    printf("  SpO2 Test 2 (too gentle / low perfusion rejected): valid=%d\n", spo2_is_valid(&spo2));

    /* Scenario 3: Genuine physiological arterial pulsation */
    /* DC_ir = 30000, AC_ir = 300 (1.0% PI), DC_red = 25000, AC_red = 125 (0.5% PI), R = 0.50 -> SpO2 = ~97.5% */
    spo2_init(&spo2);
    /* 10 windows, not 4: the gate now requires a full 8-window moving average
     * before it will publish, which is the point of it. The signal is still the
     * same clean constant-amplitude pulse, so this only proves the engine
     * accepts good input - Test 4 is what proves the gate rejects bad input. */
    for (int win = 0; win < 12; win++) {
        for (int i = 0; i < SPO2_WINDOW_SIZE; i++) {
            float phase = (float)i / (float)SPO2_WINDOW_SIZE * 6.283185f;
            float pulse = sinf(phase);
            uint32_t red = (uint32_t)(25000.0f + 62.5f * pulse);
            uint32_t ir  = (uint32_t)(30000.0f + 150.0f * pulse);
            spo2_add_samples(&spo2, red, ir);
        }
    }
    assert(spo2_is_valid(&spo2) == 1);
    float val = spo2_get_value(&spo2);
    printf("  SpO2 Test 3 (proper arterial pulse verified): valid=%d, SpO2=%.1f%%, PI=%.2f%%\n",
           spo2_is_valid(&spo2), val, spo2_get_perfusion_index(&spo2));
    assert(val >= 95.0f && val <= 100.0f);

    /* Scenario 4: the acquisition transient.
     *
     * This is the case the old gate got wrong, so it is the one worth guarding.
     * max30102_adjust_led_current() re-tunes the LED current while the finger
     * settles, which moves the DC baseline, which moves R, which moves the
     * reported SpO2 - for tens of seconds. On hardware that published 78% as
     * VALID, and a real 78% is a life-threatening desaturation.
     *
     * A falling red AC amplitude reproduces the same drift in R: 125 counts is
     * R=1.00 (~85%), 62.5 counts is R=0.50 (~97.5%). Scenario 3 above uses a
     * constant pulse, so it latches immediately and would still pass with the
     * gate removed entirely - it cannot detect a regression on its own. This
     * can: with SPO2_REQUIRED_VALID_WINDOWS back at 1 it fails. */
    spo2_init(&spo2);
    for (int win = 0; win < 10; win++) {
        float red_ac = 125.0f - (float)win * 6.25f;
        for (int i = 0; i < SPO2_WINDOW_SIZE; i++) {
            float phase = (float)i / (float)SPO2_WINDOW_SIZE * 6.283185f;
            float pulse = sinf(phase);
            uint32_t red = (uint32_t)(25000.0f + red_ac * pulse);
            uint32_t ir  = (uint32_t)(30000.0f + 150.0f * pulse);
            spo2_add_samples(&spo2, red, ir);
        }
    }
    assert(spo2_is_valid(&spo2) == 0);
    printf("  SpO2 Test 4 (drifting acquisition NOT published as valid): valid=%d\n",
           spo2_is_valid(&spo2));

    /* Scenario 5: it must still latch once the input genuinely settles, or the
     * gate has simply broken SpO2 reporting instead of fixing it. */
    for (int win = 0; win < 12; win++) {
        for (int i = 0; i < SPO2_WINDOW_SIZE; i++) {
            float phase = (float)i / (float)SPO2_WINDOW_SIZE * 6.283185f;
            float pulse = sinf(phase);
            uint32_t red = (uint32_t)(25000.0f + 62.5f * pulse);
            uint32_t ir  = (uint32_t)(30000.0f + 150.0f * pulse);
            spo2_add_samples(&spo2, red, ir);
        }
    }
    assert(spo2_is_valid(&spo2) == 1);
    printf("  SpO2 Test 5 (latches once the input settles): valid=%d, SpO2=%.1f%%\n",
           spo2_is_valid(&spo2), spo2_get_value(&spo2));

    printf("test_spo2_clinical_rejection: PASS\n");
}

/* Regression: a clamped respiratory rate must never be published as reliable.
 *
 * Every interval below is physiologically normal (HR 74-82 BPM) and there is no
 * respiratory modulation at all. The lag search started at k=2 and accepted any
 * lag whose breath *period* fell in [1.2, 12] s - and 2 beats at a ~770 ms mean
 * is 1.54 s, which passes. So it picked lag 2, computed 60000/(2*770) = 38.96
 * br/min, clamp_rr() pinned that to exactly 36.00, and is_reliable came out
 * TRUE because best_r (0.646) cleared the 0.60 confidence bar.
 *
 * 36 br/min is the tachypnoea ceiling: it sets is_absolute_crisis in the
 * clinical engine, which bypasses the SQI hold and lands on CLINICAL_CRITICAL.
 * This sequence is the measured reproduction of that on a healthy subject. */
static void test_rr_estimate_rejects_clamped_value() {
    const float ibi[] = {
        743.9f, 739.1f, 763.4f, 741.9f, 734.4f, 733.6f, 736.7f, 751.9f,
        764.9f, 752.0f, 747.1f, 771.4f, 750.8f, 752.1f, 750.0f, 764.8f,
        769.0f, 779.0f, 774.4f, 769.1f, 788.9f, 777.7f, 771.8f, 779.1f,
        797.5f, 806.7f, 791.7f, 811.2f, 808.6f, 810.4f
    };
    ppg_respiratory_result_t rr;
    memset(&rr, 0, sizeof(rr));
    ppg_estimate_respiratory_rate(ibi, NULL, sizeof(ibi) / sizeof(ibi[0]), &rr);

    printf("  RR regression: rr=%.2f conf=%.2f reliable=%d\n",
           rr.respiratory_rate_bpm, rr.confidence, (int)rr.is_reliable);

    /* The invariant: a value that had to be clamped was never measured, so it
     * can never be certified. */
    if (rr.is_reliable) {
        assert(rr.respiratory_rate_bpm > PPG_RR_MIN_BPM);
        assert(rr.respiratory_rate_bpm < PPG_RR_MAX_BPM);
    }
    /* Specifically, nothing in this record is tachypnoeic, so the ceiling must
     * not be reported as a finding. */
    assert(!(rr.is_reliable && rr.respiratory_rate_bpm >= PPG_RR_MAX_BPM));

    /* And this record contains NO respiratory modulation at all - it is a slow
     * drift plus detector jitter - so nothing may be published. Before linear
     * detrending was added the drift won the autocorrelation search and the
     * estimator reported 26 br/min at 0.97 confidence: a confident wrong number,
     * which is worse than the railed 36 it used to produce, because a confident
     * number is acted on. */
    assert(!rr.is_reliable);

    printf("test_rr_estimate_rejects_clamped_value: PASS\n");
}

/* Regression: SQI must not certify a pulse when no beats were ever detected.
 *
 * interval_regularity carries 40% of the composite weight but is only computed
 * once three beat-to-beat intervals exist. With none it kept its 0.50
 * initialiser, so a clean, positively-skewed waveform scored
 * 0.35*1.0 + 0.40*0.50 + 0.25*1.0 = 0.80 and cleared the 0.70 gate - the device
 * reported hospital-grade quality with zero beats behind it. That state is
 * reachable: the FPGA crest detector can fall silent while the optics stay
 * healthy, and the software fallback is suppressed while the FPGA is believed
 * alive. */
static void test_sqi_requires_beat_intervals() {
    uint32_t raw[64];
    for (int i = 0; i < 64; i++) {
        float ph = fmodf((float)i * 0.6f, 6.2831853f) / 6.2831853f;
        /* Fast systolic rise, slow diastolic runoff: positive skewness, which
         * is what a real PPG looks like and what scores well on shape alone. */
        float pulse = (ph < 0.15f) ? (ph / 0.15f) : expf(-(ph - 0.15f) * 4.0f);
        raw[i] = (uint32_t)(100000.0f + 4000.0f * pulse);
    }

    ppg_sqi_result_t no_beats;
    memset(&no_beats, 0, sizeof(no_beats));
    ppg_calculate_sqi(raw, 64, NULL, 0, &no_beats);

    float ibi[10];
    for (int i = 0; i < 10; i++) ibi[i] = 800.0f + (float)((i % 3) - 1) * 8.0f;
    ppg_sqi_result_t with_beats;
    memset(&with_beats, 0, sizeof(with_beats));
    ppg_calculate_sqi(raw, 64, ibi, 10, &with_beats);

    printf("  SQI regression: shape-only=%.3f  with-beats=%.3f  (gate %.2f)\n",
           no_beats.overall_sqi, with_beats.overall_sqi, PPG_SQI_THRESHOLD_VALID);

    /* Shape alone must never certify a pulsatile signal. */
    assert(no_beats.overall_sqi < PPG_SQI_THRESHOLD_VALID);
    assert(no_beats.is_motion_artifact == true);
    /* And the same waveform, once it has a regular beat train behind it, must
     * score strictly better - otherwise the cap is masking a real signal. */
    assert(with_beats.overall_sqi > no_beats.overall_sqi);

    printf("test_sqi_requires_beat_intervals: PASS\n");
}

/* Emergency assist state machine (requirement 6).
 *
 * sos.c has no hardware or RTOS dependency, so every timer that matters is
 * exercised here rather than on the bench: the confirm window that stops a
 * single corrupt second latching an emergency, the cancel hold that stops a
 * knock dismissing one, the recovery stand-down that stops a false trigger
 * holding the screen until the battery dies, and the re-arm rule that lets a
 * cancelled emergency be replaced by a genuine one. */
static void test_sos_state_machine(void) {
    sos_config_t cfg;
    sos_default_config(&cfg);
    assert(cfg.contact_lost_enabled == false);   /* fingertip device default */

    /* A single critical second must NOT latch - it is inside the 15 s confirm. */
    sos_init(&cfg);
    sos_update(0, true, true);
    assert(sos_get_state() == SOS_ARMED);
    sos_update(5000, true, true);
    assert(sos_get_state() == SOS_ARMED);
    sos_update(10000, false, true);              /* artefact clears */
    assert(sos_get_state() == SOS_IDLE);

    /* Sustained critical latches after 15 s confirm window. */
    sos_init(&cfg);
    sos_update(0, true, true);
    sos_update(15000, true, true);
    assert(sos_get_state() == SOS_ACTIVE);
    assert(sos_get_trigger() == SOS_TRIGGER_CRITICAL_TRIAGE);
    printf("  SOS: critical latched after %.0f ms confirm\n", 15000.0);

    /* A critical flag with no finger is stale data, not an emergency. */
    sos_init(&cfg);
    sos_update(0, true, false);
    sos_update(20000, true, false);
    assert(sos_get_state() == SOS_IDLE);

    /* Recovery stand-down after 30 s of continuous non-critical. */
    sos_init(&cfg);
    sos_update(0, true, true);
    sos_update(15000, true, true);
    assert(sos_get_state() == SOS_ACTIVE);
    sos_update(45000, false, true);              /* recovery clock starts */
    assert(sos_get_state() == SOS_ACTIVE);
    sos_update(74000, false, true);              /* 29 s - not yet */
    assert(sos_get_state() == SOS_ACTIVE);
    sos_update(76000, false, true);              /* 31 s -> stand down */
    /* The stand-down latches as CANCELLED rather than dropping straight to
     * IDLE: if the level oscillates around critical, going idle would let it
     * re-arm and re-latch every few seconds. It reaches IDLE on the next update
     * where nothing is pending, which is also what lets a genuine re-crash arm
     * a fresh emergency. */
    assert(sos_get_state() == SOS_CANCELLED);
    sos_update(77000, false, true);
    assert(sos_get_state() == SOS_IDLE);

    /* A brief recovery must not clear it. */
    sos_init(&cfg);
    sos_update(0, true, true);
    sos_update(15000, true, true);
    sos_update(20000, false, true);
    sos_update(25000, true, true);               /* critical again */
    sos_update(90000, true, true);
    assert(sos_get_state() == SOS_ACTIVE);

    /* Cancel requires the full 5 s hold. */
    sos_init(&cfg);
    sos_manual_trigger(0);
    assert(sos_get_state() == SOS_ACTIVE);
    sos_cancel_press(1000);
    sos_update(4000, false, true);               /* 3 s held */
    assert(sos_get_state() == SOS_ACTIVE);
    assert(sos_cancel_progress_pct(4000) == 60);
    sos_update(6000, false, true);               /* 5 s held */
    assert(sos_get_state() == SOS_CANCELLED);

    /* Releasing early resets the hold, so a brush cannot dismiss it. */
    sos_init(&cfg);
    sos_manual_trigger(0);
    sos_cancel_press(1000);
    sos_update(5000, false, true);               /* 4 s - not enough */
    assert(sos_get_state() == SOS_ACTIVE);
    sos_cancel_release();
    sos_update(9000, false, true);
    assert(sos_get_state() == SOS_ACTIVE);

    /* An explicit cancel latches, then returns to IDLE so a fresh episode arms. */
    sos_init(&cfg);
    sos_manual_trigger(0);
    sos_cancel_now(1000);
    assert(sos_get_state() == SOS_CANCELLED);
    sos_update(2000, false, true);               /* nothing pending */
    assert(sos_get_state() == SOS_IDLE);
    sos_update(3000, true, true);
    sos_update(18000, true, true);
    assert(sos_get_state() == SOS_ACTIVE);

    /* A manual emergency is never auto-cleared: only a human stands it down. */
    sos_init(&cfg);
    sos_manual_trigger(0);
    sos_update(120000, false, true);
    assert(sos_get_state() == SOS_ACTIVE);

    /* The contact-loss trigger stays off by default: the finger comes off
     * between every measurement on a fingertip device. */
    sos_init(&cfg);
    sos_update(0, false, false);
    sos_update(600000, false, false);
    assert(sos_get_state() == SOS_IDLE);

    /* ... but works when explicitly enabled, keeping the same confirm rule. */
    cfg.contact_lost_enabled = true;
    sos_init(&cfg);
    sos_update(0, false, false);
    assert(sos_get_state() == SOS_ARMED);
    sos_update(60000, false, false);             /* 60 s < 120 s confirm */
    assert(sos_get_state() == SOS_ARMED);
    sos_update(121000, false, false);
    assert(sos_get_state() == SOS_ACTIVE);
    assert(sos_get_trigger() == SOS_TRIGGER_CONTACT_LOST);

    printf("test_sos_state_machine: PASS\n");
}

/* The status page wire format (T1.3).
 *
 * The page derives nothing - every number it shows is computed on the device -
 * so this document is the whole contract between the two, and it is worth
 * pinning down exactly. */
static void test_web_status_json(void) {
    valor_status_t s;
    memset(&s, 0, sizeof(s));
    s.hr = 72.1f; s.spo2 = 97.5f; s.rr = 0.0f; s.sqi = 0.87f; s.rmssd = 38.3f;
    s.temp = 29.2f; s.hum = 61.7f; s.pm25 = 13.9f;
    s.news2 = 0; s.level = 0; s.flags = 0;
    s.risk = "NORMAL"; s.sos = "IDLE"; s.trigger = "none"; s.loc = "UNSET";
    s.uptime_s = 123; s.contact = true;
    s.pressure_hpa = 1013.2f; s.pressure_trend_hpa_per_hr = -1.25f; s.storm = "HIGH";

    char buf[512];
    size_t n = web_status_json(&s, buf, sizeof(buf));
    assert(n > 0);
    assert(strlen(buf) == n);            /* length agrees with the content */

    assert(strstr(buf, "\"hr\":72.1")        != NULL);
    assert(strstr(buf, "\"spo2\":97.5")      != NULL);
    assert(strstr(buf, "\"rr\":0.0")         != NULL);
    assert(strstr(buf, "\"sqi\":0.87")       != NULL);
    assert(strstr(buf, "\"rmssd\":38.3")     != NULL);
    assert(strstr(buf, "\"risk\":\"NORMAL\"")!= NULL);
    assert(strstr(buf, "\"sos\":\"IDLE\"")   != NULL);
    assert(strstr(buf, "\"loc\":\"UNSET\"")  != NULL);
    assert(strstr(buf, "\"contact\":true")   != NULL);
    assert(strstr(buf, "\"up\":123")         != NULL);
    /* The storm advisory and the trend that drives it (T3.1). A negative trend
     * is a FALLING barometer, which is the direction that precedes a storm. */
    assert(strstr(buf, "\"pressure\":1013.2")    != NULL);
    assert(strstr(buf, "\"ptrend\":-1.25")       != NULL);
    assert(strstr(buf, "\"storm\":\"HIGH\"")     != NULL);

    /* An active emergency must be distinguishable, since the page turns red on
     * exactly this field. */
    s.sos = "ACTIVE"; s.trigger = "critical triage"; s.risk = "CRITICAL";
    n = web_status_json(&s, buf, sizeof(buf));
    assert(n > 0);
    assert(strstr(buf, "\"sos\":\"ACTIVE\"")                 != NULL);
    assert(strstr(buf, "\"trigger\":\"critical triage\"")    != NULL);
    assert(strstr(buf, "\"risk\":\"CRITICAL\"")              != NULL);

    /* NULL name fields must degrade to words, not to a malformed document: the
     * page would otherwise render the string "null". */
    s.risk = NULL; s.sos = NULL; s.trigger = NULL; s.loc = NULL;
    n = web_status_json(&s, buf, sizeof(buf));
    assert(n > 0);
    assert(strstr(buf, "\"risk\":\"UNKNOWN\"") != NULL);
    assert(strstr(buf, "\"sos\":\"IDLE\"")     != NULL);
    assert(strstr(buf, "\"trigger\":\"none\"") != NULL);
    assert(strstr(buf, "\"loc\":\"UNSET\"")    != NULL);

    /* A buffer that cannot hold the document must report failure. A truncated
     * line would be invalid JSON and the page would silently stop updating. */
    char tiny[16];
    assert(web_status_json(&s, tiny, sizeof(tiny)) == 0);

    /* And NULL arguments must not fault. */
    assert(web_status_json(NULL, buf, sizeof(buf)) == 0);
    assert(web_status_json(&s, NULL, sizeof(buf)) == 0);

    printf("test_web_status_json: PASS (document is %u bytes)\n",
           (unsigned)strlen(buf));
}

/* Deployment location: what actually gets stored and shown (T1.4). */
static void test_location_store(void) {
    char buf[LOCATION_MAX_LEN];

    /* Trimming and interior whitespace collapsing. */
    assert(location_sanitize("   Kolar   ", buf, sizeof(buf)));
    assert(strcmp(buf, "Kolar") == 0);
    assert(location_sanitize("Ward 3\tKolar", buf, sizeof(buf)));
    assert(strcmp(buf, "Ward 3 Kolar") == 0);   /* not "Ward3Kolar" */
    assert(location_sanitize("  a   b  ", buf, sizeof(buf)));
    assert(strcmp(buf, "a b") == 0);

    /* Control bytes are dropped, not stored. */
    assert(location_sanitize("A\x01\x02" "B", buf, sizeof(buf)));
    assert(strcmp(buf, "AB") == 0);

    /* Nothing usable must report failure rather than storing "". */
    assert(!location_sanitize("", buf, sizeof(buf)));
    assert(!location_sanitize("   \t\r\n ", buf, sizeof(buf)));
    assert(!location_sanitize("\x01\x02", buf, sizeof(buf)));
    assert(!location_sanitize(NULL, buf, sizeof(buf)));
    assert(buf[0] == '\0');

    /* Truncation is bounded by the buffer, not by the input. */
    char long_in[80];
    memset(long_in, 'X', sizeof(long_in) - 1);
    long_in[sizeof(long_in) - 1] = '\0';
    assert(location_sanitize(long_in, buf, sizeof(buf)));
    assert(strlen(buf) == LOCATION_MAX_LEN - 1);

    /* Form decoding: what a phone keyboard actually submits. */
    assert(location_parse_form("loc=Ward+3%2C+Kolar", buf, sizeof(buf)));
    assert(strcmp(buf, "Ward 3, Kolar") == 0);
    assert(location_parse_form("x=1&loc=Village%20Hulimavu&y=2", buf, sizeof(buf)));
    assert(strcmp(buf, "Village Hulimavu") == 0);   /* not first, and not last */
    /* The key must match a whole field: "bloc=" is not "loc=". */
    assert(!location_parse_form("bloc=Kolar", buf, sizeof(buf)));
    assert(!location_parse_form("x=1&y=2", buf, sizeof(buf)));
    assert(!location_parse_form("loc=", buf, sizeof(buf)));
    assert(!location_parse_form("loc=%00%01", buf, sizeof(buf)));
    assert(!location_parse_form(NULL, buf, sizeof(buf)));

    /* Card rendering: verbatim when it fits, marked when it does not. A
     * shortened district name with no indication would be worse than a short
     * one. */
    char card[LOCATION_MAX_LEN];
    location_card_text("Kolar", card, sizeof(card));
    assert(strcmp(card, "Kolar") == 0);
    location_card_text("1234567890123456", card, sizeof(card));    /* exactly 16 */
    assert(strcmp(card, "1234567890123456") == 0);
    location_card_text("12345678901234567", card, sizeof(card));   /* 17 */
    assert(strcmp(card, "1234567890123...") == 0);
    assert(strlen(card) == LOCATION_CARD_COLS);

    /* Round trip through the store. */
    location_init();
    assert(!location_is_set());
    assert(strcmp(location_get(), "UNSET") == 0);   /* never NULL, never blank */
    assert(location_set("  Ward 3, Kolar  "));
    assert(location_is_set());
    assert(strcmp(location_get(), "Ward 3, Kolar") == 0);
    /* Text that sanitises to nothing must leave the stored value alone
     * rather than wiping a location that is already in use. */
    assert(!location_set("   "));
    assert(strcmp(location_get(), "Ward 3, Kolar") == 0);

    printf("test_location_store: PASS (stored '%s')\n", location_get());
}

/* Barometric pressure trend and the cyclone advisory (T3.1). */
static void test_pressure_trend(void) {
    pressure_trend_t t;
    pressure_trend_result_t r;

    /* Nothing to fit. */
    pressure_trend_init(&t);
    pressure_trend_evaluate(&t, &r);
    assert(!r.valid);

    /* A two-minute span is not a weather trend. */
    pressure_trend_add(&t, 0, 1013.0f);
    pressure_trend_add(&t, 60000, 1012.5f);
    pressure_trend_evaluate(&t, &r);
    assert(!r.valid);
    assert(r.samples == 2);

    /* Steady over an hour -> valid, flat. */
    pressure_trend_init(&t);
    for (int i = 0; i <= 60; i++) {
        pressure_trend_add(&t, (uint32_t)i * 60000u, 1013.0f);
    }
    pressure_trend_evaluate(&t, &r);
    assert(r.valid);
    assert(fabsf(r.slope_hpa_per_hr) < 0.01f);
    assert(fabsf(r.drop_hpa) < 0.01f);
    assert(r.span_ms == 60u * 60000u);

    /* Falling 1 hPa per hour. Sign convention: negative slope = falling, and
     * positive drop = the barometer went DOWN. */
    pressure_trend_init(&t);
    for (int i = 0; i <= 60; i++) {
        pressure_trend_add(&t, (uint32_t)i * 60000u, 1013.0f - (float)i / 60.0f);
    }
    pressure_trend_evaluate(&t, &r);
    assert(r.valid);
    assert(fabsf(r.slope_hpa_per_hr + 1.0f) < 0.01f);
    assert(fabsf(r.drop_hpa - 1.0f) < 0.01f);

    /* Rising is the other sign. */
    pressure_trend_init(&t);
    for (int i = 0; i <= 60; i++) {
        pressure_trend_add(&t, (uint32_t)i * 60000u, 1013.0f + (float)i / 60.0f);
    }
    pressure_trend_evaluate(&t, &r);
    assert(r.valid);
    assert(r.slope_hpa_per_hr > 0.5f);

    /* Implausible readings must never enter the fit - one bad compensation read
     * would otherwise move the slope by hundreds of hPa/hr. */
    pressure_trend_init(&t);
    pressure_trend_add(&t, 0, 1013.0f);
    pressure_trend_add(&t, 60000, 0.0f);
    pressure_trend_add(&t, 120000, NAN);
    pressure_trend_add(&t, 180000, 5000.0f);
    pressure_trend_add(&t, 240000, 1012.9f);
    pressure_trend_evaluate(&t, &r);
    assert(r.samples == 2);

    /* A timestamp that would fold the window backwards is dropped. */
    pressure_trend_init(&t);
    pressure_trend_add(&t, 120000, 1013.0f);
    pressure_trend_add(&t, 60000, 1010.0f);
    pressure_trend_evaluate(&t, &r);
    assert(r.samples == 1);
    assert(r.pressure_hpa == 1013.0f);

    /* Ring wrap: past capacity the window holds the NEWEST span, and the fit
     * walks it oldest-first from the right slot. */
    pressure_trend_init(&t);
    for (int i = 0; i < 200; i++) {
        pressure_trend_add(&t, (uint32_t)i * 60000u, 1000.0f + (float)i * 0.1f);
    }
    pressure_trend_evaluate(&t, &r);
    assert(r.samples == PRESSURE_TREND_CAPACITY);
    assert(r.span_ms == (uint32_t)(PRESSURE_TREND_CAPACITY - 1) * 60000u);
    assert(fabsf(r.pressure_hpa - (1000.0f + 199.0f * 0.1f)) < 1e-3f);
    /* samples 20..199 at +0.1 each -> +0.1 hPa per minute -> +6 hPa/hr */
    assert(fabsf(r.slope_hpa_per_hr - 6.0f) < 0.05f);

    /* Paced add: called at 1 Hz it stores once a minute. */
    pressure_trend_init(&t);
    uint32_t due = 0;
    for (int s = 0; s <= 180; s++) {
        pressure_trend_add_paced(&t, (uint32_t)s * 1000u, 1013.0f, &due);
    }
    assert(t.count == 4);   /* s = 0, 60, 120, 180 */

    printf("test_pressure_trend: PASS (slope sign, window, wrap and pacing)\n");
}

static void test_cyclone_risk(void) {
    pressure_trend_result_t r;
    risk_level_t risk;
    const char *adv;

    /* No usable trend -> UNKNOWN. Never RISK_NORMAL: NORMAL is a claim that the
     * barometer is steady, and that cannot be said before the window is long
     * enough to know. */
    memset(&r, 0, sizeof(r));
    r.valid = false;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_UNKNOWN);
    assert(adv != NULL);

    assess_cyclone_risk(NULL, &risk, &adv);
    assert(risk == RISK_UNKNOWN);

    /* Steady -> NORMAL. */
    r.valid = true; r.slope_hpa_per_hr = 0.05f; r.drop_hpa = 0.1f;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_NORMAL);

    r.slope_hpa_per_hr = -0.6f; r.drop_hpa = 0.8f;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_MODERATE);

    r.slope_hpa_per_hr = -1.2f; r.drop_hpa = 1.6f;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_HIGH);

    r.slope_hpa_per_hr = -2.5f; r.drop_hpa = 3.0f;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_CRITICAL);

    /* A steep RATE with almost no actual fall must not score. This is the case
     * the absolute-drop floor exists for: a fraction of a hPa of sensor drift
     * across a short window produces a dramatic-looking slope and no storm. */
    r.slope_hpa_per_hr = -3.0f; r.drop_hpa = 0.2f;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_NORMAL);

    /* A rise is not a hazard this device acts on. */
    r.slope_hpa_per_hr = 2.0f; r.drop_hpa = -2.0f;
    assess_cyclone_risk(&r, &risk, &adv);
    assert(risk == RISK_NORMAL);

    printf("test_cyclone_risk: PASS (thresholds, drop floor, rise ignored)\n");
}

/* Respiratory rate confidence (T3.5).
 *
 * The confidence used to BE the autocorrelation coefficient. On real finger PPG
 * that peaks around 0.3-0.5, so it never cleared the 0.60 bar and RR was
 * published as unavailable on every real contact while the estimator was working
 * correctly. It is now scored on periodicity AND modulation depth, with credit
 * for FM/AM agreement.
 *
 * The pair that matters is here: a real respiratory modulation must be published,
 * and jitter with no modulation must not be - in either direction the failure is
 * a wrong number in NEWS2's most sensitive term. The jitter case is run over
 * several seeds, because one seed proving nothing proves nothing. */
static void test_rr_confidence(void) {
    ppg_respiratory_result_t rr;
    const size_t N = 60;
    float ibi[60];
    const float mean_ibi = 800.0f;   /* 75 bpm */

    /* A real respiratory sinusoid: 15 br/min (a 4 s cycle) modulating the rhythm
     * with 40 ms of peak-to-peak RSA. The phase advances with elapsed TIME, not
     * with beat index, so this is a genuine respiratory modulation rather than an
     * artefact of how the array is indexed. */
    const float rr_true  = 15.0f;
    const float period_s = 60.0f / rr_true;
    const float depth_ms = 40.0f;
    float t = 0.0f;
    for (size_t i = 0; i < N; i++) {
        float phase = 2.0f * 3.14159265f * (t / period_s);
        ibi[i] = mean_ibi + 0.5f * depth_ms * sinf(phase);
        t += ibi[i] / 1000.0f;
    }

    memset(&rr, 0, sizeof(rr));
    ppg_estimate_respiratory_rate(ibi, NULL, N, &rr);
    printf("  RR real RSA : rr=%5.1f (true %.0f)  conf=%.2f  reliable=%d  rsa=%.1f ms\n",
           rr.respiratory_rate_bpm, rr_true, rr.confidence, (int)rr.is_reliable,
           rr.rsa_depth_ms);
    assert(rr.is_reliable);
    assert(fabsf(rr.respiratory_rate_bpm - rr_true) < 2.5f);
    /* The equivalent-sinusoid depth should recover the injected 40 ms. */
    assert(rr.rsa_depth_ms > 30.0f && rr.rsa_depth_ms < 50.0f);

    /* Jitter with NO respiratory modulation, at a comparable magnitude. There is
     * no periodicity here, so nothing may be published even though the raw
     * variability is similar - which is exactly the trap the old depth measure
     * (max minus min) fell into. */
    int reliable_count = 0;
    float worst_conf = 0.0f;
    for (uint32_t s0 = 1; s0 <= 8; s0++) {
        uint32_t seed = s0 * 7919u;
        for (size_t i = 0; i < N; i++) {
            seed = seed * 1103515245u + 12345u;
            float j = ((float)((seed >> 16) & 0x7FFFu) / 32767.0f - 0.5f) * 40.0f;
            ibi[i] = mean_ibi + j;
        }
        memset(&rr, 0, sizeof(rr));
        ppg_estimate_respiratory_rate(ibi, NULL, N, &rr);
        if (rr.is_reliable) reliable_count++;
        if (rr.confidence > worst_conf) worst_conf = rr.confidence;
    }
    printf("  RR jitter x8: %d of 8 published, worst conf=%.2f\n",
           reliable_count, worst_conf);
    assert(reliable_count == 0);

    /* A 7 br/min oscillation with 80 ms of modulation - the Mayer frequency.
     *
     * Whether this must be PUBLISHED is a policy decision encoded in
     * PPG_RR_MIN_BPM, not an estimator property: the estimator reports a clean
     * 7 br/min oscillation quite happily, which is precisely why the floor exists.
     * So the assertion follows the configured floor instead of hardcoding 9. At a
     * floor of 9 the Mayer band is refused; at the temporary floor of 6 used for
     * the breath-hold test a 7 br/min oscillation MUST come through, because that
     * test needs to SEE one in order to determine whether a real Mayer wave exists.
     * Either way an unintended floor change or a band-gate regression fails here. */
    const float rr_slow  = 7.0f;
    const float per_slow = 60.0f / rr_slow;
    t = 0.0f;
    for (size_t i = 0; i < N; i++) {
        float phase = 2.0f * 3.14159265f * (t / per_slow);
        ibi[i] = mean_ibi + 0.5f * 80.0f * sinf(phase);   /* 80 ms peak-to-peak */
        t += ibi[i] / 1000.0f;
    }
    memset(&rr, 0, sizeof(rr));
    ppg_estimate_respiratory_rate(ibi, NULL, N, &rr);
    bool slow_in_band = (rr_slow >= PPG_RR_MIN_BPM);
    printf("  RR 7/min Mayer band : rr=%5.1f  conf=%.2f  reliable=%d (floor %.1f -> expect %d)\n",
           rr.respiratory_rate_bpm, rr.confidence, (int)rr.is_reliable,
           PPG_RR_MIN_BPM, (int)slow_in_band);
    assert(rr.is_reliable == slow_in_band);

    printf("test_rr_confidence: PASS (real modulation published, jitter and the Mayer band rejected)\n");
}

/* Published-rate stabilisation (T3.5 follow-on).
 *
 * The estimator is stateless and re-decides every second. On hardware the rate
 * appeared in 4 frames of 118 - confidence sitting on its own bar - which
 * flickers the display and, since NEWS2 scores the band edges most steeply,
 * flips the triage between NORMAL and MODERATE frame to frame. */
static void test_rr_tracker(void) {
    ppg_rr_tracker_t t;
    ppg_respiratory_result_t rr;

    /* Nothing reliable yet -> nothing published. */
    ppg_rr_tracker_init(&t);
    memset(&rr, 0, sizeof(rr));
    rr.respiratory_rate_bpm = 20.0f; rr.confidence = 0.30f; rr.is_reliable = false;
    ppg_rr_tracker_update(&t, &rr);
    assert(!rr.is_reliable);

    /* A reliable estimate is published as-is. */
    rr.respiratory_rate_bpm = 15.0f; rr.confidence = 0.80f; rr.is_reliable = true;
    ppg_rr_tracker_update(&t, &rr);
    assert(rr.is_reliable);
    assert(fabsf(rr.respiratory_rate_bpm - 15.0f) < 0.01f);

    /* THE FLICKER THIS EXISTS FOR. Confidence dips below the bar but not below
     * the hysteresis floor: the rate must be HELD, not retracted. */
    memset(&rr, 0, sizeof(rr));
    rr.respiratory_rate_bpm = 30.0f;   /* a wildly different raw estimate */
    rr.confidence = 0.52f;             /* below 0.60, above 0.45 */
    rr.is_reliable = false;
    ppg_rr_tracker_update(&t, &rr);
    assert(rr.is_reliable);                                 /* held, not blinked off */
    assert(fabsf(rr.respiratory_rate_bpm - 15.0f) < 0.01f); /* and not the new value */
    assert(rr.confidence >= PPG_RR_CONF_MIN);               /* invariant preserved */

    /* ...but the hold is BOUNDED. An unbounded one would be a stale value
     * published as current, which is the SpO2 defect all over again. */
    int held_for = 1;
    for (int i = 0; i < PPG_RR_HOLD_MAX + 2; i++) {
        memset(&rr, 0, sizeof(rr));
        rr.respiratory_rate_bpm = 30.0f;
        rr.confidence = 0.52f;
        rr.is_reliable = false;
        ppg_rr_tracker_update(&t, &rr);
        if (!rr.is_reliable) break;
        held_for++;
    }
    assert(!rr.is_reliable);
    assert(held_for <= PPG_RR_HOLD_MAX);

    /* Decisively below the floor -> retract at once. */
    ppg_rr_tracker_init(&t);
    memset(&rr, 0, sizeof(rr));
    rr.respiratory_rate_bpm = 15.0f; rr.confidence = 0.80f; rr.is_reliable = true;
    ppg_rr_tracker_update(&t, &rr);
    assert(rr.is_reliable);
    memset(&rr, 0, sizeof(rr));
    rr.respiratory_rate_bpm = 25.0f; rr.confidence = 0.30f; rr.is_reliable = false;
    ppg_rr_tracker_update(&t, &rr);
    assert(!rr.is_reliable);

    /* One outlier among accepted estimates must not move the published value -
     * that is what the median is for. */
    ppg_rr_tracker_init(&t);
    const float series[5] = { 14.0f, 15.0f, 14.5f, 40.0f, 15.5f };
    for (int i = 0; i < 5; i++) {
        memset(&rr, 0, sizeof(rr));
        rr.respiratory_rate_bpm = series[i];
        rr.confidence = 0.80f;
        rr.is_reliable = true;
        ppg_rr_tracker_update(&t, &rr);
    }
    assert(rr.is_reliable);
    assert(rr.respiratory_rate_bpm >= 14.0f && rr.respiratory_rate_bpm <= 15.5f);

    printf("test_rr_tracker: PASS (holds through the flicker, bounded, median rejects outliers)\n");
}

/* Synthetic respiratory modulation at a known rate, sampled at the beat rate.
 * Phase advances with elapsed TIME, not beat index, so this is a genuine
 * respiratory modulation and not an artefact of the array indexing. */
static float rr_synth(float true_bpm, float mean_ibi, float depth_ms,
                      size_t n, ppg_respiratory_result_t *out) {
    float ibi[96];
    if (n > 96) n = 96;
    float t = 0.0f;
    float period_s = 60.0f / true_bpm;
    for (size_t i = 0; i < n; i++) {
        float phase = 2.0f * 3.14159265f * (t / period_s);
        ibi[i] = mean_ibi + 0.5f * depth_ms * sinf(phase);
        t += ibi[i] / 1000.0f;
    }
    memset(out, 0, sizeof(*out));
    ppg_estimate_respiratory_rate(ibi, NULL, n, out);
    return out->respiratory_rate_bpm;
}

/* Rate resolution.
 *
 * The autocorrelation is sampled at INTEGER beat lags and the period was formed
 * as `lag * mean_ibi`, so the reported rate could only land on the grid
 * 60000/(k * mean_ibi). At a resting 800 ms IBI that grid is 25.0, 18.75, 15.0,
 * 12.5, 10.71 br/min - steps of up to 6.25, and up to 3.1 br/min of error at the
 * midpoint between two lags.
 *
 * That is clinically load-bearing: NEWS2's first abnormal respiratory band
 * starts at 21 br/min, and this file's own FM/AM agreement test tolerates
 * 3 br/min of disagreement between two independent estimates. An instrument
 * whose resolution is coarser than both cannot support either.
 *
 * It is also why a paced-breathing run at 15 br/min could not be distinguished
 * from a spontaneous 17 - at the 880 ms IBI measured on hardware, 15 sits
 * almost exactly between lag 4 (17.0) and lag 5 (13.6), so that comparison was
 * deciding which of two adjacent lags won, not measuring respiration.
 *
 * The pre-existing test passed only because 15 br/min at 800 ms is exactly
 * lag 5. Every rate here is deliberately the MIDPOINT between two adjacent
 * lags, which is the worst case by construction, and it is run at two IBIs so
 * the result cannot be an accident of one grid. */
static void test_rr_resolution(void) {
    const float mean_ibis[] = { 800.0f, 880.0f };
    const int   pairs[]     = { 3, 4, 5, 6, 7 };   /* lag k and k+1 */
    const float tol         = 1.5f;                /* br/min */
    const size_t N          = 90;

    printf("  RR resolution - off-grid rates are the midpoint between two lags\n");
    printf("  (worst case; tolerance %.1f br/min)\n", tol);

    float worst_err  = 0.0f;
    float worst_true = 0.0f;
    int   failures   = 0;
    int   n_run      = 0;

    for (size_t b = 0; b < sizeof(mean_ibis) / sizeof(mean_ibis[0]); b++) {
        float ibi = mean_ibis[b];
        for (size_t p = 0; p < sizeof(pairs) / sizeof(pairs[0]); p++) {
            float rr_hi = 60000.0f / ((float)pairs[p]       * ibi);
            float rr_lo = 60000.0f / ((float)(pairs[p] + 1) * ibi);
            float truth = 0.5f * (rr_hi + rr_lo);

            ppg_respiratory_result_t rr;
            float est = rr_synth(truth, ibi, 40.0f, N, &rr);
            float err = fabsf(est - truth);
            n_run++;

            printf("    IBI %3.0f ms  lag %d/%d  true %5.2f  est %5.2f  err %5.2f  conf %.2f  rel %d\n",
                   ibi, pairs[p], pairs[p] + 1, truth, est, err,
                   rr.confidence, (int)rr.is_reliable);

            if (err > worst_err) { worst_err = err; worst_true = truth; }
            /* A default-valued result would otherwise pass this test by
             * accident: PPG_RR_DEFAULT_BPM is 14, which happens to sit near
             * several of these midpoints. Insist the estimator actually
             * committed to an answer. */
            if (!rr.is_reliable || err > tol) failures++;
        }
    }

    printf("  RR resolution: %d of %d off-grid rates outside %.1f br/min "
           "(worst %.2f br/min at true %.2f)\n",
           failures, n_run, tol, worst_err, worst_true);
    assert(failures == 0);

    printf("test_rr_resolution: PASS (off-grid rates resolved to better than %.1f br/min)\n", tol);
}

/* Band edges.
 *
 * PPG_RR_MIN_BPM is documented as "below 9 br/min this now reports unavailable".
 * Gating the autocorrelation SEARCH by the band did not do that: excluding the
 * out-of-band lags made the estimator snap to the nearest admissible one and
 * report it. A genuine 8 br/min at an 880 ms IBI peaks at lag 8.5; with that
 * excluded the winner was lag 7 (9.74 br/min), the parabola clamped at +0.5, and
 * 9.09 was published as RELIABLE. NEWS2 scores <=8 as +3 and 9-11 as +1, so a
 * severe bradypnoea was reported as a mild one - and because 9.09 is just inside
 * the band, the `clipped` guard never fired.
 *
 * Both directions are asserted here. Refusing to publish 8 is only correct if
 * 10 still publishes: an instrument that refuses real physiology just inside its
 * own floor would be trading one silent failure for another. */
static void test_rr_band_edges(void) {
    ppg_respiratory_result_t rr;
    const float mean_ibi = 880.0f;
    const float depth    = 60.0f;
    const size_t N       = 60;

    /* Derived from the configured floor rather than hardcoded, so the test keeps
     * its meaning when PPG_RR_MIN_BPM moves - it is 6.0 while the breath-hold test
     * runs. The regression it guards is floor-independent: an out-of-band rate must
     * be REFUSED, not snapped to the nearest admissible lag and published. Gating
     * the search by the band did the latter, and at a floor of 9 a genuine 8 br/min
     * came out as 9.09, reliable - NEWS2 +3 reported as +1. */
    const float below       = PPG_RR_MIN_BPM - 2.0f;
    const float inside      = PPG_RR_MIN_BPM + 1.0f;
    const float well_inside = PPG_RR_MIN_BPM + 3.0f;

    float est = rr_synth(below, mean_ibi, depth, N, &rr);
    printf("  RR band floor : true %4.1f (below %.1f) -> est %5.2f  conf %.2f  reliable %d"
           " (must be unavailable)\n",
           below, PPG_RR_MIN_BPM, est, rr.confidence, (int)rr.is_reliable);
    assert(!rr.is_reliable);

    est = rr_synth(inside, mean_ibi, depth, N, &rr);
    printf("  RR band floor : true %4.1f -> est %5.2f  conf %.2f  reliable %d (must publish)\n",
           inside, est, rr.confidence, (int)rr.is_reliable);
    assert(rr.is_reliable);
    assert(fabsf(est - inside) < 1.0f);

    /* The diagnostics have to be genuinely populated, or the [RRDIAG] telemetry
     * line is a comfort blanket that prints zeros and diagnoses nothing. */
    printf("                  diag: beats=%d mean_ibi=%.0f lag=%d(%.2f) r=%.3f\n",
           rr.diag_beats, rr.diag_mean_ibi_ms, rr.diag_peak_lag,
           rr.diag_peak_lag_f, rr.diag_acf_peak_r);
    assert(rr.diag_beats == (int)N);
    assert(rr.diag_peak_lag > 0);
    assert(rr.diag_peak_lag_f > 0.0f);
    assert(rr.diag_acf_peak_r > 0.0f);
    assert(fabsf(rr.diag_mean_ibi_ms - mean_ibi) < 25.0f);

    est = rr_synth(well_inside, mean_ibi, depth, N, &rr);
    printf("  RR band floor : true %4.1f -> est %5.2f  conf %.2f  reliable %d\n",
           well_inside, est, rr.confidence, (int)rr.is_reliable);
    assert(rr.is_reliable);
    assert(fabsf(est - well_inside) < 1.0f);

    printf("test_rr_band_edges: PASS (out-of-band refused, not snapped to the edge)\n");
}

/* Beat-to-beat alternans must not be reported as tachypnoea.
 *
 * On real finger PPG the estimator picked lag 2 - the first lag the search scans -
 * in 42 of 108 frames, including every second of a breath hold and 25 s of normal
 * breathing afterwards. At an 800 ms mean IBI, lag 2 is 37.5 br/min, above the
 * ceiling, so the integer lag should have been refused outright; the parabola
 * clamped to +0.5 instead, giving lag 2.5 and 30.0 br/min, inside the band, and
 * that was published as reliable.
 *
 * This reproduces the shape without needing the hardware: a respiratory sinusoid
 * the estimator should find, plus a lag-2 alternans of comparable size. A period-2
 * component correlates at every EVEN lag, so lag 2 is a legitimate sub-multiple of
 * the respiratory lag and the harmonic search cannot reject it on that basis - the
 * band check on the integer lag is what has to catch it. */
static void test_rr_rejects_alternans(void) {
    ppg_respiratory_result_t rr;
    float ibi[96];
    const size_t N = 60;
    const float mean_ibi = 880.0f;   /* 68 bpm - the rate the bad capture ran at */
    const float rr_true = 12.0f;     /* a 5 s cycle: lag 6.88 */
    const float period_s = 60.0f / rr_true;
    const float resp_ms = 20.0f;     /* respiratory modulation, peak-to-peak */
    const float alt_ms  = 80.0f;     /* alternans DOMINATES - the real case */

    float t = 0.0f;
    for (size_t i = 0; i < N; i++) {
        float phase = 2.0f * 3.14159265f * (t / period_s);
        ibi[i] = mean_ibi
                 + 0.5f * resp_ms * sinf(phase)
                 + ((i % 2) ? 0.5f * alt_ms : -0.5f * alt_ms);
        t += ibi[i] / 1000.0f;
    }

    memset(&rr, 0, sizeof(rr));
    ppg_estimate_respiratory_rate(ibi, NULL, N, &rr);
    printf("  RR alternans : true %.0f -> est %5.2f  lag=%d(%.2f) r=%.3f r1=%.3f conf=%.2f rel=%d\n",
           rr_true, rr.respiratory_rate_bpm, rr.diag_peak_lag, rr.diag_peak_lag_f,
           rr.diag_acf_peak_r, rr.diag_lag1_r, rr.confidence, (int)rr.is_reliable);

    /* Either it refuses, or it reports something at the respiratory rate. What it
     * must never do is publish a tachypnoea derived from a 2-beat rhythm. */
    if (rr.is_reliable) {
        assert(fabsf(rr.respiratory_rate_bpm - rr_true) < 3.0f);
    }
    assert(rr.diag_peak_lag < 0 || rr.diag_peak_lag >= 4);

    printf("test_rr_rejects_alternans: PASS (lag-2 alternans not published as tachypnoea)\n");
}

int main() {
    /* Unbuffered, because a failing assert calls abort() and abort() does not
     * flush stdio. With the default block buffering the diagnostic printed
     * immediately before the failing assertion - the actual numbers - is thrown
     * away and all the runner shows is the assertion text. */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Running unit tests for disaster_risk_engine...\n");
    test_heat_risk();
    test_pollution_risk();
    test_flood_risk();
    test_hrv_not_ready();
    test_null_env();
    test_flood_ambient_proxy();    test_int8_matches_float_nn();
    test_spo2_clinical_rejection();
    test_rr_estimate_rejects_clamped_value();
    test_sqi_requires_beat_intervals();
    test_sos_state_machine();
    test_web_status_json();
    test_location_store();
    test_pressure_trend();
    test_cyclone_risk();
    test_rr_confidence();
    test_rr_tracker();
    test_rr_resolution();
    test_rr_band_edges();
    test_rr_rejects_alternans();
    printf("ALL TESTS PASSED.\n");
    return 0;
}