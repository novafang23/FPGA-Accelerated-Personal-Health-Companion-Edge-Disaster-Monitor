import os

files_to_remove = [
    'VALOR_COMPLETE_GUIDE1.md',
    'docs/VALOR_COMPLETE_GUIDE.pdf',
    'docs/PROJECT_GUIDE.md',
    'docs/REMAINING_WORK.md'
]

for f in files_to_remove:
    if os.path.exists(f):
        try:
            os.remove(f)
            print(f"DELETED: {f}")
        except Exception as e:
            print(f"ERROR deleting {f}: {e}")
    else:
        print(f"NOT FOUND (already removed): {f}")
