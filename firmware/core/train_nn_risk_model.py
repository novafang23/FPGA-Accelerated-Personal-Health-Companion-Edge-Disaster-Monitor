#!/usr/bin/env python3
"""
train_nn_risk_model.py — Knowledge-Distillation Training for SIH26181 TinyML Model
===================================================================================

Trains the 6->12->3 feedforward network used in nn_risk_model.c against the
existing rule-based CTSI / PRSI / flood scoring engine (disaster_risk_engine.c)
as the "teacher." This replaces the previously hand-typed weight matrix with
weights actually produced by gradient descent on labeled data, so the
"knowledge distillation" claim in the docs is backed by a real artifact.

Architecture (unchanged from nn_risk_model.h):
    Input(6) -> Dense(12, ReLU) -> Dense(3, Sigmoid)
    Inputs:  [HR, RMSSD, SpO2, Temp, Humidity, PM2.5]  (min-max normalized)
    Outputs: [heat_risk, pollution_risk, flood_risk]   (regressed to teacher/100)

Quantization:
    Supports quantization-aware training (QAT) with fake-quant nodes.
    Exports INT8 weights + per-tensor scales/zero-points for embedded inference.

Usage:
    python3 train_nn_risk_model.py           # float32 training (default)
    python3 train_nn_risk_model.py --qat     # quantization-aware training
    python3 train_nn_risk_model.py --int8    # post-training quantization

Outputs:
    - Prints training/validation loss curve summary
    - Prints validation against the 3 example scenarios from the original docstring
    - Writes trained weights as a ready-to-paste C struct to
      nn_risk_model_trained.c.inc (float32) or nn_risk_model_int8.c.inc (INT8)
"""

import numpy as np
import argparse
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

np.random.seed(42)

# 1. Feature normalization ranges
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


# 2. Teacher models (port of disaster_risk_engine.c)
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


# 3. Synthetic labeled dataset
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

# Joint multi-feature extreme scenarios: the per-feature jitter above covers
# each threshold individually (e.g. "temp near 45C" OR "HR near 130"), but
# never combines them the way a real disaster does (temp near 45C AND HR
# near 130 AND humidity high, all at once). Sample directly from the
# neighborhood of each named disaster archetype so those joint combinations
# get real training signal, not just each axis in isolation.
N_SCENARIO = 12_000
scenario_centers = [
    # (hr, rmssd, spo2, temp, hum, pm25)  -- roughly matches the project's own demo scenarios
    (140.0,  8.0, 95.0, 50.0, 65.0,  25.0),   # Heat wave
    (150.0,  6.0, 93.0, 47.0, 70.0,  20.0),   # Severe heat wave / heatstroke
    (123.0, 12.0, 86.0, 12.0, 85.0, 400.0),   # Severe smog
    (115.0, 15.0, 87.0, 20.0, 60.0, 320.0),   # High pollution + moderate heat
    (140.0,  6.0, 93.0,  6.0, 98.0,  20.0),   # Flash flood / cold shock (tachycardia)
    ( 42.0,  6.0, 92.0,  4.0, 95.0,  15.0),   # Hypothermia (bradycardia)
    ( 72.0, 45.0, 98.0, 25.0, 45.0,  15.0),   # Normal resting (anchor so "everything fine" stays fine)
]


def scenario_pick(centers, ranges, n):
    """Sample n points jittered around randomly chosen scenario centers."""
    idx = np.random.choice(len(centers), n)
    chosen = np.array(centers)[idx]  # (n, 6)
    spans = np.array([ranges[k][1] - ranges[k][0] for k in
                       ("hr", "rmssd", "spo2", "temp", "hum", "pm25")])
    jitter = np.random.normal(0, 1, (n, 6)) * (spans * 0.06)
    out = chosen + jitter
    for i, k in enumerate(("hr", "rmssd", "spo2", "temp", "hum", "pm25")):
        out[:, i] = np.clip(out[:, i], *ranges[k])
    return out


scenario_pts = scenario_pick(scenario_centers, RANGES, N_SCENARIO)
hr_s, rmssd_s, spo2_s, temp_s, hum_s, pm25_s = [scenario_pts[:, i] for i in range(6)]

hr    = np.concatenate([hr, hr_b, hr_s])
rmssd = np.concatenate([rmssd, rmssd_b, rmssd_s])
spo2  = np.concatenate([spo2, spo2_b, spo2_s])
temp  = np.concatenate([temp, temp_b, temp_s])
hum   = np.concatenate([hum, hum_b, hum_s])
pm25  = np.concatenate([pm25, pm25_b, pm25_s])

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

# 4. Model Definition (6->12->3, Adam)
IN, HID, OUT = 6, 12, 3


def relu(x):
    return np.maximum(0, x)


def relu_grad(x):
    return (x > 0).astype(x.dtype)


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-np.clip(x, -30, 30)))


def forward(X, W1, b1, W2, b2):
    z1 = X @ W1.T + b1
    a1 = relu(z1)
    z2 = a1 @ W2.T + b2
    a2 = sigmoid(z2)
    return z1, a1, z2, a2


# Quantization utilities
def quantize_per_tensor(x, bits=8):
    """Symmetric per-tensor quantization to INT8/UINT8."""
    x_max = np.max(np.abs(x))
    if x_max == 0 or not np.isfinite(x_max):
        return np.zeros_like(x, dtype=np.int8), 1.0, 0
    scale = x_max / (2**(bits-1) - 1)
    if not np.isfinite(scale) or scale == 0:
        scale = 1.0
    q = np.clip(np.round(x / scale), -(2**(bits-1)), 2**(bits-1) - 1).astype(np.int8)
    return q, float(scale), 0


def quantize_asymmetric(x, bits=8):
    """Asymmetric per-tensor quantization (for activations >= 0)."""
    x_min, x_max = np.min(x), np.max(x)
    if x_max == x_min or not np.isfinite(x_min) or not np.isfinite(x_max):
        return np.zeros_like(x, dtype=np.uint8), 1.0, 0
    scale = (x_max - x_min) / (2**bits - 1)
    if not np.isfinite(scale) or scale == 0:
        scale = 1.0
    zp = int(np.round(-x_min / scale))
    zp = np.clip(zp, 0, 2**bits - 1)
    q = np.clip(np.round(x / scale + zp), 0, 2**bits - 1).astype(np.uint8)
    return q, float(scale), zp


def fake_quant(x, scale, zp, bits=8):
    """Fake quantization for QAT: quantize then dequantize."""
    if zp == 0:
        x_q = np.round(x / scale)
        x_q = np.clip(x_q, -(2**(bits-1)), 2**(bits-1) - 1)
        return x_q * scale
    else:
        x_q = np.round(x / scale + zp)
        x_q = np.clip(x_q, 0, 2**bits - 1)
        return (x_q - zp) * scale


class QATWrapper:
    """Wraps weights/biases with fake-quant for quantization-aware training."""
    def __init__(self, W1, b1, W2, b2, qat=False):
        self.W1 = W1.astype(np.float32)
        self.b1 = b1.astype(np.float32)
        self.W2 = W2.astype(np.float32)
        self.b2 = b2.astype(np.float32)
        self.qat = qat
        # Quantization params (learned/calibrated) - PER TENSOR (for C compatibility)
        self.W1_scale = self.W1_zp = None
        self.b1_scale = self.b1_zp = None
        self.W2_scale = self.W2_zp = None
        self.b2_scale = self.b2_zp = None
        self.act1_scale = self.act1_zp = None
        self.act2_scale = self.act2_zp = None
    
    def calibrate(self, X_calib):
        """Calibrate quantization parameters using calibration data."""
        # Forward pass to get activation ranges
        z1 = X_calib @ self.W1.T + self.b1
        a1 = relu(z1)
        z2 = a1 @ self.W2.T + self.b2
        a2 = sigmoid(z2)
        
        # Weight scales (symmetric, zp=0)
        _, self.W1_scale, _ = quantize_per_tensor(self.W1)
        self.W1_zp = 0
        _, self.b1_scale, _ = quantize_per_tensor(self.b1)
        self.b1_zp = 0
        _, self.W2_scale, _ = quantize_per_tensor(self.W2)
        self.W2_zp = 0
        _, self.b2_scale, _ = quantize_per_tensor(self.b2)
        self.b2_zp = 0
        
        # Activation scales (asymmetric for ReLU/sigmoid outputs >= 0)
        _, self.act1_scale, self.act1_zp = quantize_asymmetric(a1)
        _, self.act2_scale, self.act2_zp = quantize_asymmetric(a2)
    
    def get_quantized_weights(self):
        """Return quantized integer weights + scales/zps for deployment."""
        W1_q, _, _ = quantize_per_tensor(self.W1)
        b1_q, _, _ = quantize_per_tensor(self.b1)
        W2_q, _, _ = quantize_per_tensor(self.W2)
        b2_q, _, _ = quantize_per_tensor(self.b2)
        return {
            'W1': W1_q, 'b1': b1_q,
            'W2': W2_q, 'b2': b2_q,
            'W1_scale': self.W1_scale, 'W1_zp': 0,
            'b1_scale': self.b1_scale, 'b1_zp': 0,
            'W2_scale': self.W2_scale, 'W2_zp': 0,
            'b2_scale': self.b2_scale, 'b2_zp': 0,
            'act1_scale': self.act1_scale, 'act1_zp': self.act1_zp,
            'act2_scale': self.act2_scale, 'act2_zp': self.act2_zp,
        }
    
    def forward(self, X):
        """Forward pass with optional fake quantization."""
        # Layer 1
        if self.qat and self.W1_scale is not None:
            W1_fq = fake_quant(self.W1, self.W1_scale, self.W1_zp)
            b1_fq = fake_quant(self.b1, self.b1_scale, self.b1_zp)
        else:
            W1_fq, b1_fq = self.W1, self.b1
        z1 = X @ W1_fq.T + b1_fq
        a1 = relu(z1)
        if self.qat and self.act1_scale is not None:
            a1 = fake_quant(a1, self.act1_scale, self.act1_zp)
        
        # Layer 2
        if self.qat and self.W2_scale is not None:
            W2_fq = fake_quant(self.W2, self.W2_scale, self.W2_zp)
            b2_fq = fake_quant(self.b2, self.b2_scale, self.b2_zp)
        else:
            W2_fq, b2_fq = self.W2, self.b2
        z2 = a1 @ W2_fq.T + b2_fq
        a2 = sigmoid(z2)
        if self.qat and self.act2_scale is not None:
            a2 = fake_quant(a2, self.act2_scale, self.act2_zp)
        return z1, a1, z2, a2


# 5. Training functions
def train_float32(X_train, Y_train, X_val, Y_val, epochs=400, batch=512, lr=0.03, l2=1e-4):
    """Standard float32 training."""
    rng = np.random.default_rng(42)
    W1 = rng.normal(0, np.sqrt(2.0 / IN), (HID, IN)).astype(np.float32)
    b1 = np.zeros(HID, dtype=np.float32)
    W2 = rng.normal(0, np.sqrt(1.0 / HID), (OUT, HID)).astype(np.float32)
    b2 = np.zeros(OUT, dtype=np.float32)

    # Adam optimizer state
    mW1, vW1 = np.zeros_like(W1), np.zeros_like(W1)
    mb1, vb1 = np.zeros_like(b1), np.zeros_like(b1)
    mW2, vW2 = np.zeros_like(W2), np.zeros_like(W2)
    mb2, vb2 = np.zeros_like(b2), np.zeros_like(b2)
    beta1, beta2, eps = 0.9, 0.999, 1e-8

    n_train = len(X_train)
    t = 0
    
    def forward(X, W1, b1, W2, b2):
        z1 = X @ W1.T + b1
        a1 = relu(z1)
        z2 = a1 @ W2.T + b2
        a2 = sigmoid(z2)
        return z1, a1, z2, a2

    for epoch in range(1, epochs + 1):
        perm = np.random.permutation(n_train)
        epoch_loss = 0.0
        for start in range(0, n_train, batch):
            batch_idx = perm[start:start + batch]
            xb, yb = X_train[batch_idx], Y_train[batch_idx]
            m = len(xb)

            z1, a1, z2, a2 = forward(xb, W1, b1, W2, b2)

            # MSE loss
            diff = (a2 - yb)
            loss = np.mean(diff ** 2)
            epoch_loss += loss * m

            # Backprop
            dz2 = diff * a2 * (1 - a2) * (2.0 / m)
            dW2 = dz2.T @ a1 + l2 * W2
            db2 = dz2.sum(axis=0)

            da1 = dz2 @ W2
            dz1 = da1 * relu_grad(z1)
            dW1 = dz1.T @ xb + l2 * W1
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
                param -= lr * m_hat / (np.sqrt(v_hat) + eps)

        if epoch % 50 == 0 or epoch == 1:
            _, _, _, val_pred = forward(X_val, W1, b1, W2, b2)
            val_loss = np.mean((val_pred - Y_val) ** 2)
            print(f"  epoch {epoch:4d}  train_mse={epoch_loss / n_train:.5f}  val_mse={val_loss:.5f}")

    return W1, b1, W2, b2


def train_qat(X_train, Y_train, X_val, Y_val, epochs=400, batch=512, lr=0.01, l2=1e-4):
    """Quantization-aware training."""
    # Initialize with float32 weights
    rng = np.random.default_rng(42)
    W1 = rng.normal(0, np.sqrt(2.0 / IN), (HID, IN)).astype(np.float32)
    b1 = np.zeros(HID, dtype=np.float32)
    W2 = rng.normal(0, np.sqrt(1.0 / HID), (OUT, HID)).astype(np.float32)
    b2 = np.zeros(OUT, dtype=np.float32)

    # Adam optimizer state
    mW1, vW1 = np.zeros_like(W1), np.zeros_like(W1)
    mb1, vb1 = np.zeros_like(b1), np.zeros_like(b1)
    mW2, vW2 = np.zeros_like(W2), np.zeros_like(W2)
    mb2, vb2 = np.zeros_like(b2), np.zeros_like(b2)
    beta1, beta2, eps = 0.9, 0.999, 1e-8

    model = QATWrapper(W1, b1, W2, b2, qat=True)
    
    # Calibrate on subset of training data
    calib_idx = np.random.choice(len(X_train), min(1000, len(X_train)), replace=False)
    model.calibrate(X_train[calib_idx])

    n_train = len(X_train)
    t = 0
    
    for epoch in range(1, epochs + 1):
        perm = np.random.permutation(n_train)
        epoch_loss = 0.0
        for start in range(0, n_train, batch):
            batch_idx = perm[start:start + batch]
            xb, yb = X_train[batch_idx], Y_train[batch_idx]
            m = len(xb)

            z1, a1, z2, a2 = model.forward(xb)

            # MSE loss
            diff = (a2 - yb)
            loss = np.mean(diff ** 2)
            if not np.isfinite(loss):
                loss = 1.0
            epoch_loss += loss * m

            # Backprop (straight-through estimator for fake-quant)
            dz2 = diff * a2 * (1 - a2) * (2.0 / m)
            dW2 = dz2.T @ a1 + l2 * model.W2
            db2 = dz2.sum(axis=0)

            da1 = dz2 @ model.W2
            dz1 = da1 * relu_grad(z1)
            dW1 = dz1.T @ xb + l2 * model.W1
            db1 = dz1.sum(axis=0)

            # Gradient clipping
            max_grad = 1.0
            for grad in [dW1, db1, dW2, db2]:
                np.clip(grad, -max_grad, max_grad, out=grad)

            # Adam update
            t += 1
            for (param, grad, m_state, v_state) in [
                (model.W1, dW1, mW1, vW1), (model.b1, db1, mb1, vb1),
                (model.W2, dW2, mW2, vW2), (model.b2, db2, mb2, vb2),
            ]:
                m_state[...] = beta1 * m_state + (1 - beta1) * grad
                v_state[...] = beta2 * v_state + (1 - beta2) * (grad ** 2)
                m_hat = m_state / (1 - beta1 ** t)
                v_hat = v_state / (1 - beta2 ** t)
                param -= lr * m_hat / (np.sqrt(v_hat) + eps)

            # Sanity check - prevent NaN weights
            for param in [model.W1, model.b1, model.W2, model.b2]:
                param[~np.isfinite(param)] = 0.0

        # Recalibrate quantization params every 25 epochs to track weight drift
        if epoch % 25 == 0:
            model.calibrate(X_train[calib_idx])

        if epoch % 25 == 0 or epoch == 1:
            _, _, _, val_pred = model.forward(X_val)
            val_loss = np.mean((val_pred - Y_val) ** 2)
            if not np.isfinite(val_loss):
                val_loss = 1.0
            print(f"  epoch {epoch:4d}  train_mse={epoch_loss / n_train:.5f}  val_mse={val_loss:.5f}")

    # Final calibration
    model.calibrate(X_train[calib_idx])
    return model


def train_post_quant(X_train, Y_train, X_val, Y_val, epochs=400, batch=512, lr=0.03, l2=1e-4):
    """Train float32 then post-training quantize."""
    W1, b1, W2, b2 = train_float32(X_train, Y_train, X_val, Y_val, epochs, batch, lr, l2)
    model = QATWrapper(W1, b1, W2, b2, qat=False)
    calib_idx = np.random.choice(len(X_train), min(1000, len(X_train)), replace=False)
    model.calibrate(X_train[calib_idx])
    return model


# 6. Validation against exact scenarios
def predict(model, hr_v, rmssd_v, spo2_v, temp_v, hum_v, pm25_v):
    x = np.array([[
        normalize(hr_v, *RANGES["hr"]),
        normalize(rmssd_v, *RANGES["rmssd"]),
        normalize(spo2_v, *RANGES["spo2"]),
        normalize(temp_v, *RANGES["temp"]),
        normalize(hum_v, *RANGES["hum"]),
        normalize(pm25_v, *RANGES["pm25"]),
    ]])
    _, _, _, out = model.forward(x)
    return out[0]


def classify(scores):
    classes = np.zeros_like(scores, dtype=int)
    classes[scores >= 0.25] = 1
    classes[scores >= 0.50] = 2
    classes[scores >= 0.70] = 3
    return classes


# 7. C code emission
def c_format_array2d(name, arr, rows_comment=None, is_float=False):
    lines = [f"    .{name} = {{"]
    for i, row in enumerate(arr):
        if is_float:
            vals = ", ".join(f"{v:.6f}f" for v in row)
        else:
            vals = ", ".join(f"{int(v)}" for v in row)
        comment = f"  /* {rows_comment[i]} */" if rows_comment else ""
        lines.append(f"        {{ {vals} }},{comment}")
    lines.append("    },")
    return "\n".join(lines)


def c_format_array1d(name, arr):
    vals = ", ".join(f"{int(v)}" for v in arr)
    return f"    .{name} = {{ {vals} }},"


def c_format_array1d_float(name, arr):
    vals = ", ".join(f"{v:.6f}f" for v in arr)
    return f"    .{name} = {{ {vals} }},"


def emit_float32_c(W1, b1, W2, b2, train_loss, out_path="nn_risk_model_trained.c.inc"):
    if not os.path.isabs(out_path):
        out_path = os.path.join(SCRIPT_DIR, out_path)
    with open(out_path, "w") as f:
        f.write("/* Auto-generated by train_nn_risk_model.py — float32 weights */\n")
        f.write(f"/* Final train_mse={train_loss:.5f} */\n")
        f.write("static const nn_model_t default_model = {\n")
        f.write(c_format_array2d("W1", W1, is_float=True) + "\n")
        f.write(c_format_array1d_float("b1", b1) + "\n")
        f.write(c_format_array2d("W2", W2, is_float=True) + "\n")
        f.write(c_format_array1d_float("b2", b2) + "\n")
        f.write("};\n")
    print(f"Wrote float32 weights -> {out_path}")


def emit_int8_c(model, train_loss, out_path="nn_risk_model_int8.c.inc"):
    if not os.path.isabs(out_path):
        out_path = os.path.join(SCRIPT_DIR, out_path)
    q = model.get_quantized_weights()
    with open(out_path, "w") as f:
        f.write("/* Auto-generated by train_nn_risk_model.py — INT8 quantized weights */\n")
        f.write("/* Symmetric per-tensor quantization for weights, asymmetric for activations */\n")
        f.write(f"/* Final train_mse={train_loss:.5f} */\n\n")
        
        f.write("#include \"nn_risk_model_int8.h\"\n\n")
        
        f.write("const nn_model_int8_t nn_default_model_int8 = {\n")
        f.write(c_format_array2d("W1", q['W1']) + "\n")
        f.write(c_format_array1d("b1", q['b1']) + "\n")
        f.write(c_format_array2d("W2", q['W2']) + "\n")
        f.write(c_format_array1d("b2", q['b2']) + "\n")
        f.write("};\n\n")
        
        f.write("const nn_quant_params_t nn_quant_params = {\n")
        f.write(f"    .W1_scale = {q['W1_scale']:.6f}f,\n")
        f.write(f"    .W1_zp    = {q['W1_zp']},\n")
        f.write(f"    .b1_scale = {q['b1_scale']:.6f}f,\n")
        f.write(f"    .b1_zp    = {q['b1_zp']},\n")
        f.write(f"    .W2_scale = {q['W2_scale']:.6f}f,\n")
        f.write(f"    .W2_zp    = {q['W2_zp']},\n")
        f.write(f"    .b2_scale = {q['b2_scale']:.6f}f,\n")
        f.write(f"    .b2_zp    = {q['b2_zp']},\n")
        f.write(f"    .act1_scale = {q['act1_scale']:.6f}f,\n")
        f.write(f"    .act1_zp    = {q['act1_zp']},\n")
        f.write(f"    .act2_scale = {q['act2_scale']:.6f}f,\n")
        f.write(f"    .act2_zp    = {q['act2_zp']},\n")
        f.write("};\n")
    print(f"Wrote INT8 weights -> {out_path}")


def emit_int8_header(out_path="nn_risk_model_int8.h"):
    if not os.path.isabs(out_path):
        out_path = os.path.join(SCRIPT_DIR, out_path)
    with open(out_path, "w") as f:
        f.write("""/*
 * nn_risk_model_int8.h
 * INT8 Quantized Neural Network Risk Assessment Model
 * Auto-generated by train_nn_risk_model.py
 */

#ifndef NN_RISK_MODEL_INT8_H
#define NN_RISK_MODEL_INT8_H

#include <stdint.h>
#include "nn_risk_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Quantized Model Structure */
typedef struct {
    int8_t  W1[NN_HIDDEN_SIZE][NN_INPUT_SIZE];
    int8_t  b1[NN_HIDDEN_SIZE];
    int8_t  W2[NN_OUTPUT_SIZE][NN_HIDDEN_SIZE];
    int8_t  b2[NN_OUTPUT_SIZE];
} nn_model_int8_t;

/* Quantization Parameters (per-tensor scales & zero-points) */
typedef struct {
    float W1_scale, W2_scale, b1_scale, b2_scale;
    float act1_scale, act2_scale;
    int8_t W1_zp, W2_zp, b1_zp, b2_zp;
    uint8_t act1_zp, act2_zp;
} nn_quant_params_t;

/* Extern declarations */
extern const nn_model_int8_t nn_default_model_int8;
extern const nn_quant_params_t nn_quant_params;

/* INT8 Inference API */
void nn_predict_int8(
    const nn_model_int8_t *model,
    const nn_quant_params_t *qparams,
    float hr, float rmssd, float spo2,
    float temp, float hum, float pm25,
    nn_output_t *out
);

#ifdef __cplusplus
}
#endif

#endif /* NN_RISK_MODEL_INT8_H */
""")
    print(f"Wrote INT8 header -> {out_path}")


def main():
    parser = argparse.ArgumentParser(description="Train NN risk model (float32 or INT8)")
    parser.add_argument("--qat", action="store_true", help="Quantization-aware training")
    parser.add_argument("--int8", action="store_true", help="Post-training quantization")
    parser.add_argument("--epochs", type=int, default=400, help="Training epochs")
    args = parser.parse_args()

    if args.qat:
        print("=== Quantization-Aware Training (QAT) ===")
        model = train_qat(X_train, Y_train, X_val, Y_val, epochs=args.epochs)
        _, _, _, val_pred = model.forward(X_val)
        val_loss = np.mean((val_pred - Y_val) ** 2)
        emit_int8_c(model, val_loss)
        emit_int8_header()
    elif args.int8:
        print("=== Post-Training Quantization (PTQ) ===")
        model = train_post_quant(X_train, Y_train, X_val, Y_val, epochs=args.epochs)
        _, _, _, val_pred = model.forward(X_val)
        val_loss = np.mean((val_pred - Y_val) ** 2)
        emit_int8_c(model, val_loss)
        emit_int8_header()
    else:
        print("=== Float32 Training ===")
        W1, b1, W2, b2 = train_float32(X_train, Y_train, X_val, Y_val, epochs=args.epochs)
        _, _, _, val_pred = forward(X_val, W1, b1, W2, b2)
        val_loss = np.mean((val_pred - Y_val) ** 2)
        emit_float32_c(W1, b1, W2, b2, val_loss)
        # Also emit INT8 for backward compatibility
        model = QATWrapper(W1, b1, W2, b2, qat=False)
        calib_idx = np.random.choice(len(X_train), min(1000, len(X_train)), replace=False)
        model.calibrate(X_train[calib_idx])
        emit_int8_c(model, val_loss, out_path="nn_risk_model_int8_from_float.c.inc")
        emit_int8_header()

    # Validation against exact scenarios
    print("\nValidation against original docstring examples:")
    scenarios = [
        ("Normal (HR=72, RMSSD=50, SpO2=98, Temp=25, Hum=50, PM2.5=20)", 72, 50, 98, 25, 50, 20, "all outputs < 0.15"),
        ("Heat wave (HR=140, RMSSD=8, SpO2=97, Temp=47, Hum=60, PM2.5=20)", 140, 8, 97, 47, 60, 20, "heat > 0.85"),
        ("Severe smog (HR=100, RMSSD=30, SpO2=86, Temp=25, Hum=50, PM2.5=400)", 100, 30, 86, 25, 50, 400, "pollution > 0.85"),
    ]
    for label, hr_v, rmssd_v, spo2_v, temp_v, hum_v, pm25_v, expected in scenarios:
        out = predict(model, hr_v, rmssd_v, spo2_v, temp_v, hum_v, pm25_v)
        print(f"  {label}")
        print(f"    -> heat={out[0]:.3f}  pollution={out[1]:.3f}  flood={out[2]:.3f}   (expected: {expected})")

    # Classification Accuracy
    _, _, _, val_pred = model.forward(X_val)
    val_pred_class = classify(val_pred)
    val_true_class = classify(Y_val)
    accuracy = np.mean(val_pred_class == val_true_class) * 100.0
    print(f"\nClassification Accuracy on Validation Set: {accuracy:.2f}%")
    
    # Get parameter count from model
    if hasattr(model, 'W1'):
        total_params = model.W1.size + model.b1.size + model.W2.size + model.b2.size
    else:
        total_params = W1.size + b1.size + W2.size + b2.size
    print(f"Total parameters: {total_params} ({total_params * 4} bytes float32, {total_params} bytes INT8)")


if __name__ == "__main__":
    main()