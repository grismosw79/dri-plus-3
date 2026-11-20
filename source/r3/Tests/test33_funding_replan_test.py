"""Recorded W1 cap-blocked primary shape through compiled pre-allocation replan.
Exact identities/fit/OVR/star/cap snapshot are recorded; year/roster, remaining
budget and already-offered-at-entry Jean are controlled adaptations.
"""
from pathlib import Path
import json,copy
T=Path(__file__).parent
exec(compile((T/'test31_hs_runtime_test.py').read_text().split('# Full inherited room')[0],'HS definitions','exec'))
fixture=json.loads((T/'Fixtures/test33_w1_regression.json').read_text())['indiana']
reset8();tid=fixture['tid'];allowed8.add(tid);school=allteams[tid];year=next_single_year(fixture['position'],tid)
for t in allteams:records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:0})
budget=records[school]['ref'][224];records[budget]['scalar'][3]=fixture['school_total_hours']
players=[]
for observed in fixture['candidates']:
 r,p=rr(observed['rid'],score=1000,ovr=observed['ovr'],stage=0,schools=((tid,100),),offers=(tid,),portal=False)
 records[p]['scalar'].update({3:observed['position'],231:observed['internal_stars'],261:255})
 scores[r]=observed['allocation_native_score']
 target=next(t for t in records[board_targets(school)]['array']if records[t]['ref'].get(10)==r)
 records[target]['scalar'].update({7:0,12:0})
 players.append((r,p,target))
(primary,pp_,pt),(backup,bp,bt)=players
before_roles=(role31(primary,tid),role31(backup,tid))
assert before_roles==(1,2),before_roles
records[bt]['scalar'][12]=2
actual=[valid_offer(primary,destination=allteams[t])for t in fixture['candidates'][0]['actual_cpu_offer_tids_final']]
# Native acquired/final/portal entries are out of HS reweight scope.
acq,ap,at=target31(33301,300,76,pos=1)
records[board_targets(allteams[96])]['array'].remove(at);records[board_targets(school)]['array'].append(at)
records[records[school]['ref'][33]]['array'].append(ap)
final,fp,ft=target31(33302,300,76,pos=2,stage=5)
records[board_targets(allteams[96])]['array'].remove(ft);records[board_targets(school)]['array'].append(ft)
portal,portal_player=rr(33303,score=1000,ovr=70,stage=0,schools=((tid,100),),offers=(tid,),portal=True)
portal_target=next(t for t in records[board_targets(school)]['array']if records[t]['ref'].get(10)==portal)
frame=alloc(0x900);regs=alloc(0x200);wq(regs+80,frame+0x100);fb=frame+0x100;wq(regs+72,context)
holder=alloc(8);wq(holder,school);wq(fb-0x60,holder)
vec=alloc(360);indices=alloc(40)
wq(fb-0x50,vec);wq(fb-0x48,vec+360);wq(fb+0x58,indices);wq(fb+0x60,indices+40)
entries=[(primary,pt,583,0),(acq,at,101,1),(backup,bt,293,0),(final,ft,102,1),(portal,portal_target,777,1)]
for j,(r,t,score,bucket)in enumerate(entries):
 uc.mem_write(vec+j*72,b'\0'*72);wq(vec+j*72,r);wi(vec+j*72+40,score);wq(vec+j*72+48,t);wi(vec+j*72+64,bucket);wq(indices+j*8,j)
preserved={j:bytes(uc.mem_read(vec+j*72,72))for j in [1,3,4]}
before_budget=copy.deepcopy(records[budget]);before_points={r:values(r)for r,p,t in players}
game_api(0x0817e7b0,lambda:50)
call(exports['DriTestHs32Begin'],context);call(hooks[23],regs)
assert call(exports['DriTestQualityFault'])==0
weights={rq(vec+j*72):ri(vec+j*72+40)for j in range(5)}
assert weights[primary]==0 and weights[backup]==876,weights
assert role31(backup,tid)==1 and role31(primary,tid)==0
assert rq(vec)==backup and rq(vec+2*72)==primary
assert all(bytes(uc.mem_read(vec+j*72,72))==v for j,v in preserved.items())
assert records[budget]==before_budget and before_points=={r:values(r)for r,p,t in players}
assert all(records[t]['scalar'][12]>=2 for t in actual),'existing16 actual scholarships are grandfathered'
assert sum(weights.values())==1856
# Post-scholarship safe pass keeps zero blocked score and never moves score
# from untouched portal/acquired/final paths into HS.
call(exports['DriTest33BetweenHsFunding'],regs)
assert weights=={rq(vec+j*72):ri(vec+j*72+40)for j in range(5)}
# A backup in another native stage bucket cannot inherit blocked hours.
# Keep the empty executable bucket's old score inert; the actual offered guard
# (tested in native spending) prevents its target from consuming productive time.
for j in [0,2]:
 rec=rq(vec+j*72)
 wi(vec+j*72+40,583 if rec==primary else 293)
 wi(vec+j*72+64,0 if rec==primary else 1)
call(hooks[23],regs)
cross={rq(vec+j*72):ri(vec+j*72+40)for j in range(5)}
assert cross[primary]==583 and cross[backup]==293,cross
assert all(bytes(uc.mem_read(vec+j*72,72))==v for j,v in preserved.items())
assert records[budget]==before_budget
call(exports['DriTestHs32Reset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
report={'passed':True,'source':'Fixtures/test33_w1_regression.json','school':tid,'native_scores':[445,470],
 'reweighted_scores':{'2484':0,'2002':876},'existing_actual_offers_preserved':16,'cross_bucket_control':{'blocked_inert':583,'offered_other_bucket':293},
 'controlled_adaptations':['year and synthetic roster with one annual CB slot','Jean starts offered; recorded BEGIN was unoffered and POST was offered','total1950 hours known, remaining1950 is controlled','native entitlement50 is modeled'],
 'unchanged':['same native HS bucket total876','acquired/final/portal physical bytes and index slots','school budget','recruit interest','existing offers'],
 'live_game_verified':False}
(OUTPUT/'test33_funding_replan_results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS W1 Indiana cap-blocked Nick reselects Jean before allocation; blocked0/backup876 same-bucket score,16 legacy offers and unrelated native bytes preserved')
