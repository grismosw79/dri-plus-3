"""Compiled rejection tracing consumes existing gates without native writes."""
from pathlib import Path
import json
src=(Path(__file__).parent/'test31_class_recovery_test.py').read_text()
exec(compile(src.split('# Early normal intake')[0],'class recovery fixture','exec'))
checks=[]
for why,expected in [('native',11),('score',14),('hours',16),('annual',5),('accepted',22)]:
 t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
 r,p=candidate(134400,10)
 if why=='native':allowed8.clear()
 if why=='score':scores[134400]=0
 if why=='hours':records[records[t]['ref'][224]]['scalar'][3]=9
 if why=='annual':
  signed,sp=candidate(134401,10);records[signed]['scalar'][9]=5
 before=snap8(r);budget=dict(records[records[t]['ref'][224]]['scalar'])
 got=call(exports['DriTestHs34RecoveryGate'],r,t,context)
 # Enum is deliberately stable; report both label scenario and actual code.
 assert got==expected,(why,got,expected)
 assert snap8(r)==before and records[records[t]['ref'][224]]['scalar']==budget
 if why=='native':
  text=bytes(call(exports['DriTestHs34ReasonByte'],i) for i in range(96)).split(b'\0',1)[0]
  assert text==b'NATIVE_REJECTED_NO_REASON',text
 checks.append({'case':why,'gate':got,'candidate_and_budget_unchanged':True})
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test34_diagnostics_runtime_results.json').write_text(json.dumps({'checks':checks,'passed':True,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
