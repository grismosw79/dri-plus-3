"""Test31 compiled HS renewal, native removal and retry accounting.
Uses existing SDF/Windows/native-service fixtures without executing their old
vacancy-policy assertions. Native scorer/eligibility and spending are controlled;
captured removal machine code, compiled plan/funding callbacks are executed.
"""
from pathlib import Path
import os,json,struct,hashlib
os.environ.setdefault('DRI_TEST_REUSE_NATIVE_HEAP','1')
os.environ.setdefault('DRI_TEST_INSTRUCTION_LIMIT','100000000')
T=Path(__file__).parent
exec(compile((T/'runtime_fixture_test.py').read_text().split('\nQ1,TQ1=')[0],'Test31 native fixture setup','exec'))
source=(T/'portal_runtime_test.py').read_text()
exec(compile('star_cb,weekly_cb,instant_cb,sign_cb,deadline_cb=hooks[3:8]'+source.split('star_cb,weekly_cb,instant_cb,sign_cb,deadline_cb=hooks[3:8]')[1].split('\nkeep=pp')[0],'Test31 portal record setup','exec'))
source=(T/'recovery_runtime_test.py').read_text()
exec(compile('newchecks=[]'+source.split('newchecks=[]',1)[1].split('\nreset_recovery()\n# Test10-shaped')[0],'Test31 full market setup','exec'))
source=(T/'test28r8_abandoned_test.py').read_text()
exec(compile('checks8=[]'+source.split('checks8=[]',1)[1].split('\nreset8();r,p=recruit8(85000)')[0],'Test31 recovery helpers','exec'))
entry=call(exports['DriTestHsShortlistHook'])
uc.mem_write(GAME+0x0862d990,b'\xff\x25\0\0\0\0'+struct.pack('<Q',entry)+b'\x90\x90')
checks=[];observations=[]
scores={}
def score31():
 scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and selection==0 and portal==0
 cand=rq(reg('RSP')+0x30);wi(cand+40,scores.get(rq(cand),300));return 0
game_api(0x08185840,score31)
def role31(r,tid=96,field=0):return call(exports['DriTestHsPlanRole'],r,allteams[tid],context,field)
def annual31(tid,pos):
 out=alloc(73*4);assert call(exports['DriTest29ClassPlan'],allteams[tid],context,out)==1
 return ri(out+4*(7+pos))
def next_single_year(pos,tid=96):
 for year in range(2050,2100):
  records[season]['scalar'][2]=year
  if annual31(tid,pos)==1:return year
 raise AssertionError(('no annual slot',tid,pos))
def target31(rid,score,ovr=72,pos=10,stage=0,points=100):
 r,p=rr(rid,score=1000,ovr=ovr,stage=stage,schools=((96,points),(10,max(0,points-10))),offers=(96,),portal=False)
 records[p]['scalar'].update({3:pos,231:3,261:255});scores[r]=score
 target=next(x for x in records[board_targets(allteams[96])]['array']if records[x]['ref'].get(10)==r)
 return r,p,target
# Full inherited room no longer removes the approved normal annual pursuit.
reset8();year=next_single_year(10)
records[records[allteams[96]]['ref'][239]]['array']=[pp('LE returner '+str(i),82,pos=10,tid=96)for i in range(records[ideal_entries[10]]['scalar'][2])]
a,ap,at=target31(31001,447,72);assert role31(a)==1
b,bp,bt=target31(31002,382,82);records[bp]['scalar'][231]=4
assert (role31(a),role31(b))==(2,1)
assert (role31(a,field=3),role31(b,field=3))==(447,382)
assert call(exports['DriTestHs29Offer'],b,allteams[96],context)==2
checks.append('Alabama 72/447 versus 82/382 shape upgrades through the compiled board plan and firm-offer gate in an inherited full room; native scores unchanged')
observations.append({'shape':'Alabama','fixture_school':96,'year':year,'primary_ovr':82,'backup_ovr':72,'actual_native_scores':[447,382]})
# Native opponent-commit cleanup releases target resources before replacement.
reset8();next_single_year(10)
a,ap,at=target31(31010,500);b,bp,bt=target31(31011,400)
assert (role31(a),role31(b))==(1,2)
c,cp,ct=target31(31012,393,73)
assert role31(a)==1
records[a]['scalar'][9]=5
leader=records[records[a]['ref'][13]]['array'][0];records[leader]['scalar'][0]=10
valid_offer(a,destination=allteams[10]);records[a]['scalar'][14]=2
records[at]['ref'][11]=record('old visit');records[at]['scalar'][2]=25
before_points={r:values(r)for r in [a,b,c]};before_offer=records[a]['scalar'][14]
assert call(exports['DriTestHsRivalCleanup'],allteams[96],context)==1
assert records[at]['ref'][10]==0 and records[a]['scalar'][14]==before_offer-1,(records[at],records[a]['scalar'],before_offer,native_events)
assert ('refund_hours',25) in native_events and ('visit_cancel',at) in native_events
assert (role31(b),role31(c))==(2,1)
assert role31(b,field=2)==role31(c,field=2)
assert before_points=={r:values(r)for r in [a,b,c]}
# Actual funding callback consumes this exact current primary/backup ordering.
ff=STACK+0x55000;fb=ff+0x400;wq(ff+80,fb);wq(ff+72,context)
tr=alloc(8);wq(tr,allteams[96]);wq(fb-0x60,tr)
vec=alloc(144);ind=alloc(16);wq(fb-0x50,vec);wq(fb-0x48,vec+144);wq(fb+0x58,ind);wq(fb+0x60,ind+16)
for j,(r,t)in enumerate([(b,bt),(c,ct)]):
 wq(vec+j*72,r);wi(vec+j*72+40,300);wq(vec+j*72+48,t);wi(vec+j*72+64,1);wq(ind+j*8,j)
# Test32 adds an allowance query. This legacy controlled-service fixture has no
# native allowance frame; preserve its weighting-only scope with sufficient
# modeled allowance. Dedicated Test32 tests execute native limits/spending.
game_api(0x0817e7b0,lambda:50)
call(hooks[23],ff)
# Test32 permutes complete candidate records for both native allocation passes.
# Bind assertions to recruit identity, not the original physical vector slots.
funding_by_recruit={rq(vec+j*72):ri(vec+j*72+40) for j in range(2)}
assert set(funding_by_recruit)=={b,c}
assert sum(funding_by_recruit.values())==600 and funding_by_recruit[c]>funding_by_recruit[b]>0
assert call(exports['DriTestHs29Offer'],c,allteams[96],context)==2
assert call(exports['DriTestHs29Offer'],b,allteams[96],context)==1
checks.append('Captured rival cleanup refunds hours and visit, decrements offered counter and releases stale target; compiled replacement compares fallback with new alternative; actual funding callback preserves native budget sum and primary priority')
observations.append({'shape':'lost primary','native_score_budget':600,'backup_weight':funding_by_recruit[b],'primary_weight':funding_by_recruit[c],'native_removal':True,'interest_unchanged':True})
# Native-eligible, positive-score but nonviable narrowed race cannot be backup.
reset8();next_single_year(10)
a,ap,at=target31(31020,500)
b,bp,bt=target31(31021,450,74,stage=2,points=0)
records[records[b]['ref'][13]]['array']=[record('nonviable interest',{0:tid,1:points})for tid,points in [(10,1000),(11,900),(12,800),(96,0)]]
assert role31(a)==1 and role31(b)==0
checks.append('LSU nonviable-backup shape is excluded by the compiled board planner despite positive native fit')
# Failure credit lasts only the current allocation scope. Repeated safe passes
# do not retry the same unchanged school; next week revalidates from scratch.
for failure in ['not_admitted','offered_unfunded','funded_then_reset']:
 reset8();call(exports['DriTest31FundingReset']);next_single_year(0);r,p=recruit8(31100+len(failure));recover8()
 t=allteams[96];before=snap8(r)
 assert call(exports['DriTestHsRecoveryPending'],r,context)==1
 if failure!='not_admitted':
  target=valid_offer(r,destination=t);records[target]['scalar'][8]=0
  records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
  records[target]['scalar'][15]=int(failure=='funded_then_reset')
  records[records[t]['ref'][224]]['scalar'][1]=5 if failure=='funded_then_reset' else 0
 call(exports['DriTestHsCoverageObserve'],t,context)
 pending=call(exports['DriTestHs31RetryPending'],r,t)
 assert pending==int(failure!='funded_then_reset'),(failure,pending)
 if failure=='not_admitted':
  assert call(exports['DriTestHsRecoveryPending'],r,context)==0
  recover8();recover8();assert call(exports['DriTestHsRecoveryPending'],r,context)==0 and snap8(r)==before
 if failure=='funded_then_reset':
  records[target]['scalar'][15]=0;records[records[t]['ref'][224]]['scalar'][1]=0
  call(exports['DriTestHsCoverageObserve'],t,context)
  assert call(exports['DriTestHs31RetryPending'],r,t)==0
 records[season]['scalar'][4]+=1;recover8()
 assert call(exports['DriTestHs31RetryPending'],r,t)==0
 if failure=='not_admitted':
  assert call(exports['DriTestHsRecoveryPending'],r,context)==1 and snap8(r)==before
 else:assert role31(r)==1 and offer_status(r,96)
 if failure=='funded_then_reset':
  # A subsequently removed offer is not protected by stale lifetime credit.
  records[target]['scalar'][12]=1;recover8()
  assert call(exports['DriTestHsRecoveryPending'],r,context)==1
  call(exports['DriTestHsCoverageObserve'],t,context)
  assert call(exports['DriTestHs31RetryPending'],r,t)==1
 print('PASS recovery',failure,flush=True)
 observations.append({'recovery_case':failure,'deferred_before_week':bool(pending),'next_week_valid':True})
checks.append('Failed admission and never-funded offers permit next-week revalidation, never same-week retry churn; previously funded legitimate offer continuity survives later zero-hour reset without withdrawing the offer')
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHsCoverageReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
result={'checks':checks,'observations':observations,'dll_sha256':hashlib.sha256((OUTPUT/'retention_fixture.dll').read_bytes()).hexdigest(),'live_game_verified':False,'method':'compiled Windows DLL callbacks and captured native removal; controlled eligibility/native scores/actions/SDF; funding callback weights are not live native spending'}
(OUTPUT/'test31_hs_runtime_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
