"""Compiled Test02 callbacks with native ownership/removal fixtures. Not a live game."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
prepare_cb,score_cb,quota_cb,select_cb,fb_start_cb,fb_cb=hooks[8:14]
newchecks=[]
configure(1)
records[team]['scalar'][384]=9
records[tplist]['array']=[];records[hslist]['array']=[];records[committed]['array']=[];records[arr]['array']=[]
oregon=[pp('Akili',74,0),pp('Brock',73,2),pp('Dayton',69,0)]
moore=pp('Dante - confirmed NFL but TeamIndex retained',95,2)
leaver=record('final draft entrant',{1:0,2:1},ref={4:moore});records[departures]['array']=[leaver]
records[roster]['array']=oregon+[moore]
h,h_t,h_p=pr(3000,71,0,stage=6,portal=False);records[committed]['array']=[h_p]
c,c_t,c_p=pr(3001,83);c2,c2_t,c2_p=pr(3002,91)
qout=alloc(1584)
assert call(exports['DriTestQualityRoom'],team,context,0,qout)==1
qvalues=struct.unpack('<394i',uc.mem_read(qout,1576))
assert qvalues[:6]==(0,4,1,4,4,-1),qvalues[:6]
assert qvalues[6:10]==(74,73,71,69),qvalues[6:10]
assert decide(c)==0
newchecks.append('Oregon: confirmed NFL player excluded despite matching TeamIndex; committed HS deduplicated; 74/73/71/69 room allows 83')
# Test30: this scoring component must use a real portal calendar and an
# affordable, native-positive proposal. Inherited fixtures were regular W4,
# which cannot authorize portal funding. Only the scorer's input fit is
# modeled; the actual global market and hard authorization execute unchanged.
_quality_season=dict(records[season]['scalar'])
_quality_budget=dict(records[board]['scalar'])
_quality_actions=records[tuning]['ref'].get(49)
_quality_scorer=handlers[GAME+0x08185840]
records[season]['scalar'].update({4:3,5:6})
records[board]['scalar'].update({1:0,2:0,3:100})
records[tuning]['ref'][49]=record('quality component actions',array=[
 record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
records[c]['scalar'][0]=1;records[c2]['scalar'][0]=1
_quality_subject=c
def quality_component_fit():
 scorer,selection,portal,tr=args()
 if not (portal&255):return _quality_scorer()
 assert scorer==fixture_hs_scorer and (selection&255,portal&255) in [(1,1),(0,1)]
 cand=rq(reg('RSP')+0x30)
 wi(cand+40,522 if rq(cand)==_quality_subject else 0);return 0
game_api(0x08185840,quality_component_fit)
# This component scores an existing legitimate offered pursuit; native target
# creation is tested separately against the captured AddToBoard routine.
valid_offer(c,c_t)
call(exports['DriTestMarket29PortalReset'])
# Captured prepare point: R13 eval and R12 Team-reference, before candidate enumeration.
wq(frame+2*8,context);wq(frame+3*8,team_ref);uc.mem_write(sp+0x42,b'\1')
call(prepare_cb,frame);assert call(exports['DriTestQualityFault'])==0
assert call(exports['DriTestPortal30AdmissionBlock'],c,team,context)==0
sf=STACK+0x18000;ss=sf+128
wq(sf,candidate);wq(candidate,c);wq(ss+0xa8,team_ref);wq(ss+0x88,GAME+0x0818c282)
def score(base,rec=c,caller=0x0818c282):
 wq(candidate,rec);wq(sf+1*8,base&0xffffffffffffffff);wq(sf+3*8,0);wq(ss+0x88,GAME+caller)
 call(score_cb,sf);return rq(sf+1*8)&0xffffffff
assert score(522)==3000857
assert score(522,caller=0x081733fa)==857
# Restore the inherited calendar/services after the authorized funding score.
records[season]['scalar']=_quality_season
records[board]['scalar']=_quality_budget
if _quality_actions is None:records[tuning]['ref'].pop(49,None)
else:records[tuning]['ref'][49]=_quality_actions
game_api(0x08185840,_quality_scorer)
assert score(0)==0 and score(-10)==0xfffffff6
assert score(522,caller=0x08103d10)==522
# Quota augmentation is confined to the current team/evaluator.
qb=STACK+0x1b000;wq(sf+10*8,qb);wq(qb+0x38,team_ref);wq(qb+0x30,context);wq(sf+7*8,0);wq(sf+13*8,0)
call(quota_cb,sf);assert rq(sf+13*8)==12000
wq(qb+0x30,0);wq(sf+13*8,0);call(quota_cb,sf);assert rq(sf+13*8)==0
newchecks.append('Actual prepare/score/quota callbacks: projected quality bonus; transient starter tier at fresh shortlist site only; complete bounded eligible pool; native nonpositive scores and unrelated callers preserved')
for m in [0,-1]:
 configure(m);assert score(522)==522
configure(1)
# Same-advance hard commitment reaches the unchanged position overage cap.
records[c]['scalar'][9]=5
valid_offer(c,c_t)
assert decide(c2)==4
records[arr]['array']=[];assert c_p not in records[committed]['array']
newchecks.append('Same-advance offer-backed hard commitment is remembered: count 5/base 4 reaches the cap and blocks even a 91 second QB before roster publication')
# Primary native selector: first school is vetoed, native loop can continue.
pb=STACK+0x1d000;wq(sf+10*8,pb);wq(pb+0x67,context);rr=alloc(8);wq(rr,c2);wq(pb+0x77,rr);wq(pb-0x39,team);wq(sf+14*8,1)
call(select_cb,sf);assert rq(sf+14*8)==0
# Existing hard commitments never redirected.
wq(rr,c);wq(sf+14*8,1);call(select_cb,sf);assert rq(sf+14*8)==1
# Offer fallback captures evaluator at entry, filters before taking a Team reference.
wq(sf+13*8,context);call(fb_start_cb,sf)
fa=alloc(16);wq(fa,team);wq(pb-0x69,fa);wq(pb-0x21,c2);wq(sf+9*8,0)
assert call(fb_cb,sf)==1
records[c]['scalar'][9]=1;assert call(fb_cb,sf)==0
newchecks.append('Primary destination filter and offered-team fallback reject redundant destination; allow it when opening exists; secured commitment untouched')
# Backup fit uses second HB rather than requiring candidate to beat HB1.
records[departures]['array']=[];records[tplist]['array']=[];records[hslist]['array']=[];records[committed]['array']=[]
records[ideal_entries[1]]['scalar'].update({2:6,4:1});records[team]['scalar'][384]=8
records[roster]['array']=[pp('HB'+str(i),v,pos=1) for i,v in enumerate([87,76,75,73,69,65])]
b,b_t,b_p=pr(3003,84,pos=1);assert decide(b)==0
records[b_p]['scalar'][191]=78;assert decide(b)==0
records[b_p]['scalar'][191]=77;assert decide(b)==1
newchecks.append('Missouri: 84 improves HB2 and 78 improves HB3 by three; marginal 77 rejected')
# A secured starter can coexist with a genuine lower backup vacancy.
records[tplist]['array']=[];records[roster]['array']=[pp('QB backup',70)]
records[team]['scalar'][384]=9
starter,starter_t,starter_p=pr(3004,88,stage=5)
call(prepare_cb,frame)
wq(sf+10*8,qb);wq(qb+0x38,team_ref);wq(qb+0x30,context);wq(sf+7*8,0);wq(sf+13*8,0)
call(quota_cb,sf);assert rq(sf+13*8)==12000
newchecks.append('After a starter signs, a genuine lower QB backup vacancy still receives quality shortlist space')
# User-team exclusions apply to preparation, scoring and both selectors.
records[team]['ref'][407]=record('user character 2');call(prepare_cb,frame);assert score(522)==522;assert decide(b)==0
records[team]['ref'].pop(407)
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
newchecks.append('User team untouched; all fixture references balanced; no runtime faults')
(OUTPUT/'quality_runtime_results.json').write_text(json.dumps({'checks':newchecks,'live_game_verified':False},indent=2)+'\n');print(json.dumps(newchecks,indent=2))
