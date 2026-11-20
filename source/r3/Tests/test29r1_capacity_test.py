"""Controlled compiled acceptance regression for better paid portal alternatives."""
from pathlib import Path
import json
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'portal fixture','exec'))
records[tuning]['ref'][49]=record('actions',array=[record('social',{0:5,1:10,2:5,5:1,6:0})])
# R4 allows both paid candidates when they coexist in useful quality depth.
# Neither needs to spend the old scarce shared allowance. At NSD both remain
# below threshold; the separate native final selector still owns signing.
records[ideal_entries[1]]['scalar'][2]=5
records[roster]['array']=[pp('HB returner',v,pos=1)for v in [82,74,70,68,67]]
strong,st,sp=pr(4317,87,pos=1,stage=0);lower,lt,lp=pr(3725,80,pos=1,stage=0)
for r,t,points in [(strong,st,129),(lower,lt,130)]:
 valid_offer(r,t);records[r]['scalar'][3]=136;records[t]['scalar'].update({15:1,8:points})
 records[records[r]['ref'][13]]['array']=records[records[r]['ref'][13]]['array'][:1]
 for e in records[records[r]['ref'][13]]['array']:records[e]['scalar'].update({0:TEST_TEAM_ID,1:points})
records[board]['scalar'][1]=10
assert call(exports['DriTest29R1Capacity'],lower,team,context,1)==0
assert call(exports['DriTest29R1Capacity'],strong,team,context,1)==0
records[st]['scalar'][12]=0
assert call(exports['DriTest29R1Capacity'],lower,team,context,1)==0
records[st]['scalar'][12]=3;records[st]['scalar'][15]=0
assert call(exports['DriTest29R1Capacity'],lower,team,context,1)==0
# R3 supersedes recruiting own entrants: all stars blocked until restoration.
records[team]['scalar'][383]=0
records[sp]['scalar'][230]=0
records[ideal_entries[1]]['scalar'][2]=6
for star in range(5):
 records[sp]['scalar'][231]=star
 assert call(exports['DriTestPortalDecision'],strong,team,context)==5
records[sp]['scalar'][230]=55
assert call(exports['DriTestPortalDecision'],strong,team,context)==5
records[sp]['scalar'][230]=255
assert call(exports['DriTestPortalDecision'],strong,team,context)==5
# Reload-safe ownership recognizes a real accepted return, while a phantom
# academy leader without an offer remains unowned.
records[sp]['scalar'][230]=0
records[teams]['array']=fixture_team_array()
records[records[strong]['ref'][13]]['array']=[record('own academy',{0:0,1:129})]
records[strong]['scalar'][9]=5
owner=alloc(4)
assert call(exports['DriTestHardOwner'],strong,context,owner)==1 and ri(owner)==0
call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestHardOwner'],strong,context,owner)==1 and ri(owner)==0
call(exports['DriTestOwnershipReset']);records[st]['scalar'][12]=0
assert call(exports['DriTestHardOwner'],strong,context,owner)==0
records[team]['scalar'][383]=TEST_TEAM_ID
records[teams]['array']=fixture_team_array()
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'r1_capacity.json').write_text(json.dumps({'same_position_paid_material_upgrade':True,'R4_quality_coexistence_replaces_shared_capacity_veto':True,'withdrawal_does_not_remove_quality_permission':True,'academy_origin_all_star_levels':True,'cross_academy_blocked':True,'academy_hard_owner_reload_offer_required':True,'live_game_verified':False},indent=2))
print('PASS R4 useful HB coexistence; academy pursuit prohibition and real hard-owner reload checks retained')
