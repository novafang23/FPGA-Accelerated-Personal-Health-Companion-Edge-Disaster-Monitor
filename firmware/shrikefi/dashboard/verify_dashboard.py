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
import urllib.parse
from pathlib import Path

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except AttributeError:
    pass

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
    } else if (new URLSearchParams(location.search).get('case') === 'pm_published') {
      /* The device publishes the corrected PM2.5 and the raw sensor value
         behind it, with a confidence code. Both must be shown: the raw number
         is what the sensor actually said, the published one is the estimate. */
      handleLine('[TELEMETRY] HR=72.0,SPO2=98.0,RMSSD=42.0,TEMP=25.0,HUM=50.0,PM25=25.0,RR=15.0,SQI=0.90,RRDEPTH=0.0,RRCONF=0.0,PI=3.0,HRVN=240,PMRAW=34.7,PMCONF=OK');
    } else if (new URLSearchParams(location.search).get('case') === 'pm_nodata') {
      /* No PM2.5 reading at all. This must not render as 0 ug/m3, which would
         be a claim of clean air that was never measured. */
      handleLine('[TELEMETRY] HR=72.0,SPO2=98.0,RMSSD=42.0,TEMP=25.0,HUM=50.0,PM25=-1.0,RR=15.0,SQI=0.90,RRDEPTH=0.0,RRCONF=0.0,PI=3.0,HRVN=240,PMRAW=-1.0,PMCONF=NODATA');
    } else if (new URLSearchParams(location.search).get('case') === 'pm_humid') {
      /* A humidity caveat has to be visible next to the number, not hidden. */
      handleLine('[TELEMETRY] HR=72.0,SPO2=98.0,RMSSD=42.0,TEMP=25.0,HUM=88.0,PM25=65.0,RR=15.0,SQI=0.90,RRDEPTH=0.0,RRCONF=0.0,PI=3.0,HRVN=240,PMRAW=85.0,PMCONF=HUMID');
    } else if (new URLSearchParams(location.search).get('case') === 'freezing') {
      /* 0 C is data; the old +5 C validity floor hid a genuine freezing hour. */
      handleLine('[TELEMETRY] HR=72.0,SPO2=98.0,RMSSD=42.0,TEMP=0.0,HUM=50.0,PM25=12.0,RR=15.0,SQI=0.90,RRDEPTH=0.0,RRCONF=0.0,PI=3.0,HRVN=240,PMRAW=17.0,PMCONF=OK');
    } else if (new URLSearchParams(location.search).get('case') === 'hrv_settle') {
      /* Contact present but RMSSD withheld: the device's beat window is 120/240
         beats, so the card must show progress rather than a bare dash. */
      V.contact = true; V.hr = 72; V.spo2 = 98; V.hrv = 0; V.hrvN = 10; V.sqi = 95;
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
      text: C.text, faint: C.faint, crimson: C.crimson, amber: C.amber,
      hr: txt('v-hr'), hrNote: txt('hr-note'), hrNumCol: col('v-hr'),
      spo2: txt('v-spo2'), spo2Note: txt('spo2-note'), spo2NumCol: col('v-spo2'),
      hrv: txt('v-hrv'), hrvNote: txt('hrv-note'), hrvNumCol: col('v-hrv'),
      mHrv: txt('m-hrv'),
      gHeat: txt('g-heat-val'), gPoll: txt('g-poll-val'), gFlood: txt('g-flood-val'),
      rr: txt('v-rr'), rrNumCol: col('v-rr'), rrNote: txt('rr-note'),
      pi: txt('m-pi'), idleRrNote: idleRrNote, hrSource: txt('hr-source'),
      psi: txt('psi-val'), aha: txt('aha-val'),
      deviceVerdict: txt('device-verdict'),
      histSub: txt('hist-sub'), histSpan: txt('hist-span'),
      histExport: txt('hist-export'),
      histExportDisabled: document.getElementById('hist-export').disabled,
      scopeSrc: txt('scope-src'), envStrip: txt('v-env'),
      temp: txt('v-temp'), hum: txt('v-hum'), pm: txt('v-pm'),
      pmRaw: txt('pm-raw'), pmLabel: txt('pm-label'),
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


PROBE_STATE = """
<script>
window.__err = null;
window.onerror = function (m) { window.__err = String(m); };
(async function () {
  try {
    const q = new URLSearchParams(location.search);
    const caseName = q.get('case');
    const txt = (id) => {
      const e = document.getElementById(id);
      return e ? e.textContent.trim() : '<missing>';
    };
    const vis = (id) => {
      const e = document.getElementById(id);
      return e ? getComputedStyle(e).display !== 'none' : false;
    };
    const snap = {
      mode: mode,
      gateHidden: document.getElementById('gate').classList.contains('hidden'),
      status: txt('status-text'),
      foot: txt('foot-note'),
      hr: txt('v-hr'), hrNote: txt('hr-note'),
      spo2: txt('v-spo2'), spo2Note: txt('spo2-note'),
      hrv: txt('v-hrv'), hrvNote: txt('hrv-note'),
      rr: txt('v-rr'), rrNote: txt('rr-note'),
      sqi: txt('v-sqi'), sqiPill: txt('sqi-pill'),
      temp: txt('v-temp'), hum: txt('v-hum'), pm: txt('v-pm'), pmLabel: txt('pm-label'),
      psi: txt('psi-val'), aha: txt('aha-val'), hrSource: txt('hr-source'),
      scopeSrc: txt('scope-src'), profile: profile,
      activeProfile: (function () {
        const a = document.querySelector('#profile-buttons button.active');
        return a ? a.textContent : null;
      })(),
      histSpan: txt('hist-span'), histSub: txt('hist-sub'),
      histMeanHr: txt('h-mean-hr'), histPeakN2: txt('h-peak-news2'),
      histPeakPm: txt('h-peak-pm25'), histSampleCount: txt('h-sample-count'),
      btnSim: vis('btn-sim'), profiles: vis('profile-buttons'), connect: vis('btn-connect'),
    };
    if (caseName === 'idle') {
      /* The idle contract includes text painted by the canvas, not only DOM
         text. Capture fillText calls while redrawing the idle scope. */
      const oldFill = CanvasRenderingContext2D.prototype.fillText;
      const seen = [];
      CanvasRenderingContext2D.prototype.fillText = function (t) {
        seen.push(String(t));
        return oldFill.apply(this, arguments);
      };
      try { PPG.draw(); } catch (e) { snap.canvasError = String(e); }
      finally { CanvasRenderingContext2D.prototype.fillText = oldFill; }
      snap.canvasText = seen.join(' | ');
    }
    if (caseName === 'tooltip') {
      /* An environmental-only hour has valid=true but null clinical values.
         Hovering it must show dashes, not throw on null.toFixed(). */
      mode = 'live';
      HIST.reset();
      HIST.setPoint(0, { hr: null, spo2: null, rmssd: null, pm25: null, news2: null, valid: true });
      HIST.setPoint(1, { hr: 72, spo2: 98, rmssd: 42, pm25: 0, news2: 0, valid: true });
      const cv = document.getElementById('hist-canvas');
      cv.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowLeft' }));
      const tt = document.getElementById('hist-tooltip');
      snap.tooltipNull = { display: tt.style.display, text: tt.textContent };
      cv.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowRight' }));
      snap.tooltipZeroPm = { display: tt.style.display, text: tt.textContent };
      HIST.reset();
      HIST.setPoint(2, { hr: null, spo2: null, rmssd: null, pm25: 12, news2: null, valid: true });
      snap.envSummary = {
        meanHr: txt('h-mean-hr'), peakN2: txt('h-peak-news2'),
        peakPm: txt('h-peak-pm25'), count: txt('h-sample-count'),
      };
    }
    if (caseName === 'serial') {
      /* Split a serial frame across chunks and verify readLoop reassembles
         CRLF lines, skips blank lines and holds a partial line. */
      const calls = [];
      const originalHandleLine = handleLine;
      handleLine = function (line) { calls.push(line); };
      const enc = new TextEncoder();
      const chunks = [
        '[TELEMETRY] HR=72.0,SPO2=98.0\\r\\n[TRIAGE] NEWS2=',
        '0\\r\\n\\n[HISTORY] H0,PM25=12.0\\npart',
        'ial'
      ];
      const fake = {
        read: async function () {
          if (!chunks.length) { keepReading = false; return { value: undefined, done: true }; }
          return { value: enc.encode(chunks.shift()), done: false };
        },
        releaseLock: function () {}
      };
      port = { readable: { getReader: function () { return fake; } } };
      keepReading = true;
      mode = 'live';
      await readLoop();
      handleLine = originalHandleLine;
      snap.serialCalls = calls;
    }
    const pre = document.createElement('pre');
    pre.id = 'probe';
    pre.textContent = JSON.stringify(Object.assign({ error: window.__err }, snap));
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


def render(case, extra=None, probe=PROBE):
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
    probe_script = re.search(r"<script>(.*?)</script>", probe, re.S)
    probe_hash = base64.b64encode(hashlib.sha256(probe_script.group(1).encode("utf-8")).digest()).decode("ascii")
    html, n = re.subn(r"(script-src\s+[^;]+)", r"\1 'sha256-%s'" % probe_hash, html, count=1)
    if n != 1:
        raise RuntimeError("CSP script-src policy missing")
    html = html.replace("</body>", probe + "\n</body>", 1)
    with tempfile.NamedTemporaryFile("w", suffix=".html", delete=False,
                                     encoding="utf-8", dir=str(SRC.parent)) as fh:
        fh.write(html)
        tmp = Path(fh.name)
    params = {"case": case}
    if extra:
        params.update(extra)
    try:
        proc = subprocess.run(
            [CHROME, "--headless=new", "--disable-gpu", "--no-sandbox",
             "--virtual-time-budget=3000", "--dump-dom",
             tmp.as_uri() + "?" + urllib.parse.urlencode(params)],
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
              "psi", "aha", "hrSource",
              "hrNumCol", "spo2NumCol", "hrvNumCol", "rrNumCol", "sqiNumCol"):
        print("  %-12s %s" % (k, r.get(k)))
    print("  %-12s %s" % ("n2-grid", "  ".join(r.get("n2", []))))

    for key, want in expect.items():
        got = r.get(key)
        if got != want:
            bad.append("  %s: expected %r, got %r" % (key, want, got))
    return bad


failures = []
for case in ("zero", "healthy", "hrv_settle", "device_pts", "ai_declined", "ai_scored", "nofinger", "sensor_missing", "ambient", "freezing", "pi", "stale",
             "pm_published", "pm_nodata", "pm_humid"):
    try:
        r = render(case)
    except (RuntimeError, json.JSONDecodeError) as e:
        failures.append("  %s: %s" % (case, e))
        print("=" * 66)
        print("CASE: %s -> could not render: %s" % (case, e))
        continue

    neutral = rgb(r["faint"]) if "faint" in r else None
    alarm = {rgb(r["crimson"]), rgb(r["amber"])} if "crimson" in r else set()

    if case == "zero":
        expect = {
            "hr": "\u2014", "spo2": "\u2014", "hrv": "\u2014", "rr": "\u2014", "sqi": "\u2014",
            "hrNote": "acquiring", "spo2Note": "calibrating",
            "hrvNote": "needs 30 beats \u00b7 20 s", "sqiPill": "waiting for contact",
            "psi": "--", "aha": "--", "hrSource": "awaiting IBI",
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
            "psi": "0.0/10", "aha": "0.00/1.0",
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
        expect = {"hr": "\u2014", "temp": "25.0", "psi": "--", "aha": "--",
                  "hrSource": "awaiting IBI"}
    elif case == "sensor_missing":
        expect = {"hr": "\u2014", "temp": "\u2014", "psi": "--", "aha": "--",
                  "hrSource": "awaiting IBI"}
    elif case == "ambient":
        expect = {
            "hr": "\u2014", "temp": "29.7", "hum": "63", "pm": "86.6",
            "psi": "--", "aha": "--", "hrSource": "awaiting IBI",
            "scopeSrc": "AMBIENT",
            "envStrip": "Ta 29.7 \u00b0C \u00b7 RH 63 % \u00b7 PM2.5 86.6 \u00b5g/m\u00b3",
        }
    elif case == "freezing":
        expect = {
            "temp": "0.0",
            "envStrip": "Ta 0.0 \u00b0C \u00b7 RH 50 % \u00b7 PM2.5 12.0 \u00b5g/m\u00b3",
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
            "hrv": "10", "mHrv": "10/30",
            "hrvNote": "settling \u00b7 10 of 30 beats",
            "psi": "0.0/10", "aha": "--",
        }
    elif case == "ai_declined":
        expect = {"gHeat": "--", "gPoll": "--", "gFlood": "--"}
    elif case == "ai_scored":
        expect = {"gHeat": "25.0%", "gPoll": "10.0%", "gFlood": "5.0%"}
    elif case == "pm_published":
        # The published (corrected) value is the headline; the raw sensor value
        # and the confidence code sit under it.
        expect = {
            "pm": "25.0",
            "pmRaw": "raw 34.7",
            "pmLabel": "Moderate",
        }
    elif case == "pm_humid":
        # A humidity caveat must be visible, and the band must come from the
        # EPA 2024 breakpoints the device uses.
        expect = {
            "pm": "65.0",
            "pmRaw": "raw 85.0 \u00b7 humidity-corrected, RH high",
            "pmLabel": "Unhealthy",
        }
    elif case == "pm_nodata":
        # No reading is not a reading of zero.
        expect = {
            "pm": "\u2014",
            "pmRaw": "no reading",
            "pmLabel": "Not assessed",
        }
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

# ---------------------------------------------------------------------------
# URL/state coverage: idle, ?sim=1, ?full=1, ?profile=, canvas text, the
# nullable history tooltip and the chunked serial reader.
# ---------------------------------------------------------------------------
state_cases = [
    ("idle", None),
    ("sim", {"sim": "1"}),
    ("full", {"full": "1"}),
    ("profile", {"sim": "1", "profile": "Cardiopulmonary ICU Emergency"}),
    ("tooltip", None),
    ("serial", None),
]
for case, extra in state_cases:
    try:
        r = render(case, extra=extra, probe=PROBE_STATE)
    except (RuntimeError, json.JSONDecodeError) as e:
        failures.append("  %s: %s" % (case, e))
        print("=" * 66)
        print("CASE: %s -> could not render: %s" % (case, e))
        continue

    if case == "idle":
        expect = {
            "mode": "idle",
            "gateHidden": False,
            "status": "NOT CONNECTED \u00b7 no data",
            "foot": "no data \u2014 connect a device or start a simulation",
            "hr": "\u2014", "spo2": "\u2014", "hrv": "\u2014", "rr": "\u2014",
            "sqi": "\u2014", "hrNote": "", "spo2Note": "", "hrvNote": "",
            "sqiPill": "waiting for signal",
            "temp": "\u2014", "hum": "\u2014", "pm": "\u2014", "pmLabel": "Not assessed",
            "psi": "\u2014", "aha": "\u2014", "hrSource": "--",
            "rrNote": "PPG-derived \u00b7 RSA modulation",
            "scopeSrc": "NONE",
            "histSpan": "NO DATA",
            "histSub": "No history yet \u2014 connect a device or start a simulation",
            "histSampleCount": "0/24",
            "btnSim": True, "profiles": False, "connect": True,
        }
    elif case == "sim":
        expect = {
            "mode": "sim",
            "gateHidden": True,
            "status": "SIMULATION \u00b7 profile data",
            "foot": "simulation mode \u2014 profile data, no device",
            "hrSource": "profile",
            "scopeSrc": "SIM PPG",
            "profile": "Normal Baseline",
            "activeProfile": "Normal Baseline",
            "histSpan": "SYNTHETIC",
            "histSub": "Synthetic 24-point demo history \u2014 not device data",
            "histSampleCount": "24/24",
            "btnSim": True, "profiles": True, "connect": True,
        }
    elif case == "full":
        expect = {
            "mode": "sim",
            "gateHidden": True,
            "status": "SIMULATION \u00b7 profile data",
            "foot": "simulation mode \u2014 profile data, no device",
            "scopeSrc": "SIM PPG",
            "btnSim": False, "profiles": False, "connect": True,
        }
    elif case == "profile":
        expect = {
            "mode": "sim",
            "gateHidden": True,
            "status": "SIMULATION \u00b7 profile data",
            "profile": "Cardiopulmonary ICU Emergency",
            "activeProfile": "Cardiopulmonary ICU Emergency",
            "scopeSrc": "SIM PPG",
        }
    else:
        expect = {}

    bad = check(case, r, expect)

    if case == "idle":
        canvas = r.get("canvasText", "")
        if "NO SIGNAL" not in canvas:
            bad.append("  canvas: idle scope does not paint NO SIGNAL")
        if "connect a device" not in canvas:
            bad.append("  canvas: idle guidance text missing")
        if r.get("canvasError"):
            bad.append("  canvas: PPG.draw() threw: %s" % r["canvasError"])
    elif case in ("sim", "full", "profile"):
        if r.get("hr") in (None, "--", "<missing>"):
            bad.append("  sim: HR did not render")
        if r.get("pm") in (None, "--", "<missing>"):
            bad.append("  sim: PM2.5 did not render")
        if r.get("temp") in (None, "--", "<missing>"):
            bad.append("  sim: temperature did not render")
    elif case == "tooltip":
        ttn = r.get("tooltipNull") or {}
        text = ttn.get("text", "")
        if ttn.get("display") != "block":
            bad.append("  tooltip: null-hour tooltip is not visible")
        for needle in ("HR: --", "SpO2: --", "RMSSD: --", "NEWS2: --", "PM2.5: --"):
            if needle not in text:
                bad.append("  tooltip: expected %r, got %r" % (needle, text))
        if "null" in text.lower() or "NaN" in text:
            bad.append("  tooltip: leaked a null/NaN token: %r" % text)
        tz = r.get("tooltipZeroPm") or {}
        if "PM2.5: 0" not in tz.get("text", ""):
            bad.append("  tooltip: a real PM2.5=0 was not rendered as zero")
        env = r.get("envSummary") or {}
        for key, want in (("meanHr", "--"), ("peakN2", "--"),
                          ("peakPm", "12 \u00b5g/m\u00b3"), ("count", "1/24")):
            if env.get(key) != want:
                bad.append("  history summary %s: expected %r, got %r"
                           % (key, want, env.get(key)))
    elif case == "serial":
        want_calls = [
            "[TELEMETRY] HR=72.0,SPO2=98.0",
            "[TRIAGE] NEWS2=0",
            "[HISTORY] H0,PM25=12.0",
        ]
        calls = r.get("serialCalls")
        if calls != want_calls:
            bad.append("  serial: expected %r, got %r" % (want_calls, calls))
        if any("\r" in c for c in (calls or [])):
            bad.append("  serial: a trailing CR survived line reassembly")

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
print("PASSED - absent readings, live data, idle/sim/full, serial framing and tooltip nulls verified")
