import re
import os
import subprocess

script_dir = os.path.dirname(os.path.abspath(__file__))
repo_root = os.path.dirname(script_dir)

md_path = os.path.join(repo_root, "docs", "theory", "THEORY_NOTES.md")
html_path = os.path.join(repo_root, "docs", "theory", "theory_notes_print.html")
pdf_path = os.path.join(repo_root, "docs", "theory", "SIH26181_Master_Theory_Notes.pdf")

with open(md_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

html_body = []
in_code = False
code_lang = ""
code_buf = []
in_table = False
table_buf = []
in_blockquote = False
bq_buf = []

def escape_html(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

def inline_format(text):
    # bold italic
    text = re.sub(r'\*\*\*(.*?)\*\*\*', r'<strong><em>\1</em></strong>', text)
    # bold
    text = re.sub(r'\*\*(.*?)\*\*', r'<strong>\1</strong>', text)
    # italic
    text = re.sub(r'\*(.*?)\*', r'<em>\1</em>', text)
    text = re.sub(r'_(.*?)_', r'<em>\1</em>', text)
    # inline code
    text = re.sub(r'`(.*?)`', r'<code>\1</code>', text)
    # links
    text = re.sub(r'\[(.*?)\]\((.*?)\)', r'<a href="\2">\1</a>', text)
    return text

def render_table(rows):
    if not rows:
        return ""
    out = ['<div class="table-container"><table>']
    # header
    header_cells = [c.strip() for c in rows[0].split('|')[1:-1]]
    out.append('<thead><tr>')
    for c in header_cells:
        out.append(f'<th>{inline_format(c)}</th>')
    out.append('</tr></thead><tbody>')
    
    # data rows (skip separator row at index 1)
    for r in rows[2:]:
        cells = [c.strip() for c in r.split('|')[1:-1]]
        out.append('<tr>')
        for c in cells:
            out.append(f'<td>{inline_format(c)}</td>')
        out.append('</tr>')
    out.append('</tbody></table></div>')
    return "\n".join(out)

i = 0
while i < len(lines):
    line = lines[i]
    stripped = line.strip()
    
    # Code block
    if stripped.startswith("```"):
        if in_code:
            html_body.append(f'<pre class="code-block {code_lang}"><code>' + escape_html("".join(code_buf)) + '</code></pre>')
            code_buf = []
            in_code = False
        else:
            in_code = True
            code_lang = stripped[3:].strip()
        i += 1
        continue
    
    if in_code:
        code_buf.append(line)
        i += 1
        continue

    # Tables
    if stripped.startswith("|") and stripped.endswith("|"):
        table_buf.append(stripped)
        i += 1
        continue
    else:
        if table_buf:
            html_body.append(render_table(table_buf))
            table_buf = []

    # Blockquotes
    if stripped.startswith(">"):
        content = stripped[1:].strip()
        bq_buf.append(content)
        i += 1
        continue
    else:
        if bq_buf:
            bq_text = "<br>".join([inline_format(x) for x in bq_buf if x])
            html_body.append(f'<div class="callout-box">{bq_text}</div>')
            bq_buf = []

    # Empty line
    if not stripped:
        html_body.append("")
        i += 1
        continue

    # Images
    img_match = re.match(r'^!\[(.*?)\]\((.*?)\)$', stripped)
    if img_match:
        caption = img_match.group(1)
        rel_img = img_match.group(2)
        abs_img = os.path.normpath(os.path.join(os.path.dirname(md_path), rel_img))
        img_url = abs_img.replace(os.sep, '/')
        html_body.append(f'<div class="img-container"><img src="file:///{img_url}" alt="{caption}"/><div class="img-caption">{caption}</div></div>')
        i += 1
        continue

    # Headers
    if stripped.startswith("# "):
        html_body.append(f'<h1 class="part-title">{inline_format(stripped[2:])}</h1>')
    elif stripped.startswith("## "):
        html_body.append(f'<h2 class="section-title">{inline_format(stripped[3:])}</h2>')
    elif stripped.startswith("### "):
        html_body.append(f'<h3 class="subsection-title">{inline_format(stripped[4:])}</h3>')
    elif stripped.startswith("#### "):
        html_body.append(f'<h4 class="question-title">{inline_format(stripped[5:])}</h4>')
    elif stripped.startswith("---"):
        html_body.append('<hr class="divider"/>')
    elif stripped.startswith("- ") or stripped.startswith("* "):
        html_body.append(f'<div class="bullet-item">• {inline_format(stripped[2:])}</div>')
    elif re.match(r'^\d+\.\s', stripped):
        num, text = stripped.split(".", 1)
        html_body.append(f'<div class="numbered-item"><span class="num">{num}.</span> {inline_format(text.strip())}</div>')
    else:
        html_body.append(f'<p class="paragraph">{inline_format(stripped)}</p>')
    
    i += 1

if table_buf:
    html_body.append(render_table(table_buf))
if bq_buf:
    bq_text = "<br>".join([inline_format(x) for x in bq_buf if x])
    html_body.append(f'<div class="callout-box">{bq_text}</div>')

full_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>SIH26181 — Master Theory Notes &amp; Judge Defense Companion</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&family=JetBrains+Mono:wght@400;500;700&display=swap');

@page {{
    size: A4;
    margin: 16mm 14mm 16mm 14mm;
}}

body {{
    font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    color: #1e293b;
    background-color: #ffffff;
    line-height: 1.55;
    font-size: 13.5px;
    margin: 0;
    padding: 0;
}}

.header-banner {{
    background: linear-gradient(135deg, #0A2540 0%, #006699 100%);
    color: white;
    padding: 24px 28px;
    border-radius: 8px;
    margin-bottom: 24px;
}}

.header-banner h1 {{
    font-size: 24px;
    font-weight: 800;
    margin: 0 0 6px 0;
    color: #ffffff;
    letter-spacing: -0.5px;
}}

.header-banner .subtitle {{
    font-size: 15px;
    font-weight: 600;
    color: #38bdf8;
    margin: 0 0 10px 0;
}}

.header-banner .tagline {{
    font-size: 12px;
    color: #cbd5e1;
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 600;
}}

.part-title {{
    font-size: 19px;
    font-weight: 800;
    color: #0A2540;
    border-bottom: 2px solid #0284c7;
    padding-bottom: 6px;
    margin-top: 32px;
    margin-bottom: 16px;
    page-break-after: avoid;
}}

.section-title {{
    font-size: 16px;
    font-weight: 700;
    color: #0369a1;
    margin-top: 24px;
    margin-bottom: 12px;
    page-break-after: avoid;
}}

.subsection-title {{
    font-size: 14.5px;
    font-weight: 700;
    color: #0f172a;
    margin-top: 18px;
    margin-bottom: 8px;
    page-break-after: avoid;
}}

.question-title {{
    font-size: 13.5px;
    font-weight: 700;
    color: #0369a1;
    margin-top: 16px;
    margin-bottom: 6px;
    page-break-after: avoid;
}}

.paragraph {{
    margin: 0 0 10px 0;
    color: #334155;
    text-align: justify;
}}

.bullet-item {{
    margin: 3px 0 3px 14px;
    color: #334155;
}}

.numbered-item {{
    margin: 4px 0 4px 14px;
    color: #334155;
}}

.numbered-item .num {{
    font-weight: 600;
    color: #0284c7;
}}

.callout-box {{
    background-color: #f0fdf4;
    border-left: 4px solid #16a34a;
    padding: 12px 16px;
    border-radius: 0 6px 6px 0;
    margin: 12px 0 16px 0;
    font-size: 13px;
    color: #14532d;
    page-break-inside: avoid;
}}

.table-container {{
    margin: 14px 0 18px 0;
    page-break-inside: avoid;
}}

table {{
    width: 100%;
    border-collapse: collapse;
    font-size: 12px;
    background: #ffffff;
}}

th {{
    background: #0A2540;
    color: #ffffff;
    font-weight: 600;
    text-align: left;
    padding: 8px 10px;
    border: 1px solid #cbd5e1;
}}

td {{
    padding: 7px 10px;
    border: 1px solid #e2e8f0;
    color: #334155;
    vertical-align: top;
}}

tr:nth-child(even) {{
    background-color: #f8fafc;
}}

.code-block {{
    background: #0f172a;
    color: #f1f5f9;
    font-family: 'JetBrains Mono', monospace;
    font-size: 11.5px;
    padding: 12px 14px;
    border-radius: 6px;
    overflow-x: auto;
    line-height: 1.45;
    margin: 12px 0 16px 0;
    border: 1px solid #1e293b;
    page-break-inside: avoid;
}}

code {{
    font-family: 'JetBrains Mono', monospace;
    background: #f1f5f9;
    color: #0f172a;
    padding: 1px 5px;
    border-radius: 4px;
    font-size: 12px;
}}

pre code {{
    background: transparent;
    color: inherit;
    padding: 0;
}}

.divider {{
    border: 0;
    border-top: 1px solid #e2e8f0;
    margin: 24px 0;
}}

a {{
    color: #0284c7;
    text-decoration: none;
}}

strong {{
    color: #0f172a;
}}

.img-container {{
    margin: 18px auto 22px auto;
    text-align: center;
    page-break-inside: avoid;
}}

.img-container img {{
    max-width: 95%;
    height: auto;
    border-radius: 6px;
    border: 1px solid #cbd5e1;
    box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.08);
}}

.img-caption {{
    font-size: 11px;
    color: #64748b;
    margin-top: 6px;
    font-weight: 500;
    font-style: italic;
}}
</style>
</head>
<body>

<div class="header-banner">
    <div class="tagline">Qualcomm Hardware Challenge • Smart India Hackathon 2026</div>
    <h1>SIH26181 — Master Theory Notes &amp; Judge Defense Companion</h1>
    <div class="subtitle">AI-Powered Personal Health Companion &amp; Edge Disaster Monitor</div>
    <div style="font-size: 12px; color: #e0f2fe;">FPGA-Accelerated, Cloud-Free Biometric &amp; Disaster Resilience Engine</div>
</div>

{"\n".join(html_body)}

</body>
</html>
"""

with open(html_path, "w", encoding="utf-8") as f:
    f.write(full_html)

print("HTML generated successfully. Launching Edge headless to print PDF...")

edge_path = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
cmd = [
    edge_path,
    "--headless",
    "--disable-gpu",
    "--no-pdf-header-footer",
    f"--print-to-pdf={pdf_path}",
    f"file:///{html_path.replace(os.sep, '/')}"
]

res = subprocess.run(cmd, capture_output=True, text=True)
print("Return code:", res.returncode)
if os.path.exists(pdf_path):
    print("PDF successfully generated at:", pdf_path, "Size:", os.path.getsize(pdf_path), "bytes")
else:
    print("PDF generation failed.")
