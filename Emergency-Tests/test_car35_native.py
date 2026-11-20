from car35_fixture import *
from pathlib import Path
import capstone
cap=Path(os.environ.get('DRI_CAPTURE_DIR','unpacked/DRI_ReadOnly_20261031_170052_33d487'))
raw=(cap/'section_00_00001000.bin').read_bytes()
def native(r,n):u.mem_write(G+r,raw[r-0x1000:r-0x1000+n])
def patch(r,handler,relay):
 u.mem_write(relay,b'\xff\x25\0\0\0\0'+struct.pack('<Q',handler));u.mem_write(G+r,b'\xe8'+struct.pack('<i',relay-G-r-5))
season=record('season',{3:1,4:17,5:2,6:2026,12:1});tuning=record('tuning',{94:1,95:2})
m=record('module');wq(m+0x138,season);wq(m+0x148,tuning)
c=coach(**{'12':4,'134':0,'8':10});t=record('target',{383:20});o=record('old team',{383:10});cr=ref(c);tr=ref(t);old=ref(o)
def wrapper():
 out,cref,_,_=args();cc=rq(cref);incref(cc);wq(out,VT);wq(out+8,cc);wq(out+16,SCHEMA);return out
api(wrapper,G+0x8606aa0);api(lambda:reg('RCX'),G+0x37f5ed0)
status=[0];capacity=[1];gates=[0,0,0]
api(lambda:status[0],G+0x834eea0);api(lambda:capacity[0],G+0x832cce0)
for i,r in enumerate([0x8346e40,0x832e800,0x8346de0]):api(lambda i=i:gates[i],G+r)
native(0x8330e30,724);patch(0x8330f0c,exports['car35_gate_hook'],G+0x105000)
# Compiled outer rule calls captured native broad gate, not its model.
def broad(dest=0,limit=6,flag=0):return call('car35_broad_hook',m,cr,tr,old,limit,flag,dest)&255
assert broad()==1;check('Actual captured broad gate accepts scoped pending-renewal promotion')
records[c]['scalar'][128]=6;assert broad()==0;records[c]['scalar'][128]=0
wq(old,t);assert broad()==0;wq(old,o)
gates[0]=1;gates[1]=1;assert broad()==0;gates[:]=[0,0,0]
records[t]['refs'][407]=c;assert broad()==0;records[t]['refs'].pop(407)
capacity[0]=0;assert broad()==0;capacity[0]=1
check('Native offer capacity, same-school, competing-job and user-school gates remain effective after the exception')
status[0]=1;records[c]['scalar'][12]=0
for role,threshold in [(1,1),(0,2)]:
 records[c]['scalar'][16]=role
 for tenure in [0,threshold,threshold+1]:
  records[c]['scalar'][19]=tenure;assert broad()==int(tenure>threshold)
check('Captured native HC/coordinator tenure boundaries unchanged')
records[c]['scalar'].update({12:7,16:0,17:0,19:10})
for side in [0,1]:
 records[c]['scalar'][134]=side
 for dest in [0,1,2]:assert broad(dest)==int(dest==0 or dest-1==side)
check('Captured native broad gate plus compiled structural side rule permits HC mobility and rejects opposite-side free agents')
assert_refs()
# Execute exact captured fallback instructions as a negative control: zero -> 1.
native(0x83460df,12)
u.reg_write(UC_X86_REG_RCX,7);u.reg_write(UC_X86_REG_R14,0)
u.emu_start(G+0x83460df,G+0x83460eb,count=10)
assert reg('R14')==1
check('Native negative control proves free-agent fallback revives score zero; structural rejection is necessary')
# The native builder branches past insertion on our rejected broad result.
native(0x8345b50,0x30);patch(0x8345b6d,call('DriTestCar35Address',0),G+0x106000)
frame=alloc(0x200);sp=ST+0x90000;wi(frame+0xb0,2)
wq(sp+0x38,c);wq(sp+0x48,o);wi(sp+0x70,6);records[c]['scalar'][134]=0
for regid,val in [(UC_X86_REG_RSP,sp),(UC_X86_REG_RBP,frame),(UC_X86_REG_RSI,m),(UC_X86_REG_R14,tr)]:u.reg_write(regid,val)
stops=[]
def stop_branch(uc,a,n,d):
 if a in [G+0x834620a,G+0x8345b7a]:stops.append(a);uc.emu_stop()
h=u.hook_add(unicorn.UC_HOOK_CODE,stop_branch);u.emu_start(G+0x8345b50,0,count=500000);u.hook_del(h)
assert stops==[G+0x834620a],stops
check('Patched native builder executes its rejection cleanup branch before fallback or candidate insertion')
# Prevalidation adapter validates the full eight-argument bridge and real failure flag.
pre=call('DriTestCar35Address',8);records[c]['scalar'].update({16:2,134:0});success=alloc(8);wi(success,1);seen=[]
api(lambda:1,G+0x88057f0)
def validate():
 seen.append(list(args())+[rq(reg('RSP')+0x28)&255,rq(reg('RSP')+0x30),rq(reg('RSP')+0x38)]);return 1
api(validate,G+0x88384b0)
u.reg_write(UC_X86_REG_RDI,cr)
assert call(pre,m,0x100,7,tr,1,0x200,success)==0 and not seen and ri(success)==0
records[c]['scalar'][134]=1;wi(success,1);u.reg_write(UC_X86_REG_RDI,cr)
assert call(pre,m,0x100,7,tr,1,0x200,success)==1
assert seen[-1]==[m,0x100,7,tr,1,0x200,success]
api(lambda:0,G+0x88057f0);records[c]['scalar'][134]=0;u.reg_write(UC_X86_REG_RDI,cr)
assert call(pre,m,0x100,7,tr,1,0x200,success)==1
check('User prevalidation rejects before native transaction, forwards seven original args, and preserves non-coach character paths')
assert_refs()
r={'passed':True,'checks':checks,'captured_native_broad_and_builder_branches':True,'sdf_and_os_modeled':True,'live_acceptance':False}
(ROOT/'native_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
