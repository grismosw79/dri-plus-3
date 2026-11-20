"""Rebuild the current launcher; preserve its paired DLL byte-for-byte.
Usage: python build_launcher_only.py /path/to/zig-0.13.0/zig
"""
from pathlib import Path
import hashlib,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE.parent
zig=sys.argv[1] if len(sys.argv)>1 else 'zig'
assert subprocess.check_output([zig,'version'],text=True).strip()=='0.13.0','Zig 0.13.0 required'
# The CSV producer and monitor must use the same literal prefix. This catches
# an omitted include file when a candidate revision label changes.
producer=re.search(r'L"(DRI_PortalTest[^_]+)_%',(HERE/'retention.c').read_text()).group(1)
consumer=re.search(r'L"(DRI_PortalTest[^_]+)_\*_', (HERE/'launcher_monitor.inc').read_text()).group(1)
assert producer==consumer, f'CSV producer/monitor mismatch: {producer} vs {consumer}'
dll=OUT/'DRIChallengerRetention.dll'
before=hashlib.sha256(dll.read_bytes()).hexdigest()
expected=re.search(r'#define DRI_DLL_HASH "([a-f0-9]{64})"',(HERE/'dll_hash.h').read_text()).group(1)
assert before==expected,'Existing DLL does not match its pairing hash'
subprocess.run([zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(HERE),'-municode',str(HERE/'launcher.c'),'-lbcrypt','-luser32','-o',str(OUT/'DRIRetentionTest.exe'),'-Wl,--dynamicbase','-Wl,--nxcompat'],check=True)
assert hashlib.sha256(dll.read_bytes()).hexdigest()==before
assert before.encode() in (OUT/'DRIRetentionTest.exe').read_bytes()
print('Built launcher with paired DLL SHA256:',before)
