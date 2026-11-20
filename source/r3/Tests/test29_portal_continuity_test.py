"""Compiled pre-funding portal recovery distinguishes conditional destinations.
Real native offer counters/ownership remain authoritative; no signing promise."""
from pathlib import Path
import json
fixture=(Path(__file__).parent/'test29_portal_witness_test.py').read_text()
exec(compile(fixture.split('# Displayed scholarship,')[0],'test29_portal_witness_test.py','exec'))
for rec,target in [(primary,pt),(backup,bt)]:
 records[rec]['scalar'][9]=3
 records[target]['scalar'].update({8:250,15:1})
records[board]['scalar'][1]=20
records[ideal_entries[9]]['scalar'][2]=1
assert call(exports['DriTestCommitBlock'],primary,team,context,0)==0
assert call(exports['DriTestCommitBlock'],backup,team,context,0)==9
budget_before=dict(records[board]['scalar']);board_before=records[arr]['array'][:]
player_before={p:dict(records[p]['scalar'])for p in [ppp,bpp]}
primary_top=records[records[primary]['ref'][13]]['array']
backup_top=records[records[backup]['ref'][13]]['array']
paid_points=records[backup_top[0]]['scalar'][1]
call(exports['DriTestRecoverMarket'],context,0)
assert records[primary]['scalar'][9]==3,'independent viable primary should not reopen'
assert records[backup]['scalar'][9]==0,'conditional-only destinations must reopen before another native funding round'
assert records[bt]['scalar'][12]==3 and records[bt]['scalar'][8]==250
assert records[backup_top[0]]['scalar'][1]==paid_points
assert records[arr]['array']==board_before and records[board]['scalar']==budget_before
assert player_before=={p:dict(records[p]['scalar'])for p in player_before}
# If the prior primary's real offer is withdrawn, the same backup becomes a
# usable existing destination; it need not lose earned interest or be assigned.
records[pt]['scalar'][12]=0
records[backup]['scalar'][9]=3
assert call(exports['DriTestCommitBlock'],backup,team,context,0)==0
call(exports['DriTestRecoverMarket'],context,0)
assert records[backup]['scalar'][9]==3 and records[bt]['scalar'][12]==3
assert records[backup_top[0]]['scalar'][1]==paid_points
# A new session's initial open stage remains open; no manufactured preference.
records[backup]['scalar'][9]=0
before=dict(records[backup]['scalar']);call(exports['DriTestRecoverMarket'],context,1)
assert records[backup]['scalar']==before
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(v==1 for v in refs.values())
checks=['Conditional-only paid backup reopens while an independent primary keeps its stage',
 'Existing legal offer and its earned influence survive reconsideration; no hours, player, board or ownership changes',
 'Withdrawal of the primary releases the existing backup into an independently usable opportunity',
 'Initial open-list grace remains intact; no predetermined destination or auto-signing']
(OUTPUT/'test29_portal_continuity_results.json').write_text(json.dumps({'checks':checks,'method':'compiled recovery and capacity callbacks with controlled SDF records','live_game_verified':False,'native_new_offer_execution_in_this_test':False},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
