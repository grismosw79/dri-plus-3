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
 records[season]['scalar'].update({4:5,5:6})
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

# Saved-shaped positional exposures: final source counts and all actual
# incoming identities/ratings are loaded. Ideal settings are deliberately
# narrowed to the named observed vacancy; this is not a full-save simulation.
for tid,pos,want_class,want_total,want in [(0,8,13,83,True),(7,5,8,84,True),(55,2,16,86,True)]:
 t=fresh(tid);actual=next(x for x in saved['teams'] if x['tid']==tid)
 records[t]['scalar'][384]=actual['prestige']
 counts=[0]*22;hs_count=0;roster=[]
 for x in actual['roster']:
  pp0=pp(x['name'],x['ovr'],year={'Freshman':0,'Sophomore':1,'Junior':2,'Senior':3}[x['year']],pos=position_names.index(x['position']),tid=tid)
  records[pp0]['scalar'][231]=star_names.index(x['stars']);roster.append(pp0);counts[position_names.index(x['position'])]+=1
 records[records[t]['ref'][239]]['array']=roster
 for x in actual['incoming']:
  is_portal=x['Class'].startswith('Transfer_');r,p=rr(x['rid'],ovr=x['ovr'],stage=6,schools=((tid,1000),),portal=is_portal)
  records[p]['scalar'].update({3:position_names.index(x['position']),231:star_names.index(x['stars']),261:tid,242:0})
  records[records[t]['ref'][33]]['array'].append(p);counts[position_names.index(x['position'])]+=1
  hs_count+=not is_portal
 assert hs_count==want_class and sum(counts)==want_total,(tid,hs_count,sum(counts))
 for p,n in enumerate(counts):records[ideal_entries[p]]['scalar'][2]=n+(p==pos)
 r,p=candidate(129200+tid,tid,pos)
 before=snap8(r);budget=dict(records[records[t]['ref'][224]]['scalar'])
 seed();pending=call(exports['DriTestHsRecoveryPending'],r,context)
 assert bool(pending)==want,(tid,pending)
 assert snap8(r)==before and records[records[t]['ref'][224]]['scalar']==budget
 if want:
  # Test30 native execution requires the real frozen market after reservation.
  call(exports['DriTestHs32Begin'],context)
  assert call(exports['DriTestHs30AdmissionBlock'],r,t,context)==0
  call(exports['DriTestHs32Offseason'],t,context)
  assert adds==[(tid,129200+tid)] and not offer_status(r,tid)
  target=records[board_targets(t)]['array'][0]
  assert call(exports['DriTestHsPlanRole'],r,t,context,0)==1
  assert records[target]['scalar'].get(12,0)==0
  if tid==7:
   donor,dp=candidate(129299,tid,pos,ovr=60);scores[129299]=100
   dt=record('conditional native board target',{12:0},ref={10:donor});records[board_targets(t)]['array'].append(dt)
   vec=alloc(144);indices=alloc(16)
   for k,(rec,tg,weight)in enumerate([(r,target,1),(donor,dt,99)]):
    wq(vec+k*72,rec);wi(vec+k*72+40,weight);wq(vec+k*72+48,tg);wi(vec+k*72+64,0);wq(indices+k*8,k)
   def native_limit():
    ev,tr,targ,index=args();assert ev==context and rq(tr)==t and rq(targ)==target and index==0
    return ri(vec+40)//2
   game_api(0x0817e7b0,native_limit)
   wq(STACK+0xf0000-8+0x28,indices);call(exports['DriTest29R1Funding'],t,context,vec,2)
   assert [ri(vec+40),ri(vec+112)]==[20,80]
   assert records[records[t]['ref'][224]]['scalar']==budget and not offer_status(r,tid)
   checks.append('Same Army-shaped ordinary recruit travels discovery to verified reservation to native board admission to minimum usable native allowance; no scholarship or hours are fabricated')
 checks.append({'saved_school':actual['name'],'HS':hs_count,'projected_total':sum(counts),'position':position_names[pos],'new_primary_reserved_and_admitted':want,'native_signing':False})

# Ordinary schools get the same usable source route; class completion cannot
# override native eligibility, native-positive score, affordability or access.
for blocked in ['none','eligibility','score','hours','preference','user','full_position']:
 t=fresh(10);records[ideal_entries[0]]['scalar'][2]=1
 r,p=candidate(129300,10)
 if blocked=='eligibility':allowed8.clear()
 if blocked=='score':scores[129300]=0
 if blocked=='hours':records[records[t]['ref'][224]]['scalar'][3]=9
 if blocked=='preference':
  top=records[r]['ref'][13];records[top]['array']=[record('established leader',{0:11,1:900}),record('second choice',{0:12,1:800}),record('third choice',{0:13,1:700}),record('distant school',{0:10,1:100})];records[r]['scalar'][9]=3
 if blocked=='user':records[t]['ref'][407]=record('user coach')
 if blocked=='full_position':records[records[t]['ref'][239]]['array']=[pp('existing QB',70,pos=0,tid=10)]
 before=snap8(r);seed()
 assert bool(call(exports['DriTestHsRecoveryPending'],r,context))==(blocked=='none'),blocked
 assert snap8(r)==before
checks.append('Ordinary under15 school gains ordinary native-positive primary discovery; native refusal, zero score, insufficient hours, established preference, full positional room and user control block it without any save mutation')

# The weekly frozen catalogue runs once, with two native-scored candidates
# per position even when many plausible candidates exist in the pool.
t=fresh(10);records[ideal_entries[0]]['scalar'][2]=2
for i in range(30):candidate(129400+i,10,ovr=50+i%20)
seed();first=len(score_calls);seed();assert len(score_calls)==first
assert len(set(score_calls))<=2,len(set(score_calls))
assert sum(call(exports['DriTestHsRecoveryPending'],r,context)>0 for r in records[hslist]['array'])==2
checks.append('Thirty ordinary candidates are narrowed to two native-scored recruits for two genuine openings, both primary rather than one primary plus conditional backup; repeat same-week boundary is inert')
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test29r2_class_recovery_results.json').write_text(json.dumps({'saved_source_sha256':saved['sha256'],'checks':checks,'passed':True,'native_action_execution':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
