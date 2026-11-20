"""Compiled R4 acceptance/witness/callback regression, controlled records.
No saved game is mutated and no actual signing rate is inferred.
"""
from pathlib import Path
src=(Path(__file__).parent/'test29r2_acceptance_test.py').read_text()
exec(compile(src.split('# Hosley-shaped:')[0],__file__,'exec'))

# An already secured primary does not close an independently useful HB3.
reset();records[ideal_entries[1]]['scalar'][2]=6
records[roster]['array']=[pp('HB returner '+str(i),v,pos=1)for i,v in enumerate([82,74,70,68,66])]
first,ft,fp=candidate(7101,92,stage=5)
second,st,sp=candidate(7102,88,stage=5)
fallback,bt,bp=candidate(7103,86)
records[board]['scalar'][1]=15
plan([(first,1,1,1),(fallback,1,1,2),(second,1,2,1)])
assert call(exports['DriTestPortalDecision'],fallback,team,context)==0
assert call(exports['DriTestCommitBlock'],fallback,team,context,1)==0
assert firm(fallback)==1
assert witness(fallback)==11  # still planned as a backup, despite legal coexistence
assert execution(bt)[5:]==[0,1,1]
records[fallback]['scalar'][9]=5
redundant,rt,rp=candidate(7104,84)
records[board]['scalar'][1]=20
assert call(exports['DriTestPortalDecision'],redundant,team,context)==4
assert firm(redundant)==0
checks.append('Full plus one HB room admits an additional real HB3 through compiled acceptance/execution; secured arrivals remain counted; subsequent HB4 rejected')

# Old primary preference is conditional only when the two cannot coexist.
reset();records[ideal_entries[1]]['scalar'][2]=5
records[roster]['array']=[pp('HB incumbent '+str(i),v,pos=1)for i,v in enumerate([82,74,70,68,67])]
strong,st,sp=candidate(7201,87,stage=0);lower,lt,lp=candidate(7202,80,stage=0)
for r,t,points in [(strong,st,129),(lower,lt,130)]:
 records[r]['scalar'][3]=136;records[t]['scalar'][8]=points
 records[records[records[r]['ref'][13]]['array'][0]]['scalar'][1]=points
records[board]['scalar'][1]=10
plan([(strong,1,1,1),(lower,1,1,2)])
assert firm(strong)==1 and firm(lower)==1
assert call(exports['DriTest29R1Capacity'],lower,team,context,1)==0
assert witness(lower)==11
assert execution(lt)[5:]==[0,1,1]
records[strong]['scalar'][9]=5
assert firm(lower)==1
call(exports['DriTestOwnershipReset']);call(exports['DriTestCommitReset'])
assert firm(lower)==1
checks.append('A below-threshold native NSD alternative remains acceptable alongside a better paid primary when both fit useful rotation; actual arrival and reload preserve this')

# Prestige-floor guard is independent of merely improving a low-end reserve.
reset();records[ideal_entries[3]]['scalar'].update({2:12,4:3})
records[team]['scalar'][384]=10
records[roster]['array']=[pp('WR returner '+str(i),v,pos=3)for i,v in enumerate([94,90,87,82,78,76,72,71,70,69,68,67])]
secured,ut,up=candidate(7301,84,pos=3,stage=5)
useful,vt,vp=candidate(7302,81,pos=3)
bench,xt,xp=candidate(7303,74,pos=3)
records[board]['scalar'][1]=15
plan([(useful,3,1,1),(bench,3,1,2)])
assert call(exports['DriTestPortalDecision'],useful,team,context)==0 and firm(useful)==1
assert call(exports['DriTestPortalDecision'],bench,team,context)==4 and firm(bench)==0
assert witness(useful)==7
checks.append('Compiled already-overfull prestige-10 WR room admits useful rotation but rejects deeper bench; native-funded primary witness agrees')

game_api(0x0816b130,lambda:0)
assert firm(useful)==0
game_api(0x0816b130,fixture_eligibility)
assert firm(useful)==1
records[vt]['scalar'][15]=0
assert witness(useful)==1
records[vt]['scalar'][15]=1;records[vt]['scalar'][12]=0
assert witness(useful)==0
records[vt]['scalar'][12]=3
for academy in [0,7,55]:
 records[team]['scalar'][383]=academy
 assert call(exports['DriTestPortalDecision'],useful,team,context)==5
records[team]['scalar'][383]=TEST_TEAM_ID
checks.append('Native eligibility, productive assignment, displayed offer and academy restrictions remain independent mandatory gates; no records changed by observations')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'r4_quality_depth_runtime.json').write_text(json.dumps({'checks':checks,'method':'compiled callbacks with controlled SDF/native eligibility services','live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
