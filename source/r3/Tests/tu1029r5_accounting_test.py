"""Current HS/JUCO accounting: retained Test15 cases with the shipped R1 five-star vacancy exception. Mocked SDF, no live game."""
exec(compile((__import__('pathlib').Path(__file__).parent/'runtime_fixture_test.py').read_text(),
             'runtime_fixture_test.py','exec'))

checks=[]
context=eval_ptr;franchise=hs_fixture_franchise
hslist=hs_fixture_class;tplist=hs_fixture_portal
committed=hs_fixture_committed;departures=hs_fixture_departures
empty=record('snapshot empty source',array=[])
empty_board=record('snapshot empty board',ref={0:empty})
dummy_teams=[record('snapshot team '+str(tid),{383:tid,384:5},
                   ref={33:empty,224:empty_board,239:empty,64:config}) for tid in range(138)]
teams=record('snapshot team array',array=dummy_teams.copy())
records[teams]['array'][TEST_TEAM_ID]=team
records[franchise]['ref'][45]=teams
out=alloc(157*4);next_rid=41000

def reset():
    call(exports['DriTestOwnershipReset']);configure(1)
    records[team]['ref'].pop(407,None)
    records[team]['scalar'].update({383:TEST_TEAM_ID,384:8})
    records[season]['scalar'].update({2:2026,4:4,5:1})
    records[tuning]['scalar'][39]=0
    for entry,ideal in zip(ideal_entries,ideals):
        records[entry]['scalar'].update({0:0,2:ideal,4:1})
    for a in [roster,arr,committed,hslist,tplist,departures]:records[a]['array']=[]

def player(name,pos=9,ovr=65,stars=1,year=1,tid=TEST_TEAM_ID):
    return record(name,{3:pos,191:ovr,231:stars,242:year,261:tid})

def prospect(pos=9,ovr=75,stars=2,stage=1,portal=False,rank=1):
    global next_rid
    next_rid+=1;r,t=recruit(next_rid,pos,rank,250,stage=stage)
    p=records[r]['ref'][5]
    records[p]['scalar'].update({191:ovr,231:stars,242:1 if portal else 0,261:255})
    records[r]['scalar'].update({2:5 if portal else 0,14:1})
    records[tplist if portal else hslist]['array'].append(r)
    records[arr]['array'].append(t)
    return r,p

def snapshot():
    assert call(exports['DriTestHsSnapshot'],team,context,out)==1
    assert call(exports['DriTestQualityFault'])==0
    data=struct.unpack('<157i',uc.mem_read(out,157*4))
    return {name:data[i*22:(i+1)*22] for i,name in enumerate(
        ['ideal','count','returning','hs','four','five','flex'])} | {
        'qb85':data[154],'total':data[155],'total_ideal':data[156]}

reset()
keep=player('eligible returner');senior=player('graduating',year=3)
wrong=player('already transferred',tid=255);draft=player('confirmed early departure')
records[roster]['array']=[keep,senior,wrong,draft]
records[departures]['array']=[record('declared departure',{1:0,2:1},ref={4:draft})]
s=snapshot();assert s['count'][9]==1 and s['returning'][9]==1 and s['total']==1
checks.append('Projection includes eligible returners and excludes graduates, wrong-team roster entries, and confirmed early departures')

# Native ProspectQuality.Invalid is6. It is legal historical metadata on a
# current college player, and is irrelevant to current HS/JUCO class star caps.
reset()
mathews=player('Jacoby Mathews actual Auburn values',pos=18,ovr=76,stars=6,year=2)
records[roster]['array']=[mathews]
before=dict(records[mathews]['scalar']);s=snapshot()
assert s['count'][18]==s['returning'][18]==1 and s['hs'][18]==0
assert s['four'][18]==s['five'][18]==0 and records[mathews]['scalar']==before
current_hs,hp=prospect(pos=18,stars=4,stage=5)
s=snapshot();assert s['count'][18]==2 and s['hs'][18]==s['five'][18]==1
second_hs,_=prospect(pos=18,stars=4)
assert call(exports['DriTestHsDecision'],second_hs,team,context)==0  # Existing R1 developmental vacancy exception
checks.append('Actual Jacoby Mathews unset historical star enum6 counts as an eligible returner without rewriting it; the existing R1 second-five-star developmental vacancy remains allowed')

reset()
transfer,tp=prospect(pos=18,stars=6,stage=5,portal=True)
before=dict(records[tp]['scalar']);s=snapshot()
assert s['count'][18]==1 and s['hs'][18]==s['four'][18]==s['five'][18]==0
assert records[tp]['scalar']==before
records[transfer]['scalar'][9]=6;records[committed]['array']=[tp]
s=snapshot();assert s['count'][18]==1 and s['hs'][18]==0
checks.append('Unset stars on valid hard/signed portal arrivals count and deduplicate without consuming HS star allowances or changing player metadata')

reset()
bad_hs,hp=prospect(pos=18,stars=6,stage=5)
records[committed]['array']=[hp]
before=dict(records[hp]['scalar'])
assert call(exports['DriTestHsSnapshot'],team,context,out)==0
assert records[hp]['scalar']==before
checks.append('A published player subsequently classified as a current HS/JUCO recruit still requires valid star metadata; publication cannot bypass current-class validation')

reset()
hard,hp=prospect(stage=5,stars=4)
signed,sp=prospect(stage=6,stars=3)
soft,_=prospect(stage=4,stars=4)
rival,_=prospect(stage=5,stars=4,rank=2)
unknown,_=prospect(stage=6,stars=4)
portal,pp15=prospect(stage=5,portal=True,stars=4)
records[committed]['array']=[hp,sp,pp15]
s=snapshot()
assert s['count'][9]==3 and s['hs'][9]==2 and s['four'][9]==1 and s['five'][9]==1
assert s['returning'][9]==0 and s['total']==3
# A saved hard commitment remains known when native advances it to Signed,
# even before Team.CommittedPlayers publishes that identity.
records[hard]['scalar'][9]=6
records[committed]['array']=[sp,pp15]
s=snapshot();assert s['count'][9]==3 and s['hs'][9]==2 and s['five'][9]==1
checks.append('Hard/Signed HS and portal players deduplicate across saved class and published commits; soft/rival/unknown Signed players do not consume vacancies or elite allowances; known ownership survives native Signed transition')

reset()
r,unowned=prospect(stage=6,stars=4)
records[unowned]['scalar'].update({242:1,261:TEST_TEAM_ID})
records[roster]['array']=[unowned]
s=snapshot();assert s['count'][9]==1 and s['returning'][9]==1 and s['hs'][9]==0
checks.append('Being on a roster alone does not invent Signed HS ownership or consume the current class five-star allowance')

reset();records[tuning]['scalar'][39]=2
records[roster]['array']=[player('projected elite returning QB',pos=0,ovr=83)]
hs,hp=prospect(pos=0,ovr=83,stage=5)
portal,pp15=prospect(pos=0,ovr=83,stage=5,portal=True)
s=snapshot();assert s['count'][0]==3 and s['qb85']==2
assert s['hs'][0]==1 and s['returning'][0]==1
checks.append('Shared elite QB count uses one development step for returners/portal arrivals and current rating for incoming HS/JUCO')

reset()
# An 84-place scheme has exactly one flexible addition available. Filling a
# genuine shortage does not spend it; an existing surplus does.
assert sum(ideals)==74
records[ideal_entries[3]]['scalar'][2]+=10
records[roster]['array']=[player('RT full '+str(i)) for i in range(ideals[9])]
records[roster]['array'] += [player('LG full '+str(i),pos=6) for i in range(ideals[6])]
s=snapshot();assert s['total_ideal']==84 and s['flex'][9]==1 and s['flex'][6]==1
assert all(s['flex'][pos]==0 for pos in [0,2,19,20,21])
records[roster]['array'].append(player('one surplus RT'))
s=snapshot();assert not any(s['flex'])
checks.append('83/84 scheme flex reserves every genuine positional need first and consumes the global 85-place budget when one 84-scheme position gains a surplus; QB, K, P, and unused positions are excluded')

reset()
invalid,_=prospect();records[invalid]['scalar'][2]=8
assert call(exports['DriTestHsDecision'],invalid,team,context)==0
records[invalid]['scalar'][2]=0
records[ideal_entries[9]]['scalar'][2]=0
assert call(exports['DriTestHsDecision'],invalid,team,context)==1
for stage_type,week in [(2,4),(6,1),(6,7)]:
    records[season]['scalar'].update({5:stage_type,4:week})
    assert call(exports['DriTestHsDecision'],invalid,team,context)==0
records[season]['scalar'].update({5:1,4:4})
records[team]['ref'][407]=record('human user')
assert call(exports['DriTestHsDecision'],invalid,team,context)==0
checks.append('Unknown classes and other calendar stages do not enter HS policy; an unused position closes for CPU in recruiting windows but leaves human recruiting untouched')

call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
checks.append('Snapshot and decision reads balance every fixture record reference')
result={'checks':checks,'method':'compiled Test15 Windows callbacks with captured native ownership helper and mocked SDF/Windows','live_game_verified':False}
(OUTPUT/'tu1029r5_accounting_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
