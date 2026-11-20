import json,hashlib,subprocess,argparse,shutil
from pathlib import Path
P=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument('--zig',required=True);ap.add_argument('--launcher-only',action='store_true');args=ap.parse_args();Z=Path(args.zig).resolve()
src=P/'source';out=P/'Runtime';out.mkdir(exist_ok=True)
flags=[str(Z),'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-const-variable','-fms-extensions','-mno-avx','-Wl,--dynamicbase','-Wl,--nxcompat']
files=['combined.c','r3_unit.c','positions_unit.c','capacity_unit.c','roster_unit.c','reserve_unit.c']+['r3/'+n for n in ['eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]
dll=out/'DRIPlus3-RM18R22.dll'
if not args.launcher_only:subprocess.run(flags+['-shared']+[str(src/n)for n in files]+['-o',str(dll)],check=True)
h=hashlib.sha256(dll.read_bytes()).hexdigest();(src/'dll_hash.h').write_text('#define DRI_DLL_HASH "'+h+'"\n')
if (src/'launcher.c').exists():subprocess.run(flags+['-municode',str(src/'launcher.c'),'-lbcrypt','-o',str(out/'DRIPlus3-RM18R22.exe')],check=True)
print(json.dumps({'dll_sha256':h,'bytes':dll.stat().st_size}))
