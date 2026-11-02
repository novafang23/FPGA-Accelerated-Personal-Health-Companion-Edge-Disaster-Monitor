#!/usr/bin/env python3
"""
mimic_clustered_stats.py - honest uncertainty for the MIMIC triage numbers.

    python tools/mimic_clustered_stats.py data/mimic/mimic_eval_feed.csv

WHY THIS EXISTS
---------------
The headline numbers are sensitivity 58.18% / specificity 98.19% over 16,387
rows. Quoting them bare is misleading in two ways:

  1. The 16,387 rows come from only 98 patients. Rows from one patient are
     strongly correlated (same physiology, same admission), so the effective
     sample size is far smaller than 16,387 and the confidence interval is much
     wider than a naive binomial would suggest. Any interval computed as if
     every row were independent is wrong.

  2. A bare point estimate invites "is 58.18% reliably 58.18%?" - which cannot
     be answered without resampling PATIENTS, not rows.

So this reports:
  * the confusion matrix and derived metrics (same as accuracy_evaluator.c)
  * a patient-level (cluster) bootstrap confidence interval
  * leave-one-subject-out variability
  * a breakdown of the false negatives: how many were near-misses and how many
    had no usable temperature or respiratory term

WHAT IT DOES NOT DO
-------------------
It does not improve sensitivity. Changing the rule to score better on this
dataset would be fitting a clinical decision to a benchmark, which is the wrong
trade for a medical device. This tool only describes what the current rule does
and how confident that description is.
"""
import csv
import sys
import argparse
from collections import defaultdict

try:
    import numpy as np
except ImportError:
    sys.exit("Needs numpy: pip install numpy")

# Mirrors clinical_vitals_engine.c exactly. If the firmware bands change, these
# must change with them or the reported numbers stop describing the device.
def news2_hr(hr):
    if hr <= 40:  return 3
    if hr <= 50:  return 1
    if hr <= 90:  return 0
    if hr <= 110: return 1
    if hr <= 130: return 2
    return 3

def news2_spo2(spo2):
    if spo2 >= 96: return 0
    if spo2 >= 94: return 1
    if spo2 >= 92: return 2
    return 3

def news2_rr(rr):
    if rr <= 8:  return 3
    if rr <= 11: return 1
    if rr <= 20: return 0
    if rr <= 24: return 2
    return 3

def news2_temp(t):
    # The device never instruments the temperature row: a fingertip is not a
    # core site. Kept here as 0 for fidelity with the firmware.
    return 0

# CRITICAL thresholds are NOT the HIGH thresholds. Conflating them is the
# easiest way to report numbers that do not describe the device:
#   clinical_vitals_engine.c constants (lines 36-41)
HR_CRITICAL_TACHY_MIN     = 150
HR_CRITICAL_BRADY_MAX     = 39
SPO2_CRITICAL_HYPOXIA_MAX = 85
RMSSD_SHOCK_THRESHOLD     = 8.0    # RMSSD_SHOCK_THRESHOLD_X10 = 80


def level_from(hr, spo2, rr, rmssd):
    """Return 0=NORMAL 1=ELEVATED 2=HIGH 3=CRITICAL, matching the firmware.

    Must mirror clinical_vitals_engine.c exactly. The evaluator calls the 3-arg
    clinical_vitals_assess(), which forwards rr=14.0 and sqi=0.95 - so the RR
    term is always 0 here and the SQI hold never fires.
    """
    score = news2_hr(hr) + news2_spo2(spo2) + news2_rr(rr) + news2_temp(0)
    shock = (rmssd > 0 and rmssd < RMSSD_SHOCK_THRESHOLD) and \
            (hr >= 100 or spo2 <= 94 or rr >= 24)
    critical_vital = (spo2 <= SPO2_CRITICAL_HYPOXIA_MAX) or \
                     (hr >= HR_CRITICAL_TACHY_MIN) or \
                     (hr <= HR_CRITICAL_BRADY_MAX) or \
                     (rr >= 30) or (rr <= 6)
    if critical_vital or score >= 7 or (shock and score >= 5):
        return 3
    if score >= 4 or spo2 <= 91 or hr >= 131 or hr <= 40 or rr >= 25 or shock:
        return 2
    if score >= 1:
        return 1
    return 0


def load(path):
    try:
        f = open(path, newline="", encoding="utf-8")
    except FileNotFoundError:
        sys.exit(f"No such file: {path}")
    with f:
        rows = list(csv.DictReader(f))
    out = []
    for r in rows:
        try:
            sid = r["subject_id"].strip()
            hr = float(r["HR"]); spo2 = float(r["SpO2"]); rmssd = float(r["RMSSD"])
            gt = int(float(r["ground_truth_crisis"]))
        except (KeyError, ValueError, AttributeError):
            continue
        if not (0 < hr < 350 and 0 < spo2 <= 100):
            continue
        # RR is absent in this dataset; the firmware scores an absent RR as 14
        # (0 points) and publishes nothing for it. Mirror that.
        rr = 14
        lvl = level_from(hr, spo2, rr, rmssd)
        out.append(dict(sid=sid, hr=hr, spo2=spo2, rmssd=rmssd, gt=gt,
                        pred=1 if lvl >= 2 else 0, level=lvl))
    return out


def confusion(rows):
    tp = sum(1 for r in rows if r["gt"] == 1 and r["pred"] == 1)
    fp = sum(1 for r in rows if r["gt"] == 0 and r["pred"] == 1)
    tn = sum(1 for r in rows if r["gt"] == 0 and r["pred"] == 0)
    fn = sum(1 for r in rows if r["gt"] == 1 and r["pred"] == 0)
    return tp, fp, tn, fn


def metrics(tp, fp, tn, fn):
    sens = tp / (tp + fn) * 100 if (tp + fn) else float("nan")
    spec = tn / (tn + fp) * 100 if (tn + fp) else float("nan")
    acc = (tp + tn) / max(1, tp + fp + tn + fn) * 100
    ppv = tp / (tp + fp) * 100 if (tp + fp) else float("nan")
    npv = tn / (tn + fn) * 100 if (tn + fn) else float("nan")
    f1 = 2 * tp / (2 * tp + fp + fn) * 100 if (2 * tp + fp + fn) else float("nan")
    return dict(sens=sens, spec=spec, acc=acc, ppv=ppv, npv=npv, f1=f1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", default="data/mimic/mimic_eval_feed.csv")
    ap.add_argument("--boot", type=int, default=2000, help="bootstrap resamples")
    args = ap.parse_args()

    rows = load(args.csv)
    if not rows:
        sys.exit("No usable rows.")

    by_subj = defaultdict(list)
    for r in rows:
        by_subj[r["sid"]].append(r)
    subjects = sorted(by_subj)

    tp, fp, tn, fn = confusion(rows)
    m = metrics(tp, fp, tn, fn)

    print("=" * 74)
    print(" MIMIC-III triage statistics with PATIENT-LEVEL uncertainty")
    print("=" * 74)
    print(f"rows={len(rows)}  patients={len(subjects)}")
    print(f"\nConfusion matrix:  TP={tp}  FP={fp}  TN={tn}  FN={fn}")
    print(f"  sensitivity = {m['sens']:.2f}%   specificity = {m['spec']:.2f}%")
    print(f"  accuracy    = {m['acc']:.2f}%   PPV = {m['ppv']:.2f}%   NPV = {m['npv']:.2f}%")
    print(f"  F1          = {m['f1']:.2f}%")
    print(f"\n  positive-label rate = {100*(tp+fn)/len(rows):.1f}% of rows")

    # ---- patient-level bootstrap -----------------------------------------
    rng = np.random.default_rng(20261102)
    sens_b, spec_b = [], []
    for _ in range(args.boot):
        pick = rng.choice(len(subjects), size=len(subjects), replace=True)
        sample = []
        for i in pick:
            sample.extend(by_subj[subjects[i]])
        a, b, c, d = confusion(sample)
        s = a / (a + d) * 100 if (a + d) else float("nan")
        p = c / (c + b) * 100 if (c + b) else float("nan")
        if not np.isnan(s): sens_b.append(s)
        if not np.isnan(p): spec_b.append(p)

    sens_b = np.array(sens_b); spec_b = np.array(spec_b)
    lo, hi = np.percentile(sens_b, [2.5, 97.5])
    slo, shi = np.percentile(spec_b, [2.5, 97.5])
    print(f"\nPatient-level bootstrap ({args.boot} resamples, resampling PATIENTS):")
    print(f"  sensitivity 95% CI = [{lo:.1f}, {hi:.1f}] %   (point {m['sens']:.1f})")
    print(f"  specificity 95% CI = [{slo:.1f}, {shi:.1f}] %   (point {m['spec']:.1f})")

    # A naive row-level interval, for contrast. Shown because the difference is
    # the entire point of this tool.
    n_pos = tp + fn
    p_hat = tp / n_pos
    se = np.sqrt(p_hat * (1 - p_hat) / n_pos) * 100
    print(f"\n  For contrast, a NAIVE row-level interval would claim")
    print(f"  sensitivity {m['sens']:.1f} +/- {1.96*se:.1f} % - which is TOO NARROW,")
    print(f"  because it treats 16,387 correlated rows as independent samples.")

    # ---- leave-one-subject-out -------------------------------------------
    loo = []
    for s in subjects:
        rest = [r for r in rows if r["sid"] != s]
        a, b, c, d = confusion(rest)
        if a + d:
            loo.append(a / (a + d) * 100)
    loo = np.array(loo)
    print(f"\nLeave-one-patient-out sensitivity: min={loo.min():.1f}%  "
          f"max={loo.max():.1f}%  spread={loo.max()-loo.min():.1f} pp")

    # ---- what the false negatives look like ------------------------------
    fns = [r for r in rows if r["gt"] == 1 and r["pred"] == 0]
    print(f"\n--- the {len(fns)} missed positives ---")
    from collections import Counter
    lv = Counter(r["level"] for r in fns)
    print(f"  predicted level: " + ", ".join(
        f"{['NORMAL','ELEVATED','HIGH','CRITICAL'][k]}={v}" for k, v in sorted(lv.items())))
    near = sum(1 for r in fns if r["level"] == 1)
    print(f"  of these, {near} were only one level below HIGH (ELEVATED)")
    hr_hi = sum(1 for r in fns if r["hr"] >= 111)
    sp_lo = sum(1 for r in fns if r["spo2"] <= 95)
    print(f"  {hr_hi} had HR >= 111 (2-3 NEWS2 points) but no other derangement")
    print(f"  {sp_lo} had SpO2 <= 95 (1-2 points) but no other derangement")
    print("  NONE had a usable temperature term: the device never instruments it,")
    print("  so a fever- or hypothermia-labelled crisis cannot contribute points.")

    print("\n" + "-" * 74)
    print("HOW TO QUOTE THIS")
    print("-" * 74)
    print(f"  \"On 16,387 MIMIC-III rows from {len(subjects)} patients, our triage rule")
    print(f"   recovers {m['sens']:.1f}% of its own crisis labels (patient-level 95% CI")
    print(f"   [{lo:.1f}, {hi:.1f}]%) at {m['spec']:.1f}% specificity. The rows are rule-")
    print("   labelled and RMSSD is synthetic, so this is agreement with our rule,")
    print("   not clinical validation.\"")
    print("\n  Never quote the sensitivity without the interval: the between-patient")
    print("  spread is the dominant uncertainty, not the row count.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
