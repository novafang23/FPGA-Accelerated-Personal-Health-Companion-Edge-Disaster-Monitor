"""Render the firmware-embedded SoftAP page against a local status stub.

This needs Chrome/Edge (or CHROME=path) but no ESP32, WiFi, or internet. It
checks the actual PAGE_HTML C strings, CSP hash, and live/no-finger/link-loss
DOM behavior, including the phone SOS request handler.
"""
import ast
import base64
import hashlib
import html
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "web_status.c"
CHROME = os.environ.get("CHROME", r"C:\Program Files\Google\Chrome\Application\chrome.exe")


def embedded_page():
    source = SRC.read_text(encoding="utf-8")
    start = source.index("static const char PAGE_HTML[] =")
    end = source.index(";\n\n/* ------------------------------------------------------------------------- */", start)
    literals = re.findall(r'"(?:\\.|[^"\\])*"', source[start:end])
    page = "".join(ast.literal_eval(item) for item in literals)
    if not page.startswith("<!DOCTYPE html>"):
        raise RuntimeError("could not extract the embedded page")
    return source, page


def sha256_source(text):
    return base64.b64encode(hashlib.sha256(text.encode("utf-8")).digest()).decode("ascii")


def build_fixture_page(page, data, *, lose_link=False, click_sos=False):
    script = re.search(r"<script>(.*?)</script>", page, re.S).group(1)
    payload = json.dumps(data, separators=(",", ":"))
    fail_after_first = "true" if lose_link else "false"
    shim = (
        "window.__calls=[];window.__polls=0;window.fetch=function(u,o){"
        "window.__calls.push([u,o&&o.method||'GET']);"
        "if(u==='/status.json'&&" + fail_after_first + "&&++window.__polls>1)"
        "return Promise.reject(new Error('offline'));"
        "return Promise.resolve({ok:true,json:function(){return Promise.resolve(" + payload + ")}})};"
    )
    test_script = script.replace("var D='--';var locFilled=false;", shim + "var D='--';var locFilled=false;")
    if test_script == script:
        raise RuntimeError("status page fetch shim anchor changed")
    page = page.replace(script, test_script)
    main_hash = sha256_source(test_script)
    probe = """(async function(){await new Promise(function(r){setTimeout(r,80)});
if(location.search.indexOf('sos=1')>=0){document.getElementById('sos-trigger').click();await new Promise(function(r){setTimeout(r,80)})}
if(location.search.indexOf('lose=1')>=0){await new Promise(function(r){setTimeout(r,1100)})}
var o={hr:document.getElementById('hr').textContent,spo2:document.getElementById('spo2').textContent,
temp:document.getElementById('temp').textContent,
hum:document.getElementById('hum').textContent,pm:document.getElementById('pm').textContent,
link:document.getElementById('link').textContent,risk:document.getElementById('risk').textContent,
sos:document.getElementById('sos-note').textContent,calls:window.__calls};
var p=document.createElement('pre');p.id='softap-probe';p.textContent=JSON.stringify(o);document.body.appendChild(p)})();"""
    probe_hash = sha256_source(probe)
    page, count = re.subn(
        r"(script-src )'sha256-[^']+'",
        "\\1'sha256-" + main_hash + "' 'sha256-" + probe_hash + "'",
        page,
        count=1,
    )
    if count != 1:
        raise RuntimeError("embedded page CSP script-src missing")
    return page.replace("</body>", "<script>" + probe + "</script></body>")


def render(page, query):
    with tempfile.NamedTemporaryFile("w", suffix=".html", delete=False, encoding="utf-8") as fh:
        fh.write(page)
        path = Path(fh.name)
    try:
        proc = subprocess.run(
            [CHROME, "--headless=new", "--disable-gpu", "--no-sandbox",
             "--virtual-time-budget=3000", "--dump-dom", path.as_uri() + query],
            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60,
        )
        match = re.search(r'<pre id="softap-probe">(.*?)</pre>', proc.stdout, re.S)
        if not match:
            raise RuntimeError("Chrome did not render the SoftAP probe: " + proc.stderr[-600:])
        return json.loads(html.unescape(match.group(1)))
    finally:
        path.unlink(missing_ok=True)


source, page = embedded_page()
script = re.search(r"<script>(.*?)</script>", page, re.S).group(1)
script_hash = sha256_source(script)
if "'sha256-%s'" % script_hash not in page:
    raise RuntimeError("embedded page CSP hash does not match its script")
if "'sha256-%s'" % script_hash not in source:
    raise RuntimeError("HTTP response CSP hash does not match the embedded script")
if len(re.findall(r"<h1\b", page, re.I)) != 1 or "<html lang='en'>" not in page:
    raise RuntimeError("SoftAP page needs one h1 and a language declaration")
if not re.search(r"name='viewport'", page) or not re.search(r"<label[^>]+for='locin'", page):
    raise RuntimeError("mobile viewport or location input label missing")
if re.search(r"\son[a-z]+\s*=", page, re.I) or re.search(r"(?:src|href)\s*=\s*['\"]https?://", page, re.I):
    raise RuntimeError("inline event handler or external dependency found")
if "Access-Control-Allow-Origin" in source:
    raise RuntimeError("wildcard CORS header must not expose patient history")

healthy = {
    "hr": 72.0, "spo2": 98.4, "rr": 15.0, "sqi": 0.87, "rmssd": 42.0,
    "temp": -3.5, "hum": 0.0, "pm25": 0.0,
    "pressure": 1013.2, "ptrend": -1.25, "news2": 0, "level": 0, "flags": 0,
    "risk": "NORMAL", "storm": "HIGH", "sos": "IDLE", "trigger": "none",
    "loc": "UNSET", "motion": 0.1, "fall": "MONITORING", "contact": True, "up": 12,
}
live = render(build_fixture_page(page, healthy, click_sos=True), "?case=live&sos=1")
assert (live["hr"], live["spo2"], live["temp"]) == ("72", "98.4", "-3.5"), live
assert (live["hum"], live["pm"], live["link"]) == ("0", "0", "finger detected"), live
assert live["sos"] == "SOS trigger request sent", live
assert ["/api/emergency/trigger", "POST"] in live["calls"], live
print("SoftAP live values and SOS button: PASS")

no_finger = dict(healthy, hr=0.0, spo2=0.0, rr=0.0, sqi=0.0, rmssd=0.0,
                 risk="UNKNOWN", storm="NORMAL", contact=False)
absent = render(build_fixture_page(page, no_finger), "?case=nofinger")
assert (absent["hr"], absent["link"]) == ("--", "no finger"), absent
print("SoftAP absent finger values: PASS")

offline = render(build_fixture_page(page, healthy, lose_link=True), "?case=offline&lose=1")
assert (offline["hr"], offline["risk"], offline["link"]) == (
    "--", "NO CONNECTION", "reconnecting"), offline
print("SoftAP link-loss stale-value clearing: PASS")
