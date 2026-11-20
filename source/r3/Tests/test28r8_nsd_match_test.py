"""Compiled R8 deadline matching over school-specific primary/backup plans.
Native storage/offer gates are fixtures. No game process or full season runs.
"""
# Reuse controlled SDF/native-removal setup; the full legacy recovery cases
# remain an independent suite gate.
fixture=(__import__('pathlib').Path(__file__).parent/'recovery_runtime_test.py').read_text()
exec(compile(fixture.split('\nreset_recovery()\n# Test10-shaped',1)[0],'recovery_runtime_test.py','exec'))
records[season]['scalar'].update({4:6,5:6})
scores={}
def native_scores():
 scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and selection==0 and portal==0
 candidate=rq(reg('RSP')+0x30);r=rq(candidate);tid=records[rq(tr)]['scalar'][383]
 wi(candidate+40,scores.get((tid,ri(r+0x18)),300));return 0
game_api(0x08185840,native_scores)
def reset():
 reset_recovery();scores.clear();call(exports['DriTestNsdReset'])
 # Every test school's QB room has precisely one authorized vacancy.
 for tid in (10,11,20):fill(tid,(65,64,63))
def candidate(rid,schools,offers,priority):
 r,p=rr(rid,ovr=75,stage=3,schools=schools,offers=offers,portal=False)
 records[p]['scalar'][231]=1
 for tid,value in priority.items():scores[(tid,rid)]=value
 return r,p
def winner(r,prune=0):
 value=call(exports['DriTestNsdWinner'],r,context,prune)
 if value:decref(value)
 assert call(exports['DriTestQualityFault'])==0
 return value
checks=[]
# A likes X over Y. B likes Z over X. X has more influence at B, but B
# accepts Z; X must fall back to A and prevent A taking its backup Y.
reset()
x,xp=candidate(65000,((20,100),(10,90)),(10,20),{10:400,20:300})
y,yp=candidate(65001,((10,80),(11,50)),(10,11),{10:300,11:400})
z,zp=candidate(65002,((20,110),),(20,),{20:400})
before={r:values(r)for r in (x,y,z)}
assert winner(y,1)==allteams[11]
assert winner(x)==allteams[10] and winner(z)==allteams[20]
assert offer_status(y,10) and before=={r:values(r)for r in (x,y,z)}
checks.append('Multi-school rejection chain resolves actual primary destination before accepting fallback; deferred scholarship and earned influence preserved')
# Crossed school preferences and recruit preferences form a potential cycle.
# Recruit-proposing matching terminates and gives both recruits their highest
# influence accepting destinations, with one recruit at each opening.
reset()
x,xp=candidate(65010,((20,100),(10,90)),(10,20),{10:400,20:300})
y,yp=candidate(65011,((10,100),(20,90)),(10,20),{10:300,20:400})
assert winner(x)==allteams[20] and winner(y)==allteams[10]
assert winner(y)==allteams[10] and winner(x)==allteams[20]
checks.append('Crossed primary/backup cycle terminates deterministically without circular deferral; repeated lookup retains one destination per opening')
# Exercise the compiled signing callback's ownership/cache invalidation;
# the native service then publishes Signed, as in the supported NSD path.
nf=STACK+0x82000;nb=nf+0x400
wq(nf+10*8,nb);wq(nf+128+0x30,x);wq(nf+11*8,allteams[20]);wq(nb+0x57,context)
assert call(sign_cb,nf)==0
records[x]['scalar'][9]=6
assert winner(y,1)==allteams[10] and not offer_status(y,20)
checks.append('Actual compiled signing callback invalidates matching; same-advance signed ownership closes the occupied opening and removes only invalid fallback offer')
# A live user's offer participates by influence without CPU pair restrictions.
reset()
x,xp=candidate(65020,((10,90),(20,100)),(10,20),{10:400,20:300})
z,zp=candidate(65021,((20,110),),(20,),{20:400})
records[allteams[20]]['ref'][407]=record('manual user')
assert winner(x)==allteams[20] and winner(z)==allteams[20]
checks.append('User offers retain existing manual roster policy and highest-influence eligibility; CPU opening limits are not imposed on the user')
# Equal influence remains deterministic regardless of array/offer order.
reset()
x,xp=candidate(65030,((20,100),(10,100)),(20,10),{10:400,20:400})
assert winner(x)==allteams[10]
checks.append('Equal influence retains smaller TeamIndex tie break')
# An empty component is a normal unsignable result, not a fault.
reset()
x,xp=candidate(65040,((10,100),),(),{})
assert winner(x)==0 and not native_events
checks.append('No eligible offered destination returns no match without changing player or board')
# A malformed partner is discovered after retaining the first node. All
# references must still unwind, and the complete preflight prevents pruning.
reset()
x,xp=candidate(65050,((10,100),),(10,),{10:400})
y,yp=candidate(65051,((10,90),(10,80)),(10,),{10:300})
before=(values(x),values(y),offer_status(x,10),offer_status(y,10))
assert call(exports['DriTestNsdWinner'],x,context,1)==0
assert call(exports['DriTestQualityFault'])==1
assert before==(values(x),values(y),offer_status(x,10),offer_status(y,10)) and not native_events
configure(1)
checks.append('Malformed connected partner fails preflight without pruning or losing record references')
call(exports['DriTestOwnershipReset']);call(exports['DriTestNsdReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
(OUTPUT/'test28r8_nsd_match_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
