"""First-preseason refresh regression. Native scoring is a controlled evidence
producer; actual compiled hooks and captured aging/setter/getter code execute.
This is not a live game or a full native projected-draft simulation."""
from pathlib import Path
common=Path(__file__).with_name('pp28_runtime_test.py').read_text()
exec(compile(common.split("team = record('PP team'")[0],__file__,'exec'))
wq(TABLEVT+0xf8,api(native_scalar))
for page in [0x08406da0,0x08421da0,0x08422da0,0x0860eda0,0x08610da0]:
 map_game(page);uc.mem_write(GAME+page,raw[page-0x1000:page])
configure()
addresses=alloc(32);call(exports['DriTestPPAddresses'],addresses)
hook_age,hook_publish=[rq(addresses+8*i) for i in range(2)]
call(exports['DriTestPPRefreshAddresses'],addresses)
hook_getter,trampoline=[rq(addresses+8*i) for i in range(2)]
uc.mem_write(GAME+0x084075b0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook_getter)+b'\x90')
uc.mem_write(GAME+0x08422835,b'\x48\x83\xc4\x28\xc3');uc.mem_write(GAME+0x08422f4c,b'\x48\x83\xc4\x28\xc3')
fields=[30,31,33,32,28,25,27,24,26,29]
tracks=[record('tracking'+str(i),{k:12 for k in fields}) for i in range(138)]
teams=[record('school'+str(i),{383:i,368:0},ref={165:tracks[i]}) for i in range(138)]
team_array=record('all schools',array=teams.copy())
season=record('actual Test28 PRE calendar',{0:2026,2:2026,3:0,4:0,5:0,6:0,24:1})
picks=[record('history'+str(i),{0:age,1:i%7,4:i%10},ref={3:teams[i]}) for i,age in enumerate([2,3,4])]
history=record('active draft history',array=picks.copy())
franchise=record('franchise',{28:3118973581},ref={44:season,8:history})
ctx_record=record('grade eval',ref={2:franchise})
context=alloc(0x1800);wq(context+0x10,rq(ctx_record+0x10));wi(context+0x18,ri(ctx_record+0x18));wq(context+0x30,0xabcdef)
wq(context+0x80,team_array);wq(context+0x88,record('tuning'))
for i,c in enumerate([5,10,15,25,35,45,55,65,75,90,105,120,250]):wi(context+0x10ec+4*i,c)
saved=json.loads(Path(__file__).with_name('pp28_saved_evidence.json').read_text())
fixtures=list(saved['groups'].values());vector=alloc(138*0xb0)
for i in range(138):
 wq(vector+i*0xb0,tracks[i]);wq(vector+i*0xb0+8,teams[i])
 for g in range(10):
  h,p,c=fixtures[g]['evidence'][i]
  for off,val in [(0x34,h),(0x5c,p),(0x84,c)]:wi(vector+i*0xb0+off+4*g,val)

def imm(prefix,value):return bytes.fromhex(prefix)+struct.pack('<Q',value)
def mov_frame(offset,value):return imm('48b8',value)+b'\x48\x89\x45'+bytes([offset&255])
def invoke(address):return imm('48b8',address)+b'\xff\xd0'
adapters={}
def mid_invoke(address):
 # Mid-function hooks expect aligned writable outgoing-call home space,
 # not a function-entry return address in that space.
 if address not in adapters:
  code=b'\x48\x83\xec\x28\xff\x25\0\0\0\0'+struct.pack('<Q',address)
  adapters[address]=alloc(len(code));uc.mem_write(adapters[address],code)
 return invoke(adapters[address])
def driver(limit=1380,repeat_age=False):
 # Windows nonvolatile preservation, aligned calls, native RBP-relative frame.
 b=bytes.fromhex('5355565741544155415641574881ec08020000488dac2400010000')
 b+=imm('49bd',context)+mov_frame(0x6f,history)+mov_frame(0x5f,0x123456)
 b+=mid_invoke(hook_age)
 if repeat_age:b+=mid_invoke(hook_age)
 # Reentrant notification-style refresh must return immediately, not deadlock.
 b+=imm('48b9',context)+invoke(exports['DriTestPPRefresh'])
 # A different evaluator on this same notification thread must not acquire
 # the already-held refresh lock or consume another dynasty's state.
 b+=imm('48b9',0x12345678)+invoke(exports['DriTestPPRefresh'])
 b+=mov_frame(-0x59,vector)+mov_frame(-0x51,vector+138*0xb0)+mov_frame(-0x69,0xabcdef)+mov_frame(-0x79,0)
 b+=imm('48bb',138)
 for n in range(limit):
  g,i=divmod(n,138)
  b+=imm('49bc',g)+imm('49bf',i)+imm('48be',12)+mov_frame(-0x71,tracks[i])+mid_invoke(hook_publish)
 b+=bytes.fromhex('4881c408020000415f415e415d415c5f5e5d5bc3')
 address=alloc(len(b));uc.mem_write(address,b)
 uc.mem_write(GAME+0x084225f0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',address))
 return address
driver_address=driver()
visits={'scorer':0,'native_age':0}
def count_visits(u,address,size,data):
 if address==GAME+0x084225f0:visits['scorer']+=1
 if address==GAME+0x083fe420:visits['native_age']+=1
uc.hook_add(unicorn.UC_HOOK_CODE,count_visits)
def run(address,*values):
 rsp=STACK+0xf0000-8;wq(rsp,STOP);uc.reg_write(UC_X86_REG_RSP,rsp)
 for name,value in zip(['RCX','RDX','R8','R9'],values):uc.reg_write(globals()['UC_X86_REG_'+name],value)
 if len(values)>4:wq(rsp+0x28,values[4])
 try:uc.emu_start(address,STOP+1,count=20000000)
 except unicorn.UcError:
  print('Fault',hex(reg('RIP')),list(recent));raise
 assert reg('RIP')==STOP,hex(reg('RIP'))
 return reg('RAX')&0xffffffff
team_holder=alloc(8);wq(team_holder,teams[0]);player_holder=alloc(8);wq(player_holder,0)
before_ages=[records[p]['scalar'][0] for p in picks]
assert run(GAME+0x084075b0,context,team_holder,6,0,player_holder)==fixtures[0]['expected'][0]
assert visits=={'scorer':1,'native_age':0},visits
assert [records[p]['scalar'][0] for p in picks]==before_ages and not removed
for i in range(138):
 for g in range(10):assert records[tracks[i]]['scalar'][fields[g]]==fixtures[g]['expected'][i],(i,g)
assert run(exports['DriTestPPRefresh'],context)==1 and visits['scorer']==1
for g in range(10):assert run(GAME+0x084075b0,context,team_holder,6,g,player_holder)==fixtures[g]['expected'][0]
assert visits['scorer']==1
results=['First grade query bootstraps exactly once through actual group entry/trampoline; all 1380 grade cells match model evidence; captured setter/getter agree',
 'Compiled pre-aging hook skips actual native aging for refresh only; history identities/ages unchanged; reentry and repeated queries do not rerun']
call(exports['DriTestPPRefreshReset'])
assert run(exports['DriTestPPRefresh'],context)==1 and visits['scorer']==2
assert [records[p]['scalar'][0] for p in picks]==before_ages
results.append('New process/session-style reset can republish without aging history')

# Once refresh scope ends, the exact same hook still executes native aging.
call(exports['DriTestPPRefreshReset'])
for field,value in [(24,0),(3,1),(6,1),(4,1)]:
 old=records[season]['scalar'][field];records[season]['scalar'][field]=value
 assert run(exports['DriTestPPRefresh'],context)==0 and visits['scorer']==2
 records[season]['scalar'][field]=old
configure(0);assert run(exports['DriTestPPRefresh'],context)==0;configure()
assert visits['scorer']==2
results.append('Pre-league, regular season, later dynasty year, nonzero preseason week and diagnostic mode do not trigger refresh')

records[team_array]['array'].pop()
assert run(exports['DriTestPPRefresh'],context)==0 and call(exports['DriTestQualityFault'])==1 and visits['scorer']==2
records[team_array]['array'].append(teams[-1]);configure()
wi(context+0x10ec,-1)
assert run(exports['DriTestPPRefresh'],context)==0 and call(exports['DriTestQualityFault'])==1 and visits['scorer']==2
wi(context+0x10ec,5);configure()
driver(limit=1379)
assert run(exports['DriTestPPRefresh'],context)==0 and call(exports['DriTestQualityFault'])==1
assert [records[p]['scalar'][0] for p in picks]==before_ages
configure();driver(repeat_age=True)
assert run(exports['DriTestPPRefresh'],context)==0 and call(exports['DriTestQualityFault'])==1
assert [records[p]['scalar'][0] for p in picks]==before_ages and visits['native_age']==0
results.append('Incomplete team set fails before scoring; missing publication and repeated aging boundary fail closed without aging')
call(exports['DriTestPPRefreshReset']);configure()
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
results.append('Owned franchise and all temporary SDF references released after reset/failures')
report={'test':'pp28r1_nonaging_preseason_refresh','passed':results,'scorer_boundary':'controlled evidence producer; native projection algorithm unchanged','live_game':False}
(OUTPUT/'pp28_refresh_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
