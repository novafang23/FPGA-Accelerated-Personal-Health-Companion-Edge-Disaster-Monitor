# AI Model Audit — SIH26181 VALOR TinyML stack

**Date:** 2026-10-20
**Scope:** every AI/ML artefact in the repository: the hazard network (FP32 reference + deployed INT8),
the PM2.5 calibration network, the training pipeline, the risk engines that consume them, and the
documentation that describes them.
**Method:** static review of every file, plus an **independent re-implementation of the deployed C
kernels in Python** (parsing the committed weight files, not the trainer's in-memory model), an
independent re-run of the training pipeline, a feature-sensitivity sweep, an adversarial-input sweep,
and an end-to-end build of the real firmware.

Every number below was measured on this machine on 2026-10-20. Nothing is quoted from the model's own
training log unless it was independently reproduced.

---

## 1. Verdict summary

| Question | Verdict | Evidence |
| :--- | :--- | :--- |
| Is the model actually used on the device? | **Yes** — and after this audit, *only* the INT8 model is in the image | §3, link map |
| Is the training reproducible? | **Yes** — byte-identical artefacts on a fresh run | §5 |
| Is the INT8 implementation faithful to the reference? | **Yes** — max error 0.0914, 97.32% tier agreement | §4 |
| Does the deployed model do what it claims? | **Yes for heat, partially for pollution, weak for flood** | §6 |
| Are there errors? | **Three found and fixed** (§2), one judgement call left open (§8) | §2 |
| Is it useful? | **As an advisory second opinion only** — it is not more accurate than the rule engine it distils from | §7 |

---

## 2. Findings and what was done about them

| # | Finding | Severity | Action |
| :--- | :--- | :--- | :--- |
| F1 | **A withheld RMSSD was scored as a collapsed RMSSD.** `normalize(0, NN_RMSSD_MIN=1.0, …)` clamps to the *most-suppressed* end of the feature, so "not measured" fed the network as "HRV collapsed". | High (fabricated input) | **Fixed** (commit `7eaf798`): both network entry points now require a published RMSSD and decline with `RISK_UNKNOWN`; `[TRIAGE]` carries `AIVALID=0/1`; both dashboards render `--`. |
| F2 | **A non-finite or implausible sensor value produced a finite, plausible score.** A NaN humidity made every layer-1 pre-activation NaN; the uint8 requantiser clamped it to 0, so the network returned the bias vector as a "measurement" (measured: heat 0.008 / pollution 0.191 / flood 0.034). The same path scored `hr = 0` (no finger) as flood 0.31 MODERATE, and clamped out-of-range values into the input cube. | Medium (silent fabrication) | **Fixed**: `nn_inputs_valid()` in `firmware/core/disaster_risk_engine.c` refuses non-finite values and anything outside bpm 30-220, ambient -20..65 C, humidity 0-100 % and SpO2 50-100 % (exactly 0 still means "oximeter not locked"), returning `RISK_UNKNOWN` with "AI assessment declined: <reason>". The `hr = 0` case is reachable on the device (valid ambient, no finger), so this is a real fix, not only defence in depth. |
| F3 | **The float32 model was dead payload in the shipped image.** `nn_predict()` has no caller on the device; the 2.5 kB of float weights shipped for nothing. | Low (wasted flash, misleading) | **Fixed**: the ESP32 build now compiles `nn_risk_model_int8.c` only (`NN_ENABLE_FLOAT32_MODEL=0`); the link map confirms `nn_risk_model.c.obj` is absent. |
| F4 | **The documentation described a kernel the device does not run.** `docs/JUDGE_QA.md` and `docs/PROJECT_GUIDE.md` claimed `[-128,127]` int8 inputs, int32 accumulation, fixed-point shifts and a 32-step sigmoid LUT for the hazard model. The real kernel min-max normalizes to `[0,1]`, dequantizes the INT8 weights, multiplies in **float**, and calls **`expf()`**. (The `[-128,127]`/int32/LUT description belongs to the *PM2.5 calibration* net.) | **High (a judge could disprove it by reading the code)** | **Fixed**: both documents now describe the real pipeline, and the link map is cited as the proof that the deployed sigmoid is a real exponential. |
| F5 | **Several published numbers did not match reality.** "88.47% (INT8)" (that is the float32 reference; the shipped INT8 model is 88.39%), "6,200-sample held-out split" (it is 7,000), "r = 0.97/0.97/0.94" (measured 0.99/0.98/0.97), "~18 µs on the ESP32-S3" (never measured), "0.38 µs" (0.44 µs is the quoted host figure), "rule/NN 99.95%" (that is the MIMIC-III FP32↔INT8 agreement). | **High (credibility)** | **Fixed** in `README.md`, `docs/JUDGE_QA.md`, `docs/PROJECT_GUIDE.md`, `docs/MASTER_PROJECT_GUIDE.md`, the dashboard footnote, and the presentation scripts. |
| F6 | **The weight-file header called the validation MSE `train_mse`** and quoted only the float32 accuracy inside the *INT8* weight file. | Medium | **Fixed**: the trainer now emits `val_mse`, and both numbers — `val_accuracy` (FP32 reference) and `int8_teacher_accuracy` (shipped model) — with a CI floor on the INT8 figure (`--min-int8-accuracy`). |
| F7 | The 619-byte network ignores humidity almost entirely (layer-1 weight mass 0.21 versus 5.78 for PM2.5); a full-range humidity sweep moves the outputs by ≤0.004. | Low (a modelling limitation, not a bug) | Documented, not "fixed": it is a property of the distilled teacher, which itself barely uses humidity. Recorded in §6 and in the judge notes. |
| F8 | The PM2.5 calibration network can return a lower reading than the sensor produced, and can return 0.0 where the raw reading was 20. | **Open — judgement call** | See §8. |

---

## 3. Is it actually used? (wiring map)

```
MAX30102 ─┐
BME280 ───┼─> main_shrikefi.c  (1 Hz clinical loop)
PMS5003 ──┤        │
MPU6050 ──┘        ├─> clinical_vitals_assess_ext()  ── NEWS2 ──┐
                  │                                            │
                  ├─> disaster_assess()      (rules) ──────────┼─> clinical_fuse_triage()
                  ├─> disaster_assess_nn_int8() (INT8 NN) ─────┤        │
                  └─> hrv_is_settled() gate (240 beats/120 s)  │        └─> [TRIAGE] RISK=
                                                                       -> dashboard, SoftAP, SOS
```

* `disaster_assess_nn_int8()` is called from `firmware/shrikefi/main_shrikefi.c` (device) and
  `firmware/shrikefi/shrikefi_dashboard.c` (the Win32 dashboard re-runs the model locally, so the
  desktop app needs no network).
* The per-modality network levels reach the wire as `NNHEAT/NNPOLL/NNFLOOD` on `[TRIAGE]` and are
  parsed by the web dashboard. The **fused** verdict uses only `nn_risk.overall_risk`; the network's
  per-modality levels are shown, not re-fused.
* **Removed dead code (this audit):** the float32 model and `nn_predict()` are no longer compiled
  into the device image; `g_state.nn_scores` is written but never read and has been left alone only
  because it is harmless and cheap to carry.
* `accuracy_evaluator.c` (the MIMIC-III harness) is not in any device build — it is a bench tool,
  which is correct.

**Link-map proof (shipped image, `build/shrikefi_health_companion.map`):**

```
esp-idf/main/libmain.a(nn_risk_model_int8.c.obj) (expf)
esp-idf/main/libmain.a(nn_risk_model_int8.c.obj) (roundf)
```

`nn_risk_model.c.obj` is absent, and the only math functions the model pulls in are `expf` and
`roundf` — i.e. the deployed sigmoid is a real exponential and the requantiser uses `roundf`.

---

## 4. Is the implementation correct?

An independent Python re-implementation of **both** C kernels was written from the committed weight
files (not from the trainer's in-memory model), including the C semantics that differ from numpy:
`roundf` is half-away-from-zero (numpy is half-to-even), the uint8 requantiser clamps to 0…255 and
maps non-finite to 0, and the float path clamps the sigmoid argument to ±10.

| Measurement | Value |
| :--- | :--- |
| INT8 vs FP32 mean absolute error | **0.00790** |
| INT8 vs FP32 max absolute error | **0.09136** |
| INT8 vs FP32 tier agreement | **97.32%** |
| Deployed INT8 vs teacher, overall | **88.39%** (FP32 reference 88.47%) |
| Deployed INT8 vs teacher, per axis | heat **91.99%**, pollution **87.20%**, flood **85.97%** |
| Correlation vs teacher, per axis | heat **r = 0.99**, pollution **r = 0.98**, flood **r = 0.97** |
| Continuous MAE vs teacher, per axis | heat 0.028, pollution 0.044, flood 0.043 |
| Validation MSE | 0.002979 |

**Quantization audit.** Per-tensor symmetric INT8 weights (zero-point 0) with per-layer activation
scales: W1 0.03131007, act1 0.00622647, act2 0.01018247, act3 0.00382716. Representable ranges:
act1 0…1.5877, act2 0…2.5965, act3 0…0.9759. No overflow of the uint8 activation range was observed
anywhere in the validation set or in the adversarial sweep.

**Structure.** 11 of 24 first-layer rows and 6 of 16 second-layer rows are all-zero (dead neurons
from the distillation) — a third of the network's parameters do nothing. That is not a bug (it is
what distillation to a smooth teacher produces) but it means the honest description is "619 bytes
of parameter storage, of which 17 rows are inactive".

---

## 5. Is it reproducible?

A fresh run of `python firmware/core/train_nn_risk_model.py` in an isolated directory reproduced all
three generated artefacts **byte for byte** (SHA-256 identical):
`nn_risk_model_trained.c.inc`, `nn_risk_model_int8.c.inc`, `nn_risk_model_int8.h`, and the printed
metrics (`val_mse 0.002979`, `val_accuracy 88.47%`, `int8_mae 0.00790`, `int8_max_err 0.09136`,
`tier_agreement 97.32%`) and the `MODEL_METRICS_JSON` line. The three archetype scenarios pass.

The CI job `nn-train` regenerates the weights and diffs them against the committed files, so a drift
is visible in the build log.

---

## 6. What does the model actually compute?

Feature-sensitivity sweeps over the full legal input range, using the deployed INT8 kernel:

| Feature | Effect on the outputs | Reading |
| :--- | :--- | :--- |
| Ambient temperature | heat 0.000 → 0.413, flood 0.019 → 0.425 | dominant, as intended; heat is correctly 0.000 below 27 °C |
| PM2.5 | pollution 0.000 → 0.398 | dominant; **not monotone**: the network returns 0.100 at PM2.5 = 35 where the teacher returns 0. The teacher's hard "clean air ≤ 35 ⇒ 0" guard is not representable by a smooth network |
| HR | strong influence on all three axes | as intended |
| RMSSD | heat 0.597 at 50 ms → **0.884 at 0 ms** | this is the quantitative reason for F1: RMSSD = 0 is the most-suppressed end of the feature, not a neutral one |
| SpO₂ | small effect on pollution (the clean-air guard dominates) | weak |
| Humidity | ≤ 0.004 across the whole range | effectively ignored (F7) |

**Where the network is weaker than the rules it was distilled from** (per-axis tier agreement with
the teacher, on the validation set):

| Axis | Rule engine | INT8 network |
| :--- | :--- | :--- |
| Heat | 82.39% | **92.69%** |
| Pollution | **99.09%** | 87.11% |
| Flood | 78.16% | **86.40%** |
| Overall (fused verdict) | **85.57%** | 81.56% |

The network is *better* on heat and flood and *worse* on pollution (because it cannot reproduce the
teacher's hard clean-air step). It is not a better risk engine than the rules; it is a second,
independent opinion.

**Adversarial inputs.** Temperature ±1e6, SpO₂ 0, HR 0 and 5000, PM2.5 1e7, NaN in each input, and
all-extremes: no crash, no hang, all outputs finite. RMSSD = NaN, 0 or negative is now declined
(`RISK_UNKNOWN`) by both entry points.

---

## 7. Is it useful?

Fusion is "worst wins": the environmental verdict is the rule engine's, raised if the network is
higher. Measured on the validation set:

* The network changes the fused verdict in **3.61%** of cases (always upward, by construction).
* On the 253 cases where it *does* change the verdict, the rule engine matches the teacher 58.5% of
  the time and the network 5.9% — the network's additions are more often wrong than right, because
  the rule engine is already the teacher it was distilled from.
* On the seven documented demo archetypes, the rule engine and the network agree on the overall
  level in **6 of 7**. They differ only on *hypothermia* (rules: HIGH, network: CRITICAL).

**Conclusion:** the network earns its place as an *independent second opinion that can add caution*,
not as a decision maker. The current fusion policy (may raise, never lower) is the right one for
that role, and the dashboard shows the network's own scores next to the rule engine's so the
difference is visible rather than hidden.

---

## 8. Open judgement call (RESOLVED later the same day)

> **Update, 2026-10-20 (later the same day):** this judgement call was taken and the
> private PM2.5 calibration network was **deleted**. It was replaced by the published
> EPA/Barkjohn (2021) correction in the PMS5003 "atm" form published by AirGradient,
> applied to a 60 s rolling mean (`firmware/core/pm25_correction.c`). The reasoning,
> the measured behaviour of the old model and the literature are in
> `docs/reports/PM25_CALIBRATION_2026-10-20.md`. The text below is kept as the audit
> record of the state at the time of the AI audit.

The **PM2.5 calibration network** (`firmware/core/pm25_calibration_int8.c`) is a different model with
a different role: it turns a raw optical PMS5003 reading into a calibrated concentration. Measured
on the real 0–500 µg/m³ range it can return a value **up to ~70% lower** than the raw reading
(raw 500 → 153), and at high humidity it can return 0.0 for a raw reading of 20 — a "clean" result
for a day that is not clean. That value then feeds the pollution rule and the pollution network.

The calibration is a legitimate correction (a real optical PM sensor does over-read), but applying
it *and* then treating the result as a measurement is the part that can mislead. Options were listed
in `docs/REMAINING_WORK.md`; no behaviour was changed by the AI audit itself, because the right answer depends on how
much of the sensor's own correction the team wants to keep.

---

## 9. Reproducing this audit

```bash
# full training + the model metrics
python firmware/core/train_nn_risk_model.py

# C-level tests (includes the INT8-vs-FP32 parity test)
make -C firmware/zynq test          # -> ALL TESTS PASSED.

# device build (INT8 only) and the link-map check
idf.py -C firmware/shrikefi build
grep nn_risk build/shrikefi_health_companion.map
```

The independent Python re-implementation used for §4–§7 is a scratch tool (it parses the committed
`.inc` files and re-implements the C kernels) and is not part of the repository; the numbers it
produced are reproduced above so they can be re-derived from the artefacts in the repo.

## 10. Evidence index

| Claim | Where |
| :--- | :--- |
| Deployed model = 619 bytes, INT8, 6→24→16→3 | `firmware/core/nn_risk_model_int8.c.inc`, `firmware/core/nn_risk_model_int8.h` |
| Trainer, teacher, dataset, metrics | `firmware/core/train_nn_risk_model.py` |
| Guard: no published RMSSD ⇒ no score | `firmware/core/disaster_risk_engine.c` (`disaster_assess_nn`, `disaster_assess_nn_int8`) |
| Guard: non-finite or implausible input ⇒ no score | `firmware/core/disaster_risk_engine.c` (`nn_inputs_valid`) |
| Device call site | `firmware/shrikefi/main_shrikefi.c` (1 Hz clinical loop) |
| Fusion | `firmware/core/clinical_vitals_engine.c` (`clinical_fuse_triage`) |
| Link map of the shipped image | `firmware/shrikefi/build/shrikefi_health_companion.map` |
| CI gate | `.github/workflows/ci.yml` (`nn-train` job) |
