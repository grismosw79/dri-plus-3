"""R8 full-pool HS/JUCO recovery through compiled DLL + captured lockout.
Native eligibility/scoring and SDF storage are controlled fixtures. No native
recruiting spending or live season outcome is claimed by these checks.
"""
from pathlib import Path
_fixture=Path(__file__).parent/'recovery_runtime_test.py'
exec(compile(_fixture.read_text().split('\nreset_recovery()\n# Test10-shaped')[0],str(_fixture),'exec'))
checks8=[]
scenario8=0
allowed8={96}
def gate8():
 ev,tr,rr,arg=args();assert ev==(rq(context+0xc8) if (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))))) else context) and (arg!=0 if (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))) else arg==0)
 return int(records[rq(tr)]['scalar'][383] in allowed8)
game_api(0x0816b130,gate8)

def reset8():
 global scenario8
 scenario8+=1
 reset_recovery();call(exports['DriTestHs28RecoveryReset'])
 records[season]['scalar'].update({2:2042+scenario8,4:8,5:1})
 ideals8=[4,6,1,10,5,4,4,4,4,4,5,5,4,4,6,4,4,4,2,2,2,0]
 for p,v in enumerate(ideals8):records[ideal_entries[p]]['scalar'].update({2:v,0:0,4:1})
 records[tuning]['scalar'][39]=0
 # Controlled action costs exercise affordability; they are not claimed to be
 # the delivered FTC amounts. Native execution remains a separate test.
 records[tuning]['ref'][49]=record('recovery native actions',array=[
  record('scholarship action',{0:3,1:0,2:5,5:1,6:1}),
  record('weekly recruiting action',{0:1,1:10,2:10,5:1,6:0})])
 for team8 in allteams:
  records[records[team8]['ref'][224]]['scalar'].update({1:0,2:0,3:1500})
 allowed8.clear();allowed8.add(96)
 # A changed season resets the reservation catalogue via its actual scope.
 # The explicit fixture reset prevents cross-case plans retaining numeric IDs.
 if 'DriTestHsPlanReset' in exports:call(exports['DriTestHsPlanReset'])
 if 'DriTestHsCoverageReset' in exports:call(exports['DriTestHsCoverageReset'])

def recruit8(rid,stage=3,offers=(),stars=3,cls=0,pos=0):
 r,p=rr(rid,score=1000,ovr=78,stage=stage,schools=((10,1000),(11,800),(12,600)),offers=offers,portal=False)
 records[r]['scalar'][2]=cls;records[p]['scalar'].update({3:pos,231:stars,261:255})
 return r,p

def snap8(r):
 return (dict(records[r]['scalar']),tuple(values(r)),
  tuple((tid,tuple(sorted(records[t]['scalar'].items())))for tid in range(138)
        for t in records[board_targets(allteams[tid])]['array']if records[t]['ref'].get(10)==r))

def recover8(fault=0):
 global next_heap
 before=next_heap
 call(exports['DriTestHs28RecoverMarket'],context)
 end=next_heap
 if end>before:uc.mem_write(before,bytes(end-before))
 next_heap=before
 assert call(exports['DriTestQualityFault'])==fault

reset8();r,p=recruit8(85000)
before=snap8(r);assert locked(r,96)==1
recover8()
assert records[r]['scalar'][9]==0 and locked(r,96)==0
assert values(r)==list(before[1]) and records[r]['scalar'][14]==0
assert not offer_status(r,96) and not native_events
assert records[r]['scalar']==before[0]|{9:0}
checks8.append('Full unsigned pool discovers zero-offer closed race; qualified outsider96 unlocks despite deadleader1000, every schoolidentity/influence and offercount unchanged')
prior=snap8(r);recover8();assert snap8(r)==prior
records[r]['scalar'][9]=3;prior=snap8(r);recover8();assert snap8(r)==prior
allowed8.add(97);recover8();assert records[r]['scalar'][9]==0 and values(r)==list(before[1])
checks8.append('Open race repeat is inert; same unchanged closed race never retries faileddestination, but a different feasible destination can rescue it')
# Model the subsequent native offer publication, then execute the same compiled
# role lookup consumed by funding. The new destination starts at zero points.
new_target=valid_offer(r,destination=allteams[97]);records[new_target]['scalar'][8]=0
assert call(exports['DriTestHsPlanRole'],r,allteams[97],context,0)==1
assert call(exports['DriTestHsPlanRole'],r,allteams[97],context,1)==40
assert records[new_target]['scalar'][8]==0 and values(r)==list(before[1])
checks8.append('A recovered outsider with zero earned points retains primary role and funding weight40 despite preserved deadleader1000; no influence is manufactured')


reset8();r,p=recruit8(85005,stage=0);prior=snap8(r)
recover8();assert snap8(r)==prior and call(exports['DriTestHs28WasRecovered'],r)==1
restored=valid_offer(r,destination=allteams[96]);records[restored]['scalar'][8]=0
assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==1
assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,1)==40
reset8();r,p=recruit8(85006,stage=0)
for e in records[records[r]['ref'][13]]['array']:records[e]['scalar'][1]=100
prior=snap8(r);recover8();assert snap8(r)==prior and call(exports['DriTestHs28WasRecovered'],r)==0
checks8.append('Reload-shaped progressed Top10 race reconstructs its outsider reservation without any save write; ordinary low-progress Top10 races remain untouched')

# Weekly reservations are transient, but a successful rescue is a real race.
# Retain its offered primary against a stronger viable alternative without
# manufacturing points; failed native admissions may try a new destination.
reset8();r,p=recruit8(85800);recover8();prior=snap8(r)
allowed8.add(97);records[season]['scalar'][4]+=1
recover8();assert snap8(r)==prior
assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[97],context,0)==15
assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[96],context,0)==0xffffffff
retry_target=valid_offer(r,destination=allteams[97]);records[retry_target]['scalar'][8]=0
assert call(exports['DriTestHsPlanRole'],r,allteams[97],context,0)==1
assert call(exports['DriTestHsPlanRole'],r,allteams[97],context,1)==40
assert not offer_status(r,96) and values(r)==list(prior[1])
checks8.append('Same-runtime next-week stage0 retry selects another qualified destination after failed native admission, without stage/interest/offer mutation')

reset8();r,p=recruit8(85805);allowed8.add(97);recover8();prior=snap8(r)
assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[96],context,0)==15
recover8();assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[97],context,0)==0xffffffff
call(exports['DriTestHsCoverageObserve'],allteams[96],context)
assert call(exports['DriTestHsRecoveryPending'],r,context)==0
recover8();assert snap8(r)==prior
assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[97],context,0)==15
call(exports['DriTestHsCoverageObserve'],allteams[97],context)
recover8();assert snap8(r)==prior and call(exports['DriTestHsRecoveryPending'],r,context)==0
checks8.append('Pending native pursuit suppresses duplicate recovery; observed failed admission releases it, permitting one alternative at the next safe scan and no unchanged retry after both schools fail')

for stars in (1,3):
 reset8();records[ideal_entries[0]]['scalar'][2]=1
 r,p=recruit8(85810+stars,stars=stars);recover8()
 t=valid_offer(r,destination=allteams[96]);records[t]['scalar'][8]=0
 assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==1
 rival,rp=recruit8(85820+stars,stage=0,stars=stars)
 top=records[records[rival]['ref'][13]]['array']
 records[top[0]]['scalar'].update({0:96,1:100})
 records[top[1]]['scalar'][1]=90;records[top[2]]['scalar'][1]=80
 records[rp]['scalar'][191]=90
 valid_offer(rival,destination=allteams[96])
 records[season]['scalar'][4]+=1;prior=snap8(r)
 recover8();assert snap8(r)==prior
 assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==1
 assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,1)==40
 assert call(exports['DriTestHsPlanRole'],rival,allteams[96],context,0)==2
 assert records[t]['scalar'][8]==0
checks8.append('Existing offered rescued primary retains continuity next week across lower/elite stars despite stale deadleader and stronger viable backup; protected live race receives no new recovery write')

for exclusion in ('unoffered','closed','ineligible','different_season'):
 reset8();records[ideal_entries[0]]['scalar'][2]=1
 r,p=recruit8(85830+len(exclusion));recover8()
 t=valid_offer(r,destination=allteams[96]);records[t]['scalar'][8]=0
 assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==1
 rival,rp=recruit8(85860+len(exclusion),stage=0)
 top=records[records[rival]['ref'][13]]['array']
 records[top[0]]['scalar'].update({0:96,1:100})
 records[top[1]]['scalar'][1]=90;records[top[2]]['scalar'][1]=80
 valid_offer(rival,destination=allteams[96])
 records[season]['scalar'][4]+=1
 if exclusion=='unoffered':records[t]['scalar'][12]=1
 if exclusion=='closed':records[r]['scalar'][9]=1
 if exclusion=='ineligible':allowed8.clear()
 if exclusion=='different_season':records[season]['scalar'][2]+=1
 assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)!=1,exclusion
checks8.append('Rescue continuity never protects an unoffered target, a newly closed shortlist, a native-ineligible pursuit or another season')

evidence8=json.loads((ROOT/'Tests/Fixtures/test28r7_abandoned_cases.json').read_text())
replays8=[]
for case in evidence8['cases']:
 reset8()
 stage={'Battle':3,'SoftCommitted':4}[case['stage']]
 r,p=rr(case['rid'],score=case['commit_score'],ovr=case['ovr'],stage=stage,
  schools=tuple(tuple(s)for s in case['top']),offers=(),portal=False)
 records[p]['scalar'].update({3:case['position_id'],231:3,261:255})
 before=snap8(r);recover8()
 assert records[r]['scalar']==before[0]|{9:0} and values(r)==list(before[1])
 assert records[r]['scalar'][14]==case['offers']==0 and not native_events
 replays8.append({'rid':case['rid'],'name':case['name'],'oldStage':case['stage'],
  'newStage':'Top10','influencePreserved':True,'offersCreated':0,
  'destinationConditions':'controlled eligible outsider with opening and resources'})
checks8.append('Recorded R7 EOS Hawk, Randall, Strange and Mond fields reopen under a qualified controlled destination, preserving their exact ten-school influence lists')

for problem in ('eligibility','hours','assigned','below_action_cost','full_board','closed_room'):
 reset8();r,p=recruit8(85010+len(problem))
 if problem=='eligibility':allowed8.clear()
 if problem=='hours':records[records[allteams[96]]['ref'][224]]['scalar'][3]=0
 if problem=='below_action_cost':records[records[allteams[96]]['ref'][224]]['scalar'][3]=14
 if problem=='assigned':records[records[allteams[96]]['ref'][224]]['scalar'][1]=1500
 if problem=='full_board':
  target_array=board_targets(allteams[96])
  for k in range(50):
   other=record('already secured unrelated target',{2:4,9:6})
   records[target_array]['array'].append(record('occupied board entry',{12:1},ref={10:other}))
 if problem=='closed_room':
  records[ideal_entries[0]]['scalar'][2]=1
  secured,sp=recruit8(85100+len(problem),stage=6,stars=2)
  records[records[allteams[96]]['ref'][33]]['array']=[sp]
 prior=snap8(r);recover8();assert snap8(r)==prior,(problem,snap8(r),prior)
checks8.append('No eligible school, no hours, fully assigned or below-action-cost budget, full board and filled opening are ordinary no-destination results with no race mutation or runtime fault')
reset8();r,p=recruit8(85118,stage=0)
board8=records[allteams[96]]['ref'][224];records[board8]['scalar'][3]=0
recover8();assert call(exports['DriTestHs28WasRecovered'],r)==0
records[board8]['scalar'][3]=1500;prior=snap8(r)
recover8();assert snap8(r)==prior and call(exports['DriTestHs28WasRecovered'],r)==1
checks8.append('A fresh pass sees actual newly available resources in the same week and resumes the abandoned open race without stale-budget rejection')


for tid,user in ((10,False),(10,True),(13,True)):
 reset8();r,p=recruit8(85120+tid,offers=(tid,));allowed8.add(tid)
 if user:records[allteams[tid]]['ref'][407]=record('user with valid offer')
 records[records[allteams[tid]]['ref'][224]]['scalar'][3]=0
 for t in records[board_targets(allteams[tid])]['array']:records[t]['scalar'][6]=0
 prior=snap8(r);recover8();assert snap8(r)==prior
checks8.append('One legal current CPU finalist protects race; eligible useroffer protects inside or outside stalecut even at zero weekly hours')
reset8();r,p=recruit8(85150,offers=(10,));records[allteams[10]]['ref'][407]=record('ineligible user')
old=values(r);recover8();assert records[r]['scalar'][9]==0 and values(r)==old and offer_status(r,10)
checks8.append('A native-ineligible former useroffer does not block legitimate abandonment repair; its recorded points/offer remain untouched')

for kind in ('published','roster','assigned','hard','signed'):
 reset8();r,p=recruit8(85200+len(kind),stage=5 if kind=='hard' else 6 if kind=='signed' else 4)
 if kind=='published':records[records[allteams[10]]['ref'][33]]['array']=[p]
 if kind=='roster':records[records[allteams[10]]['ref'][239]]['array']=[p]
 if kind=='assigned':records[p]['scalar'][261]=10
 prior=snap8(r);recover8();assert snap8(r)==prior
checks8.append('Published soft acquisitions, roster membership, ambiguous assigned-team records and hard/signed recruits never reopen')

for cls,pos,stars in ((0,1,0),(1,3,1),(2,10,2),(3,16,4)):
 reset8();r,p=recruit8(85300+cls,cls=cls,pos=pos,stars=stars)
 prior=values(r);recover8();assert records[r]['scalar'][9]==0 and values(r)==prior
checks8.append('All HS/JUCO classes and lower as well as elite star levels use full-pool recovery across positions')

reset8();records[ideal_entries[0]]['scalar'][2]=1
pool8=[recruit8(85400+i)for i in range(3)]
recover8();expected_paths=1 if (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)) else 2
assert sum(records[r]['scalar'][9]==0 for r,p in pool8)==expected_paths
if (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)):
 assert sum(call(exports['DriTestHsRecoveryPending'],r,context)!=0 for r,p in pool8)==1

assert all(records[r]['scalar'][14]==0 for r,p in pool8)
checks8.append('Three abandoned candidates sharing one opening: R12 reserves one primary path; a conditional backup cannot count as firm recovery. Legacy controls retain their original primary+backup expectation. No extra signing place or fabricated offer.')
reset8();records[records[allteams[96]]['ref'][224]]['scalar'][3]=29
pool8=[recruit8(85420+i)for i in range(2)]
recover8();assert sum(records[r]['scalar'][9]==0 for r,p in pool8)==1
checks8.append('Two potential pursuits share a 29-hour budget with 15-hour native action cost each: only one recovery reserves resources')


for typ,week in ((6,1),(6,7),(2,8),(3,0),(5,0)):
 reset8();r,p=recruit8(85500+typ+week);records[season]['scalar'].update({4:week,5:typ})
 prior=snap8(r);recover8();assert snap8(r)==prior
for wanted in (0,-1):
 reset8();r,p=recruit8(85600-wanted);configure(wanted);prior=snap8(r);recover8();assert snap8(r)==prior
checks8.append('Non-recruiting calendar windows and diagnostic/disabled modes do not mutate races')

for issue in ('duplicate_interest','invalid_points'):
 reset8();r,p=recruit8(85700+len(issue));entries=records[records[r]['ref'][13]]['array']
 if issue=='duplicate_interest':records[entries[1]]['scalar'][0]=10
 else:records[entries[0]]['scalar'][1]=65536
 prior=snap8(r);recover8(fault=1);assert snap8(r)==prior
checks8.append('Malformed top-school identities or influence abort before the first stage or interest write')

reset8();call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks8.append('All native SDF references balance; expected no-destination cases do not disable runtime')
(OUTPUT/'test28r8_abandoned_results.json').write_text(json.dumps({'checks':checks8,'saved_recruit_replays':replays8,'source_save_sha256':evidence8['source_sha256'],'mocked':['SDF storage','native eligibility/scoring verdicts','Windows APIs'],'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks8,indent=2))
