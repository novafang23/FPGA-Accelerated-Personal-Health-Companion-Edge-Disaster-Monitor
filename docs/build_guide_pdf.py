import markdown
import codecs
import subprocess
import os

input_file = 'docs/PROJECT_GUIDE.md'
html_file = 'docs/PROJECT_GUIDE.html'
pdf_file = 'docs/VALOR_COMPLETE_GUIDE.pdf'

with codecs.open(input_file, mode='r', encoding='utf-8') as f:
    text = f.read()

# Convert markdown to html with tables and fenced_code
html_body = markdown.markdown(text, extensions=['tables', 'fenced_code'])

styled_html = f'''<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Project VALOR Complete Guide</title>
    <style>
        @page {{
            size: A4;
            margin: 20mm;
        }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            line-height: 1.5;
            color: #1a1a1a;
            font-size: 10.5pt;
        }}
        h1 {{
            color: #0b3d91;
            font-size: 20pt;
            border-bottom: 2px solid #0b3d91;
            padding-bottom: 6px;
            margin-top: 24px;
            page-break-after: avoid;
        }}
        h2 {{
            color: #1e3a8a;
            font-size: 15pt;
            border-bottom: 1px solid #cbd5e1;
            padding-bottom: 4px;
            margin-top: 18px;
            page-break-after: avoid;
        }}
        h3 {{
            color: #334155;
            font-size: 12.5pt;
            margin-top: 14px;
            page-break-after: avoid;
        }}
        h4 {{
            color: #475569;
            font-size: 11pt;
            margin-top: 12px;
            page-break-after: avoid;
        }}
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 14px 0;
            font-size: 9pt;
            page-break-inside: avoid;
        }}
        th, td {{
            border: 1px solid #cbd5e1;
            padding: 6px 10px;
            text-align: left;
        }}
        th {{
            background-color: #f1f5f9;
            font-weight: 600;
        }}
        tr:nth-child(even) {{
            background-color: #f8fafc;
        }}
        code {{
            background-color: #f1f5f9;
            color: #b91c1c;
            padding: 1px 4px;
            border-radius: 3px;
            font-family: Consolas, "Liberation Mono", Menlo, Courier, monospace;
            font-size: 9pt;
        }}
        pre {{
            background-color: #0f172a;
            color: #f8fafc;
            padding: 12px;
            border-radius: 6px;
            overflow-x: auto;
            font-size: 8.5pt;
            line-height: 1.4;
            page-break-inside: avoid;
        }}
        pre code {{
            background-color: transparent;
            color: inherit;
            padding: 0;
        }}
        blockquote {{
            border-left: 4px solid #3b82f6;
            margin: 12px 0;
            padding: 8px 16px;
            background-color: #eff6ff;
            color: #1e40af;
            font-style: italic;
        }}
        hr {{
            border: none;
            border-top: 1px solid #e2e8f0;
            margin: 20px 0;
        }}
    </style>
</head>
<body>
{html_body}
</body>
</html>'''

with codecs.open(html_file, mode='w', encoding='utf-8') as f:
    f.write(styled_html)

print("Generated HTML successfully.")

edge_path = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
if os.path.exists(edge_path):
    abs_html = os.path.abspath(html_file)
    abs_pdf = os.path.abspath(pdf_file)
    cmd = [
        edge_path,
        "--headless",
        "--disable-gpu",
        f"--print-to-pdf={abs_pdf}",
        "--no-pdf-header-footer",
        abs_html
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode == 0:
        print(f"Generated PDF successfully: {pdf_file}")
    else:
        print(f"Edge PDF generation returned {res.returncode}: {res.stderr}")
