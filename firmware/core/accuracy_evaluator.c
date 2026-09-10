#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clinical_vitals_engine.h"
#include "nn_risk_model.h"
#include "nn_risk_model_int8.h"

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;

    char line[256];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 1; }

    unsigned long total = 0;
    // Confusion matrix for Clinical Engine (HIGH or CRITICAL = Predicted Positive)
    unsigned long tp = 0, fp = 0, tn = 0, fn = 0;

    // Quantization discrepancy between FP32 and INT8 TinyML model
    double max_err_heat = 0.0, max_err_poll = 0.0, max_err_flood = 0.0;
    double sum_err_heat = 0.0, sum_err_poll = 0.0, sum_err_flood = 0.0;
    unsigned long agreement_count = 0;

    clinical_assessment_t assess;
    nn_output_t out_fp32;
    nn_output_t out_int8;

    const nn_model_t *model_fp32 = nn_get_default_model();

    while (fgets(line, sizeof(line), f)) {
        int pid, gt;
        float hr, spo2, rmssd;
        if (sscanf(line, "%d,%f,%f,%f,%d", &pid, &hr, &spo2, &rmssd, &gt) != 5) continue;
        total++;

        // 1. Evaluate Clinical Engine
        clinical_vitals_assess(hr, spo2, rmssd, &assess);
        int pred_positive = (assess.level >= CLINICAL_HIGH) ? 1 : 0;

        if (gt == 1 && pred_positive == 1) tp++;
        else if (gt == 0 && pred_positive == 1) fp++;
        else if (gt == 0 && pred_positive == 0) tn++;
        else if (gt == 1 && pred_positive == 0) fn++;

        // 2. Evaluate AI Engine (FP32 vs INT8 Quantized model fidelity)
        nn_predict(model_fp32, hr, rmssd, spo2, 25.0f, 50.0f, 15.0f, &out_fp32);
        nn_predict_int8(&nn_default_model_int8, &nn_quant_params, hr, rmssd, spo2, 25.0f, 50.0f, 15.0f, &out_int8);

        double err_h = fabs((double)out_fp32.heat_score - (double)out_int8.heat_score);
        double err_p = fabs((double)out_fp32.pollution_score - (double)out_int8.pollution_score);
        double err_f = fabs((double)out_fp32.flood_score - (double)out_int8.flood_score);

        sum_err_heat += err_h;
        sum_err_poll += err_p;
        sum_err_flood += err_f;

        if (err_h > max_err_heat) max_err_heat = err_h;
        if (err_p > max_err_poll) max_err_poll = err_p;
        if (err_f > max_err_flood) max_err_flood = err_f;

        // Decision agreement: Do FP32 and INT8 models classify into the same risk tier?
        int tier_fp32 = (out_fp32.heat_score > 0.6f) ? 3 : ((out_fp32.heat_score > 0.4f) ? 2 : 1);
        int tier_int8 = (out_int8.heat_score > 0.6f) ? 3 : ((out_int8.heat_score > 0.4f) ? 2 : 1);
        if (tier_fp32 == tier_int8) agreement_count++;
    }
    fclose(f);

    double sensitivity = (tp + fn > 0) ? ((double)tp / (tp + fn) * 100.0) : 0.0;
    double specificity = (tn + fp > 0) ? ((double)tn / (tn + fp) * 100.0) : 0.0;
    double ppv = (tp + fp > 0) ? ((double)tp / (tp + fp) * 100.0) : 0.0;
    double npv = (tn + fn > 0) ? ((double)tn / (tn + fn) * 100.0) : 0.0;
    double accuracy = (total > 0) ? ((double)(tp + tn) / total * 100.0) : 0.0;
    double f1 = (tp + fp + fn > 0) ? (2.0 * tp / (2.0 * tp + fp + fn) * 100.0) : 0.0;

    double mae_heat = (total > 0) ? (sum_err_heat / total) : 0.0;
    double mae_poll = (total > 0) ? (sum_err_poll / total) : 0.0;
    double mae_flood = (total > 0) ? (sum_err_flood / total) : 0.0;
    double agreement_pct = (total > 0) ? ((double)agreement_count / total * 100.0) : 0.0;

    printf("METRICS_JSON:{\"total\":%lu,\"tp\":%lu,\"fp\":%lu,\"tn\":%lu,\"fn\":%lu,\"sensitivity\":%.2f,\"specificity\":%.2f,\"ppv\":%.2f,\"npv\":%.2f,\"accuracy\":%.2f,\"f1\":%.2f,\"mae_heat\":%.5f,\"mae_poll\":%.5f,\"mae_flood\":%.5f,\"max_err_heat\":%.5f,\"agreement_pct\":%.2f}\n",
           total, tp, fp, tn, fn, sensitivity, specificity, ppv, npv, accuracy, f1,
           mae_heat, mae_poll, mae_flood, max_err_heat, agreement_pct);

    return 0;
}
