"""Full class plan feeds captured native scholarship/productive allocation loops.
Only one three-entry native vector is exercised; the other ordinary primaries
remain on the real compiled board plan. Native influence delivery is not run.
"""
from pathlib import Path
T=Path(__file__).parent
exec(compile((T/'test33_native_funding_spend_test.py').read_text().split('\nresults=[]')[0],'native two-pass definitions','exec'))
# The former normal LE primary commits. Its former backup must become the
# optional primary even when the complete ordinary annual class is planned.
t=allteams[96];old=primary;records[old]['scalar'][9]=5
primary=backup;pt=bt
for entry,ideal in zip(ideal_entries,[4,5,4,6,5,3,3,3,3,3,4,4,6,2,4,4,8,4,4,2,2,0]):records[entry]['scalar'][2]=ideal
# Keep the known calendar year with one LE place under this full scheme.
for year in range(2026,2066):
 records[season]['scalar'][2]=year
 annual_out=alloc(46*4);assert call(exports['DriTest31ClassAnnual'],t,native_eval,annual_out)==1
 av=list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)))
 if av[11]==1:break
else:raise AssertionError('one-LE annual fixture missing')
q=av[1:23];ordinary=[]
for pos,count in enumerate(q):
 if pos==10:continue
 for k in range(count):
  r,p,tg=target31(342000+pos*4+k,300,68,pos=pos,stage=1,points=100)
  records[p]['scalar'][231]=2;ordinary.append(r)
records[records[t]['ref'][239]]['array']=[pp('LE returning',70,pos=10,tid=96) for _ in range(2)]
backup,bp,bt=target31(342100,350,72,pos=10,stage=1,points=90)
records[primary]['scalar'][9]=1
plan_out=alloc((4+128*4)*4)
def fullplan():
 n=call(exports['DriTest34R2PlanSnapshot'],t,native_eval,plan_out);assert n>=0
 a=list(struct.unpack('<'+str(4+n*4)+'i',uc.mem_read(plan_out,(4+n*4)*4)))
 return a[:4],{a[i]:a[i+1:i+4] for i in range(4,len(a),4)}
results=[]
for remaining in [5,0,4]:
 reset_case33(remaining,primary_offered=2,backup_offered=2)
 state,roles=fullplan()
 assert state==[av[0]+1,1,av[0],0],state
 assert roles[ri(primary+0x18)][:2]==[1,10*32+31]
 assert all(roles[ri(r+0x18)][0]==1 for r in ordinary)
 before_points={r:values(r) for r in [primary,backup]+ordinary}
 before,after=first33();spent=second33(primary if remaining==5 else None)
 assert before_points=={r:values(r) for r in before_points}
 assert records[old]['scalar'][9]==5 and records[primary]['scalar'][9]==1
 after_roles=fullplan()[1]
 assert all(after_roles[ri(r+0x18)][0]==1 for r in ordinary)
 results.append({'available_hours':remaining,'productive_spent':spent,'effective_target':state[0],'ordinary_primaries_retained':len(ordinary),'optional_primary':ri(primary+0x18),'events':events[:]})
call(exports['DriTestHs32Reset']);call(exports['DriTestHsPlanReset']);call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values())
report={'passed':True,'cases':results,'captured_native':regions+regions33,'live_game_verified':False,
 'modeled_services':['SDF storage','native eligibility continuation','scholarship publication/payment leaf (both HS offers pre-existing in these cases)','per-target entitlement','readiness and diagnostic leaves','scalar AVX operations','Windows'],
 'limitations':['Full board plan evaluated; only three native allocation vector entries executed.','No weekly influence delivery or live week advance. Native-earned points remain unchanged by planning and allocation.']}
(OUTPUT/'test34_r2_native_funding_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
