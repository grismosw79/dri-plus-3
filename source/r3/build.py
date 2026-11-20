"""Rebuild with Zig 0.13.0's Windows cross compiler. Python 3 required.
Usage: python build.py /path/to/zig
"""
from pathlib import Path
import sys,subprocess,hashlib,re
HERE=Path(__file__).resolve().parent
OUT=HERE.parent
zig=sys.argv[1] if len(sys.argv)>1 else 'zig'
version=subprocess.check_output([zig,'version'],text=True).strip()
if version!='0.13.0':
    raise SystemExit('Test36R3 requires Zig 0.13.0; found '+version+'. No binaries were rebuilt.')
producer=re.search(r'L"(DRI_PortalTest[^_]+)_%',(HERE/'retention.c').read_text()).group(1)
consumer=re.search(r'L"(DRI_PortalTest[^_]+)_\*_',(HERE/'launcher_monitor.inc').read_text()).group(1)
assert producer==consumer=='DRI_PortalTest36R3','DLL/monitor version mismatch'
base=[zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(HERE)]
subprocess.run(base+['-shared','-fms-extensions','-mno-avx',str(HERE/'retention.c'),str(HERE/'eps_core.c'),str(HERE/'eps_collect.c'),str(HERE/'eps_snapshot.c'),str(HERE/'pt28_core.c'),str(HERE/'pp28_core.c'),str(HERE/'staff28_core.c'),str(HERE/'hooks.S'),'-o',str(OUT/'DRIChallengerRetention.dll'),'-Wl,--dynamicbase','-Wl,--nxcompat'],check=True)
h=hashlib.sha256((OUT/'DRIChallengerRetention.dll').read_bytes()).hexdigest()
(HERE/'dll_hash.h').write_text('#define DRI_DLL_HASH "'+h+'"\n')
subprocess.run(base+['-municode',str(HERE/'launcher.c'),'-lbcrypt','-luser32','-o',str(OUT/'DRIRetentionTest.exe'),'-Wl,--dynamicbase','-Wl,--nxcompat'],check=True)
print('Built DLL and matching launcher. DLL SHA-256:',h)
