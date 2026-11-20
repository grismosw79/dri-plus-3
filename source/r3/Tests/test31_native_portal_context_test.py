"""Captured native school cache, position need component and scorer aggregate.
Storage, needs vector and other scoring components are controlled services.
This proves the context mismatch/fix, not live execution or season outcomes.
"""
from pathlib import Path
import json
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'portal fixture','exec'))
rec,target,player=pr(31001,88,pos=18,stage=0)
records[rec]['scalar'][0]=1
for entry,end in [(0x08142630,0x081427cb),(0x081bd2f0,0x081bd600),(0x0815c5e0,0x0815c758),(0x0815c760,0x0815c958),(0x08185840,0x08185afb)]:
 map_game(entry,end-entry);uc.mem_write(GAME+entry,raw[entry-0x1000:end-0x1000]);handlers.pop(GAME+entry,None)
map_game(0x0da92840,8)
for address in [0x08042fd0,0x08052210,0x080529b0,0x078cb430]:game_api(address,lambda:0)
game_api(0x078cd652,lambda:win_import('memset'))
need_by_team={TEST_TEAM_ID:0};native_cache_calls=[]
def need_vector():
 _,holder,vector,_=args();t=rq(holder);n=22
 memory=rq(vector)
 for pos in range(n):wi(memory+pos*4,-need_by_team.get(records[t]['scalar'][383],0) if pos==18 else 0)
 wq(vector+8,memory+n*4);native_cache_calls.append(t);return 0
game_api(0x085a6f00,need_vector)
game_api(0x0818ab80,lambda:0)
game_api(0x08188a10,lambda:0)
game_api(0x085a69f0,lambda:(-12)&0xffffffff) # aggregate null-cache fallback
for offset in [0x408,0x2d8]:wi(fixture_hs_scorer+offset,10)
for address,value in [(0x0815af30,0),(0x08158180,0),(0x0815c380,-50),(0x0815b300,0),(0x0815b700,0),(0x081586e0,0),(0x08158a20,0)]:
 game_api(address,lambda value=value:value&0xffffffff)
out=alloc(4);tr=alloc(8);wq(tr,team);candidate=alloc(48);wq(candidate,rec)
def direct(cache,selection):
 entry_sp=STACK+0xf0000-8
 wq(entry_sp+0x28,cache);wq(entry_sp+0x30,candidate);wi(entry_sp+0x38,3)
 call(GAME+0x08185840,fixture_hs_scorer,selection,1,tr)
 return ri(candidate+40)
assert direct(0,1)==70,'Test30 null-cache forecast should see aggregate need'
assert call(exports['DriTestMarket29PortalNativeScore'],rec,team,context,out)==1
assert ri(out)==((-50)&0xffffffff),('Test31 must see real zero need at this position',ri(out))
need_by_team[TEST_TEAM_ID]=8
assert call(exports['DriTestMarket29PortalNativeScore'],rec,team,context,out)==1 and ri(out)==30
cache=alloc(0x218);call(GAME+0x08142630,cache);call(GAME+0x0815c5e0,fixture_hs_scorer,tr,cache)
assert direct(cache,1)==30 and direct(cache,0)==30
assert rq(cache)==cache+0x18 and rq(cache+0xa8)==cache+0xc0 and rq(cache+0x150)==cache+0x168
assert len(native_cache_calls)==3
assert all(v==1 for v in refs.values())
checks=['Captured initializer/fill creates the same inline per-school vectors used by native board preparation',
 'Reproduced positive70 null-cache forecast versus negative50 native position-context score for the same recruit/school',
 'Correct native position deficit produces positive30 for both new-candidate and retained-target scoring routes',
 'Compiled production forecast calls captured cache/scorer instructions; no record, budget, offer or ownership writes']
(OUTPUT/'test31_native_portal_context_results.json').write_text(json.dumps({'checks':checks,'captured':['08142630','0815c5e0','0815c760','08185840'],'modeled':['SDF storage','native roster-needs vector','non-need score components','locks'],'live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+x for x in checks))
