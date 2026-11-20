"""R4 quality-depth admission plus 15 historical unsigned elite identities through compiled guarded admission and
captured native AddToBoard. Native storage/eligibility services are modeled;
destination rooms are controlled, not a prediction that all15 will sign."""
from pathlib import Path
import json
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'test29_portal_fixture.py','exec'))
map_game(0x08149660,0xc00)
uc.mem_write(GAME+0x08149660,raw[0x08149660-0x1000:0x0814a210-0x1000])
handlers.pop(GAME+0x08149660,None);uc.ctl_remove_cache(GAME+0x08149660,GAME+0x0814a210)
map_game(0x0bd0c000,0x2000)
new_targets=[];block_native=False
def create_target():
 _,out,_,_=args();target=record('native allocated portal target');new_targets.append(target)
 incref(target);wq(out,target);return out
def write_reference():
 target,field,holder,_=args();records[target]['ref'][field-0x100000]=rq(holder);return 0
def append_target():
 array,holder,_,_=args();records[array]['array'].append(rq(holder));return 0
def return_null():wq(reg('RDX'),0);return reg('RDX')
for address,fn in [(0x081bc0a0,lambda:int(block_native)),(0x03803770,lambda:50),(0x08103c20,lambda:0),
 (0x0804b440,create_target),(0x0805e4d0,write_reference),(0x082c0be0,lambda:0),(0x0862db70,lambda:0),
 (0x08045cd0,append_target),(0x0862d2a0,return_null),(0x081ae440,lambda:0)]:game_api(address,fn)
obj=alloc(16);vt=alloc(0x200);wq(obj,vt);wq(vt+0x198,api(return_null));wq(manager+0x100,obj)
# Test30 component setup: use actual global forecast, with modeled native
# fit and action costs declared explicitly. No production authorization flag
# is patched; failed/missing market authorization would still reject entry.
records[tuning]['ref'][49]=record('Test30 controlled actions',array=[
 record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
market_score=450
def component_score():
 scorer,selection,portal,tr=args()
 assert scorer==fixture_hs_scorer and (selection&255,portal&255) in ((0,1),(1,1))
 candidate=rq(reg('RSP')+0x30);wi(candidate+40,market_score);return 0
game_api(0x08185840,component_score)
def authorize_component(r,base=450):
 global market_score
 market_score=base;records[r]['scalar'][0]=1
 call(exports['DriTestMarket29PortalReset']);call(exports['DriTestMarket29PortalBegin'],context)
 assert call(exports['DriTestQualityFault'])==0
 assert call(exports['DriTestPortal30AdmissionBlock'],r,team,context)==0
cases=json.loads((Path(__file__).parent/'Fixtures/test29_portal_unsigned.json').read_text())['cases']
for case in cases:
 for a in [tplist,hslist,committed,arr,roster]:records[a]['array']=[]
 year=['Freshman','Sophomore','Junior','Senior'].index(case['class'].split('_')[1])
 r,t,p=pr(case['rid'],case['ovr'],year=year,pos=case['pos'],stage=0)
 records[r]['scalar'][3]=case['commit_score']
 records[r]['scalar'][14]=0
 authorize_component(r)
 original=dict(records[r]['scalar']);original_player=dict(records[p]['scalar']);budget_before=dict(records[board]['scalar'])
 assert call(exports['DriTestPortal29Admit'],team,r,context,450)==1,case
 targets=records[arr]['array'];assert len(targets)==1 and targets[0]in new_targets
 assert records[targets[0]]['ref'][10]==r and records[targets[0]]['scalar'][12]==0
 assert records[r]['scalar']==original and records[p]['scalar']==original_player and records[board]['scalar']==budget_before
 assert call(exports['DriTestPortal29Admit'],team,r,context,450)==1 and len(records[arr]['array'])==1
 # A rejected native creation remains a failed path, not a synthetic offer.
 records[arr]['array']=[];block_native=True
 assert call(exports['DriTestPortal29Admit'],team,r,context,450)==0 and not records[arr]['array']
 assert records[r]['scalar']==original;block_native=False
checks=['All15 actual unsigned R14 identities/positions/OVRs use compiled guarded bridge and captured native AddToBoard',
 'Native target starts without scholarship; recruit stage/influence/player/ownership/native budgets unchanged',
 'Repeat is idempotent and native refusal leaves no fabricated target or offer']
# Actual captured room shape: Auburn RT projected best78, full3, Luke83+2.
records[arr]['array']=[];records[tplist]['array']=[]
records[ideal_entries[9]]['scalar'][2]=3
records[roster]['array']=[pp('adequate returner',76,pos=9),pp('rotation',74,pos=9),pp('depth',73,pos=9)]
upgrade,ut,up=pr(3202,83,year=2,pos=9,stage=0)
assert call(exports['DriTestPortalDecision'],upgrade,team,context)==0
authorize_component(upgrade,480)
assert call(exports['DriTestPortal29Admit'],team,upgrade,context,480)==1
records[up]['scalar'][191]=78
assert call(exports['DriTestPortalDecision'],upgrade,team,context)==0
# R4 permits a prestige-qualified starter even without the older +5 gap.
# A candidate below the entire useful two-deep still has no new permission.
records[up]['scalar'][191]=74
assert call(exports['DriTestPortalDecision'],upgrade,team,context)==1
checks.append('Controlled Auburn/Luke projected shape reaches actual current-room admission; prestige-qualified starter admitted without the old gap; redundant reserve rejected')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test29r4_quality_depth_native_results.json').write_text(json.dumps({'checks':checks,'actual_unsigned_fixtures':len(cases),'captured_native':'0x08149660','live_game_verified':False,'not_proven':['native offer funding in this test','live cohort signings','all actual school destinations']},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
