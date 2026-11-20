"""Compiled R14 frozen crowding with native scorer/eligibility services modeled.
No live season or full native budget execution is claimed.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
checks=[];scores={}
def score14():
 scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and not selection and not portal
 cand=rq(reg('RSP')+0x30);r=rq(cand);tid=records[rq(tr)]['scalar'][383]
 wi(cand+40,scores.get((records[r]['scalar'].get(10000,ri(r+0x18)),tid),0));return 0
game_api(0x08185840,score14)
def begin():
 reset29();call(exports['DriTestHs32Reset']);scores.clear()
 records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 records[ideal_entries[0]]['scalar'][2]=4
 for tid,t in enumerate(allteams):
  records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:100 if 10<=tid<28 else 0})
  records[records[t]['ref'][239]]['array']=[pp('QB returner',65,pos=0,tid=tid)for _ in range(3)]
 allowed8.clear();allowed8.update(range(10,28))
def new(rid,ovr=78):
 r,p=make(rid,(),[],ovr=ovr);return r,p
begin();hot,hp=new(1995);alternatives={}
for tid in range(10,28):
 cold,cp=new(99000+tid,77);alternatives[tid]=cold
 scores[(1995,tid)]=500;scores[(99000+tid,tid)]=480
before={r:snap8(r)for r in [hot]+list(alternatives.values())}
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],hot,0)==0
assert call(exports['DriTestHs32Count'],hot,2)==18
assert call(exports['DriTestHs32Count'],hot,3)==18
picks={t:call(exports['DriTestHs32Pick'],hot,alternatives[t],allteams[t],context)for t in alternatives}
assert sum(picks.values())==15,picks
selected={t:call(exports['DriTestHs32Selected'],hot,alternatives[t],allteams[t],context)for t in alternatives}
assert sum(bool(x&1)for x in selected.values())==3 and all(x&2 for x in selected.values()),selected
assert all(snap8(r)==v for r,v in before.items())
checks.append('Zero-offer opening demand is forecast independently: 18 proposed primary pursuits redirect 15 schools to close, less-crowded alternatives; three strong-fit exceptions remain; no offers/points/budgets written')
# Frozen snapshot is independent of native school processing order.
reverse={t:call(exports['DriTestHs32Pick'],hot,alternatives[t],allteams[t],context)for t in reversed(alternatives)}
assert picks==reverse
call(exports['DriTestHs32Reset']);call(exports['DriTestHs32Begin'],context)
assert picks=={t:call(exports['DriTestHs32Pick'],hot,alternatives[t],allteams[t],context)for t in alternatives}
checks.append('Reversing evaluation order and reconstructing the snapshot on reload yields identical school choices')
# Actual native candidate-vector allocator and admission callbacks consume
# the same frozen decision, preserving every native candidate byte.
tid=next(t for t in picks if picks[t]==1);t=allteams[tid];cold=alternatives[tid]
tr=alloc(8);wq(tr,t)
af=alloc(0x1000);asp=af+128;abp=asp+0x200
wq(af+80,abp);wq(af+16,context);wq(af+24,tr)
wq(abp+0x28,tr);wq(abp+0x58,season);wq(abp+0x180,need);wq(asp+0x58,context);uc.mem_write(asp+0x42,b'\0')
call(hooks[8],af);call(start,af)
entries=[]
for r,value in [(hot,500),(cold,480)]:
 e=bytearray(48);struct.pack_into('<Q',e,0,r);struct.pack_into('<i',e,40,value);entries.append(bytes(e))
vec=alloc(24);mem=alloc(192);main=mem+48;canary=b'\xa5'*48
uc.mem_write(mem,canary+b''.join(entries)+canary);wq(vec,main);wq(vec+8,main+96);wq(vec+16,main+96)
wq(abp-0x68,vec);wq(asp+0x68,2);wi(abp+0x30,50);wq(asp+0x50,0);wq(asp+0x78,0);wq(abp-0x80,0)
call(hooks[15],af);admitted=[]
for r in [hot,cold]:
 candidate=alloc(48);wq(candidate,r);wq(af+88,candidate);admitted.append(call(admit,af)==0)
assert admitted==[False,True],admitted
assert sorted(bytes(uc.mem_read(main+i*48,48))for i in range(2))==sorted(entries)
assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+96,48))==canary
checks.append('Compiled allocator/admission path defers the crowded new target and admits its alternative; native candidate bytes/scores and vector boundaries are exact')

# Current productive offers survive, even in a crowded race and after restart.
for tid in alternatives:
 target=valid_offer(hot,destination=allteams[tid]);records[target]['scalar'].update({15:1,8:0})
 records[records[allteams[tid]]['ref'][224]]['scalar'][1]=5
call(exports['DriTestHsPlanReset']);call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],hot,0)==18
assert call(exports['DriTestHs32Count'],hot,1)==18
for tid in alternatives:
 assert call(exports['DriTestHsPlanRole'],hot,allteams[tid],context,0)==1
 assert call(exports['DriTestHs32Penalty'],hot,allteams[tid],context)==0
checks.append('All 18 existing funded/eligible firm offers retain their primary roles; reload reconstructs actual attention without aggregate offer counters')
# Paper offers alone do not count as attention; native eligibility losses do not.
for tid in alternatives:
 for target in records[board_targets(allteams[tid])]['array']:records[target]['scalar'][15]=0
 records[records[allteams[tid]]['ref'][224]]['scalar'].update({1:0,3:0})
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],hot,0)==0 and call(exports['DriTestHs32Count'],hot,2)==0
checks.append('Unfunded paper offers with no available productive budget contribute zero actual attention and zero forecast demand')
# All-conditional offers cannot suppress primary recovery.
begin();r,rp=new(1247)
for tid in range(10,28):
 primary,ppp=new(98000+tid,85);scores[(98000+tid,tid)]=600;scores[(1247,tid)]=500
 for prospect in [r,primary]:
  target=valid_offer(prospect,destination=allteams[tid]);records[target]['scalar'][15]=1
 records[records[allteams[tid]]['ref'][224]]['scalar'][1]=10
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],r,0)==18 and call(exports['DriTestHs32Count'],r,1)==0
assert call(exports['DriTestHs32Count'],r,3)==0
records[records[allteams[28]]['ref'][224]]['scalar'][3]=100;allowed8.add(28)
cold,cp=new(99100);scores[(1247,28)]=500;scores[(99100,28)]=480
assert call(exports['DriTestHs32Pick'],r,cold,allteams[28],context)==0
checks.append('18 funded backup offers count as competition but not firm coverage; a fresh primary destination still chooses the stranded recruit')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test28r14_crowding_results.json').write_text(json.dumps({'checks':checks,'opening_choices':picks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
