"""Captured whole native portal finalizer and compiled R2 retry coordinator.
Native selectors, SDF storage, destination append/publication services are
controlled fixtures. This proves sequencing/idempotence, not a live season.
"""
from pathlib import Path
import json,re
exec(compile((Path(__file__).parent/'portal_runtime_test.py').read_text().split("keep=pp('StClair'")[0],str(Path(__file__).parent/'portal_runtime_test.py'),'exec'))
map_game(0x081b5610,0x1000)
uc.mem_write(GAME+0x081b5610,raw[0x081b5610-0x1000:0x081b5f71-0x1000])
# Full captured function uses existing evaluator, native SDF adapters and explicit map services.
wq(context+0xa8,franchise)
records[tplist]['array']=[]
selections=[];appends=[];finalized=[];transitions=[];books=[];assertions=[]
winners={};target_for={};aux_for={}
map_object=alloc(0x70);map_node=alloc(0x50);vector=map_node+8
vdata=alloc(100*8);wq(vector,vdata);wq(vector+8,vdata);wq(vector+16,vdata+800)
book=alloc(0x60);buckets=alloc(2*8);wq(book+8,buckets);wi(book+0x10,1);wq(buckets,0);wq(buckets+8,0)

def select():
 ev,out,rh,_=args();r=rq(rh);assert ev==context
 selections.append((ri(r+0x18),records[r]['scalar'][9]))
 dest=winners.get(r,0)
 if callable(dest):dest=dest()
 incref(dest);wq(out,dest);return out

def target_lookup():
 mgr,th,rh,out=args();assert mgr==manager
 r=rq(rh);t=target_for[r];incref(t);wq(out,t);return out

def lookup():
 mp,out,unused,tidref=args();assert mp==map_object
 tid=ri(tidref);assert tid==records[team]['scalar'][383]
 assert rq(reg('RSP')+0x28)==tid
 wq(out,map_node);wq(out+8,buckets);uc.mem_write(out+16,b'\0');return out

def append():
 v,holder,_,_=args();assert v==vector
 t=rq(holder);incref(t);end=rq(v+8);wq(end,t);wq(v+8,end+8);appends.append(t);return 0

def remember(which):
 ev,th,rh,_=args();assert ev==context and rq(th)==team
 which.append(ri(rq(rh)+0x18));return 0

def get_aux():
 wrapper,out,_,_=args();r=rq(wrapper+8);a=aux_for[r];incref(a);wq(out,a);return out

def insert_book():
 mp,out,node,key=args();assert mp==book
 assert ri(node)==key;wq(buckets,node);books.append(key);return out

def assertion():assertions.append(args());return 0
for address,fn in [(0x081acd10,select),(0x081ac8b0,select),(0x08178da0,target_lookup),(0x081689e0,lookup),(0x03811bf0,append),(0x081af0f0,lambda:remember(transitions)),(0x081aeb00,lambda:remember(finalized)),(0x0862d570,get_aux),(0x08057470,lambda:ri(reg('RCX')+0x18)),(0x019ea660,lambda:alloc(reg('RDX'))),(0x0212a5f0,insert_book),(0x037f4530,assertion),(0x019ea6a0,lambda:0)]:game_api(address,fn)

cases=[]
for i,stage in enumerate([6,5,4,3,0]):
 r,t,p=pr(98000+i,80,pos=0,stage=stage);valid_offer(r,t)
 target_for[r]=t;aux_for[r]=record('native aux',{0:42});winners[r]=team
 cases.append((r,t,p,stage))
# Stage6 must never select or append. Other initial stages are native-only signings.
before={r:dict(records[r]['scalar']) for r,t,p,s in cases}
call(GAME+0x081b5610,context,map_object,book)
assert len(appends)==4 and len(finalized)==4 and len(transitions)==2
assert [records[r]['scalar'][9] for r,t,p,s in cases]==[6]*5
first=(list(selections),list(appends),list(finalized),list(transitions),list(books))
call(GAME+0x081b5610,context,map_object,book)
assert first==(selections,appends,finalized,transitions,books)
assert len(set(appends))==4 and not assertions
# Null winner is inert; if native eligibility/acceptance later permits, same body signs.
r,t,p=pr(98100,80,pos=0,stage=0);valid_offer(r,t);target_for[r]=t;aux_for[r]=record('native aux',{0:43})
before=dict(records[r]['scalar']);n=len(appends)
call(GAME+0x081b5610,context,map_object,book)
assert records[r]['scalar']==before and len(appends)==n
winners[r]=team
call(GAME+0x081b5610,context,map_object,book)
assert records[r]['scalar'][9]==6 and len(appends)==n+1
call(GAME+0x081b5610,context,map_object,book)
assert len(appends)==n+1 and not assertions
# Native destination vectors own each appended target once.
for t in appends:decref(t)
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]

checks=['Captured081b1900 exact three-argument caller ABI; stage6 bypasses selectors and side effects',
 'Stages0/3/4/5 use actual native append→Signed6; repeat causes no second selection/append/finalization/bookkeeping',
 'Null native winner is inert; later valid winner signs once; maps and queued vectors survive every call']
# Original guarded caller proves the exact stack-map identities for retry.
assert raw[0x081a2f78-0x1000:0x081a2f88-0x1000]==bytes.fromhex('4c8d45e8498bcd488d5588e888260100')
ins=list(cs.disasm(raw[0x081b5610-0x1000:0x081b5f71-0x1000],0x081b5610))
assert [(i.address,i.op_str) for i in ins if i.address==0x081b582e]==[(0x081b582e,'eax, 6')]
# R2 adds byte guards, not an unverified same-address native dependency.
guards=(ROOT/'signatures.h').read_text();parsed={}
for m in re.finditer(r'\{(0x[0-9a-fA-F]+),\s*(\d+),\s*\{([^}]+)\}',guards):
 parsed[int(m[1],16)]=bytes(int(b,16)for b in re.findall(r'0x([0-9a-fA-F]+)',m[3]))[:int(m[2])]
for address in [0x081b5610,0x081b5665,0x081b5828,0x081b5eeb,0x081a2f78]:
 assert address in parsed,hex(address)
 assert parsed[address]==raw[address-0x1000:address-0x1000+len(parsed[address])]
checks.append('New native entry/pool/stage/loop/caller guards exactly match supplied executable capture')
# Keep this test isolated from previously checked signings and their vectors.
configure(1)
appends.clear();selections.clear();finalized.clear();transitions.clear();books.clear();winners.clear()
records[tplist]['array']=[];records[arr]['array']=[]
wq(vector+8,rq(vector));wq(buckets,0)
frame=STACK+0x65000;outer_bp=frame+0x400
wq(frame+2*8,context);wq(frame+10*8,outer_bp)
map_object=outer_bp-0x78;book=outer_bp-0x18
wq(book+8,buckets);wi(book+0x10,1);wq(buckets,0);wq(buckets+8,0)
entries=0
def observe_entry(u,address,size,data):
 global entries
 if address==GAME+0x081b5610:entries+=1
entry_hook=uc.hook_add(unicorn.UC_HOOK_CODE,observe_entry)
def add_case(rid,stage=0):
 r,t,p=pr(rid,80,pos=0,stage=stage);valid_offer(r,t)
 target_for[r]=t;aux_for[r]=record('native aux '+str(rid),{0:rid});return r,t,p
A,at,ap=add_case(98200);B,bt,bp=add_case(98201);C,ct,cp=add_case(98202)
winners[A]=lambda:team if records[B]['scalar'][9]==6 else 0
winners[B]=lambda:team if records[C]['scalar'][9]==6 else 0
winners[C]=team
# Normal one-pass native order cannot return to A or B after C resolves.
call(GAME+0x081b5610,context,map_object,book)
assert [records[r]['scalar'][9]for r in [A,B,C]]==[0,0,6]
prior=entries
assert call(exports['DriTest29R2PortalRetry'],frame)==2
assert entries-prior==2 and [records[r]['scalar'][9]for r in [A,B,C]]==[6,6,6]
assert appends==[ct,bt,at] and len(set(appends))==3
prior=entries;assert call(exports['DriTest29R2PortalRetry'],frame)==0 and entries==prior
checks.append('Compiled retry closes reversed three-recruit dependency in two productive native passes; signed entries never append twice; no pending means no native call')
# A displayed offer without a valid native winner remains unsigned, preserving
# source of truth rather than treating attention as a manufactured signing.
D,dt,dp=add_case(98203)
before=dict(records[D]['scalar']);prior=entries
assert call(exports['DriTest29R2PortalRetry'],frame)==0 and entries==prior+1
assert records[D]['scalar']==before and appends==[ct,bt,at]
checks.append('No native progress stops after one pass; offer, stage and previously queued targets preserved')
for mode_value in [0,-1]:
 configure(mode_value);prior=entries
 assert call(exports['DriTest29R2PortalRetry'],frame)==0 and entries==prior
configure(1)
records[D]['scalar'][14]=0;prior=entries
assert call(exports['DriTest29R2PortalRetry'],frame)==0 and entries==prior
records[D]['scalar'][9]=5;winners[D]=team
assert call(exports['DriTest29R2PortalRetry'],frame)==0 and entries==prior+1
# Native still needs an actual offer to select a hard target; helper does not
# synthesize it. Once real offer evidence returns, native finalization succeeds.
records[D]['scalar'][14]=1
assert call(exports['DriTest29R2PortalRetry'],frame)==1 and records[D]['scalar'][9]==6
checks.append('Disabled/diagnostic/zero-offer soft cases do not run; hard commitments can retry native finalization but still need native offer selection')
# Exercise the actual production pre-publication callback, including its empty
# HS ownership validation/HS pass and one class-stat refresh after portal work.
records[hslist]['array']=[]
for tr in records[teams]['array']:records[records[tr]['ref'][33]]['array']=[]
G,gt,gp=add_case(98210);H,ht,hp=add_case(98211)
winners[G]=lambda:team if records[H]['scalar'][9]==6 else 0
winners[H]=team
call(GAME+0x081b5610,context,map_object,book)
assert records[G]['scalar'][9]==0 and records[H]['scalar'][9]==6
refresh=[]
def refresh_stats():
 assert args()[0]==context
 refresh.append([records[r]['scalar'][9]for r in [G,H]])
 return 0
game_api(0x081bb580,refresh_stats)
uc.mem_write(context+0x1a0,b'\1');prior=entries
call(hooks[24],frame)
assert call(exports['DriTestQualityFault'])==0
assert entries==prior+1 and refresh==[[6,6]] and appends[-2:]==[ht,gt]
prior=entries;call(hooks[24],frame)
assert entries==prior and refresh==[[6,6]]
uc.mem_write(context+0x1a0,b'\0');call(hooks[24],frame)
assert entries==prior and refresh==[[6,6]]
checks.append('Actual DriBeforeNsdPublish callback runs portal retry after an empty valid HS pass with original maps; refreshes class statistics once after signing; repeated/non-NSD callbacks are inert')
# Fault paths are checked without allowing a malformed pool to reach native.
for kind in ['missing_pool','invalid_stage','wrong_class']:
 configure(1);E,et,ep=add_case(98300+len(records))
 original_pool=records[franchise]['ref'][46];original_class=records[E]['scalar'][2]
 if kind=='missing_pool':records[franchise]['ref'].pop(46)
 elif kind=='invalid_stage':records[E]['scalar'][9]=7
 else:records[E]['scalar'][2]=0
 prior=entries
 assert call(exports['DriTest29R2PortalRetry'],frame)==0xffffffff and entries==prior,kind
 assert call(exports['DriTestQualityFault'])==1,kind
 records[franchise]['ref'][46]=original_pool;records[E]['scalar'][2]=original_class;records[E]['scalar'][9]=6
configure(1)
F,ft,fp=add_case(98400)
# Simulate a forbidden population mutation at a native service boundary. The
# compiled coordinator must fault and decline a subsequent retry.
def mutated_native():records[tplist]['array'].append(F);return 0
api(mutated_native,GAME+0x081b5610);prior=entries
assert call(exports['DriTest29R2PortalRetry'],frame)==0xffffffff and entries==prior+1
assert call(exports['DriTestQualityFault'])==1
prior=entries;assert call(exports['DriTest29R2PortalRetry'],frame)==0 and entries==prior
records[tplist]['array'].pop();handlers.pop(GAME+0x081b5610)
checks.append('Missing/malformed portal pool fails before first native call; population mutation faults after one pass and prevents further calls')
for t in appends:decref(t)
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
uc.hook_del(entry_hook)
checks.append('Captured native pass and compiled coordinator retain balanced record references')
result={'checks':checks,'captured_native':'081b1900–081b2261','native_abi':['eval','destination_map','newly_signed_map'],'live_game_verified':False,
 'modeled_services':['native selectors and target lookup','SDF storage','map/vector allocation and insertion','publication side effects'],
 'not_proven':['full native acceptance/eligibility services','live game behavior and performance']}
(OUTPUT/'test29r2_portal_retry_native_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
