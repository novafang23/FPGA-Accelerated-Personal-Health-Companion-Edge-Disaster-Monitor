#!/usr/bin/env python3
"""Generate the flat ForgeFPGA source set from the shared sources.

Why this exists
---------------
The Renesas ForgeFPGA Workshop consumes ONE flat Verilog source set, so the
DSP modules that live in ``hardware/common/`` have to appear *inside* the file
the vendor tool compiles. Historically that was done by hand: the two modules
were pasted into ``forgefpga_ppg_top.v``, and the pasted copy silently drifted
away from ``hardware/common/`` (the Zynq path kept an older crest-detection
rule for months as a result).

This script removes the hand-maintained duplication. ``forgefpga_ppg_top.v``
only *instantiates* the DSP modules; the vendor-facing flat file is generated
from it plus ``hardware/common/`` on demand. Duplication still exists on disk,
but it is derived, so it cannot drift.

Usage
-----
    python gen_flat_source.py            # regenerate the flat source set
    python gen_flat_source.py --check    # exit 1 if it is stale (CI / pre-commit)

Exit codes: 0 ok, 1 stale or invalid input.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent

TOP = HERE / "forgefpga_ppg_top.v"
FLAT = HERE / "forgefpga_project" / "ffpga" / "src" / "forgefpga_ppg_top.v"

# The Renesas project file (forgefpga_project/FPGA-SHRIKE.ffpga) references this
# path, so the file has to exist -- but it must not be a second hand-maintained
# copy of the testbench. It is synced from the real one instead.
TB_SRC = HERE / "tb_forgefpga_system.v"
TB_DST = HERE / "forgefpga_project" / "ffpga" / "sim" / "tb_forgefpga_system.v"

# Order matters only for readability -- Verilog allows any declaration order.
# It mirrors the historical hand-maintained file so that regenerating it
# produces a minimal, reviewable diff.
COMMON_MODULES = [
    ("moving_average_8tap", REPO / "hardware" / "common" / "moving_average_8tap.v"),
    ("ppg_peak_detector", REPO / "hardware" / "common" / "ppg_peak_detector.v"),
]

BANNER = """\
// =============================================================================
// GENERATED FILE -- DO NOT EDIT
//
// Produced by hardware/shrikefi/gen_flat_source.py by combining
//   hardware/shrikefi/forgefpga_ppg_top.v        (the top level)
// with the shared DSP modules in hardware/common/.
//
// The Renesas ForgeFPGA Workshop needs one flat source set, which is the only
// reason this duplication exists. Edit the sources above, then re-run the
// generator. Hand-editing this file reintroduces the drift it was created to
// eliminate.
// =============================================================================

"""


def read_text(path: Path) -> str:
    # newline="" so on-disk CRLF survives the read intact; without it Python
    # translates line endings and every comparison against generated CRLF
    # text reports a false "stale".
    try:
        with path.open("r", encoding="utf-8", newline="") as handle:
            return handle.read()
    except FileNotFoundError:
        sys.exit("error: missing source: %s" % path)


def normalise(text: str) -> str:
    """Normalise to LF and strip trailing whitespace-only lines."""
    return text.replace("\r\n", "\n").replace("\r", "\n").rstrip("\n") + "\n"


def extract_module(text: str, module: str, origin: Path) -> str:
    """Return the `module <name> ... endmodule` region of a source file."""
    pattern = re.compile(
        r"^module\s+" + re.escape(module) + r"\b.*?^endmodule\s*$",
        re.MULTILINE | re.DOTALL,
    )
    match = pattern.search(text)
    if not match:
        sys.exit("error: %s declares no module %r" % (origin, module))
    return match.group(0)


def declared_modules(text: str) -> list[str]:
    return re.findall(r"^module\s+(\w+)", text, re.MULTILINE)


GENERATED_BANNER = """\
// =============================================================================
// GENERATED FILE -- DO NOT EDIT
//
// Synced from hardware/shrikefi/tb_forgefpga_system.v by
// hardware/shrikefi/gen_flat_source.py. It exists only because the Renesas
// project file references this path. Edit the original, then re-run the
// generator.
// =============================================================================

"""


def build_tb() -> str:
    tb = normalise(read_text(TB_SRC))
    return (GENERATED_BANNER + tb).replace("\n", "\r\n")


def build() -> str:
    top = normalise(read_text(TOP))

    # Guard against someone pasting the DSP modules back into the top level.
    # That is exactly the fork this generator exists to prevent, and it would
    # make the generated file a duplicate-module compile error.
    forbidden = {name for name, _ in COMMON_MODULES}
    present = forbidden.intersection(declared_modules(top))
    if present:
        sys.exit(
            "error: %s declares %s, which must live only in hardware/common/.\n"
            "       Remove the inline copy and instantiate the shared module instead."
            % (TOP.name, ", ".join(sorted(present)))
        )

    parts = [BANNER, top]
    for module, path in COMMON_MODULES:
        if module not in declared_modules(top):  # only referenced, not defined
            parts.append(
                "// ============================================================================\n"
                "// From hardware/common/%s -- generated, do not edit here\n"
                "// ============================================================================\n\n"
                % path.name
            )
            parts.append(normalise(extract_module(read_text(path), module, path)))
            parts.append("\n")

    flat = "\n".join(part.rstrip("\n") for part in parts).rstrip("\n") + "\n"
    return flat.replace("\n", "\r\n")  # match the rest of hardware/shrikefi


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="do not write; exit 1 if a checked-in generated file is stale",
    )
    args = parser.parse_args()

    outputs = [
        (FLAT, build(), "flat vendor source set"),
        (TB_DST, build_tb(), "vendor-side testbench copy"),
    ]

    if args.check:
        stale = [path for path, text, _ in outputs
                 if (read_text(path) if path.exists() else "") != text]
        if stale:
            sys.exit(
                "error: stale generated file(s): %s\n"
                "       re-run hardware/shrikefi/gen_flat_source.py"
                % ", ".join(str(p.relative_to(REPO)) for p in stale)
            )
        for path, _, what in outputs:
            print("ok: %s is up to date (%s)" % (path.relative_to(REPO), what))
        return 0

    for path, text, what in outputs:
        path.parent.mkdir(parents=True, exist_ok=True)
        # newline='' so the CRLF we built above survives the write
        path.write_text(text, encoding="utf-8", newline="")
        print(
            "wrote %s (%d bytes, %s)"
            % (path.relative_to(REPO), len(text), what)
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
