"""Compiled Test15R1 roster accounting against saved player rows from 138 teams.

The fixture retains every roster/published player and source-owned secured
arrival for each team. Its compact franchise omits disjoint foreign identities;
cross-team ownership resolution is tested by the separate ownership suites.
This is a saved-data accounting regression, not a live-game simulation.
"""
import contextlib,gzip,hashlib,io,subprocess
from pathlib import Path

# Reuse captured native helpers and the SDF harness without changing its tests.
with contextlib.redirect_stdout(io.StringIO()):
    exec(compile((Path(__file__).parent/'runtime_fixture_test.py').read_text(),
                 'runtime_fixture_test.py','exec'))

fixture_path=Path(__file__).parent/'Fixtures/test15r1_real_rosters.json.gz'
fixture=json.loads(gzip.decompress(fixture_path.read_bytes()))
assert fixture['version']==1 and len(fixture['cases'])==552
assert all(len([c for c in fixture['cases'] if c['phase']==p])==138
           for p in ['TP1','TP3','TP4','NSD'])
baseline_mode='--baseline-auburn' in sys.argv
franchise=hs_fixture_franchise
teams=record('real snapshot compact team array',array=[team])
records[franchise]['ref'][45]=teams
snapshot_out=alloc(157*4)
snapshot_arrays=[roster,arr,hs_fixture_committed,hs_fixture_class,
                 hs_fixture_portal,hs_fixture_departures]
saved_record_keys=set(records);saved_table_keys=set(tables);heap_mark=next_heap

def load_case(case):
    """Populate actual fields; only array/table wrappers are synthetic."""
    call(exports['DriTestOwnershipReset']);configure(1)
    for a in snapshot_arrays:records[a]['array']=[]
    records[team]['ref'].pop(407,None)
    records[team]['scalar'].update({383:case['tid'],384:5})
    season_source=next(x['season'] for x in fixture['sources'] if x['phase']==case['phase'])
    assert season_source['CurrentWeekType']=='OffSeason'
    records[season]['scalar'].update({2:season_source['CurrentSeasonYear'],
        4:season_source['CurrentWeek'],5:6})
    records[tuning]['scalar'][39]=fixture['projection_step']
    for entry,ideal in zip(ideal_entries,case['ideal']):
        records[entry]['scalar'].update({0:0,2:ideal,4:1})
    player_table=alloc(0x20);wq(player_table,TABLEVT);tables[player_table]={}
    players={}
    for pid,pos,ovr,stars,year,tid in case['players']:
        p=record('real player '+str(pid),{3:pos,191:ovr,231:stars,242:year,261:tid},row_id=pid)
        # Real save player identities share one native table. This also makes
        # early-departure and duplicate identity checks exercise real semantics.
        wq(p+0x10,player_table);tables[player_table][pid]=p;players[pid]=p
    records[roster]['array']=[players[p] for p in case['roster']]
    records[hs_fixture_committed]['array']=[players[p] for p in case['published']]
    records[hs_fixture_departures]['array']=[record('real early departure',
        {1:2,2:1},ref={4:players[p]}) for p in case['departures']]
    recruits={}
    source_recruits=case['recruits']+([case['trigger']] if case['trigger'] else [])
    for rid,pid,stage,kind,leader,offer in source_recruits:
        leaders=[] if leader==255 else [record('real leading school',{0:leader,1:500})]
        top=record('real top list',array=leaders)
        r=record('real recruit '+str(rid),{2:kind,3:500,9:stage,14:int(offer>=2)},
                 ref={5:players[pid],13:top},row_id=rid)
        recruits[rid]=r
        target=record('real target '+str(rid),{12:offer},ref={10:r})
        records[arr]['array'].append(target)
        records[hs_fixture_class if kind<=3 else hs_fixture_portal]['array'].append(r)
    before={p:dict(records[p]['scalar']) for p in players.values()}
    return players,recruits,before

def clear_case():
    global next_heap
    call(exports['DriTestOwnershipReset'])
    assert all(x==1 for x in refs.values()),[(records[r]['name'],x) for r,x in refs.items() if x!=1]
    for a in snapshot_arrays:records[a]['array']=[]
    for r in set(records)-saved_record_keys:
        del records[r];del refs[r]
    for t in set(tables)-saved_table_keys:del tables[t]
    uc.mem_write(heap_mark,bytes(next_heap-heap_mark));next_heap=heap_mark

def exercise_case(case,negative=False):
    players,recruits,before=load_case(case)
    success=call(exports['DriTestHsSnapshot'],team,eval_ptr,snapshot_out)
    result={'phase':case['phase'],'tid':case['tid'],'team':case['team'],
            'snapshot_success':success,'expected_total':case['expected'][155]}
    if negative:
        assert case['phase']=='TP1' and case['tid']==8
        assert records[players[13984]]['scalar'][231]==6
        assert success==0,'Original shipped Test15 must reproduce the Auburn failure'
        result['decision']=call(exports['DriTestHsDecision'],recruits[1038],team,eval_ptr)
        result['faulted']=call(exports['DriTestQualityFault'])
        assert result['decision']==7 and result['faulted']==1,result
    else:
        assert success==1,(case['phase'],case['team'],'snapshot failed')
        actual=list(struct.unpack('<157i',uc.mem_read(snapshot_out,157*4)))
        assert actual==case['expected'],(case['phase'],case['team'],
            [(i,a,b) for i,(a,b) in enumerate(zip(actual,case['expected'])) if a!=b])
        result['actual_total']=actual[155]
        if case['trigger']:
            result['decision']=call(exports['DriTestHsDecision'],recruits[1038],team,eval_ptr)
            assert result['decision']==0,result
            result['unset_star_preserved']=records[players[13984]]['scalar'][231]==6
        result['faulted']=call(exports['DriTestQualityFault'])
        assert result['faulted']==0,result
    assert all(records[p]['scalar']==v for p,v in before.items()),'Player metadata was rewritten'
    clear_case()
    return result

if baseline_mode:
    case=next(c for c in fixture['cases'] if c['phase']=='TP1' and c['tid']==8)
    result=exercise_case(case,negative=True)
    (OUTPUT/'test15r1_real_rosters_negative_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));sys.exit(0)

# Require a separately compiled fixture from the exact original shipment. This
# proves the old-code failure rather than merely asserting the patched result.
baseline=os.environ.get('DRI_TEST15_BASELINE_DLL')
assert baseline and Path(baseline).is_file(),'Set DRI_TEST15_BASELINE_DLL to the original shipped Test15 fixture'
assert Path(baseline).name=='retention_fixture.dll'
env=os.environ.copy();env['DRI_TEST_OUTPUT_DIR']=str(Path(baseline).resolve().parent)
child=subprocess.run([sys.executable,str(Path(__file__).resolve()),'--baseline-auburn'],
                     env=env,capture_output=True,text=True)
assert child.returncode==0,(child.stdout,child.stderr)
negative=json.loads((Path(baseline).parent/'test15r1_real_rosters_negative_results.json').read_text())
cases=[exercise_case(case) for case in fixture['cases']]
result={'checks':[
    'Original shipped Test15 rejects the actual Auburn TP1 roster and returns a faulting HS decision',
    'Repaired compiled DLL exactly matches all 157 accounting outputs for 138 teams at each of TP1, TP3, TP4 and NSD',
    'Actual Auburn soft-commit decision resumes without a fault; Jacoby Mathews retains native Invalid star enum6',
    'All player fields remain unchanged; SDF reference counts balance after every case'],
    'case_count':len(cases),'negative_control':negative,
    'auburn_fixed':next(c for c in cases if c['phase']=='TP1' and c['tid']==8),
    'method':fixture['method'],'fixture_sha256':hashlib.sha256(fixture_path.read_bytes()).hexdigest(),
    'sources':fixture['sources'],'cases':cases,'live_game_verified':False}
(OUTPUT/'test15r1_real_rosters_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='cases'},indent=2))
