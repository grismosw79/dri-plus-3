"""Compiled R5 budget regression. Native ownership helper; mocked SDF/services.
No live-game outcome or signing-volume prediction is made by these tests.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'hs15_runtime_test.py').read_text(),'hs15_runtime_test.py','exec'))
checks=[]
reset()
records[team]['scalar'][384]=10
for pos,e in enumerate(ideal_entries):records[e]['scalar'].update({2:4 if pos<21 else 0,0:0,4:1})
records[ideal_entries[3]]['scalar'][2]=10
for pos,n in [(3,12),(5,4),(9,4),(8,4)]:
 records[roster]['array'] += [player('inherited '+str(pos)+'/'+str(i),pos=pos,ovr=70) for i in range(n)]
first,fp=prospect(pos=5,ovr=76,stars=2)
second,sp5=prospect(pos=9,ovr=75,stars=2)
third,tp5=prospect(pos=8,ovr=74,stars=2)
before=[list(records[a]['array']) for a in [roster,committed,hslist,tplist,arr]]
assert call(exports['DriTestHsDecision'],first,team,context)==0
assert before==[list(records[a]['array']) for a in [roster,committed,hslist,tplist,arr]]
checks.append('Two inherited WR overages do not consume general places before any HS commitment; decision is read-only')
records[first]['scalar'][9]=5
assert call(exports['DriTestHsDecision'],second,team,context)==0
records[second]['scalar'][9]=5
assert call(exports['DriTestHsDecision'],third,team,context)==8
checks.append('Two new HS overages consume both general places; a third cannot borrow emergency or RB exceptions')
for r,p in [(first,fp),(second,sp5)]:
 records[r]['scalar'][9]=6;records[committed]['array'].append(p)
call(exports['DriTestOwnershipReset'])
records[season]['scalar'][4]+=1
assert call(exports['DriTestHsDecision'],third,team,context)==8
assert snapshot()['hs'][5]==1 and snapshot()['hs'][9]==1
checks.append('Publication, next week, and a cold ownership cache reconstruct the same used allowance without double-counting')
vacancy,vp=prospect(pos=10,ovr=78,stars=3)
assert call(exports['DriTestHsDecision'],vacancy,team,context)==0
checks.append('A genuine vacancy remains legal even with both general places spent')
records[committed]['array'].remove(fp);records[first]['scalar'][9]=1
assert call(exports['DriTestHsDecision'],third,team,context)==0
checks.append('Only removing an actual acquisition replenishes its general place')
for m in [0,-1]:
 configure(m);assert call(exports['DriTestHsDecision'],third,team,context)==0
configure(1)
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test28r5_hs_budget_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
