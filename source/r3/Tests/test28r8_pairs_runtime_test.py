"""R8 actual compiled DLL: one scored plan, promotion, funding and hard closure."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
records[season]['scalar'].update({2:2040,4:8,5:1})
for x in [departures,roster,hslist,tplist,committed,arr]:records[x]['array']=[]
for pos,e in enumerate(ideal_entries):records[e]['scalar'].update({2:ideals[pos],4:1})
records[ideal_entries[0]]['scalar'][2]=4
records[roster]['array']=[pp('QBreturner '+str(j),65,pos=0)for j in range(3)]
for pos in range(1,22):records[roster]['array'] += [pp('filled '+str(pos),65,pos=pos)for j in range(ideals[pos])]
a,at,ap=pr(63000,79,year=0,pos=0,stage=1,portal=False)
b,bt,bpp=pr(63001,74,year=0,pos=0,stage=1,portal=False)
c,ct,cp=pr(63002,80,year=0,pos=0,stage=1,portal=False)
for p in [ap,bpp,cp]:records[p]['scalar'][231]=3
valid_offer(a,at);valid_offer(b,bt)
scores={a:400,b:500,c:600}
def score():
 scorer,selection,portal,tr=args();assert scorer==fixture_hs_scorer and selection==0 and portal==0
 cand=rq(reg('RSP')+0x30);wi(cand+40,scores[rq(cand)]);return 0
game_api(0x08185840,score)
role=lambda r,f=0:call(exports['DriTestHsPlanRole'],r,team,context,f)
decide=lambda r:call(exports['DriTestHsPairDecision'],r,team,context,0)
checks=[]
assert (role(a),role(b))==(2,1) and (decide(a),decide(b))==(17,0)
assert (role(a,3),role(b,3))==(400,500)
checks.append('Native score makes lower-OVR B the primary in board, funding and commitment planning; no quality-only rerank')
# A real primary loss promotes the existing backup before evaluating a new rival.
influence_before={r:[dict(records[e]['scalar'])for e in records[records[r]['ref'][13]]['array']]for r in [a,b]}
game_api(0x0816b130,lambda:int(rq(args()[2])!=b));valid_offer(c,ct)
assert (role(a),role(b),role(c))==(1,0,2)
assert (role(a,1),role(c,1))==(40,20) and role(a,2)==role(c,2)
for r in [a,b]:assert influence_before[r]==[records[e]['scalar']for e in records[records[r]['ref'][13]]['array']]
assert records[at]['scalar'][12]>=2
checks.append('Lost B promotes offered A ahead of higher-scored newcomer C; C becomes backup for same opening, influence and scholarship preserved')
# Funding consumes the reconciled plan, retaining the 40:20 relative priority.
ff=STACK+0x55000;fb=ff+0x400;wq(ff+10*8,fb);wq(ff+9*8,context)
tr=alloc(8);wq(tr,team);wq(fb-0x60,tr)
vec=alloc(144);ind=alloc(16);wq(fb-0x50,vec);wq(fb-0x48,vec+144);wq(fb+0x58,ind);wq(fb+0x60,ind+16)
for j,(r,t)in enumerate([(a,at),(c,ct)]):
 wq(vec+j*72,r);wi(vec+j*72+40,300);wq(vec+j*72+48,t);wi(vec+j*72+64,1);wq(ind+j*8,j)
call(hooks[23],ff)
assert (ri(vec+40),ri(vec+112))==(400,200)
checks.append('Actual funding callback preserves exact 600-point native budget and gives promoted primary/backup 400:200 weights')
# No replacement is valid: primary continues alone, no phantom roster opening.
game_api(0x0816b130,lambda:int(rq(args()[2])==a))
assert role(a)==1 and role(c)==0 and role(a,1)==40
checks.append('No viable replacement leaves one primary, without inventing another target or place')
# Restart reconstructs the approved native scorer from live saved board state.
game_api(0x0816b130,lambda:int(rq(args()[2])!=b));call(exports['DriTestHsPlanReset'])
assert (role(a),role(c))==(2,1)
checks.append('Reload without cached roles rebuilds from live native scores, not zero-score OVR sorting')
# Move to a new week, lose C: A promotes, and hard ownership closes the opening.
records[season]['scalar'][4]=9;game_api(0x0816b130,lambda:int(rq(args()[2])==a))
assert role(a)==1
records[a]['scalar'][9]=5
assert decide(c)==16
assert call(exports['DriTestHsPairPrune'],team,context,1)==1
assert records[arr]['array']==[at] and records[ct]['ref'][10]==0 and records[bt]['ref'][10]==0
checks.append('Same-advance hard commitment closes the opening and withdraws both surplus alternatives; committed owner remains')
# Two same-position openings retain the surviving pair after the first fills.
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
records[arr]['array']=[];records[hslist]['array']=[];records[committed]['array']=[]
records[roster]['array']=[p for p in records[roster]['array'] if records[p]['scalar'][3]!=0]
records[roster]['array'] += [pp('two seat QB '+str(j),65,pos=0)for j in range(2)]
game_api(0x0816b130,lambda:1)
items=[pr(63100+j,75-j,year=0,pos=0,stage=1,portal=False)for j in range(4)]
for j,(r,t,p)in enumerate(items):
 records[p]['scalar'][231]=3;valid_offer(r,t);scores[r]=500-j*100
assert [role(r)for r,t,p in items]==[1,1,2,2]
records[items[0][0]]['scalar'][9]=5
assert [role(r)for r,t,p in items]==[0,1,0,2]
assert role(items[1][0],2)==role(items[3][0],2)
records[items[1][0]]['scalar'][9]=5
assert decide(items[2][0])==decide(items[3][0])==16
checks.append('Two native QB openings: first hard signing closes only its pair; surviving primary and its own backup stay paired; second hard signing closes both')
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
checks.append('All native record references balanced; no runtime fault')
(OUTPUT/'test28r8_pairs_runtime_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
