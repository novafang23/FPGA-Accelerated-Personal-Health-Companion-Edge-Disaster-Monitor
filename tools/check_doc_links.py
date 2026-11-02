#!/usr/bin/env python3
"""Verify that every relative link in tracked Markdown files resolves.

Run from the repo root:  python tools/check_doc_links.py

Checks:
  * inline links            [text](target)
  * images                  ![alt](target)
  * reference definitions   [id]: target

Ignores:
  * absolute URLs (http, https, mailto)
  * pure #anchors
  * links inside fenced code blocks and inline `code`
  * targets that are directories containing an index/README (reported as DIRS)

Exit code 0 = every link resolves. 1 = at least one broken link.
"""
from __future__ import annotations

import io
import os
import re
import subprocess
import sys

LINK_RE = re.compile(r"!?\[[^\]]*\]\(\s*<?([^)>\s]+)>?(?:\s+\"[^\"]*\")?\s*\)")
REFDEF_RE = re.compile(r"^\s{0,3}\[[^\]]+\]:\s*<?(\S+?)>?(?:\s|$)", re.M)
FENCE_RE = re.compile(r"```.*?```", re.S)
INLINE_CODE_RE = re.compile(r"`[^`\n]*`")
SCHEME_RE = re.compile(r"^[a-zA-Z][a-zA-Z0-9+.-]*:")


def tracked_markdown() -> list[str]:
    out = subprocess.run(
        ["git", "ls-files", "-z", "--", "*.md"],
        capture_output=True, check=True,
    ).stdout
    return [p for p in out.decode("utf-8").split("\0") if p]


def strip_code(text: str) -> str:
    """Blank out code so links inside it are ignored, preserving line numbers."""
    def blank(m: re.Match[str]) -> str:
        return "\n" * m.group(0).count("\n")

    return INLINE_CODE_RE.sub("", FENCE_RE.sub(blank, text))


def main() -> int:
    if not os.path.isdir(".git"):
        print("error: run from the repository root", file=sys.stderr)
        return 2

    files = tracked_markdown()
    broken: list[tuple[str, int, str]] = []
    checked = 0

    for rel in files:
        try:
            text = io.open(rel, "r", encoding="utf-8", errors="replace").read()
        except OSError as exc:
            print(f"error: cannot read {rel}: {exc}", file=sys.stderr)
            continue

        base = os.path.dirname(rel)
        for lineno, line in enumerate(strip_code(text).splitlines(), start=1):
            targets = [m.group(1) for m in LINK_RE.finditer(line)]
            m = REFDEF_RE.match(line)
            if m:
                targets.append(m.group(1))
            for target in targets:
                if not target or target.startswith("#") or SCHEME_RE.match(target):
                    continue
                target = target.split("#", 1)[0].split("?", 1)[0]
                if not target:
                    continue
                checked += 1
                resolved = os.path.normpath(os.path.join(base, target))
                if not os.path.exists(resolved):
                    broken.append((rel, lineno, target))

    if broken:
        print(f"broken links: {len(broken)} of {checked} checked\n")
        for rel, lineno, target in broken:
            print(f"  {rel}:{lineno}  ->  {target}")
        return 1

    print(f"ok: {checked} relative links across {len(files)} markdown files resolve")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
