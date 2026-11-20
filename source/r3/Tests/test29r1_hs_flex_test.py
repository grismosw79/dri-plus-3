"""Compiled five-star development and actionable-list continuity."""
from pathlib import Path
import json
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'HS fixture','exec'))
reset29();t=allteams[10]
records[season]['scalar'].update({4:4,5:6})
# Actual LE vacancy, one secured five-star in that exact position.
records[ideal_entries[10]]['scalar'][2]=4
first,fp=make(1241,((10,1000),),[10],80,5);records[fp]['scalar'].update({3:10,231:4})
second,sp=make(1242,((10,1000),),[10],80,0);records[sp]['scalar'].update({3:10,231:4})
assert call(exports['DriTestHsDecision'],second,t,context)==0
records[second]['scalar'][9]=5
third,tp=make(1243,((10,1000),),[10],81,0);records[tp]['scalar'].update({3:10,231:4})
assert call(exports['DriTestHsDecision'],third,t,context)==2
# Shared across groups: an existing second LE consumes the class permission.
one,op=make(1244,((10,1000),),[10],79,5);records[op]['scalar'].update({3:16,231:4})
two,tp2=make(1245,((10,1000),),[10],80,0);records[tp2]['scalar'].update({3:16,231:4})
assert call(exports['DriTestHsDecision'],two,t,context)==2
call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestHsDecision'],two,t,context)==2,'reload cannot replenish'
reset29()
r,p=make(1290,((10,966),(11,914),(12,639),(13,1000)),[13],80,2)
points_before=dict(values(r));call(exports['DriTest29R1HsOrder'],r,context)
assert dict(values(r))==points_before and values(r)[0]==(13,1000)
assert records[r]['scalar'][9]==2
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'r1_hs.json').write_text(json.dumps({'second_five_real_vacancy':True,'third_blocked':True,'one_per_school_across_groups':True,'reload_reconstructed':True,'actionable_order_points_preserved':True,'live_game_verified':False},indent=2))
print('PASS bounded five-star class allowance; viable offer leads without invented interest')
