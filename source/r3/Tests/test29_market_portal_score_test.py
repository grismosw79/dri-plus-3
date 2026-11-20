"""Captured native portal aggregate called through the compiled Test29 ABI.
Native component services are modeled; captured aggregation/branch code runs.
"""
from pathlib import Path
exec(compile((Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset'])
records[season]['scalar'].update({2:2040,4:3,5:6})
rec,target,player=pr(29990,78,year=1,stage=0,portal=True)
records[rec]['scalar'][0]=1
checks=[]
# Execute the captured native scorer aggregate with Test29's production call
# ABI. Only native component services are mocked; arithmetic and branch logic
# below are the actual supported executable instructions.
score_entry=GAME+0x08185840;handlers.pop(score_entry,None)
map_game(0x08184d10,0x1000)
uc.mem_write(score_entry,raw[0x08185840-0x1000:0x08185afb-0x1000])
values={'pipeline':30,'home':20,'prestige':300,'position':35,'scheme':20,'interest':60,'need':10,'motivation':0};seen=[]
services={0x0815af30:'pipeline',0x08158180:'home',0x0815c380:'prestige',0x0815b300:'position',0x0815b700:'scheme',0x081586e0:'interest',0x0815c760:'need',0x08158a20:'motivation'}
def component(name):
 scorer,selection,portal,_=args();assert scorer==fixture_hs_scorer and selection&255==1 and portal&255==1,(name,args())
 seen.append(name);return values[name]
for address,name in services.items():game_api(address,lambda name=name:component(name))
game_api(0x07e0eff0,lambda:0)
out=alloc(4)
for pipeline,prestige,expected in [(30,300,455),(0,300,445),(0,-500,-355)]:
 values.update(pipeline=pipeline,prestige=prestige);seen.clear();wi(out,123)
 assert call(exports['DriTestMarket29PortalNativeScore'],rec,team,context,out)==1
 actual=struct.unpack('<i',uc.mem_read(out,4))[0];assert actual==expected,(actual,expected,seen)
 assert ('home'in seen)==(pipeline==0)
 assert all(n in seen for n in ['prestige','position','scheme','interest','need','motivation'])
wq(context+0xf8,0);wi(out,123)
assert call(exports['DriTestMarket29PortalNativeScore'],rec,team,context,out)==0 and ri(out)==123
wq(context+0xf8,fixture_hs_scorer)
checks.append('Captured native aggregate executes with portal selection1/Boolean1/team-reference/candidate48 ABI, including native nonstacking geography, negative fit and missing-scorer preservation')
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test29_market_portal_score_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False,'native_component_services_mocked':True},indent=2)+'\n')
print(json.dumps(checks,indent=2))
