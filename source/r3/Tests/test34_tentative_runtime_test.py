"""Actual compiled choice-reader continuity classification, controlled native services."""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'Test34 fixture','exec'))
checks=[]
reset();r,p=prospect(pos=1,stars=3,stage=0);target=records[arr]['array'][-1]
top=records[records[r]['ref'][13]]['array'];own=next(x for x in top if records[x]['scalar'][0]==TEST_TEAM_ID)
records[target]['scalar'].update({12:0,7:0,8:0})
records[own]['scalar'][1]=100
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==1
checks.append('Seeded interest on an unoffered board proposal does not establish paid continuity')
records[target]['scalar'][12]=2
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==2
records[board]['scalar'][1]=0
call(exports['DriTestHsPlanReset'])
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==2
checks.append('Actual offered positive interest retains conservative continuity through assigned-hour reset and cold plan reload')
records[own]['scalar'][1]=0
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==1
records[target]['scalar'][7]=3
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==2
checks.append('An offered proposal without interest, gain or payment is tentative; observed native gain establishes continuity')
records[target]['scalar'][7]=0
records[tuning]['ref'][49]=record('Test34 productive actions',array=[record('social',{0:5,1:10,2:5,5:1,6:0})])
records[target]['scalar'][15]=1
records[board]['scalar'].update({1:5,3:100})
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==2
records[board]['scalar'][1]=0
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==1
records[board]['scalar'].update({1:5,3:4})
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==1
checks.append('Positive productive assignment establishes continuity only within the native assigned/total budget; stale flags and over-budget ledgers do not')
records[board]['scalar'].update({1:0,3:100});records[r]['scalar'][9]=4
assert call(exports['DriTestHs34PursuitState'],r,target,team,context)==2
checks.append('Soft commitment never becomes tentative merely because its points/gain are zero')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test34_tentative_runtime_results.json').write_text(json.dumps({'passed':True,
 'checks':checks,'method':'Compiled production choice reader, controlled records and native services',
 'native_action_execution':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
