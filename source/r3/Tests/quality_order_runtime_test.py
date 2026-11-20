"""Test13 current-room admission through compiled Windows callbacks, fixture SDF."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);records[team]['scalar'][384]=9
checks=[]
caseid=43000
for pos in [0,19,20]:
 for first,second in [(76,81),(81,76)]:
  call(exports['DriTestOwnershipReset'])
  for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
  records[ideal_entries[pos]]['scalar'].update({2:4 if pos==0 else 2,4:1})
  # Two genuine vacancies in each room: QB 2/4, specialists 0/2.
  if pos==0:records[roster]['array']=[pp('returner',76,pos=pos),pp('depth',65,pos=pos)]
  r,t,p=pr(caseid,first,pos=pos,stage=5);other,ot,op=pr(caseid+1,second,pos=pos);caseid+=10
  valid_offer(other,ot)
  assert decide(other)==0 and decide(r)==0,(pos,first,second,decide(other),decide(r))
  # Every live signing path reads the same fresh roster and permits the complement.
  wq(ps+0x30,other);wq(pf+11*8,team);wq(pb+0x57,context);assert call(sign_cb,pf)==0
  wq(pf,team);wq(pb+0x40,context);assert call(deadline_cb,pf)==0
  wq(pb-0x41,other);wq(pb+0x77,tr);wq(pb+0x67,context);assert call(instant_cb,pf)==0
  records[other]['scalar'][9]=5;valid_offer(other,ot)
  assert decide(other)==0 and decide(r)==0
  close,clt,clp=pr(caseid+2,83,pos=pos);assert decide(close)==2
  # Publication must not double count or revoke the two valid commitments.
  records[r]['scalar'][9]=records[other]['scalar'][9]=6
  records[committed]['array']=[p,op]
  assert decide(close)==2
  # Both useful roles are already filled. Test24 does not reopen an adequate
  # full room merely because a 95-rated recruit is available.
  extra,et,ep=pr(caseid+3,95,pos=pos);assert decide(extra)==1
  checks.append(f'Position {pos}: {first} then {second} allowed by decision/instant/sign/deadline; published roster agrees; close duplicate and filled-role closure enforced')
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
assert call(exports['DriTestQualityFault'])==0
checks.append('All fixture references balanced; no runtime fault')
(OUTPUT/'quality_order_runtime_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
