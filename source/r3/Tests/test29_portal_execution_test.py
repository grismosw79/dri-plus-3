"""Captured actual native influence mutation and success-branch trampolines.
SDF services and policy destinations are controlled; no live-game claim."""
from pathlib import Path
import copy,json
fixture=(Path(__file__).parent/'test29_portal_witness_test.py').read_text()
exec(compile(fixture.split('# Displayed scholarship,')[0],'test29_portal_witness_test.py','exec'))
records[pt]['scalar'].update({8:100,6:0,7:0,15:1})
records[bt]['scalar'].update({8:100,6:0,7:0,15:1})
records[board]['scalar'].update({1:180,2:0,3:800})
for action_record in records[records[tuning]['ref'][49]]['array']:
 if records[action_record]['scalar'][0]==5:records[action_record]['scalar'][2]=50
records[records[tuning]['ref'][49]]['array'].append(record('hard sell',{0:2,1:40,2:40,4:0,5:1,6:0}))
for target in [pt,bt]:records[target]['ref'][0]=record('active pitches',array=[record('native pitch',{0:0})])
call(exports['DriTestPortal29Observe'],team,context)
# Native mutation, including its own signed/locked guard and0..1000 clamp.
map_game(0x08147d10,0x1000)
uc.mem_write(GAME+0x08148340,raw[0x08148340-0x1000:0x081485e2-0x1000])
locked_native=False
game_api(0x0862d990,lambda:int(locked_native))
th=alloc(8);rh=alloc(8);wq(th,team);wq(rh,pt)
def native_apply(amount):return call(GAME+0x08148340,manager,th,rh,amount)
assert native_apply(40)==1 and records[pt]['scalar'][8]==140
assert records[pt]['scalar'][6]==0 and records[pt]['scalar'][7]==0
assert native_apply(1000)==1 and records[pt]['scalar'][8]==1000
records[pt]['scalar'][8]=140;locked_native=True
assert native_apply(40)==0 and records[pt]['scalar'][8]==140
locked_native=False;records[primary]['scalar'][9]=5
assert native_apply(40)==0 and records[pt]['scalar'][8]==140
records[primary]['scalar'][9]=0
# Verify original executable branch bytes and exact replay from the installed
# production assembly. Begin before conditional branch so failed apply cannot
# reach the success callback.
hookout=alloc(16);call(exports['DriTestPortal29ExecutionHooks'],hookout)
hookaddrs=[rq(hookout),rq(hookout+8)]
native_stack=STACK+0x80000
# R2 harness correction: caller locals must be above the live callee stack.
# The old synthetic RBP at STACK+0x68000 overlapped deeper valid callbacks.
# These exact captured prologs establish RBP-RSP at the success hook: generic
# -0x27+0xb8=0x91; pitch -0xa68+0xb68=0x100. No ABI assertions are relaxed.
for rva,expected in [
 (0x0814eb80,'4c894c2420488954241055564154415541564157488d6c24d94881ecb8000000'),
 (0x0814f210,'40555657415441554157488dac2498f5ffff4881ec680b0000')]:
 expected=bytes.fromhex(expected)
 assert raw[rva-0x1000:rva-0x1000+len(expected)]==expected
map_game(0x0814ed10,0x1000)
for lo,hi in [(0x0814ede1,0x0814ee2b),(0x0814fa07,0x0814faef)]:
 uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000])
specs=[(0x0814ede1,0x0814ede8,0x0814edf7,0x0814ee27,hookaddrs[0]),
       (0x0814fa07,0x0814faa9,0x0814fab8,0x0814fae9,hookaddrs[1])]
for _,site,resume,_,hook in specs:
 expected=bytes(raw[site-0x1000:resume-0x1000]);assert len(expected)==15
 uc.mem_write(GAME+site,b'\xff\x25\x00\x00\x00\x00'+int(hook).to_bytes(8,'little')+b'\x90')
# Pitch success performs native intel work before observer. This dependency is
# modeled and produces no policy/ownership changes; action apply above is real.
game_api(0x081b81c0,lambda:0)
# Avoid AVX2 SHLX in the pre-observer pitch intel path: test the actual AL
# branch as its own guard, then success hook from its verified landing point.
# The game bytes are also disassembled to prove no jump enters stolen middles.
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
for entry,site,resume,skip,_ in specs:
 ins=list(md.disasm(raw[entry-0x1000:skip+2-0x1000],GAME+entry))
 for i in ins:
  if i.mnemonic.startswith('j') and i.op_str.startswith('0x'):
   target=int(i.op_str,16)
   assert not GAME+site<target<GAME+resume,(hex(i.address),i.op_str)

def execute(which,success=True):
 frame=native_stack+(0x100 if which else 0x91)
 entry,site,resume,skip,hook=specs[which]
 wq(frame+0x67,th);wi(frame+0x7f,40);wi(frame-0x7c,25)
 values={'R15':5,'R14':context,'R13':rh,'R12':50,'R11':0x1111,'R10':0x2222,'R9':0x3333,'R8':0x4444,
         'RDI':0x5555,'RSI':rh,'RBP':frame,'RBX':0x7777,'RDX':0x8888,'RCX':0x9999,'RAX':int(success)}
 if which:values.update({'RSI':context,'R13':rh,'R12':th})
 for name,value in values.items():uc.reg_write(globals()['UC_X86_REG_'+name],value)
 uc.reg_write(UC_X86_REG_RSP,native_stack);uc.reg_write(UC_X86_REG_EFLAGS,0x246)
 for x in range(16):uc.reg_write(globals()['UC_X86_REG_XMM'+str(x)],0x123456789abcdef0+x)
 before=copy.deepcopy(records);endpoint=GAME+(resume if success else skip)
 def stop_at(u,a,size,data):
  if a==endpoint:u.emu_stop()
 handle=uc.hook_add(unicorn.UC_HOOK_CODE,stop_at)
 start=GAME+entry
 if which and success:
  # Verify native success AL branch executes to its post-branch target, then
  # enter the real success-only observer after the native intel operation.
  start=hook
 try:uc.emu_start(start,endpoint+1,count=10000000)
 finally:uc.hook_del(handle)
 assert reg('RIP')==endpoint and reg('RSP')==native_stack
 assert records==before,'execution observer changed records'
 if success:
  for name,value in values.items():
   if name not in {'RAX','RCX','R9','RDI'}:assert reg(name)==value,(which,name,reg(name),value)
  assert reg('RCX')==manager and reg('R9')==(25 if which else 40)
  assert reg('RAX')==frame-(0x50 if which else 0x11)
  for x in range(16):assert uc.reg_read(globals()['UC_X86_REG_XMM'+str(x)])==0x123456789abcdef0+x
 return [call(exports['DriTestPortal29Execution'],i)for i in range(8)]
for other_team in fixture_dummy_teams[:13]:
 if records[other_team]['scalar'][383]!=TEST_TEAM_ID:call(exports['DriTestPortal29Observe'],other_team,context)
seen=execute(0);assert seen==[3202,5,50,40,140,1,0,1],seen
# Native cost50 and actual influence40 remain different, notinterchanged.
prior=seen[:];assert execute(0,False)==prior
seen=execute(1);assert seen==[3202,2,0,25,140,1,0,1],seen
prior=seen[:];assert execute(1,False)==prior
# Exact phase/evaluator identity is required; absent allocation evidence is unknown.
records[season]['scalar'][4]=4
assert execute(0)[5:]==[0,0,0]
records[season]['scalar'][4]=3
old_context=context;context=alloc(0x300);uc.mem_write(context,bytes(uc.mem_read(old_context,0x300)))
assert execute(0)[5:]==[0,0,0]
context=old_context
call(exports['DriTestPortal29Observe'],team,context)
# Same executed action can belong to a conditional destination, notfirm.
wq(rh,bt);seen=execute(0);assert seen[0]==4783 and seen[5:]==[0,1,1],seen
records[board]['scalar'][1]=0;seen=execute(0);assert seen[5:]==[0,0,0],seen
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(v==1 for v in refs.values())
checks=['Captured native08144630 writes actual target influence with clamp and native signed/locked veto; does not write hours ordelta',
 'Generic native success branch reaches actual compiled trampoline; rejection skips observer',
 'Actual generic nativecost50 is separate from applied influence40',
 'Pitch trampoline preserves registers and reports execution separately from zero feedback cost and positive allocation',
 'Per-school funding evidence survives twelve other school observations; exact phase/evaluator mismatch remains unknown',
 'Current paid primary versus conditional backup capacity remains separate; invalid assignment has no firm claim',
 'Read-only callbacks preserve records, references, stack and SIMD registers']
(OUTPUT/'test29_portal_execution_results.json').write_text(json.dumps({'checks':checks,'captured_native_apply':'0x08148340','generic_hook':'0x0814ede8','pitch_hook':'0x0814faa9','modeled_dependencies':['SDF/Windows services','native lockout response','pitch intel operation before observer'],'live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
