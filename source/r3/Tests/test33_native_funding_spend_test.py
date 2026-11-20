"""Test33 compiled gate/replan through captured native allocation passes.
Both enclosing loops, native allowance/cost checks and second-pass productive
writes execute; native scholarship publication is a disclosed controlled leaf.
No live weekly advance or influence delivery is simulated by these assertions.
"""
from pathlib import Path
import json,struct,copy
T=Path(__file__).parent
exec(compile((T/'test32_native_funding_spend_test.py').read_text().split('results=[execute_case')[0],'native spending definitions','exec'))
# All fixture schools remain real, but only this school proposes new offers.
for tid,t in enumerate(allteams):
 if tid!=96:records[records[t]['ref'][224]]['scalar'][3]=0
# A complete13-school controlled franchise keeps the12-offer cap-race exact.
records[teams]['array']=[allteams[t]for t in list(range(10,22))+[96]]
# The captured first pass uses these existing slots as well as the second pass.
wq(loop_frame-0x48,vec+216);wq(loop_frame+0x58,indices);wq(loop_frame+0x60,indices+24)
wq(loop_frame-0x20,0)
phase_frame=alloc(0x200);wq(phase_frame+80,loop_frame);wq(phase_frame+72,native_eval)
regions33=[(0x08171f20,0x08172310),(0x0815f1c0,0x0815f383),(0x0862b580,0x0862b61a)]
for lo,hi in regions33:
 map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000]);handlers.pop(GAME+lo,None);uc.ctl_remove_cache(GAME+lo,GAME+hi)
# Install actual compiled eligibility hook. Its continuation is the native
# eligibility service only; it cannot bypass the production callback's veto.
hook=call(exports['DriTest31OfferHook']);map_game(0x0819adf0,0x1000)
handlers.pop(GAME+0x0819adf0,None)
uc.mem_write(GAME+0x0819adf0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook)+b'\x90')
refuse=set();events=[]
def continuation33():
 target=rq(reg('R8'));action=ri(reg('RSP')+0x28)
 events.append(('eligibility',ri(records[target]['ref'][10]+0x18),action))
 return int(action==5 or (action==3 and target not in refuse))
game_api(0x0819adff,continuation33)
def candidate_gate33():
 target=rq(reg('R8'));recruit=records[target]['ref'][10]
 return int(records[recruit]['scalar'].get(9,0)<5)
game_api(0x0819a9b0,candidate_gate33)
# Native diagnostic/expression/readiness services do not allocate any hours.
for address,fn in [(0x037f5e40,lambda:reg('RCX')),(0x07e18e10,lambda:0),(0x082dfd10,lambda:0),(0x0815ee70,lambda:1)]:game_api(address,fn)
# Actual native scholarship eligibility checks offered state, action3 gate,
# allowance, school hours and readiness before reaching this publication leaf.
def scholarship33():
 eval_,team_,target_,unused=args();target=rq(target_)
 assert records[target]['scalar'].get(12,0)<2
 assert records[budget]['scalar'][1]+5<=records[budget]['scalar'][3]
 records[target]['scalar'][12]=2
 records[target]['scalar'][2]=records[target]['scalar'].get(2,0)+5
 records[budget]['scalar'][1]+=5
 wi(eval_+0x230,ri(eval_+0x230)+5)
 events.append(('scholarship_published',ri(records[target]['ref'][10]+0x18),5))
 return 5
game_api(0x0814daf0,scholarship33)
# Unicorn scalar-AVX workarounds preserve the exact scalar operations; all
# surrounding captured branches and arithmetic remain original instructions.
def scalar_first33(u,address,size,data):
 ops={0x08172010:(UC_X86_REG_XMM2,UC_X86_REG_XMM1,UC_X86_REG_XMM0,'mul',4),
      0x0817201d:(UC_X86_REG_XMM3,UC_X86_REG_XMM2,UC_X86_REG_XMM0,'div',4),
      0x0817203b:(UC_X86_REG_XMM6,UC_X86_REG_XMM7,UC_X86_REG_XMM3,'min',4),
      0x0817203f:(UC_X86_REG_XMM1,UC_X86_REG_XMM0,UC_X86_REG_XMM6,'mul',4),
      0x0817211a:(UC_X86_REG_XMM0,UC_X86_REG_XMM6,UC_X86_REG_XMM8,'mul',5)}
 op=ops.get(address-GAME)
 if op:
  dest,a,b,kind,size=op;x=f32(u.reg_read(a));y=f32(u.reg_read(b));v=x*y if kind=='mul' else x/y if kind=='div' else min(x,y)
  u.reg_write(dest,struct.unpack('<I',struct.pack('<f',v))[0]);u.reg_write(UC_X86_REG_RIP,address+size)
uc.hook_add(unicorn.UC_HOOK_CODE,scalar_first33)
wi(native_eval+0x1bc,5)
# The native NIL-expression context is a valid read-only fixture record.
if not rq(native_eval+0xd8):wq(native_eval+0xd8,tuning)
actual_extra=[]
def reset_case33(remaining,primary_offered=0,backup_offered=2):
 global writes,elig_calls,events,refuse
 events=[];writes=[];elig_calls=[];refuse=set()
 records[budget]['scalar'].update({1:300-remaining,2:0,3:300})
 records[pt]['scalar'][12]=primary_offered;records[bt]['scalar'][12]=backup_offered
 # Portal remains untouched by the HS callback; refuse its controlled action
 # here by using its native acquired stage so it cannot obscure the HS witness.
 records[portal]['scalar'][9]=5
 for target in [pt,bt,portal_target]:
  for field in range(15,20):records[target]['scalar'][field]=0
  records[target]['scalar'][2]=0
 for target in actual_extra:records[target]['scalar'][12]=0
 for off in [0x224,0x228,0x22c,0x230,0x254,0x25c,0x260]:wi(native_eval+off,0)
 wi(native_eval+0x250,300-remaining);wi(native_eval+0x238,1377)
 for j,(r,t,s) in enumerate([(backup,bt,300),(portal,portal_target,777),(primary,pt,300)]):
  uc.mem_write(vec+j*72,b'\0'*72);wq(vec+j*72,r);wi(vec+j*72+40,s);wq(vec+j*72+48,t);wi(vec+j*72+64,1);wq(indices+j*8,j)
 call(exports['DriTestHs32Reset']);call(exports['DriTestHsPlanReset']);call(exports['DriTest31FundingReset'])
 call(exports['DriTestHs32Begin'],native_eval);call(hooks[23],phase_frame)
 assert call(exports['DriTestQualityFault'])==0

def scores33():return {ri(rq(vec+j*72)+0x18):ri(vec+j*72+40) for j in range(3)}
def first33():
 uc.reg_write(UC_X86_REG_RSP,loop_sp);uc.reg_write(UC_X86_REG_RBP,loop_frame)
 uc.reg_write(UC_X86_REG_RSI,native_eval);uc.reg_write(UC_X86_REG_R12,team_holder);uc.reg_write(UC_X86_REG_R13,0);uc.reg_write(UC_X86_REG_RDI,0)
 uc.reg_write(UC_X86_REG_XMM7,0x3f800000);uc.reg_write(UC_X86_REG_XMM8,0x42c80000)
 end=GAME+0x081722f1
 def stop(u,address,size,data):
  if address==end:u.emu_stop()
 token=uc.hook_add(unicorn.UC_HOOK_CODE,stop)
 try:uc.emu_start(GAME+0x08171f20,end+1,count=2000000)
 except unicorn.UcError:print('firstpass RIP',hex(reg('RIP')),'recent',list(recent));raise
 finally:uc.hook_del(token)
 assert reg('RIP')==end,hex(reg('RIP'))
 assert not any(records[t]['scalar'].get(15,0) for t in [pt,bt]),'first pass must only attempt scholarships'
 before=scores33();call(exports['DriTest33BetweenHsFunding'],phase_frame)
 assert call(exports['DriTestQualityFault'])==0
 assert sum(scores33().values())==sum(before.values())==1377
 return before,scores33()

def second33(expected):
 before=records[budget]['scalar'][1];uc.reg_write(UC_X86_REG_RSP,loop_sp);uc.reg_write(UC_X86_REG_RBP,loop_frame)
 uc.reg_write(UC_X86_REG_RSI,native_eval);uc.reg_write(UC_X86_REG_R12,team_holder);uc.reg_write(UC_X86_REG_R13,0)
 end=GAME+0x08172852
 def stop(u,address,size,data):
  if address==end:u.emu_stop()
 token=uc.hook_add(unicorn.UC_HOOK_CODE,stop)
 try:uc.emu_start(GAME+0x081722f1,end+1,count=2000000)
 finally:uc.hook_del(token)
 assert reg('RIP')==end,hex(reg('RIP'))
 winners=[r for r,t in [(primary,pt),(backup,bt)]if records[t]['scalar'].get(15,0)]
 assert winners==([expected]if expected else []),(winners,expected,events,writes)
 spent=records[budget]['scalar'][1]-before;assert spent==(5 if expected else 0),(spent,expected,events,writes)
 assert records[budget]['scalar'][1]<=300
 return spent
results=[]
# Both real scholarships and newly published scholarships can fund actions.
for name,remaining,failed,expected in [('new_offer_success',10,False,primary),('native_offer_refusal',5,True,backup),('zero_hours',0,False,None),('below_action_minimum',4,False,None)]:
 reset_case33(remaining);refuse={pt}if failed else set();before,after=first33();spent=second33(expected)
 if failed:
  assert after[ri(primary+0x18)]==0 and after[ri(backup+0x18)]==600
  assert call(exports['DriTestHsPlanRole'],backup,allteams[96],native_eval,0)==1
 if expected==primary:
  publication=next(i for i,e in enumerate(events)if e[0]=='scholarship_published'and e[1]==ri(primary+0x18))
  productive=next(i for i,e in enumerate(events)if e[0]=='eligibility'and e[1]==ri(primary+0x18)and e[2]==5)
  assert publication<productive
 results.append({'case':name,'before':before,'after':after,'productive_spent':spent,'events':events[:]});print('PASS Test33 native case '+name,flush=True)
# Freeze a permit then introduce12 actual CPU offers before native execution.
reset_case33(5)
actual_extra=[valid_offer(primary,destination=allteams[tid])for tid in range(10,22)]
before,after=first33();spent=second33(backup)
assert records[pt]['scalar'][12]==0 and after[ri(primary+0x18)]==0 and after[ri(backup+0x18)]==600
results.append({'case':'same_pass_actual_cap_race','before':before,'after':after,'productive_spent':spent,'events':events[:]})
# No same-bucket recipient: preserve denominator but no unoffered native spend.
reset_case33(5,backup_offered=0);refuse={pt,bt};before,after=first33();spent=second33(None)
assert sum(after.values())==1377 and records[budget]['scalar'][1]==295
results.append({'case':'no_eligible_peer_inert_bucket','before':before,'after':after,'productive_spent':spent,'events':events[:]})
call(exports['DriTestHs32Reset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
report={'passed':True,'cases':results,'captured_native':regions+regions33,'between_callback':'compiled test export at exact081722f1 boundary; separate hook test covers installed detour and ABI',
 'modeled_services':['SDF storage','native eligibility continuation and nonfinal candidate eligibility','13-school controlled franchise','native scholarship publication/payment leaf','native per-target entitlement','NIL clamp/readiness/diagnostic leaves','scalar AVX workaround','Windows'],
 'live_game_verified':False,'limitations':['Scholarship publication is modeled after captured eligibility; full weekly advance and influence delivery do not execute.','Optional first-pass helper081861a0 branch is not exercised ([rsp+0x50]=0).']}
(OUTPUT/'test33_native_funding_spend_results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS native scholarship loop precedes productive loop; compiled refusal/cap replan funds offered backup; zero hours and inert buckets spend nothing')
