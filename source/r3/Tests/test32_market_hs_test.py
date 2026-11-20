"""Compiled Test32 advisory HS forecasts with native scorer/eligibility services modeled.
Supersedes exclusive frozen membership: actual hard16 is tested at native payment
in test32_offer_action_native_test.py; primary8 soft selection remains.
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
assert call(exports['DriTestHs32Count'],hot,2)==16
assert call(exports['DriTestHs32Count'],hot,3)==8
picks={t:call(exports['DriTestHs32Pick'],hot,alternatives[t],allteams[t],context)for t in alternatives}
assert sum(picks.values())==10,picks
selected={t:call(exports['DriTestHs32Selected'],hot,alternatives[t],allteams[t],context)for t in alternatives}
assert all(x==3 for x in selected.values()),selected
assert all(snap8(r)==v for r,v in before.items())
checks.append('18 zero-offer schools forecast8 primaries and16 total recommended routes; 10 choose close alternatives, while native selected backups outside speculative membership remain usable until actual payment enforces16; no offers/points/budgets written')
# Forecast and soft primary choices remain independent of native school processing order.
reverse={t:call(exports['DriTestHs32Pick'],hot,alternatives[t],allteams[t],context)for t in reversed(alternatives)}
assert picks==reverse
records[teams]['array']=list(reversed(allteams))
call(exports['DriTestHs32Reset']);call(exports['DriTestHs32Begin'],context)
assert picks=={t:call(exports['DriTestHs32Pick'],hot,alternatives[t],allteams[t],context)for t in alternatives}
records[teams]['array']=allteams
checks.append('Reversing candidate evaluation and the global native team-array order, then reconstructing the snapshot on reload, yields identical school choices')
# Actual native candidate-vector allocator and admission callbacks consume
# the current feasible plan, preserving every native candidate byte.
tid=next(t for t in picks if picks[t]==1 and not call(exports['DriTestHs30Authorized'],hot,t));t=allteams[tid];cold=alternatives[tid]
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
assert admitted==[True,True],admitted
assert sorted(bytes(uc.mem_read(main+i*48,48))for i in range(2))==sorted(entries)
assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+96,48))==canary
# Current selected roles enter native funding before stage totals; exact actual cap remains at scholarship action.
sf=alloc(0x200);holder=alloc(8);wq(sf,holder);wq(sf+128+0xa8,tr)
def hs_funding_score(r,base):
 wq(holder,r);wq(sf+8,base);wq(sf+24,0);wq(sf+128+0x88,GAME+0x081733fa);call(hooks[9],sf)
 return ri(sf+8)+ri(sf+24) if ri(sf+24)<0x80000000 else ri(sf+8)+ri(sf+24)-0x100000000
assert hs_funding_score(hot,500)>0
assert hs_funding_score(cold,480)==480
# Execute the captured native scorer tail and zero-score eligibility branch.
# This establishes selected advisory paths retain positive native funding scores;
# it does not pretend that modeled SDF fixtures execute actual game spending.
candidate=alloc(48)
map_game(0x08185adf,0x1000);map_game(0x0817350a,0x2000)
uc.mem_write(GAME+0x08185adf,raw[0x08185adf-0x1000:0x08185aeb-0x1000])
uc.mem_write(GAME+0x0817350a,raw[0x0817350a-0x1000:0x08173514-0x1000])
for prospect,base,denied in [(hot,500,False),(cold,480,False)]:
 hs_funding_score(prospect,base)
 uc.reg_write(UC_X86_REG_R12,rq(sf+8));uc.reg_write(UC_X86_REG_R14,rq(sf+24));uc.reg_write(UC_X86_REG_R15,candidate)
 uc.emu_start(GAME+0x08185adf,GAME+0x08185aeb,count=10)
 assert ri(candidate+40)==(0 if denied else base)
 observed=[]
 def branch_stop(u,address,size,data):
  if address in (GAME+0x08173514,GAME+0x0817363b):observed.append(address);u.emu_stop()
 hook=uc.hook_add(unicorn.UC_HOOK_CODE,branch_stop)
 uc.reg_write(UC_X86_REG_RBX,candidate);uc.emu_start(GAME+0x0817350a,STOP,count=10);uc.hook_del(hook)
 assert observed==[GAME+(0x0817363b if denied else 0x08173514)],observed
checks.append('Compiled allocator/admission and captured funding-score branch retain selected backup outside frozen recommendation; both native candidate byte sequences/canaries and budgets preserved')

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
# A mixed-source board can carry stale action flags whose sum exceeds the
# native assigned counter. Neither source may count those flags as actual.
extras=[]
for tid in alternatives:
 extra,ep=rr(99800+tid,score=1000,ovr=77,stage=0,schools=(),offers=(),portal=True)
 target=valid_offer(extra,destination=allteams[tid]);records[target]['scalar'][15]=1;extras.append((tid,target))
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],hot,0)==0 and call(exports['DriTestHs32Count'],hot,1)==0
for tid,target in extras:records[board_targets(allteams[tid])]['array'].remove(target)
checks.append('HS5 plus portal5 with only5 native assigned hours excludes stale flags from actual/firm pressure; affordable planned demand remains separate')
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
 normal=1
 if 'DriTest31ClassAnnual' in exports:
  annual_out=alloc(46*4);assert call(exports['DriTest31ClassAnnual'],allteams[tid],context,annual_out)==1
  normal=ri(annual_out+4)
 primaries=[]
 for j in range(normal):
  rid=98000+tid*10+j;primary,ppp=new(rid,85);scores[(rid,tid)]=600+j;primaries.append(primary)
 scores[(1247,tid)]=500
 for prospect in [r]+primaries:
  target=valid_offer(prospect,destination=allteams[tid]);records[target]['scalar'][15]=1
 records[records[allteams[tid]]['ref'][224]]['scalar'][1]=5*(normal+1)
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],r,0)==18 and call(exports['DriTestHs32Count'],r,1)==0
assert call(exports['DriTestHs32Count'],r,3)==0
records[records[allteams[28]]['ref'][224]]['scalar'][3]=100;allowed8.add(28)
cold,cp=new(99100);scores[(1247,28)]=500;scores[(99100,28)]=480
assert call(exports['DriTestHs32Pick'],r,cold,allteams[28],context)==1
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[28],context)==0
assert call(exports['DriTestHs32Selected'],r,cold,allteams[28],context)==2
checks.append('Grandfathered18 actual funded backups remain conditional; full actual snapshot blocks new19th school in selection, while scope-only admission delegates exact live ceiling to native payment')
# A close alternative can fill only one genuine quantity vacancy. Once it is
# used, a deferred target may fill a second actual seat, but not become a backup.
begin();hot,hp=new(1998);alternatives={}
if 'DriTest31ClassAnnual' in exports:records[ideal_entries[0]]['scalar'][2]=8
for tid in range(10,28):
 records[records[allteams[tid]]['ref'][239]]['array']=[pp('QB returner',65,pos=0,tid=tid)for _ in range(2)]
 cold,cp=new(99700+tid,77);alternatives[tid]=cold
 scores[(1998,tid)]=500;scores[(99700+tid,tid)]=480
before={r:snap8(r)for r in [hot]+list(alternatives.values())}
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],hot,2)==16 and call(exports['DriTestHs32Count'],hot,3)==16
assert sum(call(exports['DriTestHs32Selected'],hot,r,allteams[t],context)==3 for t,r in alternatives.items())==18
assert all(snap8(r)==v for r,v in before.items())
checks.append('Quantity forecast remains16; current legal quantity selection is advisory until actual native offer payment checks the hard ceiling; all close alternatives remain usable')
# W5 Ratchford-shaped witness: two real offered backups, but many qualified
# schools and16 reserved standby identities. An outside school can actually
# select this candidate after the better targets leave its native shortlist.
# Scores/records here intentionally isolate this mechanism, not a live replay.
begin();r,rp=new(1879,73);records[rp]['scalar'][231]=3
for tid in range(10,28):
 scores[(1879,tid)]=450
 for j,ovr in enumerate([83,80]):
  alt,ap=new(97000+tid*10+j,ovr);scores[(97000+tid*10+j,tid)]=600-j*20
for tid in [10,11]:valid_offer(r,destination=allteams[tid])
call(exports['DriTestHs32Begin'],context)
outsiders=[t for t in range(12,28)if not call(exports['DriTestHs30Authorized'],r,t)]
assert outsiders, 'fixture must reproduce occupied speculative standby slots'
tid=outsiders[0];school=allteams[tid]
assert call(exports['DriTestHs30AdmissionBlock'],r,school,context)==0
# A native-shortlist candidate has no prior frozen primary or backup role.
other,op=new(98001,72);scores[(98001,tid)]=420
assert call(exports['DriTestHs32Pick'],r,other,school,context)==0
assert call(exports['DriTestHs32Selected'],r,other,school,context)==3
assert sum(offer_status(r,t)>0 for t in range(138))==2
checks.append('Ratchford-shaped2 actual offers plus16 speculative authorizations: outside school now selects low-covered elite as primary with a legal backup; no forecast-role barrier or offer fabrication')
# Global early elite competition and native incumbent reassessment. The
# same legal four-star choices are available at18 schools. Their18 offered
# paper/early productive targets have no earned progress or close-race lead.
begin();hot,hp=new(1996);records[hp]['scalar'][231]=3;alternatives={}
for tid in range(10,28):
 cold,cp=new(99600+tid,77);records[cp]['scalar'][231]=3;alternatives[tid]=cold
 scores[(1996,tid)]=500;scores[(99600+tid,tid)]=480
 valid_offer(cold,destination=allteams[tid])
 target=valid_offer(hot,destination=allteams[tid]);records[target]['scalar'].update({15:1,8:0,7:0})
 records[records[allteams[tid]]['ref'][224]]['scalar'][1]=5
 # Store a genuine preexisting primary in the production plan cache.
 assert call(exports['DriTestHsPlanRole'],hot,allteams[tid],context,0)==1
before={r:snap8(r)for r in [hot]+list(alternatives.values())}
call(exports['DriTestHs32Begin'],context)
elite_picks={t:int(call(exports['DriTestHsPlanRole'],hot,allteams[t],context,0)!=1)for t in alternatives}
assert 3<=sum(v==0 for v in elite_picks.values())<=8,elite_picks
assert sum(v==1 for v in elite_picks.values())>=10,elite_picks
for t,redirected in elite_picks.items():
 assert call(exports['DriTestHsPlanRole'],hot,allteams[t],context,0)==(2 if redirected else 1)
 assert call(exports['DriTestHsPlanRole'],alternatives[t],allteams[t],context,0)==(1 if redirected else 2)
assert all(snap8(r)==v for r,v in before.items())
checks.append('Full native planner reassesses18 existing early elite primaries without deleting offers or writing influence; retains at least3 strongest contenders and redirects marginal schools to comparable elites')
# Valid high-interest challengers remain even in a crowded race. This is not
# a mandatory3-contender rule: all18 close prospects can still retain primacy.
top=records[records[hot]['ref'][13]]['array'];top.clear()
for tid in range(10,28):top.append(record('close elite interest',{0:tid,1:500}))
for tid in alternatives:
 for target in records[board_targets(allteams[tid])]['array']:
  if records[target]['ref'].get(10)==hot:records[target]['scalar'][8]=500
call(exports['DriTestHsPlanReset']);call(exports['DriTestHs32Begin'],context)
assert all(call(exports['DriTestHsPlanRole'],hot,allteams[t],context,0)==1 for t in alternatives)
checks.append('All18 genuinely close contenders survive the same crowded elite market; opportunity preference does not impose a winner or fixed contender quota')
r=hot
# Stale/missing scope fails closed; explicit user control still passes.
old_week=records[season]['scalar'][4];records[season]['scalar'][4]=old_week+1
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[10],context)==1
records[season]['scalar'][4]=old_week
call(exports['DriTestHs32Reset'])
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[10],context)==1
records[allteams[10]]['ref'][407]=record('user marker')
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[10],context)==0
records[allteams[10]]['ref'].pop(407)
checks.append('Missing/stale frozen execution scope rejects CPU entry; explicit user decision remains native')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test32_market_hs_results.json').write_text(json.dumps({'checks':checks,'opening_choices':picks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
