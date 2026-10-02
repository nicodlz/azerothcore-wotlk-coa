import os
from pathlib import Path
import runpy
import shutil
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
method = runpy.run_path(str(HERE.parent / 'client_compat/run.py'))['method']


def main():
    relative = 'src/server/scripts/World/server_mail.cpp'
    reference = os.environ.get('COA_SERVER_MAIL_SOURCE_REF')
    source = (subprocess.check_output(['git', 'show', f'{reference}:{relative}'], cwd=ROOT).decode('utf-8')
              if reference else (ROOT / relative).read_text(encoding='utf-8'))
    harness = (HERE / 'harness.cpp').read_text(encoding='utf-8')
    harness = harness.replace('// ACTUAL_LOGIN', method(source, 'void OnPlayerLogin('))
    compiler = shutil.which(os.environ.get('CXX', 'cl.exe' if os.name == 'nt' else 'c++'))
    assert compiler, 'Enable a C++20 compiler.'
    with tempfile.TemporaryDirectory(prefix='coa-server-mail-callback-') as directory:
        output = Path(directory)
        cpp = output / 'harness.cpp'
        cpp.write_text(harness, encoding='utf-8')
        executable = output / ('regressions.exe' if os.name == 'nt' else 'regressions')
        if Path(compiler).stem.lower() == 'cl':
            flags = ['/nologo', '/std:c++20', '/EHsc', '/utf-8', str(cpp), '/Fe' + str(executable)]
        else:
            flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(executable)]
        subprocess.run([compiler, *flags], cwd=output, check=True, timeout=60)
        subprocess.run([str(executable)], cwd=output, check=True, timeout=15)


if __name__ == '__main__':
    main()
