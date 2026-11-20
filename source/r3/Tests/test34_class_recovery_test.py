"""Test34 bounded ranked continuation and post-week next-allocation queue.
Retains Test31 eligibility, score, affordability, user, annual, PRE and native
board-admission controls. Native services are controlled fixtures; a queued
candidate is never counted as an offer, current reservation or funded pursuit.
"""
from pathlib import Path
import json
prior=(Path(__file__).parent/'test31_class_recovery_test.py').read_text()
exec(compile(prior.split('# Only two prospects')[0],'retained class recovery controls','exec'))

# Top-two native-score rejection must not hide a valid third candidate.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
items=[]
for i in range(3):
 r,p=candidate(134100+i,10,ovr=69-i);items.append(r);scores[134100+i]=0 if i<2 else 400
seed()
assert call(exports['DriTestHsRecoveryPending'],items[2],context)>0
assert not any(call(exports['DriTestHsRecoveryPending'],r,context) for r in items[:2])
assert set(score_calls)>={134100,134101,134102}
checks.append('Native-score failures of top two candidates continue to qualifying third; no fabricated offer')

# Work is bounded, but the ranked cursor survives a week transition instead
# of repeatedly checking the same first44 failures. Identity tie-breaks remain
# stable when the underlying pool array order changes.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
records[season]['scalar'].update({4:1,5:1})
items=[]
for i in range(80):
 r,p=candidate(134200+i,10,ovr=99-i);items.append(r);scores[134200+i]=400 if i==60 else 0
seed();first=list(score_calls)
assert len(set(first))==44 and not call(exports['DriTestHsRecoveryPending'],items[60],context)
records[hslist]['array'].reverse();records[season]['scalar'][4]=2
seed()
assert call(exports['DriTestHsRecoveryPending'],items[60],context)>0
assert 134260 in score_calls and len(set(score_calls)-set(first))<=44
checks.append('44-check limit resumes through later ranked candidates across week and pool permutation')

# Needed positions each receive a turn before a deep rejected list consumes
# the budget. A specialist route remains discoverable in the same pass.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
# In the actual deterministic cadence, team10 has K intake in2026 and P
# intake in2027. The fairness case must contain a genuine K opening.
records[season]['scalar'][2]=2026
annual_out=alloc(46*4)
assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
assert ri(annual_out+(1+19)*4)==1 and ri(annual_out+(23+19)*4)==0
for i in range(70):
 candidate(134400+i,10,ovr=99-i);scores[134400+i]=0
k,kp=candidate(134480,10,pos=19,ovr=65)
seed()
assert call(exports['DriTestHsRecoveryPending'],k,context)>0
assert score_calls.index(134480)<44
checks.append('Position round-robin reaches a valid kicker despite a deep rejected QB list')

# Re-entering an unchanged scope does not rescore an exhausted candidate set.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
for i in range(10):candidate(134500+i,10,ovr=60+i);scores[134500+i]=0
seed();first=len(score_calls)
call(exports['DriTest29R2ClassSeed'],context)
assert len(score_calls)==first
checks.append('Unchanged same-phase scope does not retry already rejected candidate identities')

# A completed school's spent budget and admission closure must not be reset.
# Post-week queue remains intent only. On a later scope it is fully checked
# again and may enter normal native admission, or be rejected if eligibility
# changed. There is no same-week offer or debit.
for changed in [False,True]:
 t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
 r,p=candidate(134600+int(changed),10)
 budget=records[records[t]['ref'][224]]['scalar'];budget[1]=budget[3]
 seed();assert not call(exports['DriTestHsRecoveryPending'],r,context)
 call(exports['DriTestHs32Begin'],context)
 call(exports['DriTestHs33ReleaseSchool'],t,context)
 before=snap8(r);before_budget=dict(budget);before_adds=list(adds)
 call(exports['DriTest34ClassRefill'],context)
 assert call(exports['DriTest34ClassQueued'],r,10)>0
 assert not call(exports['DriTestHsRecoveryPending'],r,context)
 assert call(exports['DriTestHs30AdmissionBlock'],r,t,context)==1
 assert snap8(r)==before and budget==before_budget and adds==before_adds and not offer_status(r,10)
 count_before=len(score_calls);call(exports['DriTest34ClassRefill'],context);assert len(score_calls)==count_before
 records[season]['scalar'][4]=6;budget[1]=0
 if changed:allowed8.clear()
 seed()
 assert bool(call(exports['DriTestHsRecoveryPending'],r,context))==(not changed)
 if not changed:
  call(exports['DriTestHs32Begin'],context)
  assert call(exports['DriTestHs30AdmissionBlock'],r,t,context)==0
  call(exports['DriTestHs32Offseason'],t,context)
  assert adds==[(10,134600)] and not offer_status(r,10)
  target=records[board_targets(t)]['array'][0]
  assert records[target]['scalar'].get(12,0)==0
 checks.append({'case':'postweek_queue_rechecks_next_scope','changed_eligibility':changed,'next_scope_admitted':not changed,'current_offer_or_hours_created':False})

call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test34_class_recovery_results.json').write_text(json.dumps({'checks':checks,'passed':True,'native_action_execution':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
