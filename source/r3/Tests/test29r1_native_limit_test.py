"""Captured per-target funding ABI, stage switch and clamps;
controlled SDF, per-recruit ceiling and entitlement services."""
from pathlib import Path
import json,struct
exec(compile((Path(__file__).parent/'test29_portal_fixture.py').read_text(),'fixture','exec'))
r,t,p=pr(1242,80,pos=10,stage=0,portal=False);valid_offer(r,t)
for lo,hi in [(0x0817e7b0,0x0817e9f9),(0x0819a8f0,0x0819a9b0)]:
 map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000]);handlers.pop(GAME+lo,None)
uc.ctl_remove_cache(GAME+0x0817e7b0,GAME+0x0817e9f9)
uc.ctl_remove_cache(GAME+0x0819a8f0,GAME+0x0819a9b0)
def native_float():
 uc.reg_write(UC_X86_REG_XMM0,0x3f800000);return 0
wq(TABLEVT+0x110,api(native_float))
# Unicorn2.1.4 reads/executes the surrounding AVX conversions/division but
# returns zero for these captured three-register VMULSS instructions. Model
# exactly those two scalar multiplications, retaining all native branches,
# arguments, conversion/truncation, remaining-budget and per-target clamps.
# This is disclosed as a modeled arithmetic dependency, not a full-native run.
def f32(bits):return struct.unpack('<f',struct.pack('<I',bits&0xffffffff))[0]
def multiplication(u,a,size,data):
 if a==GAME+0x0817e915:
  value=f32(u.reg_read(UC_X86_REG_XMM2))*f32(u.reg_read(UC_X86_REG_XMM0));dest=UC_X86_REG_XMM3
 elif a==GAME+0x0817e95c:
  value=f32(u.reg_read(UC_X86_REG_XMM0))*f32(u.reg_read(UC_X86_REG_XMM3));dest=UC_X86_REG_XMM1
 else:return
 u.reg_write(dest,struct.unpack('<I',struct.pack('<f',value))[0]);u.reg_write(UC_X86_REG_RIP,a+4)
uc.hook_add(unicorn.UC_HOOK_CODE,multiplication)
ceiling=50
game_api(0x081784c0,lambda:reg('R9'))
game_api(0x08180320,lambda:ceiling)
vec=alloc(72);vh=alloc(8);th=alloc(8);tr=alloc(8)
wq(vec,r);wq(vec+48,t);wq(vh,vec);wq(th,t);wq(tr,team)
wi(context+0x234,100);wi(context+0x238,200)
wi(context+0x240,100);wi(context+0x244,80)
wi(context+0x24c,0);wi(context+0x250,0)
cases=[]
for stage,score,spent,cap,expected in [(0,1,0,50,1),(0,10,0,50,10),(0,90,0,50,50),(0,10,95,50,5),(1,50,0,50,20),(1,50,79,50,1),(0,90,0,3,3)]:
 records[r]['scalar'][9]=stage;wi(vec+40,score);wi(context+(0x250 if stage else 0x24c),spent);ceiling=cap
 wq(STACK+0xf0000-8+0x28,vh)
 result=call(GAME+0x0817e7b0,context,tr,th,0)
 assert result==expected,(stage,score,spent,cap,result,expected)
 cases.append([stage,score,spent,cap,result])
assert all(v==1 for v in refs.values())
(OUTPUT/'r1_native_limit.json').write_text(json.dumps({'native_helper':'0x0817e7b0','cases':cases,'captured_stage_switch':'0x0819a8f0','modeled_services':['SDF','per-target entitlement','per-recruit cap','two VMULSS operations at0817ac05/0817ac4c'],'live_game_verified':False},indent=2))
print('PASS captured native stage selection, remaining budget, per-recruit cap and ABI; two scalar multiplications modeled')
