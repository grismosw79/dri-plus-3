"""R14 bridge calls captured native AddToBoard machine code. SDF allocation,
record writes, cache and scorer/eligibility services are modeled.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r14_offseason_test.py').read_text()
exec(compile(src.split('for rid,tid,pos,ovr in ')[0],'offseason definitions','exec'))
map_game(0x08149660,0xc00);uc.mem_write(GAME+0x08149660,raw[0x08149660-0x1000:0x0814a210-0x1000]);handlers.pop(GAME+0x08149660,None)
uc.ctl_remove_cache(GAME+0x08149660,GAME+0x0814a210)
map_game(0x0bd0c000,0x2000)
new_targets=[];block_native=False
# Complete native creation runs through the captured CPU branch. No helper
# emulates the admission decision, target initialization or duplicate checks.
def create_target():
 _,out,_,_=args();target=record('native allocated target');new_targets.append(target)
 incref(target);wq(out,target);return out
def write_reference():
 target,field,holder,_=args();records[target]['ref'][field-0x100000]=rq(holder);return 0
def append_target():
 array,holder,_,_=args();records[array]['array'].append(rq(holder));return 0
def return_null():wq(reg('RDX'),0);return reg('RDX')
def native_user():return 0
for address,fn in [(0x081bc0a0,lambda:int(block_native)),(0x03803770,lambda:50),(0x08103c20,native_user),(0x0804b440,create_target),(0x0805e4d0,write_reference),(0x082c0be0,lambda:0),(0x0862db70,lambda:0),(0x08045cd0,append_target),(0x0862d2a0,return_null),(0x081ae440,lambda:0)]:game_api(address,fn)
# The native target defaults query returns an empty pitch array factory.
obj=alloc(16);vt=alloc(0x200);wq(obj,vt);wq(vt+0x198,api(return_null));wq(manager+0x100,obj)
r,t=case(1247,17,3,74);before=snap8(r)
call(exports['DriTestHs32Offseason'],t,context)
a=records[board_targets(t)]['array'];assert len(a)==1 and a[0]in new_targets
assert records[a[0]]['ref'][10]==r and records[a[0]]['scalar'][12]==0
assert records[r]['scalar']==before[0] and values(r)==list(before[1]) and not offer_status(r,17)
assert call(exports['DriTestQualityFault'])==0
# Direct repeat is rejected by native duplicate protection, independently of
# bridge's own idempotence precheck.
tr=alloc(8);wq(tr,t);candidate=alloc(48);wq(candidate,r)
assert call(GAME+0x08149660,manager,tr,candidate)==1
assert len(records[board_targets(t)]['array'])==1
block_native=True
r2,p2=make(99155,(),[],ovr=74);wq(candidate,r2)
assert call(GAME+0x08149660,manager,tr,candidate)==7 and len(records[board_targets(t)]['array'])==1
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test28r14_native_admit_results.json').write_text(json.dumps({'captured_native_AddToBoard':'0x08149660','compiled_bridge':True,'native_target_initialization':True,'duplicate_return':1,'native_block_return':7,'offer_created':False,'live_game_verified':False},indent=2)+'\n')
print('PASS captured native board admission + compiled offseason bridge')
