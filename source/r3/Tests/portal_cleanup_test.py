"""Execute Test11 cleanup against the captured native RemoveFromBoard code.

The compiled callbacks and native protected-owner/accounting branches execute;
SDF storage, allocator, visit/hour services remain controlled fixtures.
"""
from pathlib import Path
import re
fixture_path=Path(__file__).parent/'runtime_fixture_test.py'
fixture=fixture_path.read_text().split('reset_qb_board();configure(0);call(start,frame)')[0]
# A sweep allocates bounded temporary maps. The fixture's bump allocator has
# no reclamation, so reserve enough memory for independent repeated scenarios.
fixture=fixture.replace('0x800000','0x8003d10')
exec(compile(fixture,str(fixture_path),'exec'))

checks=[]
all_cleanup=exports['DriTestCleanupAll'];rival_cleanup=exports['DriTestCleanupRivals']
franchise=record('cleanup franchise');context_record=record('cleanup context',ref={6:franchise})
context=alloc(0x300);wq(context+0x10,rq(context_record+0x10));wi(context+0x18,ri(context_record+0x18));wq(context+0x30,0xabcdef);wq(context+0xc8,manager)
schools=[];boards=[];commits=[]
for tid in range(56):
 b=record('targets '+str(tid),array=[]);c=record('commits '+str(tid),array=[])
 br=record('board '+str(tid),{1:100},ref={0:b})
 schools.append(record('school '+str(tid),{383:tid},ref={224:br,33:c}))
 boards.append(b);commits.append(c)
teams=record('cleanup teams',array=schools);portal=record('cleanup portal',array=[])
records[franchise]['ref'].update({44:season,45:teams,46:portal})

def generalized_board():
 mgr,out,tr,_=args();assert mgr==manager
 school=rq(tr);b=records[records[school]['ref'][224]]['ref'][0]
 incref(b);wq(out,b);return out
def generalized_erase():
 b,index,_,_=args();assert b in boards
 native_events.append(('erase_target',records[b]['array'][index]));records[b]['array'].pop(index);return 0
def generalized_cache():
 mgr,tr,cand,_=args();assert mgr==manager and rq(tr) in schools
 native_events.append(('cache_cleanup',rq(cand)));return 0
game_api(0x0818af10,generalized_board);game_api(0x08052300,generalized_erase);game_api(0x081ae980,generalized_cache)
game_api(0x081861a0,lambda:20)
game_api(0x081913e0,lambda:(native_events.append(('user_hours_adjust',reg('RDX')&0xffffffff)) or 0))

# Connect the ACTUAL compiled protection hook, including its two native tails.
offsets=[];at=0
while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
names=re.findall(r'^\.global (Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
names=[name for name in names if not name.startswith('HookEps')]
hook_index=names.index('HookCleanupProtection')
entry=BASE+offsets[hook_index]
ins=list(cs.disasm(image[offsets[hook_index]:offsets[hook_index]+400],entry))
indirect=[i for i in ins if 'rip' in i.op_str and i.mnemonic in ['jmp','mov']]
# The callback preservation prologue has no RIP memory operands. The native
# replay loads its ref-vtable and jumps to protected/allow continuations.
for i in indirect[:3]:
 mem=next(o.mem for o in i.operands if o.type==capstone.CS_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP)
 slot=i.address+i.size+mem.disp
 if i.mnemonic=='mov':wq(slot,GAME+0x0b4c7e48)
 else:
  # The first jump is the preserved native protected-owner path.
  wq(slot,GAME+(0x081a7dbd if not globals().get('_first_tail') else 0x081a7df9));_first_tail=True
assert len(indirect)>=3
native_protection=raw[0x081a7dae-0x1000:0x081a7dbd-0x1000]
assert native_protection==bytes.fromhex('84c07447488b4def488d1d8b003203')
uc.mem_write(GAME+0x081a7dae,bytes.fromhex('ff2500000000')+struct.pack('<Q',entry)+b'\x90')
uc.ctl_remove_cache(GAME+0x081a7ab0,GAME+0x081a8432)

def reset():
 call(exports['DriTestOwnershipReset']);configure(1);native_events.clear()
 records[portal]['array']=[]
 for s,b,c in zip(schools,boards,commits):
  records[b]['array']=[];records[c]['array']=[];records[s]['ref'].pop(407,None)
 wq(context+0xc8,manager)
def make(rid,stage=5,leader=2,is_portal=True):
 r,t=recruit(rid,0,1,250,stage)
 records[r]['scalar'].update({2:5 if is_portal else 0,14:0})
 records[records[r]['ref'][13]]['array']=[record('leader',{0:leader,1:250})]
 if is_portal:records[portal]['array'].append(r)
 return r,records[r]['ref'][5]
def target(r,tid,status=3,user=False,visit_hours=0):
 t=record('target school '+str(tid),{12:status,2:visit_hours},ref={10:r})
 if visit_hours:records[t]['ref'][11]=record('visit')
 if user:records[schools[tid]]['ref'][407]=record('user')
 records[boards[tid]]['array'].append(t)
 if 2<=status<=4:records[r]['scalar'][14]+=1
 return t
def snapshot():
 return {r:({'scalar':dict(v['scalar']),'ref':dict(v['ref']),'array':None if v['array'] is None else list(v['array'])}) for r,v in records.items()}
def clean(r=None,owner=2):
 result=call(rival_cleanup,r,owner,context) if r else call(all_cleanup,context)
 assert result==1 and call(exports['DriTestQualityFault'])==0
def direct_remove(r,tid):
 tr=alloc(8);wq(tr,schools[tid]);cand=alloc(48);wq(cand,r)
 return call(GAME+0x081a7ab0,manager,tr,cand)

reset();r,p=make(20000);win=target(r,2);cpu=target(r,1,visit_hours=25);user=target(r,3,user=True)
open_r,_=make(20001,stage=2);open_t=target(open_r,1)
hs,_=make(20002,stage=5,is_portal=False);hs_t=target(hs,1)
clean()
assert records[boards[2]]['array']==[win] and records[boards[1]]['array']==[open_t,hs_t] and records[boards[3]]['array']==[]
assert records[r]['scalar'][9]==5 and records[r]['scalar'][14]==1 and records[win]['ref'][10]==r
assert not records[cpu]['ref'][10] and not records[user]['ref'][10]
assert ('visit_cancel',cpu) in native_events and ('refund_hours',25) in native_events
assert ('user_hours_adjust',(-20)&0xffffffff) in native_events
before=snapshot();clean();assert snapshot()==before
checks.append('One sweep removes CPU and user rivals, compacts boards, preserves winner/HS/open targets, decrements only losing offers, cancels CPU visit, refunds CPU hours, adjusts user assigned hours; repeated sweep is idempotent')

reset();r,p=make(20010,stage=6,leader=1);old=target(r,1);win=target(r,2);other=target(r,3,user=True)
records[commits[2]]['array']=[p]
assert direct_remove(r,1)==7 and records[old]['ref'][10]==r
clean()
assert records[boards[1]]['array']==[] and records[boards[3]]['array']==[] and records[boards[2]]['array']==[win]
assert records[commits[2]]['array']==[p] and records[r]['scalar'][9]==6 and records[r]['scalar'][14]==1
assert records[records[r]['ref'][13]]['array'] and records[records[records[r]['ref'][13]]['array'][0]]['scalar'][0]==1
assert direct_remove(r,2)==0  # native rule protects stale rank-one, not actual Signed owner
checks.append('Published Signed owner overrides stale interest leader only in scoped cleanup; ordinary native removal still rejects stale leader; cleanup preserves actual acquisition and saved interest list')

reset();r,p=make(20020,stage=5);win=target(r,2);lose=target(r,1)
clean(r);records[r]['scalar'][9]=6
late=target(r,3);clean(r)
assert records[boards[3]]['array']==[] and records[boards[2]]['array']==[win] and not records[commits[2]]['array']
checks.append('Accepted hard ownership remains usable for same-advance Signed cleanup before publication')

reset();r,p=make(20030,stage=6,leader=1);target(r,1);target(r,2)
orphan,_=make(20031,stage=5,leader=2);target(orphan,1)
soft,_=make(20032,stage=4,leader=2);target(soft,1);target(soft,2)
before=snapshot();clean();assert snapshot()==before
checks.append('Unknown Signed destination, offerless hard leader and soft commitment never imply rival ownership')

reset()
for tid in [0,7,55]:
 for stage in [0,1,2,3,4]:
  r,_=make(20100+tid*10+stage,stage=stage,leader=tid);target(r,tid,user=(tid==7))
 hs,_=make(20800+tid,stage=4,leader=tid,is_portal=False);h=target(hs,tid)
 signed,p=make(20900+tid,stage=6,leader=tid);s=target(signed,tid);records[commits[tid]]['array']=[p]
clean()
for tid in [0,7,55]:
 assert len(records[boards[tid]]['array'])==2
 for t in records[boards[tid]]['array']:
  r=records[t]['ref'][10];assert records[r]['scalar'][2]==0 or records[r]['scalar'][9]==6
checks.append('All three academies including a user academy lose unsecured portal targets at every stage, preserving HS recruiting and previously published Signed acquisitions')

for mode_value in [0,-1]:
 reset();r,_=make(21000+mode_value);target(r,2);target(r,1);configure(mode_value)
 before=snapshot();assert call(all_cleanup,context)==1;assert snapshot()==before and not native_events
checks.append('Diagnostic and disabled modes leave every board, offer and target unchanged')

reset();r,p=make(21010,stage=6);target(r,1);target(r,2);records[commits[2]]['array']=[p];records[commits[3]]['array']=[p]
before=snapshot();assert call(all_cleanup,context)==0 and call(exports['DriTestQualityFault'])==1
assert snapshot()==before and not native_events
reset();r,_=make(21011);target(r,1);target(r,2);wq(context+0xc8,0)
before=snapshot();assert call(all_cleanup,context)==0 and call(exports['DriTestQualityFault'])==1
assert snapshot()==before and not native_events
checks.append('Conflicting published ownership and missing manager fail closed before any board mutation')
call(exports['DriTestOwnershipReset']);configure(1)
assert all(n==1 for n in refs.values()),[(records[r]['name'],n) for r,n in refs.items() if n!=1]
checks.append('All retained SDF references balance after independent successful and rejected sweeps')
result={'checks':checks,'native_remove':'081A3DA0..081A4722','compiled_hook':'081A409E','live_game_verified':False}
(OUTPUT/'portal_cleanup_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
