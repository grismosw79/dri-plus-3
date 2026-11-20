"""Current captured native broad gate and builder rejection branch."""
from test_car35_native import *
checks.clear()
records[c]['scalar'].update({0:0,8:10,16:1,17:1,134:0,19:3,128:0})
records[t]['refs'].pop(407,None);wq(old,o);capacity[0]=1;gates[:]=[0,0,0]
for contract in [0,2,4,7]:
 records[c]['scalar'][12]=contract;status[0]=0 if contract==4 else 1
 for grade in [0,5,6,7,8,12,13]:
  records[c]['scalar'][112]=grade
  for emergency in [0,1]:assert broad(0,6,emergency)==int(grade<=6),(contract,grade,emergency)
check('Captured native broad gate plus compiled policy: C+ inclusive across active, fired, renewed and unemployed coordinators, including emergency flag')
records[c]['scalar'].update({12:4,112:12});status[0]=0
assert broad()==0
records[c]['scalar'][112]=6;assert broad()==1
check('The narrow pending-renewal promotion exception cannot override the minimum grade')
records[c]['scalar'].update({12:7,112:12,16:1,17:1,134:0});status[0]=1
frame=alloc(0x200);sp=ST+0x90000;wi(frame+0xb0,0)
wq(sp+0x38,c);wq(sp+0x48,o);wi(sp+0x70,6)
for regid,val in [(UC_X86_REG_RSP,sp),(UC_X86_REG_RBP,frame),(UC_X86_REG_RSI,m),(UC_X86_REG_R14,tr)]:u.reg_write(regid,val)
stops=[]
def stop_grade_branch(uc,a,n,d):
 if a in [G+0x834620a,G+0x8345b7a]:stops.append(a);uc.emu_stop()
h=u.hook_add(unicorn.UC_HOOK_CODE,stop_grade_branch)
u.emu_start(G+0x8345b50,0,count=500000);u.hook_del(h)
assert stops==[G+0x834620a],stops
check('F-grade unemployed coordinator takes the captured builder rejection cleanup branch before zero-to-one fallback or insertion')
# Normal offer scarcity is not an empty emergency vector. Execute the native
# emergency first-entry selection with an eligible C+ coordinator scoring zero.
native(0x83362d8,0x70)
records[c]['scalar'].update({112:6,16:1,17:1,134:0})
assert call('DriTestCar35Eligible',c,0,0)==1
vec=alloc(48);wq(vec,c);wi(vec+24,0);frame=alloc(0x200)+0x80;sp=ST+0x90000
wq(frame+0xf,vec);wq(frame+0x17,vec+48)
for regid,val in [(UC_X86_REG_RSP,sp),(UC_X86_REG_RBP,frame),(UC_X86_REG_RSI,0),(UC_X86_REG_R14,0)]:u.reg_write(regid,val)
stops=[]
def stop_emergency_pick(uc,a,n,d):
 if a in [G+0x833630e,G+0x8336347]:stops.append(a);uc.emu_stop()
h=u.hook_add(unicorn.UC_HOOK_CODE,stop_emergency_pick)
u.emu_start(G+0x83362d8,0,count=500000);u.hook_del(h)
assert stops==[G+0x8336347]and rq(frame-0x49)==c
decref(c)
check('Captured native emergency selection accepts a nonempty eligible C+ vector even when its leading score is zero; no score/interest/tenure override added')
assert_refs()
result={'passed':True,'checks':checks,'captured_game':json.loads((cap/'report.json').read_text(encoding='utf-8-sig'))['ExecutableSha256'],
 'captured_native_control_flow':True,'windows_and_record_services_modeled':True,'live_acceptance':False}
(ROOT/'promotion_native_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
