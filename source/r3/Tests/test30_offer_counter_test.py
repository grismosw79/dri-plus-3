"""Compiled NSD derived counter repair; actual native rival removal retained."""
from pathlib import Path
src=(Path(__file__).parent/'test28r8_hs_rival_cleanup_test.py').read_text()
exec(compile(src.split('# Confirmed acquisition')[0],'cleanup fixture','exec'))
checks=[]
reset();r,p=hs(64000,stage=6,leader=2);winner=target(r,2)
records[commits[2]]['array']=[p];records[r]['scalar'][14]=0
before=snapshot()
frame=alloc(128);wq(frame+16,context)
call(hooks[names.index('HookPostNsd')],frame)
assert call(exports['DriTestQualityFault'])==0
after=snapshot();before[r]['scalar'][14]=1
assert after==before and records[boards[2]]['array']==[winner]
checks.append('Robinson-shaped zero counter with one actual winning Offered target repairs only the counter at the real post-NSD callback')
before=snapshot();call(hooks[names.index('HookPostNsd')],frame);assert snapshot()==before
checks.append('Repeated post-NSD reconciliation is idempotent')
reset();r,p=hs(64001,stage=6,leader=1);target(r,1);target(r,2);target(r,3,user=True)
records[commits[2]]['array']=[p];records[r]['scalar'][14]=0
call(hooks[names.index('HookPostNsd')],frame)
assert call(exports['DriTestQualityFault'])==0
assert not records[boards[1]]['array'] and records[r]['scalar'][14]==2
assert records[commits[2]]['array']==[p] and len(records[boards[3]]['array'])==1
checks.append('After native CPU rival cleanup the counter counts both retained winner and user offers, even when the incoming counter was already zero')
reset();r,p=hs(64002,stage=2);target(r,1,status=0);target(r,2,status=1);target(r,3,status=2);target(r,4,status=3);target(r,5,status=4)
pr,pp=make(64003,stage=2);target(pr,2,status=3)
records[r]['scalar'][14]=99;records[pr]['scalar'][14]=0
before=snapshot();assert call(exports['DriTest30OfferCounters'],context)==1
before[r]['scalar'][14]=3;before[pr]['scalar'][14]=1
assert snapshot()==before and not records[commits[2]]['array']
checks.append('HS and portal open recruits count exactly native statuses 2..4 without signing, target, influence or ownership changes')
reset();r,p=hs(64004,stage=5);target(r,3,user=True);records[hs_pool]['array']=[];records[r]['scalar'][14]=0
before=snapshot();assert call(exports['DriTest30OfferCounters'],context)==1
before[r]['scalar'][14]=1;assert snapshot()==before
checks.append('Valid user-board recruit outside current pools remains supported and counted without moving it into a pool')
for broken in ['duplicate_target','duplicate_owner','bad_status','bad_stage']:
 reset();r,p=hs(64010,stage=6);target(r,2);records[commits[2]]['array']=[p];records[r]['scalar'][14]=0
 if broken=='duplicate_target':target(r,2)
 if broken=='duplicate_owner':records[commits[3]]['array']=[p]
 if broken=='bad_status':target(r,3,status=5)
 if broken=='bad_stage':records[r]['scalar'][9]=7
 before=snapshot();assert call(exports['DriTest30OfferCounters'],context)==0
 assert call(exports['DriTestQualityFault'])!=0 and snapshot()==before
checks.append('Duplicate targets/owners and invalid statuses/stages fail before any counter write')
for mode_value in [0,-1]:
 reset();r,p=hs(64020);target(r,2);records[r]['scalar'][14]=0;configure(mode_value)
 before=snapshot();assert call(exports['DriTest30OfferCounters'],context)==1 and snapshot()==before
checks.append('Diagnostic and disabled modes do not mutate counters')
call(exports['DriTestOwnershipReset']);configure(1)
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items()if v!=1]
(OUTPUT/'test30_offer_counter_results.json').write_text(json.dumps({'passed':True,'checks':checks,'historical_first_bad_writer_identified':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
