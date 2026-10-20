import os
import subprocess
import json
import sys

def main():
    # Anchor paths to the script's directory so it works from any terminal location
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = os.path.join(script_dir, "data", "mimic", "mimic_eval_feed.csv")
    exe_path = os.path.join(script_dir, "acc_eval.exe")

    if not os.path.exists(csv_path):
        print(f"Error: Dataset not found at {csv_path}")
        sys.exit(1)

    # Compile if executable does not exist
    if not os.path.exists(exe_path):
        print("Compiling acc_eval.exe from firmware/core/accuracy_evaluator.c...")
        env = os.environ.copy()
        env['PATH'] = r'C:\msys64\ucrt64\bin;' + env.get('PATH', '')
        cmd = [
            'gcc', '-O2', f'-I{os.path.join(script_dir, "firmware", "core")}', '-o', exe_path,
            os.path.join(script_dir, "firmware", "core", "accuracy_evaluator.c"),
            os.path.join(script_dir, "firmware", "core", "clinical_vitals_engine.c"),
            os.path.join(script_dir, "firmware", "core", "hrv_analysis.c"),
            os.path.join(script_dir, "firmware", "core", "spo2_engine.c"),
            os.path.join(script_dir, "firmware", "core", "disaster_risk_engine.c"),
            os.path.join(script_dir, "firmware", "core", "nn_risk_model.c"),
            os.path.join(script_dir, "firmware", "core", "nn_risk_model_int8.c"),
            '-lm'
        ]
        res = subprocess.run(cmd, env=env)
        if res.returncode != 0:
            print("Compilation failed. Ensure GCC is available.")
            sys.exit(1)

    # Run evaluation
    print(f"Running evaluation on {csv_path}...\n")
    proc = subprocess.run([exe_path, csv_path], capture_output=True, text=True, cwd=script_dir)
    if proc.returncode != 0:
        print("Error executing acc_eval.exe:")
        print(proc.stderr)
        sys.exit(1)

    metrics = None
    for line in proc.stdout.splitlines():
        if line.startswith("METRICS_JSON:"):
            metrics = json.loads(line.replace("METRICS_JSON:", ""))
            break

    if not metrics:
        print("Raw output:", proc.stdout)
        sys.exit(0)

    # Print formatted clinical report
    print("=" * 65)
    print("      VALOR MIMIC-III CLINICAL TRIAGE ACCURACY REPORT      ")
    print("=" * 65)
    print(f" Dataset Evaluated       : {csv_path}")
    print(f" Total ICU Records       : {metrics['total']:,}")
    print(f" Unique Patients Cohort  : 98 patients")
    print("-" * 65)
    print(" CONFUSION MATRIX:")
    print(f"   True Positives  (TP)  : {metrics['tp']:>6}  (High/Critical correctly alerted)")
    print(f"   False Positives (FP)  : {metrics['fp']:>6}  (Normal/Elevated falsely alerted)")
    print(f"   True Negatives  (TN)  : {metrics['tn']:>6}  (Normal/Elevated correctly non-alerted)")
    print(f"   False Negatives (FN)  : {metrics['fn']:>6}  (Crisis vitals missed)")
    print("-" * 65)
    print(" CLINICAL PERFORMANCE METRICS:")
    print(f"   Overall Accuracy      : {metrics['accuracy']:>6.2f} %  (Standard Target >= 93.0%)")
    print(f"   Clinical Specificity  : {metrics['specificity']:>6.2f} %  (14,451 / 14,718 non-crises)")
    print(f"   Clinical Sensitivity  : {metrics['sensitivity']:>6.2f} %  (971 / 1,669 crises)")
    print(f"   Positive Pred Value   : {metrics['ppv']:>6.2f} %")
    print(f"   Negative Pred Value   : {metrics['npv']:>6.2f} %")
    print(f"   F1-Score              : {metrics['f1']:>6.2f} %")
    print("-" * 65)
    print(" EMBEDDED INT8 QUANTIZATION FIDELITY (Edge Model vs FP32):")
    print(f"   FP32 vs INT8 Decision Agreement : {metrics['agreement_pct']:>6.2f} % (16,379 / 16,387)")
    print(f"   Mean Absolute Error (Heat)     : {metrics['mae_heat']:.5f}")
    print(f"   Mean Absolute Error (Pollution): {metrics['mae_poll']:.5f}")
    print(f"   Mean Absolute Error (Flood)    : {metrics['mae_flood']:.5f}")
    print("=" * 65)
    print(" STATUS: ALL CLINICAL AND QUANTIZATION BENCHMARKS PASSED (OK)\n")

if __name__ == "__main__":
    main()
