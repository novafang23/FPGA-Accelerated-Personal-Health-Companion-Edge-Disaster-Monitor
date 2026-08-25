#!/usr/bin/env python3
"""
train_nn_risk_model.py — Knowledge-Distillation Training for SIH26181 TinyML Model
=====================================================================================

Trains the 6->12->3 feedforward network used in nn_risk_model.c against the
existing rule-based CTSI / PRSI / flood scoring engine (disaster_risk_engine.c)
as the "teacher." This replaces the previously hand-typed weight matrix with
weights actually produced by gradient descent on labeled data, so the
"knowledge distillation" claim in the docs is backed by a real artifact.

Architecture (unchanged from nn_risk_model.h):
    Input(6) -> Dense(12, ReLU) -> Dense(3, Sigmoid)
    Inputs:  [HR, RMSSD, SpO2, Temp, Humidity, PM2.5]  (min-max normalized)
    Outputs: [heat_risk, pollution_risk, flood_risk]   (regressed to teacher/100)

Usage:
    python3 train_nn_risk_model.py

Outputs:
    - Prints training/validation loss curve summary
    - Prints validation against the 3 example scenarios from the original docstring
    - Writes trained weights as a ready-to-paste C struct to
      nn_risk_model_trained.c.inc
"""

import numpy as np

np.random.seed(42)

# =====================================================================
# 1. Feature normalization ranges — MUST match nn_risk_model.h exactly
# =====================================================================
RANGES = {
    "hr":    (40.0, 200.0),
    "rmssd": (1.0, 100.0),
    "spo2":  (70.0, 100.0),
    "temp":  (-10.0, 55.0),
    "hum":   (0.0, 100.0),
    "pm25":  (0.0, 500.0),
}


def normalize(v, lo, hi):
    return np.clip((v - lo) / (hi - lo), 0.0, 1.0)


# =====================================================================
# 2. Teacher models — faithful port of disaster_risk_engine.c
#    (same thresholds, same point values, vectorized over numpy arrays)
# =====================================================================
def ctsi_score(bpm, rmssd, temp, hum):
    """Cardio-Thermal Strain Index, 0-100. Direct port of assess_heat_risk()."""
    heat_index = np.where(
        (temp > 27.0) & (hum > 40.0),
        temp + 0.5 * (hum - 40.0) * 0.1,
        temp,
    )
    ctsi = np.zeros_like(bpm)
    ctsi += np.select(
        [heat_index > 54.0, heat_index > 45.0, heat_index > 40.0, heat_index > 35.0],
        [40.0, 30.0, 20.0, 10.0], default=0.0)
    ctsi += np.select(
        [bpm > 130.0, bpm > 110.0, bpm > 95.0],
        [30.0, 20.0, 10.0], default=0.0)
    ctsi += np.select(
        [rmssd < 10.0, rmssd < 20.0, rmssd < 35.0],
        [30.0, 20.0, 10.0], default=0.0)
    return ctsi


def prsi_score(bpm, spo2, pm25, rmssd):
    """Pollution Respiratory Strain Index, 0-100. Direct port of assess_pollution_risk()."""
    prsi = np.zeros_like(bpm)
    prsi += np.select(
        [pm25 > 300.0, pm25 > 150.0, pm25 > 75.0, pm25 > 35.0],
        [40.0, 30.0, 20.0, 10.0], default=0.0)
    prsi += np.select(
        [spo2 < 88.0, spo2 < 92.0, spo2 < 94.0, spo2 < 96.0],
        [40.0, 30.0, 20.0, 10.0], default=0.0)
    prsi += np.select(
        [bpm > 120.0, bpm > 100.0],
        [15.0, 8.0], default=0.0)
    prsi += np.select(
        [rmssd < 15.0, rmssd < 25.0],
        [10.0, 5.0], default=0.0)
    return prsi


def flood_score(bpm, temp, rmssd):
    """
    Cold-exposure / flood risk, 0-100.

    NOTE — adaptation: the original assess_flood_risk() in disaster_risk_engine.c
    scores against *skin* temperature (thresholds ~28/32/34 C, i.e. near body
    temp). The NN's 6-feature input vector only carries *ambient* temperature,
    not a separate skin-temp channel (same limitation the original hand-typed
    weights had — they used ambient temp as the cold proxy too). This teacher
    re-thresholds for ambient air temperature in a cold/flood exposure scenario
    instead of skin temperature, so the label is physically sensible for the
    input the network actually receives.
    """
    score = np.zeros_like(bpm)
    score += np.select(
        [temp < 0.0, temp < 8.0, temp < 15.0],
        [40.0, 25.0, 10.0], default=0.0)
    score += np.select(
        [bpm < 50.0, bpm > 150.0, bpm > 130.0],
        [30.0, 30.0, 15.0], default=0.0)
    score += np.select(
        [rmssd < 8.0, rmssd < 15.0],
        [20.0, 10.0], default=0.0)
    return score


# =====================================================================
# 3. Synthetic labeled dataset
#    Uniform coverage of the full physiological/environmental space,
#    plus extra sampling near the decision boundaries so the network
#    doesn't just learn the easy interior of each class.
# =====================================================================
def make_dataset(n):
    hr    = np.random.uniform(*RANGES["hr"], n)
    rmssd = np.random.uniform(*RANGES["rmssd"], n)
    spo2  = np.random.uniform(*RANGES["spo2"], n)
    temp  = np.random.uniform(*RANGES["temp"], n)
    hum   = np.random.uniform(*RANGES["hum"], n)
    pm25  = np.random.uniform(*RANGES["pm25"], n)
    return hr, rmssd, spo2, temp, hum, pm25


N_UNIFORM  = 40_000
N_BOUNDARY = 10_000  # extra samples clustered near class boundaries

hr, rmssd, spo2, temp, hum, pm25 = make_dataset(N_UNIFORM)

# Boundary-focused samples: jitter around the known threshold values so the
# sigmoid decision edges get properly fit, not just the bulk of the space.
thresholds_hr    = [95, 110, 130, 100, 120, 150, 130, 50]
thresholds_rmssd = [10, 20, 35, 15, 25, 8, 15]
thresholds_spo2  = [88, 92, 94, 96]
thresholds_temp  = [27, 35, 40, 45, 54, 0, 8, 15]
thresholds_hum   = [40]
thresholds_pm25  = [35, 75, 150, 300]

def jittered_pick(thresh_list, lo, hi, n):
    centers = np.random.choice(thresh_list, n)
    return np.clip(centers + np.random.normal(0, (hi - lo) * 0.02, n), lo, hi)

hr_b    = jittered_pick(thresholds_hr, *RANGES["hr"], N_BOUNDARY)
rmssd_b = jittered_pick(thresholds_rmssd, *RANGES["rmssd"], N_BOUNDARY)
spo2_b  = jittered_pick(thresholds_spo2, *RANGES["spo2"], N_BOUNDARY)
temp_b  = jittered_pick(thresholds_temp, *RANGES["temp"], N_BOUNDARY)
hum_b   = jittered_pick(thresholds_hum, *RANGES["hum"], N_BOUNDARY)
pm25_b  = jittered_pick(thresholds_pm25, *RANGES["pm25"], N_BOUNDARY)
# shuffle independently per-feature so boundary jitters combine across all dims
for arr in (hr_b, rmssd_b, spo2_b, temp_b, hum_b, pm25_b):
    np.random.shuffle(arr)

hr    = np.concatenate([hr, hr_b])
rmssd = np.concatenate([rmssd, rmssd_b])
spo2  = np.concatenate([spo2, spo2_b])
temp  = np.concatenate([temp, temp_b])
hum   = np.concatenate([hum, hum_b])
pm25  = np.concatenate([pm25, pm25_b])

N = len(hr)
X = np.stack([
    normalize(hr,    *RANGES["hr"]),
    normalize(rmssd, *RANGES["rmssd"]),
    normalize(spo2,  *RANGES["spo2"]),
    normalize(temp,  *RANGES["temp"]),
    normalize(hum,   *RANGES["hum"]),
    normalize(pm25,  *RANGES["pm25"]),
], axis=1)  # (N, 6)

y_heat = np.clip(ctsi_score(hr, rmssd, temp, hum) / 100.0, 0, 1)
y_pol  = np.clip(prsi_score(hr, spo2, pm25, rmssd) / 100.0, 0, 1)
y_flo  = np.clip(flood_score(hr, temp, rmssd) / 100.0, 0, 1)
Y = np.stack([y_heat, y_pol, y_flo], axis=1)  # (N, 3)

# Train/val split
idx = np.random.permutation(N)
n_val = int(0.1 * N)
val_idx, train_idx = idx[:n_val], idx[n_val:]
X_train, Y_train = X[train_idx], Y[train_idx]
X_val, Y_val = X[val_idx], Y[val_idx]

print(f"Dataset: {N} samples ({len(train_idx)} train / {len(val_idx)} val)")
print(f"Label distribution — heat  mean={y_heat.mean():.3f}  pollution mean={y_pol.mean():.3f}  flood mean={y_flo.mean():.3f}")

# =====================================================================
# 4. Model — same 6->12->3 architecture, trained with Adam + backprop
# =====================================================================
IN, HID, OUT = 6, 12, 3

def relu(x):
    return np.maximum(0, x)

def relu_grad(x):
    return (x > 0).astype(x.dtype)

def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-np.clip(x, -30, 30)))

# He init for ReLU layer, Xavier for sigmoid layer
rng = np.random.default_rng(42)
W1 = rng.normal(0, np.sqrt(2.0 / IN), (HID, IN)).astype(np.float64)
b1 = np.zeros(HID)
W2 = rng.normal(0, np.sqrt(1.0 / HID), (OUT, HID)).astype(np.float64)
b2 = np.zeros(OUT)

# Adam optimizer state
mW1, vW1 = np.zeros_like(W1), np.zeros_like(W1)
mb1, vb1 = np.zeros_like(b1), np.zeros_like(b1)
mW2, vW2 = np.zeros_like(W2), np.zeros_like(W2)
mb2, vb2 = np.zeros_like(b2), np.zeros_like(b2)
beta1, beta2, eps = 0.9, 0.999, 1e-8
LR = 0.03
L2 = 1e-4  # small weight decay so weights stay small enough for later INT8 quantization

EPOCHS = 400
BATCH = 512
n_train = len(X_train)

def forward(X, W1, b1, W2, b2):
    z1 = X @ W1.T + b1        # (n, HID)
    a1 = relu(z1)
    z2 = a1 @ W2.T + b2       # (n, OUT)
    a2 = sigmoid(z2)
    return z1, a1, z2, a2

t = 0
for epoch in range(1, EPOCHS + 1):
    perm = np.random.permutation(n_train)
    epoch_loss = 0.0
    for start in range(0, n_train, BATCH):
        batch_idx = perm[start:start + BATCH]
        xb, yb = X_train[batch_idx], Y_train[batch_idx]
        m = len(xb)

        z1, a1, z2, a2 = forward(xb, W1, b1, W2, b2)

        # MSE loss
        diff = (a2 - yb)
        loss = np.mean(diff ** 2)
        epoch_loss += loss * m

        # Backprop
        dz2 = diff * a2 * (1 - a2) * (2.0 / m)         # d(MSE)/dz2, (m, OUT)
        dW2 = dz2.T @ a1 + L2 * W2
        db2 = dz2.sum(axis=0)

        da1 = dz2 @ W2                                  # (m, HID)
        dz1 = da1 * relu_grad(z1)
        dW1 = dz1.T @ xb + L2 * W1
        db1 = dz1.sum(axis=0)

        # Adam update
        t += 1
        for (param, grad, m_state, v_state) in [
            (W1, dW1, mW1, vW1), (b1, db1, mb1, vb1),
            (W2, dW2, mW2, vW2), (b2, db2, mb2, vb2),
        ]:
            m_state[...] = beta1 * m_state + (1 - beta1) * grad
            v_state[...] = beta2 * v_state + (1 - beta2) * (grad ** 2)
            m_hat = m_state / (1 - beta1 ** t)
            v_hat = v_state / (1 - beta2 ** t)
            param -= LR * m_hat / (np.sqrt(v_hat) + eps)

    if epoch % 50 == 0 or epoch == 1:
        _, _, _, val_pred = forward(X_val, W1, b1, W2, b2)
        val_loss = np.mean((val_pred - Y_val) ** 2)
        print(f"  epoch {epoch:4d}  train_mse={epoch_loss / n_train:.5f}  val_mse={val_loss:.5f}")

# =====================================================================
# 5. Validate against the exact scenarios from the original docstring
# =====================================================================
def predict(hr_v, rmssd_v, spo2_v, temp_v, hum_v, pm25_v):
    x = np.array([[
        normalize(hr_v, *RANGES["hr"]),
        normalize(rmssd_v, *RANGES["rmssd"]),
        normalize(spo2_v, *RANGES["spo2"]),
        normalize(temp_v, *RANGES["temp"]),
        normalize(hum_v, *RANGES["hum"]),
        normalize(pm25_v, *RANGES["pm25"]),
    ]])
    _, _, _, out = forward(x, W1, b1, W2, b2)
    return out[0]

print("\nValidation against original docstring examples:")
scenarios = [
    ("Normal (HR=72, RMSSD=50, SpO2=98, Temp=25, Hum=50, PM2.5=20)", 72, 50, 98, 25, 50, 20, "all outputs < 0.15"),
    ("Heat wave (HR=140, RMSSD=8, SpO2=97, Temp=47, Hum=60, PM2.5=20)", 140, 8, 97, 47, 60, 20, "heat > 0.85"),
    ("Severe smog (HR=100, RMSSD=30, SpO2=86, Temp=25, Hum=50, PM2.5=400)", 100, 30, 86, 25, 50, 400, "pollution > 0.85"),
]
for label, hr_v, rmssd_v, spo2_v, temp_v, hum_v, pm25_v, expected in scenarios:
    out = predict(hr_v, rmssd_v, spo2_v, temp_v, hum_v, pm25_v)
    print(f"  {label}")
    print(f"    -> heat={out[0]:.3f}  pollution={out[1]:.3f}  flood={out[2]:.3f}   (expected: {expected})")

# =====================================================================
# 6. Emit ready-to-paste C weights for nn_risk_model.c
# =====================================================================
def c_format_array2d(name, arr, rows_comment=None):
    lines = [f"    .{name} = {{"]
    for i, row in enumerate(arr):
        vals = ", ".join(f"{v: .6f}f" for v in row)
        comment = f"  /* {rows_comment[i]} */" if rows_comment else ""
        lines.append(f"        {{ {vals} }},{comment}")
    lines.append("    },")
    return "\n".join(lines)

def c_format_array1d(name, arr):
    vals = ", ".join(f"{v: .6f}f" for v in arr)
    return f"    .{name} = {{ {vals} }},"

out_path = "nn_risk_model_trained.c.inc"
with open(out_path, "w") as f:
    f.write("/* Auto-generated by train_nn_risk_model.py — real gradient-descent fit,\n")
    f.write(" * distilled from the CTSI/PRSI/flood rule engine in disaster_risk_engine.c.\n")
    f.write(" * Paste this in place of the `default_model` initializer body in nn_risk_model.c\n")
    f.write(f" * Final train_mse={epoch_loss / n_train:.5f}\n */\n")
    f.write("static const nn_model_t default_model = {\n")
    f.write(c_format_array2d("W1", W1) + "\n")
    f.write(c_format_array1d("b1", b1) + "\n")
    f.write(c_format_array2d("W2", W2) + "\n")
    f.write(c_format_array1d("b2", b2) + "\n")
    f.write("};\n")

print(f"\nWrote trained weights -> {out_path}")
print(f"Total parameters: {W1.size + b1.size + W2.size + b2.size} "
      f"({(W1.size + b1.size + W2.size + b2.size) * 4} bytes as float32)")
