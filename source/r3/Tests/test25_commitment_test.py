"""Production acceptance gate, Windows x64 emulation and native rank lockout.
SDF, Windows services and native dealbreaker result are controlled fixtures.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
import ctypes as C
class Choice(C.Structure):
 _fields_=[(k,C.c_int) for k in ['pos','role','score','deficit','existing','round','selected','index']]+[('rid',C.c_uint),('rec',C.c_uint64)]+[(k,C.c_int) for k in ['ovr','slot','priority','native_base','plan_score','offered','influence','leader','commit_score','stage','race_ok','raw_ovr']]
configure(1);call(exports['DriTestCommitReset']);wq(context+0xd0,tuning)
records[season]['scalar'].update({2:2040,4:3,5:6});records[team]['scalar'][384]=9
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
for e,v in zip(ideal_entries,ideals):records[e]['scalar'].update({2:v,4:1})
records[ideal_entries[0]]['scalar'][2]=4
records[roster]['array']=[pp('incumbent1',80),pp('incumbent2',70)]
primary,pt,pp1=pr(45000,88,stage=2);backup,bt,bp1=pr(45001,75,stage=2)
other,ot,op1=pr(45002,78,stage=2);otherfallback,oft,ofp=pr(45003,71,stage=2)
unplanned,ut,up=pr(45004,73,stage=2)
for r,t in [(primary,pt),(backup,bt),(other,ot),(otherfallback,oft),(unplanned,ut)]:valid_offer(r,t)
def interest(r,own,others):
 top=records[r]['ref'][13];rows=[(TEST_TEAM_ID,own)]+others;rows.sort(key=lambda x:-x[1]);records[top]['array']=[record('rank',{0:tid,1:v})for tid,v in rows]
 records[r]['scalar'][3]=160
interest(primary,150,[(10,180)]);interest(backup,170,[(10,10)])
interest(other,170,[(10,10)]);interest(otherfallback,170,[(10,10)])
interest(unplanned,170,[(10,10)])
eligible=True
game_api(0x0816b130,lambda:int(eligible))
pool=(Choice*4)()
for i,(rec,slot,round_) in enumerate([(primary,1,1),(backup,1,2),(other,2,1),(otherfallback,2,2)]):
 pool[i].rec=rec;pool[i].rid=records[rec]['scalar'].get('rid',ri(rec+0x18));pool[i].selected=1;pool[i].pos=0;pool[i].slot=slot;pool[i].round=round_;pool[i].priority=i+1
addr=alloc(C.sizeof(pool));uc.mem_write(addr,bytes(pool))
def store():call(exports['DriTestCommitPlan'],addr,4,team,context)
def block(r,deadline=0):return call(exports['DriTestCommitBlock'],r,team,context,deadline)
store();assert block(backup)==9 and block(other)==0 and block(primary)==0
assert block(unplanned)==9
checks=['A close rank-two first choice protects only its own opening; another primary remains legal; unplanned leftovers cannot steal both reserved places']
# The actual weekly/instant/NSD acceptance hooks all consume the same plan.
wq(ps+0x38,transition);records[transition]['scalar'][1]=5;wq(pb+0x68,backup);wq(pb+0x60,context);wq(pf+14*8,1);call(weekly_cb,pf);assert rq(pf+14*8)==0
wq(pb-0x41,backup);wq(pb+0x77,tr);wq(pb+0x67,context);assert call(instant_cb,pf)==9
# At deadline, the losing or below-threshold priority cannot hold a ready fallback.
assert block(backup,1)==0
interest(primary,190,[(10,180)]);assert block(backup,1)==9
wq(ps+0x30,backup);wq(pf+11*8,team);wq(pb+0x57,context);assert call(sign_cb,pf)==0
wq(pf,team);wq(pb+0x40,context);assert call(deadline_cb,pf)==0
# Filter the destination before selection as well as vetoing its final signing.
sel=STACK+0x43000;selbp=sel+0x200;rr=alloc(8);wq(rr,backup)
wq(sel+10*8,selbp);wq(selbp+0x67,context);wq(selbp+0x77,rr);wq(selbp-0x39,team);wq(sel+14*8,1)
call(hooks[11],sel);assert rq(sel+14*8)==1
wq(sel+13*8,context);wq(sel+7*8,0);call(hooks[12],sel)
fa=alloc(8);wq(fa,team);wq(selbp-0x69,fa);wq(selbp-0x21,backup);wq(sel+9*8,0)
assert call(hooks[13],sel)==0
checks.append('NSD destination filters release priority holds while retaining roster admission')
checks.append('Weekly and instant callbacks retain reservations; board and portal NSD signings release them')
interest(primary,60,[(10,180)]);assert block(backup)==0
interest(primary,150,[(10,180),(11,170),(12,160)]);records[primary]['scalar'][9]=0;assert block(backup)==0
interest(primary,150,[(10,180)]);records[primary]['scalar'][9]=2
eligible=False;assert block(backup)==0;eligible=True
records[pt]['scalar'][12]=1;assert block(backup)==0;records[pt]['scalar'][12]=3
checks.append('Hopeless deficit, rank four, failed native eligibility or withdrawn offer each promote the fallback immediately')
# A real offer-backed commitment to a different school releases the opening.
records[primary]['scalar'][9]=5;interest(primary,150,[(10,180)])
valid_offer(primary,destination=fixture_dummy_teams[10]);assert block(backup)==0
records[primary]['scalar'][9]=2
foreign_board=records[fixture_dummy_teams[10]]['ref'][224]
records[records[foreign_board]['ref'][0]]['array']=[]
checks.append('An offer-backed hard commitment elsewhere immediately releases the fallback')
# Same-opening choices cannot use two distinct QB openings in one advance.
interest(other,170,[(10,10)]);records[other]['scalar'][9]=5
assert block(otherfallback)==10 and block(backup)==9
interest(primary,30,[(10,180)]);assert block(backup)==0
assert block(otherfallback)==0 # Separate vacancy is now unreserved.
interest(primary,150,[(10,180)])
records[other]['scalar'][9]=6;records[committed]['array']=[op1]
assert block(otherfallback)==10
checks.append('A hard commitment and its published Signed successor close their own opening; its fallback cannot consume another protected opening')
# An absent or stale plan never vetoes native behavior. Same team ID is not identity.
records[season]['scalar'][4]=5;assert block(otherfallback)==0
records[season]['scalar'][4]=3;records[season]['scalar'][2]=2041;assert block(otherfallback)==0
records[season]['scalar'][2]=2040;records[team]['ref'][407]=record('user');assert block(otherfallback)==0;records[team]['ref'].pop(407)
for mode in [0,-1]:configure(mode);assert block(otherfallback)==0
configure(1);call(exports['DriTestCommitReset']);assert block(otherfallback)==0
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values());assert call(exports['DriTestQualityFault'])==0
checks.append('User/diagnostic/disabled/no-plan/year-change/week-expiry pass through; all references balanced and no runtime fault')
(OUTPUT/'test25_commitment_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
