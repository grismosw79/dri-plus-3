"""Compiled market coverage: native assignment evidence, slots and scarce budgets."""
import os
# A single coverage begin validates all 138 schools and makes both market
# sweeps. The instruction cap is an emulator budget, not a game-side loop limit.
os.environ.setdefault('DRI_TEST_INSTRUCTION_LIMIT','50000000')
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHsCoverageReset'])
records[season]['scalar'].update({2:2042,4:1,5:1})
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
for pos,e in enumerate(ideal_entries):records[e]['scalar'].update({2:ideals[pos],4:1})
# Model verified native action costs, including scholarship-only vs productive work.
offer=record('offer action',{0:3,1:0,2:5,5:1,6:1})
social=record('search action',{0:5,1:4,2:5,5:1,6:0})
pitch_action=record('soft sell action',{0:2,1:20,2:20,4:1,5:1,6:0})
actions=record('native recruiting actions',array=[social,pitch_action,offer])
records[tuning]['ref'][49]=actions
records[team]['scalar'][384]=5
other=fixture_dummy_teams[3]
# Only two CPU schools have resources; both have one genuine QB vacancy.
for school in [team,other]:
 tid=records[school]['scalar'][383]
 board_ref=records[school]['ref'][224];records[board_ref]['scalar'].update({1:0,2:0,3:1500})
 ra=records[school]['ref'][239];records[ra]['array']=[]
 for pos in range(22):records[ra]['array'] += [pp('coverage returner '+str(tid)+'/'+str(pos),65,pos=pos,tid=tid) for j in range(max(0,ideals[pos]-(pos==0)))]
r,t,p=pr(82000,78,year=0,pos=0,stage=0,portal=False);records[p]['scalar'][231]=3
records[r]['scalar'][3]=1000
for e in records[records[r]['ref'][13]]['array']:records[e]['scalar'][1]=0
call(exports['DriTestHsCoverageBegin'],context)
assert call(exports['DriTestHsCoverageCount'],r,1)==2
assert call(exports['DriTestHsCoverageCount'],r,0)==0
assert not records[arr]['array']
checks=['Full-market zero/one pass reserves two distinct suitable CPU paths without creating offers or influence']
# Scheduled values at a begin/end observation cannot establish executed/funded allocation.
valid_offer(r,t);records[t]['scalar'].update({6:5,7:0,15:0})
records[board]['scalar'].update({1:5,2:5})
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsCoverageCount'],r,0)==0
assert call(exports['DriTestHsCoverageCount'],r,1)==1 # rejected/offer-only path released from coverage count
checks.append('Scholarship-only five-hour spending is not a funded pursuit and does not block another school')
# Real native generic assignment: flag15 + livecost5 + assignedbudget; current hours may stillonlyoffer5.
records[t]['scalar'].update({15:1});records[board]['scalar'].update({1:10})
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsCoverageCount'],r,0)==1
# Second school softsell-only funded pursuit, no genericflags.
ot=valid_offer(r,destination=other);ob=records[other]['ref'][224]
records[ot]['ref'][0]=record('active pitches',array=[record('soft sell',{0:1,1:8})])
records[ot]['scalar'].update({6:5,7:0});records[ob]['scalar'].update({1:25,2:5})
call(exports['DriTestHsCoverageObserve'],other,context)
assert call(exports['DriTestHsCoverageCount'],r,0)==2
checks.append('Generic and pitch-only native allocations count distinctly from deferred weekly execution')
# Resetcalendarflags/hours: priorverifiedfundingremains but withdrawingactualoffer removescoverage.
records[t]['scalar'].update({6:0,15:0});records[board]['scalar'][1]=0
call(exports['DriTestHsCoverageEnd'],context)
assert call(exports['DriTestHsCoverageCount'],r,0)==2
records[t]['scalar'][12]=0
call(exports['DriTestHsCoverageEnd'],context)
assert call(exports['DriTestHsCoverageCount'],r,0)==1
checks.append('Weekly reset does not erase verified funding; a withdrawn offer immediately removes real coverage')
# A ten-hourbudgetcanreserveone newoffer+productiveaction, notmultipleprospects.
call(exports['DriTestHsCoverageReset']);call(exports['DriTestHsPlanReset']);records[arr]['array']=[]
records[board]['scalar'].update({1:0,2:0,3:10})
a,at,ap=pr(82001,78,year=0,pos=0,stage=0,portal=False);records[ap]['scalar'][231]=3
b,bt,bp=pr(82002,78,year=0,pos=0,stage=0,portal=False);records[bp]['scalar'][231]=3
for rec in [a,b]:
 records[rec]['scalar'][3]=1000
 for e in records[records[rec]['ref'][13]]['array']:records[e]['scalar'][1]=0
slot=call(exports['DriTestHsRecoveryDestination'],a,team,context,1);role=call(exports['DriTestHsRecoveryDestination'],a,team,context,2)
assert slot>0 and role==1
assert call(exports['DriTestHsRecoveryReserve'],a,team,context,slot*4+role)==1
assert call(exports['DriTestHsRecoveryDestination'],b,team,context,0)==0
checks.append('Newoffer+productiveaction costs reserve native hours once; second prospect cannot spend same ten hours')
# Freshbudget rereadafterwarmcache, plus fiftyboardcapacityincludesreserved additions.
call(exports['DriTestHsCoverageReset']);records[board]['scalar'].update({1:0,2:0,3:1500})
assert call(exports['DriTestHsRecoveryDestination'],a,team,context,0)>0
records[board]['scalar'][1]=1500
assert call(exports['DriTestHsRecoveryDestination'],a,team,context,0)==0
checks.append('Final destination probe rereads live assigned budget rather than using stale weekly cache')
# Optional primary reservations share the same two allowances across positions;
# a backup attaches to its reserved primary without inventing a third signing.
call(exports['DriTestHsCoverageReset']);call(exports['DriTestHsPlanReset']);records[arr]['array']=[]
records[board]['scalar'].update({1:0,2:0,3:1500})
records[roster]['array']=[pp('full room '+str(pos),65,pos=pos) for pos in range(22) for j in range(ideals[pos])]
x,xt,xp=pr(82003,78,year=0,pos=3,stage=0,portal=False);records[xp]['scalar'][231]=3
y,yt,yp=pr(82004,78,year=0,pos=16,stage=0,portal=False);records[yp]['scalar'][231]=3
for rec in [x,y]:
 records[rec]['scalar'][3]=1000
 for e in records[records[rec]['ref'][13]]['array']:records[e]['scalar'][1]=0
for rec in [a,x]:
 slot=call(exports['DriTestHsRecoveryDestination'],rec,team,context,1);role=call(exports['DriTestHsRecoveryDestination'],rec,team,context,2)
 assert slot%32==31 and role==1,(slot,role)
 assert call(exports['DriTestHsRecoveryReserve'],rec,team,context,slot*4+role)==1
assert call(exports['DriTestHsRecoveryDestination'],y,team,context,0)==0
assert call(exports['DriTestHsRecoveryDestination'],b,team,context,1)==31
assert call(exports['DriTestHsRecoveryDestination'],b,team,context,2)==2
checks.append('Two shared optional reservations prevent a third position; backup uses the existing reserved opening')
# Abandonment recovery applies to lower-star recruits too. Its numeric
# resource ledger must reconcile even when no elite catalogue row exists.
call(exports['DriTestHsCoverageReset']);call(exports['DriTestHsPlanReset']);records[arr]['array']=[]
records[committed]['array']=[]
records[roster]['array']=[pp('lower star room '+str(pos),65,pos=pos) for pos in range(22) for j in range(max(0,ideals[pos]-(pos==0)))]
records[board]['scalar'].update({1:0,2:0,3:20})
low,lt,lp=pr(83000,72,year=0,pos=0,stage=0,portal=False);records[lp]['scalar'][231]=2
next_low,nt,np=pr(83001,72,year=0,pos=0,stage=0,portal=False);records[np]['scalar'][231]=2
for rec in [low,next_low]:
 records[rec]['scalar'][3]=1000
 for e in records[records[rec]['ref'][13]]['array']:records[e]['scalar'][1]=0
slot=call(exports['DriTestHsRecoveryDestination'],low,team,context,1);role=call(exports['DriTestHsRecoveryDestination'],low,team,context,2)
assert call(exports['DriTestHsRecoveryReserve'],low,team,context,slot*4+role)==1
assert call(exports['DriTestHsRecoveryReservedCost'],low,team,context,0)==10
assert call(exports['DriTestHsRecoveryReservedCost'],low,team,context,1)==1
assert call(exports['DriTestHsCoverageCount'],low,0)==0xffffffff # deliberately outside4/5-star catalogue
valid_offer(low,lt);records[lt]['scalar'].update({6:5,15:0});records[board]['scalar'].update({1:5,2:5})
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsRecoveryReservedCost'],low,team,context,0)==5
assert call(exports['DriTestHsRecoveryReservedCost'],low,team,context,1)==0
records[lt]['scalar'][15]=1;records[board]['scalar'][1]=10
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsRecoveryReservedCost'],low,team,context,0)==0
assert call(exports['DriTestHsRecoveryPending'],low,context)==1
slot2=call(exports['DriTestHsRecoveryDestination'],next_low,team,context,1);role2=call(exports['DriTestHsRecoveryDestination'],next_low,team,context,2)
assert slot2==slot and role2==2
assert call(exports['DriTestHsRecoveryReserve'],next_low,team,context,slot2*4+role2)==1
checks.append('Three-star native offer and funding release pending board/hour charges, allowing the affordable backup in the same week')
# No native target was created for the new backup: release only its reservation.
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsRecoveryPending'],next_low,context)==0
assert call(exports['DriTestHsRecoveryPending'],low,context)==1
# A materialized target whose scholarship was rejected also releases its reservation.
slot2=call(exports['DriTestHsRecoveryDestination'],next_low,team,context,1);role2=call(exports['DriTestHsRecoveryDestination'],next_low,team,context,2)
assert call(exports['DriTestHsRecoveryReserve'],next_low,team,context,slot2*4+role2)==1
records[arr]['array'].append(nt);records[nt]['scalar'][12]=0
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsRecoveryPending'],next_low,context)==0
checks.append('Missing native target and rejected scholarship release the pending attempt for another qualified destination')
# Published acquisition with a stale stage cannot hold a pending recovery role.
records[arr]['array']=[lt];records[committed]['array']=[lp]
call(exports['DriTestHsCoverageObserve'],team,context)
assert call(exports['DriTestHsRecoveryPending'],low,context)==0
checks.append('Published lower-star acquisition closes its reservation even with a stale open recruit stage')
call(exports['DriTestOwnershipReset']);assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
checks.append('All fixture reference ownership remains balanced')
(OUTPUT/'test28r8_coverage_runtime_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False,'funding_evidence':'native assigned actions; weekly execution separately observed'},indent=2)+'\n')
print(json.dumps(checks,indent=2))
