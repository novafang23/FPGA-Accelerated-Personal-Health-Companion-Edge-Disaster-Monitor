#!/usr/bin/env python3
"""Analyse a captured ShrikeFi console session.

Counterpart to replay_fpga_link_log.py. That tool checks the FPGA; this one
checks the FIRMWARE pipeline that consumes it - the IBI acceptance rules, the
eventual HRV statistics, and whether the two detectors hand over cleanly.

HOW TO CAPTURE
--------------
    # in firmware/shrikefi
    idf.py -p COM5 build flash monitor

Hold a finger for 60-120 s, press Ctrl+], then point this script at
firmware/shrikefi/build/log/idf_py_stdout_output_<pid>.

If a background process is doing the capture instead, any file containing the
raw console lines works - the script only reads text.

WHAT IT REPORTS
---------------
1. Crest-detector cadence, split into contact sessions. Gives the detector's
   true interval distribution and how often it double-fires (< 400 ms) or drops
   a beat (> 1500 ms).
2. IBI count change points, i.e. how fast the HRV window is filling.
3. RMSSD over time, with every upward JUMP flagged.
4. For each jump, the crest intervals in that window - which is how the cause
   gets identified rather than guessed.

RESULTS FROM THE 2026-09-13 00:58 CAPTURE
-----------------------------------------
Session 2 (t=380034..486074, 106 s, 125 crests):

    interval median 770 ms (77.9 BPM), min 360, max 1770
    <400 ms (split double-fire) : 3
    400..1500 ms                : 115
    >1500 ms (missed beat)      : 6
    escape warnings             : none

    RMSSD settled at 27.8 ms - a healthy resting value - then jumped to 152.0 ms
    between t=397194 and t=398264.

The jump correlates with crests at 396234 and 397534: a 1300 ms interval. The
rhythm is 770 ms, so that is a dropped crest - but 1300 < IBI_MAX_MS (1500), so
it was accepted. RMSSD is an RMS of successive differences, so one interval is
enough:

    before: n=15, RMSSD 27.8  ->  sum(diff^2) =  772.84 * 14 =  10,820
    after : n=16, RMSSD 152.0 ->  sum(diff^2) = 23104.00 * 15 = 346,560
    delta = 335,740  ->  sqrt = 579 ms, one difference of ~579 ms (1300 - 721)

That is the whole cause. It then decayed only by dilution, 152 -> 113 ms over
the following 87 s, because the HRV window holds 300 intervals and never
forgets. Hence the missed-beat ceiling (IBI_MISSED_BEAT_RATIO in
main_shrikefi.c), which rejects intervals above ~1.6x this subject's own median.
"""

import argparse
import math
import re
import sys

BEAT = re.compile(r"^I \((\d+)\).*\[FPGA ACCEL\] Systolic crest detected! Filtered=(\d+)")
IBI_CNT = re.compile(r"^I \((\d+)\).*IBI samples: (\d+) \(need")
IBI_OPT = re.compile(r"^I \((\d+)\).*\| IBI: (\d+) \(need")
RMSSD = re.compile(r"^I \((\d+)\).*RMSSD: ([\d.]+) ms")
HANDOVER = re.compile(r"^I \((\d+)\).*IBI detector handover -> (\w+)")
ESCAPE = re.compile(r"^I \((\d+)\).*IBI filter rejected (\d+) intervals")
# Per-interval audit trace emitted by ibi_pipeline_submit() when
# SHRIKEFI_IBI_TRACE is on: IBI <src> <ms> <verdict> ref=<r> n=<c> rmssd=<m>
IBI_TRACE = re.compile(r"^IBI (\d+) (\d+) (\S+) ref=(\d+) n=(\d+) rmssd=([\d.]+)")
NOFINGER = re.compile(r"Status: NO FINGER")

SESSION_GAP_MS = 3000     # a gap this long starts a new contact session
SPLIT_MS = 400            # matches IBI_MIN_MS  in main_shrikefi.c
HARD_GAP_MS = 1500        # matches IBI_MAX_MS  in main_shrikefi.c
JUMP_MS = 20.0            # RMSSD rise worth investigating

# idf.py monitor writes its log with --force-color, so every line is wrapped in
# ANSI SGR sequences. A bare `^I (123)` regex silently matches nothing against
# those files, which is the capture path the docstring tells you to use.
ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")


def read_lines(path):
    with open(path, encoding="utf-8", errors="replace") as fh:
        return [ANSI_RE.sub("", ln).rstrip("\r") for ln in fh]


def parse(path):
    beats, counts, rmssd, handovers, escapes, trace = [], [], [], [], [], []
    for ln in read_lines(path):
        if m := BEAT.search(ln):     beats.append((int(m.group(1)), int(m.group(2))))
        if m := IBI_CNT.search(ln):  counts.append((int(m.group(1)), int(m.group(2))))
        if m := IBI_OPT.search(ln):  counts.append((int(m.group(1)), int(m.group(2))))
        if m := RMSSD.search(ln):    rmssd.append((int(m.group(1)), float(m.group(2))))
        if m := HANDOVER.search(ln): handovers.append((int(m.group(1)), m.group(2)))
        if m := ESCAPE.search(ln):   escapes.append((int(m.group(1)), int(m.group(2))))
        if m := IBI_TRACE.search(ln):
            trace.append((int(m.group(2)), m.group(3), int(m.group(4)),
                          int(m.group(5)), float(m.group(6))))
    return beats, counts, rmssd, handovers, escapes, trace


def sessions(beats):
    out, cur = [], [beats[0]]
    for b in beats[1:]:
        if b[0] - cur[-1][0] > SESSION_GAP_MS:
            out.append(cur)
            cur = [b]
        else:
            cur.append(b)
    out.append(cur)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("capture", help="captured console text")
    ap.add_argument("--jump-window", type=float, default=3000.0,
                    help="ms of crest history to show before an RMSSD jump")
    args = ap.parse_args()

    beats, counts, rmssd, handovers, escapes, trace = parse(args.capture)
    if not beats:
        sys.exit(f"no '[FPGA ACCEL]' lines found in {args.capture}")

    print(f"capture        : {args.capture}")
    print(f"crest events   : {len(beats)}")
    print(f"handovers      : {handovers}")
    print(f"escape warnings: {escapes or 'none'}")
    if not escapes:
        print("                 (good: the rejection rules are not over-rejecting)")

    print("\n=== 1. crest-detector cadence ===")
    segs = sessions(beats)
    for i, s in enumerate(segs):
        iv = [s[j + 1][0] - s[j][0] for j in range(len(s) - 1)]
        if not iv:
            continue
        ivs = sorted(iv)
        med = ivs[len(ivs) // 2]
        splits = [v for v in iv if v < SPLIT_MS]
        gaps = [v for v in iv if v > HARD_GAP_MS]
        ok = [v for v in iv if SPLIT_MS <= v <= HARD_GAP_MS]
        span = s[-1][0] - s[0][0]
        print(f"\n  session {i}: t={s[0][0]}..{s[-1][0]} ms, {len(s)} crests, "
              f"{span} ms")
        print(f"    interval median {med} ms ({60000/med:.1f} BPM)   "
              f"min {min(iv)}  max {max(iv)}")
        print(f"    <{SPLIT_MS} ms  (split double-fire): {len(splits):3d}  {splits[:12]}")
        print(f"    ok          : {len(ok):3d}")
        print(f"    >{HARD_GAP_MS} ms (missed beat)    : {len(gaps):3d}  {gaps[:12]}")
        if iv:
            print(f"    artifact rate: "
                  f"{100.0*(len(splits)+len(gaps))/len(iv):.1f}% of intervals")

    print("\n=== 2. IBI count change points ===")
    prev, changes = None, []
    for t, c in sorted(counts):
        if c != prev:
            changes.append((t, c))
            prev = c
    for t, c in changes:
        print(f"  {t:>8} ms   count {c}")

    print("\n=== 3. RMSSD over time (jumps flagged) ===")
    jumps = []
    prev_r = None
    for t, r in rmssd:
        flag = ""
        if prev_r is not None and r - prev_r > JUMP_MS:
            flag = "   <<< JUMP"
            jumps.append((t, prev_r, r))
        print(f"  {t:>8} ms   {r:6.1f} ms{flag}")
        prev_r = r

    print("\n=== 4. what caused each jump ===")
    if not jumps:
        print("  no jumps above "
              f"{JUMP_MS} ms - RMSSD is behaving")
    for t, before, after in jumps:
        # backtrack the exact arithmetic: RMSSD^2 * (n-1) is the running sum
        print(f"\n  jump at t={t} ms: {before:.1f} -> {after:.1f} ms")
        lo = t - args.jump_window
        window = [(bt, bf) for bt, bf in beats if lo <= bt <= t + 500]
        if len(window) >= 2:
            iv = [(window[k + 1][0] - window[k][0], window[k][0])
                  for k in range(len(window) - 1)]
            print(f"    crests in the preceding {args.jump_window:.0f} ms:")
            for v, at in iv:
                mark = ""
                if v > HARD_GAP_MS:
                    mark = "  rejected (>IBI_MAX_MS)"
                elif v > 1200:
                    mark = "  <-- spans ~2 cycles, ACCEPTED unless a ratio rule exists"
                print(f"      {at:>7} ms   interval {v:>5} ms{mark}")
            big = max(iv)
            print(f"    largest interval in window: {big[0]} ms at t={big[1]}")

    print("\n=== 5. arithmetic of the largest jump ===")
    if jumps:
        t, before, after = max(jumps, key=lambda j: j[2] - j[1])
        # We do not know n exactly, but the ratio of sums identifies the culprit.
        print(f"  largest rise: {before:.1f} -> {after:.1f} ms at t={t}")
        print("  RMSSD^2 * (n-1) is the running sum of squared successive diffs.")
        print("  A single extra difference d raises the sum by d^2, so if the")
        print("  count went from n to n+1 the culprit difference is:")
        for n in range(10, 40):
            s_before = before * before * (n - 1)
            s_after = after * after * n
            d = s_after - s_before
            if d > 0:
                print(f"    n={n:2d}: d = sqrt({d:.0f}) = {math.sqrt(d):6.1f} ms")

    print("\n=== 6. per-interval verdicts (SHRIKEFI_IBI_TRACE) ===")
    if not trace:
        print("  no 'IBI ...' lines found - rebuild with SHRIKEFI_IBI_TRACE=1 to get"
              " the per-interval audit trail")
    else:
        tally = {}
        for ms, verdict, ref, n, r in trace:
            tally[verdict] = tally.get(verdict, 0) + 1
        total = len(trace)
        print(f"  {total} intervals classified")
        for v in ("ok", "first", "low", "high", "pair", "gap", "abs-low"):
            if v in tally:
                print(f"    {v:8s} {tally[v]:4d}  ({100.0*tally[v]/total:5.1f}%)")
        unknown = {k: c for k, c in tally.items() if k not in
                   ("ok", "first", "low", "high", "pair", "gap", "abs-low")}
        if unknown:
            print(f"    UNRECOGNISED: {unknown}")

        rej = [(ms, v, ref, n) for ms, v, ref, n, _ in trace if v != "ok"]
        print(f"\n  {len(rej)} rejected intervals, with the rhythm reference at the time:")
        for ms, v, ref, n in rej:
            ratio = ms / ref if ref else 0.0
            print(f"    {ms:>5} ms  {v:8s} ref={ref:>4}  ratio={ratio:.2f}  n={n}")

        ok = [ms for ms, v, _, _, _ in trace if v == "ok"]
        if ok:
            o = sorted(ok)
            print(f"\n  accepted series: {len(ok)} intervals, median {o[len(o)//2]} ms "
                  f"({60000.0/o[len(o)//2]:.1f} BPM), min {o[0]}, max {o[-1]}")
            import math as _m
            d = [ok[i + 1] - ok[i] for i in range(len(ok) - 1)]
            if d:
                print(f"  successive-difference RMS of the ACCEPTED series: "
                      f"{_m.sqrt(sum(x*x for x in d)/len(d)):.1f} ms")
                print(f"  (that is the floor RMSSD would sit at; the reported RMSSD "
                      f"differs only because the window is longer than this capture)")
            print(f"  accepted sequence: {ok}")

    print()


if __name__ == "__main__":
    main()
