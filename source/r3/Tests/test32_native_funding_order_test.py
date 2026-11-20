"""Compiled HS funding + captured native two-pass dispatch and cost gates.
SDF, Windows, action eligibility and per-school hours are controlled services.
This does not run a whole weekly advance or infer live signing outcomes.
"""
from pathlib import Path
import json, os, struct
T=Path(__file__).parent
source=(T/'test31_hs_runtime_test.py').read_text()
exec(compile(source.split('# Full inherited room')[0], 'Test32 funding fixture definitions', 'exec'))
reset8();next_single_year(10)
backup,bp,bt=target31(32001,400,72)
primary,pp_,pt=target31(32002,500,80)
assert (role31(backup),role31(primary))==(2,1)
# An unrelated positive portal entry must remain in the same physical and
# permutation slot. Its distinctive metadata must not be rewritten as HS.
portal,portal_player=rr(32003,score=1000,ovr=70,stage=0,schools=((96,100),),offers=(96,),portal=True)
portal_target=next(x for x in records[board_targets(allteams[96])]['array'] if records[x]['ref'].get(10)==portal)
ff=STACK+0x55000;fb=ff+0x400;wq(ff+80,fb);wq(ff+72,context)
team_holder=alloc(8);wq(team_holder,allteams[96]);wq(fb-0x60,team_holder)
vec=alloc(216);indices=alloc(24)
wq(fb-0x50,vec);wq(fb-0x48,vec+216);wq(fb+0x58,indices);wq(fb+0x60,indices+24)
for i,(r,t,score) in enumerate([(backup,bt,300),(portal,portal_target,777),(primary,pt,300)]):
 uc.mem_write(vec+i*72,b'\0'*72);wq(vec+i*72,r);wi(vec+i*72+40,score);wq(vec+i*72+48,t);wi(vec+i*72+64,1);wq(indices+i*8,i)
portal_bytes=bytes(uc.mem_read(vec+72,72));before_native_budget=dict(records[records[allteams[96]]['ref'][224]]['scalar'])
# The full callback also checks native minimum-share affordability. Keep this
# test about ordering: native cap service grants50, already above minimum.
game_api(0x0817e7b0,lambda:50)
# Replay exact captured prefixes up to the unchanged eligibility call. The
# prefixes execute AddRef, so release that one reference after the checkpoint.
for lo,hi in [(0x08171f20,0x08171f5a),(0x08172310,0x08172343),(0x0815f2d3,0x0815f336)]:
 map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000]);uc.ctl_remove_cache(GAME+lo,GAME+hi)
dispatch_frame=STACK+0x66000;dispatch_sp=STACK+0x68000
wq(dispatch_frame-0x50,vec);wq(dispatch_frame+0x58,indices);wq(dispatch_frame-0x60,team_holder)
def dispatched(second):
 out=[];entry=0x08172310 if second else 0x08171f20;checkpoint=0x0817233e if second else 0x08171f55
 def stop_dispatch(u,address,size,data):
  if address==GAME+checkpoint:u.emu_stop()
 token=uc.hook_add(unicorn.UC_HOOK_CODE,stop_dispatch)
 try:
  for i in range(3):
   uc.reg_write(UC_X86_REG_RSP,dispatch_sp);uc.reg_write(UC_X86_REG_RBP,dispatch_frame)
   uc.reg_write(UC_X86_REG_RSI,context);uc.reg_write(UC_X86_REG_R12,team_holder);uc.reg_write(UC_X86_REG_R13,i)
   uc.emu_start(GAME+entry,GAME+checkpoint+1,count=1000)
   assert reg('RIP')==GAME+checkpoint,(hex(reg('RIP')),second,i)
   target=rq(reg('R8'));out.append(records[target]['ref'][10]);decref(target)
 finally:uc.hook_del(token)
 return out
# Old implementation: it can sort the first pass only. Demonstrate the exact
# native disagreement without changing either loop or mocking its indexing.
for i,v in enumerate([2,1,0]):wq(indices+i*8,v)
negative={'first':[records[r]['row'] if 'row' in records[r] else ri(r+0x18) for r in dispatched(False)],
          'second':[ri(r+0x18) for r in dispatched(True)]}
assert dispatched(False)==[primary,portal,backup] and dispatched(True)==[backup,portal,primary]
# Return the incoming native permutation, then use the actual compiled hook.
for i in range(3):wq(indices+i*8,i)
call(hooks[23],ff)
assert call(exports['DriTestQualityFault'])==0
assert dispatched(False)==[primary,portal,backup]
baseline=os.environ.get('DRI_TEST32_FUNDING_BASELINE')=='1'
assert dispatched(True)==([backup,portal,primary] if baseline else [primary,portal,backup]), 'second native pass still visits the backup first'
assert bytes(uc.mem_read(vec+72,72))==portal_bytes and rq(indices+8)==1
assert sum(ri(vec+i*72+40) for i in range(3))==1377
assert sorted(rq(indices+i*8) for i in range(3))==[0,1,2]
assert records[records[allteams[96]]['ref'][224]]['scalar']==before_native_budget
# Captured scholarship-action gate: even a positive allowance must reach the
# native action cost, and neither native eligibility nor school hours is waived.
allowance_sp=STACK+0x70000;target_holder=alloc(8);wq(target_holder,pt)
allowed=True;hours=True;ready=True;gate_calls=[]
def eligibility():gate_calls.append('native eligibility');return int(allowed)
def school_hours():gate_calls.append('school hours');return int(hours)
def readiness():gate_calls.append('native readiness');return int(ready)
for address,fn in [(0x0819adf0,eligibility),(0x0816b870,school_hours),(0x0815ee70,readiness)]:game_api(address,fn)
wi(context+0x1bc,5)
checkpoint=GAME+0x0815f336
def stop_gate(u,address,size,data):
 if address==checkpoint:u.emu_stop()
token=uc.hook_add(unicorn.UC_HOOK_CODE,stop_gate)
cases=[]
try:
 for amount,eligible,hours_ok,ready_ok,expected in [(0,1,1,1,0),(4,1,1,1,0),(5,1,1,1,1),(50,0,1,1,0),(50,1,0,1,0),(50,1,1,0,0)]:
  allowed,hours,ready=eligible,hours_ok,ready_ok;gate_calls=[]
  uc.reg_write(UC_X86_REG_RSP,allowance_sp);uc.reg_write(UC_X86_REG_RDI,context)
  uc.reg_write(UC_X86_REG_RBP,team_holder);uc.reg_write(UC_X86_REG_RSI,target_holder);uc.reg_write(UC_X86_REG_R14,0)
  wi(allowance_sp+0x90,amount)
  uc.emu_start(GAME+0x0815f2d3,checkpoint+1,count=1000)
  assert reg('RIP')==checkpoint and (reg('RDI')&255)==expected,(amount,eligible,hours_ok,ready_ok,hex(reg('RIP')),reg('RDI')&255)
  cases.append({'allowance':amount,'cost':5,'eligible':bool(eligible),'hours_available':bool(hours_ok),'native_ready':bool(ready_ok),'passes':bool(expected),'calls':gate_calls[:]})
finally:uc.hook_del(token)
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
assert call(exports['DriTestQualityFault'])==0
report={'passed':True,'baseline_defect_reproduced':baseline,'negative_control':negative,'fixed_order':[ri(r+0x18) for r in [primary,portal,backup]],
 'native_dispatch_rvas':['0x08171f20','0x08172310'],'native_action_gate':'0x0815f2d3..0x0815f336','cost_gate_cases':cases,
 'unchanged':['HS stage score total600 and portal score777','portal bytes and slot','native school budget','native eligibility and affordability gates'],
 'modeled_services':['Windows','SDF records','native minimum allowance50 for ordering case','native eligibility/school hours/readiness leaves'],
 'live_game_verified':False,'limitations':['Prefixes and action gates execute captured native code; full two-pass spending and weekly advance do not execute.']}
(OUTPUT/'test32_native_funding_order_results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS '+('baseline second-pass ordering defect reproduced' if baseline else 'compiled HS permutation reaches primary first in both captured native funding passes')+'; six native action-cost/eligibility gates preserved')
