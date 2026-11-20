"""Full saturated plan through captured weekly native condition/stage writers.
Native expression storage is a controlled threshold leaf, as in the retained
weekly-destination regression. It does not assert actual live FTC outcomes.
"""
from pathlib import Path
T=Path(__file__).parent
weekly_source=(T/'test31_weekly_destination_test.py').read_text()
prefix_src=weekly_source.split('checks=[]\nr,p,target=prepare()')[0]
prefix_src=prefix_src.replace("records[transition]['scalar'][1]==4", "records[transition]['scalar'][1] in (4,5)")
exec(compile(prefix_src,'native condition definitions','exec'))
boundary=weekly_source.split('# Execute the new original-condition boundary,')[1].split('r,p,t=prepare(31020)')[0]
boundary='# Execute the new original-condition boundary,'+boundary
boundary=boundary.replace('def full_attempt(r):','def full_attempt(r,next_stage=4):').replace("record('native boundary soft transition',{1:4})", "record('native boundary transition',{1:next_stage})")
exec(compile(boundary,'captured condition branch definitions','exec'))
# The unchanged manager side-effect service reached only on hard commitment
# creates additional SDF records outside this fixture. Model that leaf without
# writing the recruit/offer/budget; the captured writer still sets stage5.
hard_service_calls=[]
def hard_manager_service():
 manager_,team_ref,recruit_ref,_=args()
 hard_service_calls.append((rq(team_ref),rq(recruit_ref)));return 0
game_api(0x081ae5c0,hard_manager_service)
game_api(0x0815f8a0,lambda:0) # subsequent manager side-effect leaf; no fixture record writes
checks=[];plan_out=alloc((4+128*4)*4);annual_out=alloc(46*4)
def prepare_full():
 reset29();records[season]['scalar'].update({2:2026,4:5,5:6})
 records[tuning]['scalar'][13]=10;t=allteams[55]
 ideals34=[4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0]
 for entry,ideal in zip(ideal_entries,ideals34):records[entry]['scalar'][2]=ideal
 assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
 av=list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)));assert av[0]==18 and av[2]==1
 ordinary=[]
 for pos,count in enumerate(av[1:23]):
  for k in range(count):
   hard=pos in [1,4,9,13] and k==0
   r,p=make(343000+pos*4+k,((55,650 if hard else 200),),[55],74 if pos==1 else 68,5 if hard else 0)
   records[p]['scalar'].update({3:pos,231:3 if pos==1 else 2})
   if not hard:ordinary.append(r)
 observed=json.loads((T/'Fixtures/test34r2_navy_w5.json').read_text())
 records[records[t]['ref'][239]]['array']=[pp('observed Navy returner',70 if p==1 else 60,pos=p,tid=55) for p,n in enumerate(observed['returning_by_position']) for _ in range(n)]
 r,p=make(2408,((55,452),),[55],74,3);records[p]['scalar'].update({3:1,231:3});records[r]['scalar'][3]=745
 target=next(x for x in records[board_targets(t)]['array'] if records[x]['ref'].get(10)==r)
 records[target]['scalar'].update({8:452,9:452,7:100})
 records[records[t]['ref'][224]]['scalar'].update({1:5,2:0,3:800})
 return t,r,target,ordinary
def fullplan(t):
 n=call(exports['DriTest34R2PlanSnapshot'],t,context,plan_out);assert n>=0
 a=list(struct.unpack('<'+str(4+n*4)+'i',uc.mem_read(plan_out,(4+n*4)*4)))
 return a[:4],{a[i]:a[i+1:i+4] for i in range(4,len(a),4)}
def earned(r,target,value):
 # External observed/controlled input, not production behavior or a simulated
 # delivery calculation. The tested runtime must never elevate this value.
 for e in records[records[r]['ref'][13]]['array']:records[e]['scalar'][1]=value
 records[target]['scalar'].update({8:value,9:value})
t,r,target,ordinary=prepare_full();state,roles=fullplan(t)
assert state==[19,4,15,0] and roles[2408][:2]==[1,63],(state,roles.get(2408))
assert len(ordinary)==14 and all(roles[ri(x+0x18)][0]==1 for x in ordinary)
for points in [0,452,744]:
 earned(r,target,points);before=values(r)
 assert not full_attempt(r) and records[r]['scalar'][9]==3 and values(r)==before,(points,values(r))
checks.append('Actual captured false branch rejects optional primary at0/452/744 against controlled native threshold745; planning never supplies missing influence')
earned(r,target,745);extra_condition=False;before=values(r)
assert not full_attempt(r) and values(r)==before and records[r]['scalar'][9]==3
extra_condition=True
assert full_attempt(r) and records[r]['scalar'][9]==4
assert values(r)==[(55,671)] and records[target]['scalar'][8]==671,values(r)
checks.append('At actual input745 with other native conditions true, captured weekly branch and stage writer complete soft commitment and native74-point deduction for threshold745 and10-percent tuning')
# Later native-earned input is supplied by the fixture; no production credit
# is invented. The same original transition branch executes HardCommitted.
earned(r,target,745)
assert full_attempt(r,5) and records[r]['scalar'][9]==5
assert hard_service_calls[-1]==(t,r)
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
state,roles=fullplan(t);assert state==[19,5,14,0],state
assert all(roles[ri(x+0x18)][0]==1 for x in ordinary)
assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1 and ri(annual_out+45*4)==(1<<6)
checks.append('Captured native hard-commit transition records the extra HB, preserves14 ordinary primaries and charges one shared general place after reload')
# A missing real scholarship still blocks acceptance even with sufficient points.
t,r,target,ordinary=prepare_full();earned(r,target,745);records[target]['scalar'][12]=1
before=values(r);assert not full_attempt(r) and records[r]['scalar'][9]==3 and values(r)==before
checks.append('Withdrawn scholarship cannot commit at threshold')
uc.hook_del(stop_hook)
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0 and all(v==1 for v in refs.values())
report={'passed':True,'checks':checks,'native_executed':['08627910 condition evaluator','081b98d6 weekly transition branch','0814c3c0 stage writer','081b2730 influence adjustment'],
 'controlled_expression_leaf':True,'modeled_hard_manager_side_effect_leaf':'081ae5c0 and0815f8a0; first checked for selected team/recruit; no recruit/offer/budget writes', 'influence_delivery_executed':False,'live_game_verified':False}
(OUTPUT/'test34_r2_native_commitment_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
