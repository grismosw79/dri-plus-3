"""Actual compiled R2 callbacks on save-shaped SDF fixtures.
No native season is simulated and no live R2 outcome is asserted.
"""
from pathlib import Path
import ctypes as C, copy, json
fixture=(Path(__file__).parent/'test29_portal_witness_test.py').read_text()
exec(compile(fixture.split('primary,pt,ppp=')[0],'test29 witness definitions','exec'))
records[tuning]['ref'][49]=record('native actions',array=[record('social',{0:5,1:10,2:5,5:1,6:0})])
checks=[]
def reset():
 for a in [roster,hslist,tplist,committed,arr]:records[a]['array']=[]
 records[board]['scalar'].update({1:0,2:0,3:800})
 for p,e in enumerate(ideal_entries):records[e]['scalar'].update({2:ideals[p],4:1,0:0})
 records[team]['scalar'][383]=TEST_TEAM_ID
 records[teams]['array']=fixture_team_array()
 call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
def candidate(rid,ovr,pos=1,stage=3):
 r,t,p=pr(rid,ovr,pos=pos,stage=stage);valid_offer(r,t)
 records[t]['scalar'].update({15:1,8:250})
 records[records[r]['ref'][13]]['array']=records[records[r]['ref'][13]]['array'][:1]
 records[records[records[r]['ref'][13]]['array'][0]]['scalar'].update({0:TEST_TEAM_ID,1:250})
 return r,t,p
def plan(entries):
 pool=(Choice*len(entries))()
 for i,(r,pos,slot,round_) in enumerate(entries):
  pool[i].rec=r;pool[i].rid=ri(r+0x18);pool[i].selected=1;pool[i].pos=pos
  pool[i].slot=slot;pool[i].round=round_;pool[i].priority=i+1
 address=alloc(C.sizeof(pool));uc.mem_write(address,bytes(pool))
 call(exports['DriTestCommitPlan'],address,len(entries),team,context)
def firm(r,deadline=1):
 before=copy.deepcopy(records)
 value=call(exports['DriTest29R2Committable'],r,team,context,deadline)
 assert records==before,'acceptance observation modified native records'
 assert call(exports['DriTestQualityFault'])==0
 return value
def witness(r):
 before=copy.deepcopy(records);call(exports['DriTestPortal29Observe'],team,context)
 assert records==before
 return call(exports['DriTestPortal29Witness'],ri(r+0x18))&15
def execution(target):
 # Existing captured-branch test proves the hook ABI. Exercise its actual
 # observer here with a newly changed destination/capacity state.
 frame=alloc(128);bp=alloc(256);th=alloc(8);rh=alloc(8)
 wq(th,team);wq(rh,target);wq(bp+0x67,th);wi(bp+0x7f,10)
 wq(frame,5);wq(frame+8,context);wq(frame+24,5);wq(frame+72,rh);wq(frame+80,bp)
 before=copy.deepcopy(records)
 call(exports['DriTestPortal29GenericAction'],frame)
 assert records==before
 return [call(exports['DriTestPortal29Execution'],i)for i in range(8)]

# Hosley-shaped: the old primary has signed, but the current room permits a
# material rotation upgrade through an existing optional position place.
reset();records[ideal_entries[1]]['scalar'][2]=6
records[roster]['array']=[pp('HB returner '+str(i),v,pos=1)for i,v in enumerate([78,72,70,68])]
first,ft,fp=candidate(6101,89,stage=5);second,st,sp=candidate(6102,84,stage=5)
fallback,bt,bp=candidate(6103,84)
records[board]['scalar'][1]=15
plan([(first,1,1,1),(fallback,1,1,2),(second,1,2,1)])
assert call(exports['DriTestPortalDecision'],fallback,team,context)==0
assert call(exports['DriTestCommitBlock'],fallback,team,context,1)==0
assert firm(fallback)==1
# Native acceptance uses the same complete gates, and an actual signing
# consumes this place immediately; no second fallback receives the same seat.
records[fallback]['scalar'][9]=5
extra,et,ep=candidate(6104,90)
records[board]['scalar'][1]=20
assert call(exports['DriTestPortalDecision'],extra,team,context)==4
assert firm(extra)==0
checks.append('Closed old opening can reuse current legal optional capacity; actual signing prevents duplicate position use')

# Green/Lindsey/Johnson-shaped: the one actual remaining vacancy belongs to
# another viable live opening. A closed-chain fallback cannot steal it.
reset();records[ideal_entries[1]]['scalar'][2]=6
records[roster]['array']=[pp('HB incumbent '+str(i),v,pos=1)for i,v in enumerate([74,70,68,66])]
first,ft,fp=candidate(6201,85,stage=5);green,gt,gp=candidate(6202,81)
johnson,jt,jp=candidate(6203,81);lindsey,lt,lp=candidate(6204,82)
records[board]['scalar'][1]=20
plan([(first,1,1,1),(green,1,1,2),(johnson,1,2,1),(lindsey,1,2,2)])
assert call(exports['DriTestCommitBlock'],green,team,context,1)==10
assert call(exports['DriTestCommitBlock'],lindsey,team,context,1)==9
assert firm(green)==0 and firm(lindsey)==0 and firm(johnson)==1
assert witness(johnson)==7 and witness(green)==11
# The closed-chain candidate responds to withdrawal immediately without a
# week change, then loses the opportunity again when the real offer returns.
records[jt]['scalar'][12]=0;records[lt]['scalar'][12]=0
assert firm(green)==1
records[jt]['scalar'][12]=3;assert firm(green)==0
records[johnson]['scalar'][9]=5
assert firm(green)==1
checks.append('Other live opening remains protected; offer withdrawal/restoration and actual destination publication are re-read')

# A paid planned primary is conditional when a superior paid choice needs
# the same final allowance. Native final positive progress below threshold
# still qualifies for priority; strict quality ordering prevents a cycle.
reset();records[ideal_entries[1]]['scalar'][2]=5
records[roster]['array']=[pp('full HB '+str(i),v,pos=1)for i,v in enumerate([82,74,70,68,67])]
strong,st,sp=candidate(6301,87,stage=0);lower,lt,lp=candidate(6302,80,stage=0)
for r,t,points in [(strong,st,129),(lower,lt,130)]:
 records[r]['scalar'][3]=136;records[t]['scalar'][8]=points
 records[records[records[r]['ref'][13]]['array'][0]]['scalar'][1]=points
records[board]['scalar'][1]=10
plan([(lower,1,1,1),(strong,1,2,1)])
assert call(exports['DriTest29R1Capacity'],lower,team,context,1)==19, (call(exports['DriTestPortalDecision'],lower,team,context),call(exports['DriTestPortalDecision'],strong,team,context),call(exports['DriTestQualityFault']))
assert firm(lower)==0 and firm(strong)==1
assert witness(lower)==11 and witness(strong)==7
assert execution(lt)[5:]==[0,1,1]
assert execution(st)[5:]==[1,0,1]
# The same current acceptance definition drives recovery. A capacity-blocked
# paid offer keeps its earned points/offer while the recruit can reconsider.
for r,t in [(strong,st),(lower,lt)]:
 records[r]['scalar'][9]=3;records[t]['scalar'][8]=250
 records[records[records[r]['ref'][13]]['array'][0]]['scalar'][1]=250
paid=records[lt]['scalar'][8];top=records[records[lower]['ref'][13]]['array'][0]
earned=records[top]['scalar'][1]
call(exports['DriTestRecoverMarket'],context,0)
assert records[lower]['scalar'][9]==0 and records[strong]['scalar'][9]==3
assert records[lt]['scalar'][12]==3 and records[lt]['scalar'][8]==paid and records[top]['scalar'][1]==earned
records[st]['scalar'][15]=0
assert firm(lower)==1
records[st]['scalar'][15]=1
assert firm(lower)==0
# Stronger candidate cannot be trapped behind an inferior old same-slot
# priority while that inferior candidate waits for its capacity in return.
plan([(lower,1,1,1),(strong,1,1,2)])
assert firm(lower)==0 and firm(strong)==1
checks.append('Firm telemetry checks paid capacity priority, lower choice reopens without losing earned offer, no reciprocal deferral')

# Two different positions share the final general allowance. Neither is an
# inadequate-role emergency: the superior useful upgrade retains priority,
# and its actual signing exhausts the same budget seen by the other room.
reset();records[ideal_entries[9]]['scalar'][2]=3
records[ideal_entries[16]]['scalar'][2]=6;records[ideal_entries[18]]['scalar'][2]=4
records[roster]['array']=[pp('RT incumbent '+str(i),84,pos=9)for i in range(3)]+[pp('CB incumbent '+str(i),90,pos=16)for i in range(6)]+[pp('SS incumbent '+str(i),90,pos=18)for i in range(4)]
used,ut,up=candidate(6401,90,pos=9,stage=5)
strong,st,sp=candidate(6402,99,pos=18);lower,lt,lp=candidate(6403,95,pos=16)
records[board]['scalar'][1]=10
plan([(lower,16,1,1),(strong,18,2,1)])
assert call(exports['DriTestPortalDecision'],strong,team,context)==0
assert call(exports['DriTestPortalDecision'],lower,team,context)==0
assert firm(lower)==0 and firm(strong)==1
assert witness(lower)==11 and witness(strong)==7
records[strong]['scalar'][9]=5
assert call(exports['DriTestPortalDecision'],lower,team,context)==8
assert firm(lower)==0
records[strong]['scalar'][9]=3;records[st]['scalar'][12]=0
call(exports['DriTestOwnershipReset'])
assert firm(lower)==1
records[st]['scalar'][12]=3
checks.append('Different positions cannot duplicate the last general allowance; strict better-role priority and actual shared consumption agree')

# Native eligibility remains an independent gate and never gets replaced by
# plan role, a positive influence number, or a material-quality exception.
game_api(0x0816b130,lambda:0)
assert firm(strong)==0
game_api(0x0816b130,fixture_eligibility)
assert firm(strong)==1
checks.append('Native eligibility remains mandatory; observation is read-only and carries no guaranteed signing claim')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'r2_acceptance.json').write_text(json.dumps({'checks':checks,'method':'compiled R2 acceptance/recovery/witness callbacks with save-shaped controlled records',
 'actual_game_records_replayed':False,'live_game_verified':False,
 'limitation':'This acceptance fixture does not execute the separately tested guarded native final-pass retry.'},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
