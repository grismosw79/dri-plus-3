"""Compiled R14 roster accounting from actual R13R1 NSD saved player rows.
SDF wrappers are synthetic; expected counts come from the independent NSD audit.
"""
from pathlib import Path
src=(Path(__file__).parent/'test15r1_real_rosters_test.py').read_text()
exec(compile(src.split('if baseline_mode:')[0],'saved-roster definitions','exec'))
current=json.loads(gzip.decompress((Path(__file__).parent/'Fixtures/test28r14_nsd_rosters.json.gz').read_bytes()))
fixture['sources']=[{'phase':'R13R1_NSD','season':current['season']}]
results=[]
for case in current['cases']:
 players,recruits,before=load_case(case)
 assert call(exports['DriTestHsSnapshot'],team,eval_ptr,snapshot_out)==1,case['team']
 actual=list(struct.unpack('<157i',uc.mem_read(snapshot_out,157*4)))
 for label,values,expected in [('position totals',actual[22:44],case['expected_counts']),('returners',actual[44:66],case['expected_returners']),('HS arrivals',actual[66:88],case['expected_hs'])]:
  assert values==expected,(case['team'],label,values,expected)
 assert actual[155]==case['expected_total']
 assert all(records[p]['scalar']==v for p,v in before.items())
 assert call(exports['DriTestQualityFault'])==0
 results.append({'team':case['team'],'tid':case['tid'],'projected_total':actual[155],'passed':True})
 clear_case()
assert len(results)==137
(OUTPUT/'test28r14_saved_rosters_results.json').write_text(json.dumps({'source_save_sha256':current['source_save_sha256'],'cases':results,'checks':'All 137 CPU schools: projected totals, returners and HS arrivals match independent saved-roster audit; player fields unchanged; refs balanced','live_game_verified':False},indent=2)+'\n')
print('PASS 137 actual NSD CPU rosters: totals, returners, HS arrivals, unchanged player fields')
