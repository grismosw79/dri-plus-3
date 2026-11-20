"""Test31 native NSD collection/acceptance/publication consistency.
Reproduces Test30's retained soft Target double-publication shape and executes
captured collector, native Signed writer and publication consumer. SDF storage,
Windows, allocator and append event are fixtures; this is not a live game.
"""
from pathlib import Path
import os,subprocess,shutil,json,re,hashlib
negative=os.environ.get('DRI31_SIGN_NEGATIVE')=='1'
fixture=Path(__file__).parent/'recovery_runtime_test.py'
exec(compile(fixture.read_text().split('\nreset_recovery()\n# Test10-shaped')[0],str(fixture),'exec'))
wq(TABLEVT+0xf8,api(native_scalar))
# Execute complete native collector and consumer; only SDF/allocator/event
# services are supplied by the fixture. Captured caller ordering is asserted.
def load_code(start,end):
 map_game(start,end-start);uc.mem_write(GAME+start,raw[start-0x1000:end-0x1000])
load_code(0x08187730,0x08187c5d)
load_code(0x08164d40,0x08165330)
load_code(0x037f7ec0,0x037f7f40)
game_api(0x019ea6a0,lambda:0)
caller=list(cs.disasm(raw[0x081a2ddd-0x1000:0x081a2f8f-0x1000],0x081a2ddd))
call_sites=[(i.address,i.operands[0].imm) for i in caller if i.mnemonic=='call' and i.operands[0].type==capstone.CS_OP_IMM]
assert [(a,b) for a,b in call_sites if b in (0x08187730,0x08162c60,0x081b40f0)]==[(0x081a2ec1,0x08187730),(0x081a2ee3,0x08187730),(0x081a2ef9,0x08162c60),(0x081a2f31,0x081b40f0)]
assert 0x081a2ddd<call_sites[0][0]<0x081a2f90
names=re.findall(r'^\.global\s+(Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
wrappers={'HookEpsGrade','HookEpsGeneric','HookEpsRequired','HookEpsUpdate','HookStaff28Update','HookPt28Grade','HookProximity28Grade','HookPPGroupGrade'}
names=[n for n in names if n not in wrappers]
pre=hooks[names.index('HookNsdCollect')]
late=hooks[names.index('HookNsdResolve')]
fr=STACK+0x65000;bp=fr+0x400
wq(fr+10*8,bp);wq(fr+2*8,context)
refresh=[];game_api(0x081bb580,lambda:refresh.append(args()[0]) or 0)
game_api(0x0816b130,lambda:1)
def callback(address,active=True):
 global next_heap
 uc.mem_write(context+0x1a0,bytes([active]));saved=next_heap
 result=call(address,fr);next_heap=saved;return result
# Native collector returns Target refs, not Recruit refs. Each vector owns
# retained references. Consumer sort/de-duplicate semantics remain native.
def vector(items=()):
 v=alloc(32);a=alloc(256*8);wq(v,a);wq(v+8,a);wq(v+16,a+256*8);wq(v+24,0)
 for x in items:incref(x);wq(rq(v+8),x);wq(v+8,rq(v+8)+8)
 return v
def vitems(v):return [rq(x) for x in range(rq(v),rq(v+8),8)]
def push_target():
 v,holder,_,_=args();x=rq(holder);end=rq(v+8);assert end<rq(v+16)
 incref(x);wq(end,x);wq(v+8,end+8);return 0
def destroy(v):
 for x in vitems(v):decref(x)
 wq(v+8,rq(v))
game_api(0x03811bf0,push_target)
game_api(0x03803770,lambda:192)
game_api(0x08057470,lambda:ri(reg('RCX')+0x18))
def sort_targets():
 start,end,_,_=args();items=sorted((rq(x) for x in range(start,end,8)),key=lambda r:ri(r+0x18))
 for x,r in zip(range(start,end,8),items):wq(x,r)
 return 0
game_api(0x0813f1c0,sort_targets);game_api(0x08138640,sort_targets)
def erase_tail():
 v,start,end,_=args();assert end==rq(v+8)
 for x in range(start,end,8):decref(rq(x));wq(x,0)
 wq(v+8,start);return 0
game_api(0x081bc3e0,erase_tail)
emitted=[]
def publish_event():
 _,team_holder,player_holder,flag=args();tid=records[rq(team_holder)]['scalar'][383];p=rq(player_holder)
 assert flag==0
 records[records[allteams[tid]]['ref'][33]]['array'].append(p)
 emitted.append((tid,p));return 0
game_api(0x0859b890,publish_event)
def collect(tid,stage,v):
 # Native six-argument ABI: exclude-special false, emit Target wrapper true.
 holder=alloc(8);wq(holder,allteams[tid]);sp=STACK+0xf0000-8
 uc.mem_write(sp+40,b'\x00');uc.mem_write(sp+48,b'\x01')
 return call(GAME+0x08187730,context,holder,stage,v)
def publish(tid,v):
 holder=alloc(8);wq(holder,allteams[tid]);call(GAME+0x08164d40,context,holder,v)
 # Native consumer tail-calls the real retained-vector destructor.
 wq(v+8,rq(v))
def owned():return {i:list(records[records[t]['ref'][33]]['array']) for i,t in enumerate(allteams)}

bulk_checks=[]
bulk_cb=hooks[17]
offsets=[];at=0
while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
hook_address=BASE+offsets[17]
ins=list(cs.disasm(image[offsets[17]:offsets[17]+400],hook_address))
native_call=next(x for x in ins if x.mnemonic=='call' and 'rip' in x.op_str)
native_jump=next(x for x in ins if x.mnemonic=='jmp' and 'rip' in x.op_str)
wq(native_call.address+native_call.size+native_call.operands[0].mem.disp,GAME+0x08055d00)
wq(native_jump.address+native_jump.size+native_jump.operands[0].mem.disp,GAME+0x081630d3)
START,JOIN,RESUME,END,SKIP=0x08162eda,0x081630c1,0x081630d3,0x081630f8,0x08163108
map_game(START,SKIP-START+1)
native_bytes=raw[START-0x1000:END-0x1000]
uc.mem_write(GAME+START,native_bytes)
game_api(END,lambda:0);game_api(SKIP,lambda:0)
book=alloc(0x60);buckets=alloc(0x18);wq(book+8,buckets);wi(book+0x10,1)
game_api(0x08057470,lambda:ri(reg('RCX')+0x18))
game_api(0x019ea660,lambda:alloc(reg('RDX')))
game_api(0x0212a5f0,lambda:0)
def unexpected_duplicate():raise AssertionError('Unexpected duplicate in fresh bookkeeping fixture')
game_api(0x037f4530,unexpected_duplicate)
bulk_bp=STACK+0x2c000

# Guard the provenance of the path: collect both stages, compare ownership,
# branch hard commits directly to the shared writer, write Signed=6.
assert raw[0x08162d56-0x1000:0x08162d5c-0x1000]==bytes.fromhex('41b804000000')
assert raw[0x08162d73-0x1000:0x08162d79-0x1000]==bytes.fromhex('41b805000000')
for rva in [0x08162d65,0x08162d84]:
 i=next(cs.disasm(raw[rva-0x1000:rva-0x1000+5],rva))
 assert i.mnemonic=='call' and i.operands[0].imm==0x08187730
assert raw[0x08162f98-0x1000:0x08162fa1-0x1000]==bytes.fromhex('83f8050f8420010000')
assert raw[JOIN-0x1000:RESUME-0x1000]==bytes.fromhex('488b4defba09000000488b7de7e82d2cefff')
bulk_checks.append('Captured collector calls select stage 4 and stage 5; both converge at the exact Signed writer')

def bulk_signed(rec,observe=True,leader_tid=None):
 stage=records[rec]['scalar'][9];assert stage in [4,5]
 tid=records[team]['scalar'][383]
 leader=record('bulk leading school',{0:tid if leader_tid is None else leader_tid,1:100})
 wq(bulk_bp-1,0xabcdef);wq(bulk_bp-0x11,0xabcdef)
 wq(bulk_bp-0x19,rec);wq(bulk_bp-0x49,rec);wq(bulk_bp+0x77,book)
 uc.mem_write(GAME+JOIN,(bytes.fromhex('ff2500000000')+struct.pack('<Q',hook_address)+b'\x90'*4) if observe else native_bytes[JOIN-START:RESUME-START])
 uc.ctl_remove_cache(GAME+START,GAME+SKIP+1)
 rsp=STACK+0x32000;wq(rsp,STOP)
 for n,v in [('RSP',rsp),('RBP',bulk_bp),('R12',team),('R13',context),('R14',leader),('R15',book),('RCX',leader)]:
  uc.reg_write(globals()['UC_X86_REG_'+n],v)
 uc.emu_start(GAME+START,STOP+1,count=2000000)
 assert reg('RIP')==STOP
 assert records[rec]['scalar'][9]==(6 if leader_tid is None or leader_tid==tid else stage)


# Resolve the exact existing bulk guard callback by exported assembly order.
guard=hooks[names.index('HookBulkGuard')]
def guard_call(r,tid):
 frame=alloc(128);base=alloc(256)+128
 wq(frame+10*8,base);wq(frame+2*8,context);wq(frame+3*8,allteams[tid]);wq(base-0x19,r)
 return call(guard,frame)
def accept(r,tid):
 global team
 team=allteams[tid];bulk_signed(r)
def native_map(queues):
 m=alloc(64);b=alloc(8*139);wi(m+0x10,139);wq(m+8,b);out={}
 for tid,v in queues.items():
  node=alloc(48);wi(node,tid);uc.mem_write(node+8,bytes(uc.mem_read(v,32)));wq(b+tid*8,node);out[tid]=node+8
 return m,out
def target_for(r,tid):
 return next(t for t in records[board_targets(allteams[tid])]['array'] if records[t]['ref'][10]==r)
def append_target(v,r,tid):
 t=target_for(r,tid);incref(t);wq(rq(v+8),t);wq(v+8,rq(v+8)+8)
def setup(rid=1153):
 reset_recovery();records[season]['scalar'].update({4:6,5:6});emitted.clear();refresh.clear()
 r,p=rr(rid,ovr=69,stage=4,portal=False,schools=((15,1000),(29,900)),offers=(15,29))
 records[p]['scalar'][3]=10
 records[allteams[15]]['ref'][407]=record('user path fixture')
 return r,p
checks=[]
r,p=setup();q15=vector();collect(15,4,q15);assert len(vitems(q15))==1
blocked=guard_call(r,15)
if negative:
 assert blocked==11,blocked
 # Test30 falls through to Georgia, but BYU's earlier Target is retained.
 accept(r,29);q29=vector();append_target(q29,r,29)
 publish(15,q15);publish(29,q29)
 assert emitted==[(15,p),(29,p)],emitted
 checks.append('Test30 negative: original soft guard11 leaves captured BYU Target queued; native fallback Signed write to Georgia then captured consumers publish both owners')
else:
 assert blocked==0,blocked
 accept(r,15);assert guard_call(r,29)==13 and guard_call(r,15)==13
 m,qs=native_map({15:q15})
 uc.mem_write(bp-0x78,bytes(uc.mem_read(m,64)));callback(late)
 assert call(exports['DriTestQualityFault'])==0
 publish(15,qs[15]);assert emitted==[(15,p)]
 call(hooks[names.index('HookPostNsd')],fr)
 assert call(exports['DriTestQualityFault'])==0 and records[r]['scalar'][9]==6
 assert not offer_status(r,29) and offer_status(r,15)
 assert records[r]['scalar'][14]==1
 assert call(exports['DriTest31Finalize'],context)==0
 checks.append('Valid native soft choice: actual bulk Signed writer, compiled pre-publication callback, captured consumer, post-NSD rival cleanup and offer-counter1 all agree on BYU')
 # Rejected original queue and accepted later destination: general failure shape,
 # independent of the former unconditional soft-guard veto.
 r,p=setup(1154);q15=vector();collect(15,4,q15)
 accept(r,29);q29=vector();append_target(q29,r,29);m,qs=native_map({15:q15,29:q29})
 before=values(r);assert call(exports['DriTest31NsdQueue'],m,context)==1
 assert vitems(qs[15])==[] and len(vitems(qs[29]))==1 and values(r)==before
 publish(15,qs[15]);publish(29,qs[29]);assert emitted==[(29,p)]
 assert call(exports['DriTest31Finalize'],context)==0
 checks.append('Unaccepted original collection is removed before publication; real accepted Georgia signing is retained, with no arbitrary post-publication owner deletion')
 # Different native Target wrappers for one accepted Player still publish once.
 r,p=setup(1157);q15=vector();collect(15,4,q15);accept(r,15)
 duplicate=record('distinct queued Target for the same accepted recruit',ref={10:r})
 incref(duplicate);wq(rq(q15+8),duplicate);wq(q15+8,rq(q15+8)+8)
 m,qs=native_map({15:q15});assert call(exports['DriTest31NsdQueue'],m,context)==1
 assert len(vitems(qs[15]))==1
 publish(15,qs[15]);assert emitted==[(15,p)]
 checks.append('Two distinct queued Target wrappers for the same accepted Player publish once; retained references remain balanced')
 # An observer alone is not proof of acceptance: only actual Signed activates it.
 r,p=setup(1155);q15=vector();collect(15,4,q15)
 frame=alloc(128);base=alloc(256)+128;wq(frame+80,base);wq(frame+16,context);wq(frame+24,allteams[15]);wq(base-0x19,r)
 call(bulk_cb,frame);m,qs=native_map({15:q15});assert records[r]['scalar'][9]==4
 assert call(exports['DriTest31NsdQueue'],m,context)==1 and not vitems(qs[15])
 publish(15,qs[15]);assert not emitted and records[r]['scalar'][9]==4
 checks.append('Pending observer with no native stage completion cannot authorize queued publication')
 # Same-advance Madden-shaped publication gets final stage normalization.
 r,p=setup(1687);records[records[allteams[15]]['ref'][33]]['array'].append(p)
 before=values(r);call(hooks[names.index('HookPostNsd')],fr)
 assert call(exports['DriTestQualityFault'])==0
 assert records[r]['scalar'][9]==6 and values(r)==before
 assert not offer_status(r,29) and offer_status(r,15) and records[r]['scalar'][14]==1
 assert call(exports['DriTest31Finalize'],context)==0
 checks.append('Newly published soft HS identity is normalized to Signed exactly once after publication, retaining destination and earned interest')
 # Real cross-owner corruption stops before any queue mutation.
 r,p=setup(1156);q15=vector();collect(15,4,q15);accept(r,15);m,qs=native_map({15:q15})
 for tid in (15,29):records[records[allteams[tid]]['ref'][33]]['array'].append(p)
 before=vitems(qs[15]);assert call(exports['DriTest31NsdQueue'],m,context)==0
 assert call(exports['DriTestQualityFault'])==1 and vitems(qs[15])==before
 destroy(qs[15]);configure(1)
 checks.append('Actual duplicate published ownership remains fatal and is never repaired by deleting an owner')
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[k]['name'],v) for k,v in refs.items() if v!=1]
report={'passed':True,'negative_control':negative,'checks':checks,'native_executed':['08187730 collector','08162eda native bulk owner comparison and Signed writer','08164d40 publication consumer'],'fixture_services':['SDF storage','allocator','native append event','bulk map allocator'],'live_game_verified':False}
report.update(fixture_sha256=hashlib.sha256((OUTPUT/'retention_fixture.dll').read_bytes()).hexdigest(),capture_code_sha256=hashlib.sha256(raw).hexdigest())
(OUTPUT/'test31_signing_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
