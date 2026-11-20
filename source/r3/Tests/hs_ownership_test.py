"""Observe the separate native HS transition path in compiled Test06 callbacks.
Captured Signed-state writer runs; storage and Windows services are fixtures.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'signing_ownership_test.py').read_text(),'signing_ownership_test.py','exec'))
hs_cb=hooks[16];hs_checks=[]
def hs_attempt(rec,next_stage=5):
 # R11 requires a real offer at new hard commitment; this fixture measures
 # ownership publication, while the R11 regression tests the withdrawn case.
 if next_stage==5 and (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))) and records[rec]['scalar'].get(2,0)<4:valid_offer(rec)
 records[transition]['scalar'][1]=next_stage
 wq(osp+0x38,transition);wq(obp+0x68,rec);wq(obp+0x60,context)
 before=bytes(uc.mem_read(of,128));state=dict(records[rec]['scalar'])
 call(hs_cb,of)
 assert bytes(uc.mem_read(of,128))==before
 assert records[rec]['scalar']==state

# Test05's unobserved HS path loses the newly signed player before publication.
reset();records[roster]['array']=[pp('returner',74)]
r,_,p=pr(8200,72,stage=4,portal=False);listed(r,TEST_TEAM_ID)
native_signed(r)
assert quality()[:6]==(0,4,1,1,1,-1)
hs_checks.append('Negative control: an unobserved new HS signing is omitted before list publication, reproducing the Test05 gap')

for next_stage in [5,6]:
 for start_stage in [2,3,4]:
  reset();records[roster]['array']=[pp('returner',74)]
  r,_,p=pr(8210+10*next_stage+start_stage,72,stage=start_stage,portal=False);listed(r,TEST_TEAM_ID)
  hs_attempt(r,next_stage)
  assert quality()[:6]==(0,4,1,1,1,-1)  # permission alone never adds a player
  if next_stage==5:records[r]['scalar'][9]=5
  native_signed(r)  # no intervening room scan or published committed list
  assert quality()[:6]==(0,4,1,2,2,-1)
  assert p not in records[committed]['array']
  records[committed]['array']=[p]
  assert quality()[:6]==(0,4,1,2,2,-1)  # deduplicates published record
hs_checks.append('Top5, Top3 and SoftCommitted HS transitions to HardCommitted or Signed preserve base depth without mutating decisions, stages or register frames')

# Diagnostic, disabled, nonterminal and wrong-class observations have no effect.
for mode_value,next_stage,is_portal in [(0,5,False),(-1,5,False),(1,4,False),(1,5,True)]:
 reset();configure(mode_value)
 r,_,p=pr(8300+next_stage+mode_value,72,stage=3,portal=is_portal);listed(r,TEST_TEAM_ID)
 hs_attempt(r,next_stage);native_signed(r);configure(1)
 assert quality()[:6]==(0,4,1,0,0,-1)
hs_checks.append('Diagnostic/disabled, soft transitions and portal-class records are ignored')

# User HS recruiting is also observed, with no recruiting-policy veto.
reset();records[team]['ref'][407]=record('user character')
r,_,p=pr(8350,72,stage=3,portal=False);listed(r,TEST_TEAM_ID);hs_attempt(r);native_signed(r)
assert quality()[:6]==(0,4,1,1,1,-1)
hs_checks.append('User-team HS destination is observed without changing user recruiting')

# The exact two live Test05 false-admission rooms, with HS ownership learned
# at its separate transition before the native Signed writer runs.
case_results=[]
for ci,event in enumerate(json.loads((ROOT.parent/'Audit/test05_nsd_false_admission_cases.json').read_text())):
 pos=POS.index(event['position']);reset(event['tid'],event['prestige'],pos,event['ideal'],event['starters']);isolate_archived_room(pos)
 records[teams]['array']=fixture_team_array()
 missing={p['pid'] for p in event['omitted_HS']}
 for p in event['actual_before']:
  if not p['incoming'] and p['pid'] not in missing:records[roster]['array'].append(pp(p['name'],p['ovr'],pos=pos,tid=event['tid']))
 for j,p in enumerate(event['actual_before']):
  if p['pid'] in missing:
   r,_,_=pr(8400+ci*20+j,p['ovr'],pos=pos,stage=3,portal=False);listed(r,event['tid']);hs_attempt(r);native_signed(r)
  elif p['incoming']:
   r,_,_=pr(8400+ci*20+j,p['ovr'],pos=pos,stage=5);listed(r,event['tid']);quality(pos);native_signed(r)
 candidate,_,_=pr(8419+ci*20,event['ovr'],pos=pos)
 assert event['actual_reason']==4
 expected=4
 revisions=json.loads((ROOT/'Tests/Fixtures/test29r4_archive_quality_permissions.json').read_text())['cases']
 matches=[x for x in revisions if x['archive']=='test05_nsd_false_admission_cases.json' and (x['team'],x['player'])==(event['team'],event['name'])]
 if matches:
  revision=matches[0]
  assert revision['rating']==event['ovr'] and revision['rank']==1+sum(pl['ovr']>=event['ovr'] for pl in event['actual_before'])
  assert revision['rank']<=revision['useful_depth'] and 4*event['ovr']>=revision['floor_q4']
  expected=revision['expected']
 got=decide(candidate);assert got==expected,(event['team'],got,expected)
 assert quality(pos)[3]==event['actual_count']
 case_results.append({'team':event['team'],'player':event['name'],'count':quality(pos)[3],'reason':got})
hs_checks.append('Exact HS ownership/counts retained: Tulsa TE73 remains below prestige floor74; Central Michigan MLB63 now qualifies at useful rank4/floor59')
call(exports['DriTestOwnershipReset']);assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
hs_checks.append('All references balanced and no fixture faults')
(OUTPUT/'hs_ownership_results.json').write_text(json.dumps({'checks':hs_checks,'actual_test05_cases':case_results,'live_game_verified':False},indent=2)+'\n')
print(json.dumps({'checks':hs_checks,'actual_test05_cases':case_results},indent=2))
