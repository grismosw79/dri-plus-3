"""Observed ASU class20/base19 with an empty HB and two acquired extras.
Execute actual compiled recovery; reservation still needs native permission.
"""
from pathlib import Path
source=(Path(__file__).parent/'test31_class_recovery_test.py').read_text()
exec(compile(source.split('# Early normal intake')[0],'recovery fixture definitions','exec'))
observed=json.loads((Path(__file__).parent/'Fixtures/test34r3_observed.json').read_text())['recovery_asu']
checks=[]
for blocker in ['none','none_regular','eligibility','score','hours','closed_allocation_window']:
 t=fresh(4);records[season]['scalar'].update({2:2026,4:5,5:6})
 if blocker=='none_regular':records[season]['scalar'].update({4:17,5:1})
 for entry,ideal in zip(ideal_entries,observed['ideal']):records[entry]['scalar'][2]=ideal
 for i,sr in enumerate(observed['secured']):
  r,p=make(135000+i,((4,800),),[4],ovr=sr['ovr'],stage=6)
  records[p]['scalar'].update({3:position_names.index(sr['position']),231:star_names.index(sr['stars'])})
  records[records[t]['ref'][33]]['array'].append(p)
 r,p=candidate(135100,4,pos=1,ovr=68)
 if blocker=='eligibility':allowed8.clear()
 if blocker=='score':scores[135100]=0
 if blocker=='hours':records[records[t]['ref'][224]]['scalar'][3]=9
 if blocker=='closed_allocation_window':records[season]['scalar'].update({4:1,5:6})
 annual_out=alloc(46*4);assert call(exports['DriTest31ClassAnnual'],t,context,annual_out)==1
 av=list(struct.unpack('<46i',uc.mem_read(annual_out,46*4)));assert av[0]==19 and sum(av[23:45])==20 and av[2]==1 and av[24]==0,av
 before=snap8(r);budget=dict(records[records[t]['ref'][224]]['scalar']);seed()
 wanted=blocker in ['none','none_regular'];assert bool(call(exports['DriTestHsRecoveryPending'],r,context))==wanted,(blocker,av)
 assert snap8(r)==before and records[records[t]['ref'][224]]['scalar']==budget and not offer_status(r,4)
 if wanted:
  call(exports['DriTestHs32Begin'],context);assert call(exports['DriTestHs30AdmissionBlock'],r,t,context)==0
  call(exports['DriTestHs32Offseason'],t,context)
  assert adds==([(4,135100)] if blocker=='none' else []) and not offer_status(r,4),(blocker,adds,offer_status(r,4))
 checks.append({'blocker':blocker,'base':19,'secured':20,'unfilled_ordinary_HB':1,'reserved':wanted,'offer_or_hours_fabricated':False})
call(exports['DriTestOwnershipReset']);call(exports['DriTestHs29Reset']);call(exports['DriTestHsPlanReset']);assert all(v==1 for v in refs.values())
(OUTPUT/'test34_r3_recovery_runtime_results.json').write_text(json.dumps({'passed':True,'checks':checks,'native_services_modeled':True,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
