import os, re

files_to_check = [
    'VALOR_COMPLETE_GUIDE1.md',
    'docs/VALOR_COMPLETE_GUIDE.pdf',
    'docs/PROJECT_GUIDE.md',
    'docs/REMAINING_WORK.md',
    'docs/reports/VERIFICATION_REPORT.md',
    'docs/reports/AUDIT_2.md',
    'docs/FPGA_PPG_FIX_HANDOFF.md',
    'docs/archive/CODEX_HANDOVER.md'
]

for f in files_to_check:
    if os.path.exists(f):
        sz = os.path.getsize(f)
        with open(f, 'rb') as fp:
            data = fp.read()
        has_315 = bool(re.search(b'315', data))
        has_336 = bool(re.search(b'336', data))
        has_hw_pending = bool(re.search(b'hardware\\s+pending', data, re.I))
        has_buzzer_pending = bool(re.search(b'buzzer.*pending', data, re.I))
        print(f'{f}: size={sz}, 315={has_315}, 336={has_336}, hw_pending={has_hw_pending}, buzzer_pending={has_buzzer_pending}')
