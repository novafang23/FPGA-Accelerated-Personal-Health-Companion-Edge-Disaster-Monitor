#!/usr/bin/env python3
"""
scripts/check_presentation_claims.py
Cross-checks technical numbers, paths, and claim phrasing across markdown documentation
and PowerPoint (.pptx) presentation decks without requiring python-pptx (uses stdlib zipfile/xml).

Exit code: 0 if no violations found, 1 if any fatal violation found.
"""

import os
import sys
import re
import glob
import zipfile
import xml.etree.ElementTree as ET

# Reconfigure stdout to utf-8
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

CANONICAL = {
    "ZYNQ_LUT": 185,
    "ZYNQ_FMAX": 69.45,
    "ZYNQ_WNS": 5.603,
    "FORGEFPGA_LUT": 515,
    "FORGEFPGA_TOTAL_LUT": 1120,
    "FORGEFPGA_FF": 303,
    "FORGEFPGA_FMAX": 78.85,
    "FORGEFPGA_WNS": 7.316,
    "INT8_PARAMS": 619,
    "INT8_MACS": 576,
    "TEACHER_AGREE": 88.39,
    "MIMIC_TOTAL": 16387,
    "MIMIC_ACC": 94.11,
    "MIMIC_SENS": 58.18,
    "MIMIC_SPEC": 98.19,
}

violations = []
warnings = []

def add_violation(file_path, line_no, rule_id, message, excerpt=""):
    violations.append((file_path, line_no, rule_id, message, excerpt.strip()))

def add_warning(file_path, line_no, rule_id, message, excerpt=""):
    warnings.append((file_path, line_no, rule_id, message, excerpt.strip()))

def check_text_line(file_path, line_no, line, context_lines=None):
    clean = line.strip()
    if not clean or clean.startswith("# SDD ledger"):
        return

    # RULE: STALE-PATH
    if "docs/PROJECT_GUIDE.md" in clean:
        add_violation(file_path, line_no, "STALE-PATH", "Referenced nonexistent docs/PROJECT_GUIDE.md (use docs/MASTER_PROJECT_GUIDE.md)", clean)

    # RULE: TEACHER - 88.39% or 88.47% without teacher/agreement/distil
    if re.search(r"88\.(39|47)\s*%", clean, re.IGNORECASE):
        if not re.search(r"teacher|agreement|distil|fidelity|synthetic", clean, re.IGNORECASE):
            add_violation(file_path, line_no, "TEACHER", "88.39% quoted without 'teacher' / 'agreement' / 'distil' in context", clean)

    # RULE: HOST-LATENCY - 0.44 us in sentence with ESP32 without host/x86
    if re.search(r"0\.44\s*(µs|us|microseconds)", clean, re.IGNORECASE):
        if re.search(r"ESP32|microcontroller|on-device|chip", clean, re.IGNORECASE) and not re.search(r"host|x86|not yet measured", clean, re.IGNORECASE):
            add_violation(file_path, line_no, "HOST-LATENCY", "0.44 µs latency associated with ESP32 instead of host x86", clean)

    # RULE: OVERCLAIM - clinically validated / medical grade / FDA / diagnostic device
    if re.search(r"\b(clinically validated|medical-grade|FDA approved|FDA cleared|diagnostic device)\b", clean, re.IGNORECASE):
        # Exclude guidance/rules/roadmap that explicitly instruct to avoid or explain non-clinical status
        if not re.search(r"not|open|roadmap|standards|non-|unvalidated|avoid|do not|never|negative|rule|guidance|prohibited", clean, re.IGNORECASE):
            add_violation(file_path, line_no, "OVERCLAIM", "Forbidden clinical/medical claim phrasing used", clean)

    # RULE: HRV-ACCURACY - claiming HRV/RMSSD accuracy as proven
    if re.search(r"\b(HRV|RMSSD)\b[^.]{0,40}\b(accuracy|validated|precise|proven)\b", clean, re.IGNORECASE):
        if not re.search(r"not|open|pending|caveat|timing|before|required|still|independent", clean, re.IGNORECASE):
            add_violation(file_path, line_no, "HRV-ACCURACY", "Claimed HRV accuracy before ECG Holter comparison (T4.1 is open)", clean)

    # RULE: NUM-MISMATCH
    # Check ForgeFPGA LUT count near /1120
    m = re.search(r"(\d+)\s*(?:CLB\s*)?LUT5?s?\s*/\s*1[,.]?120", clean, re.IGNORECASE)
    if m:
        val = int(m.group(1))
        if val != CANONICAL["FORGEFPGA_LUT"]:
            add_violation(file_path, line_no, "NUM-MISMATCH", f"ForgeFPGA LUT count is {val}, expected {CANONICAL['FORGEFPGA_LUT']}", clean)

    # Check Zynq LUT count
    if re.search(r"Zynq|xc7z020", clean, re.IGNORECASE) and re.search(r"\bLUT\b", clean):
        m = re.search(r"(\d+)\s*LUT\b", clean)
        if m:
            val = int(m.group(1))
            if val != CANONICAL["ZYNQ_LUT"] and val != 184 and not ("198" in clean and "re-run" in clean.lower()):
                add_violation(file_path, line_no, "NUM-MISMATCH", f"Zynq LUT count is {val}, expected {CANONICAL['ZYNQ_LUT']}", clean)

    # Check INT8 parameters count
    if re.search(r"INT8|TinyML|MLP", clean, re.IGNORECASE) and re.search(r"param|weight", clean, re.IGNORECASE):
        m = re.search(r"(\d{3,4})\s*(?:INT8\s*)?(?:params|parameters|weights)", clean, re.IGNORECASE)
        if m:
            val = int(m.group(1))
            if val != CANONICAL["INT8_PARAMS"] and val != 576:
                add_violation(file_path, line_no, "NUM-MISMATCH", f"Model parameters count is {val}, expected {CANONICAL['INT8_PARAMS']}", clean)

    # Check MIMIC records count
    if re.search(r"MIMIC", clean, re.IGNORECASE) and re.search(r"record|patient|sample", clean, re.IGNORECASE):
        m = re.search(r"(\d{1,2}[,.]?\d{3})\s*(?:records|patients|samples|rows)", clean, re.IGNORECASE)
        if m:
            val = int(m.group(1).replace(",", "").replace(".", ""))
            if val not in [CANONICAL["MIMIC_TOTAL"], 98, 16387]:
                add_violation(file_path, line_no, "NUM-MISMATCH", f"MIMIC count is {val}, expected {CANONICAL['MIMIC_TOTAL']} (or 98 demo subset)", clean)

    # Check SENSITIVITY-HIDDEN (warning level)
    if "94.11" in clean:
        has_sens = False
        if context_lines:
            for cl in context_lines:
                if "58.18" in cl:
                    has_sens = True
                    break
        else:
            has_sens = "58.18" in clean
        if not has_sens:
            add_warning(file_path, line_no, "SENSITIVITY-HIDDEN", "Accuracy 94.11% cited without adjacent sensitivity 58.18%", clean)

def check_markdown_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
        in_avoid_block = False
        for line_no, line in enumerate(lines, 1):
            stripped = line.strip()
            if stripped.startswith("### Avoid") or stripped.startswith("## Avoid"):
                in_avoid_block = True
            elif stripped.startswith("#") and in_avoid_block:
                in_avoid_block = False
            
            # If line is in avoid block, pass 'avoid' as context flag
            line_to_check = line
            if in_avoid_block:
                line_to_check = f"[AVOID BLOCK] {line}"

            # Context window of ±5 lines for SENSITIVITY-HIDDEN
            start_ctx = max(0, line_no - 6)
            end_ctx = min(len(lines), line_no + 5)
            ctx = lines[start_ctx:end_ctx]

            check_text_line(file_path, line_no, line_to_check, context_lines=ctx)
    except Exception as e:
        add_violation(file_path, 0, "FILE-READ-ERROR", str(e), "")

def check_pptx_file(pptx_path):
    if not os.path.exists(pptx_path):
        add_violation(pptx_path, 0, "MISSING-FILE", f"Presentation file not found: {pptx_path}", "")
        return

    try:
        with zipfile.ZipFile(pptx_path, "r") as z:
            slides = [n for n in z.namelist() if n.startswith("ppt/slides/slide") and n.endswith(".xml")]
            for s in sorted(slides, key=lambda x: int("".join(filter(str.isdigit, x)) or 0)):
                slide_num = "".join(filter(str.isdigit, s)) or "?"
                xml_data = z.read(s)
                root = ET.fromstring(xml_data)
                texts = [elem.text for elem in root.iter() if elem.text and elem.text.strip()]
                for t in texts:
                    check_text_line(f"{pptx_path} (Slide {slide_num})", 1, t, context_lines=texts)
    except Exception as e:
        add_violation(pptx_path, 0, "PPTX-PARSE-ERROR", str(e), "")

def main():
    explicit_pptx = []
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == "--pptx" and i + 1 < len(args):
            explicit_pptx.append(args[i + 1])
            i += 2
        else:
            i += 1

    # 1. Check Markdown Documentation
    doc_files = ["README.md"]
    doc_files.extend(glob.glob("docs/*.md"))
    doc_files.extend(glob.glob("docs/presentation/*.md"))
    
    # Filter out archive and superpowers internal plans
    doc_files = [f for f in doc_files if "archive" not in f and "superpowers" not in f]

    print(f"Scanning {len(doc_files)} markdown document(s)...")
    for df in doc_files:
        check_markdown_file(df)

    # 2. Check PPTX decks
    if explicit_pptx:
        decks = explicit_pptx
    else:
        decks = glob.glob("submission/*.pptx") + glob.glob("docs/presentation/*.pptx")

    print(f"Scanning {len(decks)} presentation deck(s)...")
    for d in decks:
        check_pptx_file(d)

    # 3. Report Results
    print("\n==========================================================")
    print("   PRESENTATION CLAIM & NUMBER VERIFICATION REPORT")
    print("==========================================================")

    if warnings:
        print(f"NOTICE: {len(warnings)} warning(s) detected (non-fatal):")
        for f, l, rule, msg, ex in warnings:
            print(f"  [WARN:{rule}] {f}:{l} - {msg}")
            if ex:
                print(f"    Excerpt: \"{ex[:80]}\"")
        print()

    if not violations:
        print("ALL CHECKS PASSED: 0 fatal violations found across docs and slides.\n")
        return 0

    print(f"FAILED: Found {len(violations)} fatal violation(s):\n")
    for f, l, rule, msg, ex in violations:
        print(f"  [{rule}] {f}:{l}")
        print(f"    Issue:   {msg}")
        if ex:
            print(f"    Excerpt: \"{ex[:100]}\"")
        print()

    return 1

if __name__ == "__main__":
    sys.exit(main())
