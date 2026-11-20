"""Compiled saved-shaped recovery through advisory stages, rollover and reload.
Uses captured shortlist code with controlled SDF/native score/eligibility. Not
an entire saved-league replay, native spending, or live Windows validation.
"""
from pathlib import Path
import os,json,struct
# Repeated full-market callbacks release their Windows scratch allocations.
# Preserve that lifetime in this long fixture rather than accumulating every
# freed allocation in the permanent SDF record bump arena.
os.environ.setdefault('DRI_TEST_REUSE_NATIVE_HEAP','1')
src=(Path(__file__).parent/'test28r13_open_lists_test.py').read_text()
exec(compile(src.split('checks=[]')[0],'R13 fixture','exec'))
heap_fixture_checks=fixture_native_heap_self_test() if fixture_native_heap else {'legacy_negative_control':True}
entry=call(exports['DriTestHsShortlistHook'])
uc.mem_write(GAME+0x0862d990,b'\xff\x25\0\0\0\0'+struct.pack('<Q',entry)+b'\x90\x90')
fixed=modern_r14 or b'DRI_PORTAL_TEST28R13R1' in image
checks=[];observations=[]
def begin_case(rid=98100):
 reset8();r,p=recruit8(rid);recover8()
 records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 call(exports['DriTestHsCoverageBegin'],context)
 target=valid_offer(r,destination=allteams[96]);records[target]['scalar'].update({8:0,15:1})
 records[records[allteams[96]]['ref'][224]]['scalar'][1]=5
 return r,p,target
for persistent in [False,True]:
 r,p,target=begin_case(98100+int(persistent))
 if persistent:
  call(exports['DriTestHsCoverageObserve'],allteams[96],context)
  records[season]['scalar'][4]+=1;recover8();call(exports['DriTestHsCoverageBegin'],context)
 for stage in range(5):
  records[r]['scalar'][9]=stage;before=snap8(r)
  role=call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)
  state=call(exports['DriTestHs29Offer'],r,allteams[96],context)
  call(exports['DriTestHsCoverageObserve'],allteams[96],context)
  funded=call(exports['DriTestHsCoverageCount'],r,0)
  observations.append({'persistent':persistent,'stage':stage,'funded_coverage':funded,'role':role,'offer_state':state})
  assert role==1 and state==2 and locked(r,96)==0 and snap8(r)==before
  assert funded==(1 if fixed or stage==0 else 0),(persistent,stage,funded)
checks.append('Current-week reservation and next-week observed-offer continuity survive Top10/5/3/Battle/Soft without any point, offer, stage or budget mutation')
if not fixed:
 print('R13_NEGATIVE_REPRODUCED: advisory stage alone erases valid funded coverage while firm offer and shared lockout remain correct')
else:
 # Real W9 shapes: fresh valid offer begins well behind withdrawn leaders.
 cases=json.loads((Path(__file__).parent/'Fixtures/test28r13r1_w9_recovery.json').read_text())
 for case in cases:
  reset29();allowed8.clear();allowed8.add(case['destination'])
  r,p=make(case['rid'],tuple(tuple(x) for x in case['top']),[],ovr=case['ovr'],stage=0)
  records[p]['scalar'].update({3:case['position'],231:3});records[r]['scalar'][3]=case['commit_score']
  records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
  recover8();call(exports['DriTestHsCoverageBegin'],context)
  t=allteams[case['destination']];target=valid_offer(r,destination=t)
  records[target]['scalar'].update({8:case['points'],15:1});records[records[t]['ref'][224]]['scalar'][1]=5
  for stage in range(4):
   records[r]['scalar'][9]=stage;before=snap8(r)
   call(exports['DriTestHsCoverageObserve'],t,context)
   assert call(exports['DriTestHsCoverageCount'],r,0)==1,(case['name'],stage)
   assert snap8(r)==before
  observations.append({'saved_shape':case['name'],'rid':case['rid'],'stage_checks':4,'all_funded':True})
 print('PASS saved W9 shapes',flush=True)
 checks.append('Booker, von Oelhoffen, Pryor and Koehler W9 interest/OVR/position/commit-score shapes keep qualified funded pursuits at all four preference stages')
 # Scope-loss restart: both in-progress actions and completed-week delta
 # reconstruct saved evidence. An unexecuted/unoffered proposal cannot do so.
 for evidence in ['action','earned_delta','none','no_offer']:
  r,p,target=begin_case(98120+len(evidence));records[r]['scalar'][9]=2
  call(exports['DriTestHsCoverageObserve'],allteams[96],context)
  records[target]['scalar'][15]=int(evidence=='action')
  records[target]['scalar'][7]=61 if evidence=='earned_delta' else 0
  if evidence=='no_offer':records[target]['scalar'][12]=0
  call(exports['DriTestHs28RecoveryReset']);call(exports['DriTestHsCoverageReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHs29Reset'])
  before=snap8(r)
  # Inspect the plan before coordinator recovery can reserve a new path.
  call(exports['DriTestHsCoverageBegin'],context)
  planned=call(exports['DriTestHsCoverageCount'],r,1)
  assert planned==(1 if evidence in ['action','earned_delta'] else 0),('reload_planned',evidence,planned)
  if evidence=='earned_delta':records[target]['scalar'][15]=1 # next native pass assigns effort
  call(exports['DriTestHsCoverageObserve'],allteams[96],context)
  reload_count=call(exports['DriTestHsCoverageCount'],r,0)
  assert reload_count==(1 if evidence in ['action','earned_delta'] else 0),('reload',evidence,reload_count)
  assert values(r)==list(before[1])
  assert offer_status(r,96)==(evidence!='no_offer')
 print('PASS saved-evidence reload cases',flush=True)
 checks.append('Reload with native productive action or completed-week earned delta reconstructs continuity; a proposal without either and an unoffered target do not manufacture funded coverage')
 for loss in ['eligibility','withdrawn_offer','hard_commit','published','roster']:
  r,p,target=begin_case(98200+len(loss));records[r]['scalar'][9]=2
  call(exports['DriTestHsCoverageObserve'],allteams[96],context)
  if loss=='eligibility':allowed8.clear()
  elif loss=='withdrawn_offer':records[target]['scalar'][12]=0
  elif loss=='hard_commit':records[r]['scalar'][9]=5
  elif loss=='published':records[records[allteams[96]]['ref'][33]]['array']=[p]
  else:
   records[records[allteams[96]]['ref'][239]]['array']=[p];records[p]['scalar'][261]=96
  before=snap8(r)
  call(exports['DriTestHsCoverageEnd'],context)
  assert snap8(r)==before
  if loss in ['eligibility','withdrawn_offer']:assert call(exports['DriTestHsCoverageCount'],r,0)==0,loss
  else:assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==0,loss
 checks.append('Continuity cannot bypass native eligibility, withdrawn offers, hard commitments, published ownership or roster ownership')
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0 and all(v==1 for v in refs.values())
result={'heap_fixture_checks':heap_fixture_checks,'native_heap':fixture_native_heap.report() if fixture_native_heap else None,'fixed':fixed,'checks':checks,'observations':observations,'live_game_verified':False,'source_save_sha256':'893e96bba96b9515727238a14f382cbcf34163a4e25225436b580b47f8d1fcaa'}
(OUTPUT/'test28r13r1_continuity_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
