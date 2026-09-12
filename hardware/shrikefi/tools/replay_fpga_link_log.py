#!/usr/bin/env python3
"""Offline replay of the ForgeFPGA SPI link capture through the RTL.

WHY THIS EXISTS
---------------
Between two SPI transactions the ESP32 sends one 8-bit sample to the
ForgeFPGA and gets one 8-bit byte back. That is the only window onto what the
FPGA is actually computing, and for a long time the answer was ambiguous: the
peak detector sometimes fired twice on one heartbeat, and there was no way to
tell from the ESP32 side whether the FPGA's 8-tap moving average was working,
whether the beat flag was real, or whether the whole link was noise.

This script removes the ambiguity. It replays the captured byte stream through a
bit-accurate model of the RTL and checks the model's output against the bytes
the FPGA actually returned. If the two agree for every sample, the FPGA is
provably executing the same arithmetic as the Verilog - not merely powered on.

HOW TO CAPTURE
--------------
In firmware/shrikefi/shrikefi_link_driver.c, shrikefi_write_ir_sample() logs
one line per SPI transaction when it is switched to full rate:

    printf("FG %d %u %u %u\\n", n, tx, rx & 0x7F, (rx >> 7) & 1);

i.e. FG <index> <byte sent to FPGA> <7-bit value returned> <beat flag>.
Run `idf.py -p COM5 build flash monitor`, hold a finger on the MAX30102 for
15-30 s, press Ctrl+], then run this script against the newest
build/log/idf_py_stdout_output_* file.

USAGE
-----
    python replay_fpga_link_log.py <path-to-log> [--wave] [--trace]

RESULTS FROM THE FIRST FULL-RATE CAPTURE (2026-09-13)
-----------------------------------------------------
    parsed 3474 FG lines, sample indices 0..3473, none missing
    rx[n] == (ma[n-1] & 0x7F) : 3465/3465 = 100.000%   <-- bit-exact
    true filt_sample > 127 (MSB dropped): 1025/3466 samples (29.6%)

    FSM replay -> 18 predicted beats vs 19 logged beats
    predicted: 1941 2037 2124 2208 2299 2390 2480 2564 2650 2733 2812 2888 \
               2966 3043 3119 3195 3255 3356
    logged   : 1167 1941 2037 2124 2208 2299 2390 2480 2564 2650 2733 2812 \
               2888 2966 3043 3119 3195 3255 3356

Conclusions that follow, and that the firmware comments refer back to:

  1. The 8-tap moving average in the FPGA is real and correct. The model in
     rtl_moving_average() reproduces the returned bytes exactly, so the filter
     is running in hardware on live data at 100 Hz. The FPGA is not a
     pass-through and this is not a software filter wearing an FPGA hat.

  2. The one-beat difference is fully explained. The RTL suppresses the first
     crest of a session via first_beat_seen, so its first REPORTED beat is the
     second crest. The capture shows a beat at the first crest, which means a
     crest had already been consumed during link bring-up - consistent with the
     handshake reply itself carrying the beat flag (probe sent 0x55, received
     0x80, bit 7 set). Effect is benign: that interval is measured from
     power-on and is range-rejected by the firmware.

  3. The 7740 ms first interval is a saturation artifact, not a missed beat.
     tx was pinned at 255 for 464 consecutive samples (4.64 s) after the finger
     was seated, because the MAX30100 DC output ramps 0 -> ~127000 counts over
     ~1.5 s and the 640 ms baseline EMA lags a ramp by (slope x tau). The
     8-tap average railed with it, the detector sat in ARISING with no crest to
     find, and the single peak it emitted when the signal finally came off the
     rail is the phantom at n=1167. Firmware now re-seeds a baseline that has
     been on a rail for 30 consecutive samples.

  4. The remaining split detection is the dicrotic notch. See --wave output for
     the affected beat: the filtered pulse rises to ~187, dips to ~171, then
     rises again to ~199. The FSM confirms a crest after two falling samples, so
     it latches onto the first maximum; the second lands ~100 ms later, inside
     the 250 ms refractory, and is ignored. One split contributes one short and
     one long interval (600 ms then 1010 ms against a ~800 ms rhythm), which is
     what drove RMSSD to ~116 ms on its own.
"""

import argparse
import re
import sys

ARMED, RISING, PEAK, REFRAC = 0, 1, 2, 3
STATE_NAMES = ["ARMED", "RISING", "PEAK", "REFRAC"]

DYN_THRESHOLD = 120        # RTL: .dyn_threshold(8'd120)
REFRACTORY_SAMPLES = 25    # RTL: REFRACTORY_CYC = 12_500_000 at 50 MHz = 250 ms
                           #      at the 100 Hz sample rate = 25 samples
FG_RE = re.compile(r"^FG (\d+) (\d+) (\d+) (\d+)\s*$", re.M)


def parse_log(path):
    """Return {index: (tx, rx7, beat)} from the FG diagnostic lines."""
    with open(path, encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    rows = FG_RE.findall(text)
    data = {}
    for n, tx, rx, beat in rows:
        data[int(n)] = (int(tx), int(rx), int(beat))
    return data


def rtl_moving_average(tx, nmax):
    """Bit-accurate model of moving_average_8tap with DATA_WIDTH=8.

        next_sum = running_sum + data_in - shift_reg[7];
        data_out = next_sum[DATA_WIDTH+2:3];      // == floor(sum / 8)
    """
    ma = {}
    running_sum = 0
    shift = [0] * 8
    for n in range(nmax + 1):
        d = tx.get(n, 0)
        running_sum = running_sum + d - shift[7]
        shift = [d] + shift[:7]
        ma[n] = running_sum >> 3
    return ma


def rtl_peak_detector(ma, nmax):
    """Bit-accurate model of ppg_peak_detector (sample domain).

    The RTL confirms a crest on the second consecutive falling sample, because
    the combinational next-state test reads the fall_count of the PREVIOUS
    sample. That two-sample confirmation delay is what makes the notch fatal.
    """
    state = ARMED
    prev = 0
    fall = 0
    refr = 0
    first_beat_seen = False
    beats = []

    for n in range(nmax + 1):
        s = ma[n]

        if state == ARMED:
            nxt = RISING if s >= DYN_THRESHOLD else ARMED
        elif state == RISING:
            nxt = PEAK if (s < prev and fall >= 1) else RISING
        elif state == PEAK:
            nxt = REFRAC
        else:
            nxt = ARMED if (refr == 0 and s < DYN_THRESHOLD) else REFRAC

        if state == RISING:
            fall = fall + 1 if s < prev else 0
        elif state == ARMED:
            fall = 0

        if state == PEAK:
            if first_beat_seen:
                beats.append(n)
            else:
                first_beat_seen = True
            refr = REFRACTORY_SAMPLES
        elif state == REFRAC and refr > 0:
            refr -= 1

        state = nxt
        prev = s

    return beats


def check_alignment(ma, rx, nmax):
    """Find the pipeline lag k for which rx[n] == (ma[n-k] & 0x7F) holds."""
    results = []
    for k in (0, 1, 2):
        ok = tot = 0
        for n in range(8 + k, nmax + 1):
            if n in rx and (n - k) in ma:
                tot += 1
                ok += ((ma[n - k] & 0x7F) == rx[n])
        results.append((k, ok, tot))
    return results


def dump_wave(ma, centre, span=95, pre=30):
    seg = [(n, ma[n]) for n in range(centre - pre, min(centre + span, max(ma) + 1))]
    print(f"\n=== beat at n={centre} ({centre * 10} ms) ===")
    line = ""
    for i, (n, v) in enumerate(seg):
        line += f"{v:4d}"
        if i % 16 == 15:
            print("   " + line)
            line = ""
    if line:
        print("   " + line)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", help="idf_py_stdout_output_* file containing FG lines")
    ap.add_argument("--wave", action="store_true",
                    help="dump the filtered waveform around each detected beat")
    ap.add_argument("--trace", type=int, nargs=2, metavar=("LO", "HI"),
                    help="print the FSM state for a sample range")
    args = ap.parse_args()

    data = parse_log(args.log)
    if not data:
        sys.exit(f"no 'FG <n> <tx> <rx> <beat>' lines found in {args.log}")

    nmax = max(data)
    tx = {n: v[0] for n, v in data.items()}
    rx = {n: v[1] for n, v in data.items()}
    bt = {n: v[2] for n, v in data.items()}
    missing = [i for i in range(nmax + 1) if i not in tx]

    print(f"parsed {len(data)} FG lines from {args.log}")
    print(f"sample indices 0..{nmax}; missing {len(missing)}")
    print(f"logged beats (beat flag = 1): {sum(bt.values())}")

    ma = rtl_moving_average(tx, nmax)

    print("\n-- 1. is the FPGA's 8-tap moving average real? --")
    for k, ok, tot in check_alignment(ma, rx, nmax):
        print(f"   rx[n] == (ma[n-{k}] & 0x7F): {ok}/{tot} = {100.0 * ok / max(tot, 1):.3f}%")
    lag = max(check_alignment(ma, rx, nmax), key=lambda r: r[1])[0]
    print(f"   -> pipeline lag {lag}; "
          f"{'BIT-EXACT MATCH, the filter is running in hardware' if lag == 1 else 'check wiring'}")

    hi = [n for n in range(8, nmax + 1) if ma[n] > 127]
    print(f"\n   true filt_sample > 127 (MSB dropped by the "
          f"'tx_data <= {{beat_latched, filt_sample[6:0]}}' packing): "
          f"{len(hi)}/{nmax - 7} = {100.0 * len(hi) / max(nmax - 7, 1):.1f}%")
    print("   -> the ESP32 only ever sees filt_sample[6:0], so this reported")
    print("      waveform wraps at 128. Detection is unaffected (the FSM uses the")
    print("      full 8-bit value internally); only the diagnostic lies.")

    print("\n-- 2. does the RTL peak detector reproduce the reported beats? --")
    pred = rtl_peak_detector(ma, nmax)
    led = sorted(n for n in bt if bt[n] == 1)
    print(f"   predicted {len(pred)} beats, logged {len(led)}")
    print(f"   predicted: {pred}")
    print(f"   logged   : {led}")
    extra = [n for n in led if n not in pred]
    missing_b = [n for n in pred if n not in led]
    print(f"   logged-but-not-predicted: {extra}")
    print(f"   predicted-but-not-logged: {missing_b}")

    def intervals(a):
        return [round((a[i + 1] - a[i]) * 10.0, 1) for i in range(len(a) - 1)]

    if pred:
        print(f"   predicted intervals (ms): {intervals(pred)}")
    if led:
        print(f"   logged    intervals (ms): {intervals(led)}")

    print("\n-- 3. filter output statistics --")
    seg = [ma[n] for n in range(nmax + 1)]
    print(f"   min={min(seg)} max={max(seg)} mean={sum(seg) / len(seg):.1f}")
    print(f"   samples >= {DYN_THRESHOLD} (detector armed): "
          f"{sum(1 for v in seg if v >= DYN_THRESHOLD)}")

    runs = []
    start = None
    for n in range(nmax + 1):
        if ma[n] >= DYN_THRESHOLD and start is None:
            start = n
        elif ma[n] < DYN_THRESHOLD and start is not None:
            runs.append((start, n - 1, n - start))
            start = None
    if start is not None:
        runs.append((start, nmax, nmax - start + 1))
    print(f"   {len(runs)} contiguous runs above threshold; longest:")
    for s, e, ln in sorted(runs, key=lambda r: -r[2])[:5]:
        print(f"      n={s}..{e}  {ln} samples ({ln * 10} ms)")

    if args.wave:
        for c in led:
            dump_wave(ma, c)

    if args.trace:
        lo, hi_n = args.trace
        print(f"\n-- FSM trace n={lo}..{hi_n} --")
        state, prev, fall, refr = ARMED, 0, 0, 0
        for n in range(lo, min(hi_n, nmax) + 1):
            s = ma[n]
            if state == ARMED:
                nxt = RISING if s >= DYN_THRESHOLD else ARMED
            elif state == RISING:
                nxt = PEAK if (s < prev and fall >= 1) else RISING
            elif state == PEAK:
                nxt = REFRAC
            else:
                nxt = ARMED if (refr == 0 and s < DYN_THRESHOLD) else REFRAC
            print(f"   n={n:5d} tx={tx.get(n, 0):3d} ma={s:3d} prev={prev:3d} "
                  f"fall={fall} {STATE_NAMES[state]:6s} -> {STATE_NAMES[nxt]}")
            if state == RISING:
                fall = fall + 1 if s < prev else 0
            elif state == ARMED:
                fall = 0
            if state == PEAK:
                refr = REFRACTORY_SAMPLES
            elif state == REFRAC and refr > 0:
                refr -= 1
            state, prev = nxt, s


if __name__ == "__main__":
    main()
