"""All ten observed PLV quota/extra violations through the compiled collector.
Exact annual schemes and secured class compositions; native eligibility modeled.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'class fixture','exec'))
data=json.loads((Path(__file__).parent/'Fixtures/test34r3_observed.json').read_text());checks=[]
positions='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
stars=['ONE_STAR','TWO_STAR','THREE_STAR','FOUR_STAR','FIVE_STAR']
for case in data['cases']:
 TEST_TEAM_ID=case['tid'];reset();call(exports['DriTestHsPlanReset']);records[teams]['array']=dummy_teams.copy();records[teams]['array'][TEST_TEAM_ID]=team
 for entry,ideal in zip(ideal_entries,case['ideal']):records[entry]['scalar'][2]=ideal
 for sr in case['secured']:
  hard,p=prospect(pos=positions.index(sr['position']),ovr=sr['ovr'],stars=stars.index(sr['stars']),stage=6)
  records[committed]['array'].append(p)
 for item in case['earlier_same_batch_admissions']:
  sr=item['recruit'];prospect(pos=positions.index(sr['position']),ovr=sr['ovr'],stars=stars.index(sr['stars']),stage=5)
 cr=case['candidate'];r,p=prospect(pos=positions.index(cr['position']),ovr=cr['ovr'],stars=stars.index(cr['stars']),stage=4)
 records[r]['scalar'][3]=cr['commitScore'];records[board]['scalar'].update({1:0,2:0,3:1000})
 # Keep full inherited roster size out of the admission decision.
 records[roster]['array']=[player('unrelated returning WR',pos=3,ovr=60) for _ in range(85)]
 result=[]
 for typ,week in [(1,16),(2,17),(6,0),(6,1),(6,2),(6,6),(6,7)]:
  records[season]['scalar'].update({2:2026,4:week,5:typ});call(exports['DriTest29ClassInvalidate'],team)
  before=dict(records[r]['scalar']);reason=call(exports['DriTest34R3Admission'],r,team,context)
  assert reason and records[r]['scalar']==before,(case['school'],typ,week,reason)
  result.append({'type':typ,'week':week,'reason':reason})
 checks.append({'school':case['school'],'observed_recruit':cr['name'],'annual':case['annual'],'secured':len(case['secured']),'rejected_in_each_native_outcome_phase':result})
 # Already acquired owner is honored even if it would not be accepted now.
 records[r]['scalar'][9]=5
 assert call(exports['DriTest34R3Admission'],r,team,context)==0
 records[r]['scalar'][9]=6;records[committed]['array'].append(p)
 before=dict(records[r]['scalar']);assert call(exports['DriTest34R3Admission'],r,team,context)==13
 assert records[r]['scalar']==before and p in records[committed]['array']
checks.append('Hard acquired owners remain accepted; published HS ownership is retained and duplicate publication stays blocked')
# Ordinary intake remains allowed above total85, including the exact closed
# allocation periods. No additional allocation window is introduced.
TEST_TEAM_ID=55;reset();call(exports['DriTestHsPlanReset']);records[teams]['array']=dummy_teams.copy();records[teams]['array'][55]=team
records[roster]['array']=[player('inherited returning player',pos=3,ovr=60) for _ in range(90)]
r,p=prospect(pos=0,stars=2,ovr=68,stage=3)
records[board]['scalar'].update({1:0,2:0,3:1000})
for typ,week in [(2,17),(6,0),(6,1),(6,2),(6,7)]:
 records[season]['scalar'].update({2:2026,4:week,5:typ});call(exports['DriTestHsPlanReset'])
 before=dict(records[board]['scalar']);assert call(exports['DriTest34R3Admission'],r,team,context)==0
 assert records[board]['scalar']==before
checks.append('Legal normal QB intake remains permitted with90 inherited returners at every phase, without creating hours or offers')
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);assert call(exports['DriTestQualityFault'])==0 and all(v==1 for v in refs.values())
(OUTPUT/'test34_r3_phase_runtime_results.json').write_text(json.dumps({'passed':True,'checks':checks,'native_leaves_modeled':True,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
