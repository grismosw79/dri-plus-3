"""R2 frozen forecast -> real callbacks -> captured native board admission.
Native storage/scorer/eligibility are modeled. Formed funding bucket totals are
conserved; productive action witness is modeled explicitly, not live spending.
"""
from pathlib import Path
import json,copy
src=(Path(__file__).parent/'test29_market_portal_test.py').read_text()
exec(compile(src.split('begin();hot,hp=new(2995)')[0],'market fixture definitions','exec'))
begin(10);hot,hp=new(2995,78,3);second,sp=new(2996,78,3);alternatives={}
for tid in range(10,28):
 cold,cp=new(99500+tid,78,2);alternatives[tid]=cold
 scores[(2995,tid)]=450;scores[(2996,tid)]=449;scores[(99500+tid,tid)]=500
before=state([hot,second]+list(alternatives.values()));snapshot()
assert 0<count(hot,3)<=2,[count(hot,k)for k in range(4)]
assert count(hot,2)==count(hot,3)
assert 0<count(second,3)<=2 and count(second,2)==count(second,3)
assert sum(count(r,3)for r in alternatives.values())+count(hot,3)+count(second,3)==18
first={t:(flags(hot,t),flags(r,t))for t,r in alternatives.items()}
assert state([hot,second]+list(alternatives.values()))==before
for t,r in alternatives.items():assert bool(flags(hot,t)&1)+bool(flags(second,t)&1)+bool(flags(r,t)&1)==1
# Read the frozen plan in reverse order: no per-school mutation or new scorer work.
old_calls=scorer_calls
assert first=={t:(flags(hot,t),flags(alternatives[t],t))for t in reversed(alternatives)}
snapshot();assert scorer_calls==old_calls
checks=['Uncovered shared elite receives at most2 bounded primary recovery nominations instead of18 simultaneous promotions',
 'Each school spends one10-hour forecast bundle on exactly one primary; actual offers remain0; reversed reads/same-phase reuse preserve frozen decisions']
tid=next(t for t,r in alternatives.items()if flags(r,t)&2);t=allteams[tid];cold=alternatives[tid]
tr=alloc(8);wq(tr,t)
af=alloc(0x1000);asp=af+128;abp=asp+0x200
wq(af+80,abp);wq(af+16,context);wq(af+24,tr)
wq(abp+0x28,tr);wq(abp+0x58,season);wq(abp+0x180,need);wq(asp+0x58,context);uc.mem_write(asp+0x42,b'\1')
call(hooks[8],af);call(start,af);assert call(exports['DriTestQualityFault'])==0
sf=alloc(0x200);holder=alloc(8);wq(sf,holder);wq(sf+128+0xa8,tr)
def score_callback(r,base,route):
 wq(holder,r);wq(sf+8,base);wq(sf+24,0);wq(sf+128+0x88,GAME+route);call(hooks[9],sf)
 return ri(sf+8)+ri(sf+24)
entries=[]
for r,base in [(hot,450),(cold,500)]:
 value=score_callback(r,base,0x0818c282);assert value>0
 e=bytearray(48);struct.pack_into('<Q',e,0,r);struct.pack_into('<i',e,40,value);entries.append(bytes(e))
vec=alloc(24);mem=alloc(192);main=mem+48;canary=b'\xa5'*48
uc.mem_write(mem,canary+b''.join(entries)+canary);wq(vec,main);wq(vec+8,main+96);wq(vec+16,main+96)
wq(abp-0x68,vec);wq(asp+0x68,2);wi(abp+0x30,50);wq(asp+0x50,0);wq(asp+0x78,0);wq(abp-0x80,0)
call(hooks[15],af);assert call(exports['DriTestQualityFault'])==0
assert {rq(main+i*48)for i in range(2)}=={hot,cold} and rq(main)==cold
for i in range(2):
 current=bytearray(uc.mem_read(main+i*48,48));r=rq(main+i*48)
 original=bytearray(entries[0 if r==hot else 1]);original[40:44]=current[40:44]
 assert current==original and ri(main+i*48+40)>0
assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+96,48))==canary
admitted=[]
for r in [hot,cold]:
 candidate=alloc(48);wq(candidate,r);wq(af+88,candidate);admitted.append(call(admit,af)==0)
assert admitted==[False,True],admitted
checks.append('Positive main-vector entries remain owned exactly once; compiled allocator restores scores and admission veto rejects hot/permits cold despite both native scores being positive')
# Captured native AddToBoard owns target creation, initialization and duplicate checks.
map_game(0x08149660,0xc00);uc.mem_write(GAME+0x08149660,raw[0x08149660-0x1000:0x0814a210-0x1000])
handlers.pop(GAME+0x08149660,None);uc.ctl_remove_cache(GAME+0x08149660,GAME+0x0814a210);map_game(0x0bd0c000,0x2000)
new_targets=[]
def create_target():
 _,out,_,_=args();target=record('native R2 market target');new_targets.append(target);incref(target);wq(out,target);return out
def write_reference():
 target,field,holder,_=args();records[target]['ref'][field-0x100000]=rq(holder);return 0
def append_target():
 array,holder,_,_=args();records[array]['array'].append(rq(holder));return 0
def return_null():wq(reg('RDX'),0);return reg('RDX')
for address,fn in [(0x081bc0a0,lambda:0),(0x03803770,lambda:50),(0x08103c20,lambda:0),(0x0804b440,create_target),(0x0805e4d0,write_reference),(0x082c0be0,lambda:0),(0x0862db70,lambda:0),(0x08045cd0,append_target),(0x0862d2a0,return_null),(0x081ae440,lambda:0)]:game_api(address,fn)
obj=alloc(16);vt=alloc(0x200);wq(obj,vt);wq(vt+0x198,api(return_null));wq(manager+0x100,obj)
assert call(exports['DriTestPortal29Admit'],t,hot,context,450)==0
assert call(exports['DriTestPortal29Admit'],t,cold,context,500)==1
board_t=records[t]['ref'][224];targets=records[board_targets(t)]['array'];assert len(targets)==1
ct=targets[0];assert ct in new_targets and records[ct]['ref'][10]==cold and records[ct]['scalar'][12]==0
assert not offer_status(hot,tid) and not offer_status(cold,tid) and records[board_t]['scalar'][1]==0
# Score-stage selection veto occurs before the native stage sums are formed.
assert score_callback(hot,450,0x081733fa)==0
cold_score=score_callback(cold,500,0x081733fa);assert cold_score>0
fv=alloc(144);ix=alloc(16)
for i,(r,target,value)in enumerate([(cold,ct,cold_score),(hot,0,0)]):
 wq(fv+i*72,r);wi(fv+i*72+40,value);wq(fv+i*72+48,target);wi(fv+i*72+64,0);wq(ix+i*8,1-i)
ff=alloc(0x900);fbp=ff+0x400;wq(ff+72,context);wq(ff+80,fbp);wq(fbp-0x60,tr)
wq(fbp-0x50,fv);wq(fbp-0x48,fv+144);wq(fbp+0x58,ix);wq(fbp+0x60,ix+16)
original=bytes(uc.mem_read(fv,144));budget=copy.deepcopy(records[board_t]['scalar'])
call(hooks[23],ff);assert call(exports['DriTestQualityFault'])==0
assert ri(fv+40)+ri(fv+112)==cold_score and ri(fv+112)==0
assert bytes(uc.mem_read(fv+72,72))==original[72:] and records[board_t]['scalar']==budget
assert records[ct]['scalar'][12]==0 and not offer_status(hot,tid)
checks.append('Captured native creation admits only the authorized target without creating an offer; funding-score gate leaves rejected target zero before stage totals, and compiled funding preserves exact formed score sum and native budget')
# Model native publication/allocated action separately from the above proof.
valid_offer(cold,ct,destination=t);records[ct]['scalar'].update({15:1,8:0});records[board_t]['scalar'][1]=5
before=copy.deepcopy(records);call(exports['DriTestPortal29Observe'],t,context)
assert records==before and call(exports['DriTestPortal29Witness'],ri(cold+0x18))&7==7
assert not offer_status(hot,tid)
checks.append('Explicitly modeled native scholarship/productive assignment yields a genuine primary witness for cold; admission alone had no scholarship/funding success')
# Zero-primary R1 negative control is documented from its unchanged fixture;
# this test asserts the corrected bounded result and live execution boundary.
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestCommitReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test29r2_market_execution_results.json').write_text(json.dumps({'checks':checks,'captured_native_AddToBoard':'0x08149660','modeled':['record storage','native scoring','eligibility services','productive action assignment witness'], 'native_funding_engine_executed':False,'live_game_verified':False,'freeze':first},indent=2)+'\n')
print('\n'.join('PASS '+s for s in checks))
