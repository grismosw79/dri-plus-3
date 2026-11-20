"""Compiled R5 ceiling repair with the captured native availability helper.
SDF record/ref services, scratch-vector initialization and Win32 are modeled.
"""
from car35_fixture import *
import hashlib
capture=Path(os.environ.get('DRI_CAPTURE_DIR','unpacked/DRI_ReadOnly_20261031_174604_3f8adb'))
raw=(capture/'section_00_00001000.bin').read_bytes()
u.mem_write(G+0x833bd00,raw[0x833bd00-0x1000:0x833c308-0x1000])
api(lambda:reg('RCX'),G+0x37f5ed0)
api(lambda:0,G+0x78cb4c0)
def scratch():
    vector,count,_,_=args();assert count==3
    begin=rq(vector);u.mem_write(begin,b'\0'*36);wq(vector+8,begin+36);return 0
api(scratch,G+0x8330600)
team=record('Indiana',{383:34,59:280,184:280,227:1465})
job=record('unfilled DC job',{1:0,3:40,6:2},ref={10:team})
other=record('other school',{383:10,227:9000})
otherjob=record('unrelated job',{1:0,3:9000,6:2},ref={10:other})
array=record('openings',array=[otherjob,job])
m=record('staff hiring module',ref={21:array})
def ceiling(original=85,caller=0x8352cdd,role=2,enabled=1,active=1):
 config(active);call('DriTestCar35Context',m,team,role,enabled)
 return call('DriTestCar35Budget',original,caller)
before={r:dict(x['scalar'])for r,x in records.items()}
assert ceiling()==1505
assert before=={r:dict(x['scalar'])for r,x in records.items()}
check('Normal CPU ceiling can reach native 1505 availability (1465 uncommitted plus own 40 reservation); no budget or job record changes')
records[team]['scalar'][227]=150
assert ceiling()==190 # 150 remaining + own unfilled offer reservation 40
records[job]['scalar'][1]=1;records[job]['scalar'][2]=9000
assert ceiling()==85 # filled slot has zero available, not its final contract
records[job]['scalar'][1]=0
records[team]['scalar'][227]=0
assert ceiling()==85 # insufficient native funds never create an increase
records[team]['scalar'][227]=1465
check('Actual native per-job reservations honored; other-school money and filled-job final contracts never expand the ceiling')
for original,role,enabled,active,caller in [(0,2,1,1,0x8352cdd),(85,0,1,1,0x8352cdd),(85,2,0,1,0x8352cdd),(85,2,1,0,0x8352cdd),(85,2,1,1,0x8329419),(85,2,1,1,0x83362d8),(1600,2,1,1,0x8352cdd)]:
 assert ceiling(original,caller,role,enabled,active)==original
assert ceiling(caller=0x892df4e)==ceiling()==1505
check('CPU and UI callers have equal available-fund ceilings; zero budgets, HC, observe/disabled, emergencies and already-higher ceilings stay native')
# OC uses its own role budget and its own reservation.
records[job]['scalar'][6]=1;records[team]['scalar'][184]=170;records[team]['scalar'][59]=900
assert ceiling(role=1)==1505
records[team]['scalar'][227]=80;assert ceiling(role=1)==120
check('OC uses native own-role reservation availability; existing allocation is logged without rewriting or fabricating funds')
# The actual builder receives the repaired integer and all other arguments once.
# module observer is bypassed via same-phase sentinel by using real records;
# invoking the hook also verifies the context's CPU/user decision.
season=record('active carousel',{3:1,4:18,5:3,6:2026,12:1});wq(m+0x138,season);wi(m+0x1f8,240)
tuning=record('paired R9 tuning',{118:0,119:0});wq(m+0x148,tuning)
# Observe functions need no franchise: no mutations or retained refs.
seen=[];out=alloc(24);u.mem_write(out,b'\0'*24)
def native_builder():
 seen.append(list(args())+[rq(reg('RSP')+0x28),rq(reg('RSP')+0x30),rq(reg('RSP')+0x38)]);return 0
api(native_builder,G+0x8345870)
# Call from a tiny fixture at the real normal-caller return RVA; sets the
# stack argument slots and RCX/RDX/R8/R9 through call() then executes patched call.
relay=G+0x105000
u.mem_write(relay,b'\xff\x25\0\0\0\0'+struct.pack('<Q',exports['car35_builder_hook']))
u.mem_write(G+0x8352cd8,b'\xe8'+struct.pack('<i',relay-G-0x8352cdd)+b'\xc3')
# Wrapper's call changes the stack argument placement: copy arguments down
# from the fixture entry before this native call using a separate small stub.
stub=G+0x106000
# sub rsp, 0x38; move original stack args into outgoing 5/6/7; call site;
# use a jump to the real call then return into cleanup after a patched jump.
code=bytes.fromhex('4883ec38488b4424604889442420488b4424684889442428488b4424704889442430')
code+=b'\xe9'+struct.pack('<i',G+0x8352cd8-(stub+len(code)+5))
u.mem_write(stub,code)
cleanup=stub+0x80;u.mem_write(cleanup,bytes.fromhex('4883c438c3'))
u.mem_write(G+0x8352cdd,b'\xe9'+struct.pack('<i',cleanup-G-0x8352ce2))
records[team]['scalar'][227]=1465;records[team]['scalar'][184]=170
config();call(stub,m,ref(team),1,out,0,0,85)
assert len(seen)==1 and seen[0][-3:]==[0,0,1505],seen
records[team]['refs'][407]=coach('user coach')
config();call(stub,m,ref(team),1,out,0,0,85)
assert seen[-1][-1]==1505 and len(seen)==2
records[tuning]['scalar'][119]=25
config();call(stub,m,ref(team),1,out,0,0,85)
assert seen[-1][-1]==85 and len(seen)==3
check('Real normal-call ABI supplies identical 1505 ceilings to CPU/user schools; unpaired FTCs disable new evaluation and retain native 85')
assert_refs()
result={'passed':True,'checks':checks,'captured_native_available_funds_executed':True,'native_helper_sha256':hashlib.sha256(raw[0x833bd00-0x1000:0x833c308-0x1000]).hexdigest(),'live_acceptance':False,'modeled':['SDF/OS services','scratch-vector initialization','candidate builder body']}
(ROOT/'r9_budget_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
