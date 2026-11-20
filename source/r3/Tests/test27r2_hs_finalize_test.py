"""Compiled NSD callback with real Test27 identities and controlled SDF storage.
Native admission guard machine path is separately covered by bulk_guard_test.
Publication in this fixture is explicit; this is not a full game advance.
Set DRI_TEST27R2_EXPECT_OLD=1 with the unchanged R1 DLL for negative control.
"""
from pathlib import Path
fixture=Path(__file__).parent/'recovery_runtime_test.py'
exec(compile(fixture.read_text().split('\nreset_recovery()\n# Test10-shaped')[0],str(fixture),'exec'))
import re
old=os.environ.get('DRI_TEST27R2_EXPECT_OLD')=='1'
data=json.loads((ROOT.parent/'Audit/Test27R2/live-hs-cases.json').read_text())
names=re.findall(r'^\.global\s+(Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
names=[n for n in names if not n.startswith('HookEps')]
bulk=hooks[names.index(next(n for n in names if 'BulkGuard' in n))]
frame27=STACK+0x65000;bp27=frame27+0x400
refresh=[];game_api(0x081bb580,lambda:refresh.append(args()[0]) or 0)
game_api(0x0816b130,lambda:1)
def acquire(player,tid):records[records[allteams[tid]]['ref'][33]]['array'].append(player)
def snapshot():return {tid:list(records[records[t]['ref'][33]]['array']) for tid,t in enumerate(allteams)}
def guard(rec,tid):
 wq(frame27+10*8,bp27);wq(bp27-0x19,rec);wq(frame27+3*8,allteams[tid]);wq(frame27+2*8,context)
 return call(bulk,frame27)
def finish(active=True):
 wq(frame27+10*8,bp27);wq(frame27+2*8,context);uc.mem_write(context+0x1a0,bytes([active]))
 global next_heap
 before=next_heap
 result=call(hooks[24],frame27)
 # The real heap frees its scratch buffers; fixture arena reclamation.
 next_heap=before
 return result
stages={'Top10':0,'SoftCommitted':4,'HardCommitted':5}
checks=[]
reset_recovery();records[season]['scalar'].update({4:6,5:6})
held=[]
for case in data['cases']:
 rec,player=rr(case['rid'],ovr=case['ovr'],stage=stages[case['stage']],portal=False,offers=())
 acquire(player,case['owner']);held.append((rec,player,case))
before=snapshot()
# These 94 published hard commits entered the old native destination queue
# a second time. Model the consequent append only if the compiled guard permits.
for rec,player,case in held:
 if case['stage']=='HardCommitted':
  result=guard(rec,case['owner'])
  assert result==(0 if old else 13)
  if result==0:records[rec]['scalar'][9]=6;acquire(player,case['owner'])
finish()
assert call(exports['DriTestQualityFault'])==0
if old:
 assert sum(len(v) for v in snapshot().values())-sum(len(v) for v in before.values())==94
 assert sum(records[r]['scalar'][9]!=6 for r,p,c in held)==14
 checks.append('R1 negative control reproduces 94 repeat publications and leaves all 14 stale stages')
else:
 assert snapshot()==before and all(records[r]['scalar'][9]==6 for r,p,c in held)
 assert refresh==[context]
 finish();assert snapshot()==before and refresh==[context]
 checks.append('All 108 actual published HS identities finish Signed once at original schools; repeat call is idempotent; class stats refresh once')
 # No acquisition can be inferred from shortlist rank, active offer, or stage.
 reset_recovery();refresh.clear()
 unowned,_=rr(59000,stage=5,portal=False,offers=())
 portal,portal_player=rr(59001,stage=4,portal=True,offers=())
 acquire(portal_player,10)
 finish()
 assert records[unowned]['scalar'][9]==5 and records[portal]['scalar'][9]==4 and refresh==[]
 checks.append('Unpublished HS hard commits and published portal stages remain untouched')
 for category in range(4):
  for stage in range(6):
   for user in (False,True):
    reset_recovery();refresh.clear()
    rec,player=rr(59100+stage,stage=stage,portal=False,offers=())
    records[rec]['scalar'][2]=category;acquire(player,10)
    if user:records[allteams[10]]['ref'][407]=record('user coach')
    state=dict(records[rec]['scalar']);interests=values(rec);saved=snapshot()
    assert guard(rec,10)==13 and guard(rec,11)==13
    finish();assert records[rec]['scalar']==(state|{9:6})
    assert values(rec)==interests and snapshot()==saved and refresh==[context]
 checks.append('All four HS/JUCO classes, stages 0 through 5, CPU and user: only stage changes; admission, interests and membership preserved')
 for mode_value,active in [(0,True),(-1,True),(1,False)]:
  reset_recovery();refresh.clear();rec,player=rr(59200,stage=4,portal=False,offers=());acquire(player,10)
  configure(mode_value);finish(active)
  assert records[rec]['scalar'][9]==4 and refresh==[]
 checks.append('Diagnostic, disabled, and non-NSD callback calls do not write')
 # Preflight the entire batch: even an earlier valid item must not be changed
 # when a later acquisition or HS identity is ambiguous.
 for bad in ('cross_owner','same_owner','duplicate_recruit','bad_stage'):
  reset_recovery();refresh.clear();a,ap=rr(59300,stage=4,portal=False,offers=());acquire(ap,10)
  b,bp=rr(59301,stage=4,portal=False,offers=());acquire(bp,11)
  if bad=='cross_owner':acquire(bp,12)
  if bad=='same_owner':acquire(bp,11)
  if bad=='duplicate_recruit':records[hslist]['array'].append(b)
  if bad=='bad_stage':records[b]['scalar'][9]=7
  saved=snapshot();finish()
  assert call(exports['DriTestQualityFault'])==1 and records[a]['scalar'][9]==4 and snapshot()==saved and refresh==[]
 checks.append('Cross-school conflicts, same-school duplicates, duplicate HS identities and malformed stages are refused before any batch mutation')
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[k]['name'],v) for k,v in refs.items() if v!=1]
report={'checks':checks,'cases':len(data['cases']),'negativeControl':old,'preSaveSHA256':data['preSaveSHA256'],'postR1SaveSHA256':data['postR1SaveSHA256'],'live_game_verified':False}
(OUTPUT/'test27r2_hs_finalize_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
