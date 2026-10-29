#!/usr/bin/env python3
"""
scripts/inspect_presentation.py
Inspects slide text from a .pptx presentation using Python stdlib (zipfile + xml.etree).
Usage: python scripts/inspect_presentation.py [path_to_presentation.pptx]
"""

import os
import sys
import zipfile
import xml.etree.ElementTree as ET

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

def inspect_pptx(pptx_path):
    if not os.path.exists(pptx_path):
        print(f"Error: Presentation not found at {pptx_path}", file=sys.stderr)
        sys.exit(1)

    print(f"Inspecting: {pptx_path}\n" + "=" * 60)
    
    with zipfile.ZipFile(pptx_path, "r") as z:
        # Find all slide xml files
        slide_names = [n for n in z.namelist() if n.startswith("ppt/slides/slide") and n.endswith(".xml")]
        # Sort slides numerically
        def slide_num(name):
            digits = "".join(filter(str.isdigit, name))
            return int(digits) if digits else 0
        
        slide_names.sort(key=slide_num)
        
        for name in slide_names:
            num = slide_num(name)
            xml_content = z.read(name)
            root = ET.fromstring(xml_content)
            
            # XML namespace for DrawingML is usually http://schemas.openxmlformats.org/drawingml/2006/main
            # Extract all text elements (<a:t>)
            texts = []
            for elem in root.iter():
                if elem.tag.endswith("}t") and elem.text:
                    clean = elem.text.strip()
                    if clean:
                        texts.append(clean)
            
            print(f"\n--- Slide {num} ({len(texts)} text elements) ---")
            for t in texts:
                print(f"  • {t}")

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "submission/VALOR_Hackathon_Submission.pptx"
    inspect_pptx(path)

if __name__ == "__main__":
    main()
