"""Compiled Test29 portal market and captured native scorer aggregate.
R2 explicitly supersedes8-primary expectation with5primary plus up to8total attention.
Storage, scorer components and native eligibility are fixtures. Forecasts and
allocated offers are not evidence of executed funding or live signings.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
checks=[];scores={};scorer_calls=0;snapshots=0

def score29():
 global scorer_calls
 scorer,selection,portal,tr=args()
 assert scorer==fixture_hs_scorer and (selection&255,portal&255) in [(1,1),(0,1),(0,0)],(scorer,selection,portal)
 # The mixed-source counter fixture may score an HS board record via its
 # unchanged native ABI. Portal aggregate ABI is checked in the separate captured-code test.
 cand=rq(reg('RSP')+0x30);r=rq(cand);tid=records[rq(tr)]['scalar'][383]
 wi(cand+40,scores.get((ri(r+0x18),tid),0));scorer_calls+=1;return 0
game_api(0x08185840,score29)
def begin(budget=100):
 reset29();call(exports['DriTestMarket29PortalReset']);scores.clear()
 records[season]['scalar'].update({4:3,5:6})
 records[tuning]['ref'][49]=record('market actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 records[ideal_entries[0]]['scalar'][2]=4
 for tid,t in enumerate(allteams):
  records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:budget if 10<=tid<28 else 0})
  records[records[t]['ref'][239]]['array']=[pp('QB returner',65,pos=0,tid=tid)for _ in range(3)]
 allowed8.clear();allowed8.update(range(10,28))
def new(rid,ovr=78,stars=2):
 r,p=rr(rid,score=1000,ovr=ovr,stage=0,schools=(),offers=(),portal=True)
 records[r]['scalar'][0]=1;records[p]['scalar'].update({3:0,231:stars,261:255})
 return r,p
def count(r,k):return call(exports['DriTestMarket29PortalCount'],r,k)
def flags(r,tid):return call(exports['DriTestMarket29PortalFlags'],r,tid)
def state(rs):
 return [snap8(r)for r in rs], [dict(records[records[t]['ref'][224]]['scalar'])for t in allteams]
def snapshot():
 global snapshots
 snapshots+=1;print('portal snapshot',snapshots,flush=True)
 call(exports['DriTestMarket29PortalBegin'],context)
 assert call(exports['DriTestQualityFault'])==0

begin();hot,hp=new(2995);alternatives={}
for tid in range(10,28):
 cold,cp=new(99500+tid,77);alternatives[tid]=cold
 scores[(2995,tid)]=500;scores[(99500+tid,tid)]=480
rs=[hot]+list(alternatives.values());before=state(rs);snapshot()
assert count(hot,0)==0 and count(hot,1)==0
assert count(hot,2)==18 and count(hot,3)==5, [count(hot,k)for k in range(4)]
assert sum(bool(flags(hot,t)&4) for t in alternatives)==13
assert all(count(r,3)==1 for t,r in alternatives.items() if flags(hot,t)&4)
assert state(rs)==before
first={t:(flags(hot,t),flags(r,t))for t,r in alternatives.items()}
old_calls=scorer_calls;snapshot();assert scorer_calls==old_calls
checks.append('18 simultaneous new portal pursuits reconcile to5primary;13 schools redirect to close legal alternatives; sole remaining backups survive the soft secondary guide; actual allocated offers remain0; same-phase reuse makes no scorer calls')
# The snapshot visits teams in the opposite order; it must retain identical
# proposals, demonstrating that shared numeric team identity is not priority.
records[teams]['array']=list(reversed(allteams));call(exports['DriTestMarket29PortalReset']);snapshot()
assert first=={t:(flags(hot,t),flags(r,t))for t,r in alternatives.items()}
assert state(rs)==before
records[teams]['array']=allteams
checks.append('Reversing the global native team-array order and reconstructing the snapshot retains identical affordable selections with no save mutations')
# Only one complete native action bundle is affordable. No second selected
# backup may be counted as a credible planned destination for the same hours.
begin(10);hot,hp=new(2996);alternatives={}
for tid in range(10,28):
 cold,cp=new(99600+tid,77);alternatives[tid]=cold
 scores[(2996,tid)]=500;scores[(99600+tid,tid)]=480
snapshot()
assert count(hot,2)==5
for t,r in alternatives.items():assert bool(flags(hot,t)&1)+bool(flags(r,t)&1)==1,(t,flags(hot,t),flags(r,t))
checks.append('Whole-school remaining budget is shared across all planned choices; one10-hour bundle never funds two forecast pursuits')
# Existing offers backed by actual native allocated actions remain intact even
# when they exceed the new-entry soft target. They are not counted twice.
for tid in alternatives:
 target=valid_offer(hot,destination=allteams[tid]);records[target]['scalar'].update({15:1,8:0})
 records[records[allteams[tid]]['ref'][224]]['scalar'].update({1:5,3:100})
call(exports['DriTestMarket29PortalReset']);snapshot()
assert count(hot,0)==18 and count(hot,1)==18 and count(hot,2)==18
assert all(flags(hot,t)&3==3 for t in alternatives)
checks.append('All18 existing legal allocated primary offers survive the portal boundary; actual/planned union does not double count the same school')
# Mixed stale assignments must not inflate actual portal attention merely
# because each individual five-hour flag fits the shared five-hour counter.
for tid in alternatives:
 extra,ep=make(99900+tid,(),[],ovr=77)
 records[extra]['scalar'][0]=0
 target=valid_offer(extra,destination=allteams[tid]);records[target]['scalar'][15]=1
call(exports['DriTestMarket29PortalReset']);snapshot()
assert count(hot,0)==0 and count(hot,1)==0
checks.append('Portal5 plus HS5 with only5 native assigned hours contributes no actual or firm pressure; native counters and forecast demand are not rewritten')
# Zero actual firm coverage can designate affordable primary admission, but
# the snapshot itself must never manufacture an offer or spend the budget.
begin(10);elite,ep=new(2997,stars=3)
for tid in range(10,28):scores[(2997,tid)]=500
before=state([elite]);snapshot()
assert count(elite,0)==0 and count(elite,1)==0
assert all(flags(elite,t)&8 for t in range(10,28) if flags(elite,t)&2)
assert state([elite])==before
checks.append('A four-star with no actual firm offer receives only affordable primary recovery intentions; admission, productive execution and signing remain separate later evidence')
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
assert call(exports['DriTestQualityFault'])==0
(OUTPUT/'test29_market_portal_results.json').write_text(json.dumps({'checks':checks,'scorer_calls':scorer_calls,'live_game_verified':False,'native_services_mocked':True,'execution_not_implied_by_allocations':True},indent=2)+'\n')
print(json.dumps(checks,indent=2))
