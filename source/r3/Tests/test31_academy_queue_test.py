"""Test31 academy return detours, native finalizer, queue validation and consumer.
SDF storage and final player append are controlled; no live season claim.
"""
from pathlib import Path
import json,hashlib
exec(compile((Path(__file__).parent/'test29r2_portal_retry_native_test.py').read_text().split('cases=[]')[0],__file__,'exec'))
assert hashlib.sha256(raw).hexdigest() in {'7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e','f04870ba6c6490b6353157d0bd7fddbbb615413635b7861cfe357b28308899cf'}
configure(1);records[season]['scalar'].update({4:6,5:6})
records[team]['scalar'][383]=7;records[teams]['array']=fixture_team_array()
hook=call(exports['DriTest29R3DeadlineHook'])
uc.mem_write(GAME+0x081b590d,b'\xff\x25\0\0\0\0'+struct.pack('<Q',hook)+b'\x90'*13)
target_hook=call(exports['DriTest29R4ReturnTargetHook'])
uc.mem_write(GAME+0x081b5c57,b'\xff\x25\0\0\0\0'+struct.pack('<Q',target_hook))
uc.ctl_remove_cache(GAME+0x081b5610,GAME+0x081b5f71)
# Execute the captured reference setter; only its SDF table service is modeled.
map_game(0x0805e4d0,0x50)
uc.mem_write(GAME+0x0805e4d0,raw[0x0805e4d0-0x1000:0x0805e4d0-0x1000+0x50])
handlers.pop(GAME+0x0805e4d0,None);uc.ctl_remove_cache(GAME+0x0805e4d0,GAME+0x0805e520)
writes=[]
def set_reference_native():
 table,index,field,holder=args();rec=tables[table][index]
 records[rec]['ref'][field-0x100000]=rq(holder);writes.append((rec,field-0x100000,rq(holder)));return 0
wq(TABLEVT+0x160,api(set_reference_native))
frame=STACK+0x65000;outer_bp=frame+0x400
wq(frame+2*8,context);wq(frame+10*8,outer_bp)
map_object=outer_bp-0x78;book=outer_bp-0x18
wq(book+8,buckets);wi(book+0x10,1);wq(buckets,0);wq(buckets+8,0)
game_api(0x08103c20,lambda:0)
global_address=0x0bd0c84c;map_game(global_address,4)
# Real publication consumer, including missing Recruit reference skip branch.
map_game(0x08164d40,0x600)
uc.mem_write(GAME+0x08164d40,raw[0x08164d40-0x1000:0x0816532e-0x1000])
game_api(0x03803770,lambda:192) # native array capacity
game_api(0x08138640,lambda:0) # one-element insertion sort
game_api(0x0813f1c0,lambda:0) # one-element quicksort
game_api(0x081bc3e0,lambda:0) # erase empty tail after uniqueness scan
game_api(0x037f7ec0,lambda:0) # local destination-vector destructor
checks=[];published=[];assert_predicates=[]
def validation():
 assert_predicates.append(bool(reg('RCX')));return int(bool(reg('RCX')))
game_api(0x037f5e40,validation)
def player_append():
 manager_arg,th,ph,_=args();assert rq(th)==team
 p=rq(ph);records[committed]['array'].append(p);published.append(p);return 0
game_api(0x0859b890,player_append)
for origin,link in [(o,l) for o in [0,7,55] for l in [False,True]]:
 case=str(origin)+('_linked_control' if link else '_new_unlinked')
 records[team]['scalar'][383]=origin;records[teams]['array']=fixture_team_array()
 records[committed]['array']=[];call(exports['DriTestOwnershipReset'])
 r,t,p=pr(129800+len(checks),70,1,pos=12,stage=0)
 records[p]['scalar'][230]=origin;records[r]['scalar'][14]=0;records[t]['scalar'][12]=0
 aux_for[r]=record('native aux',{0:100+len(checks)})
 fresh=record('new native RecruitTarget',ref={10:r} if link else {})
 def target_missing():
  _,_,_,out=args();wq(out,0);return out
 def native_create():
  _,out,_,_=args();incref(fresh);wq(out,fresh);return out
 game_api(0x08178da0,target_missing);game_api(0x0804b440,native_create)
 n=len(appends);before_writes=len(writes);wq(rq(fresh+0x10)+0x10,alloc(0x200));result=call(exports['DriTest29R3Restore'],frame)
 assert result==1 and records[r]['scalar'][9]==6 and appends[n:]==[fresh]
 assert records[fresh]['ref'].get(10,0)==r
 assert len(writes)-before_writes==int(not link)
 # Consumer owns one retained local-vector entry, as the native caller does.
 local_vector=alloc(24);local_data=alloc(8);wq(local_data,fresh);incref(fresh)
 wq(local_vector,local_data);wq(local_vector+8,local_data+8);wq(local_vector+16,local_data+8)
 # Real native queued return has Signed6, observed own-academy destination,
 # a repaired Target.Recruit link and no active offer. Test31 must retain it.
 checked_map=alloc(64);checked_buckets=alloc(8);checked_node=alloc(48)
 wq(checked_map+8,checked_buckets);wi(checked_map+0x10,1);wq(checked_buckets,checked_node)
 wi(checked_node,origin);uc.mem_write(checked_node+8,bytes(uc.mem_read(local_vector,24)))
 local_vector=checked_node+8
 assert call(exports['DriTest31NsdQueue'],checked_map,context)==1
 assert rq(local_vector+8)-rq(local_vector)==8
 assert records[r]['scalar'][14]==0 and records[fresh]['scalar'].get(12,0)==0
 th=alloc(8);wq(th,team);before=len(published)
 call(GAME+0x08164d40,context,th,local_vector)
 decref(fresh) # controlled caller owns and releases its extra vector entry
 added=len(published)-before
 assert added==1,(case,added)
 assert call(exports['DriTest29R4VerifyReturns'],context)==1
 assert call(exports['DriTest29R4VerifyReturns'],context)==0
 assert call(exports['DriTest29R3Restore'],frame)==0
 assert call(exports['DriTestQualityFault'])==0
 checks.append({'case':case,'queued_for_native_publication':result,'recruit_stage':records[r]['scalar'][9],'target_has_recruit_link':bool(records[fresh]['ref'].get(10)),'validated_queue_retained_without_offer':True,'native_consumer_reached_player_append':bool(added),'published_to_modeled_incoming':p in records[committed]['array']})
# A native Signed flag with no actual publication must fail verification.
records[team]['scalar'][383]=7;records[teams]['array']=fixture_team_array()
records[committed]['array']=[];call(exports['DriTestOwnershipReset'])
r,t,p=pr(129810,70,1,pos=12,stage=0);records[p]['scalar'][230]=7
records[r]['scalar'][14]=0;aux_for[r]=record('missing publication aux',{0:210})
fresh=record('unpublished target');wq(rq(fresh+0x10)+0x10,alloc(0x200))
assert call(exports['DriTest29R3Restore'],frame)==1
assert call(exports['DriTest29R4VerifyReturns'],context)==0xffffffff
assert call(exports['DriTestQualityFault'])==1
configure(1);call(exports['DriTest29R4ClearReturns'])
# A target already linked to a different recruit is never overwritten or appended.
r,t,p=pr(129811,70,1,pos=12,stage=0);records[p]['scalar'][230]=7
records[r]['scalar'][14]=0;aux_for[r]=record('conflict aux',{0:211})
fresh=record('conflicting target',ref={10:next(iter(targets_by_recruit))})
original=records[fresh]['ref'][10];n=len(appends)
assert call(exports['DriTest29R3Restore'],frame)==0xffffffff
assert records[fresh]['ref'][10]==original and len(appends)==n
assert records[r]['scalar'][9]!=6 and call(exports['DriTestQualityFault'])==1
call(exports['DriTest29R4ClearReturns']);call(exports['DriTestOwnershipReset'])
for target in appends:decref(target)
assert all(v==1 for v in refs.values()),[(records[x]['name'],v)for x,v in refs.items()if v!=1]
checks.append({'negative_cases':'Missing publication is a fault; conflicting target identity fails closed before append; native cleanup balances references'})
out={'passed':True,'live_game_verified':False,'checks':checks,
 'captured_native':['0x081b5610 finalizer','0x0805e4d0 reference setter','0x08164d40 publication consumer'],
 'fixture_sha256':hashlib.sha256((OUTPUT/'retention_fixture.dll').read_bytes()).hexdigest(),'capture_code_sha256':hashlib.sha256(raw).hexdigest(),'limitation':'Controlled evidence-shaped SDF storage and final append event; no live save replay or season result.'}
(OUTPUT/'test31_academy_queue_results.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
