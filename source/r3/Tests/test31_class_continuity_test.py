"""Compiled paid HS source continuity and native minimum-share recovery.
The native limit/records are controlled; no live spending is claimed.
"""
from pathlib import Path
src=(Path(__file__).parent/'test29_class_source_test.py').read_text()
exec(compile(src.split('reset29();call(exports[\'DriTestHs32Reset\'])')[0],'class fixture','exec'))
checks=[]
reset29();call(exports['DriTestHs32Reset']);t=allteams[7]
records[tuning]['ref'][49]=record('source actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
records[season]['scalar'].update({2:2027,4:4,5:6})
records[ideal_entries[12]]['scalar'][2]=6
records[records[t]['ref'][239]]['array']=[pp('saved-shaped DT returner',69,pos=12,tid=7)for _ in range(5)]
budget=records[t]['ref'][224];records[budget]['scalar'].update({1:5,2:0,3:800})
r,p=make(129601,((7,285),),[7],ovr=70);records[p]['scalar'].update({3:12,231:2});scores[129601]=480
target=valid_offer(r,destination=t);records[target]['scalar'][15]=1
out=alloc(88)
assert call(exports['DriTestHsPlanRole'],r,t,context,0)==1
assert call(exports['DriTest29HsReserved'],t,context,out)==1 and ri(out+12*4)==1
old_slot=call(exports['DriTestHsPlanRole'],r,t,context,2);assert old_slot==385
portal,pp0=rr(129602,ovr=70,stage=6,schools=((7,1000),),portal=True)
records[pp0]['scalar'].update({3:12,231:3,261:7,230:7})
records[records[t]['ref'][33]]['array'].append(pp0)
call(exports['DriTest29ClassInvalidate'],t)
assert call(exports['DriTestHsPlanRole'],r,t,context,0)==1
assert call(exports['DriTestHsPlanRole'],r,t,context,2)==old_slot
assert call(exports['DriTest29HsReserved'],t,context,out)==1 and ri(out+12*4)==1
assert call(exports['DriTestHsPairDecision'],r,t,context,1)==0
checks.append('Paid DT primary in five-of-six room remains a legal normal annual HS primary after one native-published portal return fills the room, without converting normal intake into a discretionary allowance; funded source reservation remains one')
# The reservation describes paid current attention. An hour reset invalidates
# that claim immediately, without erasing the historical offer or influence.
before=dict(records[r]['scalar']);records[budget]['scalar'][1]=0
call(exports['DriTest29ClassInvalidate'],t)
assert call(exports['DriTest29HsReserved'],t,context,out)==1 and ri(out+12*4)==0
assert records[r]['scalar']==before and offer_status(r,7)
checks.append('Assigned-hour reset drops the funded source claim; the offer and earned interest remain untouched')

reset29();call(exports['DriTestHs32Reset']);scores.clear();t=allteams[10]
records[season]['scalar'].update({2:2027,4:3,5:6})
records[ideal_entries[0]]['scalar'][2]=4
records[tuning]['ref'][49]=record('funding actions',array=[record('scholarship',{0:3,1:0,2:5,5:1,6:1}),record('social',{0:5,1:10,2:5,5:1,6:0})])
r,p=make(129610,((10,100),),[],ovr=70);scores[129610]=400
target=record('unfunded HS board target',{12:0},ref={10:r});records[board_targets(t)]['array'].append(target)
other,op=rr(129611,ovr=60,stage=0,schools=((10,20),),offers=[10],portal=True);ot=valid_offer(other,destination=t)
vec=alloc(144);indices=alloc(16)
for i,(rec,tg,score)in enumerate([(r,target,1),(other,ot,99)]):
 wq(vec+i*72,rec);wi(vec+i*72+40,score);wq(vec+i*72+48,tg);wi(vec+i*72+64,0);wq(indices+i*8,i)
reject=False
def native_limit():
 ev,tr,targ,index=args();assert ev==context and rq(tr)==t and rq(targ)==target and index==0
 return 0 if reject else ri(vec+40)//2
game_api(0x0817e7b0,native_limit)
budget=records[t]['ref'][224];before=dict(records[budget]['scalar']);snapshot=snap8(r)
assert call(exports['DriTestHsRecoveryPending'],r,context)==0
wq(STACK+0xf0000-8+0x28,indices);call(exports['DriTest29R1Funding'],t,context,vec,2)
assert [ri(vec+40),ri(vec+112)]==[20,80]
assert records[budget]['scalar']==before and snap8(r)==snapshot and not offer_status(r,10)
wi(vec+40,1);wi(vec+112,99);reject=True
call(exports['DriTest29R1Funding'],t,context,vec,2)
assert [ri(vec+40),ri(vec+112)]==[1,99]
checks.append('Non-recovery under-annual-target HS primary receives minimum usable native share by exact same-bucket20/80 transfer; native refusal restores1/99; budgets, offers and influence unchanged')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'test31_class_continuity_results.json').write_text(json.dumps({'checks':checks,'passed':True,'native_action_execution':False,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
