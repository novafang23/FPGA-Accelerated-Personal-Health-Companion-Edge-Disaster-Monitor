"""Headless DOM check for valor_dashboard.html.

The dashboard is one self-contained file with no build step, and a reviewer
reading the source cannot see what it actually renders. This renders the real
file in Chrome with mode forced to 'live' and reads the resulting DOM, twice:

  zero    - every vital absent (0), i.e. connected but still acquiring
  healthy - HR 72, SpO2 98, RMSSD 42, RR 15

The rule being enforced is that an absent reading must render as absent - a
dash, a neutral colour and a note saying the reading is still coming. A raw 0
falls into the bottom bucket of every clinical threshold chain, so without that
rule the first seconds of every contact show a crimson card reading
"bradycardia" / "hypoxia" / "Low vagal reserve" next to a dash, and the NEWS2
tiles show +3 apiece while the device's own score is still 0.

The file is never modified: a temporary copy with an injected probe script is
rendered instead.

Usage:  python verify_dashboard.py
Exit code is non-zero if a case fails to render or any assertion fails.
"""
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SRC = Path(__file__).resolve().with_name("valor_dashboard.html")
CHROME = os.environ.get("CHROME", r"C:\Program Files\Google\Chrome\Application\chrome.exe")

PROBE = """
<script>
window.__err = null;
window.onerror = function (m) { window.__err = String(m); };
(function () {
  try {
    mode = 'live';
    if (new URLSearchParams(location.search).get('case') === 'zero') {
      V.hr = 0; V.spo2 = 0; V.hrv = 0; V.rr = 0; V.sqi = 0; V.pi = 0;
      V.news2 = 0; V.level = 0; V.flags = 0;
    } else {
      V.hr = 72; V.spo2 = 98; V.hrv = 42; V.rr = 15; V.sqi = 0.95; V.pi = 1.2; V.news2 = 0;
    }
    render();
    const txt = (id) => {
      const e = document.getElementById(id);
      return e ? e.textContent.trim() : '<missing>';
    };
    const col = (id) => {
      const e = document.getElementById(id);
      return e ? getComputedStyle(e).color : '<missing>';
    };
    const grid = document.getElementById('n2-grid');
    const out = {
      error: window.__err,
      text: C.text, crimson: C.crimson, amber: C.amber,
      hr: txt('v-hr'), hrNote: txt('hr-note'), hrNumCol: col('v-hr'),
      spo2: txt('v-spo2'), spo2Note: txt('spo2-note'), spo2NumCol: col('v-spo2'),
      hrv: txt('v-hrv'), hrvNote: txt('hrv-note'), hrvNumCol: col('v-hrv'),
      rr: txt('v-rr'), rrNumCol: col('v-rr'),
      histMeanHr: txt('h-mean-hr'),
      histSampleCount: txt('h-sample-count'),
      n2: grid ? Array.from(grid.children).map((c) =>
        c.querySelector('.k').textContent + '=' + c.querySelector('.v').textContent
      ) : ['<missing>'],
    };
    const pre = document.createElement('pre');
    pre.id = 'probe';
    pre.textContent = JSON.stringify(out);
    document.body.appendChild(pre);
  } catch (e) {
    const pre = document.createElement('pre');
    pre.id = 'probe';
    pre.textContent = JSON.stringify({ thrown: String(e) });
    document.body.appendChild(pre);
  }
})();
</script>
"""


def render(case):
    """Return the probe dict for one case, or raise RuntimeError."""
    html = SRC.read_text(encoding="utf-8")
    if "</body>" not in html:
        raise RuntimeError("no </body> in %s" % SRC)
    html = html.replace("</body>", PROBE + "\n</body>", 1)
    with tempfile.NamedTemporaryFile("w", suffix=".html", delete=False,
                                     encoding="utf-8", dir=str(SRC.parent)) as fh:
        fh.write(html)
        tmp = Path(fh.name)
    try:
        proc = subprocess.run(
            [CHROME, "--headless=new", "--disable-gpu", "--no-sandbox",
             "--virtual-time-budget=3000", "--dump-dom",
             tmp.as_uri() + "?case=" + case],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=120,
        )
        m = re.search(r'<pre id="probe">(.*?)</pre>', proc.stdout, re.S)
        if not m:
            raise RuntimeError(
                "probe element missing (rc=%s, dom=%d bytes). Is Chrome at %s?"
                % (proc.returncode, len(proc.stdout), CHROME))
        raw = m.group(1)
        for a, b in (("&quot;", '"'), ("&amp;", "&"), ("&lt;", "<"), ("&gt;", ">")):
            raw = raw.replace(a, b)
        return json.loads(raw)
    finally:
        tmp.unlink(missing_ok=True)


def rgb(hexcol):
    """'#2D3F5C' -> 'rgb(45, 63, 92)', the form getComputedStyle returns."""
    h = hexcol.lstrip("#")
    return "rgb(%d, %d, %d)" % tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def check(case, r, expect):
    """Print the render and return a list of failure strings."""
    print("=" * 66)
    print("CASE: %s" % case)
    print("=" * 66)
    if "thrown" in r:
        return ["  JS threw: %s" % r["thrown"]]
    bad = []
    if r.get("error"):
        bad.append("  JS error: %s" % r["error"])
    for k in ("hr", "hrNote", "spo2", "spo2Note", "hrv", "hrvNote", "rr", "hrNumCol",
              "spo2NumCol", "hrvNumCol", "rrNumCol"):
        print("  %-12s %s" % (k, r.get(k)))
    print("  %-12s %s" % ("n2-grid", "  ".join(r.get("n2", []))))

    for key, want in expect.items():
        got = r.get(key)
        if got != want:
            bad.append("  %s: expected %r, got %r" % (key, want, got))
    return bad


failures = []
for case in ("zero", "healthy"):
    try:
        r = render(case)
    except (RuntimeError, json.JSONDecodeError) as e:
        failures.append("  %s: %s" % (case, e))
        print("=" * 66)
        print("CASE: %s -> could not render: %s" % (case, e))
        continue

    neutral = rgb(r["text"]) if "text" in r else None
    alarm = {rgb(r["crimson"]), rgb(r["amber"])} if "crimson" in r else set()

    if case == "zero":
        expect = {
            "hr": "--", "spo2": "--", "hrv": "--", "rr": "--",
            "hrNote": "acquiring", "spo2Note": "calibrating",
            "hrvNote": "needs 10 beats",
            "histMeanHr": "--", "histSampleCount": "0/24",
            "n2": ["RR=--", "SpO2=--", "HR=--", "SBP=n/a", "Temp=n/a", "Consc=n/a"],
        }
    else:
        expect = {
            "hr": "72", "spo2": "98.0", "hrv": "42.0", "rr": "15",
            "hrNote": "normal sinus rhythm", "spo2Note": "normal",
            "hrvNote": "Adequate reserve",
            "histMeanHr": "72 bpm", "histSampleCount": "1/24",
            "n2": ["RR=+0", "SpO2=+0", "HR=+0", "SBP=n/a", "Temp=n/a", "Consc=n/a"],
        }

    bad = check(case, r, expect)

    # An absent reading must not be painted in an alarm colour.
    if case == "zero" and neutral:
        for k in ("hrNumCol", "spo2NumCol", "hrvNumCol", "rrNumCol"):
            if r.get(k) != neutral:
                bad.append("  %s: absent reading painted %s, expected neutral %s"
                           % (k, r.get(k), neutral))
            if r.get(k) in alarm:
                bad.append("  %s: absent reading painted in an alarm colour" % k)

    if bad:
        print("\n".join(bad))
        failures += ["%s: %s" % (case, b.strip()) for b in bad]
    else:
        print("  -> OK")

print("=" * 66)
if failures:
    print("FAILED (%d)" % len(failures))
    for f in failures:
        print("  " + f)
    sys.exit(1)
print("PASSED - absent readings render as absent, live readings unchanged")
