"""Execute the production build guard on capture bytes and corrupt every guarded site.
No live-game behavior claim. Windows services are provided by the common fixture.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'runtime_fixture_test.py').read_text().split('prefix=bytes.fromhex')[0],__file__,'exec'))
import hashlib,re
report=json.loads((CAPTURE/'report.json').read_text(encoding='utf-8-sig'))
approved_captures={
 '8247e6dcc68b78672ac1c79e2deb41900925e6303a71c1e0ace02ef8602b3be2':'7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e',
 '99ac5da70511c0c8ec5f8677d0e847e6443e1bfa56b22842228edae4723387f8':'f04870ba6c6490b6353157d0bd7fddbbb615413635b7861cfe357b28308899cf',
}
assert report['Complete'] and report['ExecutableSha256'] in approved_captures
assert hashlib.sha256((CAPTURE/'section_00_00001000.bin').read_bytes()).hexdigest()==approved_captures[report['ExecutableSha256']]
uc.mem_map(GAME,0x1000);headers=(CAPTURE/'pe_headers.bin').read_bytes();uc.mem_write(GAME,headers)
guards=[]
for file in ['signatures.h','retention.c']:
 for a,n,b in re.findall(r'\{(0x[\da-f]+),(\d+),\{([^}]+)\}',(ROOT/file).read_text()):
  guards.append((int(a,16),bytes(int(v.strip(),0)for v in b.split(','))[:int(n)],file))
for section in report['Sections']:
 if not section.get('Selected'):continue
 data=(CAPTURE/section['File']).read_bytes();assert len(data)==section['RequestedBytes']==section['ReadBytes']
 assert not section['UnreadableSpans'] and hashlib.sha256(data).hexdigest()==section['Sha256']
 # Only the primary code and rdata sections contain release guards.
 if section['File']not in ['section_00_00001000.bin','section_02_0AC9C000.bin']:continue
 for a,expected,file in guards:
  off=a-section['Rva']
  if 0<=off and off+len(expected)<=len(data):
   uc.mem_map(GAME+a,len(expected));uc.mem_write(GAME+a,data[off:off+len(expected)])
   assert bytes(uc.mem_read(GAME+a,len(expected)))==expected
# Retain every prior native guard/hook, then add the exact native expression
# evaluator required by Test30. Count expectations are explicit, not inferred
# from whatever source happens to compile.
import os
baseline=Path(os.environ['DRI_BASELINE_DIR'])/'Runtime/Portal/Source'
old=[]
for file in ['signatures.h','retention.c']:
 for a,n,b in re.findall(r'\{(0x[\da-f]+),(\d+),\{([^}]+)\}',(baseline/file).read_text()):
  old.append((int(a,16),bytes(int(v.strip(),0)for v in b.split(','))[:int(n)],file))
assert len(old)==211+47
assert set(old).issubset(set(guards)), 'A preexisting native compatibility guard was altered or removed'
assert len(guards)==212+47
assert {a for a,data,file in guards if (a,data,file) not in old}=={0x08627910}
entry=exports['DriTestTU1029VerifyBuild'];assert call(entry,GAME)==1
for a,data,file in guards:
 uc.mem_write(GAME+a,bytes([data[0]^1]));assert call(entry,GAME)==0,(hex(a),file)
 uc.mem_write(GAME+a,data[:1]);assert call(entry,GAME)==1
peoff=int.from_bytes(headers[0x3c:0x40],'little')
for offset,size,value in [(0,2,0),(peoff,4,0),(peoff+4,2,0x14c),(peoff+8,4,1791226072),(peoff+24+56,4,301981696)]:
 original=bytes(uc.mem_read(GAME+offset,size));uc.mem_write(GAME+offset,value.to_bytes(size,'little'))
 assert call(entry,GAME)==0;uc.mem_write(GAME+offset,original)
assert call(entry,GAME)==1
result={'passed':True,'capture_complete':True,'executable_sha256':report['ExecutableSha256'],'guards':212,'hooks':47,'single_site_mismatch_rejections':259,'all_baseline_guards_preserved':True,'header_mismatch_rejections':5,'live_game_verified':False}
(OUTPUT/'test30_compat_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
