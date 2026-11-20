"""Regular HS minimum-share correction through captured native allowance code.
SDF, Windows, entitlement/per-recruit cap services and two scalar VMULSS
instructions are controlled; native branches, truncation and clamps execute.
No scholarship/action/hour spending is fabricated or claimed.
"""
from pathlib import Path
import json,struct
T=Path(__file__).parent
exec(compile((T/'test31_hs_runtime_test.py').read_text().split('# Full inherited room')[0],
             'Test32 funding fixture definitions','exec'))
reset8();next_single_year(10)
primary,pp_,pt=target31(32101,500,80,stage=0)
backup,bp,bt=target31(32102,400,72,stage=0)
assert (role31(primary),role31(backup))==(1,2)
team=allteams[96]
records[tuning]['ref'][49]=record('native action table',array=[
 record('scholarship',{0:3,1:0,2:5,5:1,6:1}),
 record('social',{0:5,1:10,2:5,5:1,6:0})])
# Execute the native proportional share, stage dispatch, integer truncation,
# stage remaining and per-target clamp. Only dependencies are modeled.
for lo,hi in [(0x0817e7b0,0x0817e9f9),(0x0819a8f0,0x0819a9b0)]:
 map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000]);handlers.pop(GAME+lo,None)
 uc.ctl_remove_cache(GAME+lo,GAME+hi)
def native_float():uc.reg_write(UC_X86_REG_XMM0,0x3f800000);return 0
wq(TABLEVT+0x110,api(native_float))
def f32(bits):return struct.unpack('<f',struct.pack('<I',bits&0xffffffff))[0]
def scalar_multiply(u,a,size,data):
 if a==GAME+0x0817e915:value=f32(u.reg_read(UC_X86_REG_XMM2))*f32(u.reg_read(UC_X86_REG_XMM0));dest=UC_X86_REG_XMM3
 elif a==GAME+0x0817e95c:value=f32(u.reg_read(UC_X86_REG_XMM0))*f32(u.reg_read(UC_X86_REG_XMM3));dest=UC_X86_REG_XMM1
 else:return
 u.reg_write(dest,struct.unpack('<I',struct.pack('<f',value))[0]);u.reg_write(UC_X86_REG_RIP,a+4)
uc.hook_add(unicorn.UC_HOOK_CODE,scalar_multiply)
ceiling=50
game_api(0x081784c0,lambda:reg('R9'))
game_api(0x08180320,lambda:ceiling)
vec=alloc(144);indices=alloc(16);vh=alloc(8);tr=alloc(8);th=alloc(8)
wq(vh,vec);wq(tr,team);wq(th,pt)
for i,(rec,tg) in enumerate([(primary,pt),(backup,bt)]):
 uc.mem_write(vec+i*72,b'\0'*72);wq(vec+i*72,rec);wq(vec+i*72+48,tg);wq(indices+i*8,i)
wi(context+0x234,100);wi(context+0x238,100)
wi(context+0x240,50);wi(context+0x244,50)
wi(context+0x24c,0);wi(context+0x250,0)
def native_limit():
 wq(STACK+0xf0000-8+0x28,vh)
 return call(GAME+0x0817e7b0,context,tr,th,0)
def run_repair():
 wq(STACK+0xf0000-8+0x28,indices)
 return call(exports['DriTest32HsMinimum'],team,context,vec,2)
def weights():return [ri(vec+40),ri(vec+112)]
def prepare(offered=1,spent=0,cap=50,cross_bucket=False):
 global ceiling
 ceiling=cap;records[pt]['scalar'][12]=2 if offered else 0
 records[primary]['scalar'][9]=0;records[backup]['scalar'][9]=1 if cross_bucket else 0
 wi(vec+40,1);wi(vec+112,99);wi(vec+64,0);wi(vec+136,1 if cross_bucket else 0)
 wi(context+0x24c,spent);wi(context+0x250,0)
checks=[]
for offered,expected_cost in [(1,5),(0,10)]:
 prepare(offered)
 before_records={r:(dict(v['scalar']),dict(v['ref'])) for r,v in records.items()}
 before=native_limit();run_repair();after=native_limit()
 assert before==0 and after==expected_cost,(offered,before,after,weights())
 assert weights()==[expected_cost*2,100-expected_cost*2]
 assert {r:(dict(v['scalar']),dict(v['ref'])) for r,v in records.items()}==before_records
 checks.append({'case':'already offered' if offered else 'offer plus productive action','minimum_cost':expected_cost,
                'before':before,'after':after,'weights':weights(),'record_writes':False})
for label,kwargs in [('per-recruit cap below action',{'cap':4}),('remaining stage budget below action',{'spent':46}),
                      ('other stage cannot donate',{'cross_bucket':True})]:
 prepare(**kwargs);before=native_limit();run_repair()
 assert weights()==[1,99],(label,weights())
 checks.append({'case':label,'before':before,'after':native_limit(),'scores_restored':True})
# Another selected primary cannot be mined to pay this one. Move the donor to
# another genuine annual position, preserving its legal/viable primary plan.
prepare();records[bp]['scalar'][3]=11
assert role31(primary)==1 and role31(backup)==1
run_repair();assert weights()==[1,99]
checks.append({'case':'another firm primary protected','weights':weights()})
# A portal record and an acquired player also cannot donate to the HS repair.
records[bp]['scalar'][3]=10
prepare();old_class=records[backup]['scalar'][2];records[backup]['scalar'][2]=4
run_repair();assert weights()==[1,99],('portal protected',weights())
records[backup]['scalar'][2]=old_class
checks.append({'case':'portal protected','weights':weights()})
prepare();incoming=records[team]['ref'][33];records[incoming]['array'].append(bp)
run_repair();assert weights()==[1,99],('acquired protected',weights())
records[incoming]['array'].remove(bp)
checks.append({'case':'acquired protected','weights':weights()})
# Integrated callback: regular40:20 primary/backup reweighting alone gives4/2
# out of a six-hour native stage allowance. The new native-confirmed transfer
# changes500/100 score share to5/1 without adding the missing hour to a budget.
prepare();wi(vec+40,300);wi(vec+112,300)
wi(context+0x234,600);wi(context+0x240,6)
ff=STACK+0x55000;fb=ff+0x400;wq(ff+80,fb);wq(ff+72,context)
wq(fb-0x60,tr);wq(fb-0x50,vec);wq(fb-0x48,vec+144);wq(fb+0x58,indices);wq(fb+0x60,indices+16)
before_records={r:(dict(v['scalar']),dict(v['ref'])) for r,v in records.items()}
call(hooks[23],ff)
assert weights()==[500,100] and native_limit()==5,weights()
assert {r:(dict(v['scalar']),dict(v['ref'])) for r,v in records.items()}==before_records
assert ri(context+0x240)==6 and ri(context+0x24c)==0
checks.append({'case':'integrated actual regular funding hook','stage_hours':6,'weights':weights(),'usable_native_allowance':5,'no_spending_claimed':True})
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
assert call(exports['DriTestQualityFault'])==0
report={'passed':True,'cases':checks,'native_allowance_rva':'0x0817e7b0','native_stage_dispatch_rva':'0x0819a8f0',
 'preserved':['per-bucket native score totals','other primary score','stage remaining hours','per-recruit cap','offers, action flags, budgets and influence'],
 'modeled_services':['SDF','Windows','entitlement and per-recruit limit','two scalar VMULSS instructions'],
 'live_game_verified':False,'limitations':['Native allowance is exercised; native actions and whole weekly advance are not executed.']}
(OUTPUT/'test32_hs_minimum_share_results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS regular-season primary reaches native productive-action allowance; native caps/buckets/other primaries remain authoritative')
