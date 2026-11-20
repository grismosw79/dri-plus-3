"""Compiled full-board transition, all ordinary primaries retained.
SDF, native scoring and eligibility leaves are modeled; no live game signing.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'class fixture','exec'))
observed34=json.loads((Path(__file__).parent/'Fixtures/test34r2_navy_w5.json').read_text())
TEST_TEAM_ID=55
navy_ideals=[4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0]
annual_out=alloc(46*4);plan_out=alloc((4+128*4)*4);checks=[]
def annual():
 assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
 return list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)))
def rid(r):return ri(r+0x18)
def plan():
 n=call(exports['DriTest34R2PlanSnapshot'],team,context,plan_out)
 assert 0<=n<=128,n
 a=list(struct.unpack('<'+str(4+n*4)+'i',uc.mem_read(plan_out,(4+n*4)*4)))
 return a[:4],{a[i]:a[i+1:i+4] for i in range(4,len(a),4)}
def prepare(tid=55,pos=1):
 global TEST_TEAM_ID
 TEST_TEAM_ID=tid;reset();call(exports['DriTestHsPlanReset']);call(exports['DriTest29ClassInvalidate'],team)
 records[teams]['array']=dummy_teams.copy();records[teams]['array'][tid]=team
 records[season]['scalar'].update({2:2026,4:5,5:6})
 for entry,ideal in zip(ideal_entries,navy_ideals):records[entry]['scalar'][2]=ideal
 # All actions are native-shaped; observation cannot manufacture funding.
 records[tuning]['ref'][49]=record('productive actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 records[board]['scalar'].update({1:5,2:0,3:800})
 v=annual();q=v[1:23];ordinary=[];hard=[];normal=None
 for p,count in enumerate(q):
  for k in range(count):
   # Navy actual hard commitments apart from Roseberry: TE, RT, LOLB.
   stage=5 if tid==55 and p in [4,9,13] and k==0 else 0
   if p==pos:stage=5 if k<count-1 else 0
   r,_=prospect(pos=p,ovr=74 if p==pos else 68,stars=3 if p==pos else 2,stage=stage)
   if stage==5:hard.append(r)
   else:ordinary.append(r)
   if p==pos and stage==0:normal=r
 # If this calendar map has zero normal places, it is already filled.
 candidate,_=prospect(pos=pos,ovr=74,stars=3,stage=0)
 records[roster]['array']=[player('room returner',pos=pos,ovr=70,tid=tid) for _ in range(navy_ideals[pos]-q[pos]-1)]
 # Provisional total88 in the Navy post-commit shape, with HB exactly4/5.
 # Other returners are irrelevant to annual permission, but over85 matters.
 for _ in range(88-len(hard)-(1 if normal else 0)-len(records[roster]['array'])):
  records[roster]['array'].append(player('other position returner',pos=3 if pos!=3 else 12,ovr=60,tid=tid))
 if tid==55 and pos==1:
  records[roster]['array']=[player('observed Navy returner',pos=p,ovr=70 if p==1 else 60,tid=55) for p,n in enumerate(observed34['returning_by_position']) for _ in range(n)]
 return v,ordinary,normal,candidate
v,ordinary,normal,r=prepare();assert v[0]==18 and v[2]==1
# Stronger earned points keep Roseberry the primary; Cutrera is his backup.
for e in records[records[normal]['ref'][13]]['array']:
 if records[e]['scalar'][0]==55:records[e]['scalar'][1]=650
for e in records[records[r]['ref'][13]]['array']:
 if records[e]['scalar'][0]==55:records[e]['scalar'][1]=452
before,bp=plan();assert before==[18,3,15,0],before
assert bp[rid(normal)][0]==1 and bp[rid(r)][0]==2,bp
assert len([x for x in bp.values() if x[0]==1])==15
# Native observed commitment is represented by the recorded stage transition;
# production hooks below must replan without a fabricated commitment or score.
records[normal]['scalar'][9]=5
call(exports['DriTest29ClassInvalidate'],team)
state,roles=plan();assert state==[19,4,15,0],state
assert roles[rid(r)][:2]==[1,63],roles[rid(r)]
remaining=[x for x in ordinary if x!=normal]
assert len(remaining)==14 and all(roles[rid(x)][0]==1 for x in remaining)
assert sorted(records[records[x]['ref'][5]]['scalar'][3] for x in remaining)==sorted(observed34['remaining_primary_positions'])
assert snapshot()['total']==88 and snapshot()['count'][1]==4
assert call(exports['DriTestHsDecision'],r,team,context)==0
assert call(exports['DriTestHsRecoveryDestination'],r,team,context,2)==1
assert call(exports['DriTestHsRecoveryDestination'],r,team,context,1)==63
checks.append('Navy observed W5-shaped class4/18, roster88, HB4/5: Roseberry commitment promotes Cutrera from backup to optional primary, preserving all14 other ordinary primaries; recovery agrees')
# The plan is not influence, payment, offer publication or ownership.
saved={x:dict(records[x]['scalar']) for x in [r,records[r]['ref'][5],board]}
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert plan()[0]==[19,4,15,0]
assert all(records[x]['scalar']==value for x,value in saved.items())
checks.append('Cold ownership/plan reset reconstructs the same capacity without changing recruit, player or budget scalars')
# Native refusal must remove the repair; other14 ordinary pursuits remain.
game_api(0x0816b130,lambda:0 if rq(reg('R8'))==r else fixture_eligibility())
call(exports['DriTestHsPlanReset']);call(exports['DriTest29ClassInvalidate'],team)
state,roles=plan();assert state==[18,4,14,0],state
assert roles[rid(r)][0]==0 and all(roles[rid(x)][0]==1 for x in remaining)
game_api(0x0816b130,fixture_eligibility)
call(exports['DriTestHsPlanReset']);call(exports['DriTest29ClassInvalidate'],team)
assert plan()[0]==[19,4,15,0]
checks.append('Native ineligibility prevents added place and leaves14 ordinary primaries intact; restoring eligibility restores bounded repair')
# An actual second HB acquisition consumes one general place, keeps the
# ordinary plan and survives reconstruction. A third HB cannot use this rule.
records[r]['scalar'][9]=5
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
state,roles=plan();assert state==[19,5,14,0],state
assert all(roles[rid(x)][0]==1 for x in remaining)
assert annual()[45]==(1<<6)
third,_=prospect(pos=1,ovr=74,stars=3,stage=0)
assert call(exports['DriTestHsDecision'],third,team,context)!=0
checks.append('Actual optional arrival retains target19/class5 and all14 normal primaries; shared allowance spent once after reload; third HB refused')
# General path at non-Navy schools and multiple line/skill/defensive positions.
for tid,pos in [(2,1),(13,3),(96,6),(7,12),(10,16),(100,18)]:
 v,ordinary,normal,r=prepare(tid,pos)
 if normal:records[normal]['scalar'][9]=5
 call(exports['DriTest29ClassInvalidate'],team)
 state,roles=plan();assert state[0]==v[0]+1,(tid,pos,v[0],state)
 assert roles[rid(r)][:2]==[1,pos*32+31],(tid,pos,roles[rid(r)])
 assert all(roles[rid(x)][0]==1 for x in ordinary if x!=normal),(tid,pos)
 assert call(exports['DriTestHsDecision'],r,team,context)==0,(tid,pos)
checks.append('Other school IDs2/13/96/7/10/100 and HB/WR/LG/DT/CB/SS retain their complete normal maps alongside the same extra-primary path')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);assert all(n==1 for n in refs.values())
result={'passed':True,'checks':checks,'native_leaves_modeled':True,'native_commitment_executed':False,'live_game_verified':False}
(OUTPUT/'test34_r2_full_plan_runtime_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
