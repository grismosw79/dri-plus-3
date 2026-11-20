"""Actual R6 DLL planning/commitment/pruning/funding, with native SDF services mocked."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestOwnershipReset']);records[season]['scalar'].update({2:2040,4:8,5:1})
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
for pos,e in enumerate(ideal_entries):records[e]['scalar'].update({2:ideals[pos],4:1})
records[ideal_entries[0]]['scalar'][2]=4
records[roster]['array']=[pp('returner '+str(j),65,pos=0)for j in range(3)]
for pos in range(1,22):records[roster]['array'] += [pp('filled '+str(pos),65,pos=pos)for j in range(ideals[pos])]
a,at,ap=pr(61000,75,year=0,pos=0,stage=1,portal=False)
b,bt,bp_player=pr(61001,70,year=0,pos=0,stage=1,portal=False)
for p in [ap,bp_player]:records[p]['scalar'][231]=1
valid_offer(a,at);valid_offer(b,bt)
checks=[]
decision=lambda r,deadline=0:call(exports['DriTestHsPairDecision'],r,team,context,deadline)
assert (decision(a),decision(b))==(0,17),(decision(a),decision(b),call(exports['DriTestQualityFault']))
checks.append('Backup waits while offered primary remains legal and competitive')
# Eligibility changes promote the fallback immediately, with no board mutation.
game_api(0x0816b130,lambda:int(rq(args()[2])!=a))
assert decision(b)==0
game_api(0x0816b130,lambda:1)
# Two legal quantity openings are distinct; both candidates are primaries.
removed=records[roster]['array'].pop(0)
assert decision(a)==decision(b)==0
records[roster]['array'].append(removed)
# R8 preserves the prior promotion across passes. Reset only for this separate
# original funding scenario; dedicated R8 tests exercise persistent promotion.
if 'DriTestHsPlanReset' in exports:call(exports['DriTestHsPlanReset'])
# Funding changes the ratio within a native budget, not the stage sum or NIL.
ff=STACK+0x55000;fb=ff+0x400;wq(ff+10*8,fb);wq(ff+9*8,context)
tr=alloc(8);wq(tr,team);wq(fb-0x60,tr)
vec=alloc(144);ind=alloc(16);wq(fb-0x50,vec);wq(fb-0x48,vec+144);wq(fb+0x58,ind);wq(fb+0x60,ind+16)
for j,(r,t)in enumerate([(a,at),(b,bt)]):
 wq(vec+j*72,r);wi(vec+j*72+40,300);wq(vec+j*72+48,t);wi(vec+j*72+64,1);wq(ind+j*8,j)
call(exports['DriOnPortalFunding'],ff) if 'DriOnPortalFunding' in exports else call(hooks[23],ff)
assert call(exports['DriTestQualityFault'])==0
assert ri(vec+40)+ri(vec+112)==600 and ri(vec+40)>ri(vec+112)
checks.append('Primary receives more funding weight while exact native stage total is conserved')
records[season]['scalar'].update({4:3,5:6})
wi(vec+40,300);wi(vec+112,300);call(hooks[23],ff)
assert ri(vec+40)+ri(vec+112)==600 and ri(vec+40)>ri(vec+112)
assert call(exports['DriTestQualityFault'])==0
checks.append('HS primary/backup funding continues during offseason without changing the native budget total')
records[season]['scalar'].update({4:8,5:1})
# Actual offer-backed hard ownership closes the one vacancy before publication.
records[a]['scalar'][9]=5
assert decision(b)==16 and decision(a)==0
old=records[b]['scalar'][14]
assert call(exports['DriTestHsPairPrune'],team,context,1)==1
assert records[arr]['array']==[at] and records[bt]['ref'][10]==0 and records[b]['scalar'][14]==old-1
checks.append('Same-advance hard commitment blocks second fill; safe native removal withdraws backup and preserves commitment')
# User and passive modes retain manual control.
records[team]['ref'][407]=record('user');assert decision(b)==0
records[team]['ref'].pop(407)
for m in [0,-1]:configure(m);assert decision(b)==0
configure(1);call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
assert call(exports['DriTestQualityFault'])==0
checks.append('User, disabled and diagnostic paths pass through; references balanced')
(OUTPUT/'test28r6_pairs_runtime_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
