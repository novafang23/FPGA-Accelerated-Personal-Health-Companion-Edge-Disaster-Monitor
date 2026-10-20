import subprocess, os

env = os.environ.copy()
env['PATH'] = r'C:\msys64\ucrt64\bin;' + env.get('PATH', '')

with open('test_gcc.c', 'w') as f:
    f.write('#include <stdio.h>\nint main() { printf("HELLO\\n"); return 0; }\n')

res = subprocess.run(['gcc', 'test_gcc.c', '-o', 'test_gcc.exe'], capture_output=True, text=True, env=env)
print('code:', res.returncode, 'err:', res.stderr, 'out:', res.stdout)
if res.returncode == 0:
    run_res = subprocess.run(['test_gcc.exe'], capture_output=True, text=True, env=env)
    print('run out:', run_res.stdout)

if os.path.exists('test_gcc.c'): os.remove('test_gcc.c')
if os.path.exists('test_gcc.exe'): os.remove('test_gcc.exe')
