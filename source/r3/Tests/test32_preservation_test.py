"""Independent release boundary: byte-identical FTC-Master-R03 and paired binary identity.
The native generator regression already established by R1 remains applicable only
if all 20 delivered FTC bytes exactly match that immutable baseline.
"""
from pathlib import Path
import hashlib,json,os,re
S=Path(__file__).resolve().parents[1]
R=S.parents[2]
BASE=Path(os.environ['DRI_TEST31R1_BASELINE_DIR'])
OUT=Path(os.environ['DRI_TEST_OUTPUT_DIR'])
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ftcs(root):
 return {str(p.relative_to(root)):sha(p) for p in sorted(root.rglob('*'))
         if p.is_file() and p.suffix.lower()=='.ftc' and 'Audit' not in p.relative_to(root).parts}
a=ftcs(BASE);b=ftcs(R)
assert len(a)==len(b)==20,(len(a),len(b))
assert a==b,'A protected FTC changed or was removed/added'
assert not any('Test32' in path for path in a),'Unexpected source FTC paths'
dll=sha(S.parent/'DRIChallengerRetention.dll')
launcher=(S.parent/'DRIRetentionTest.exe').read_bytes()
assert dll.encode() in launcher,'Launcher does not contain exact matching DLL hash'
assert (S/'dll_hash.h').read_text().strip()=='#define DRI_DLL_HASH "'+dll+'"'
result={'passed':True,'unchanged_ftcs':20,'ftc_sha256':b,
 'production_dll_sha256':dll,'launcher_sha256':sha(S.parent/'DRIRetentionTest.exe'),
 'same_baseline':'Test31R1 / FTC-Master-R03','live_game_verified':False}
(OUT/'test32_preservation_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'passed':True,'unchanged_ftcs':20,'matching_binary_pair':True}))
