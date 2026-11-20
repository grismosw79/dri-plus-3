"""Compiled eligibility on current conference arrays, native fallback branch.
No school-ID whitelist, prestige override or career-history substitution.
"""
from car35_fixture import *

c=coach(**{'16':0,'17':2,'12':0,'8':10,'134':0});old=record('current employer',{383:10},ref={111:c})
target=record('destination',{383:20});conf=record('conference',{22:3},ref={40:record('members',array=[target])})
confs=record('conferences',array=[conf]);league=record('league',ref={3:confs})
franchise=record('franchise',ref={27:league,45:record('teams',array=[old,target])})
m=record('StaffHiringEval',ref={20:franchise});cr,tr,orig=ref(c),ref(target),ref(old)
native_calls=[]
api(lambda:(native_calls.append(args())or 1),G+0x8330e30)
api(lambda:(native_calls.append(('reconstruct',args()))or 1),G+0x834a8e0)
count=0
for user in [0,1]:
 records[c]['scalar'][0]=user
 for status in [0,2,3,4,5,6,7,8]:
  records[c]['scalar'][12]=status
  for cc in range(11):
   records[conf]['scalar'][22]=cc
   assert call('DriTestCar35Conference',m,target)==cc
   for role in [0,1,2]:
    # Both coordinator sides are tested with coherent coach identity.
    records[c]['scalar'][134]=max(0,role-1)
    blocked=status in [0,4] and role in [1,2] and cc not in [0,1,2,9]
    before=len(native_calls)
    assert call('car35_broad_hook',m,cr,tr,orig,6,0,role)==int(not blocked),(user,status,cc,role)
    assert len(native_calls)==before+int(not blocked)
    assert call('DriTestCar35HCMove',m,c,0,target,role)==int(not blocked)
    count+=1
check(f'{count} CPU/user, contract-status, conference and destination combinations: only Signed/PendingRenewal incumbent HCs to non-P4 coordinators blocked')

records[c]['scalar'].update({0:0,12:0,134:0});records[conf]['scalar'][22]=3
for flag in [0,1]:
 assert call('car35_broad_hook',m,cr,tr,orig,6,flag,1)==0
recon=call('DriTestCar35Address',1)
assert call(recon,m,tr,cr,1,0,0,0)==0
records[c]['scalar'][12]=2;assert call(recon,m,tr,cr,1,0,0,0)==1
records[c]['scalar'][12]=7;assert call(recon,m,tr,cr,1,0,0,0)==1
records[c]['scalar'][12]=0;records[conf]['scalar'][22]=9
assert call(recon,m,tr,cr,1,0,0,0)==1
records[conf]['scalar'][22]=3
assert call(recon,m,tr,cr,0,0,0,0)==1
check('Normal/emergency pre-fallback and reconstruction paths enforce the same rule; fired/free-agent re-entry, P4 coordinator and HC mobility remain native')

# Realignment changes the answer without changing school identity or prestige.
pac=record('Pac-12',{22:8},ref={40:record('Pac members',array=[])})
sec=record('SEC',{22:9},ref={40:record('SEC members',array=[])})
records[confs]['array']=[conf,pac,sec]
records[records[conf]['refs'][40]]['array']=[]
records[records[pac]['refs'][40]]['array']=[target]
assert call('DriTestCar35HCMove',m,c,old,target,1)==0
records[records[pac]['refs'][40]]['array']=[];records[records[sec]['refs'][40]]['array']=[target]
assert call('DriTestCar35HCMove',m,c,old,target,1)==1
records[records[pac]['refs'][40]]['array']=[target]
assert call('DriTestCar35Conference',m,target)&0xffffffff==0xffffffff
assert call('DriTestCar35HCMove',m,c,old,target,1)==1
records[records[pac]['refs'][40]]['array']=[];records[records[sec]['refs'][40]]['array']=[]
assert call('DriTestCar35HCMove',m,c,old,target,1)==1
records[records[conf]['refs'][40]]['array']=[target]
records[old]['refs'][111]=coach();assert call('DriTestCar35HCMove',m,c,old,target,1)==1
records[old]['refs'][111]=c
records[c]['scalar'][8]=255;assert call('DriTestCar35HCMove',m,c,old,target,1)==1
records[c]['scalar'][8]=10
config(0);assert call('DriTestCar35HCMove',m,c,old,target,1)==1;config()
check('Dynamic membership respects realignment/Pac-12; duplicate/missing membership or unverified employment does not invent a restriction; observe mode remains native')

# The rule cannot revive an opposite-side coach, or block a coordinator's
# otherwise valid lateral move/promotion. It is independent of school grade.
records[c]['scalar'].update({16:1,134:0});assert call('car35_broad_hook',m,cr,tr,orig,6,1,1)==1
assert call('car35_broad_hook',m,cr,tr,orig,6,1,2)==0
records[c]['scalar'].update({16:0,134:0});assert call('car35_broad_hook',m,cr,tr,orig,6,1,2)==0
check('Structural offensive/defensive identity still gates every path; coordinator lateral eligibility unchanged')

# Execute the captured builder instructions that consume the broad result.
cap=Path(os.environ['DRI_CAPTURE_DIR']);raw=(cap/'section_00_00001000.bin').read_bytes()
start=0x8345b50;u.mem_write(G+start,raw[start-0x1000:start-0x1000+0x30])
relay=G+0x106000;u.mem_write(relay,b'\xff\x25\0\0\0\0'+struct.pack('<Q',call('DriTestCar35Address',0)))
site=0x8345b6d;u.mem_write(G+site,b'\xe8'+struct.pack('<i',relay-G-site-5))
frame=alloc(0x200);sp=ST+0x90000;wi(frame+0xb0,1)
wq(sp+0x38,c);wq(sp+0x48,old);wi(sp+0x70,6)
for regid,val in [(UC_X86_REG_RSP,sp),(UC_X86_REG_RBP,frame),(UC_X86_REG_RSI,m),(UC_X86_REG_R14,tr)]:u.reg_write(regid,val)
stops=[]
def stop(uc,a,n,d):
 if a in [G+0x834620a,G+0x8345b7a]:stops.append(a);uc.emu_stop()
h=u.hook_add(unicorn.UC_HOOK_CODE,stop);u.emu_start(G+start,0,count=500000);u.hook_del(h)
assert stops==[G+0x834620a],stops
check('Captured native builder takes rejection/cleanup branch before scoring and fallback insertion for a blocked employed HC')
assert_refs()
r={'passed':True,'checks':checks,'matrix_cases':count,'captured_native_branch':True,'OS_SDF_modeled':True,'live_acceptance':False}
(ROOT/'r10_hc_mobility_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
