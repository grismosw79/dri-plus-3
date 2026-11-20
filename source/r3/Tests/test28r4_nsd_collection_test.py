"""Captured NSD stage collector and publication consumer, compiled callbacks.
The native event dispatcher is an explicit append fixture (not the full game).
R3 control reproduces all 15 live duplicate publications from pre-collected
soft HS targets. R4 finalizes those identities before the native collector.
"""
from pathlib import Path
import os,subprocess,shutil,json,re,hashlib
negative=os.environ.get('DRI_R4_NEGATIVE')=='1'
fixture=Path(__file__).parent/'recovery_runtime_test.py'
exec(compile(fixture.read_text().split('\nreset_recovery()\n# Test10-shaped')[0],str(fixture),'exec'))
wq(TABLEVT+0xf8,api(native_scalar))
data=json.loads((ROOT.parent/'Audit/Test28R4/live-duplicate-cases.json').read_text())
# Execute complete native collector and consumer; only SDF/allocator/event
# services are supplied by the fixture. Captured caller ordering is asserted.
def load_code(start,end):
 map_game(start,end-start);uc.mem_write(GAME+start,raw[start-0x1000:end-0x1000])
load_code(0x08187730,0x08187c5d)
load_code(0x08164d40,0x08165330)
load_code(0x037f7ec0,0x037f5723)
game_api(0x019ea6a0,lambda:0)
caller=list(cs.disasm(raw[0x081a2ddd-0x1000:0x081a2f8f-0x1000],0x081a2ddd))
call_sites=[(i.address,i.operands[0].imm) for i in caller if i.mnemonic=='call' and i.operands[0].type==capstone.CS_OP_IMM]
assert [(a,b) for a,b in call_sites if b in (0x08187730,0x08162c60,0x081b40f0)]==[(0x081a2ec1,0x08187730),(0x081a2ee3,0x08187730),(0x081a2ef9,0x08162c60),(0x081a2f31,0x081b40f0)]
assert 0x081a2ddd<call_sites[0][0]<0x081a2f90
names=re.findall(r'^\.global\s+(Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
wrappers={'HookEpsGrade','HookEpsGeneric','HookEpsRequired','HookEpsUpdate','HookStaff28Update','HookPt28Grade','HookProximity28Grade','HookPPGroupGrade'}
names=[n for n in names if n not in wrappers]
pre=hooks[names.index('HookNsdCollect')] if not negative else None
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
def initialize(stage=4):
 reset_recovery();records[season]['scalar'].update({4:6,5:6});emitted.clear();refresh.clear()
 held=[]
 for c in data['cases']:
  r,p=rr(c['rid'],ovr=c['ovr'],stage=stage,portal=False,schools=((c['destination'],1000),(c['owner'],900)) if c['owner']!=c['destination'] else ((c['owner'],1000),),offers=(c['destination'],))
  records[records[allteams[c['owner']]]['ref'][33]]['array'].append(p)
  if c['destination']==67:records[allteams[67]]['ref'][407]=record('user Ohio')
  held.append((r,p,c))
 return held
checks=[]
held=initialize();before=owned();saved_values=[values(r) for r,p,c in held]
if not negative:callback(pre)
queues={tid:vector() for tid in sorted({c['destination'] for r,p,c in held})}
for tid,v in queues.items():collect(tid,4,v);collect(tid,5,v)
queued=sum(len(vitems(v)) for v in queues.values())
assert queued==(15 if negative else 0),queued
# R3's late finalizer sets Signed but cannot revoke retained Target references.
callback(late)
assert all(records[r]['scalar'][9]==6 for r,p,c in held)
assert [values(r) for r,p,c in held]==saved_values and owned()==before
for tid,v in queues.items():publish(tid,v);destroy(v)
assert call(exports['DriTestQualityFault'])==0
if negative:
 assert len(emitted)==15
 assert sum(len(v) for v in owned().values())-sum(len(v) for v in before.values())==15
 assert sum(tid!=c['owner'] for tid,p in emitted for r,pl,c in held if p==pl)==3
 checks.append('Original R3 DLL: native collector retains 15 published SoftCommitted HS targets before guards; late finalization leaves them queued; captured consumer dispatches 15 repeats, including 3 rival schools')
else:
 assert emitted==[] and owned()==before
 callback(pre);callback(late);assert owned()==before and not emitted
 checks.append('R4: all 15 live cases excluded by captured collector; original owners preserved; zero repeated publication; repeated callbacks idempotent')
 # Full stage coverage, including the two Battle acquisitions observed in TP4.
 for stage in range(7):
  held=initialize(stage);before=owned();callback(pre)
  for tid in sorted({c['destination'] for r,p,c in held}):
   v=vector();collect(tid,4,v);collect(tid,5,v);assert not vitems(v);publish(tid,v);destroy(v)
  assert owned()==before and all(records[r]['scalar'][9]==6 for r,p,c in held)
 checks.append('Published HS stages 0 through 6: never collected again, including user-school and academy cases')
 # Real new HS and portal targets must still flow through the same collector
 # and consumer, with native simulated signing between those boundaries.
 reset_recovery();records[season]['scalar'].update({4:6,5:6});emitted.clear()
 new=[]
 for rid,portal,stage in [(59001,False,4),(59002,False,5),(59003,True,4),(59004,True,5)]:
  r,p=rr(rid,stage=stage,portal=portal,schools=((10,1000),),offers=(10,));new.append((r,p,stage))
 callback(pre);v=vector();collect(10,4,v);collect(10,5,v)
 assert len(vitems(v))==4 and all(records[r]['scalar'][9]==stage for r,p,stage in new)
 for r,p,stage in new:records[r]['scalar'][9]=6
 callback(late);publish(10,v);destroy(v)
 assert sorted(emitted)==sorted((10,p) for r,p,stage in new)
 checks.append('Unpublished HS and portal soft/hard commits remain collectible and publish through captured consumer; no eligibility or signing policy change')
 for mode_value,active in [(0,True),(-1,True),(1,False)]:
  held=initialize();configure(mode_value);before=owned();callback(pre,active)
  assert owned()==before and all(records[r]['scalar'][9]==4 for r,p,c in held)
 checks.append('Diagnostic, disabled and non-NSD calls preserve stages and membership')
 for bad in ('cross_owner','same_owner','duplicate_HS','invalid_stage'):
  held=initialize();r,p,c=held[-1]
  if bad in ('cross_owner','same_owner'):records[records[allteams[10 if bad=='cross_owner' else c['owner']]]['ref'][33]]['array'].append(p)
  if bad=='duplicate_HS':records[hslist]['array'].append(r)
  if bad=='invalid_stage':records[r]['scalar'][9]=7
  before=owned();callback(pre)
  assert call(exports['DriTestQualityFault'])==1 and owned()==before and all(records[r]['scalar'][9]==(7 if bad=='invalid_stage' and i==14 else 4) for i,(r,p,c) in enumerate(held))
 checks.append('Malformed/conflicting ownership refused before first stage write; cleanup protection remains enabled')
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[k]['name'],v) for k,v in refs.items() if v!=1]
checks.append('All retained SDF references balanced')
report={'passed':True,'negative_control':negative,'cases':15,'cross_school':3,'checks':checks,'preSaveSHA256':data['preSaveSHA256'],'failedSaveSHA256':data['failedSaveSHA256'],'fixtureSHA256':hashlib.sha256((OUTPUT/'retention_fixture.dll').read_bytes()).hexdigest(),'live_game_verified':False,'limits':'Captured collector/consumer and compiled hooks; SDF services, sorting intrinsics and native append event are controlled fixtures, not a full game simulation.'}
if not negative:
 control=Path(os.environ['DRI_TEST28R3_CONTROL_DLL']);out=OUTPUT/'r3-negative';out.mkdir(exist_ok=True);shutil.copy2(control,out/'retention_fixture.dll')
 x=subprocess.run([sys.executable,__file__],env=dict(os.environ,DRI_TEST_OUTPUT_DIR=str(out),DRI_R4_NEGATIVE='1'),capture_output=True,text=True)
 (OUTPUT/'r3-negative.log').write_text(x.stdout+x.stderr);assert x.returncode==0,x.stdout[-1500:]+x.stderr[-3000:]
 report['r3_negative_control']=json.loads((out/'test28r4_nsd_collection_results.json').read_text())
 assert report['r3_negative_control']['negative_control'] and report['r3_negative_control']['fixtureSHA256']==hashlib.sha256(control.read_bytes()).hexdigest()
(OUTPUT/'test28r4_nsd_collection_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
