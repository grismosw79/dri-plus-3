"""Compiled callbacks + captured native ownership helper, mocked SDF/Windows.
No game process is started by this test.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'runtime_fixture_test.py').read_text(),'runtime_fixture_test.py','exec'))
star_cb,weekly_cb,instant_cb,sign_cb,deadline_cb=hooks[3:8]
checks=[]
# Give the context the same table/row/schema offsets used in captured code.
franchise=record('franchise');context_record=record('context',ref={6:franchise})
context=alloc(0x300);wq(context+0x10,rq(context_record+0x10));wi(context+0x18,ri(context_record+0x18));wq(context+0x30,0xabcdef);wq(context+0xc8,manager)
wq(context+0xf8,fixture_hs_scorer)
wq(context+0xd0,tuning) # native shortlist eligibility reads this evaluator reference
# Ownership validation scans the complete franchise team table. Dummy teams
# are real records with valid empty roster/commit/board arrays, not null slots.
fixture_dummy_teams=[]
for tid in range(138):
 empty_board=record('empty board '+str(tid),ref={0:record('empty targets '+str(tid),array=[])})
 fixture_dummy_teams.append(record('empty team '+str(tid),{383:tid,384:5},ref={33:record('empty commits '+str(tid),array=[]),224:empty_board,239:record('empty roster '+str(tid),array=[]),64:config}))
def fixture_team_array(*extra):
 value=list(fixture_dummy_teams)
 for tr in (team,)+extra:value[records[tr]['scalar'][383]]=tr
 return value
hslist=record('HS class',array=[]);tplist=record('portal class',array=[]);teams=record('teams',array=fixture_team_array());committed=record('committed',array=[])
departures=record('confirmed NFL draft class',array=[]);leaving_eval=record('leaving evaluator',ref={7:departures});franchise_eval=record('franchise evaluator',ref={13:leaving_eval});records[franchise]['ref'][9]=franchise_eval
records[franchise]['ref'].update({39:hslist,46:tplist,45:teams,44:season});records[team]['ref'][33]=committed
for e in ideal_entries:records[e]['scalar'].update({0:0,4:1})
records[ideal_entries[0]]['scalar'][2]=4
records[team]['ref'].pop(407,None)
records[roster]['array']=[];records[arr]['array']=[]
def pp(name,ovr,year=1,pos=0,tid=TEST_TEAM_ID):return record(name,{3:pos,191:ovr,242:year,261:tid,231:4})
targets_by_recruit={}
def valid_offer(rec,target=None,destination=team):
 """Model an actual matching scholarship, not a recruit's aggregate offer count."""
 board_rec=records[destination]['ref'][224]
 target_array=records[board_rec]['ref'][0]
 for existing in records[target_array]['array']:
  if records[existing]['ref'].get(10)==rec:
   records[existing]['scalar'][12]=3
   return existing
 target=target or (targets_by_recruit.get(rec) if destination==team else None) or record('backing scholarship',{12:3},ref={10:rec})
 records[target]['scalar'][12]=3;records[target]['ref'][10]=rec
 records[target_array]['array'].append(target)
 records[rec]['scalar'][14]=max(1,records[rec]['scalar'].get(14,0))
 return target
def pr(rid,ovr,year=1,pos=0,stage=1,portal=True):
 r,t=recruit(rid,pos,1,250,stage=stage);p=records[r]['ref'][5]
 records[p]['scalar'].update({191:ovr,242:year,261:255,231:4,230:255})
 records[r]['scalar'][2]=4+year if portal else 0
 records[r]['scalar'][14]=2
 records[tplist if portal else hslist]['array'].append(r)
 targets_by_recruit[r]=t
 if portal and stage==5:valid_offer(r,t)
 return r,t,p
keep=pp('StClair',82);records[roster]['array']=[keep]
hs,ht,hp=pr(2100,80,0,stage=6,portal=False);records[committed]['array']=[hp]
hen,hen_t,hen_p=pr(2101,81,0);hei,hei_t,hei_p=pr(2102,84,1)
old=pp('departed Sayin',94,1,tid=255);sen=pp('graduating senior',95,3)
records[roster]['array'] += [old,sen]
def decide(r):return call(exports['DriTestPortalDecision'],r,team,context)
configure(1);assert decide(hen)==0
records[hen]['scalar'][9]=5
valid_offer(hen,hen_t)
assert decide(hei)==2
checks.append('same-advance read: first offer-backed hard QB commitment blocks second close starter; departed/graduate excluded; signed HS deduplicated')
# A valid hard commit counts before Team.CommittedPlayers publication.
assert records[arr]['array']==[hen_t] and hen_p not in records[committed]['array']
# Current secured commitments are never undone.
assert decide(hen)==0
records[hen]['scalar'][9]=1;records[hei]['scalar'][9]=5;valid_offer(hei,hei_t);assert decide(hen)==2
# Hard deadline results use the same roster policy.
pf=STACK+0x13000;ps=pf+128;pb=ps+0x200;wq(pf+10*8,pb)
wq(ps+0x30,hen);wq(pf+11*8,team);wq(pb+0x57,context);assert call(sign_cb,pf)==2
wq(pf,team);wq(pb+0x40,context);assert call(deadline_cb,pf)==2
tr=alloc(8);wq(tr,team);wq(pb-0x41,hen);wq(pb+0x77,tr);wq(pb+0x67,context);assert call(instant_cb,pf)==2
checks.append('instant, board signing and portal deadline callbacks read their verified native frame locations')
transition=record('transition',{1:5});wq(ps+0x38,transition);wq(pb+0x68,hen);wq(pb+0x60,context);wq(pf+14*8,1)
call(weekly_cb,pf);assert rq(pf+14*8)==0
records[transition]['scalar'][1]=4;wq(pf+14*8,1);call(weekly_cb,pf);assert rq(pf+14*8)==1
checks.append('weekly rejection clears condition before loop; soft-stage transitions left to native game')
# Diagnostic and disabled modes preserve native decisions.
for m in [0,-1]:
 configure(m);assert decide(hen)==0;assert call(sign_cb,pf)==0
configure(1);records[team]['ref'][407]=record('user character');assert decide(hen)==0;records[team]['ref'].pop(407)
checks.append('diagnostic, disabled and user-team decisions pass through')
# Full positional room requires meaningful improvement at relevant starter, not weakest backup.
records[tplist]['array']=[];records[hslist]['array']=[];records[committed]['array']=[]
records[roster]['array']=[pp('QB'+str(i),v) for i,v in enumerate([83,76,72,70])]
c,ct,cp=pr(2110,85);assert decide(c)==1;records[cp]['scalar'][191]=86;assert decide(c)==1
records[roster]['array']=[pp('WR'+str(i),v,pos=3) for i,v in enumerate([91,86,81,79,76,75,74,72,70,68])]
records[ideal_entries[3]]['scalar'].update({2:10,4:3});w,wt,wp=pr(2111,84,pos=3);assert decide(w)==0
records[wp]['scalar'][191]=83;assert decide(w)==0
# Test29's actual rotation route: three starters imply six useful WR seats.
# Candidate79 projects80; sixth incumbent75 projects76, a genuine +4 rotation
# upgrade. R4 also permits prestige-qualified WR starter/depth additions.
# The independent QB85/86 rejections above remain unchanged.
records[wp]['scalar'][191]=79
assert decide(w)==(0 if 'DriTestPortal29Observe' in exports else 1)
checks.append('QB guards retained; R4 admits prestige-qualified WR depth and the prior +4 rotation upgrade' if 'DriTestPortal29Observe' in exports else 'Adequate QB and WR roles close even when a higher-rated candidate exists')
# Portal board prune uses the real captured removal routine before native iteration.
configure(1);records[roster]['array']=[keep];records[hslist]['array']=[];records[committed]['array']=[]
records[tplist]['array']=[hen,hei];records[hen]['scalar'][9]=4;records[hei]['scalar'][9]=5
records[hen]['scalar'][14]=2;records[arr]['array']=[hen_t,hei_t]
records[hen_t]['ref'][10]=hen;records[hei_t]['ref'][10]=hei
wq(sp+0x58,context);uc.mem_write(sp+0x42,b'\1')
configure(0);call(start,frame);assert records[arr]['array']==[hen_t,hei_t]
configure(1);call(start,frame)
assert records[arr]['array']==[hei_t] and records[hen]['scalar'][14]==1
assert records[hen_t]['ref'][10]==0 and records[hei_t]['ref'][10]==hei
wq(candidate,hen);wq(frame+11*8,candidate);assert call(admit,frame)==1
checks.append('actual native portal prune removes redundant soft-commit pursuit and decrements offer count; hard commitment preserved; re-admission blocked; diagnostic leaves board intact')
# Validate actual compiled star callback writes only ProspectStarRating, by all boundaries / years.
starplayer=pp('rating candidate',81,0);starref=alloc(8);wq(starref,starplayer);wq(pf+12*8,starref)
count=0
for y in range(4):
 for o in [0,67,68,69,70,72,73,74,75,79,80,81,82,87,88,89,90,99]:
  records[starplayer]['scalar'].update({191:o,242:y,231:4});before=dict(records[starplayer]['scalar'])
  assert call(star_cb,pf)==1
  score=o+(2 if y==0 else 1 if y==1 else 0);expected=sum(score>=x for x in [70,75,82,90])
  assert records[starplayer]['scalar']==dict(before,**{}) | {231:expected};count+=1
for m in [0,-1]:
 configure(m);before=dict(records[starplayer]['scalar']);assert call(star_cb,pf)==0;assert before==records[starplayer]['scalar']
checks.append(f'{count} actual compiled rating writes; original star floor removed; every other scalar unchanged; diagnostic and disabled do not write')
configure(1);records[starplayer]['scalar'][242]=4;assert call(star_cb,pf)==0;assert decide(w)==0
checks.append('invalid school-year disables behavior instead of inventing a fifth native grade')
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
checks.append('all fixture record references balanced')
out={'checks':checks,'method':'compiled Windows DLL callbacks + captured native own-commit code, mocked storage and Windows APIs','live_game_verified':False}
(OUTPUT/'portal_runtime_results.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
