"""Compiled R12 recovery and role decisions, shaped by W5 evidence.
Native score/eligibility inputs are controlled; this is not a live W5 replay.
"""
from pathlib import Path
import os
os.environ.setdefault('DRI_TEST_INSTRUCTION_LIMIT','100000000')
source=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(source.split('reset29()\nr,p=make(91000')[0],'R11 fixture definitions','exec'))
checks=[];scores={}
def scored():
    scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and not selection and not portal
    cand=rq(reg('RSP')+0x30);wi(cand+40,scores.get(ri(rq(cand)+0x18),300));return 0
game_api(0x08185840,scored)
def prep():
    reset29();scores.clear()
    records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
    for t in allteams:records[records[t]['ref'][224]]['scalar'][3]=0
    call(exports['DriTestHs29Reset'])
def prospect(rid,tid,ovr,stars,score,offered=True,points=500):
    r,p=make(rid,((tid,points),),[tid] if offered else [],ovr=ovr)
    records[p]['scalar'][231]=stars-1;scores[rid]=score
    return r,p
# Saved primary first, then materially stronger backup arrives: actual runtime
# plans must replace the primary and share the SAME opening with its fallback.
for tid,rid,oldid,ovr,oldovr,score,oldscore in [(105,1190,99001,72,67,460,330),(109,1952,1385,77,69,412,342)]:
    prep();t=allteams[tid];records[records[t]['ref'][224]]['scalar'][3]=100
    old,op=prospect(oldid,tid,oldovr,3,oldscore)
    assert call(exports['DriTestHsPlanRole'],old,t,context,0)==1
    elite,ep=prospect(rid,tid,ovr,4,score)
    role=call(exports['DriTestHsPlanRole'],elite,t,context,0)
    if 'DriTestHs30Seed' not in exports:
        assert role==2
        print('R11_NEGATIVE_REPRODUCED: retained lower-quality primary leaves elite backup conditional')
        raise SystemExit(0)
    assert role==1 and call(exports['DriTestHsPlanRole'],old,t,context,0)==2,(tid,role)
    assert call(exports['DriTestHs29Offer'],elite,t,context)==2
    assert call(exports['DriTestHs29Offer'],old,t,context)==1
    assert call(exports['DriTestHsPlanRole'],old,t,context,2)==call(exports['DriTestHsPlanRole'],elite,t,context,2)
checks.append('Root/Bunch-shaped saved primaries upgrade using both ability and native fit; displaced primary remains backup in the same opening')
# All conditional: higher scoring backup destination must not beat a lower
# scoring real primary opening at another school.
prep();r,p=make(94000,((10,950),(11,900)),[10,11]);records[p]['scalar'][231]=3
for tid in [10,11]:
    conditional(94100+tid,tid);records[records[allteams[tid]]['ref'][224]]['scalar'][3]=100
for tid in [12,13]:records[records[allteams[tid]]['ref'][224]]['scalar'][3]=100
allowed8.clear();allowed8.update([10,11,12,13])
assert call(exports['DriTestHs29Offer'],r,allteams[10],context)==1
before=snap8(r);call(exports['DriTestHsCoverageBegin'],context)
assert call(exports['DriTestHsRecoveryPending'],r,context)==1
assert call(exports['DriTestHsRecoveryDestination'],r,allteams[12],context,2)==1
assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[12],context,0)==10
assert snap8(r)==before and not offer_status(r,12)
checks.append('All-conditional elite gets a bounded affordable primary reservation; high dead/conditional influence does not block a fresh race; no offer or points fabricated')
# Live funding counts separately from role, and a midweek completed primary
# invalidates a funded backup immediately in END coverage.
for tid in [10,11]:
    target=next(x for x in records[board_targets(allteams[tid])]['array'] if records[x]['ref'][10]==r)
    records[target]['scalar'][15]=1;records[records[allteams[tid]]['ref'][224]]['scalar'][1]=5
    call(exports['DriTestHsCoverageObserve'],allteams[tid],context)
assert call(exports['DriTestHsCoverageCount'],r,0)==2
records[records[hslist]['array'][1]]['scalar'][9]=5
call(exports['DriTestHsCoverageEnd'],context)
assert call(exports['DriTestHsCoverageCount'],r,0)==1
checks.append('A funded backup becomes unavailable after its paired primary commits; coverage follows the live offer state')
# One firm alternative and an ineligible high-interest leader. Exact points
# stay in the save while catch-up compares to the surviving offer.
prep();r,p=make(94200,((10,1000),(11,500),(12,390),(13,380),(14,370),(15,360)),[10,11]);records[p]['scalar'][231]=3
allowed8.clear();allowed8.update([11,15]);before=snap8(r)
assert call(exports['DriTestHs30Catchup'],r,allteams[15],context)==0 # 360 < 75% of 500
entry=records[records[r]['ref'][13]]['array'][-1];records[entry]['scalar'][1]=400
assert call(exports['DriTestHs30Catchup'],r,allteams[15],context)==1
assert records[r]['scalar']==before[0] and values(r)[0]==(10,1000)
checks.append('Catch-up discounts unavailable leaders, preserves earned influence, and still enforces a 75% gap against the surviving real race')
# A closed one-firm race with demonstrably lost competition may reopen only
# after a distinct affordable CPU replacement has been proven.
prep();r,p=make(94210,((10,1000),(11,500),(12,350)),[10,11],stage=2);records[p]['scalar'][231]=3
allowed8.clear();allowed8.update([11,13])
records[records[allteams[13]]['ref'][224]]['scalar'][3]=100
before=snap8(r);call(exports['DriTestHs29Reopen'],context)
assert records[r]['scalar'][9]==0 and values(r)==list(before[1])
assert call(exports['DriTestHsRecoveryPending'],r,context)==1
assert not offer_status(r,13) and call(exports['DriTestHs29Offer'],r,allteams[11],context)==2
checks.append('Closed one-firm elite with a lost rival reopens only with a qualified, affordable, distinct CPU replacement; current firm offer and earned points are preserved')
prep();r,p=make(94211,((11,500),(12,350)),[11],stage=2);records[p]['scalar'][231]=3
allowed8.clear();allowed8.update([11,13]);records[records[allteams[13]]['ref'][224]]['scalar'][3]=100
before=snap8(r);call(exports['DriTestHs29Reopen'],context);assert snap8(r)==before
checks.append('Ordinary one-firm closed race stays closed without evidence of lost competition')
# A valid soft commitment survives departing rivals, unlike a soft choice
# whose own offer is now unavailable.
prep();r,p=make(94212,((11,1000),(10,800),(12,350)),[10,11],stage=4);records[p]['scalar'][231]=3
allowed8.clear();allowed8.update([11,13]);records[records[allteams[13]]['ref'][224]]['scalar'][3]=100
before=snap8(r);call(exports['DriTestHs29Reopen'],context);assert snap8(r)==before
checks.append('A valid firm soft commitment is not reopened merely because its rivals become unavailable')
# Off-board upgrade is reserved, then materializes only through native offer.
prep();t=allteams[105];records[records[t]['ref'][224]]['scalar'][3]=100
old,op=prospect(94300,105,67,3,330);assert call(exports['DriTestHsPlanRole'],old,t,context,0)==1
new,np=prospect(94301,105,72,4,460,False)
slot=call(exports['DriTestHsRecoveryDestination'],new,t,context,1)
assert call(exports['DriTestHsRecoveryDestination'],new,t,context,2)==1
assert call(exports['DriTestHsRecoveryReserve'],new,t,context,slot*4+1)==1
valid_offer(new,destination=t)
assert call(exports['DriTestHsPlanRole'],new,t,context,0)==1
assert call(exports['DriTestHsPlanRole'],old,t,context,0)==2
checks.append('An off-board material upgrade uses a verified primary reservation and becomes firm only after a real native offer exists')
# Fresh role/room checks retain the intentional one-five-star cap.
records[np]['scalar'][231]=4
other,otherp=prospect(94302,105,85,5,600);records[other]['scalar'][9]=5
assert call(exports['DriTestHs29Offer'],new,t,context)==0
checks.append('One-five-star positional restriction remains authoritative over apparent availability')
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
# Rebuild exact distributed R11 source and run the identical first scenario.
import subprocess,sys,shutil
control=OUTPUT/'r11-control';control.mkdir(exist_ok=True)
shutil.copy2(os.environ['DRI_TEST28R11_CONTROL_DLL'],control/'retention_fixture.dll')
p=subprocess.run([sys.executable,__file__],env=dict(os.environ,DRI_TEST_OUTPUT_DIR=str(control)),capture_output=True,text=True)
(OUTPUT/'r11-primary-negative.log').write_text(p.stdout+p.stderr)
assert p.returncode==0 and 'R11_NEGATIVE_REPRODUCED' in p.stdout,p.stdout[-2000:]+p.stderr
(OUTPUT/'test28r12_market_results.json').write_text(json.dumps({'checks':checks,'R11_negative_control':True,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
