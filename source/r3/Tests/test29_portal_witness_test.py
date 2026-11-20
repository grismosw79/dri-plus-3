"""Compiled read-only post-native-allocation witnesses; no claim that later
weekly influence execution is proved by allocated action flags."""
from pathlib import Path
import ctypes as C, copy, json
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'test29_portal_fixture.py','exec'))
import ctypes as C
class Choice(C.Structure):
 _fields_=[(k,C.c_int)for k in ['pos','role','score','deficit','existing','round','selected','index']]+[('rid',C.c_uint),('rec',C.c_uint64)]+[(k,C.c_int)for k in ['ovr','slot','priority','native_base','plan_score','offered','influence','leader','commit_score','stage','race_ok','raw_ovr','crowd_penalty','crowd_defer','crowd_admitted','coverage_priority','backup_penalty','market_known','market_planned','market_primary','market_protected']]
primary,pt,ppp=pr(3202,83,pos=9,stage=0)
backup,bt,bpp=pr(4783,82,pos=9,stage=0)
valid_offer(primary,pt);valid_offer(backup,bt)
records[tuning]['ref'][49]=record('native actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
pool=(Choice*2)()
for i,r in enumerate([primary,backup]):
 pool[i].rec=r;pool[i].rid=ri(r+0x18);pool[i].selected=1;pool[i].pos=9;pool[i].slot=1;pool[i].round=i+1;pool[i].priority=i+1
address=alloc(C.sizeof(pool));uc.mem_write(address,bytes(pool));call(exports['DriTestCommitPlan'],address,2,team,context)
def observe():
 before=copy.deepcopy(records)
 call(exports['DriTestPortal29Observe'],team,context)
 assert records==before,'read-only witness changed game records'
 assert call(exports['DriTestQualityFault'])==0
 return [call(exports['DriTestPortal29Witness'],ri(r+0x18))for r in [primary,backup]]
# Displayed scholarship, spent scholarship cost and plan alone are insufficient.
for target in [pt,bt]:records[target]['scalar'].update({6:5,7:0,15:0})
records[board]['scalar'].update({1:10,2:10,3:800})
assert all((f&15)==1 for f in observe())
# Actual native action flags plus valid assigned budget support allocation.
for target in [pt,bt]:records[target]['scalar'][15]=1
records[board]['scalar'][1]=20
flags=observe();assert flags[0]&15==7 and flags[1]&15==11
# Even a positive weekly delta cannot rescue invalid assignment accounting.
records[pt]['scalar'][7]=100;records[board]['scalar'][1]=4
assert all(not(f&14) for f in observe())
# Withdrawal removes actual coverage regardless of retained plan/flags/delta.
records[board]['scalar'][1]=20;records[pt]['scalar'][12]=0
assert not(observe()[0]&15)
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert all(v==1 for v in refs.values())
checks=['Offer-only and scholarship spend do not count as productive native allocation',
 'Native productive assignment plus valid complete-board budget distinguished from planning weight',
 'Selected primary and same-opening conditional backup reported separately',
 'Positive delta does not substitute for invalid funding evidence; withdrawal removes coverage',
 'Observer preserves all game records and balances references']
(OUTPUT/'test29_portal_witness_results.json').write_text(json.dumps({'checks':checks,'phase':'after native allocation before later weekly action processing','actual_paid_execution_verified':False,'live_game_verified':False},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
