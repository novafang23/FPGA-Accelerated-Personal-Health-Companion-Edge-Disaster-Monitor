import subprocess, os, json

env = os.environ.copy()
env['PATH'] = r'C:\msys64\ucrt64\bin;' + env.get('PATH', '')

cmd = [
    'gcc', '-O2', '-Ifirmware/core', '-o', 'acc_eval.exe',
    'firmware/core/accuracy_evaluator.c',
    'firmware/core/clinical_vitals_engine.c',
    'firmware/core/hrv_analysis.c',
    'firmware/core/spo2_engine.c',
    'firmware/core/disaster_risk_engine.c',
    'firmware/core/nn_risk_model.c',
    'firmware/core/nn_risk_model_int8.c',
    '-lm'
]

print("Compiling acc_eval...")
res = subprocess.run(cmd, capture_output=True, text=True, env=env)
if res.returncode != 0:
    print("Compilation failed!")
    print(res.stderr)
    exit(1)

print("Compilation successful! Running on data/mimic/mimic_eval_feed.csv...")
run_res = subprocess.run(['./acc_eval.exe', 'data/mimic/mimic_eval_feed.csv'], capture_output=True, text=True, env=env)
print("Output:")
print(run_res.stdout)
if run_res.stderr:
    print("Stderr:", run_res.stderr)

for line in run_res.stdout.splitlines():
    if line.startswith('METRICS_JSON:'):
        data = json.loads(line.replace('METRICS_JSON:', ''))
        print("\n--- Parsed MIMIC-III Accuracy Results ---")
        print(f"Total Records Evaluated : {data['total']}")
        print(f"True Positives (TP)     : {data['tp']}")
        print(f"False Positives (FP)    : {data['fp']}")
        print(f"True Negatives (TN)     : {data['tn']}")
        print(f"False Negatives (FN)    : {data['fn']}")
        print(f"Clinical Accuracy       : {data['accuracy']:.2f}%")
        print(f"Specificity             : {data['specificity']:.2f}%")
        print(f"Sensitivity             : {data['sensitivity']:.2f}%")
        print(f"PPV (Precision)         : {data['ppv']:.2f}%")
        print(f"NPV                     : {data['npv']:.2f}%")
        print(f"F1 Score                : {data['f1']:.2f}%")
        print(f"FP32 vs INT8 Agreement  : {data['agreement_pct']:.2f}%")
        print(f"MAE Heat Risk           : {data['mae_heat']:.5f}")
        print(f"MAE Pollution Risk      : {data['mae_poll']:.5f}")
        print(f"MAE Flood Risk          : {data['mae_flood']:.5f}")
