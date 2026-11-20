"""Compiled R5 callbacks with saved-shaped rooms; native scores/eligibility controlled.
Tests planning, acceptance, pruning and NSD selection, not live signing/funding.
"""
from pathlib import Path
src=(Path(__file__).parent/'test29_class_source_test.py').read_text()
exec(compile(src.split('reset29();call(exports[\'DriTestHs32Reset\'])')[0],'class fixture','exec'))
checks=[]
def prep(tid):
 reset29();call(exports['DriTestHs32Reset']);scores.clear()
 records[season]['scalar'].update({4:2,5:1});records[tuning]['scalar'][39]=0
 for entry,ideal in zip(ideal_entries,ideals):records[entry]['scalar'].update({0:0,2:ideal,4:1})
 # Isolate the quality exception from the pre-existing83/84-to85 flex rule.
 records[ideal_entries[3]]['scalar'][2]+=11
 records[tuning]['ref'][49]=record('actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
 t=allteams[tid];records[t]['scalar'][384]=9
 records[records[t]['ref'][224]]['scalar'].update({1:0,2:0,3:800})
 # The historic quality-exception controls require the one normal MLB seat
 # already filled. Pick a deterministic calendar year with that exact quota.
 records[ideal_entries[14]]['scalar'][2]=4
 annual_out=alloc(46*4)
 for year in range(2026,2066):
  records[season]['scalar'][2]=year
  assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
  if ri(annual_out+15*4)==1:break
 else:raise AssertionError('No single-MLB annual fixture year')
 return t

def prospect(rid,tid,pos,ovr,stars,score,stage=0,points=300):
 r,p=make(rid,((tid,points),),[tid],ovr,stage)
 records[p]['scalar'].update({3:pos,231:stars});scores[rid]=score
 return r,p

def roster(t,pos,ratings,years=None):
 a=records[t]['ref'][239];tid=records[t]['scalar'][383]
 for i,rating in enumerate(ratings):
  p=pp('saved-shaped returner',rating,pos=pos,tid=tid)
  records[p]['scalar'][242]=years[i] if years else 1
  records[a]['array'].append(p)

def role(r,t):return call(exports['DriTestHsPlanRole'],r,t,context,0)
def unchanged(r):return snap8(r)

# Miami saved W1 ability/fit comparison, including a prior primary.
t=prep(47);records[ideal_entries[14]]['scalar'][2]=4;roster(t,14,[75,72,68])
old,op=prospect(1790,47,14,73,3,420);assert role(old,t)==1
r,p=prospect(1655,47,14,80,4,395);before=unchanged(r)
assert role(r,t)==1 and role(old,t)==2
assert unchanged(r)==before and scores=={1790:420,1655:395}
checks.append('Miami W1-shaped80/395 challenger replaces73/420 uncommitted primary; old primary remains conditional; points, stages and scores untouched')

# An actual commitment fills the ordinary room. The upgrade must acquire
# an optional place, with prospective shared budget and all acceptance gates.
records[old]['scalar'][9]=5
call(exports['DriTest29ClassInvalidate'],t)
assert call(exports['DriTestHsDecision'],r,t,context)==0
assert role(r,t)==1 and call(exports['DriTestHsPlanRole'],r,t,context,2)==14*32+31
assert call(exports['DriTestHsPairDecision'],r,t,context,0)==0
assert call(exports['DriTestHs29Offer'],r,t,context)==2
before=unchanged(r)
assert call(exports['DriTestHsPairPrune'],t,context,1)==1
assert offer_status(r,47) and unchanged(r)==before
# Run the earlier real board-evaluation entry, where coarse position closure
# formerly erased this legal candidate before the pair planner could run.
ctx5=alloc(0xc00);uc.mem_write(ctx5,bytes(uc.mem_read(context,0x300)))
team_ref5=alloc(8);wq(team_ref5,t);need5=alloc(88)
af5=STACK+0x23000;sp5=af5+128;bp5=sp5+0x200
wq(af5+10*8,bp5);wq(af5+3*8,team_ref5)
wq(bp5+0x28,team_ref5);wq(bp5+0x58,season);wq(bp5+0x180,need5)
wq(sp5+0x58,ctx5);uc.mem_write(sp5+0x42,b'\0')
for pos in range(22):wi(ctx5+0x9a4+4*pos,1);wi(need5+4*pos,0)
call(start,af5);assert call(exports['DriTestQualityFault'])==0
assert offer_status(r,47) and unchanged(r)==before
call(exports['DriTestNsdReset']);win=call(exports['DriTestNsdWinner'],r,context,0)
assert win==t;decref(win)
assert unchanged(r)==before
checks.append('After primary commitment, full-room material elite earns optional479; acceptance, offer firmness, initial coarse cleanup, closed-only pruning and NSD offered-school selection agree; no owner or offer manufactured')
# Reload must reconstruct the same permission without increasing allowance.
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHs29Reset'])
assert call(exports['DriTestHsDecision'],r,t,context)==0 and role(r,t)==1
# One secured quality addition ends this exception for the now-overfull room.
records[r]['scalar'][9]=5
next_r,np=prospect(1656,47,14,85,3,500)
assert call(exports['DriTestHsDecision'],next_r,t,context)!=0
assert role(next_r,t)==0
checks.append('Reload reconstructs room/budget; secured extra ends further full-room additions')

# Full room rejection controls, including shared allowance exhausted elsewhere.
for reason in ['equal','small_upgrade','nonelite','deep_only','five_star_used','overfull','budget','native_ineligible','user']:
 t=prep(47);records[ideal_entries[14]]['scalar'][2]=4;roster(t,14,[75,72,68])
 old,op=prospect(1790,47,14,73,3,455,5)
 r,p=prospect(1655,47,14,80,4,470)
 if reason=='equal':records[p]['scalar'][191]=73
 if reason=='small_upgrade':records[p]['scalar'][191]=76
 if reason=='nonelite':records[p]['scalar'][231]=2
 if reason=='deep_only':
  records[records[t]['ref'][239]]['array']=[];roster(t,14,[90,89,88])
 if reason=='five_star_used':records[op]['scalar'][231]=4
 if reason=='overfull':roster(t,14,[60])
 if reason=='budget':
  for pos in [3,18]:
   records[ideal_entries[pos]]['scalar'][2]=2;roster(t,pos,[70,69])
   annual_out=alloc(46*4);assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
   normal=ri(annual_out+(1+pos)*4)
   for j in range(normal+1):prospect(18000+pos*20+j,47,pos,71,2,400,5)
 if reason=='native_ineligible':allowed8.remove(47)
 if reason=='user':records[t]['ref'][407]=record('user coach')
 if reason=='user':assert call(exports['DriTestHsDecision'],r,t,context)==0
 elif reason=='native_ineligible':assert call(exports['DriTestHs29Offer'],r,t,context)==0
 else:
  decision=call(exports['DriTestHsDecision'],r,t,context)
  if not decision:
   debug=alloc(157*4);call(exports['DriTestHsSnapshot'],t,context,debug)
   data=struct.unpack('<157i',uc.mem_read(debug,157*4))
   print('CONTROL DEBUG',reason,{'ideal':data[14],'count':data[36],'hs':data[80],'four':data[102],'five':data[124],'flex':data[146],'total_ideal':data[156]})
  assert decision!=0,reason
  if reason=='budget':
   assert call(exports['DriTestHsPairPrune'],t,context,1)==1
   assert not offer_status(r,47), 'A quality candidate cannot retain a closed-room offer after the shared allowance is spent'
checks.append('Reject equal/small/nonelite/deep-only, second five-star, inherited overfull and spent shared budget; native eligibility and user bypass retained')
# QB and specialists do not gain this HS quality exception.
for pos in [0,19,20]:
 t=prep(47);records[ideal_entries[pos]]['scalar'][2]=2;roster(t,pos,[70])
 prospect(1800+pos,47,pos,71,3,400,5)
 r,p=prospect(1900+pos,47,pos,82,4,500)
 assert call(exports['DriTestHsDecision'],r,t,context)!=0
checks.append('QB and K/P retain their existing full-room restrictions')

# Georgia exact relevant returning ratings and juniors, scored candidates.
# Controlled surrounding rooms/budget isolate the optional-choice stage.
t=prep(29);records[t]['scalar'][384]=10
for pos,count in [(0,4),(14,4),(12,6),(4,6)]:records[ideal_entries[pos]]['scalar'][2]=count
annual_out=alloc(46*4)
for year in range(2026,2066):
 records[season]['scalar'][2]=year
 assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
 if all(ri(annual_out+(1+p)*4)>0 for p in [0,4,12,14]):break
else:raise AssertionError('No normal-development fixture year')
roster(t,14,[89,85,70,68],[2,1,0,0])
roster(t,12,[75]*8,[2,2,2,1,1,0,0,0]);roster(t,4,[75]*8,[2,2,1,1,1,0,0,0]);roster(t,0,[75]*4)
r,p=prospect(1655,29,14,80,4,480,points=157)
qb,qp=prospect(99999,29,0,69,2,300)
assert role(r,t)==1 and role(qb,t)==1
budget=records[t]['ref'][224];tg=valid_offer(r,destination=t)
records[tg]['scalar'][15]=1;records[budget]['scalar'][1]=5
call(exports['DriTestHsCoverageObserve'],t,context)
assert role(r,t)==1
# Next weekly native reset: source funding must be zero, pursuit may persist.
records[season]['scalar'][4]+=1;records[budget]['scalar'][1]=0
records[tg]['scalar'][15]=0
te,tp=prospect(1945,29,4,76,3,425);dt,dp=prospect(2062,29,12,71,3,475)
assert role(r,t)==1 and role(dt,t)==1 and role(te,t)==1
out=alloc(88);call(exports['DriTest29ClassInvalidate'],t)
assert call(exports['DriTest29HsReserved'],t,context,out)==1
assert sum(struct.unpack('<22i',uc.mem_read(out,88)))==0
call(exports['DriTestHsPlanReset']);call(exports['DriTestHs29Reset'])
assert role(r,t)==1 and role(dt,t)==1 and role(te,t)==1
checks.append('Georgia-shaped W1-to-W2 reset keeps elite MLB, allows DT and TE in their distinct normal annual places without sharing the two discretionary places, claims zero funding after hours reset and reconstructs the same independent annual permissions after plan reset')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestNsdReset'])
assert all(v==1 for v in refs.values()),[(records[k]['name'],v)for k,v in refs.items()if v!=1]
result={'passed':True,'checks':checks,'native_score_and_eligibility':'controlled fixture inputs','native_funding_executed':False,'live_game_verified':False}
(OUTPUT/'test31_elite_runtime_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
