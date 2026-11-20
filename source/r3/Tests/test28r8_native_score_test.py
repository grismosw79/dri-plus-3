"""Compiled R8 board scorer with captured native aggregate instructions.
Component services are mocked; FTC bit-scope verification is a separate gate.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset'])
records[season]['scalar'].update({2:2040,4:8,5:1})
r,t,p=pr(61090,79,year=0,pos=3,stage=0,portal=False)
score_entry=GAME+0x08185840
handlers.pop(score_entry,None)
map_game(0x08184d10,0x1000)
uc.mem_write(score_entry,raw[0x08185840-0x1000:0x08185afb-0x1000])
values={'pipeline':30,'home':20,'prestige':300,'position':35,'scheme':20,'interest':60,'need':10,'motivation':0}
seen=[]
services={0x0815af30:'pipeline',0x08158180:'home',0x0815c380:'prestige',0x0815b300:'position',0x0815b700:'scheme',0x081586e0:'interest',0x0815c760:'need',0x08158a20:'motivation'}
def component(name):
 scorer,selection,portal,_=args()
 assert scorer==fixture_hs_scorer and selection==0 and portal==0,(name,args())
 seen.append(name);return values[name]
for address,name in services.items():game_api(address,lambda name=name:component(name))
# Current offer-count multiplier is zero. Its native rank/weight lookups remain zero here.
game_api(0x07e0eff0,lambda:0)
out=alloc(4);checks=[]
for pipeline,prestige,expected in [(30,300,455),(0,300,445),(0,-500,-355)]:
 values.update(pipeline=pipeline,prestige=prestige);seen.clear();wi(out,0xdeadbeef)
 assert call(exports['DriTestHsNativeScore'],r,team,context,out)==1
 actual=struct.unpack('<i',uc.mem_read(out,4))[0]
 assert actual==expected,(actual,expected,seen)
 assert ('home' in seen)==(pipeline==0)
 assert all(name in seen for name in ['prestige','position','scheme','interest','need','motivation'])
 checks.append({'pipeline':pipeline,'prestige':prestige,'score':actual,'native_aggregate':True})
wq(context+0xf8,0);wi(out,123)
assert call(exports['DriTestHsNativeScore'],r,team,context,out)==0 and ri(out)==123
wq(context+0xf8,fixture_hs_scorer)
assert all(v==1 for v in refs.values())
(OUTPUT/'test28r8_native_score_results.json').write_text(json.dumps({'checks':checks,'missing_scorer_preserves_output':True,'balanced_references':True,'live_game_verified':False},indent=2)+'\n')
print('PASS captured native score aggregate, ABI flags, nonstacking geography, negative scores and missing dependency')
