"""Compiled recovery admission/retention plus captured native replacement branch.
SDF/Windows services and native board storage are controlled fixtures. The
actual native retention branch executes. Complete native spending/signing is
not modeled as a live outcome.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r14_offseason_test.py').read_text()
exec(compile(src.split('for rid,tid,pos,ovr in [(2991')[0],'R14 admission fixture','exec'))
results=[]
negative=os.environ.get('DRI_R15_NEGATIVE')=='1'
def setup(rid=2991,tid=2,pos=9,ovr=75):
 r,t=case(rid,tid,pos,ovr)
 call(exports['DriTestHs32Offseason'],t,context)
 assert adds==[(tid,rid)] and len(records[board_targets(t)]['array'])==1
 target=records[board_targets(t)]['array'][0]
 # Cache scope is initialized by the actual pre-iterator callback. Maintenance
 # is already complete in the modeled calendar; disable its full-market work
 # while initializing and then restore the verified pending reservation.
 af=alloc(0x500);asp=af+128;bp=af+0x300;tr=alloc(8);wq(tr,t)
 wq(af+2*8,context);wq(af+3*8,tr);wq(af+10*8,bp);wq(bp+0x28,tr)
 uc.mem_write(asp+0x42,b'\1')
 # Diagnostic preparation executes the real cache/frame/room initialization
 # without resetting the deliberately supplied active recovery reservation.
 configure(0);call(hooks[8],af);configure(1)
 wq(bp-0x58,r);wq(bp-0x38,target)
 return r,t,target,af,asp,bp
def retain(r,t,target,af,asp,bp):
 uc.mem_write(asp+0x44,b'\1');uc.mem_write(asp+0x40,b'\1')
 before_r=dict(records[r]['scalar']);before_t=dict(records[target]['scalar']);before_b=dict(records[records[t]['ref'][224]]['scalar'])
 call(hooks[14],af)
 assert records[r]['scalar']==before_r and records[target]['scalar']==before_t and records[records[t]['ref'][224]]['scalar']==before_b
 # Native target is absent from the portal shortlist. Its default replaceable
 # flag puts it in the removal vector; a valid reservation must survive.
 map_game(0x08172d10,0x1000);uc.mem_write(GAME+0x08172d10,raw[0x08172d10-0x1000:0x08173d10-0x1000])
 uc.ctl_remove_cache(GAME+0x08172d10,GAME+0x08173d10)
 wq(bp+0x40,0);wq(asp+0x68,0)
 for rn,v in [('RSP',asp),('RBP',bp),('RAX',0),('R15',0)]:uc.reg_write(globals()['UC_X86_REG_'+rn],v)
 outcome=[]
 def stop(u,address,size,data):
  if address in [GAME+0x0817326a,GAME+0x08173409]:outcome.append(address);u.emu_stop()
 h=uc.hook_add(unicorn.UC_HOOK_CODE,stop)
 uc.emu_start(GAME+0x081731f3,GAME+0x08173410,count=100)
 uc.hook_del(h);assert len(outcome)==1
 return outcome[0]==GAME+0x08173409
for rid,tid,pos,ovr in [(2991,2,9,75),(2991,91,9,75),(1190,42,12,72),(1190,53,12,72)]:
 args_case=setup(rid,tid,pos,ovr);survives=retain(*args_case)
 assert survives is not negative,(rid,tid,survives)
 results.append({'rid':rid,'tid':tid,'native_retained':survives,'offer_created':bool(offer_status(args_case[0],tid))})
 assert not offer_status(args_case[0],tid)
if negative:
 print('R14_NEGATIVE_REPRODUCED: all four admitted HS recovery shapes enter native replacement before funding')
 (OUTPUT/'test28r15_negative_results.json').write_text(json.dumps(results,indent=2));raise SystemExit(0)
for loss in ['eligibility','budget','published','hard','user','regular','no_reservation','target_mismatch']:
 r,t,target,af,asp,bp=setup(99000,42,12,72)
 if loss=='eligibility':allowed8.clear()
 elif loss=='budget':records[records[t]['ref'][224]]['scalar'][1]=100
 elif loss=='published':records[records[t]['ref'][33]]['array']=[records[r]['ref'][5]]
 elif loss=='hard':records[r]['scalar'][9]=5
 elif loss=='user':records[t]['ref'][407]=record('user')
 elif loss=='regular':records[season]['scalar'][5]=1
 elif loss=='no_reservation':call(exports['DriTestHsCoverageReset'])
 elif loss=='target_mismatch':wq(bp-0x38,0)
 assert not retain(r,t,target,af,asp,bp),loss
 records[t]['ref'].pop(407,None)
 results.append({'invalidated':loss,'native_retained':False})
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
(OUTPUT/'test28r15_recovery_retention_results.json').write_text(json.dumps({'cases':results,'live_game_verified':False,'limits':__doc__},indent=2))
print(json.dumps(results,indent=2))
