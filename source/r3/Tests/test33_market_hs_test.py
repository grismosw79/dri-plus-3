"""Compiled Test33 simultaneous HS admission and actual allocator publication.
Native SDF/scoring services are controlled fixtures, not a live-week replay.
The market witness has40 indexed schools/30 bidders,23/13 for acute tiers;
unchanged whole-franchise controls and138-school helper stress remain separate.
"""
from pathlib import Path
import os
os.environ.setdefault('DRI_TEST_INSTRUCTION_LIMIT','200000000')
os.environ.setdefault('DRI_TEST_REUSE_NATIVE_HEAP','1')
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
checks=[];scores={}
def score14():
 scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and not selection and not portal
 cand=rq(reg('RSP')+0x30);r=rq(cand);tid=records[rq(tr)]['scalar'][383]
 wi(cand+40,scores.get((records[r]['scalar'].get(10000,ri(r+0x18)),tid),0));return 0
game_api(0x08185840,score14)
def begin(acute=False):
 reset29();call(exports['DriTestHs32Reset']);scores.clear()
 records[teams]['array']=allteams[:23 if acute else 40]
 records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 records[ideal_entries[0]]['scalar'][2]=8 if acute else 4
 for tid,t in enumerate(allteams):
  records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:100 if 10<=tid<40 else 0})
  records[records[t]['ref'][239]]['array']=[pp('QB returner',65,pos=0,tid=tid)for _ in range(2 if acute else 3)]
 allowed8.clear();allowed8.update(range(10,40))
 # The allocator is reached after the global maintenance boundary in-game.
 # Initialize that scope before constructing this isolated market witness.
 call(exports['DriTestHs28Maintenance'],context);assert call(exports['DriTestQualityFault'])==0
def new(rid,stars=3,ovr=78):
 r,p=make(rid,((0,0),),[],ovr=ovr);records[p]['scalar'][231]=stars;return r,p
def authorized(r):return [t for t in range(138)if call(exports['DriTestHs30Authorized'],r,t)]

def allocator(tid,items):
 t=allteams[tid];tr=alloc(8);wq(tr,t)
 af=alloc(0x1000);asp=af+128;abp=asp+0x200
 wq(af+80,abp);wq(af+16,context);wq(af+24,tr)
 wq(abp+0x28,tr);wq(abp+0x58,season);wq(abp+0x180,need);wq(asp+0x58,context);uc.mem_write(asp+0x42,b'\0')
 call(hooks[8],af);call(start,af)
 entries=[]
 for r,value in items:
  e=bytearray(48);struct.pack_into('<Q',e,0,r);struct.pack_into('<i',e,40,value);entries.append(bytes(e))
 vec=alloc(24);mem=alloc((len(items)+2)*48);main=mem+48;canary=b'\xa5'*48
 uc.mem_write(mem,canary+b''.join(entries)+canary);wq(vec,main);wq(vec+8,main+48*len(items));wq(vec+16,main+48*len(items))
 wq(abp-0x68,vec);wq(asp+0x68,len(items));wi(abp+0x30,50);wq(asp+0x50,0);wq(asp+0x78,0);wq(abp-0x80,0)
 call(hooks[15],af);admitted=[]
 for r,_ in items:
  candidate=alloc(48);wq(candidate,r);wq(af+88,candidate);admitted.append(call(admit,af)==0)
 assert sorted(bytes(uc.mem_read(main+i*48,48))for i in range(len(items)))==sorted(entries)
 assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+len(items)*48,48))==canary
 assert call(exports['DriTestQualityFault'])==0
 return admitted

# Thirty simultaneous opening nominations; only the soft tier limit fills.
begin();hot,hp=new(1995);cold,cp=new(99000)
for tid in range(10,40):scores[(1995,tid)]=500
before=snap8(hot);call(exports['DriTestHs32Begin'],context);opening=authorized(hot)
assert len(opening)==8,opening
assert snap8(hot)==before
records[teams]['array']=list(reversed(allteams[:40]));call(exports['DriTestHs32Reset']);call(exports['DriTestHs32Begin'],context)
assert authorized(hot)==opening
records[teams]['array']=allteams[:40]
checks.append('Thirty simultaneous 4-star nominations select exactly8 opening permits, independent of native team-array order; offers and influence unchanged')
# A user-controlled callback must not release or import CPU reservations.
human=opening[0];records[allteams[human]]['ref'][407]=record('user marker')
call(exports['DriTestHs33ReleaseSchool'],allteams[human],context)
assert authorized(hot)==opening
records[allteams[human]]['ref'].pop(407)
checks.append('User-controlled after-funding release does not change CPU market reservations')
# A failed winner releases its place for a different unprocessed school.
loser=opening[0];call(exports['DriTestHs33ReleaseOffer'],hot,allteams[loser],context)
after=authorized(hot);assert loser not in after and len(after)==8 and set(after)-set(opening),(opening,after)
assert call(exports['DriTestHs30AdmissionBlock'],hot,allteams[loser],context)==1
winner=after[0];call(exports['DriTestHs33ReleaseSchool'],allteams[winner],context)
after_school=authorized(hot);assert winner not in after_school and len(after_school)==8
assert call(exports['DriTestHs30AdmissionBlock'],hot,allteams[winner],context)==1
call(exports['DriTestHs33ReleaseSchool'],allteams[winner],context);assert authorized(hot)==after_school
checks.append('Failed scholarship and unused completed-school nominations each promote an unprocessed school; failed/current-pass-completed school cannot reclaim a permit')
# Actual native pool has lost the former hot target; its unused reservation
# is released before planning and its current alternative obtains membership.
tid=after_school[0];scores[(99000,tid)]=480
assert allocator(tid,[(cold,480)])==[True]
assert tid not in authorized(hot) and tid in authorized(cold)
print('HS33_PROGRESS: rank, order and release checks passed',flush=True)
checks.append('Actual native allocator full-pool publication drops an absent provisional target, admits snapshot-fit0 now-current-native-score480 legal alternative, preserves vector bytes/canaries and produces no invariant fault')
# Same-scope subsequent shortlist publication can change an admitted school
# from primary to backup and back without freezing its forecast role.
role_tid=next(t for t in authorized(hot) if t!=tid)
scores[(99000,role_tid)]=520
assert allocator(role_tid,[(hot,500),(cold,520)])==[True,True]
assert role_tid in authorized(hot) and role_tid in authorized(cold)
assert allocator(role_tid,[(hot,500)])==[True]
assert role_tid in authorized(hot) and role_tid not in authorized(cold)
checks.append('Current admitted membership survives primary-to-backup role change; removing its new primary immediately promotes it and releases unused alternative membership')

# Lower and elite hard extensions require an actual acute quantity need.
tier_counts={}
for stars,want in [(1,6),(2,6),(3,12),(4,12)]:
 begin(True);r,p=new(1996,stars)
 for tid in range(10,40):scores[(1996,tid)]=500
 call(exports['DriTestHs32Begin'],context);tier_counts[stars]=len(authorized(r));assert tier_counts[stars]==want,tier_counts
print('HS33_PROGRESS: actual allocator, currentfit0 and role promotion passed; all tier limits passed',flush=True)
checks.append('Actual compiled all-school forecast extends2/3-star to6 and4/5-star to12 only for genuine acute quantity need')
# Existing legitimate offers are grandfathered, never forcibly trimmed.
begin();r,p=new(1997)
for tid in range(10,28):
 scores[(1997,tid)]=500;target=valid_offer(r,destination=allteams[tid]);records[target]['scalar'].update({15:1,8:0})
 records[records[allteams[tid]]['ref'][224]]['scalar'][1]=5
for tid in range(28,40):scores[(1997,tid)]=500
before=snap8(r);call(exports['DriTestHs32Begin'],context)
assert authorized(r)==list(range(10,28)),authorized(r)
assert snap8(r)==before and all(offer_status(r,t)>0 for t in range(10,28))
call(exports['DriTestHs33ReleaseSchool'],allteams[10],context);assert 10 in authorized(r)
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[28],context)==1
checks.append('Eighteen existing legitimate offers remain intact above the new12 hard ceiling; new nineteenth CPU is blocked, completed actual offer remains authorized')
# Retained Test32 accounting controls: stale source-mixed flags cannot be
# counted as native allocated attention; paper offers without budget cannot
# masquerade as an affordable nomination.
extras=[]
for tid in range(10,28):
 extra,ep=rr(99800+tid,score=1000,ovr=77,stage=0,schools=(),offers=(),portal=True)
 target=valid_offer(extra,destination=allteams[tid]);records[target]['scalar'][15]=1;extras.append((tid,target))
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],r,0)==0 and call(exports['DriTestHs32Count'],r,1)==0
for tid,target in extras:records[board_targets(allteams[tid])]['array'].remove(target)
for tid in range(10,40):
 for target in records[board_targets(allteams[tid])]['array']:records[target]['scalar'][15]=0
 records[records[allteams[tid]]['ref'][224]]['scalar'].update({1:0,3:0})
call(exports['DriTestHs32Begin'],context)
assert call(exports['DriTestHs32Count'],r,0)==0 and call(exports['DriTestHs32Count'],r,2)==0
print('HS33_PROGRESS: grandfathered offers and mixed-source/paper accounting passed',flush=True)
checks.append('Retained Test32 source accounting: HS5+portal5 with only5 assigned hours yields zero actual/firm attention; paper offers and zero productive budget yield zero actual attention and demand')
# Native close-race evidence still preserves every legitimate existing race.
for tid in range(10,28):
 records[records[allteams[tid]]['ref'][224]]['scalar'].update({1:5,3:100})
 target=valid_offer(r,destination=allteams[tid]);records[target]['scalar'].update({15:1,8:500})
 cold2,cp2=new(99600+tid,3,77);scores[(99600+tid,tid)]=480;valid_offer(cold2,destination=allteams[tid])
 records[records[r]['ref'][13]]['array'].append(record('close native interest',{0:tid,1:500}))
call(exports['DriTestHsPlanReset']);call(exports['DriTestHs32Begin'],context)
assert all(call(exports['DriTestHsPlanRole'],r,allteams[t],context,0)==1 for t in range(10,28))
checks.append('Retained Test32 close competition: all18 existing legitimately close native-influence contenders keep primary roles despite comparable offered alternatives')
# Scope validity and explicit user control retain their native behavior.
old_week=records[season]['scalar'][4];records[season]['scalar'][4]=old_week+1
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[10],context)==1
records[season]['scalar'][4]=old_week
call(exports['DriTestHs32Reset']);assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[10],context)==1
records[allteams[10]]['ref'][407]=record('user marker')
assert call(exports['DriTestHs30AdmissionBlock'],r,allteams[10],context)==0
records[allteams[10]]['ref'].pop(407)
checks.append('Missing/stale market execution scope fails closed for CPU; explicit user control bypasses CPU admission')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test33_market_hs_results.json').write_text(json.dumps({'status':'PASS','checks':checks,'opening_authorized':opening,'acute_tier_counts':tier_counts,'modeled_school_counts':{'opening':40,'bidders':30,'acute':23,'acute_bidders':13},'native_heap':fixture_native_heap.report(),'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
