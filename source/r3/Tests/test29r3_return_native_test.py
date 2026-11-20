"""Actual R3 detour and coordinator through captured native finalizer.
SDF allocation, selection and publication dependencies are controlled services.
No live season is claimed. Native append and Signed6 instructions execute.
"""
from pathlib import Path
src=(Path(__file__).parent/'test29r2_portal_retry_native_test.py').read_text()
exec(compile(src.split('cases=[]')[0],__file__,'exec'))
configure(1);records[season]['scalar'].update({4:6,5:6})
records[team]['scalar'][383]=7;records[teams]['array']=fixture_team_array()
# Install actual assembly detour across the native null-winner branch.
hook=call(exports['DriTest29R3DeadlineHook'])
uc.mem_write(GAME+0x081b590d,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook)+b'\x90'*13)
uc.ctl_remove_cache(GAME+0x081b5610,GAME+0x081b5f71)
frame=STACK+0x65000;outer_bp=frame+0x400
wq(frame+2*8,context);wq(frame+10*8,outer_bp)
map_object=outer_bp-0x78;book=outer_bp-0x18
wq(book+8,buckets);wi(book+0x10,1);wq(buckets,0);wq(buckets+8,0)
checks=[]
def add(rid,origin,stage=0):
 r,t,p=pr(rid,70,1,pos=12,stage=stage)
 records[p]['scalar'][230]=origin;records[r]['scalar'][14]=0
 records[t]['scalar'][12]=0
 target_for[r]=t;aux_for[r]=record('native aux',{0:rid})
 return r,t,p
A,at,ap=add(129710,7);B,bt,bp=add(129711,2);C,ct,cp=add(129712,7,6)
# Ordinary NSD does not select a zero-offer recruit or restore early.
call(GAME+0x081b5610,context,map_object,book)
assert not appends and records[A]['scalar'][9]==0
records[season]['scalar'][4]=5
assert call(exports['DriTest29R3Restore'],frame)==0 and not appends
records[season]['scalar'][4]=6
before=records[A]['scalar'].copy();budget=records[board]['scalar'].copy()
assert call(exports['DriTest29R3Restore'],frame)==1
assert records[A]['scalar'][9]==6 and appends==[at]
assert records[A]['scalar'][14]==0 and records[at]['scalar'][12]==0
assert records[board]['scalar']==budget and records[B]['scalar'][9]==0
assert records[C]['scalar'][9]==6 and ct not in appends
assert call(exports['DriTest29R3Restore'],frame)==0 and appends==[at]
assert call(exports['DriTestQualityFault'])==0
checks.append('Zero-offer unsigned Army player restored only after NSD; no scholarship, hours or influence fabricated; nonacademy and Signed6 untouched; repeated pass idempotent')
# Signed elsewhere is authoritative even when origin is an academy.
D,dt,dp=add(129713,7,6);records[records[fixture_dummy_teams[3]]['ref'][33]]['array']=[dp]
assert call(exports['DriTest29R3Restore'],frame)==0 and dt not in appends
# Native destination record absent: execute captured allocation path too.
E,et,ep=add(129714,7)
def target_lookup_missing():
 mgr,th,rh,out=args();r=rq(rh)
 value=0 if r==E else target_for[r];incref(value);wq(out,value);return out
game_api(0x08178da0,target_lookup_missing)
game_api(0x08103c20,lambda:0)
created=[]
def native_create():
 _,out,_,_=args();incref(et);wq(out,et);created.append(et);return out
game_api(0x0804b440,native_create)
global_address=0x081b5af7+0x3b56be5
map_game(global_address,4)
assert call(exports['DriTest29R3Restore'],frame)==1
assert created==[et] and appends==[at,et] and records[E]['scalar'][9]==6
checks.append('Missing native destination record follows captured construction path before append/Signed; signed-away academy player never stolen')
# All academy IDs use the same late-return path, including a user academy.
def multi_lookup():
 mp,out,unused,tidref=args();assert mp==map_object and ri(tidref) in [0,7,55]
 assert rq(reg('RSP')+0x28)==ri(tidref)
 wq(out,map_node);wq(out+8,buckets);uc.mem_write(out+16,b'\0');return out
def multi_remember(which):
 ev,th,rh,_=args();rr=rq(rh);dest=rq(th)
 assert ev==context and records[dest]['scalar'][383]==records[records[rr]['ref'][5]]['scalar'][230]
 which.append(ri(rr+0x18));return 0
game_api(0x081689e0,multi_lookup)
game_api(0x081af0f0,lambda:multi_remember(transitions))
game_api(0x081aeb00,lambda:multi_remember(finalized))
G,gt,gp=add(129716,0);H,ht,hp=add(129717,55)
records[fixture_dummy_teams[0]]['ref'][407]=record('user academy coach')
assert call(exports['DriTest29R3Restore'],frame)==2
assert records[G]['scalar'][9]==records[H]['scalar'][9]==6 and appends[-2:]==[gt,ht]
checks.append('Army, Air Force and Navy origins restore independently through native finalization; a user-controlled academy also receives its own unsigned player')
# Scope cleared: ordinary academy offer still cannot recruit, including user academy.
F,ft,fp=add(129715,7);records[F]['scalar'][14]=1;winners[F]=team
valid_offer(F,ft)
call(GAME+0x081b5610,context,map_object,book)
assert records[F]['scalar'][9]==0 and ft not in appends
for wanted in [0,-1]:
 configure(wanted);assert call(exports['DriTest29R3Restore'],frame)==0
configure(1)
checks.append('Restoration scope cleared after return; ordinary pursuit remains blocked; disabled/diagnostic modes inert')
# Force the existing ordinary rejection code2 at the callback boundary and
# execute real assembly/native cleanup: it must not be mistaken for null -1.
api(lambda:2,hooks[7])
uc.ctl_remove_cache(hooks[7],hooks[7]+16)
call(GAME+0x081b5610,context,map_object,book)
assert records[F]['scalar'][9]==0 and ft not in appends
checks.append('Assembly rejection2 takes ordinary destination-release path; distinct null -1 path does not leak a retained Team')
for t in appends:decref(t)
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test29r3_return_native_results.json').write_text(json.dumps({'passed':True,'checks':checks,'live_game_verified':False},indent=2))
print(checks)
