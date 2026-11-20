from pathlib import Path
import subprocess,sys,os
S=Path(os.environ.get('DRI_CAR35_SOURCE',str(Path(__file__).resolve().parents[1]/'source/r3')))
zig=sys.argv[1] if len(sys.argv)>1 else 'zig'
assert subprocess.check_output([zig,'version'],text=True).strip()=='0.13.0','Zig 0.13.0 required'
cmd=[zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(S),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']
cmd += [str(S/x)for x in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]
cmd += [str(Path(__file__).parent/'combined_fixture_stub.c')]
cmd += ['-o',str(Path(__file__).parent/'retention_fixture.dll'),'-Wl,--dynamicbase','-Wl,--nxcompat']
subprocess.run(cmd,check=True)
