"""Actual post-NSD callback + captured native removal, with SDF fixtures."""
from pathlib import Path
src=(Path(__file__).parent/'test28r8_hs_rival_cleanup_test.py').read_text()
exec(compile(src.split('# Confirmed acquisition')[0],'cleanup fixture','exec'))
reset();r,p=hs(63000,stage=6,leader=1)
loser=target(r,1,visit_hours=25);winner=target(r,2);user=target(r,3,user=True)
records[commits[2]]['array']=[p]
frame=alloc(128);wq(frame+16,context) # Registers.r13
call(hooks[names.index('HookPostNsd')],frame)
assert records[boards[1]]['array']==[] and records[boards[2]]['array']==[winner] and records[boards[3]]['array']==[user]
assert records[commits[2]]['array']==[p] and records[r]['scalar'][9]==6 and records[r]['scalar'][14]==2
before=snapshot();call(hooks[names.index('HookPostNsd')],frame);assert snapshot()==before
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items()if v!=1]
(OUTPUT/'test28r14_nsd_cleanup_results.json').write_text(json.dumps({'post_NSD_HS_cleanup':True,'winner_and_user_preserved':True,'idempotent':True,'native_remove_executed':True,'live_game_verified':False},indent=2)+'\n')
print('PASS actual post-NSD HS rival cleanup callback')
