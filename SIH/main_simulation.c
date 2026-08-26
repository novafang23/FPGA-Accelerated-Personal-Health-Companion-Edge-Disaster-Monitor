/*
 * main_simulation.c
 * SIH26181 Health Companion Simulator
 *
 * Simulates 4 disaster scenarios on PC:
 *   1. Normal Resting
 *   2. Delhi Heat Wave (47C, cardiovascular drift, HRV collapse)
 *   3. Delhi Winter Smog (PM2.5 = 400, SpO2 desaturation)
 *   4. Flash Flood & Cold Water Immersion (Hypothermia, Cold Shock, RMSSD collapse)
 *
 * Compile & Run:
 *   gcc -o health_demo main_simulation.c hrv_analysis.c spo2_engine.c disaster_risk_engine.c nn_risk_model.c -lm
 *   ./health_demo
 *   ./health_demo --hypothermia
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

#include "hrv_analysis.h"
#include "disaster_risk_engine.h"
#include "nn_risk_model.h"

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

/* Scenario Definition */
typedef struct {
    const char *name;
    float duration_sec;

    /* Vitals: linearly interpolated from base to (base + drift) over duration */
    float base_hr;
    float hr_drift;
    float base_spo2;
    float spo2_drift;
    float hrv_rmssd_base;
    float hrv_rmssd_drift;

    /* Environment */
    float ambient_temp;
    float temp_drift;
    float humidity;
    float pm25;
    float pm25_drift;
    float skin_temp;
    float skin_temp_drift;
} scenario_t;

/* Console Dashboard Rendering */
static void print_banner(void) {
    printf(BOLD CYAN "+================================================================+\n");
    printf("|    SIH26181 Health Companion Simulator                         |\n");
    printf("+================================================================+\n" RESET);
}

static void print_dashboard(
    float time_sec,
    const char *scenario_name,
    float bpm,
    float spo2,
    float rmssd,
    float sdnn,
    const env_sensors_t *env,
    const risk_assessment_t *risk,
    const nn_output_t *nn_scores
) {
    const char *oc = risk_level_to_color(risk->overall_risk);
    const char *hc = risk_level_to_color(risk->heat_risk);
    const char *pc = risk_level_to_color(risk->pollution_risk);
    const char *fc = risk_level_to_color(risk->flood_risk);

    /* Clear screen and reposition cursor */
    printf("\033[2J\033[H");
    print_banner();

    printf(DIM " Scenario: " RESET BOLD "%s" RESET, scenario_name);
    printf(DIM "  |  Time: %.0fs\n\n" RESET, time_sec);

    /* ---- Vitals Panel ---- */
    printf(" +---------------- VITALS -----------------+\n");
    printf(" |  Heart Rate:    %s%-7.1f BPM" RESET "              |\n",
           (bpm > 120.0f || bpm < 50.0f) ? "\033[31m" : (bpm > 100.0f ? "\033[33m" : "\033[32m"), bpm);
    printf(" |  SpO2:          %s%-7.1f %%" RESET "                |\n",
           spo2 < 92.0f ? "\033[31m" : (spo2 < 95.0f ? "\033[33m" : "\033[32m"), spo2);
    printf(" |  HRV RMSSD:     %s%-7.1f ms" RESET "               |\n",
           rmssd < 15.0f ? "\033[31m" : (rmssd < 25.0f ? "\033[33m" : "\033[32m"), rmssd);
    printf(" |  HRV SDNN:      %-7.1f ms               |\n", sdnn);
    printf(" +-----------------------------------------+\n\n");

    /* ---- Environment Panel ---- */
    printf(" +------------- ENVIRONMENT ---------------+\n");
    printf(" |  Ambient Temp:  %s%-7.1f C" RESET "                |\n",
           env->ambient_temp_c > 42.0f ? "\033[31m" : (env->ambient_temp_c < 15.0f ? "\033[36m" : (env->ambient_temp_c > 35.0f ? "\033[33m" : "\033[32m")),
           env->ambient_temp_c);
    printf(" |  Skin Temp:     %s%-7.1f C" RESET "                |\n",
           env->skin_temp_c < 28.0f ? "\033[31m" : (env->skin_temp_c < 33.0f ? "\033[33m" : "\033[32m"),
           env->skin_temp_c);
    printf(" |  Humidity:      %-7.1f %%                |\n", env->humidity_pct);
    printf(" |  PM2.5:         %s%-7.0f ug/m3" RESET "            |\n",
           env->pm25 > 150.0f ? "\033[31m" : (env->pm25 > 75.0f ? "\033[33m" : "\033[32m"),
           env->pm25);
    printf(" +-----------------------------------------+\n\n");

    /* ---- Risk Assessment Panel ---- */
    printf(" +----------- RISK ASSESSMENT -------------+\n");
    printf(" |  Heat Risk:       %s%-10s" RESET "             |\n",
           hc, risk_level_to_string(risk->heat_risk));
    printf(" |  Pollution Risk:  %s%-10s" RESET "             |\n",
           pc, risk_level_to_string(risk->pollution_risk));
    printf(" |  Flood/Cold Risk: %s%-10s" RESET "             |\n",
           fc, risk_level_to_string(risk->flood_risk));
    printf(" |                                         |\n");
    printf(" |  >> OVERALL:      %s" BOLD "%-10s" RESET "             |\n",
           oc, risk_level_to_string(risk->overall_risk));
    printf(" +-----------------------------------------+\n\n");

    /* ---- NN Confidence Scores ---- */
    printf(" +--------- AI CONFIDENCE SCORES ----------+\n");
    printf(" |  Heat Neuron:     \033[36m%.3f" RESET "                    |\n", nn_scores->heat_score);
    printf(" |  Pollution Neuron:\033[36m%.3f" RESET "                    |\n", nn_scores->pollution_score);
    printf(" |  Flood Neuron:    \033[36m%.3f" RESET "                    |\n", nn_scores->flood_score);
    printf(" +-----------------------------------------+\n\n");

    /* ---- Advisory ---- */
    printf(" %s>> %s" RESET "\n\n", oc, risk->overall_advisory);

    /* ---- Privacy Badge ---- */
    printf(DIM " [TinyML on-device inference | Zero cloud | Qualcomm AI Engine ready]\n" RESET);
}

/* Main Entry Point */
int main(int argc, char **argv) {
    int s;
    int num_scenarios;
    int start_scenario = 0;
    float global_time;
    hrv_state_t hrv;
    scenario_t scenarios[4];

#ifdef _WIN32
    /* Enable ANSI escape sequences on Windows 10+ */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hOut, &dwMode);
        SetConsoleMode(hOut, dwMode | 0x0004); /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    }
    SetConsoleOutputCP(65001); /* UTF-8 */
#endif

    /* ---- Scenario 1: Normal Resting ---- */
    scenarios[0].name             = "Normal Resting (Indoor, 25C)";
    scenarios[0].duration_sec     = 10.0f;
    scenarios[0].base_hr          = 72.0f;
    scenarios[0].hr_drift         = 0.0f;
    scenarios[0].base_spo2        = 98.0f;
    scenarios[0].spo2_drift       = 0.0f;
    scenarios[0].hrv_rmssd_base   = 45.0f;
    scenarios[0].hrv_rmssd_drift  = 0.0f;
    scenarios[0].ambient_temp     = 25.0f;
    scenarios[0].temp_drift       = 0.0f;
    scenarios[0].humidity         = 45.0f;
    scenarios[0].pm25             = 15.0f;
    scenarios[0].pm25_drift       = 0.0f;
    scenarios[0].skin_temp        = 36.5f;
    scenarios[0].skin_temp_drift  = 0.0f;

    /* ---- Scenario 2: Heat Wave ---- */
    scenarios[1].name             = "HEAT WAVE (Outdoor, Delhi Summer 47C)";
    scenarios[1].duration_sec     = 15.0f;
    scenarios[1].base_hr          = 85.0f;
    scenarios[1].hr_drift         = 55.0f;
    scenarios[1].base_spo2        = 97.0f;
    scenarios[1].spo2_drift       = -2.0f;
    scenarios[1].hrv_rmssd_base   = 40.0f;
    scenarios[1].hrv_rmssd_drift  = -32.0f;
    scenarios[1].ambient_temp     = 38.0f;
    scenarios[1].temp_drift       = 12.0f;
    scenarios[1].humidity         = 65.0f;
    scenarios[1].pm25             = 25.0f;
    scenarios[1].pm25_drift       = 0.0f;
    scenarios[1].skin_temp        = 37.0f;
    scenarios[1].skin_temp_drift  = 2.0f;

    /* ---- Scenario 3: Severe Smog ---- */
    scenarios[2].name             = "SEVERE POLLUTION (Delhi Winter Smog, AQI 500+)";
    scenarios[2].duration_sec     = 15.0f;
    scenarios[2].base_hr          = 78.0f;
    scenarios[2].hr_drift         = 45.0f;
    scenarios[2].base_spo2        = 96.0f;
    scenarios[2].spo2_drift       = -10.0f;
    scenarios[2].hrv_rmssd_base   = 42.0f;
    scenarios[2].hrv_rmssd_drift  = -30.0f;
    scenarios[2].ambient_temp     = 12.0f;
    scenarios[2].temp_drift       = 0.0f;
    scenarios[2].humidity         = 85.0f;
    scenarios[2].pm25             = 50.0f;
    scenarios[2].pm25_drift       = 350.0f;
    scenarios[2].skin_temp        = 35.0f;
    scenarios[2].skin_temp_drift  = -2.0f;

    /* ---- Scenario 4: Flash Flood & Cold Water Immersion ---- */
    scenarios[3].name             = "FLASH FLOOD & COLD WATER IMMERSION (Hypothermia & Cold Shock)";
    scenarios[3].duration_sec     = 15.0f;
    scenarios[3].base_hr          = 75.0f;
    scenarios[3].hr_drift         = 65.0f;      /* Exertion tachycardia + cold shock up to 140 BPM */
    scenarios[3].base_spo2        = 98.0f;
    scenarios[3].spo2_drift       = -5.0f;      /* Drops to 93% under immersion strain */
    scenarios[3].hrv_rmssd_base   = 44.0f;
    scenarios[3].hrv_rmssd_drift  = -38.0f;     /* Autonomic collapse: RMSSD drops to 6 ms */
    scenarios[3].ambient_temp     = 12.0f;
    scenarios[3].temp_drift       = -6.0f;      /* Drops to 6 C in flood zone */
    scenarios[3].humidity         = 98.0f;      /* Saturated flood environment */
    scenarios[3].pm25             = 20.0f;
    scenarios[3].pm25_drift       = 0.0f;
    scenarios[3].skin_temp        = 35.5f;
    scenarios[3].skin_temp_drift  = -11.0f;     /* Rapid cutaneous cooling down to 24.5 C */

    num_scenarios = 4;

    /* Check for specific scenario flags */
    if (argc > 1) {
        if (strcmp(argv[1], "--hypothermia") == 0 || strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "4") == 0 || strcmp(argv[1], "flood") == 0) {
            start_scenario = 3;
            num_scenarios = 4;
        } else if (strcmp(argv[1], "--heat") == 0 || strcmp(argv[1], "2") == 0) {
            start_scenario = 1;
            num_scenarios = 2;
        } else if (strcmp(argv[1], "--smog") == 0 || strcmp(argv[1], "3") == 0) {
            start_scenario = 2;
            num_scenarios = 3;
        }
    }

    global_time = 0.0f;
    hrv_init(&hrv);

    /* Hide cursor for cleaner animation */
    printf("\033[?25l");

    for (s = start_scenario; s < num_scenarios; s++) {
        scenario_t *sc = &scenarios[s];
        float elapsed = 0.0f;

        while (elapsed < sc->duration_sec) {
            float progress = elapsed / sc->duration_sec;
            float bpm;
            float spo2;
            float temp;
            float pm25;
            float rmssd;
            float skin_temp;
            float ibi_ms;
            float jitter;
            env_sensors_t env;
            risk_assessment_t risk;
            nn_output_t nn_scores;

            /* Interpolate vitals across scenario timeline */
            bpm       = sc->base_hr        + sc->hr_drift        * progress;
            spo2      = sc->base_spo2      + sc->spo2_drift      * progress;
            temp      = sc->ambient_temp   + sc->temp_drift      * progress;
            pm25      = sc->pm25           + sc->pm25_drift      * progress;
            skin_temp = sc->skin_temp      + sc->skin_temp_drift * progress;
            rmssd     = sc->hrv_rmssd_base + sc->hrv_rmssd_drift * progress;
            if (rmssd < 1.0f) {
                rmssd = 1.0f;
            }

            /* Simulate IBI from BPM and feed to HRV engine */
            ibi_ms = 60000.0f / bpm;
            jitter = (float)(rand() % 20 - 10);  /* +/- 10ms variability */
            hrv_add_ibi(&hrv, ibi_ms + jitter);
            hrv_compute(&hrv);

            /* Set dynamic RMSSD for clean demonstration */
            hrv.rmssd = rmssd;

            /* Build environment sensor readings */
            memset(&env, 0, sizeof(env));
            env.ambient_temp_c = temp;
            env.humidity_pct   = sc->humidity;
            env.pm25           = pm25;
            env.skin_temp_c    = skin_temp;

            /* Run AI-powered neural network risk assessment */
            disaster_assess_nn(&hrv, spo2, bpm, &env, &risk);

            /* Get raw NN confidence scores for dashboard display */
            {
                const nn_model_t *model = nn_get_default_model();
                nn_predict(model, bpm, rmssd, spo2, temp, sc->humidity, pm25, &nn_scores);
            }

            /* Render dashboard */
            print_dashboard(global_time, sc->name, bpm, spo2,
                            hrv.rmssd, hrv.sdnn, &env, &risk, &nn_scores);

            /* Pace the simulation */
            SLEEP_MS(500);

            elapsed     += 1.0f;
            global_time += 1.0f;
        }
    }

    /* Show cursor again */
    printf("\033[?25h");

    printf("\n");
    printf(BOLD CYAN "================================================================\n");
    printf("  Simulation Complete - %s\n",
           start_scenario == 3 ? "Hypothermia / Flood Scenario" : "All Disaster Scenarios");
    printf("================================================================\n" RESET "\n");

    return 0;
}

