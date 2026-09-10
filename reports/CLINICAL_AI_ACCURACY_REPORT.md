# SIH26181 EdgeGuard: Clinical Triage & TinyML AI Engine Accuracy Report

**Benchmark dataset:** PhysioNet / MIT MIMIC-III Clinical Database v1.4 (demo subset)
**Evaluated set:** 16,387 vital time steps across 98 ICU subjects
**Status:** metrics below are **reproduced from committed code and committed data** — see [Reproduce](#reproduce) at the bottom. This file previously quoted a confusion matrix that the repository's own evaluator does not produce; it has been regenerated from an actual run.

---

## 0. Provenance — read this first

| Element | Status |
|---|---|
| `HR`, `SpO2` columns | **Real MIMIC-III `CHARTEVENTS` values.** Verified against the raw tables (e.g. subject 10013: SpO2 min 60, HR max 113, 82 rows — matching the feed exactly). |
| `RMSSD` column | **Synthetic.** MIMIC-III v1.4 contains no waveform / ECG / numerics tables, so beat-to-beat intervals cannot be derived from it. The feed has 16,384 distinct RMSSD values across 16,387 rows over only 1,433 distinct (HR, SpO2) pairs — essentially a unique float per row. |
| `ground_truth_crisis` labels | **Derived in-tree.** No upstream generator script is committed. Labels vary within a subject over time, consistent with a vitals-derived rule. |
| Scope | The **demo subset** (98 subjects), not the full MIMIC-III cohort. |
| What is actually evaluated | `clinical_vitals_engine.c` (mNEWS2-style triage) consuming HR / SpO2 / RMSSD. The SpO2 *engine* and the PPG front-end are **not** exercised by these numbers. |

**Conclusion:** treat these figures as *agreement with an in-tree labeling rule on real HR/SpO2 plus synthetic HRV*, not as independent clinical validation.

---

## 1. Headline metrics

| Metric | Value | Meaning |
| :--- | :--- | :--- |
| **Overall triage accuracy** | **94.11%** | (TP + TN) / total |
| **Sensitivity (recall)** | **58.18%** | Share of labeled-crisis rows flagged HIGH/CRITICAL |
| **Specificity** | **98.19%** | Share of labeled-stable rows correctly not flagged |
| **Positive predictive value** | **78.43%** | Of rows flagged, share actually labeled crisis |
| **Negative predictive value** | **95.39%** | Of rows not flagged, share actually labeled stable |
| **F1 score** | **66.80%** | Harmonic mean of precision and recall |
| **FP32↔INT8 decision agreement** | **100.00%** | Tier agreement between float32 and INT8 models |

> Sensitivity is deliberately the weakest axis here. With a 10.2%-positive label set (1,669 of 16,387), a triage rule tuned for low false alarms will under-call. If early-warning recall matters more than alarm fatigue for the intended use, the `CLINICAL_HIGH` thresholds in `clinical_vitals_engine.c` are the right place to trade specificity for sensitivity.

---

## 2. Confusion matrix

Ground truth: `ground_truth_crisis` in `data/mimic/mimic_eval_feed.csv`.
Predicted positive: `assess.level >= CLINICAL_HIGH`.

```
                        GROUND TRUTH (MIMIC-III derived label)
                      Positive (crisis)     Negative (stable)
                 +-----------------------+-----------------------+
 PREDICTED       |   True Positive (TP)  |  False Positive (FP)  |
 CRISIS (HIGH /  |          971          |          267          |  PPV: 78.43%
 CRITICAL)       +-----------------------+-----------------------+
                 |  False Negative (FN)  |   True Negative (TN)  |
 PREDICTED       |          698          |        14,451         |  NPV: 95.39%
 NORMAL/ELEVATED +-----------------------+-----------------------+
                    Sensitivity: 58.18%     Specificity: 98.19%
                              Total: 16,387   Accuracy: 94.11%
```

Row counts check out: 971 + 267 + 698 + 14,451 = 16,387.

---

## 3. Cohort triage distribution (`mimic_harness.c`)

| Level | Rows |
| :--- | ---: |
| NORMAL | 7,170 |
| ELEVATED | 7,979 |
| HIGH | 852 |
| CRITICAL | 386 |
| **Alert flags** | |
| Hypoxia | 889 |
| Tachycardia | 1,974 |
| Bradycardia | 35 |
| Autonomic shock | 8 |

Average evaluation cost: **0.610 µs per record** (x86-64 host, `-O2`). `mimic_harness.c` counts triage levels and timing only — it does **not** compute accuracy. Accuracy comes from `accuracy_evaluator.c`.

---

## 4. TinyML INT8 quantization fidelity

Measured by `accuracy_evaluator.c` over all 16,387 rows, comparing `nn_predict()` (float32) against `nn_predict_int8()` (deployed integer pipeline), both on the same inputs:

| Subnet | Mean absolute error | Max error |
| :--- | ---: | ---: |
| Heat strain | 0.00120 | 0.00796 |
| Pollution / respiratory | 0.00122 | — |
| Flood / cold exposure | 0.00139 | — |
| **Tier decision agreement** | **100.00%** | — |

Errors are on a [0, 1] normalized probability scale. These are consistent with the model's own reported INT8 fidelity (`int8_mae` 0.0079, `int8_max_err` 0.0914 over the synthetic validation set) and with `test_int8_matches_float_nn()`, which asserts a 0.18 absolute-error budget in the C unit tests.

---

## 5. Reproduce

```bash
# 1. Triage accuracy + confusion matrix + quantization fidelity
gcc -O2 -Ifirmware/core -o acc_eval \
    firmware/core/accuracy_evaluator.c firmware/core/clinical_vitals_engine.c \
    firmware/core/hrv_analysis.c firmware/core/spo2_engine.c \
    firmware/core/disaster_risk_engine.c firmware/core/nn_risk_model.c \
    firmware/core/nn_risk_model_int8.c -lm
./acc_eval data/mimic/mimic_eval_feed.csv
# -> accuracy 94.11, tp 971, fp 267, tn 14451, fn 698

# 2. Cohort distribution + per-record latency
gcc -O2 -Ifirmware/core -o mimic_harness \
    firmware/core/mimic_harness.c firmware/core/clinical_vitals_engine.c -lm
./mimic_harness data/mimic/mimic_vital_feed.csv

# 3. Model regeneration (must reproduce the committed weights)
python3 firmware/core/train_nn_risk_model.py
```

CI (`.github/workflows/ci.yml`) runs steps 1 and 3 and asserts an accuracy floor.
