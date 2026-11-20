"""Compiled primary-role checkpoint, legacy earned-route reconstruction and refusals.
SDF/native fit and sidecar transport are controlled; production codec and planner execute.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'test31_class_fixture.py').read_text(),'class fixture','exec'))
checks=[];TEST_TEAM_ID=55;plan_out=alloc((4+128*4)*4)
def plan():
 n=call(exports['DriTest34R2PlanSnapshot'],team,context,plan_out);assert 0<=n<=128
 a=list(struct.unpack('<'+str(4+n*4)+'i',uc.mem_read(plan_out,(4+n*4)*4)))
 return a[:4],{a[i]:a[i+1:i+4] for i in range(4,len(a),4)}
def progress(r,points,gain):
 target=next(t for t in records[arr]['array'] if records[t]['ref'][10]==r)
 for e in records[records[r]['ref'][13]]['array']:
  if records[e]['scalar'][0]==55:records[e]['scalar'][1]=points
 records[target]['scalar'].update({12:3,7:gain,8:points,9:points});return target
def prepare():
 reset();call(exports['DriTestHsPlanReset']);call(exports['DriTest34R3CacheIO'],1)
 records[franchise]['scalar'][28]=3184391076;records[season]['scalar'].update({4:9,5:1})
 records[teams]['array']=dummy_teams.copy();records[teams]['array'][55]=team
 for e,i in zip(ideal_entries,[4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0]):records[e]['scalar'][2]=i
 records[board]['scalar'].update({1:0,2:0,3:1650})
 hard,_=prospect(pos=1,ovr=70,stars=2,stage=5)
 records[roster]['array']=[player('HB returner',pos=1) for _ in range(3)]
 cut,_=prospect(pos=1,ovr=74,stars=3,stage=0);records[cut]['scalar'][3]=745;progress(cut,685,63)
 def score():
  candidate=rq(reg('RSP')+0x30);r=rq(candidate);wi(candidate+40,403 if r==cut else 373);return 0
 game_api(0x08185840,score)
 return cut
cut=prepare();_,roles=plan();assert roles[ri(cut+0x18)][0]==1
ogb,_=prospect(pos=1,ovr=70,stars=2,stage=0);records[ogb]['scalar'][3]=463;progress(ogb,316,146)
_,warm=plan();assert warm[ri(cut+0x18)][0]==1 and warm[ri(ogb+0x18)][0]==2
# Preserve the actual primary even after the challenger has more points.
# This cannot be recovered just by sorting native progress after a restart.
progress(ogb,720,146);_,warm=plan();assert warm[ri(cut+0x18)][0]==1
call(exports['DriTest34R3Checkpoint'],context)
before={r:dict(records[r]['scalar']) for r in [cut,ogb,board]}
call(exports['DriTestHsPlanReset']);records[board]['scalar'][1]=0
_,cold=plan();assert cold==warm,(warm,cold)
assert all(records[r]['scalar']==v for r,v in before.items())
checks.append('Matched board checkpoint restores earned primary even when backup now has more native points; reset creates no influence, offer or budget')
# Ordering changes are not identity changes.
records[arr]['array'].reverse();call(exports['DriTestHsPlanReset']);assert plan()[1]==warm
checks.append('Board permutation preserves the same native-state fingerprint and roles')
# Corruption/mismatch cannot supply a prior; ambiguous legacy pair uses utility.
for field in [0,1,2]:
 call(exports['DriTest34R3Checkpoint'],context);call(exports['DriTest34R3CacheCorrupt'],field);call(exports['DriTestHsPlanReset'])
 _,roles=plan();assert roles[ri(ogb+0x18)][0]==1,(field,roles)
 # Reestablish the controlled original primary for the next negative case.
 records[arr]['array']=[t for t in records[arr]['array'] if records[t]['ref'][10]!=ogb];call(exports['DriTestHsPlanReset']);plan()
 records[arr]['array'].append(next(t for t,v in records.items() if v.get('ref',{}).get(10)==ogb));plan()
checks.append('Corrupt checksum, wrong league and future checkpoint all fall back without restoring stale roles')
# Legacy R2 W9 input has no sidecar. Native gain + clearly dominant progress
# and no quality disadvantage reconstruct Cutrera, with no named-player rule.
progress(ogb,316,146);call(exports['DriTest34R3CacheIO'],1);call(exports['DriTestHsPlanReset'])
_,roles=plan();assert roles[ri(cut+0x18)][0]==1 and roles[ri(ogb+0x18)][0]==2
checks.append('Legacy W9-shaped 685/745 versus316/463, OVR74/70, fits403/373 reconstructs earned primary without a sidecar')
# A stale native gain cannot override a withdrawn offer or current rejection.
target=progress(cut,685,63);records[target]['scalar'][12]=1
call(exports['DriTestHsPlanReset']);_,roles=plan();assert roles[ri(cut+0x18)][0]!=1
records[target]['scalar'][12]=3
call(exports['DriTestHsPlanReset']);plan();call(exports['DriTest34R3Checkpoint'],context)
game_api(0x0816b130,lambda:0 if rq(reg('R8'))==cut else fixture_eligibility())
call(exports['DriTestHsPlanReset']);_,roles=plan();assert roles[ri(cut+0x18)][0]==0
game_api(0x0816b130,fixture_eligibility)
checks.append('Withdrawn offer and fresh native ineligibility cannot inherit primary continuity')
for why in ['no_gain','ambiguous_progress','lower_quality']:
 call(exports['DriTest34R3CacheIO'],1);call(exports['DriTestHsPlanReset'])
 progress(cut,350 if why=='ambiguous_progress' else 685,0 if why=='no_gain' else 63)
 records[records[cut]['ref'][5]]['scalar'][191]=69 if why=='lower_quality' else 74
 _,roles=plan();assert roles[ri(ogb+0x18)][0]==1,(why,roles)
checks.append('Legacy reconstruction does not override utility without native gain, clear progress advantage and equal-or-better OVR')
call(exports['DriTest34R3CacheIO'],0);records[franchise]['scalar'][28]=0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);assert call(exports['DriTestQualityFault'])==0 and all(v==1 for v in refs.values())
(OUTPUT/'test34_r3_continuity_runtime_results.json').write_text(json.dumps({'passed':True,'checks':checks,'native_services_modeled':True,'sidecar_transport_modeled':True,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
