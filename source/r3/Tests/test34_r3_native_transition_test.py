"""Retained captured native condition/transition with the closed PLV allocation phase."""
from pathlib import Path
source=(Path(__file__).parent/'test34_r2_native_commitment_test.py').read_text()
exec(compile(source.split('t,r,target,ordinary=prepare_full();state,roles=fullplan(t)')[0],'captured weekly definitions','exec'))
checks=[]
for phase in [(2,17),(6,0),(6,1)]:
 t,r,target,ordinary=prepare_full();records[season]['scalar'].update({5:phase[0],4:phase[1]})
 earned(r,target,744);before=values(r)
 assert not full_attempt(r,5) and records[r]['scalar'][9]==3 and values(r)==before
 earned(r,target,745);assert full_attempt(r,5) and records[r]['scalar'][9]==5
 assert hard_service_calls[-1]==(t,r)
 checks.append({'type':phase[0],'week':phase[1],'below_threshold_refused':True,'qualified_ordinary_native_transition_allowed':True})
uc.hook_del(stop_hook);call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0 and all(v==1 for v in refs.values())
(OUTPUT/'test34_r3_native_transition_results.json').write_text(json.dumps({'passed':True,'checks':checks,'native_condition_and_stage_writer_executed':True,'native_manager_side_effects_modeled':True,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
