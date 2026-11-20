"""Compiled class planning on137 captured saved roster cases; not a simulation."""
from pathlib import Path
src=(Path(__file__).parent/'test15r1_real_rosters_test.py').read_text()
exec(compile(src.split('if baseline_mode:')[0],'saved-roster definitions','exec'))
current=json.loads(gzip.decompress((Path(__file__).parent/'Fixtures/test28r14_nsd_rosters.json.gz').read_bytes()))
fixture['sources']=[{'phase':'R13R1_NSD','season':current['season']}]
results=[];out=alloc(73*4)
for case in current['cases']:
 players,recruits,before=load_case(case)
 assert call(exports['DriTest29ClassPlan'],team,eval_ptr,out)==1,case['team']
 value=list(struct.unpack('<73i',uc.mem_read(out,73*4)))
 target,secured,replacement,available,planned,shortfall,exception=value[:7]
 assert secured==sum(case['expected_hs']),(case['team'],secured)
 assert replacement==sum(max(0,i-r)for i,r in zip(case['ideal'],case['expected_returners']))
 academy=case['tid']in[0,7,55]
 assert (15<=target<=25)or(academy and exception and target==replacement)
 assert planned<=available and shortfall==max(0,target-secured-planned)
 assert sum(value[7:51])==planned
 for p,n in enumerate(value[7:29]):assert 0<=n<=max(0,case['ideal'][p]-case['expected_counts'][p])
 assert all(records[p]['scalar']==v for p,v in before.items())
 assert call(exports['DriTestQualityFault'])==0
 results.append({'team':case['team'],'tid':case['tid'],'target':target,'actual_secured':secured,'replacement':replacement,'available_native_slots_assuming_candidates':available,'planned':planned,'unmet_plan':shortfall,'replacement_exception':bool(exception),'passed':True})
 clear_case()
assert len(results)==137
(OUTPUT/'test29_class_rosters_results.json').write_text(json.dumps({'source_save_sha256':current['source_save_sha256'],'cases':results,'checks':'Actual archived R13R1 NSD roster rows through compiled Test29 snapshot/class planner; candidate availability optimistic only; no native signing simulation','live_game_verified':False},indent=2)+'\n')
print('PASS Test29 class planner137 archivedCPUrosters: native accounting preserved; capacity limits explicit')
