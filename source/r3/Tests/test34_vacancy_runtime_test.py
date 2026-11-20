"""Compiled collectors/acceptance against saved-shaped Navy and shared-budget cases.
SDF storage and native scoring/eligibility are controlled; no live signing.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'class fixture','exec'))
TEST_TEAM_ID=55
navy_ideals=[4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0]
annual_out=alloc(46*4);checks=[]
def prep(tid=55):
 global TEST_TEAM_ID
 TEST_TEAM_ID=tid
 reset();records[teams]['array']=dummy_teams.copy();records[teams]['array'][tid]=team
 for entry,ideal in zip(ideal_entries,navy_ideals):records[entry]['scalar'][2]=ideal
 records[roster]['array']=[player('HB returner',pos=1,ovr=70,tid=tid) for _ in range(3)]
 old,_=prospect(pos=1,ovr=74,stars=3,stage=5)
 candidate,_=prospect(pos=1,ovr=74,stars=3,stage=0)
 for year in range(2026,2066):
  records[season]['scalar'][2]=year
  assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
  values=list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)))
  if values[2]==1:break
 else:raise AssertionError('No one-HB annual fixture year')
 assert 15<=values[0]<=25 and values[24]==1
 if tid==55:assert values[0]==18 and year==2026
 return old,candidate,values
old,r,v=prep();before=snapshot()
assert before['count'][1]==4 and before['ideal'][1]==5
assert call(exports['DriTestHsDecision'],r,team,context)==0
assert snapshot()==before
assert call(exports['DriTestHsPlanRole'],r,team,context,0)==1
assert call(exports['DriTestHsPlanRole'],r,team,context,2)==63
game_api(0x0816b130,lambda:0)
call(exports['DriTestHsPlanReset']);call(exports['DriTest29ClassInvalidate'],team)
assert call(exports['DriTestHsPlanRole'],r,team,context,0)==0
game_api(0x0816b130,fixture_eligibility)
call(exports['DriTestHsPlanReset']);call(exports['DriTest29ClassInvalidate'],team)
assert call(exports['DriTestHsPlanRole'],r,team,context,0)==1
call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestHsDecision'],r,team,context)==0
records[r]['scalar'][9]=5
assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
assert ri(annual_out+45*4)==(1<<6)
next_r,_=prospect(pos=1,ovr=74,stars=3,stage=0)
assert call(exports['DriTestHsDecision'],next_r,team,context)!=0
call(exports['DriTestOwnershipReset'])
assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
assert ri(annual_out+45*4)==(1<<6)
checks.append('Navy annual1 / HS1 / HB4-of5 permits one shared optional repair; secured extra charges general and survives ownership reset; third HB refused')
old,r,v=prep()
for pos in [6,8]:
 for _ in range(v[1+pos]+1):prospect(pos=pos,ovr=60,stars=1,stage=5)
assert call(exports['DriTestHsDecision'],r,team,context)==8
checks.append('Two actual below-ideal extras elsewhere exhaust general budget, including a zero-quota position')
# Academies cannot acquire portal players. Use an ordinary school for this
# mixed-source case rather than treating a forbidden Navy portal offer as an
# actual acquisition that spends allowance.
old,r,v=prep()
records[roster]['array'] += [player('C full',pos=7,ovr=60,tid=55) for _ in range(3)]
prospect(pos=7,ovr=60,stars=1,stage=5,portal=True)
assert snapshot()['count'][7]==3 and snapshot()['hs'][7]==0
assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
assert ri(annual_out+45*4)==1
checks.append('Forbidden academy portal hard-stage target is not invented as an owned player or spent allowance')
old,r,v=prep(2)
records[roster]['array'] += [player('C full',pos=7,ovr=60,tid=2) for _ in range(3)]
prospect(pos=7,ovr=60,stars=1,stage=5,portal=True)
assert snapshot()['count'][7]==4 and snapshot()['hs'][7]==0
assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
assert ri(annual_out+45*4)==(1<<6)
assert call(exports['DriTestHsDecision'],r,team,context)==0
for _ in range(v[7]+1):prospect(pos=6,ovr=60,stars=1,stage=5)
assert call(exports['DriTestHsDecision'],r,team,context)==8
checks.append('Ordinary school: secured portal overage and one other HS extra leave no general permission for HS vacancy repair')
old,r,v=prep()
for _ in range(81):records[roster]['array'].append(player('other returning roster place',pos=3,ovr=60,tid=55))
assert snapshot()['total']==85 and call(exports['DriTestHsDecision'],r,team,context)!=0
checks.append('Projected total85 blocks new vacancy exception')
old,r,v=prep()
for _ in range(v[0]-1):prospect(pos=3,ovr=60,stars=1,stage=5)
assert call(exports['DriTestHsDecision'],r,team,context)!=0
checks.append('Class already at target blocks vacancy exception')
old,r,v=prep();records[records[r]['ref'][5]]['scalar'][231]=4
assert call(exports['DriTestHsDecision'],r,team,context)==20
checks.append('Academy five-star prohibition still applies before vacancy permission')
call(exports['DriTestOwnershipReset'])
assert all(n==1 for n in refs.values())
result={'passed':True,'checks':checks,'native_eligibility_modeled':True,'native_signing_executed':False,'live_game_verified':False}
(OUTPUT/'test34_vacancy_runtime_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
