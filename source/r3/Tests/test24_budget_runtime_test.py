"""Test24 real compiled Windows callbacks against mocked native/SDF services."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
checks=[]
def reset_budget():
 call(exports['DriTestOwnershipReset']);configure(1)
 for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
 records[team]['ref'].pop(407,None);records[team]['scalar'][384]=10
 records[season]['scalar'].update({2:2026,4:4,5:6})
 records[tuning]['scalar'][39]=2;wq(context+0xd0,tuning)
 for p,e in enumerate(ideal_entries):records[e]['scalar'].update({2:4,0:0,4:1})
 records[ideal_entries[2]]['scalar'][2]=6
 for p in [5,6,7,8,9]:records[ideal_entries[p]]['scalar'][2]=3
reset_budget()
records[ideal_entries[1]]['scalar'][2]=6
for pos in [5,9]:records[roster]['array'] += [pp(f'general {pos}/{j}',v,pos=pos) for j,v in enumerate([76,70,70])]
for pos in [6,7,8]:records[roster]['array'] += [pp(f'emergency {pos}/{j}',65,pos=pos) for j in range(3)]
records[roster]['array'] += [pp(f'RB {j}',v,pos=1) for j,v in enumerate([92,72,70,68,65,63])]
need,nt,np=pr(47000,83,pos=6)
hsnew,ht,hp=pr(47001,75,pos=7,portal=False)
assert decide(need)==0 and call(exports['DriTestHsDecision'],hsnew,team,context)==0
valid_offer(need,nt);valid_offer(hsnew,ht);assert decide(need)==0
# A secured HS overage protects its role but consumes one shared general place.
hs,hs_t,hs_p=pr(47002,70,pos=5,stage=6,portal=False)
records[committed]['array']=[hs_p]
assert decide(need)==0
secured=[]
for j,(pos,ovr) in enumerate([(5,81),(7,78),(8,78),(1,79)]):
 r,t,p=pr(47020+j,ovr,pos=pos,stage=5);secured.append((r,t,p))
 assert decide(r)==0
 if j<2:assert decide(need)==0
 else:assert decide(need)==8
assert decide(need)==8
assert call(exports['DriTestHsDecision'],hsnew,team,context)==8
pt,pt_t,pt_p=secured[1]
checks.append('One HS plus four portal overages jointly consume 2 general, 2 emergency and 1 RB; mere offers consume none')
# The actual instant, board-sign and deadline callback frames enforce reason 8.
wq(ps+0x30,need);wq(pf+11*8,team);wq(pb+0x57,context);assert call(sign_cb,pf)==8
wq(pf,team);wq(pb+0x40,context);assert call(deadline_cb,pf)==8
wq(pb-0x41,need);wq(pb+0x77,tr);wq(pb+0x67,context);assert call(instant_cb,pf)==8
records[transition]['scalar'][1]=5;wq(ps+0x38,transition);wq(pb+0x68,need);wq(pb+0x60,context);wq(pf+14*8,1)
call(weekly_cb,pf);assert rq(pf+14*8)==0
# Next-week recount does not replenish an allowance already used by players.
records[season]['scalar'][4]=5
assert decide(need)==8
records[pt]['scalar'][9]=6;records[committed]['array'].append(pt_p)
assert decide(need)==8 and decide(pt)==0
checks.append('Weekly, instant, signing and deadline paths enforce the current five-slot budget; next week and roster publication do not reset or double-count it')
# Genuine vacancies bypass the exhausted optional budget without a rating ceiling.
records[team]['scalar'][384]=1
records[ideal_entries[10]]['scalar'][2]=3
records[roster]['array'].append(pp('Coastal-like HS RT',67,pos=10))
for i,ovr in enumerate([68,69,80,90,99]):
 r,t,p=pr(47100+i,ovr,pos=10);assert decide(r)==0
hs_v,_,_=pr(47110,80,pos=10,portal=False)
assert call(exports['DriTestHsDecision'],hs_v,team,context)==0
checks.append('Exact Coastal-like 67 RT / ideal 3 regression stays open at exhausted budget for 68, 69, 80, 90 and 99 candidates; HS vacancy remains open too')
records[team]['scalar'][384]=10
for m in [0,-1]:
 configure(m);assert decide(need)==0
configure(1);records[team]['ref'][407]=record('budget user')
assert decide(need)==0 and call(exports['DriTestHsDecision'],hsnew,team,context)==0
records[team]['ref'].pop(407)
# Budget reopens after a real departure/decommit, not after mere week advancement.
records[pt]['scalar'][9]=1;records[committed]['array'].remove(pt_p)
assert decide(need)==0
checks.append('Diagnostic/disabled/user modes preserve native decisions; removing the actual secured excess reopens the allowance')
# Already adequate role creates no score bonus or extra full-room admission.
reset_budget();records[roster]['array']=[pp('LG'+str(i),v,pos=6) for i,v in enumerate([84,76,70])]
r,t,p=pr(47200,95,pos=6);assert decide(r)==1
prepare_cb,score_cb=hooks[8:10]
wq(frame+2*8,context);wq(frame+3*8,team_ref);uc.mem_write(sp+0x42,b'\1');call(prepare_cb,frame)
sf=STACK+0x18000;ss=sf+128;wq(sf,candidate);wq(candidate,r);wq(ss+0xa8,team_ref);wq(ss+0x88,GAME+0x0818c282)
wq(sf+1*8,600);wq(sf+3*8,0);call(score_cb,sf);assert (rq(sf+1*8)&0xffffffff)==0
checks.append('Adequate LG1/LG2 close both full-room admission and shortlist score despite a 95-rated prospect')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
(OUTPUT/'test24_budget_runtime_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
