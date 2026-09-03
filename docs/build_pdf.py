import markdown
import codecs

input_file = 'docs/theory/THEORY_NOTES.md'
output_file = 'docs/theory/THEORY_NOTES_TEMP.html'

with codecs.open(input_file, mode='r', encoding='utf-8') as f:
    text = f.read()

html = markdown.markdown(text, extensions=['tables'])

styled_html = f'''
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <style>
        body {{ font-family: Arial, sans-serif; line-height: 1.6; margin: 40px; color: #333; }}
        h1, h2, h3 {{ color: #222; border-bottom: 1px solid #ccc; padding-bottom: 5px; }}
        table {{ border-collapse: collapse; width: 100%; margin-bottom: 20px; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
        code {{ background-color: #f9f2f4; color: #c7254e; padding: 2px 4px; border-radius: 4px; }}
        pre code {{ background-color: #f5f5f5; color: #333; display: block; padding: 10px; overflow-x: auto; }}
        img {{ max-width: 100%; height: auto; display: block; margin: 20px auto; }}
    </style>
</head>
<body>
{html}
</body>
</html>
'''

with codecs.open(output_file, mode='w', encoding='utf-8') as f:
    f.write(styled_html)
