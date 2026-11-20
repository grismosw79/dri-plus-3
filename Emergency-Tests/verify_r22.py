from pathlib import Path
import argparse,hashlib,json,difflib,re,sys
import pefile
ap=argparse.ArgumentParser();ap.add_argument('--baseline',required=True);ap.add_argument('--candidate',required=True);ap.add_argument('--out',required=True);a=ap.parse_args()
B=Path(a.baseline);C=Path(a.candidate);O=Path(a.out);O.mkdir(parents=True,exist_ok=True)
expected={'combined.c','dll_hash.h','launcher.c','r3/car35_emergency.inc','r3/car35_emergency_test.inc','r3/car35_hooks.inc','r3/car35_identity.inc','r3/car35_policy.h','r3/car35_sites.inc','r3/car35_test_exports.inc','r3/retention.c'}
changes=[];diff=[];unchanged=0
for p in sorted((C/'source').rglob('*')):
    if not p.is_file():continue
    name=p.relative_to(C/'source').as_posix();old=B/'source'/name
    if old.exists() and old.read_bytes()==p.read_bytes():unchanged+=1;continue
    before=old.read_text()if old.exists()else '';after=p.read_text()
    kind='version_only' if after.replace('RM18R22','RM18R21')==before else 'substantive'
    if kind=='substantive':assert name in expected,('Unexpected source change',name)
    changes.append({'path':name,'kind':kind,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
    diff.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='RM18R21/'+name,tofile='RM18R22/'+name))
assert {x['path']for x in changes if x['kind']=='substantive'}==expected
assert not [p for p in (B/'source').rglob('*')if p.is_file() and not (C/'source'/p.relative_to(B/'source')).exists()]
for protected in ['r3/pp28_refresh.inc','r3/eps_runtime.inc','r3/pt28_runtime.inc','r3/car35_contract.inc','r3/car35_hc_mobility.inc','r3/car35_competition.inc','roster/rm_policy.h']:
    assert (B/'source'/protected).read_bytes()==(C/'source'/protected).read_bytes(),protected
assert 'L"DRIPlus3-RM18R21.dll"' in (C/'source/combined.c').read_text()
binary={}
for name in ['DRIPlus3-RM18R22.dll','DRIPlus3-RM18R22.exe']:
    p=C/'Runtime'/name;pe=pefile.PE(str(p));assert pe.FILE_HEADER.Machine==0x8664
    assert pe.OPTIONAL_HEADER.DllCharacteristics & 0x140==0x140
    assert not [x.name for x in getattr(pe,'DIRECTORY_ENTRY_EXPORT',type('X',(),{'symbols':[]})()).symbols if x.name and x.name.startswith(b'DriTest')]
    binary[name]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size,'windows_x64':True,'ASLR_DEP':True}
digest=binary['DRIPlus3-RM18R22.dll']['sha256']
assert digest in (C/'source/dll_hash.h').read_text()
assert digest.encode() in (C/'Runtime/DRIPlus3-RM18R22.exe').read_bytes()
result={'passed':True,'unchanged_source_files':unchanged,'changes':changes,'protected_refresh_contract_and_recruiting_policy_files_unchanged':True,'mixed_R21_R22_blocked':True,'binary':binary,'live_acceptance':False}
(O/'preservation-and-binary.json').write_text(json.dumps(result,indent=2)+'\n');(O/'R21-to-R22.patch').write_text(''.join(diff))
print(json.dumps({'passed':True,'unchanged_source_files':unchanged,'version_only_files':sum(x['kind']=='version_only'for x in changes),'substantive_files':sum(x['kind']=='substantive'for x in changes),'binary':binary},indent=2))
