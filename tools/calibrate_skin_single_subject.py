#!/usr/bin/env python3
"""
calibrate_skin_single_subject.py
Rapid Single-Subject Skin Temperature Calibration & Curve Fitter
SIH26181 ShrikeFi Health Companion (MAX30102 Die Temp + BME280 Ambient)

Fits the constrained physical thermal divider model with Newton thermal lead:
    T_die_pred(t) = T_die(t) + tau_lead * max(0, dT_die/dt)
    T_ref - T_amb = b0 + b1 * (T_die_pred - T_amb)
    => T_skin = b0 + b1 * T_die_pred + (1 - b1) * T_amb
"""

import argparse
import json
import math
import os
import re
import sys
import time

try:
    import serial
except ImportError:
    serial = None

CALIB_RE = re.compile(
    r"\[CALIB\]\s*DIE=([-\d.]+),AMB=([-\d.]+),CONTACT=(\d+),SLOPE=([-\d.]+),SETTLED=(\d+),EST=([-\d.]+)"
)


def parse_calib_line(line: str):
    m = CALIB_RE.search(line)
    if not m:
        return None
    return {
        "die": float(m.group(1)),
        "amb": float(m.group(2)),
        "contact": int(m.group(3)),
        "slope": float(m.group(4)),
        "settled": int(m.group(5)),
        "est": float(m.group(6)),
    }


def capture_contact_trace(port: str, baud: int, hold_sec: float = 20.0, timeout_sec: float = 60.0):
    """Wait for finger contact on COM port, then record hold_sec of [CALIB] samples."""
    if serial is None:
        raise RuntimeError("pyserial is required (pip install pyserial)")

    ser = serial.Serial(port, baud, timeout=1.0)
    trace = []
    start_wait = time.time()
    contact_t0 = None

    print(f"[SERIAL] Connected to {port} @ {baud}. Waiting for finger contact on MAX30102...")
    sys.stdout.flush()

    try:
        while True:
            if (time.time() - start_wait) > timeout_sec and contact_t0 is None:
                raise TimeoutError("Timed out waiting for finger contact.")

            raw = ser.readline().decode("utf-8", errors="ignore").strip()
            sample = parse_calib_line(raw)
            if not sample:
                continue

            if sample["contact"] == 1 and sample["die"] > 20.0:
                if contact_t0 is None:
                    contact_t0 = time.time()
                    print("[CONTACT] Finger detected! Hold steady...")
                    sys.stdout.flush()

                elapsed = time.time() - contact_t0
                sample["t"] = round(elapsed, 2)
                trace.append(sample)
                print(
                    f"  t={elapsed:5.1f}s | DIE={sample['die']:5.2f} C | AMB={sample['amb']:5.2f} C "
                    f"| SLOPE={sample['slope']:+.4f} C/s | EST={sample['est']:5.2f} C"
                )
                sys.stdout.flush()

                if elapsed >= hold_sec:
                    print("[DONE] Hold complete! You can lift your finger now.")
                    sys.stdout.flush()
                    break
            else:
                if contact_t0 is not None and len(trace) >= 5:
                    print("[INFO] Finger lifted early; using captured trace.")
                    sys.stdout.flush()
                    break
    finally:
        ser.close()

    return trace


def fit_thermal_divider(points):
    """
    Fit:
      1) tau_lead_s that minimizes difference between early (5-12s) die_pred and end-of-hold die_pred
      2) b0, b1 in (T_ref - T_amb) = b0 + b1 * (T_die_pred - T_amb), with b2 = 1 - b1
    """
    if len(points) < 2:
        raise ValueError("Need at least 2 calibration points to fit.")

    # Step 1: Optimize tau_lead_s across traces if traces have time series
    best_tau = 5.50
    traces_with_series = [p["trace"] for p in points if "trace" in p and len(p["trace"]) >= 6]
    if traces_with_series:
        best_err = 1e9
        for tau_cand_x10 in range(0, 121, 5):  # 0.0s to 12.0s in 0.5s steps
            tau = tau_cand_x10 / 10.0
            sq_err = 0.0
            cnt = 0
            for tr in traces_with_series:
                final_die = sum(s["die"] for s in tr[-3:]) / min(3, len(tr))
                final_slope = max(0.0, min(0.30, sum(s["slope"] for s in tr[-3:]) / min(3, len(tr))))
                target_asym = final_die + 0.5 * tau * final_slope
                for s in tr:
                    if 4.0 <= s.get("t", 0.0) <= 14.0:
                        sl = max(0.0, min(0.30, s["slope"]))
                        pred = s["die"] + tau * sl
                        sq_err += (pred - target_asym) ** 2
                        cnt += 1
            if cnt > 0 and sq_err < best_err:
                best_err = sq_err
                best_tau = tau

    # Step 2: Compute effective (T_die_pred, T_amb, T_ref) for each point
    xs = []
    ys = []
    processed = []
    for p in points:
        t_ref = float(p["t_ref"])
        if "trace" in p and len(p["trace"]) >= 3:
            tail = p["trace"][-4:]
            t_die = sum(s["die"] for s in tail) / len(tail)
            t_amb = sum(s["amb"] for s in tail) / len(tail)
            slope = sum(max(0.0, min(0.30, s["slope"])) for s in tail) / len(tail)
            # Residual lead at end of 20s hold (scaled by 0.35 since 20s is mostly settled)
            t_die_eff = t_die + 0.35 * best_tau * slope
        else:
            t_die_eff = float(p["t_die"])
            t_amb = float(p["t_amb"])

        x = t_die_eff - t_amb
        y = t_ref - t_amb
        xs.append(x)
        ys.append(y)
        processed.append({"t_die": t_die_eff, "t_amb": t_amb, "t_ref": t_ref, "x": x, "y": y})

    n = len(xs)
    mean_x = sum(xs) / n
    mean_y = sum(ys) / n
    s_xx = sum((x - mean_x) ** 2 for x in xs)
    s_xy = sum((x - mean_x) * (y - mean_y) for x, y in zip(xs, ys))

    if s_xx < 1e-4:
        # Fallback if all fingers were at nearly identical temperature: fix b1 = 1.80
        b1 = 1.80
        b0 = mean_y - b1 * mean_x
    else:
        b1 = s_xy / s_xx
        # Clamp b1 to physically sound divider range [1.05, 3.20]
        b1 = max(1.05, min(3.20, b1))
        b0 = mean_y - b1 * mean_x

    b2 = 1.0 - b1

    # Compute in-sample RMSE and Leave-One-Out RMSE
    residuals = []
    for item in processed:
        pred = b0 + b1 * item["t_die"] + b2 * item["t_amb"]
        residuals.append(item["t_ref"] - pred)

    rmse = math.sqrt(sum(r * r for r in residuals) / n)

    loo_errs = []
    if n >= 3:
        for i in range(n):
            sub_x = [xs[j] for j in range(n) if j != i]
            sub_y = [ys[j] for j in range(n) if j != i]
            mx = sum(sub_x) / len(sub_x)
            my = sum(sub_y) / len(sub_y)
            sxx = sum((x - mx) ** 2 for x in sub_x)
            sxy = sum((x - mx) * (y - my) for x, y in zip(sub_x, sub_y))
            sb1 = max(1.05, min(3.20, sxy / sxx)) if sxx >= 1e-4 else b1
            sb0 = my - sb1 * mx
            pred_i = sb0 + sb1 * xs[i] + processed[i]["t_amb"]
            loo_errs.append(processed[i]["t_ref"] - pred_i)
        loo_rmse = math.sqrt(sum(e * e for e in loo_errs) / len(loo_errs))
    else:
        loo_rmse = rmse

    return {
        "b0": round(b0, 4),
        "b1": round(b1, 4),
        "b2": round(b2, 4),
        "tau_lead_s": round(best_tau, 2),
        "rmse": round(rmse, 3),
        "loo_rmse": round(loo_rmse, 3),
        "residuals": [round(r, 3) for r in residuals],
        "n_points": n,
    }


def main():
    parser = argparse.ArgumentParser(description="ShrikeFi Skin Temperature Calibration Utility")
    parser.add_argument("--port", default="COM4", help="Serial port (default: COM4)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--capture", type=float, default=0.0, help="Capture a single hold of N seconds and print JSON")
    parser.add_argument("--fit", default="", help="Fit from JSON file of points")
    args = parser.parse_args()

    if args.capture > 0:
        tr = capture_contact_trace(args.port, args.baud, hold_sec=args.capture)
        if not tr:
            print("ERROR: No contact trace captured.")
            sys.exit(1)
        tail = tr[-4:] if len(tr) >= 4 else tr
        avg_die = sum(s["die"] for s in tail) / len(tail)
        avg_amb = sum(s["amb"] for s in tail) / len(tail)
        avg_slope = sum(s["slope"] for s in tail) / len(tail)
        print(
            f"\n[SUMMARY] Samples={len(tr)} | End DIE={avg_die:.2f} C | AMB={avg_amb:.2f} C | End SLOPE={avg_slope:+.4f} C/s"
        )
        return

    if args.fit and os.path.exists(args.fit):
        with open(args.fit, "r", encoding="utf-8") as f:
            pts = json.load(f)
        res = fit_thermal_divider(pts)
        print(json.dumps(res, indent=2))
        return

    print("=================================================================")
    print("  SIH26181 ShrikeFi Rapid 6-Point Skin Temperature Calibration")
    print("=================================================================")
    states = [
        ("Point 1/6: NEUTRAL Finger #1", "Normal resting finger at room temperature."),
        ("Point 2/6: WARM Finger #1", "Rub hands briskly or hold warm water cup for 20s, dry finger."),
        ("Point 3/6: COOL Finger #1", "Touch ice/cold bottle for 5-8s ONLY (keep >= 32.2 C so DT-112 reads!)."),
        ("Point 4/6: NEUTRAL Finger #2", "Normal resting finger at room temperature."),
        ("Point 5/6: WARM Finger #2", "Rub hands briskly or hold warm water cup for 20s, dry finger."),
        ("Point 6/6: COOL Finger #2", "Touch ice/cold bottle for 5-8s ONLY (keep >= 32.2 C so DT-112 reads!)."),
    ]

    points = []
    for title, instr in states:
        print(f"\n--- {title} ---")
        print(f"Instruction: {instr}")
        input("Press ENTER when ready, then place your finger on MAX30102 for 20s...")
        tr = capture_contact_trace(args.port, args.baud, hold_sec=20.0)
        while True:
            val_str = input("Immediately measure the SAME fingertip with DT-112 thermometer (°C): ").strip()
            try:
                t_ref = float(val_str)
                if 31.5 <= t_ref <= 41.0:
                    break
                print("Please enter a valid Celsius reading between 31.5 and 41.0.")
            except ValueError:
                print("Invalid number. Try again.")
        points.append({"state": title, "t_ref": t_ref, "trace": tr})

    out_path = os.path.join(os.path.dirname(__file__), "skin_calib_session.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(points, f, indent=2)

    res = fit_thermal_divider(points)
    print("\n=================================================================")
    print("  CALIBRATION FIT COMPLETE")
    print("=================================================================")
    print(f"  b0         = {res['b0']:+.4f}f")
    print(f"  b1         = {res['b1']:+.4f}f")
    print(f"  b2         = {res['b2']:+.4f}f")
    print(f"  tau_lead_s = {res['tau_lead_s']:.2f}f")
    print(f"  In-Sample RMSE     = {res['rmse']:.3f} C")
    print(f"  Leave-One-Out RMSE = {res['loo_rmse']:.3f} C")
    print("=================================================================")


if __name__ == "__main__":
    main()
