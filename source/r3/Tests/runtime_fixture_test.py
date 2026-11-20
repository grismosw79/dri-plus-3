"""Run actual compiled callbacks plus captured rank/commit/eligibility code.
Windows services and SDF storage are fixtures, not a running game.
"""
from pathlib import Path
from collections import deque
import sys,struct,json,os
ROOT=Path(__file__).resolve().parents[1]
OUTPUT=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',ROOT/'build'));OUTPUT.mkdir(parents=True,exist_ok=True)
CAPTURE=Path(os.environ.get('DRI_CAPTURE_DIR',ROOT.parent/'timing50/native_inputs/DRI_ReadOnly_20261013_221008_7d5d37'))
sys.path[:0]=[str(ROOT/'deps'),str(ROOT.parent/'timing50/deps')]
import pefile,capstone,unicorn
from unicorn.x86_const import *
P=pefile.PE(str(OUTPUT/'retention_fixture.dll'));BASE=P.OPTIONAL_HEADER.ImageBase
uc=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_64)
# TU1029 harness: align native mappings and reuse already mapped native pages.
# Non-native heap/stack/service mappings keep Unicorn's original strict behavior.
_original_mem_map=uc.mem_map
def _native_page_map(address,size,perms=unicorn.UC_PROT_ALL):
 if 0x140000000<=address<0x140000000+302030848:
  for page in range(address&~4095,(address+size+4095)&~4095,4096):
   if not any(lo<=page<=hi for lo,hi,_ in uc.mem_regions()):_original_mem_map(page,4096,perms)
 else:_original_mem_map(address,size,perms)
uc.mem_map=_native_page_map
image=P.get_memory_mapped_image();uc.mem_map(BASE,(len(image)+4095)&~4095);uc.mem_write(BASE,image)
exports={s.name.decode():BASE+s.address for s in P.DIRECTORY_ENTRY_EXPORT.symbols}
# Detect retained R14 behavior from a compiled capability, not a release label.
modern_r14='DriTestHs32Offseason' in exports
GAME=0x140000000;HEAP=0x30000000;STACK=0x20000000;API=0x50000000;STOP=0x60000000
for a,n in [(HEAP,0x4000000),(STACK,0x100000),(API,0x10000),(STOP,0x1000)]:uc.mem_map(a,n)
raw=(CAPTURE/'section_00_00001000.bin').read_bytes()
uc.mem_map(GAME+0x0862a000,0xa000);uc.mem_write(GAME+0x0862a000,raw[0x0862a000-0x1000:0x08633da0-0x1000])
for a in [0x08051000,0x037f3800,0x037f5800]:uc.mem_map(GAME+a,0x1000)
# Selected long-running fixtures model real HeapFree separately from permanent
# SDF records and legacy test-local next_heap rewinds. Total mapped heap stays
# 64 MiB: 48 MiB records + 16 MiB native allocations when this opt-in is active.
fixture_native_heap=None
if os.environ.get('DRI_TEST_REUSE_NATIVE_HEAP')=='1':
 sys.path.insert(0,str(ROOT/'Tests'))
 from runtime_heap_fixture import NativeHeapFixture,self_test as fixture_native_heap_self_test
 fixture_native_heap=NativeHeapFixture(uc,HEAP+0x3000000,0x1000000)
fixture_record_limit=HEAP+(0x3000000 if fixture_native_heap else 0x4000000)
next_heap=HEAP+0x1000;next_api=API;handlers={};tls={};records={};refs={};tables={}
def wq(a,v):uc.mem_write(a,struct.pack('<Q',v))
def rq(a):return struct.unpack('<Q',uc.mem_read(a,8))[0]
def wi(a,v):uc.mem_write(a,struct.pack('<I',v&0xffffffff))
def ri(a):return struct.unpack('<I',uc.mem_read(a,4))[0]
def alloc(n):
 global next_heap
 a=next_heap;next_heap=(a+n+15)&~15;assert next_heap<fixture_record_limit;return a
def api(fn,address=None):
 global next_api
 if address is None:address=next_api;next_api+=0x10
 
 if GAME<=address<GAME+302030848:uc.mem_map(address,1)
 handlers[address]=fn;uc.mem_write(address,b'\xc3');return address
def reg(n):return uc.reg_read(globals()['UC_X86_REG_'+n])
def args():return reg('RCX'),reg('RDX'),reg('R8'),reg('R9')
def incref(r):
 if r:refs[r]+=1
def decref(r):
 if r:refs[r]-=1;assert refs[r]>=1,('premature release',r,records[r]['name'],refs[r])
def native_ref():
 t,out,row,f=args();rec=tables[t][row]
 if f>=0x100000:value=records[rec]['ref'].get(f-0x100000,0)
 else:value=records[rec]['array'][f]
 incref(value);wq(out,value);return out
def native_scalar():
 t,row,f,_=args();return records[tables[t][row]]['scalar'].get(f-0x100000,0)
def query_array():
 r,out,tag,_=args();assert tag==0x41525259
 assert records[r]['array'] is not None
 incref(r);wq(out,r);return out
VT=alloc(0x200);TABLEVT=alloc(0x200)
wq(VT,api(lambda:(incref(reg('RCX')) or 0)));wq(VT+8,api(lambda:(decref(reg('RCX')) or 0)))
wq(VT+0x38,api(lambda:0xabcdef));wq(VT+0x28,api(query_array))
wq(TABLEVT+0xf8,api(native_scalar));wq(TABLEVT+0x100,api(native_scalar));wq(TABLEVT+0x108,api(native_scalar));wq(TABLEVT+0x128,api(native_ref))
def record(name,scalar=None,ref=None,array=None,row_id=None):
 r=alloc(0x30);t=alloc(0x20);idx=row_id if row_id is not None else len(records)
 wq(r,VT);wq(r+0x10,t);wi(r+0x18,idx);wq(t,TABLEVT)
 records[r]={'name':name,'scalar':scalar or {},'ref':ref or {},'array':array};tables[t]={idx:r};refs[r]=1
 return r
api(lambda:0x100000+reg('RDX'),GAME+0x08055d00)
api(lambda:len(records[reg('RCX')]['array']),GAME+0x037f40a0)
api(lambda:1,GAME+0x037f5e40)
def fixture_c_string(pointer,max_bytes=65536):
 # Model C strings through the first NUL without reading beyond the terminator.
 # The finite bound catches a malformed fixture pointer/string; memory faults
 # and missing terminators remain visible failures, never empty-string fallbacks.
 value=bytearray()
 for offset in range(max_bytes):
  char=bytes(uc.mem_read(pointer+offset,1))[0]
  if not char:return bytes(value)
  value.append(char)
 raise AssertionError('Unterminated fixture C string within '+str(max_bytes)+' bytes')
def win_import(name):
 if name in ['AcquireSRWLockExclusive','ReleaseSRWLockExclusive']:return 0
 if name=='TlsGetValue':return tls.get(reg('RCX'),0)
 if name=='TlsSetValue':tls[reg('RCX')]=reg('RDX');return 1
 if name=='GetProcessHeap':return 123
 if name=='HeapAlloc':
  if fixture_native_heap:return fixture_native_heap.allocate(reg('R8'),reg('RDX'))
  size=reg('R8');pointer=alloc(size)
  if reg('RDX')&8:uc.mem_write(pointer,b'\0'*size) # HEAP_ZERO_MEMORY, including reused fixture arenas.
  return pointer
 if name=='HeapFree':return fixture_native_heap.release(reg('R8')) if fixture_native_heap else 1
 if name=='GetCurrentThreadId':return 42
 if name in ['memset']:
  dest,val,n,_=args();uc.mem_write(dest,bytes([val&255])*n);return dest
 if name in ['memcpy','memmove']:
  dest,src,n,_=args();uc.mem_write(dest,bytes(uc.mem_read(src,n)));return dest
 if name=='memcmp':
  a,b,n,_=args();aa=bytes(uc.mem_read(a,n));bb=bytes(uc.mem_read(b,n));return (aa>bb)-(aa<bb)
 if name=='strcmp':
  a,b,_,_=args();aa=fixture_c_string(a);bb=fixture_c_string(b);return (aa>bb)-(aa<bb)
 if name=='strncmp':
  a,b,n,_=args();aa=bytes(uc.mem_read(a,n)).split(b'\0')[0];bb=bytes(uc.mem_read(b,n)).split(b'\0')[0];return (aa>bb)-(aa<bb)
 if name=='__C_specific_handler':raise AssertionError('Unexpected SEH handler')
 raise AssertionError('Unexpected Windows/CRT service '+name)
for desc in P.DIRECTORY_ENTRY_IMPORT:
 for item in desc.imports:
  name=item.name.decode() if item.name else str(item.ordinal)
  addr=api(lambda name=name:win_import(name));wq(item.address,addr)
recent=deque(maxlen=12)
def on_code(u,address,size,data):
 recent.append(hex(address))
 if address==STOP:u.emu_stop();return
 if address in handlers:
  result=handlers[address]()
  rsp=reg('RSP');ret=rq(rsp);u.reg_write(UC_X86_REG_RAX,result or 0)
  u.reg_write(UC_X86_REG_RSP,rsp+8);u.reg_write(UC_X86_REG_RIP,ret)
uc.hook_add(unicorn.UC_HOOK_CODE,on_code)
# R8 performs a bounded full-market pass across 138 schools. Measured valid
# passes exceed 10M instructions; keep a finite 50M cap, legacy controls at 2M,
# and any explicitly requested instruction limit authoritative.
def call(address,*values):
 rsp=STACK+0xf0000-8;wq(rsp,STOP);uc.reg_write(UC_X86_REG_RSP,rsp)
 for name,value in zip(['RCX','RDX','R8','R9'],values):uc.reg_write(globals()['UC_X86_REG_'+name],value)
 try:uc.emu_start(address,STOP+1,count=int(os.environ.get('DRI_TEST_INSTRUCTION_LIMIT','50000000' if (b'DRI_PORTAL_TEST28R8' in image or (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))))) else '2000000')))
 except unicorn.UcError:
  print('Fixture fault RIP',hex(reg('RIP')),'recent',list(recent));raise
 assert reg('RIP')==STOP,('did not return',hex(reg('RIP')))
 return reg('RAX')&0xffffffff
prefix=bytes.fromhex('9c5051525355565741504151415241534154415541564157')
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
hooks=[];at=0
while (at:=image.find(prefix,at))>=0:
 instructions=list(cs.disasm(image[at:at+70],BASE+at));hooks.append(next(i.operands[0].imm for i in instructions if i.mnemonic=='call'));at+=1
assert len(hooks)==(int(os.environ.get('DRI_TEST_EXPECTED_SAVED_HOOK_COUNT','30')) if modern_r14 else (30 if (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))))) else 29 if (b'DRI_PORTAL_TEST28R8' in image or (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))))) else 28 if (b'DRI_PORTAL_TEST28R4' in image or b'DRI_PORTAL_TEST28R5' in image or b'DRI_PORTAL_TEST28R6' in image or b'DRI_PORTAL_TEST28R7' in image or (b'DRI_PORTAL_TEST28R8' in image or (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))))))) else 27 if b'DRI_PORTAL_TEST28' in image else (25 if any(tag in image for tag in [b'DRI_PORTAL_TEST26',b'DRI_PORTAL_TEST27']) else 24)))
start,protect,admit=hooks[:3]
season=record('season',{2:2026,4:4,5:1})
tuning=record('tunables',{0x3c:5,0x3b:3,0x39:3,0x38:1,0x36:3})
TEST_TEAM_ID=2  # Ordinary school; academy IDs 0, 7 and 55 are policy test cases.
team=record('team',{383:TEST_TEAM_ID});team_ref=alloc(8);wq(team_ref,team)
board=record('board');arr=record('board targets',array=[]);records[team]['ref'][224]=board;records[board]['ref'][0]=arr
eval_ptr=alloc(0xc00);wq(eval_ptr+0xd0,tuning)
for p in range(22):wi(eval_ptr+0x9a4+p*4,1)
wi(eval_ptr+0x9a4+14*4,2)
need=alloc(22*4)
config=record('depth config');offense=record('offensive config');defense=record('defensive config')
records[team]['ref'][64]=config;records[config]['ref'].update({0:defense,5:offense})
field_ids=[5,2,1,9,8,4,3,0,6,7,3,6,1,4,5,7,0,2,8,1,6,4]
ideals=[3,5,0,9,3,3,3,3,3,3,4,4,6,3,3,3,6,3,3,2,2,0]
ideal_entries=[]
for pos,(field,ideal) in enumerate(zip(field_ids,ideals)):
 entry=record('ideal'+str(pos),{2:ideal});ideal_entries.append(entry)
 group=offense if pos<10 else (defense if pos<19 else config)
 records[group]['ref'][field]=entry
roster=record('roster',array=[]);records[team]['ref'][239]=roster
# Test15 needs the same complete future-roster sources as real recruiting.
hs_fixture_committed=record('HS fixture committed players',array=[])
hs_fixture_class=record('HS fixture recruits',array=[])
hs_fixture_portal=record('HS fixture portal recruits',array=[])
hs_fixture_departures=record('HS fixture confirmed departures',array=[])
hs_fixture_leaving=record('HS fixture leaving evaluator',ref={7:hs_fixture_departures})
hs_fixture_eval=record('HS fixture franchise evaluator',ref={13:hs_fixture_leaving})
hs_fixture_franchise=record('HS fixture franchise',ref={9:hs_fixture_eval,39:hs_fixture_class,44:season,46:hs_fixture_portal})
hs_fixture_context=record('HS fixture recruiting context',ref={6:hs_fixture_franchise})
wq(eval_ptr+0x10,rq(hs_fixture_context+0x10));wi(eval_ptr+0x18,ri(hs_fixture_context+0x18));wq(eval_ptr+0x30,0xabcdef)
records[team]['ref'][33]=hs_fixture_committed

def fill_hs_position(pos,count):
 records[roster]['array']=[record('HS fixture returner '+str(pos)+'/'+str(j),{3:pos,191:65,231:1,242:1,261:TEST_TEAM_ID}) for j in range(count)]

def call_start():
 # These HS scenarios replace the complete saved class with their declared board.
 # Published Signed players are represented in Team.CommittedPlayers; a mere
 # uncommitted target never joins that array or the projected roster.
 recruits=[records[t]['ref'][10] for t in records[arr]['array'] if records[t]['ref'].get(10)]
 records[hs_fixture_class]['array']=recruits
 published=[]
 for r in recruits:
  top=records[records[r]['ref'][13]]['array']
  if records[r]['scalar'].get(9)==6 and top and records[top[0]]['scalar'][0]==TEST_TEAM_ID:
   published.append(records[r]['ref'][5])
 records[hs_fixture_committed]['array']=published
 return call(start,frame)


def recruit(rid,pos,team_rank,inf,stage=1):
 player=record('player'+str(rid),{3:pos,191:65,231:1,242:0,261:255})
 entries=[]
 for j in range(max(2,team_rank)):
  tid=TEST_TEAM_ID if j+1==team_rank else j+10
  entries.append(record('top school',{0:tid,1:inf if tid==TEST_TEAM_ID else 100-j}))
 top=record('top list',array=entries)
 r=record('recruit'+str(rid),{9:stage,3:100},ref={5:player,13:top},row_id=rid)
 target=record('target'+str(rid),{12:3},ref={10:r},row_id=rid+100)
 return r,target
A,TA=recruit(10,14,1,100);B,TB=recruit(11,14,2,85);C,TC=recruit(12,14,2,90)
D,TD=recruit(13,14,2,90);E,TE=recruit(14,13,2,90)
records[arr]['array']=[TA,TB,TC]
sp=STACK+0x8000;bp=sp+0x100;frame=sp-128
wq(frame+3*8,team_ref);wq(frame+10*8,bp);wq(sp+0x58,eval_ptr)
wq(bp+0x28,team_ref);wq(bp+0x58,season);wq(bp+0x180,need)
uc.mem_write(sp+0x42,b'\0');wq(sp+0x50,3);wi(bp+0x30,3)
vector=alloc(0x60);wq(sp+0x78,vector);wq(bp-0x80,vector+0x30)
candidate=alloc(0x30);wq(candidate,D);wq(frame+11*8,candidate)
results=[]
def configure(mode):call(exports['DriTestConfigure'],GAME,mode,7)
def annual_class_slots():
 if 'DriTest31ClassAnnual' not in exports:return None
 target=alloc(46*4)
 assert call(exports['DriTest31ClassAnnual'],team,eval_ptr,target)==1
 return list(struct.unpack('<46i',uc.mem_read(target,46*4)))[1:23]
def check_protect(r,t,expected):
 wq(bp-0x58,r);wq(bp-0x38,t);uc.mem_write(sp+0x44,b'\1');call(protect,frame)
 assert bytes(uc.mem_read(sp+0x44,1))==bytes([expected]),records[r]['name']
def check_admit(outgoing,expected):
 wq(vector,outgoing);assert call(admit,frame)==expected
# Execute the ACTUAL captured native removal function. Its SDF storage,
# cache notification, hour refund, and visit services are controlled fixtures.
def map_game(rva,size=0x1000):
 a=GAME+(rva&~0xfff);end=(GAME+rva+size+0xfff)&~0xfff
 for page in range(a,end,0x1000):
  if not any(lo<=page<=hi for lo,hi,_ in uc.mem_regions()):uc.mem_map(page,0x1000)
def game_api(rva,fn):map_game(rva,1);api(fn,GAME+rva)
# Test31 production uses the native school score cache. Most legacy callback
# fixtures model this service; the dedicated Test31 native-context regression
# replaces these handlers with captured initializer/fill instructions.
def fixture_school_cache():
 cache=reg('RCX')
 for off,inline,count in [(0,0x18,35),(0xa8,0xc0,35),(0x150,0x168,43)]:
  wq(cache+off,cache+inline);wq(cache+off+8,cache+inline+count*4);wq(cache+off+16,cache+inline+count*4)
 return cache
game_api(0x08142630,fixture_school_cache)
game_api(0x0815c5e0,lambda:0)
# Pair planning rechecks the same native offer/dealbreaker service used by NSD.
# Storage fixtures default to eligible; specific tests override this service.
def fixture_eligibility():
 # Frozen pre-R9 negative controls deliberately retain the old call ABI.
 if (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))))):assert reg('RCX')==manager,'wrong eligibility receiver'
 if (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))):
  reason=reg('R9');assert reason and bytes(uc.mem_read(reason+15,1))==b'\x0f' and rq(reason+16)==0,'missing native diagnostic string'
 return 1
game_api(0x0816b130,fixture_eligibility)
fixture_hs_scorer=alloc(0x500);wq(eval_ptr+0xf8,fixture_hs_scorer)
def fixture_hs_score():
 scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and selection==0 and portal==0
 cand=rq(reg('RSP')+0x30);wi(cand+40,300);return 0
game_api(0x08185840,fixture_hs_score)

map_game(0x081a7ab0,0x1000)
uc.mem_write(GAME+0x081a7ab0,raw[0x081a7ab0-0x1000:0x081a8432-0x1000])
manager=alloc(0xc00);wq(eval_ptr+0xc8,manager)
for off,value in [(0x80,0xabc001),(0xa0,0xabc002),(0x180,0xabc003),(0x1b0,0xabc004)]:wq(manager+off,value)
game_api(0x08059c00,lambda:int(reg('RDX') in (0xabc001,0xabc002)))
native_events=[]
def get_board():
 mgr,out,tr,_=args();assert mgr==manager and rq(tr)==team
 incref(arr);wq(out,arr);return out
def set_scalar():
 t,row,f,value=args();records[tables[t][row]]['scalar'][f-0x100000]=value&0xffffffff;return 0
def set_ref():
 t,row,f,value=args();records[tables[t][row]]['ref'][f-0x100000]=value;return 0
def clear_ref():
 rec,f,p,_=args();records[rec]['ref'][f-0x100000]=rq(p);return 0
def erase_array():
 rec,index,_,_=args();assert rec==arr
 native_events.append(('erase_target',records[arr]['array'][index]));records[arr]['array'].pop(index);return 0
def cache_cleanup():
 mgr,tr,cand,_=args();assert mgr==manager and rq(tr)==team
 native_events.append(('cache_cleanup',rq(cand)));return 0
def visit_cancel():native_events.append(('visit_cancel',rq(reg('R8'))));return 0
def refund_hours():native_events.append(('refund_hours',reg('R8')));return 0
game_api(0x0818af10,get_board)
game_api(0x08103c20,lambda:bool(records[rq(reg('RCX')+8)]['ref'].get(407)))
game_api(0x08059580,lambda:0)
game_api(0x0805e4d0,clear_ref)
game_api(0x08052300,erase_array)
game_api(0x081ae980,cache_cleanup)
game_api(0x0844cab0,visit_cancel)
game_api(0x082e6720,refund_hours)
wq(TABLEVT+0x148,api(set_scalar));wq(TABLEVT+0x158,api(set_ref))
records[team]['ref'].pop(407,None)
Q1,TQ1=recruit(30,0,1,100,stage=5)
Q2,TQ2=recruit(31,0,1,100,stage=1) # native-protected leader
Q3,TQ3=recruit(32,0,2,90,stage=2)  # close challenger
Q4,TQ4=recruit(33,0,2,90,stage=5)  # committed elsewhere; also clear stale QB targets
NON,TNON=recruit(34,3,2,90,stage=1)
QNEW,TQNEW=recruit(35,0,2,90,stage=1)
visit=record('scheduled QB visit')
def reset_qb_board():
 records[arr]['array']=[TQ1,TQ2,TQ3,TQ4,TNON]
 for rr,tt in [(Q1,TQ1),(Q2,TQ2),(Q3,TQ3),(Q4,TQ4),(NON,TNON),(QNEW,TQNEW)]:
  records[tt]['ref'][10]=rr;records[tt]['scalar'][12]=3;records[rr]['scalar'][14]=4
 records[Q1]['scalar'][9]=5
 for rr,stage in [(Q2,1),(Q3,2),(Q4,5),(NON,1),(QNEW,1)]:records[rr]['scalar'][9]=stage
 records[TQ2]['ref'][11]=visit;records[TQ2]['scalar'][2]=25
 wq(candidate,QNEW);native_events.clear()
fill_hs_position(0,ideals[0])
reset_qb_board();configure(0);call_start()
assert len(records[arr]['array'])==5 and not native_events
assert records[Q2]['scalar'][14]==4
check_admit(E,0)
results.append('QB diagnostic mode leaves boards, offers, visits and admissions unchanged')

configure(1);call_start()
assert records[arr]['array']==[TQ1,TNON]
assert records[TQ1]['ref'][10]==Q1 and records[Q1]['scalar'][9]==5
for rr,tt in [(Q2,TQ2),(Q3,TQ3),(Q4,TQ4)]:
 assert not records[tt]['ref'][10] and records[rr]['scalar'][14]==3
assert records[TQ2]['ref'][11]==0
assert ('visit_cancel',TQ2) in native_events and ('refund_hours',25) in native_events
assert [x[1] for x in native_events if x[0]=='cache_cleanup']==[Q2,Q3,Q4]
check_admit(E,1);check_admit(Q2,1)
results.append('actual captured native removal clears leader, challenger and stale rival QB targets; reduces offer totals; cancels visit; refunds recorded hours; leaves own secured commitment and non-QB target intact')
results.append('closed QB position blocks new candidates even for a same-position replacement')

# Losing the commitment reopens the slot: no sticky team state persists.
records[Q1]['scalar'][9]=1;call_start();check_admit(E,0)
records[arr]['array']=[TQ1,TQNEW];call_start()
check_protect(QNEW,TQNEW,0)
QEXTRA,TQEXTRA=recruit(36,0,2,90,stage=1);wq(candidate,QEXTRA)
check_admit(E,0)
results.append('decommitment reopens QB admissions; two open QBs can coexist and a close challenger fits the protection allowance')

# Several commitments already in a loaded save are retained, never undone.
reset_qb_board();records[Q1]['scalar'][9]=5;records[Q2]['scalar'][9]=6
call_start()
assert records[arr]['array']==[TQ1,TQ2,TNON]
assert records[Q1]['scalar'][9]==5 and records[Q2]['scalar'][9]==6
results.append('existing hard and signed QB commitments both preserved; surplus open pursuits removed')

# User schools remain unchanged. Shared alternate passes enforce CPU HS guards.
reset_qb_board();records[team]['ref'][407]=record('QB user character');call_start()
assert len(records[arr]['array'])==5 and not native_events
records[team]['ref'].pop(407)
uc.mem_write(sp+0x42,b'\1');call_start()
assert records[arr]['array']==[TQ1,TNON]
assert [x[1] for x in native_events if x[0]=='cache_cleanup']==[Q2,Q3,Q4]
reset_qb_board();uc.mem_write(sp+0x42,b'\0');records[season]['scalar'][5]=2;call_start()
assert len(records[arr]['array'])==5 and not native_events
records[season]['scalar'][5]=1
results.append('user teams excluded; shared alternate pass enforces CPU HS guards; out-of-scope season phases unchanged')

# A missing native service disables behavior before a destructive call.
wq(eval_ptr+0xc8,0);configure(1);call_start()
assert len(records[arr]['array'])==5 and not native_events
check_admit(E,0);wq(eval_ptr+0xc8,manager)
results.append('missing recruiting manager disables policy before any native mutation')
# All positions: actual compiled policy, native own-commit predicate and removals.
configure(1)
for pos in range(19):
 fill_hs_position(pos,ideals[pos])
 ideal=ideals[pos];normal=annual_class_slots()
 limit=normal[pos] if normal is not None else (0 if pos==2 and ideal==0 else max(1,(ideal+1)//3))
 owned=[recruit(1000+pos*20+j,pos,1,100,stage=[5,6][j%2]) for j in range(limit)]
 extra=[recruit(1000+pos*20+10+j,pos,2,90,stage=st) for j,st in enumerate([1,2,4,5])]
 pairs=owned+extra
 records[arr]['array']=[tt for rr,tt in pairs]
 for rr,tt in pairs:records[rr]['scalar'][14]=3
 call_start()
 assert records[arr]['array']==[tt for rr,tt in owned],('position cleanup',pos)
 for rr,tt in extra:assert records[rr]['scalar'][14]==2
 inc,tinc=recruit(1000+pos*20+19,pos,2,90);wq(candidate,inc)
 check_admit(E,1)
 if limit:
  records[owned[0][0]]['scalar'][9]=1
  call_start();check_admit(E,0 if normal is not None else int(limit-1>=2 and any(tag in (OUTPUT/'retention_fixture.dll').read_bytes() for tag in [b'DRI_PORTAL_TEST26',b'DRI_PORTAL_TEST27',b'DRI_PORTAL_TEST28'])))
results.append('all 19 offensive/defensive positions: configured annual limits, hard/signed preservation, four target types removed with offer accounting, and reopening below annual intake; Test31 normal intake does not spend shared allowances; zero-ideal FB closes')

# Underfilled WR can pursue several candidates, including cross-position admissions.
records[roster]['array']=[]
records[arr]['array']=[]
wr_pairs=[recruit(1500+j,3,2,90) for j in range(6)]
records[arr]['array']=[tt for rr,tt in wr_pairs]
call_start()
wq(candidate,recruit(1510,3,2,90)[0]);check_admit(E,0)
assert len(records[arr]['array'])==6
results.append('multiple candidates remain allowed before annual limit; Test01 concurrent-pursuit admission block removed')

# Most competitive challenger still selected from the same native protection budget.
# A filled MLB room isolates that budget from Test15's genuine-vacancy allowance.
fill_hs_position(14,ideals[14])
protection_year=records[season]['scalar'][2]
if 'DriTest31ClassAnnual' in exports:
 for year in range(protection_year,protection_year+20):
  records[season]['scalar'][2]=year
  slots=annual_class_slots()
  if slots[14]==1 and slots[3]>=2:break
 else:raise AssertionError('No suitable annual primary/backup fixture year')
records[arr]['array']=[TA,TB,TC];records[A]['scalar'][9]=1
call_start();check_protect(B,TB,1);check_protect(C,TC,0)
records[A]['scalar'][9]=5;records[A]['scalar'][14]=3
# MLB ideal3 => commitment closes; use WR ideal9 to test partial ownership independently.
for rr in [A,B,C]:records[records[rr]['ref'][5]]['scalar'][3]=3
call_start();check_protect(B,TB,0);check_protect(C,TC,0)
records[season]['scalar'][2]=protection_year
results.append('80-percent challenger protection preserved while opening remains; partial commitment count does not close multi-slot WR')

# Specialist accounting, graduation, role coverage and missing-role fourth slot.
kret=record('returning K',{3:19,191:65,231:1,242:1,261:TEST_TEAM_ID});pret=record('returning P',{3:20,191:65,231:1,242:2,261:TEST_TEAM_ID})
ksenior=record('senior K',{3:19,191:65,231:1,242:3,261:TEST_TEAM_ID})
kret2=record('second returning K',{3:19,191:65,231:1,242:0,261:TEST_TEAM_ID});pret2=record('second returning P',{3:20,191:65,231:1,242:0,261:TEST_TEAM_ID})
kret3=record('third returning K',{3:19,191:65,231:1,242:2,261:TEST_TEAM_ID})
def specialists(players,kcommits=0,pcommits=0):
 records[roster]['array']=players
 pairs=[recruit(1700+j,19,1,100,stage=5) for j in range(kcommits)]
 pairs += [recruit(1710+j,20,1,100,stage=5) for j in range(pcommits)]
 kr,kt=recruit(1720,19,2,90);pr,pt=recruit(1721,20,2,90)
 pairs.extend([(kr,kt),(pr,pt)])
 records[arr]['array']=[tt for rr,tt in pairs]
 for rr,tt in pairs:records[rr]['scalar'][14]=3
 call_start()
 ko=kt in records[arr]['array'];po=pt in records[arr]['array']
 wq(candidate,recruit(1730,19,2,90)[0]);check_admit(E,0 if ko else 1)
 wq(candidate,recruit(1731,20,2,90)[0]);check_admit(E,0 if po else 1)
 return ko,po
normal=annual_class_slots()
if normal is not None:
 expected=(normal[19]>0,normal[20]>0)
 assert specialists([kret,pret])==expected
 assert specialists([kret,pret],kcommits=1)==(False,expected[1])
 assert specialists([kret,pret,ksenior])==expected
 assert specialists([kret,kret2,kret3])==expected
 assert specialists([kret,kret2,kret3],pcommits=1)==(expected[0],False)
 assert specialists([kret,kret2])==expected
 assert specialists([kret,kret2,pret,pret2])==expected
 results.append('Test31 specialist cadence is stable through returning-room differences; actual secured specialist arrivals fulfill their annual place; native removal/admission agree')
else:
 assert specialists([kret,pret])==(True,True)
 assert specialists([kret,pret],kcommits=1)==(False,True)
 assert specialists([kret,pret,ksenior])==(True,True)
 assert specialists([kret,kret2,kret3])==(False,True)
 assert specialists([kret,kret2,kret3],pcommits=1)==(False,True)
 assert specialists([kret,kret2])==(False,True)
 assert specialists([kret,kret2,pret,pret2])==(False,False)
 results.append('Legacy specialist physical-room accounting preserved')
records[roster]['array']=[]

ls,lst=recruit(1800,21,2,90);records[ls]['scalar'][14]=1
records[arr]['array']=[lst];call_start();assert not records[arr]['array']
records[ideal_entries[21]]['scalar'][2]=3
ls2,lst2=recruit(1801,21,2,90);records[arr]['array']=[lst2];call_start()
assert records[arr]['array']==([] if 'DriTest31ClassAnnual' in exports else [lst2])
records[ideal_entries[21]]['scalar'][2]=0
results.append('LS respects zero ideal; Test31 has no normal LS source quota while legacy configurations retain their former positive-ideal behavior')

# Missing or implausible config disables before mutation; signatures separately guarded.
configure(1);reset_qb_board();records[config]['ref'].pop(5)
call_start();assert len(records[arr]['array'])==5 and not native_events
check_admit(E,0);records[config]['ref'][5]=offense
configure(1);records[ideal_entries[0]]['scalar'][2]=100;call_start()
assert len(records[arr]['array'])==5 and not native_events
records[ideal_entries[0]]['scalar'][2]=3
results.append('missing scheme reference and invalid ideal fail before mutation')
call(exports['DriTestOwnershipReset'])
assert all(x==1 for x in refs.values()),[(records[r]['name'],x) for r,x in refs.items() if x!=1]
results.append('all SDF fixture reference counts balanced')
out={'passed':results,'method':'compiled DLL callbacks and captured native helper machine code; mocked SDF storage and Windows services','not_proven':['actual Windows loading','real game record lifetimes','game stability','recruiting outcomes']}
(OUTPUT/'runtime_fixture_results.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
