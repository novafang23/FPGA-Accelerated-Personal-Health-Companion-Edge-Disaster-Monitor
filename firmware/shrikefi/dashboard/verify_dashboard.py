"""Headless DOM check for valor_dashboard.html.

The dashboard is one self-contained file with no build step, and a reviewer
reading the source cannot see what it actually renders. This renders the real
file in Chrome with mode forced to 'live' and reads the resulting DOM for several
states:

  zero    - every vital absent (0), i.e. connected but still acquiring
  healthy - HR 72, SpO2 98, RMSSD 42, RR 15
  pi      - a real PI/RRDEPTH/RRCONF telemetry line is parsed and displayed

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
import base64
import hashlib
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
    const idleRrNote = document.getElementById('rr-note').textContent.trim();
    mode = 'live';
    if (new URLSearchParams(location.search).get('case') === 'zero') {
      V.hr = 0; V.spo2 = 0; V.hrv = 0; V.rr = 0; V.sqi = 0; V.pi = 0;
      V.news2 = 0; V.level = 0; V.flags = 0;
    } else if (new URLSearchParams(location.search).get('case') === 'stale') {
      V.hr = 72; V.spo2 = 98; V.hrv = 42; V.rr = 15; V.sqi = 95;
      lastTelemetryAt = performance.now() - 4001; frame(performance.now());
    } else if (new URLSearchParams(location.search).get('case') === 'nofinger') {
      V.hr = 72;
      handleLine('[TELEMETRY] NO_FINGER,TEMP=25.0,HUM=50.0,PM25=12.0');
    } else if (new URLSearchParams(location.search).get('case') === 'sensor_missing') {
      handleLine('[TELEMETRY] NO_FINGER,TEMP=-100.0,HUM=-1.0,PM25=12.0');
    } else if (new URLSearchParams(location.search).get('case') === 'ambient') {
      handleLine('[TELEMETRY] HR=0.0,SPO2=0.0,RMSSD=0.0,TEMP=29.7,HUM=63.3,PM25=86.6,RR=0.0,SQI=0.00,RRDEPTH=0.0,RRCONF=0.00,PI=0.00');
    } else if (new URLSearchParams(location.search).get('case') === 'pi') {
      handleLine('[TELEMETRY] HR=72.0,SPO2=98.0,RMSSD=42.0,TEMP=25.0,HUM=50.0,PM25=12.0,RR=15.0,SQI=0.90,RRDEPTH=42.5,RRCONF=0.80,PI=3.21');
    } else if (new URLSearchParams(location.search).get('case') === 'hrv_settle') {
      /* Contact present but RMSSD withheld: the device's beat window is 120/240
         beats, so the card must show progress rather than a bare dash. */
      V.contact = true; V.hr = 72; V.spo2 = 98; V.hrv = 0; V.hrvN = 120; V.sqi = 95;
      V.news2 = 0; V.level = 0; V.flags = 0; V.pi = 1.2;
    } else if (new URLSearchParams(location.search).get('case') === 'device_pts') {
      V.hr = 72; V.spo2 = 98; V.hrv = 42; V.rr = 15; V.sqi = 95; V.contact = true;
      handleLine('[TRIAGE] NEWS2=6,LEVEL=2,FLAGS=0x00,RISK=HIGH,NNHEAT=0.0,NNPOLL=0.0,NNFLOOD=0.0,RHEAT=NORMAL,RPOLL=NORMAL,RFLOOD=NORMAL,PSI=0,AHA=0,CYCLONE=UNKNOWN,PTREND=0,HRPTS=1,SPO2PTS=2,RRPTS=3');
    } else if (new URLSearchParams(location.search).get('case') === 'ai_declined') {
      /* The device reports that the INT8 model declined (AIVALID=0) and
         streams zeros. A bare 0.000 is indistinguishable from "the model ran
         and found no risk", so the gauges must render the absence, not 0 %. */
      V.contact = true; V.hr = 72; V.spo2 = 98; V.hrv = 45; V.sqi = 90;
      handleLine('[TRIAGE] NEWS2=0,LEVEL=1,FLAGS=0x00,RISK=MODERATE,NNHEAT=0.000,NNPOLL=0.000,NNFLOOD=0.000,RHEAT=UNKNOWN,RPOLL=UNKNOWN,RFLOOD=UNKNOWN,PSI=0.00,AHA=0.00,CYCLONE=UNKNOWN,PTREND=0.00,HRPTS=0,SPO2PTS=0,RRPTS=0,AIVALID=0');
    } else if (new URLSearchParams(location.search).get('case') === 'ai_scored') {
      /* Contrast: AIVALID=1 means the model really ran, so the number must
         render rather than being suppressed. */
      V.contact = true; V.hr = 72; V.spo2 = 98; V.hrv = 45; V.sqi = 90;
      handleLine('[TRIAGE] NEWS2=4,LEVEL=2,FLAGS=0x00,RISK=HIGH,NNHEAT=0.250,NNPOLL=0.100,NNFLOOD=0.050,RHEAT=MODERATE,RPOLL=NORMAL,RFLOOD=NORMAL,PSI=0.00,AHA=0.00,CYCLONE=UNKNOWN,PTREND=0.00,HRPTS=0,SPO2PTS=0,RRPTS=0,AIVALID=1');
    } else {
      V.hr = 72; V.spo2 = 98; V.hrv = 42; V.rr = 15; V.sqi = 95; V.pi = 1.2;
      V.news2 = 0; V.risk = 'NORMAL'; V.level = 0; V.flags = 0; V.contact = true;
      V.n2hrPts = 0; V.n2spo2Pts = 0; V.n2rrPts = 0;
      ppgSeen = true;
      handleLine('[HISTORY] H0,HR=72.0,SPO2=98.0,RMSSD=42.0,NEWS2=0,PM25=12.0');
    }
    if (new URLSearchParams(location.search).get('case') !== 'stale') render();
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
      mHrv: txt('m-hrv'),
      gHeat: txt('g-heat-val'), gPoll: txt('g-poll-val'), gFlood: txt('g-flood-val'),
      rr: txt('v-rr'), rrNumCol: col('v-rr'), rrNote: txt('rr-note'),
      pi: txt('m-pi'), idleRrNote: idleRrNote, hrSource: txt('hr-source'),
      deviceVerdict: txt('device-verdict'),
      histSub: txt('hist-sub'), histSpan: txt('hist-span'),
      histExport: txt('hist-export'),
      histExportDisabled: document.getElementById('hist-export').disabled,
      scopeSrc: txt('scope-src'), envStrip: txt('v-env'),
      temp: txt('v-temp'), hum: txt('v-hum'), pm: txt('v-pm'),
      status: txt('status-text').indexOf('NO DATA') === 0,
      sqi: txt('v-sqi'), sqiPill: txt('sqi-pill'), sqiNumCol: col('v-sqi'),
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
    script = re.search(r"<script>(.*?)</script>", html, re.S)
    if not script:
        raise RuntimeError("dashboard script block missing")
    original_hash = base64.b64encode(hashlib.sha256(script.group(1).encode("utf-8")).digest()).decode("ascii")
    if "'sha256-%s'" % original_hash not in html:
        raise RuntimeError("CSP script hash does not match the dashboard script")
    probe_script = re.search(r"<script>(.*?)</script>", PROBE, re.S)
    probe_hash = base64.b64encode(hashlib.sha256(probe_script.group(1).encode("utf-8")).digest()).decode("ascii")
    html, n = re.subn(r"(script-src\s+[^;]+)", r"\1 'sha256-%s'" % probe_hash, html, count=1)
    if n != 1:
        raise RuntimeError("CSP script-src policy missing")
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
    for k in ("hr", "hrNote", "spo2", "spo2Note", "hrv", "hrvNote", "rr", "sqi", "sqiPill",
              "hrNumCol", "spo2NumCol", "hrvNumCol", "rrNumCol", "sqiNumCol"):
        print("  %-12s %s" % (k, r.get(k)))
    print("  %-12s %s" % ("n2-grid", "  ".join(r.get("n2", []))))

    for key, want in expect.items():
        got = r.get(key)
        if got != want:
            bad.append("  %s: expected %r, got %r" % (key, want, got))
    return bad


failures = []
for case in ("zero", "healthy", "hrv_settle", "device_pts", "ai_declined", "ai_scored", "nofinger", "sensor_missing", "ambient", "pi", "stale"):
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
            "hr": "--", "spo2": "--", "hrv": "--", "rr": "--", "sqi": "--",
            "hrNote": "acquiring", "spo2Note": "calibrating",
            "hrvNote": "needs 240 beats \u00b7 120 s", "sqiPill": "waiting for contact",
            "histMeanHr": "--", "histSampleCount": "0/24",
            "histSub": "No history yet \u2014 connect a device or start a simulation",
            "histSpan": "NO DATA", "histExportDisabled": True,
            "scopeSrc": "AMBIENT",
            "n2": ["RR=--", "SpO2=--", "HR=--", "SBP=n/a", "Temp=n/a", "Consc=n/a"],
        }
    elif case == "healthy":
        expect = {
            "hr": "72", "spo2": "98.0", "hrv": "42.0", "rr": "15", "sqi": "95",
            "hrNote": "normal rate", "spo2Note": "normal",
            "hrSource": "device IBI", "deviceVerdict": "Device verdict: NORMAL",
            "scopeSrc": "LIVE IR",
            "idleRrNote": "PPG-derived \u00b7 RSA modulation",
            "histSub": "Device hourly summaries \u00b7 device hours, restored after reboot \u00b7 1/24 populated",
            "histSpan": "DEVICE HOURS", "histExport": "EXPORT CSV",
            "histExportDisabled": False,
            "hrvNote": "Adequate reserve", "sqiPill": "clean signal",
            "histMeanHr": "72 bpm", "histSampleCount": "1/24",
            "n2": ["RR=+0", "SpO2=+0", "HR=+0", "SBP=n/a", "Temp=n/a", "Consc=n/a"],
        }
    elif case == "nofinger":
        expect = {"hr": "--", "temp": "25.0"}
    elif case == "sensor_missing":
        expect = {"hr": "--", "temp": "--"}
    elif case == "ambient":
        expect = {
            "hr": "--", "temp": "29.7", "hum": "63", "pm": "86.6",
            "scopeSrc": "AMBIENT",
            "envStrip": "Ta 29.7 \u00b0C \u00b7 RH 63 % \u00b7 PM2.5 86.6 \u00b5g/m\u00b3",
        }
    elif case == "pi":
        expect = {
            "pi": "3.21 %",
            "rrNote": "43 ms RSA depth \u00b7 conf 80%",
        }
    elif case == "device_pts":
        expect = {
            "n2": ["RR=+3", "SpO2=+2", "HR=+1", "SBP=n/a", "Temp=n/a", "Consc=n/a"],
        }
    elif case == "hrv_settle":
        expect = {
            "hrv": "120", "mHrv": "120/240",
            "hrvNote": "settling \u00b7 120 of 240 beats",
        }
    elif case == "ai_declined":
        expect = {"gHeat": "--", "gPoll": "--", "gFlood": "--"}
    elif case == "ai_scored":
        expect = {"gHeat": "25.0%", "gPoll": "10.0%", "gFlood": "5.0%"}
    else:
        expect = {
            "hr": chr(0x2014),
            "status": True,
        }

    bad = check(case, r, expect)

    # An absent reading must not be painted in an alarm colour.
    if case == "zero" and neutral:
        for k in ("hrNumCol", "spo2NumCol", "hrvNumCol", "rrNumCol", "sqiNumCol"):
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
