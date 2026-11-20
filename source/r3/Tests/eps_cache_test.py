"""Compiled cache publisher with captured native getters/setters and schema maps."""
from pathlib import Path
source=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(source.split("season=record('season'")[0],'runtime_fixture_test.py','exec'))
call(exports['DriTestConfigure'],GAME,1,7)
uc.mem_map(GAME+0x0fad5000,0x1000) # Native optional validation override is absent.
for rva,size in [(0x0860eda0,0x3000),(0x08614da0,0x2000),(0x08598da0,0x1000),(0x08596da0,0x1000),(0x07e18000,0x2000),(0x08055000,0x1000)]:
 uc.mem_map(GAME+rva,size);uc.mem_write(GAME+rva,raw[rva-0x1000:rva-0x1000+size])
rdata=(CAPTURE/'section_02_0AC9C000.bin').read_bytes()
for rva in [0x0bded000,0x0bdcf000]:
 uc.mem_map(GAME+rva,0x1000);uc.mem_write(GAME+rva,rdata[rva-0x0ac9c000:rva-0x0ac9c000+0x1000])
def set_value():
 t,row,f,value=args();records[tables[t][row]]['scalar'][f-0x100000]=value&0xffffffff;return 0
wq(TABLEVT+0x148,api(set_value));api(lambda:0,GAME+0x08059580)
teamlist=[];grade_tables=[];rank_tables=[]
for tid in range(138):
 gr=record('grade '+str(tid));rr=record('rank '+str(tid));grade_tables.append(gr);rank_tables.append(rr)
 tr=record('tracking '+str(tid),ref={20:gr,21:rr});teamlist.append(record('team '+str(tid),{383:tid},ref={165:tr}))
teams=record('teams',array=teamlist);franchise=record('franchise',{28:1549427592},ref={45:teams})
call(exports['DriTestEpsSeed'],franchise,2026,1,6)
call(exports['DriTestEpsSet'],2,3,8,1) # HB receiving profile, including elusive-receiving variant.
call(exports['DriTestEpsSet'],23,19,4,0) # Provisional kicker accuracy.
out=alloc(8)
def longcall(address,*values):
 rsp=STACK+0xf0000-8;wq(rsp,STOP);uc.reg_write(UC_X86_REG_RSP,rsp)
 for name,value in zip(['RCX','RDX','R8','R9'],values):uc.reg_write(globals()['UC_X86_REG_'+name],value)
 try:uc.emu_start(address,STOP+1,count=30000000)
 except unicorn.UcError:
  print('Fault',hex(reg('RIP')),list(recent),args(),flush=True);raise
 assert reg('RIP')==STOP
 return reg('RAX')&0xffffffff
assert longcall(exports['DriTestEpsCache'],franchise,1,out)==1
published=rq(out);assert published
assert longcall(exports['DriTestEpsCache'],franchise,0,out)==1 and rq(out)==published
valid=[]
for arch in range(63):
 if call(GAME+0x07e18bf0,arch):valid.append(arch)
grade_fields=struct.unpack('<'+('II'*63),rdata[0xbded114-0x0ac9c000:0xbded114-0x0ac9c000+63*8])[::2]
rank_fields=struct.unpack('<'+('II'*63),rdata[0xbdcf4a4-0x0ac9c000:0xbdcf4a4-0x0ac9c000+63*8])[::2]
for tid in range(138):
 for arch in valid:
  expected=8 if tid==2 and arch in [7,9,11] else 13 if tid==23 and arch==61 else 4
  assert records[grade_tables[tid]]['scalar'][grade_fields[arch]]==expected,(tid,arch)
  assert records[rank_tables[tid]]['scalar'][rank_fields[arch]]==(0 if expected==13 else 39)
records[grade_tables[0]]['scalar'][grade_fields[0]]=7
assert longcall(exports['DriTestEpsCache'],franchise,0,out)==1 and rq(out)!=published
call(exports['DriTestEpsReset']);assert all(v==1 for v in refs.values())
result={'passed':True,'nativeValidArchetypes':valid,'gradeAndRankWritesVerified':138*len(valid)*2,'cacheIdentityChangesWhenNativeGradeChanges':True,'referencesBalanced':True,'live_game_verified':False}
(OUTPUT/'eps_cache_results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
