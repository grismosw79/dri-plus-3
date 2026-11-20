"""Compiled shared-source reservations and funding order with modeled SDF.

Native action assignment is observed, not claimed as executed influence or a
signing. Native shortlist/ownership helpers use the captured supported image.
"""
from pathlib import Path
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
checks=[];scores={}
def score29():
 scorer,selection,portal,tr=args()
 assert scorer==fixture_hs_scorer and not selection and not portal
 candidate=rq(reg('RSP')+0x30);r=rq(candidate)
 wi(candidate+40,scores.get(ri(r+0x18),0));return 0
game_api(0x08185840,score29)
reset29();call(exports['DriTestHs32Reset']);records[season]['scalar'][2]=2027
records[tuning]['ref'][49]=record('source actions',array=[
 record('scholarship',{0:3,1:0,2:5,5:1,6:1}),
 record('social',{0:5,1:10,2:5,5:1,6:0})])
records[ideal_entries[0]]['scalar'][2]=4
t=allteams[10];budget=records[t]['ref'][224]
records[budget]['scalar'].update({1:0,2:0,3:100})
records[records[t]['ref'][239]]['array']=[pp('QB returner',65,pos=0,tid=10)for _ in range(3)]
primary,p=make(102901,((10,500),),[10],ovr=78)
backup,bp=make(102902,((10,480),),[10],ovr=77)
pt=valid_offer(primary,destination=t);bt=valid_offer(backup,destination=t)
scores[ri(primary+0x18)]=500;scores[ri(backup+0x18)]=480
out=alloc(88)
def reserved():
 assert call(exports['DriTest29HsReserved'],t,context,out)==1
 assert call(exports['DriTestQualityFault'])==0
 return list(struct.unpack('<22i',uc.mem_read(out,88)))
assert sum(reserved())==0
checks.append('Two displayed native offers without productive assignment reserve zero portal headcount')
records[pt]['scalar'][15]=1;records[budget]['scalar'][1]=5
call(exports['DriTestHsCoverageObserve'],t,context)
assert reserved()==[1]+[0]*21
assert call(exports['DriTestHsPlanRole'],primary,t,context,0)==1
assert call(exports['DriTestHsPlanRole'],backup,t,context,0)==2
records[bt]['scalar'][15]=1;records[budget]['scalar'][1]=10
call(exports['DriTestHsCoverageObserve'],t,context)
assert reserved()==[1]+[0]*21
checks.append('Funding boundary invalidates the cache; assigned primary reserves one actual vacancy, funded backup adds none')
before={r:dict(records[r]['scalar'])for r in [primary,backup,pt,bt,budget]}
assert reserved()==[1]+[0]*21
assert before=={r:dict(records[r]['scalar'])for r in before}
records[budget]['scalar'][1]=0
call(exports['DriTestHsCoverageObserve'],t,context)
assert sum(reserved())==0
assert reserved()==[0]*22
checks.append('Assigned-hour reset rejects stale action flags and caches the inconclusive result without writing offers, points or hours')
records[pt]['scalar'][15]=records[bt]['scalar'][15]=0
records[budget]['scalar'][1]=0
call(exports['DriTest29ClassInvalidate'],t)
assert sum(reserved())==0
records[t]['ref'][407]=record('user coach')
records[pt]['scalar'][15]=1;records[budget]['scalar'][1]=5
call(exports['DriTest29ClassInvalidate'],t)
assert sum(reserved())==0
records[t]['ref'].pop(407)
call(exports['DriTest29ClassInvalidate'],t)
assert reserved()==[1]+[0]*21
checks.append('User control has no CPU source reservation; returning to CPU ownership revalidates actual assigned offers')
# Four native funding entries: optional portal, HS backup, HS primary, portal.
# The policy only changes the index permutation within one native stage bucket.
portal1,_=rr(102903,ovr=80,stage=0,schools=((10,100),),offers=[10],portal=True)
portal2,_=rr(102904,ovr=79,stage=0,schools=((10,100),),offers=[10],portal=True)
entries=[(portal1,valid_offer(portal1,destination=t)),(backup,bt),(primary,pt),(portal2,valid_offer(portal2,destination=t))]
portal_target=entries[0][1];records[portal_target]['scalar'][15]=1
before={r:dict(records[r]['scalar'])for r in [pt,bt,portal_target,budget]}
call(exports['DriTestHsCoverageObserve'],t,context)
assert sum(reserved())==0 # HS5 + portal5 cannot fit the native assigned5.
assert before=={r:dict(records[r]['scalar'])for r in before}
records[budget]['scalar'][1]=10
call(exports['DriTestHsCoverageObserve'],t,context)
assert reserved()==[1]+[0]*21
records[portal_target]['scalar'][15]=0;records[budget]['scalar'][1]=5
call(exports['DriTestHsCoverageObserve'],t,context)
checks.append('Complete-board assignment validation rejects stale mixed HS5 plus portal5 flags under assigned5; the same primary reserves one quantity seat only when assigned10 covers both sources')
vec=alloc(4*72);idx=alloc(32)
for i,(r,target)in enumerate(entries):
 wq(vec+i*72,r);wi(vec+i*72+40,500-i);wq(vec+i*72+48,target)
 wi(vec+i*72+64,0);wq(idx+i*8,i)
before=bytes(uc.mem_read(vec,4*72));budget_before=dict(records[budget]['scalar'])
wq(STACK+0xf0000-8+0x28,idx)
call(exports['DriTest29HsFundingPriority'],t,context,vec,4)
assert [rq(idx+i*8)for i in range(4)]==[2,0,1,3]
assert bytes(uc.mem_read(vec,4*72))==before and records[budget]['scalar']==budget_before
assert call(exports['DriTestQualityFault'])==0
checks.append('Below the annual secured HS target the legal HS primary moves first within its native funding bucket; backup stays stable and every candidate/budget byte is unchanged')
reset29();call(exports['DriTestHs32Reset']);records[season]['scalar'][2]=2027;scores.clear()
t=allteams[10]
for pos in range(22):records[ideal_entries[pos]]['scalar'][2]=ideals[pos]
records[ideal_entries[0]]['scalar'][2]=4
annual_out=alloc(46*4);assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
annual=list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)));target=annual[0];quota=annual[1:23]
records[records[t]['ref'][239]]['array']=[pp('source returner',65,pos=pos,tid=10)for pos in range(21)]
board_primaries=[];rid=103000
for pos,n in enumerate(quota):
 for j in range(n):
  r,p=make(rid,((10,500),),[10],ovr=78);rid+=1
  records[p]['scalar'][3]=pos;scores[ri(r+0x18)]=500
  board_primaries.append((r,valid_offer(r,destination=t)))
assert len(board_primaries)==target
newcomer,p=make(103100,((10,500),),[],ovr=78)
records[p]['scalar'][3]=0;scores[ri(newcomer+0x18)]=500
assert all(call(exports['DriTestHsPlanRole'],r,t,context,0)==1 for r,_ in board_primaries)
assert call(exports['DriTestHsRecoveryDestination'],newcomer,t,context,2)!=1
records[board_targets(t)]['array'].remove(board_primaries[0][1])
call(exports['DriTestHsCoverageReset'])
assert call(exports['DriTestHsRecoveryDestination'],newcomer,t,context,2)==1
slot=call(exports['DriTestHsRecoveryDestination'],newcomer,t,context,1)
assert call(exports['DriTestHsRecoveryReserve'],newcomer,t,context,slot*4+1)==1
nt=valid_offer(newcomer,destination=t)
assert call(exports['DriTestHsPlanRole'],newcomer,t,context,0)==1
call(exports['DriTest29ClassInvalidate'],t)
assert sum(reserved())==0
checks.append('A complete annual source plan cannot promise another recovery primary; withdrawal opens exactly one normal slot, reservation survives native board admission, and unfunded offers reserve no portal headcount')
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test31_class_source_results.json').write_text(json.dumps({'checks':checks,'passed':True,'native_action_execution':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
