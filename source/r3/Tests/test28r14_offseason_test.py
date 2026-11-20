"""Compiled offseason reservation bridge. Native board-storage mutation is
modeled with its verified three-argument ABI; no live signing is claimed.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
checks=[];adds=[];deny_add=False
def add_native():
 manager_arg,tr,cand,_=args();assert manager_arg==rq(context+0xc8)
 t=rq(tr);r=rq(cand);assert ri(cand+40)>0 and bytes(uc.mem_read(cand+8,32))==bytes(32)
 adds.append((records[t]['scalar'][383],ri(r+0x18)))
 if deny_add:return 7
 a=board_targets(t);target=record('native admitted target',{12:0},ref={10:r});records[a]['array'].append(target);return 0
game_api(0x08149660,add_native)
def case(rid,tid,pos,ovr):
 reset29();call(exports['DriTestHs32Reset']);adds.clear()
 records[season]['scalar'].update({4:6,5:6})
 records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 allowed8.clear();allowed8.add(tid)
 for t in allteams:records[records[t]['ref'][224]]['scalar'][3]=0
 t=allteams[tid];records[records[t]['ref'][224]]['scalar'][3]=100
 r,p=make(rid,(),[],ovr=ovr);records[p]['scalar'].update({3:pos,231:3})
 slot=call(exports['DriTestHsRecoveryDestination'],r,t,context,1)
 assert call(exports['DriTestHsRecoveryDestination'],r,t,context,2)==1
 assert call(exports['DriTestHsRecoveryReserve'],r,t,context,slot*4+1)==1
 # Test30: a reservation alone cannot authorize native admission. Execute the
 # real all-school frozen market on this complete controlled context first.
 call(exports['DriTestHs32Begin'],context)
 assert call(exports['DriTestHs30AdmissionBlock'],r,t,context)==0
 assert call(exports['DriTestQualityFault'])==0
 return r,t
for rid,tid,pos,ovr in [(2991,71,9,75),(1247,17,3,74),(1033,36,3,73),(1233,76,3,74),(2311,8,3,73)]:
 r,t=case(rid,tid,pos,ovr);before=dict(records[r]['scalar']);budget=dict(records[records[t]['ref'][224]]['scalar'])
 call(exports['DriTestHs32Offseason'],t,context)
 assert adds==[(tid,rid)] and len(records[board_targets(t)]['array'])==1
 assert records[r]['scalar']==before and records[records[t]['ref'][224]]['scalar']==budget and not offer_status(r,tid)
 call(exports['DriTestHs32Offseason'],t,context);assert len(adds)==1
 target=valid_offer(r,destination=t);records[target]['scalar'][15]=1;records[records[t]['ref'][224]]['scalar'][1]=5
 assert call(exports['DriTestHsPlanRole'],r,t,context,0)==1
 assert call(exports['DriTestHs29Offer'],r,t,context)==2
checks.append('Five NSD failure identity/position/OVR shapes reach native board admission exactly once; subsequent native-funded offer can become firm; bridge creates no scholarship, points or hours')
for loss in ['eligibility','hours','capacity','published','hard','user','regular']:
 r,t=case(99000,17,3,74)
 if loss=='eligibility':allowed8.clear()
 elif loss=='hours':records[records[t]['ref'][224]]['scalar'][1]=100
 elif loss=='capacity':
  for i in range(50):
   other,op=make(99100+i,(),[],ovr=60);valid_offer(other,destination=t)
 elif loss=='published':records[records[t]['ref'][33]]['array']=[records[r]['ref'][5]]
 elif loss=='hard':records[r]['scalar'][9]=5
 elif loss=='user':records[t]['ref'][407]=record('user')
 else:records[season]['scalar'][5]=1
 call(exports['DriTestHs32Offseason'],t,context);assert not adds,loss
 records[t]['ref'].pop(407,None)
checks.append('Changed eligibility, exhausted hours, full board, published ownership, hard commitment, user control and regular-season context all block replayed admission')
r,t=case(99200,17,3,74);deny_add=True
call(exports['DriTestHs32Offseason'],t,context)
assert adds==[(17,99200)] and not records[board_targets(t)]['array'] and not offer_status(r,17)
checks.append('Native admission refusal is retained without fabricating a board target or offer')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test28r14_offseason_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
