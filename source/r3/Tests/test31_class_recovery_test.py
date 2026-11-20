"""Compiled R2 ordinary class recovery; native eligibility/scoring/storage
services are controlled, not claims of native spending or live signings.
Academy source players and class totals come from the supplied R1 NSD save.
"""
from pathlib import Path
import gzip,json
src=(Path(__file__).parent/'test28r14_offseason_test.py').read_text()
exec(compile(src.split('for rid,tid,pos,ovr in [(2991')[0],'offseason fixture','exec'))
source=ROOT.parent/'Audit/Test29R2/Inputs/r1-nsd-rosters.json.gz'
saved=json.loads(gzip.decompress(source.read_bytes()))
position_names='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
star_names=['ONE_STAR','TWO_STAR','THREE_STAR','FOUR_STAR','FIVE_STAR']
scores={};score_calls=[];checks=[]
def class_score():
 scorer,selection,portal,tr=args();candidate=rq(reg('RSP')+0x30);r=rq(candidate)
 assert scorer==fixture_hs_scorer and not selection and not portal
 score_calls.append(ri(r+0x18));wi(candidate+40,scores.get(ri(r+0x18),0));return 0
game_api(0x08185840,class_score)
def fresh(tid):
 reset29();call(exports['DriTestHs32Reset']);adds.clear();scores.clear();score_calls.clear()
 records[season]['scalar'].update({2:2027,4:5,5:6})
 allowed8.clear();allowed8.add(tid)
 for t in allteams:records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:0})
 t=allteams[tid];records[records[t]['ref'][224]]['scalar'][3]=800
 records[tuning]['ref'][49]=record('class actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 return t
def seed():
 call(exports['DriTestHsCoverageBegin'],context)
 assert call(exports['DriTestQualityFault'])==0
def candidate(rid,tid,pos=0,ovr=68,points=100):
 r,p=make(rid,((tid,points),),[],ovr=ovr);records[p]['scalar'].update({3:pos,231:2});scores[rid]=400
 return r,p

# Early normal intake may enter a full returning room; actual annual arrivals
# close that same slot. Every prior native affordability/eligibility guard stays.
for blocked in ['none','eligibility','score','hours','preference','user','annual_filled','full_returning_room','outside_window']:
 t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
 r,p=candidate(131300,10)
 if blocked=='eligibility':allowed8.clear()
 if blocked=='score':scores[131300]=0
 if blocked=='hours':records[records[t]['ref'][224]]['scalar'][3]=9
 if blocked=='preference':
  top=records[r]['ref'][13];records[top]['array']=[record('established leader',{0:11,1:900}),record('second choice',{0:12,1:800}),record('third choice',{0:13,1:700}),record('distant school',{0:10,1:100})];records[r]['scalar'][9]=3
 if blocked=='user':records[t]['ref'][407]=record('user coach')
 if blocked=='annual_filled':
  signed,sp=candidate(131301,10);records[signed]['scalar'][9]=5
 if blocked=='full_returning_room':records[records[t]['ref'][239]]['array']=[pp('existing QB',70,pos=0,tid=10) for _ in range(5)]
 if blocked=='outside_window':records[season]['scalar'].update({4:4,5:2})
 before=snap8(r);budget=dict(records[records[t]['ref'][224]]['scalar']);seed()
 want=blocked in ['none','full_returning_room']
 assert bool(call(exports['DriTestHsRecoveryPending'],r,context))==want,blocked
 assert snap8(r)==before and records[records[t]['ref'][224]]['scalar']==budget
 if want:
  call(exports['DriTestHs32Begin'],context)
  assert call(exports['DriTestHs30AdmissionBlock'],r,t,context)==0
  call(exports['DriTestHs32Offseason'],t,context)
  assert adds==[(10,131300)] and not offer_status(r,10)
  target=records[board_targets(t)]['array'][0]
  assert records[target]['scalar'].get(12,0)==0
 checks.append({'case':blocked,'reserved_and_admitted':want,'native_offer_created':False})
# PRE is a live normal-source boundary, not a late completion intervention.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4;records[season]['scalar'].update({4:0,5:0})
r,p=candidate(131400,10);before=snap8(r);seed()
assert call(exports['DriTestHsRecoveryPending'],r,context)>0 and snap8(r)==before
checks.append('PRE ordinary class search uses native-positive eligibility and creates a reservation without offer/hours fabrication')
# Only two prospects per exact position reach native scoring, then the weekly
# frozen catalogue is reused. One normal QB slot cannot become two primaries.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=4
for i in range(30):candidate(131500+i,10,ovr=50+i%20)
seed();first=len(score_calls);seed();assert len(score_calls)==first
assert len(set(score_calls))<=2,len(set(score_calls))
assert sum(call(exports['DriTestHsRecoveryPending'],r,context)>0 for r in records[hslist]['array'])==1
checks.append('Thirty ordinary QB candidates narrow to two native score checks, one normal primary, and no repeated same-week sweep')
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test31_class_recovery_results.json').write_text(json.dumps({'checks':checks,'passed':True,'native_action_execution':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
