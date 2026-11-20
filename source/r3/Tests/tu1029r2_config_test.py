"""Replay the PRE roster collector with resolved FTC data and missing-row controls.

Uses actual compiled R1/R2 runtime code with controlled SDF/Windows storage.
This is not a native game load or a season simulation.
"""
from pathlib import Path
import copy, json, struct
source=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(source.split('# Execute the ACTUAL captured native removal function.')[0],
             str(Path(__file__).parent/'runtime_fixture_test.py'),'exec'))
audit=ROOT.parent/'Audit/TU1029R2'
dependency=json.loads((audit/'ftc_dependency_verification.json').read_text())
configs={t['team']:t for t in dependency['routes'][0]['teams']}
rosters=json.loads((audit/'Inputs/saved_rosters.json').read_text())['teams']
positions='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
years=['Freshman','Sophomore','Junior','Senior']
stars={'ONE_STAR':0,'TWO_STAR':1,'THREE_STAR':2,'FOUR_STAR':3,'FIVE_STAR':4,'Invalid':6}
records[hs_fixture_class]['array']=[]
records[hs_fixture_portal]['array']=[]
records[hs_fixture_committed]['array']=[]
records[hs_fixture_departures]['array']=[]
records[season]['scalar'].update({2:2026,4:0,5:0})
records[tuning]['scalar'][39]=2
out=alloc(157*4)
checks=[]
for saved in rosters:
    tid=saved['tid'];cfg=configs[tid]
    call(exports['DriTestOwnershipReset']);configure(1)
    records[team]['scalar'].update({383:tid,384:saved['prestige']})
    records[roster]['array']=[]
    for p in saved['roster']:
        records[roster]['array'].append(record('Saved PRE player '+str(p['pid']),
            {3:positions.index(p['pos']),191:p['ovr'],231:stars[p['stars']],
             242:years.index(p['year']),261:p['tid']},row_id=p['pid']))
    for pos,e in enumerate(cfg['entries']):
        v=e['values'];records[ideal_entries[pos]]['scalar'].update(
            {0:v['AdditionalStarterCount'],2:v['IdealNumberOnRoster'],4:v['StarterCount']})
    before=copy.deepcopy(records)
    assert call(exports['DriTestHsSnapshot'],team,eval_ptr,out)==1,tid
    assert call(exports['DriTestQualityFault'])==0,tid
    result=list(struct.unpack('<157i',uc.mem_read(out,157*4)))
    returned=[p for p in saved['roster'] if p['year']!='Senior' and p['tid']==tid]
    counts=[sum(p['pos']==pos for p in returned) for pos in positions]
    ideals=[e['values']['IdealNumberOnRoster'] for e in cfg['entries']]
    assert result[:22]==ideals,(tid,'ideals')
    assert result[22:44]==counts and result[44:66]==counts,(tid,'returners')
    assert result[66:132]==[0]*66,(tid,'invented incoming')
    assert result[154]==sum(p['pos']=='QB' and min(99,p['ovr']+2)>=85 for p in returned),(tid,'QB')
    assert result[155]==len(returned) and result[156]==sum(ideals),(tid,'total')
    reserved=sum(max(c,i) for c,i in zip(counts,ideals))
    flex=[int(sum(ideals) in [83,84] and reserved<85 and pos not in [0,19,20] and i>0 and c==i)
          for pos,(c,i) in enumerate(zip(counts,ideals))]
    assert result[132:154]==flex,(tid,'flex')
    assert records==before,(tid,'record mutation')
    # Resolve the same tuning against the older 71-entry league table.
    removed=[]
    for pos,e in enumerate(cfg['entries']):
        if e['row']>=71:
            group=offense if pos<10 else defense if pos<19 else config
            field=field_ids[pos];removed.append((group,field,records[group]['ref'].pop(field)))
    assert removed,tid
    assert call(exports['DriTestHsSnapshot'],team,eval_ptr,out)==0,(tid,'old table accepted')
    assert call(exports['DriTestQualityFault'])==0,(tid,'collector fault side effect')
    for group,field,value in removed:records[group]['ref'][field]=value
    assert records==before,(tid,'negative control mutation')
    call(exports['DriTestOwnershipReset'])
    assert all(v==1 for v in refs.values()),(tid,'reference balance')
    checks.append({'team':tid,'name':saved['name'],'rosterPlayers':len(saved['roster']),
                   'projectedReturners':len(returned),'totalIdeal':sum(ideals),
                   'restoredAccepted':True,'olderRejected':True,
                   'missingEntries':len(removed),'all157OutputsMatched':True})
assert len(checks)==138
result={'passed':True,'schools':138,'positiveCases':138,'negativeCases':138,
        'savedPlayers':sum(x['rosterPlayers'] for x in checks),'checks':checks,
        'method':'Compiled production collector with saved roster values and resolved FTC configuration; controlled SDF/Windows services',
        'liveGameVerified':False,'limits':['No native FTC loader or live season simulated.','PRE has no secured incoming players; retained collector fixtures separately cover other source guards.']}
(OUTPUT/'tu1029r2_config_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='checks'}))
