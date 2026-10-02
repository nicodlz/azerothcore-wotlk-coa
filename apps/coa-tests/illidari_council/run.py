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
    source = ROOT.joinpath('src/server/scripts/Outland/BlackTemple/boss_illidari_council.cpp').read_text()
    core = ROOT.joinpath('src/server/game/AI/ScriptedAI/ScriptedCreature.cpp').read_text()
    header = ROOT.joinpath('src/server/scripts/Outland/BlackTemple/black_temple.h').read_text()
    code = HERE.joinpath('harness.cpp').read_text()
    controller = source[source.index('struct boss_illidari_council :'):]
    member = source[source.index('struct boss_illidari_council_memberAI :'):]
    replacements = {
        'SAYS': method(source, 'enum Says') + ';',
        'SPELLS': method(source, 'enum Spells') + ';',
        'ACTIONS': method(source, 'enum Misc') + ';',
        'DATA': method(header, 'enum DataTypes') + ';',
        'START': method(controller, 'void DoAction(int32 param) override'),
        'DEATH': method(member, 'void JustDied(Unit*) override'),
        'ENGAGE': method(member, 'void JustEngagedWith(Unit*'),
        'RESET': method(core, 'void BossAI::_Reset()').replace('BossAI::', ''),
        'DONE': method(core, 'void BossAI::_JustDied()').replace('BossAI::', ''),
        'CONTROLLER_ENGAGE': method(core, 'void BossAI::_JustEngagedWith()').replace('BossAI::', ''),
    }
    for name, native in replacements.items():
        code = code.replace('// NATIVE_' + name, native)
    compiler = shutil.which(os.environ.get('CXX', 'cl.exe' if os.name == 'nt' else 'c++'))
    assert compiler, 'A C++20 compiler is required'
    with tempfile.TemporaryDirectory(prefix='coa-illidari-') as directory:
        out = Path(directory)
        cpp = out / 'harness.cpp'
        executable = out / ('harness.exe' if os.name == 'nt' else 'harness')
        cpp.write_text(code)
        flags = (['/nologo', '/std:c++20', '/EHsc', '/W4', '/WX', str(cpp), '/Fe' + str(executable)]
                 if Path(compiler).stem.lower() == 'cl' else
                 ['-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(executable)])
        subprocess.run([compiler, *flags], cwd=out, check=True, timeout=60)
        subprocess.run([str(executable)], cwd=out, check=True, timeout=15)
    print('PASS: native council start, reentrant engagement, one balance/scheduler/yell, reset/re-pull, '
          'completed encounter guard and all four death texts')


if __name__ == '__main__':
    main()
