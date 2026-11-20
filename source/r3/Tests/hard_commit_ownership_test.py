"""Regression for native hard-commit -> Signed conversion before list publication.
Runs compiled callbacks and the captured Signed-state writer with mocked storage.
The seventy room snapshots are actual Test04 false admissions, not predictions.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'signing_ownership_test.py').read_text(),'signing_ownership_test.py','exec'))
import os
hard_checks=[]
reset();records[roster]['array']=[pp('returning QB',74)]
r,t,p=pr(7000,83,stage=5);listed(r,TEST_TEAM_ID)
prior=quality();assert prior[:6]==(0,4,1,2,1,83)
native_signed(r)
now=quality()
if os.environ.get('DRI_EXPECT_TEST04_FAILURE')=='1':
 assert now[:6]==(0,4,1,1,1,-1),now[:6]
 (OUTPUT/'hard_commit_original_failure.json').write_text(json.dumps({'reproduced':True,'before':prior[:6],'after':now[:6],'expected_after':prior[:6]},indent=2)+'\n')
 print('REPRODUCED: original Test04 loses confirmed hard commitment after native Signed write before publication')
 raise SystemExit(0)
assert now[:6]==prior[:6],now[:6]
second,_,_=pr(7001,85);assert decide(second)==2
hard_checks.append('Existing portal hard commit retains owner through native Signed write with no signing callback and no published list')

# The stage-5 owner is authoritative even if a later stored list changes.
listed(r,1);assert quality()[:6]==prior[:6]
hard_checks.append('Remembered confirmed hard-commit destination does not become a ghost at a later stale list leader')

# Last allowed weekly commit: no subsequent roster scan in stage 5.
def weekly_attempt(rec,condition=1):
 records[transition]['scalar'][1]=5
 wq(osp+0x38,transition);wq(obp+0x68,rec);wq(obp+0x60,context);wq(of+14*8,condition)
 call(weekly_cb,of);return rq(of+14*8)
for is_portal in [True,False]:
 reset();records[roster]['array']=[pp('returner',74)]
 r,t,p=pr(7010+is_portal,83 if is_portal else 72,stage=4,portal=is_portal);listed(r,TEST_TEAM_ID)
 assert weekly_attempt(r)==1
 assert quality()[:6]==(0,4,1,1,1,-1)  # permission alone adds no player
 records[r]['scalar'][9]=5
 native_signed(r)  # deliberately no intervening stage-5 observation
 expect=(0,4,1,2,1,83) if is_portal else (0,4,1,2,2,-1)
 assert quality()[:6]==expect,(is_portal,quality()[:6])
 hard_checks.append(('Portal' if is_portal else 'HS/JUCO')+' last weekly hard commitment survives direct finalization; pending permission alone is not a roster addition')

reset();records[roster]['array']=[pp('returner',74)]
h,_,hp=pr(7020,72,stage=5,portal=False);listed(h,TEST_TEAM_ID)
assert quality()[:6]==(0,4,1,2,2,-1)
native_signed(h);assert quality()[:6]==(0,4,1,2,2,-1)
hard_checks.append('Previously secured HS/JUCO commitment remains base roster depth after automatic signing')

# Vetoed weekly attempts must not be remembered as permitted destinations.
reset();records[roster]['array']=[pp('returner',74)]
first,_,fp=pr(7030,83,stage=5);listed(first,TEST_TEAM_ID)
rejected,_,rp=pr(7031,85,stage=4);listed(rejected,TEST_TEAM_ID)
assert weekly_attempt(rejected)==0
native_signed(first);native_signed(rejected)
assert quality()[:6]==(0,4,1,2,1,83)
hard_checks.append('Blocked weekly condition is cleared and does not record pending ownership')

# A failed native condition, diagnostic mode and disabled mode do not write.
for mode_value,condition in [(1,0),(0,1),(-1,1)]:
 reset();configure(mode_value)
 r,_,p=pr(7040+mode_value,80,stage=4);listed(r,TEST_TEAM_ID)
 assert weekly_attempt(r,condition)==condition
 native_signed(r);configure(1)
 assert quality()[3:6]==(0,0,-1)
hard_checks.append('False native transition, diagnostic mode and disabled mode leave ownership untouched')

# All observed bad admissions are evaluated from their real preceding rooms.
cases=json.loads((ROOT.parent/'Audit/test04_nsd_false_admission_cases.json').read_text());results=[]
assert len(cases)==70
for ci,event in enumerate(cases):
 pos=POS.index(event['position']);reset(event['tid'],event['prestige'],pos,event['ideal'],event['starters']);isolate_archived_room(pos)
 for pl in event['actual_before']:
  if not pl['incoming']:records[roster]['array'].append(pp(pl['name'],pl['ovr'],pos=pos,tid=event['tid']))
 secured=[]
 for pi,pl in enumerate(p for p in event['actual_before'] if p['incoming']):
  r,t,p=pr(7100+ci*10+pi,pl['ovr'],pos=pos,stage=5);listed(r,event['tid']);secured.append(r)
 assert quality(pos)[3]==event['actual_count']
 for r in secured:native_signed(r)
 cand,_,_=pr(7100+ci*10+9,event['ovr'],pos=pos,stage=4);listed(cand,event['tid'])
 # Preserve the historical data. Four original duplicate-starter decisions
 # intentionally change in Test13: three clear upgrades still have room;
 # Notre Dame instead remains rejected by its unchanged headcount cap.
 revised={('Boise State','Austin Mack'):0,('Miami','Jakob Anderson'):1,
          ('California','Landon Rehkow'):1,('Notre Dame','Marcel Reed'):4}
 key=(event['team'],event['name'])
 if key in revised:assert event['actual_reason']==2
 # Test24 closes the historical earned-extra rotation exception.
 # Two archived non-starter matrix gaps are newly useful in Test21. These
 # fit the existing normal overage; neither invokes the actual80+ extra cap.
 depth21={('Liberty','Antonio Menard'),('Georgia State','Derek Colman-Brusa')}
 if key in depth21:
  assert event['actual_reason']==1 and event['position']in ['LE','DT']
  seats=sorted([p['ovr']for p in event['actual_before']],reverse=True)
  assert event['ovr']>=seats[2]+3
  revised[key]=0
 # Test29 explicitly opens four material rotation upgrades in these saved
 # rooms. The candidate stays behind the starter tier and improves an
 # unprotected useful rotation seat by at least four points. Historical
 # input, secured-player ownership and every other rejection stay exact.
 material29={('Cincinnati','Manasse Itete'):(74,1,70),
             ('Arkansas State','Marcellus Barnes Jr.'):(73,4,68),
             ('Utah State','Sam Stone'):(65,3,60),
             ('Troy','LeDarrion Menter'):(73,1,69)}
 if 'DriTestPortal29Observe' in exports and key in material29:
  rating,seat,incumbent=material29[key]
  room=sorted(event['actual_before'],key=lambda p:p['ovr'],reverse=True)
  assert event['actual_reason']==1 and event['ovr']==rating
  assert room[seat]['ovr']==incumbent and not room[seat]['incoming']
  assert rating-incumbent>=4 and seat>=event['starters']
  assert sum(p['ovr']>rating for p in room)>=event['starters']
  revised[key]=0
 expected=revised.get(key,event['actual_reason'])
 baseline=sum(not p['incoming'] for p in event['actual_before'])
 if event['actual_count']>=max(baseline,event['ideal'])+1 and expected!=2:expected=4
 revisions=json.loads((ROOT/'Tests/Fixtures/test29r4_archive_quality_permissions.json').read_text())['cases']
 matches=[x for x in revisions if x['archive']=='test04_nsd_false_admission_cases.json' and (x['team'],x['player'])==(event['team'],event['name'])]
 if matches:
  revision=matches[0];assert revision['rating']==event['ovr'] and revision['rank']<=revision['useful_depth'] and 4*event['ovr']>=revision['floor_q4']
  expected=revision['expected']
 got=decide(cand);assert got==expected,(event['team'],event['name'],got,expected)
 assert quality(pos)[3]==event['actual_count']
 results.append({'team':event['team'],'player':event['name'],'position':event['position'],'historical_reason':event['actual_reason'],'test26_reason':got,'count':event['actual_count']})
assert sum(x['test26_reason']==0 for x in results)==40
hard_checks.append('All seventy historical rooms retain correct ownership/counts: R4 quality permissions use frozen rank/floor expectations; retained QB/redundant-depth rejections and unknown whole-team flex remain isolated')

# A normal week change releases old entries; still-hard records are re-observed.
reset();r,_,p=pr(7900,80,stage=5);listed(r,TEST_TEAM_ID);assert quality()[3:6]==(1,0,80)
records[season]['scalar'][4]+=1
assert quality()[3:6]==(1,0,80)
native_signed(r);assert quality()[3:6]==(1,0,80)
records[committed]['array']=[p];call(exports['DriTestOwnershipReset']);assert quality()[3:6]==(1,0,80)
hard_checks.append('Next week re-observes still-hard commitments; published signed list survives a reset')
call(exports['DriTestOwnershipReset']);assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
hard_checks.append('All references balanced and no runtime fixture faults')
(OUTPUT/'hard_commit_ownership_results.json').write_text(json.dumps({'checks':hard_checks,'actual_test04_false_admission_cases':results,'live_game_verified':False,'scope':'Observed room regressions; not a counterfactual season simulation'},indent=2)+'\n')
print(json.dumps({'checks':hard_checks,'actual_cases':len(results)},indent=2))
