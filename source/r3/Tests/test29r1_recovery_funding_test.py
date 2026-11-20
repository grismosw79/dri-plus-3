"""Compiled reserved HS funding conservation with a controlled native limit.
Native helper ABI/branches tested separately; this is not live spending."""
from pathlib import Path
import json
src=(Path(__file__).parent/'test28r14_offseason_test.py').read_text()
exec(compile(src.split('for rid,tid,pos,ovr in [(2991')[0],'recovery fixture','exec'))
r,t=case(1242,101,10,80);call(exports['DriTestHs32Offseason'],t,context)
target=records[board_targets(t)]['array'][0]
other,op=rr(80000,ovr=60,stage=0,schools=((101,20),),offers=[101],portal=True);ot=valid_offer(other,destination=t)
vec=alloc(144);indices=alloc(16)
for i,(rec,tg,score) in enumerate([(r,target,1),(other,ot,99)]):
 wq(vec+i*72,rec);wi(vec+i*72+40,score);wq(vec+i*72+48,tg);wi(vec+i*72+64,0);wq(indices+i*8,i)
seen=[];reject=False
def limit():
 ev,tr,targ,index=args();begin=rq(rq(reg('RSP')+0x28))
 assert ev==context and rq(tr)==t and begin==vec and rq(targ)==target and index==0
 seen.append(ri(vec+40));return 0 if reject else ri(vec+40)//2
game_api(0x0817e7b0,limit)
before_budget=dict(records[records[t]['ref'][224]]['scalar']);before_target=dict(records[target]['scalar'])
wq(STACK+0xf0000-8+0x28,indices)
call(exports['DriTest29R1Funding'],t,context,vec,2)
assert ri(vec+40)==20 and ri(vec+72+40)==80,(ri(vec+40),ri(vec+72+40),seen)
assert records[records[t]['ref'][224]]['scalar']==before_budget and records[target]['scalar']==before_target
assert not offer_status(r,101),'weight repair is not a funded offer'
wi(vec+40,1);wi(vec+112,99);reject=True
call(exports['DriTest29R1Funding'],t,context,vec,2)
assert ri(vec+40)==1 and ri(vec+112)==99,'native refusal must restore scores'
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'r1_recovery.json').write_text(json.dumps({'exact_stage_total':100,'minimum_usable_native_allowance':10,'native_refusal_restores_weights':True,'budgets_offers_points_unchanged':True,'live_game_verified':False},indent=2))
print('PASS reserved recovery receives minimum usable native share; totals conserved and native refusal honored')
