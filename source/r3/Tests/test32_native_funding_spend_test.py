"""Compiled funding order through captured second allocation pass and actual
productive-action flag/hour writes. Native SDF storage/eligibility leaves and
per-target entitlement are controlled; full native budget math/control flow,
productive action selection, flag setting, school/target accounting execute.
Not a complete first-pass, week advance or live game test.
"""
from pathlib import Path
import json,os,struct
T=Path(__file__).parent
exec(compile((T/'test32_native_funding_order_test.py').read_text(),'native funding order fixture','exec'))
# Keep compiled callback's vector and index ordering. Baseline mode intentionally
# leaves the backup physically first, while the revised callback moves primary.
for r in [backup,primary,portal]:records[r]['scalar'][9]=1 # align actual stage with native vector bucket
native_eval=alloc(0x1000);uc.mem_write(native_eval,bytes(uc.mem_read(context,0x300)))
actions=alloc(8);wi(actions,5);wi(actions+4,5) # native social action/cost
wq(native_eval+0x300,actions);wq(native_eval+0x308,actions+8)
# Captured native functions; each begins at a verified function entry except
# the enclosing allocation loop, which starts with its documented live frame.
regions=[(0x08172310,0x08172852),(0x08177900,0x08177eac),(0x0814d170,0x0814d517),
 (0x0816b870,0x0816ba2c),(0x08628f30,0x08629001),(0x0862b2c0,0x0862b355),
 (0x0862b910,0x0862b99d),(0x081b84c0,0x081b85dc),(0x0817e7b0,0x0817e9f9),(0x0819a8f0,0x0819a9b0)]
for lo,hi in regions:
 map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000]);handlers.pop(GAME+lo,None);uc.ctl_remove_cache(GAME+lo,GAME+hi)
# Record access and writes are storage services. Actual captured setters choose
# the target's action15 and the school's assigned-hours field1.
writes=[]
def write_scalar():
 table,row_,field,value=args();r=tables[table][row_];records[r]['scalar'][field-0x100000]=value&0xffffffff
 writes.append((r,field-0x100000,value&0xffffffff));return 0
wq(TABLEVT+0x140,api(write_scalar));wq(TABLEVT+0x148,api(write_scalar))
game_api(0x08059580,lambda:0)
allowed=True
elig_calls=[]
def eligibility_spend():
 action=ri(reg('RSP')+0x28);target=rq(reg('R8'));elig_calls.append((target,action))
 return int(allowed and action==5)
for address,fn in [(0x0819adf0,eligibility_spend),(0x0819a9b0,lambda:1),(0x081ab1b0,lambda:0),
 (0x08180320,lambda:50),(0x081784c0,lambda:reg('R9')),(0x08103c20,lambda:0),(0x0815f390,lambda:0)]:game_api(address,fn)
# Match the published captured helper test's disclosed Unicorn workaround for
# exactly two scalar AVX multiplication instructions.
def f32(bits):return struct.unpack('<f',struct.pack('<I',bits&0xffffffff))[0]
def multiply_native(u,address,size,data):
 if address==GAME+0x0817e915:dest=UC_X86_REG_XMM3;value=f32(u.reg_read(UC_X86_REG_XMM2))*f32(u.reg_read(UC_X86_REG_XMM0))
 elif address==GAME+0x0817e95c:dest=UC_X86_REG_XMM1;value=f32(u.reg_read(UC_X86_REG_XMM0))*f32(u.reg_read(UC_X86_REG_XMM3))
 else:return
 u.reg_write(dest,struct.unpack('<I',struct.pack('<f',value))[0]);u.reg_write(UC_X86_REG_RIP,address+4)
uc.hook_add(unicorn.UC_HOOK_CODE,multiply_native)
def float_one():uc.reg_write(UC_X86_REG_XMM0,0x3f800000);return 0
wq(TABLEVT+0x110,api(float_one))
budget=records[allteams[96]]['ref'][224]
loop_frame=STACK+0x77000;loop_sp=STACK+0x75000
wq(loop_frame-0x50,vec);wq(loop_frame-0x58,3);wq(loop_frame-0x60,team_holder)
uc.mem_write(loop_sp+0x50,b'\0')
# Native context is wide enough for all allocation, pitch and source vectors.
# Same source bucket for all3 entries; five hours remain of an initial300.
wi(native_eval+0x234,0);wi(native_eval+0x238,1377);wi(native_eval+0x240,0)
wi(native_eval+0x244,300);wi(native_eval+0x24c,0)
wi(native_eval+0x1b4,0)
# No-op output stream for native diagnostics not reached by normal cases.
game_api(0x037f4530,lambda:0)
def execute_case(name,remaining,eligible,expected_winner):
 global allowed,writes,elig_calls
 allowed=eligible;writes=[];elig_calls=[]
 records[budget]['scalar'].update({1:300-remaining,2:0,3:300})
 for target in [bt,pt,portal_target]:
  for field in range(15,20):records[target]['scalar'][field]=0
  records[target]['scalar'][2]=0
 for off in [0x224,0x228,0x22c,0x230,0x254,0x25c,0x260]:wi(native_eval+off,0)
 wi(native_eval+0x250,300-remaining)
 uc.reg_write(UC_X86_REG_RSP,loop_sp);uc.reg_write(UC_X86_REG_RBP,loop_frame)
 uc.reg_write(UC_X86_REG_RSI,native_eval);uc.reg_write(UC_X86_REG_R12,team_holder);uc.reg_write(UC_X86_REG_R13,0)
 endpoint=GAME+0x08172852
 def stop(u,address,size,data):
  if address==endpoint:u.emu_stop()
 token=uc.hook_add(unicorn.UC_HOOK_CODE,stop)
 try:uc.emu_start(GAME+0x08172310,endpoint+1,count=1000000)
 except unicorn.UcError:
  print('spending RIP',hex(reg('RIP')),'recent',list(recent));raise
 finally:uc.hook_del(token)
 assert reg('RIP')==endpoint,(name,hex(reg('RIP')))
 winners=[r for r,t in [(backup,bt),(primary,pt),(portal,portal_target)] if records[t]['scalar'][15]]
 assert winners==([expected_winner] if expected_winner else []),(name,winners,expected_winner,writes,elig_calls)
 spent=records[budget]['scalar'][1]-(300-remaining)
 assert spent==(5 if expected_winner else 0),(name,spent,writes)
 assert spent<=remaining and records[budget]['scalar'][1]<=300
 assert ri(native_eval+0x250)==300-remaining+spent
 assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
 return {'case':name,'remaining_before':remaining,'eligible':eligible,'productive_winner':ri(expected_winner+0x18) if expected_winner else None,
  'native_assigned_delta':spent,'stage_spent_after':ri(native_eval+0x250),'action_flags':{str(ri(r+0x18)):records[t]['scalar'][15] for r,t in [(backup,bt),(primary,pt),(portal,portal_target)]},
  'scalar_writes':[{'record':records[r]['name'],'field':f,'value':v} for r,f,v in writes]}
results=[execute_case('tight_shared_budget',5,True,backup if baseline else primary),
 execute_case('native_eligibility_refusal',5,False,None),execute_case('no_native_hours',0,True,None),
 execute_case('below_action_minimum',4,True,None)]
assert call(exports['DriTestQualityFault'])==0
report={'passed':True,'baseline_defect_reproduced':baseline,'cases':results,
 'captured_native':[hex(a) for a,b in regions],
 'modeled_services':['SDF records and scalar writes','action eligibility leaf restricted to social5','per-target entitlement and cap','two scalar AVX multiplications','Windows'],
 'not_executed':['full first allocation pass','native building of the one-action list','weekly advance','recruit influence delivery'],
 'live_game_verified':False}
(OUTPUT/'test32_native_funding_spend_results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS captured second allocation pass selects productive social action, writes native target flag, spends5 actual shared hours, and respects refusal/empty/below-minimum budgets')
