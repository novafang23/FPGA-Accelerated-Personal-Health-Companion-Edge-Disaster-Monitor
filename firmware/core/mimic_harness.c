#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "clinical_vitals_engine.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: mimic_harness <input_csv>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Cannot open %s\n", argv[1]);
        return 1;
    }

    char line[256];
    // Skip header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 1;
    }

    unsigned long total_records = 0;
    unsigned long normal_count = 0;
    unsigned long elevated_count = 0;
    unsigned long high_count = 0;
    unsigned long critical_count = 0;

    unsigned long hypoxia_count = 0;
    unsigned long tachy_count = 0;
    unsigned long brady_count = 0;
    unsigned long shock_count = 0;

    clock_t t0 = clock();

    clinical_assessment_t assess;
    while (fgets(line, sizeof(line), f)) {
        int pid;
        float hr, spo2, rmssd;
        // Format: pid,hr,spo2,rmssd
        if (sscanf(line, "%d,%f,%f,%f", &pid, &hr, &spo2, &rmssd) != 4) continue;

        total_records++;
        clinical_vitals_assess(hr, spo2, rmssd, &assess);

        switch (assess.level) {
            case CLINICAL_NORMAL:   normal_count++; break;
            case CLINICAL_ELEVATED: elevated_count++; break;
            case CLINICAL_HIGH:     high_count++; break;
            case CLINICAL_CRITICAL: critical_count++; break;
        }

        if (assess.alert_flags & ALERT_HYPOXIA) hypoxia_count++;
        if (assess.alert_flags & ALERT_TACHYCARDIA) tachy_count++;
        if (assess.alert_flags & ALERT_BRADYCARDIA) brady_count++;
        if (assess.alert_flags & ALERT_AUTONOMIC_SHOCK) shock_count++;
    }
    fclose(f);

    clock_t t1 = clock();
    double total_time_sec = (double)(t1 - t0) / CLOCKS_PER_SEC;
    double us_per_eval = (total_records > 0) ? (total_time_sec * 1e6 / total_records) : 0.0;

    printf("RESULTS_JSON:{\"total\":%lu,\"normal\":%lu,\"elevated\":%lu,\"high\":%lu,\"critical\":%lu,\"hypoxia\":%lu,\"tachy\":%lu,\"brady\":%lu,\"shock\":%lu,\"time_sec\":%.4f,\"us_per_eval\":%.3f}\n",
           total_records, normal_count, elevated_count, high_count, critical_count,
           hypoxia_count, tachy_count, brady_count, shock_count, total_time_sec, us_per_eval);

    return 0;
}
