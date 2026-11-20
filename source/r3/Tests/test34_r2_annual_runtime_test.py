"""Actual DLL collector/acceptance/source budget, controlled SDF; no live outcomes."""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'test31 fixture','exec'))
checks=[];annual_out=alloc(46*4)
def annual():
 assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
 assert call(exports['DriTestQualityFault'])==0
 return list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)))
reset();records[team]['scalar'][383]=10;records[teams]['array'][TEST_TEAM_ID]=dummy_teams[TEST_TEAM_ID];records[teams]['array'][10]=team
plan=annual();quota=plan[1:23];assert 15<=plan[0]<=25 and sum(quota)==plan[0]
records[roster]['array']=[player('full returner',pos=p,ovr=65,tid=10) for p in range(22) for _ in range(ideals[p])]
assert annual()==plan
incoming=[]
for pos,n in enumerate(quota):
 for k in range(n):
  r,p=prospect(pos=pos,ovr=70,stars=3 if pos==3 else 2);top=records[records[r]['ref'][13]]['array'];records[top[0]]['scalar'][0]=10
  before=dict(records[r]['scalar']);budget=dict(records[board]['scalar'])
  assert call(exports['DriTestHsDecision'],r,team,context)==0,(pos,k,quota)
  assert records[r]['scalar']==before and records[board]['scalar']==budget
  records[r]['scalar'][9]=5;incoming.append((r,p))
  assert annual()[45]==1,'Normal development consumed a discretionary allowance'
assert annual()[23:45]==quota
checks.append('Every normal annual place is accepted through the compiled production HS collector on a full returning roster, including distinct four-star WR places; actual hard commitments consume slots and preserve all shared allowances')
# Completion closes ordinary intake. An eligible HB..SS may use one extra
# per position, limited by the unchanged shared general allowance.
records[roster]['array']=[]
for pos in range(22):
 if not ideals[pos]:continue
 r,p=prospect(pos=pos,ovr=60,stars=1);records[records[records[r]['ref'][13]]['array'][0]]['scalar'][0]=10
 decision=call(exports['DriTestHsDecision'],r,team,context)
 if 1<=pos<=18:
  assert decision==0,pos
 else:assert decision!=0,pos
checks.append('Annual completion leaves the ordinary map unchanged; HB..SS may request only the bounded general extra while QB and specialists retain their separate exclusions')
for r,p in incoming:records[r]['scalar'][9]=6;records[p]['scalar'][261]=10;records[committed]['array'].append(p)
call(exports['DriTestOwnershipReset']);assert annual()[23:45]==quota and annual()[45]==1
checks.append('Publication and cold ownership-cache reload reproduce the same annual consumption and source-budget state')
# Current year/metadata and budgets remain unmodified by observation.
assert records[season]['scalar'][2]==2026
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
r={'passed':True,'checks':checks,'native_services':'Controlled SDF and eligibility; actual compiled policy and captured ownership helpers','native_action_execution':False,'live_game_verified':False}
(OUTPUT/'test34_r2_annual_runtime_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
