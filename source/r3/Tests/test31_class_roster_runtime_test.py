"""Actual compiled collector and annual planner on138 saved PRE roster rows.
Foreign recruiting sources are empty controlled fixtures; this is accounting
and plan consistency, not a full-save/native signing simulation.
"""
from pathlib import Path
import gzip
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'class31 fixture','exec'))
fixture=json.loads(gzip.decompress((Path(__file__).parent/'Fixtures/test31_pre_rosters.json.gz').read_bytes()))
pos='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split();year={'Freshman':0,'Sophomore':1,'Junior':2,'Senior':3}
annual_out=alloc(46*4);results=[]
for case in fixture['teams']:
 reset();tid=case['tid'];records[team]['scalar'][383]=tid
 records[teams]['array']=dummy_teams.copy();records[teams]['array'][tid]=team
 for p,e in enumerate(ideal_entries):records[e]['scalar'][2]=case['ideal'][pos[p]]
 players=[];expected=[0]*22
 for row in case['players']:
  p=record('captured PRE roster player',{3:pos.index(row['position']),191:row['ovr'],231:6,242:year[row['year']],261:row['team']},row_id=row['ref'])
  players.append(p)
  if year[row['year']]<3 and row['team']==tid:expected[pos.index(row['position'])]+=1
 records[roster]['array']=players
 before={p:dict(records[p]['scalar']) for p in players}
 observed=snapshot();assert list(observed['returning'])==expected,(tid,observed['returning'],expected)
 assert observed['total']==sum(expected) and sum(observed['hs'])==0
 assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
 values=list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)));assert 15<=values[0]<=25 and sum(values[1:23])==values[0]
 actual_plan=values[:23]
 # Removing every returner cannot change normal intake or turn it into catch-up.
 records[roster]['array']=[]
 assert call(exports['DriTest31ClassAnnual'],team,context,annual_out)==1
 assert list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)))[:23]==actual_plan
 assert all(records[p]['scalar']==v for p,v in before.items())
 assert call(exports['DriTestQualityFault'])==0
 results.append({'tid':tid,'team':case['name'],'saved_roster_rows':len(players),'native_projected_returners':sum(expected),'annual_target':values[0],'normal_places':dict(zip(pos,values[1:23])),'passed':True})
 call(exports['DriTestOwnershipReset'])
assert len(results)==138 and all(v==1 for v in refs.values())
r={'passed':True,'inputs':fixture['inputs'],'cases':results,'scope':'Actual saved roster metadata and schemes through compiled collector; foreign pools empty, future departures not simulated, eligibility unchanged','native_action_execution':False,'live_game_verified':False}
(OUTPUT/'test31_class_roster_runtime_results.json').write_text(json.dumps(r,indent=2)+'\n');print('PASS compiled collector and stable annual plan across138 saved PRE rosters')
