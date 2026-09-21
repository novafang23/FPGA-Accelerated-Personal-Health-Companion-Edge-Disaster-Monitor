#!/usr/bin/env python3
"""
TypeSafe AI System One Intelligent Triage & Telemetry Bridge
============================================================
Project: VALOR (SIH26181) Personal Health Companion & Edge Disaster Monitor.
Integrates TypeSafe System One (Jev model) to provide calibrated semantic judgments
that replace fragile string parsing, ambiguous noise-rejection heuristics,
and conflicting multi-hazard disaster advisories.

Usage:
  python scripts/typesafe_triage.py --test
  python scripts/typesafe_triage.py parse "[TELEMETRY] HR=135,SPO2=89,RMSSD=14.2,TEMP=42.5,HUM=65,PM25=185,RR=28,SQI=0.55"
  python scripts/typesafe_triage.py arbitrate --hr 142 --spo2 87 --rmssd 9.5 --rr 30 --sqi 0.52
  python scripts/typesafe_triage.py fuse --hr 125 --spo2 89 --temp 43.0 --hum 60.0 --pm25 220.0
"""

import sys
import os
import json
import argparse
import urllib.request
import urllib.error

# Ensure UTF-8 output on Windows consoles
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

API_URL = "https://api.typesafe.ai/v1/systemone"
DEFAULT_MODEL = "jev-latest"


def get_api_key():
    """Retrieve TypeSafe API key from environment or project config."""
    key = os.environ.get("TYPESAFE_API_KEY")
    if not key:
        root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        env_file = os.path.join(root_dir, ".env")
        if os.path.exists(env_file):
            with open(env_file, "r", encoding="utf-8-sig") as f:
                for line in f:
                    if line.strip().startswith("TYPESAFE_API_KEY="):
                        key = line.split("=", 1)[1].strip().strip('"').strip("'")
                        break
    return key


def call_typesafe(state, questions, model=DEFAULT_MODEL, api_key=None):
    """
    Call TypeSafe System One API with a given state and question dictionary.
    Returns the parsed response dictionary containing answers, confidence, and probabilities.
    """
    if not api_key:
        api_key = get_api_key()
    if not api_key:
        raise ValueError(
            "TYPESAFE_API_KEY not found in environment or .env file. "
            "Please obtain a key from https://console.typesafe.ai/keys."
        )

    payload = {
        "state": state,
        "model": model,
        "questions": questions,
    }

    req_data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        API_URL,
        data=req_data,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {api_key}",
        },
    )

    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        err_msg = e.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"TypeSafe API error HTTP {e.code}: {err_msg}")
    except Exception as e:
        raise RuntimeError(f"TypeSafe request failed: {e}")


# ==============================================================================
# Capability 1: Robust Telemetry Line Classification & Parsing (Opportunity #1)
# ==============================================================================
def parse_telemetry_line(raw_line):
    """
    Classify telemetry and extract fields without failing on formatting differences,
    missing commas, or newly added metrics (RR, SQI).
    """
    questions = {
        "packet_type": {
            "type": "choice",
            "instructions": "Classify the purpose of this incoming serial/MQTT hardware log message",
            "criteria": {
                "live_vitals": "Contains active numerical vital signs (heart rate, SpO2, RMSSD, temperature, etc.)",
                "sensor_transition": "Reports sensor detachment, finger removal, or calibration warmup without stable vitals",
                "raw_waveform": "Streams optical PPG ADC samples or oscilloscope data (e.g., [PPG] <number>)",
                "system_log": "General firmware debug messages, WiFi connection notices, or initialization logs",
            },
        },
        "is_active_monitoring": {
            "type": "noul",
            "instructions": "Does this message reflect active physiological vital measurements rather than a detached or idle sensor?",
        },
        "sensor_warmup": {
            "type": "noul",
            "instructions": "Does this message indicate that the sensor is currently acquiring, calibrating, or warming up?",
        },
    }

    res = call_typesafe(state=raw_line, questions=questions)
    return res


# ==============================================================================
# Capability 2: Motion Artifact vs. Agonal Crisis Arbitration (Opportunity #2)
# ==============================================================================
def arbitrate_noise_vs_crisis(hr, spo2, rmssd, rr, sqi):
    """
    When signal quality is questionable (0.35 <= SQI <= 0.75), determine whether
    to hold triage for motion stabilization or escalate immediately for emergency care.
    """
    state = (
        f"Patient Vital State: Heart Rate = {hr:.1f} bpm, SpO2 = {spo2:.1f}%, "
        f"HRV RMSSD = {rmssd:.1f} ms, Respiration Rate = {rr:.1f} br/min. "
        f"Observed PPG Signal Quality Index (SQI) = {sqi:.2f} (scale 0.0 to 1.0, where <0.70 indicates signal degradation)."
    )

    questions = {
        "is_motion_artifact": {
            "type": "noul",
            "instructions": (
                "Is this signal degradation consistent with normal motion noise / sensor slippage "
                "in a stable patient, where alarms should be held to avoid alarm fatigue?"
            ),
        },
        "life_threat_override": {
            "type": "noul",
            "instructions": (
                "Does the combination of vitals (extreme tachycardia/bradycardia, severe hypoxia, "
                "or autonomic collapse) indicate a life-threatening crisis where noise suppression "
                "MUST be bypassed to dispatch emergency rescue immediately?"
            ),
        },
        "urgency_score": {
            "type": "score",
            "instructions": "Rate the clinical urgency level for immediate medical escalation",
            "criteria": [
                "Stable vitals; safe to suppress alarm and hold triage while sensor stabilizes",
                "Mildly abnormal; prompt user to hold finger still for re-acquisition",
                "Significant physiological strain; monitor closely with warning advisory",
                "Acute life-threatening collapse; immediate dispatch and bypass noise filter",
            ],
        },
    }

    res = call_typesafe(state=state, questions=questions)
    return res


# ==============================================================================
# Capability 3: Multi-Hazard Conflict Resolution & Action Directives (Opportunity #3)
# ==============================================================================
def resolve_disaster_fusion(hr, spo2, rmssd, rr, temp_c, hum_pct, pm25):
    """
    Fuses environmental sensor readings and clinical vitals into an unambiguous,
    prioritized first-responder tactical action, resolving conflicts between heat,
    smoke/pollution, and cold stress.
    """
    state = (
        f"Field Encounter Context:\n"
        f"- Patient Vitals: Heart Rate = {hr:.1f} bpm, SpO2 = {spo2:.1f}%, "
        f"RMSSD = {rmssd:.1f} ms, Respiration Rate = {rr:.1f} br/min.\n"
        f"- Environment: Ambient Temperature = {temp_c:.1f} °C, Relative Humidity = {hum_pct:.1f}%, "
        f"Air PM2.5 = {pm25:.1f} µg/m³."
    )

    questions = {
        "primary_threat": {
            "type": "choice",
            "instructions": "Identify the primary clinical/environmental threat that poses the highest immediate risk to patient survival",
            "criteria": {
                "thermal_hyperthermia": "Heat stroke, severe heat exhaustion, or heat index crisis",
                "toxic_asphyxiation": "Hazardous particulate inhalation (wildfire smoke / industrial dust) causing hypoxia",
                "cardiac_collapse": "Severe arrhythmia, autonomic shock, or cardiovascular decompensation",
                "hypothermia_cold": "Extreme ambient cold or cold-water exposure causing core temperature drop",
                "none_stable": "Conditions and vitals are within safe or manageable limits",
            },
        },
        "tactical_countermeasure": {
            "type": "choice",
            "instructions": "Select the single most urgent primary countermeasure for field responders",
            "criteria": {
                "evacuate_clean_shelter": "Relocate patient immediately to clean air or enclosed filtered shelter",
                "active_cooling_hydration": "Move to shade, mist with cold water, fan vigorously, and provide oral rehydration",
                "active_warming_insulate": "Remove wet clothing, insulate core with emergency blanket, apply gentle warming",
                "oxygen_airway_position": "Administer supplemental oxygen, maintain patent airway, rest in semi-Fowler position",
                "continue_routine_monitoring": "Maintain observation, no emergency intervention required",
            },
        },
        "immediate_life_threat": {
            "type": "noul",
            "instructions": "Is the patient in imminent danger of mortality within the next 30 minutes without intervention?",
        },
    }

    res = call_typesafe(state=state, questions=questions)
    return res


# ==============================================================================
# Standalone Test Suite
# ==============================================================================
def run_test_suite():
    """Run comprehensive test suite against live TypeSafe API."""
    print("=" * 70)
    print("  TypeSafe System One (Jev) Intelligent Triage Test Suite")
    print("  Project VALOR (SIH26181) — Edge Health & Disaster Companion")
    print("=" * 70)

    api_key = get_api_key()
    if not api_key:
        print("[FAIL] TYPESAFE_API_KEY is not set!")
        sys.exit(1)
    print(f"[*] API Key detected: {api_key[:15]}...")
    print(f"[*] Target Model: {DEFAULT_MODEL}\n")

    # Test 1: Telemetry stream classification
    print("[TEST 1] Telemetry Stream Demuxing (Live vitals vs. Detachment)")
    line1 = "[TELEMETRY] HR=142.0,SPO2=87.5,RMSSD=11.2,TEMP=41.8,HUM=62.0,PM25=165.0,RR=28.0,SQI=0.88"
    line2 = "[TELEMETRY] NO_FINGER,TEMP=26.4,HUM=45.0,PM25=18.0"

    print(f"  Input 1: {line1}")
    r1 = parse_telemetry_line(line1)
    ans1 = r1.get("answers", {})
    pkt1 = ans1.get("packet_type", {}).get("choice")
    f_pres1 = ans1.get("is_active_monitoring", {}).get("noul", 0.0)
    print(f"  -> Packet Type: {pkt1} (confidence: {ans1.get('packet_type', {}).get('confidence', 0):.2f})")
    print(f"  -> Active Monitoring: {f_pres1:.2f}")
    assert pkt1 == "live_vitals", f"Expected live_vitals, got {pkt1}"
    assert f_pres1 >= 0.70, f"Expected is_active_monitoring >= 0.70, got {f_pres1}"

    print(f"  Input 2: {line2}")
    r2 = parse_telemetry_line(line2)
    ans2 = r2.get("answers", {})
    pkt2 = ans2.get("packet_type", {}).get("choice")
    f_pres2 = ans2.get("is_active_monitoring", {}).get("noul", 0.0)
    print(f"  -> Packet Type: {pkt2}")
    print(f"  -> Active Monitoring: {f_pres2:.2f}")
    assert pkt2 == "sensor_transition", f"Expected sensor_transition, got {pkt2}"
    assert f_pres2 <= 0.30, f"Expected is_active_monitoring <= 0.30, got {f_pres2}"
    print("  [PASS] Telemetry stream classification verified.\n")

    # Test 2: Motion Artifact vs. Agonal Crisis
    print("[TEST 2] Noise vs. Crisis Arbitration (Borderline SQI = 0.52)")
    # Scenario: Low SQI, but severe hypoxia (86%) and tachycardia (148 bpm)
    r_arb = arbitrate_noise_vs_crisis(hr=148.0, spo2=86.0, rmssd=9.0, rr=32.0, sqi=0.52)
    ans_arb = r_arb.get("answers", {})
    override = ans_arb.get("life_threat_override", {}).get("noul", 0.0)
    urgency = ans_arb.get("urgency_score", {}).get("score", 0.0)
    print(f"  -> Life Threat Override (Noul): {override:.2f}")
    print(f"  -> Urgency Score (0-3): {urgency:.1f}")
    assert override >= 0.75, f"Expected life_threat_override >= 0.75, got {override}"
    assert urgency >= 2.0, f"Expected urgency >= 2.0, got {urgency}"
    print("  [PASS] Crisis escalation overrides noise suppression correctly.\n")

    # Test 3: Multi-Hazard Disaster Conflict Fusion
    print("[TEST 3] Multi-Hazard Conflict Fusion (Severe Wildfire Smoke + Tachycardia)")
    # Patient in wildfire environment: PM2.5 = 320 ug/m3, ambient temp 34 C, SpO2 88%, HR 122 bpm
    r_fuse = resolve_disaster_fusion(
        hr=122.0, spo2=88.0, rmssd=18.0, rr=26.0, temp_c=34.0, hum_pct=40.0, pm25=320.0
    )
    ans_fuse = r_fuse.get("answers", {})
    threat = ans_fuse.get("primary_threat", {}).get("choice")
    action = ans_fuse.get("tactical_countermeasure", {}).get("choice")
    print(f"  -> Dominant Threat: {threat}")
    print(f"  -> Tactical Countermeasure: {action}")
    assert threat in ["toxic_asphyxiation", "cardiac_collapse"], f"Unexpected threat: {threat}"
    assert action in ["evacuate_clean_shelter", "oxygen_airway_position"], f"Unexpected action: {action}"
    print("  [PASS] Conflict resolution synthesized actionable tactical directive.\n")

    print("=" * 70)
    print("  >>> ALL TYPESAFE SYSTEM ONE TESTS PASSED (100%) <<<")
    print("=" * 70)


def main():
    parser = argparse.ArgumentParser(description="TypeSafe System One Triage Bridge for VALOR")
    parser.add_argument("--test", action="store_true", help="Run automated test suite against TypeSafe API")
    subparsers = parser.add_subparsers(dest="command")

    # test
    subparsers.add_parser("test", help="Run automated test suite against TypeSafe API")

    # parse
    p_parse = subparsers.add_parser("parse", help="Classify a raw telemetry string")
    p_parse.add_argument("line", help="Raw line from UART or MQTT")

    # arbitrate
    p_arb = subparsers.add_parser("arbitrate", help="Arbitrate noise vs. crisis for questionable SQI")
    p_arb.add_argument("--hr", type=float, required=True, help="Heart rate in bpm")
    p_arb.add_argument("--spo2", type=float, required=True, help="Oxygen saturation percentage")
    p_arb.add_argument("--rmssd", type=float, default=25.0, help="HRV RMSSD in ms")
    p_arb.add_argument("--rr", type=float, default=16.0, help="Respiration rate in br/min")
    p_arb.add_argument("--sqi", type=float, default=0.55, help="PPG SQI index (0.0 - 1.0)")

    # fuse
    p_fuse = subparsers.add_parser("fuse", help="Fuse multi-hazard disaster readings into tactical action")
    p_fuse.add_argument("--hr", type=float, required=True, help="Heart rate in bpm")
    p_fuse.add_argument("--spo2", type=float, required=True, help="Oxygen saturation percentage")
    p_fuse.add_argument("--rmssd", type=float, default=25.0, help="HRV RMSSD in ms")
    p_fuse.add_argument("--rr", type=float, default=16.0, help="Respiration rate in br/min")
    p_fuse.add_argument("--temp", type=float, required=True, help="Ambient temperature in °C")
    p_fuse.add_argument("--hum", type=float, default=50.0, help="Relative humidity percentage")
    p_fuse.add_argument("--pm25", type=float, required=True, help="PM2.5 concentration in µg/m³")

    args = parser.parse_args()

    if args.command == "test" or args.test or len(sys.argv) == 1:
        run_test_suite()
    elif args.command == "parse":
        res = parse_telemetry_line(args.line)
        print(json.dumps(res, indent=2))
    elif args.command == "arbitrate":
        res = arbitrate_noise_vs_crisis(args.hr, args.spo2, args.rmssd, args.rr, args.sqi)
        print(json.dumps(res, indent=2))
    elif args.command == "fuse":
        res = resolve_disaster_fusion(args.hr, args.spo2, args.rmssd, args.rr, args.temp, args.hum, args.pm25)
        print(json.dumps(res, indent=2))
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
