#!/usr/bin/env python3
"""
fit_skin_temp_model.py - analyse VALOR's per-session skin-to-core OFFSET.

    python tools/fit_skin_temp_model.py tools/temp_validation_log.csv

WHAT THIS DOES, AND WHY IT IS NOT A REGRESSION
----------------------------------------------
The first version of this tool fitted global skin->core coefficients. That is
the wrong experiment. A fingertip sits 3-7 C below core and that gap is set by
vasoconstriction and room temperature, so a global coefficient does not
generalise and the fit fails for reasons that teach you nothing.

The published method for continuous monitoring is an INDIVIDUALISED CORRECTION
FACTOR: one reference reading per session, offset = T_ref - T_skin, then
track that offset. So this tool reports the DISTRIBUTION of offsets:

  * mean offset (what to use today)
  * SD across sessions (how much it moves)
  * whether it is stable enough to be a personal constant
  * whether it clusters by condition (cold hands vs warm room)

That answers the real question: how often must the calibration be redone?

It still refuses to work on degenerate data - a file where every reading is
identical cannot produce a meaningful SD, and pretending otherwise is how
confidence gets fabricated.
"""
import csv
import sys
import argparse

try:
    import numpy as np
except ImportError:
    sys.exit("Needs numpy: pip install numpy")

MIN_SESSIONS = 8
MIN_OFFSET_SPREAD_C = 0.05   # a constant offset means we learned nothing about stability


def load(path):
    try:
        f = open(path, newline="", encoding="utf-8")
    except FileNotFoundError:
        sys.exit(f"No such file: {path}\n"
                 "See docs/PROJECT_CONTEXT.md for the format.")
    except OSError as e:
        sys.exit(f"Cannot open {path}: {e}")

    rows = []
    with f:
        lines = [ln for ln in f if not ln.lstrip().startswith("#")]
    for r in csv.DictReader(lines):
        try:
            if (r.get("contact_ok") or "").strip() != "1":
                continue
            ref = float(r["t_ref_c"])
            sk = float(r["t_skin_c"])
            am = float(r["t_amb_c"])
        except (ValueError, KeyError, TypeError, AttributeError):
            continue
        ref_site = (r.get("ref_method") or "oral").strip()
        rows.append(dict(
            sid=(r.get("session_id") or "?").strip(),
            ref=ref, sk=sk, am=am,
            site=ref_site,
            cond=(r.get("condition") or "").strip(),
            note=(r.get("notes") or "").strip(),
            offset=ref - sk,
        ))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", default="tools/temp_validation_log.csv")
    args = ap.parse_args()

    rows = load(args.csv)
    n = len(rows)

    print("=" * 70)
    print(" VALOR skin-to-core OFFSET analysis - oral reference")
    print("=" * 70)

    if n < MIN_SESSIONS:
        print(f"\nREFUSING: {n} usable session(s); need >= {MIN_SESSIONS}.")
        print("This measures how STABLE the offset is. With a handful of")
        print("sessions the SD is not estimable, and reporting one would be")
        print("inventing precision. See docs/PROJECT_CONTEXT.md")
        return 2

    sessions = sorted({r["sid"] for r in rows})
    offs = np.array([r["offset"] for r in rows])
    mean = float(offs.mean())
    sd = float(offs.std(ddof=1))
    lo, hi = float(offs.min()), float(offs.max())

    if (hi - lo) < MIN_OFFSET_SPREAD_C:
        print(f"\nREFUSING: every offset is identical ({mean:.2f} C).")
        print("A constant offset means the measurement did not vary, which")
        print("usually means the same session was copied - not that the")
        print("offset is perfectly stable.")
        return 2

    print(f"\nsessions={len(sessions)}  rows={n}  subject=1 (see warning)")
    print(f"reference site(s): {', '.join(sorted({r['site'] for r in rows}))}")
    print("\nPer-session offset = T_oral - T_skin:")
    for r in rows:
        print(f"  {r['sid']:>4}  ref {r['ref']:5.1f}  skin {r['sk']:5.1f}  "
              f"-> offset {r['offset']:+5.2f} C   {r['cond']}")

    print(f"\n  mean offset = {mean:+.2f} C   (skin sits this far below core)")
    print(f"  SD          = {sd:.2f} C")
    print(f"  range       = [{lo:+.2f}, {hi:+.2f}] C")

    # ---- the decision this analysis exists to support ----------------------
    print("\n" + "-" * 70)
    print("WHAT THIS MEANS")
    print("-" * 70)
    if sd < 0.5:
        print(f"  STABLE (SD {sd:.2f} < 0.5 C). The offset is a usable personal")
        print(f"  constant: core = skin {mean:+.2f} C. Still re-check per session,")
        print("  and re-calibrate if the room or your hands change a lot.")
        claim = (f"Personal skin-to-core offset {mean:+.2f} +/- {sd:.2f} C "
                 f"(n=1, oral reference, {len(sessions)} sessions)")
    elif sd < 1.5:
        print(f"  CONDITIONAL (SD {sd:.2f} C). The offset moves with conditions,")
        print("  so it must be taken per session. Trend tracking is valid;")
        print("  a fixed constant is not.")
        claim = (f"Skin-to-core offset {mean:+.2f} +/- {sd:.2f} C, varies by "
                 f"session; taken per session (n=1, oral reference)")
    else:
        print(f"  UNSTABLE (SD {sd:.2f} C). A single fingertip site is too")
        print("  variable for absolute core temperature here. Use the TREND")
        print("  (peripheral thermal state) only; do not claim absolute core.")
        claim = ("Peripheral thermal TREND only; absolute skin-to-core offset "
                 "not reproducible at this site (n=1, oral reference)")

    # ---- does condition explain the spread? -------------------------------
    conds = {}
    for r in rows:
        if r["cond"]:
            conds.setdefault(r["cond"], []).append(r["offset"])
    if len(conds) > 1:
        print("\n  Offset by condition (does the room explain the spread?):")
        for c, vals in sorted(conds.items()):
            print(f"    {c:<16} n={len(vals):2d}  mean {np.mean(vals):+5.2f} C")

    print("\n" + "-" * 70)
    print("WHAT THIS IS, AND IS NOT")
    print("-" * 70)
    print("  * n=1 SUBJECT. Describes this person. Not transferable.")
    print("  * PERIPHERAL SKIN, not core. Calibrated only against YOUR oral")
    print("    reading, which is itself ~0.3-0.5 C below true core.")
    print("  * The NEWS2 temperature row stays UNINSTRUMENTED.")
    print(f"\n  Suggested one-line claim:\n  \"{claim}\"")
    return 0


if __name__ == "__main__":
    sys.exit(main())
