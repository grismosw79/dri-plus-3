"""Test11 compiled market recovery + captured lockout/native removal code.
SDF records and Windows services are controlled fixtures; this is not a live sim.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
newchecks=[]
wq(context+0xd0,tuning)
records[tuning]['scalar'].update({2:25,0x56:10}) # Test10 Battle cut and trigger margin.
records[season]['scalar'].update({4:3,5:6})

# Native removal is real machine code. Extend its controlled storage/cache
# services from the shared fixture's single team to every franchise team.
def recovery_get_board():
 mgr,out,tr,_=args();assert mgr==manager
 destination=rq(tr);board_record=records[destination]['ref'][224]
 targets=records[board_record]['ref'][0];incref(targets);wq(out,targets);return out
def recovery_erase_array():
 targets,index,_,_=args()
 native_events.append(('erase_target',records[targets]['array'][index]))
 records[targets]['array'].pop(index);return 0
def recovery_cache_cleanup():
 mgr,tr,cand,_=args();assert mgr==manager
 native_events.append(('cache_cleanup',rq(cand)));return 0
game_api(0x0818af10,recovery_get_board)
game_api(0x08052300,recovery_erase_array)
game_api(0x081ae980,recovery_cache_cleanup)

allteams=fixture_team_array()
def board_targets(destination):
 return records[records[destination]['ref'][224]]['ref'][0]
def reset_recovery():
 call(exports['DriTestOwnershipReset']);configure(1)
 records[tplist]['array']=[];records[hslist]['array']=[]
 records[departures]['array']=[];records[teams]['array']=allteams
 for destination in allteams:
  records[destination]['ref'].pop(407,None)
  records[destination]['scalar'][384]=9
  for ordinal in [33,239]:records[records[destination]['ref'][ordinal]]['array']=[]
  records[board_targets(destination)]['array']=[]
 records[ideal_entries[0]]['scalar'].update({2:4,4:1,0:0})
 native_events.clear()
def rr(rid,score=112,ovr=80,stage=3,schools=((10,99),(11,97),(12,50),(13,37)),offers=(),portal=True):
 rec,target,player=pr(rid,ovr,stage=1,portal=portal)
 records[rec]['scalar'].update({9:stage,3:score,14:len(offers)})
 top=records[rec]['ref'][13]
 records[top]['array']=[record('recovery interest',{0:tid,1:influence}) for tid,influence in schools]
 for tid in offers:valid_offer(rec,destination=allteams[tid])
 return rec,player
def values(rec):
 return [(records[s]['scalar'][0],records[s]['scalar'][1]) for s in records[records[rec]['ref'][13]]['array']]
def fill(tid,ratings=(94,92,91,90)):
 records[records[allteams[tid]]['ref'][239]]['array']=[pp('room '+str(tid)+' '+str(i),rating,tid=tid) for i,rating in enumerate(ratings)]
def offer_status(rec,tid):
 return any(records[t]['ref'].get(10)==rec and records[t]['scalar'].get(12,0)>=2 for t in records[board_targets(allteams[tid])]['array'])
def run_recovery(grace=0):
 global next_heap
 # The real DLL releases all scratch allocations before returning. Reuse this
 # fixed emulated arena across cases, preserving permanent fixture records.
 before=next_heap
 call(exports['DriTestRecoverMarket'],context,grace)
 end=next_heap
 if end>before:uc.mem_write(before,bytes(end-before))
 next_heap=before
 return call(exports['DriTestQualityFault'])
def locked(rec,tid):
 # Actual captured wrapper, same input convention used by runtime locked_out.
 holder=alloc(32);wq(holder,GAME+0x0bd2e980);wq(holder+8,rec);wq(holder+16,0xabcdef);wq(holder+24,1)
 team_holder=alloc(8);wq(team_holder,allteams[tid])
 return call(GAME+0x0862d990,holder,team_holder,context+0xd0)

reset_recovery()
# Test10-shaped Mestemaker: dead highly influenced leaders with active offers
# now have full QB rooms; a lower finalist is below the native Battle threshold.
drew,dp=rr(5001,167,90,schools=((10,397),(11,348),(12,105)),offers=(10,11))
fill(10);fill(11)
assert locked(drew,10)==0 and locked(drew,12)==1
assert run_recovery()==0
assert records[drew]['scalar'][9]==0 and values(drew)==[(10,0),(11,0),(12,0)]
assert not offer_status(drew,10) and not offer_status(drew,11)
assert records[drew]['scalar'][14]==0
assert len([e for e in native_events if e[0]=='erase_target'])==2
assert locked(drew,12)==0
newchecks.append('Mestemaker-shaped Battle: full-room leaders removed by captured native service; stale influence cleared; native stage0 permits a new race')

reset_recovery()
# Test10-shaped Lowry: WVU's real fourth-ranked offer has 37 earned influence.
lowry,lp=rr(5002,offers=(13,))
assert locked(lowry,10)==0 and locked(lowry,13)==1
assert run_recovery()==0
assert records[lowry]['scalar'][9]==0
assert values(lowry)==[(13,37),(10,0),(11,0),(12,0)]
assert offer_status(lowry,13) and records[lowry]['scalar'][14]==1
assert not native_events and locked(lowry,13)==0
before=(values(lowry),records[lowry]['scalar'].copy())
assert run_recovery()==0 and before==(values(lowry),records[lowry]['scalar'])
newchecks.append('Lowry-shaped Battle: locked-out WVU equivalent keeps its offer and 37 influence, moves ahead of withdrawn leaders, and becomes eligible; repeated recovery is inert')

for position_id in (1,3,10,14,19,20):
 reset_recovery()
 nonqb,np=rr(5100+position_id,offers=(13,))
 records[np]['scalar'][3]=position_id
 assert run_recovery()==0 and records[nonqb]['scalar'][9]==0
 assert values(nonqb)[0]==(13,37) and offer_status(nonqb,13)
newchecks.append('The same recovery path preserves valid outside-finalist offers at HB, WR, DT, CB, K and P')

reset_recovery()
viable,vp=rr(5003,offers=(11,13))
before=values(viable)
assert run_recovery()==0 and records[viable]['scalar'][9]==3 and values(viable)==before and not native_events
newchecks.append('One viable current finalist preserves the entire native race and interest list')

reset_recovery()
user,up=rr(5004,offers=(10,));fill(10)
records[allteams[10]]['ref'][407]=record('eligible user')
assert run_recovery()==0 and records[user]['scalar'][9]==3 and offer_status(user,10)
# A user outside the finalist set is preserved on reopening too.
reset_recovery()
userlate,ulp=rr(5005,offers=(13,));fill(13)
records[allteams[13]]['ref'][407]=record('late user')
assert run_recovery()==0 and records[userlate]['scalar'][9]==0
assert values(userlate)[0]==(13,37) and offer_status(userlate,13)
newchecks.append('Viable user finalist is preserved despite CPU room limits; rank-locked user offer/influence survive a legitimate reopening')

reset_recovery()
valid,vp=rr(5006,stage=5,offers=(10,))
assert run_recovery()==0 and records[valid]['scalar'][9]==5 and offer_status(valid,10)
reset_recovery()
orphan,op=rr(5007,stage=5)
assert run_recovery()==0 and records[orphan]['scalar'][9]==0 and all(inf==0 for _,inf in values(orphan))
newchecks.append('Offer-backed hard commitment remains secured; orphan hard commitment with no accepted owner reopens')

reset_recovery()
signed,sp2=rr(5008,stage=6)
hs,hp2=rr(5009,stage=3,portal=False)
before=(values(signed),values(hs))
assert run_recovery()==0 and records[signed]['scalar'][9]==6 and records[hs]['scalar'][9]==3
assert before==(values(signed),values(hs))
newchecks.append('Signed transfers and high-school recruits are outside recovery scope')

for academy in (0,7,55):
 reset_recovery()
 academy_rec,ap=rr(5010+academy,stage=5,schools=((academy,250),(10,100)),offers=(academy,))
 assert run_recovery()==0 and records[academy_rec]['scalar'][9]==0
 assert values(academy_rec)==[(academy,0),(10,0)]
 assert not offer_status(academy_rec,academy) and records[academy_rec]['scalar'][14]==0
 assert any(e[0]=='erase_target' for e in native_events)
newchecks.append('Army/Navy/Air Force orphan hard commitments reopen before native removal, so native own-hard protection cannot preserve their prohibited offers')

reset_recovery()
opening,openp=rr(5070,stage=1,offers=())
assert run_recovery(grace=1)==0 and records[opening]['scalar'][9]==1 and values(opening)[0][1]==99
assert run_recovery(grace=0)==0 and records[opening]['scalar'][9]==0
reset_recovery()
opening_orphan,oop=rr(5071,stage=5,offers=())
assert run_recovery(grace=1)==0 and records[opening_orphan]['scalar'][9]==0
newchecks.append('Opening-week Top5 seed with zero offers is preserved; ordinary recovery starts after grace; orphan hard commitments remain repairable during grace')

for narrowed_stage in (1,2,3,4):
 reset_recovery()
 closed,clp=rr(5200+narrowed_stage,stage=narrowed_stage,offers=())
 if narrowed_stage<=2:
  assert run_recovery(grace=1)==0 and records[closed]['scalar'][9]==narrowed_stage
 assert run_recovery()==0 and records[closed]['scalar'][9]==0
 assert all(inf==0 for _,inf in values(closed))
newchecks.append('Top5, Top3, Battle and SoftCommit all recover when no viable finalist remains; opening grace covers both seeded Top5 and Top3')

reset_recovery()
planned,pp2=rr(5072)
malformed,mp=rr(5073,schools=((10,99),(10,97)))
before=(values(planned),values(malformed))
assert run_recovery()==1
assert records[planned]['scalar'][9]==3 and records[malformed]['scalar'][9]==3
assert before==(values(planned),values(malformed)) and not native_events
newchecks.append('Malformed duplicate school in a later plan disables recovery before any earlier recruit/offer/influence is changed')

reset_recovery()
ordinary,orp=rr(5077)
published_academy,pap=rr(5078,stage=5,schools=((7,250),(10,100)),offers=(7,))
records[records[allteams[7]]['ref'][33]]['array']=[pap]
before=(values(ordinary),values(published_academy))
assert run_recovery()==1
assert records[ordinary]['scalar'][9]==3 and records[published_academy]['scalar'][9]==5
assert before==(values(ordinary),values(published_academy)) and offer_status(published_academy,7)
assert not native_events
newchecks.append('Already-published academy acquisition fails preflight without reopening a duplicate ownership path or changing earlier planned recruits')

reset_recovery()
missing_manager,mmp=rr(5079,offers=(10,));fill(10)
before=values(missing_manager);wq(context+0xc8,0)
assert run_recovery()==1 and records[missing_manager]['scalar'][9]==3
assert values(missing_manager)==before and offer_status(missing_manager,10) and not native_events
wq(context+0xc8,manager)
newchecks.append('Missing native recruiting manager refuses an offer-removal plan before its stage or interest can change')

reset_recovery()
duplicate_offer,dop=rr(5080,offers=(10,));fill(10)
records[board_targets(allteams[10])]['array'].append(record('duplicate matching scholarship',{12:3},ref={10:duplicate_offer}))
before=values(duplicate_offer)
assert run_recovery()==1 and records[duplicate_offer]['scalar'][9]==3
assert values(duplicate_offer)==before and len(records[board_targets(allteams[10])]['array'])==2
assert not native_events
newchecks.append('Duplicate matching scholarship targets are rejected during preflight, before native removal can partially change a board')

for wanted in (0,-1):
 reset_recovery()
 diagnostic,dip=rr(5074+wanted,offers=(10,));fill(10)
 before=values(diagnostic);configure(wanted)
 assert run_recovery()==0 and records[diagnostic]['scalar'][9]==3
 assert values(diagnostic)==before and offer_status(diagnostic,10) and not native_events
newchecks.append('Diagnostic and disabled modes do not change stages, interest, offers, or boards')

# Exercise the actual Prepare hook, including once-per-week lifecycle scope.
# Use a player-controlled school: maintenance runs first, and the existing user
# exclusion then avoids unrelated CPU quality-score preparation.
reset_recovery()
records[team]['ref'][407]=record('maintenance user')
wq(frame+2*8,context);wq(frame+3*8,team_ref);uc.mem_write(sp+0x42,b'\1')
records[season]['scalar'].update({2:2030,4:6,5:6})
call(hooks[8],frame) # Allocate the persistent TLS score cache outside scratch reuse.
assert call(exports['DriTestQualityFault'])==0
# The cleanup authorization is persistent TLS too; reserve its zeroed fixture
# block outside the transient arena (DriTestConfigure assigns index+2).
if 9 not in tls:tls[9]=alloc(64)
def run_prepare():
 global next_heap
 before=next_heap;call(hooks[8],frame);end=next_heap
 if end>before:uc.mem_write(before,bytes(end-before))
 next_heap=before
 assert call(exports['DriTestQualityFault'])==0
lifecycle,lcp=rr(5300)
records[season]['scalar'][4]=3;run_prepare()
assert records[lifecycle]['scalar'][9]==0
records[lifecycle]['scalar'][9]=3;run_prepare()
assert records[lifecycle]['scalar'][9]==3
records[season]['scalar'][4]=4;run_prepare()
assert records[lifecycle]['scalar'][9]==0
records[lifecycle]['scalar'][9]=3;records[season]['scalar'][2]=2031;run_prepare()
assert records[lifecycle]['scalar'][9]==0
records[lifecycle]['scalar'][9]=3;records[season]['scalar'].update({4:3,5:1});run_prepare()
assert records[lifecycle]['scalar'][9]==3
records[season]['scalar'].update({4:6,5:6});run_prepare()
assert records[lifecycle]['scalar'][9]==3
records[season]['scalar'].update({4:3,5:6});configure(0);run_prepare()
assert records[lifecycle]['scalar'][9]==3
configure(1);run_prepare();assert records[lifecycle]['scalar'][9]==0
newchecks.append('Actual Prepare callback runs maintenance once per franchise/year/week/mode, before user exclusion; same-week reentry does not reopen again; regular season and postportal week6 are excluded')

# The post-weekly boundary must catch races exhausted after board preparation,
# including the final portal transition. It must not clear generated seeds
# before the first actual planning pass has run in this campaign.
weekly_frame=STACK+0x1e000;weekly_bp=STACK+0x1e300
wq(weekly_frame+10*8,weekly_bp);wq(weekly_bp+0x60,context)
def run_postweekly():
 global next_heap
 before=next_heap;call(hooks[20],weekly_frame);end=next_heap
 if end>before:uc.mem_write(before,bytes(end-before))
 next_heap=before
 assert call(exports['DriTestQualityFault'])==0
reset_recovery();records[team]['ref'][407]=record('opening lifecycle user')
records[season]['scalar'].update({2:2032,4:3,5:6})
seed5,s5p=rr(5400,stage=1)
seed3,s3p=rr(5401,stage=2)
seed_battle,sbp=rr(5402,stage=3)
before=(values(seed5),values(seed3),values(seed_battle))
run_postweekly()
assert [records[r]['scalar'][9] for r in (seed5,seed3,seed_battle)]==[1,2,3]
assert before==(values(seed5),values(seed3),values(seed_battle))
run_prepare()
assert records[seed5]['scalar'][9]==1 and records[seed3]['scalar'][9]==2
assert records[seed_battle]['scalar'][9]==0
run_postweekly()
assert records[seed5]['scalar'][9]==0 and records[seed3]['scalar'][9]==0
newchecks.append('Postweekly seed generation is preserved before any ACTIVE Prepare; first observed week3 planning grants Top5/Top3 grace, then that same campaign receives no extra grace at the postweekly boundary')

reset_recovery();records[team]['ref'][407]=record('week2 lifecycle user')
records[season]['scalar'].update({2:2033,4:2,5:6})
week2seed,w2p=rr(5403,stage=1)
run_prepare();assert records[week2seed]['scalar'][9]==1
records[season]['scalar'][4]=3
run_prepare();assert records[week2seed]['scalar'][9]==0
newchecks.append('Opening grace applies only to the first observed pass: a week2 seed still without offers reopens at week3 preparation')

reset_recovery();records[team]['ref'][407]=record('late exhaustion user')
records[season]['scalar'].update({2:2034,4:4,5:6})
late,latep=rr(5404,offers=(10,))
secured,secp=rr(5405,stage=5,schools=((11,250),(10,100)),offers=(11,))
run_prepare()
assert records[late]['scalar'][9]==3 and records[secured]['scalar'][9]==5
fill(10)
run_prepare();assert records[late]['scalar'][9]==3 # Same-week maintenance is scoped.
run_postweekly()
assert records[late]['scalar'][9]==0 and not offer_status(late,10)
assert records[secured]['scalar'][9]==5 and offer_status(secured,11)
before=(values(late),values(secured));run_postweekly()
assert before==(values(late),values(secured)) and records[secured]['scalar'][9]==5
final_race,frp=rr(5406,schools=((12,99),(10,97),(11,50)),offers=(12,))
records[season]['scalar'][4]=5;run_prepare()
assert records[final_race]['scalar'][9]==3
fill(12);records[season]['scalar'][4]=6
run_postweekly()
assert records[final_race]['scalar'][9]==0 and not offer_status(final_race,12)
assert records[secured]['scalar'][9]==5 and offer_status(secured,11)
newchecks.append('Postweekly recovery catches finalists exhausted after same-week preparation, preserves valid hard ownership, is inert on an already-open race, and still runs on the final portal transition')

call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
newchecks.append('All fixture references balanced after recovery, preservation, diagnostics and failure paths')
(OUTPUT/'recovery_runtime_results.json').write_text(json.dumps({'checks':newchecks,'method':'compiled Windows recovery callback plus captured native rank/lockout/removal, controlled SDF and Windows services','live_game_verified':False},indent=2)+'\n')
print(json.dumps(newchecks,indent=2))
