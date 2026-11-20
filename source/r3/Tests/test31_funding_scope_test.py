"""Current native-vector provenance distinguishes retained historical pitches.
Compiled callbacks run against modeled record storage and native vector input.
"""
from pathlib import Path
import json,copy
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'portal fixture','exec'))
a,at,ap=pr(31101,83,pos=9,stage=0);b,bt,bp=pr(31102,82,pos=9,stage=0)
valid_offer(a,at);valid_offer(b,bt)
records[tuning]['ref'][49]=record('actions',array=[record('hard pitch',{0:2,1:40,2:40,4:1,5:1,6:0})])
for target in [at,bt]:records[target]['ref'][0]=record('pitches',array=[record('pitch',{0:1})])
records[board]['scalar'].update({1:40,2:0,3:800})
mask=alloc(4);v=alloc(144)
for i,(r,t,score)in enumerate([(a,at,300),(b,bt,0)]):
 wq(v+i*72,r);wi(v+i*72+40,score);wq(v+i*72+48,t)
before=copy.deepcopy(records)
assert call(exports['DriTest31ProductiveAllocated'],at,team,context,mask)==40
assert call(exports['DriTest31ProductiveAllocated'],bt,team,context,mask)==40
call(exports['DriTest31FundingRecord'],team,context,v,2)
assert call(exports['DriTest31ProductiveAllocated'],at,team,context,mask)==40
assert call(exports['DriTest31ProductiveAllocated'],bt,team,context,mask)==0 and ri(mask)==0
call(exports['DriTestPortal29Observe'],team,context)
assert call(exports['DriTestPortal29Witness'],ri(a+0x18))&16
assert not(call(exports['DriTestPortal29Witness'],ri(b+0x18))&2)
assert records==before,'ledger must not erase old flags or change native budget'
# A participating target still requires native funded flags and valid budget.
wi(v+112,300);call(exports['DriTest31FundingRecord'],team,context,v,2)
call(exports['DriTestPortal29Observe'],team,context)
assert not(call(exports['DriTestPortal29Witness'],ri(a+0x18))&16),'membership must not fabricate affordability'
records[board]['scalar'][1]=80
records[at]['ref'][0]=record('no current pitch',array=[])
assert call(exports['DriTest31ProductiveAllocated'],at,team,context,mask)==0
# Exact scope: an old stamp cannot suppress another week/save's valid flags.
wi(v+112,0);call(exports['DriTest31FundingRecord'],team,context,v,2)
records[season]['scalar'][4]+=1
assert call(exports['DriTest31ProductiveAllocated'],bt,team,context,mask)==40
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(x==1 for x in refs.values()),[(records[r]['name'],x)for r,x in refs.items()if x!=1]
checks=['Positive current-vector target counted; zero-score retained40h pitch excluded without native edits',
 'Complete-board certification remains current productive<=assigned<=total',
 'Positive vector membership alone cannot certify payment, affordability or firm acceptance',
 'Actual absence of funded flags returns0; week changes invalidate old provenance']
(OUTPUT/'test31_funding_scope_results.json').write_text(json.dumps({'checks':checks,'modeled':['native vector input','record storage','native productive action flags'],'live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+x for x in checks))
