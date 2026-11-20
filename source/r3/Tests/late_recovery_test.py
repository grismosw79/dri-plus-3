"""Late NSD recovery against captured native removal/rank/lockout helpers.
Native dealbreaker results are controlled fixtures, not a simulated season.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'recovery_runtime_test.py').read_text(),'recovery_runtime_test.py','exec'))
checks=[];denied=set();native_gate_calls=[]
def gate():
 ev,tr,rr,arg=args();assert ev==(rq(context+0xc8) if (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image))))) else context) and (arg!=0 if (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))) else arg==0)
 tid=records[rq(tr)]['scalar'][383];native_gate_calls.append(tid)
 return int(tid not in denied)
game_api(0x0816b130,gate)
def late(rec):
 global next_heap
 before=next_heap;call(exports['DriTestDeadlineRecovery'],rec,context);after=next_heap
 if after>before:uc.mem_write(before,bytes(after-before))
 next_heap=before
 assert call(exports['DriTestQualityFault'])==0
reset_recovery();denied.clear()
# Prince Strachan shape: two overfull listed schools, a legitimate UAB offer
# outside Top5 with 17 earned points. No offer/points are manufactured.
r,p=rr(5500,stage=1,schools=((10,100),(11,90),(12,60),(13,40),(14,30)),offers=(10,11,96))
fill(10);fill(11)
for t in records[board_targets(allteams[96])]['array']:
 if records[t]['ref'].get(10)==r:records[t]['scalar'][8]=17
assert locked(r,96)==1
late(r)
assert records[r]['scalar'][9]==0 and values(r)[0]==(96,17)
assert locked(r,96)==0 and offer_status(r,96)
assert not offer_status(r,10) and not offer_status(r,11)
assert records[r]['scalar'][14]==1
before=values(r);late(r);assert values(r)==before
checks.append('Strachan-shaped late offer outside Top5 restored with exactly 17 earned influence; native lockout clears; full-room offers removed; repeat is idempotent')
# A legal live finalist prevents recovery, even when an outsider has more points.
reset_recovery();r,p=rr(5501,stage=2,schools=((10,110),(11,100),(12,90)),offers=(10,96))
before=values(r);late(r);assert records[r]['scalar'][9]==2 and values(r)==before
checks.append('Existing viable finalist race preserved')
# A signing after an earlier check changes the verdict on the next fallback.
reset_recovery();r,p=rr(5502,stage=2,schools=((10,110),(11,100),(12,90)),offers=(10,13))
for t in records[board_targets(allteams[13])]['array']:records[t]['scalar'][8]=43
late(r);assert records[r]['scalar'][9]==2
fill(10);late(r);assert records[r]['scalar'][9]==0 and values(r)[0]==(13,43)
checks.append('Fresh roster check responds to a room filled by a preceding NSD signing')
# All native dealbreaker failures: no outsider is promoted or forced to sign.
reset_recovery();r,p=rr(5503,stage=1,offers=(10,13));fill(10);denied.add(13)
late(r);assert all(v==0 for _,v in values(r)) and not offer_status(r,13)
assert records[r]['scalar'][9]==0;denied.clear()
checks.append('Native dealbreaker failure remains disqualifying; no destination forced when all offers fail')
# Academy gets no rescue, HS and secured commits are unchanged.
reset_recovery();r,p=rr(5504,stage=2,offers=(0,7,55));late(r)
assert records[r]['scalar'][14]==0 and all(v==0 for _,v in values(r))
for stage,portal in [(5,True),(6,True),(2,False)]:
 reset_recovery();r,p=rr(5510+stage+int(portal)*10,stage=stage,offers=(10,),portal=portal)
 before=(values(r),dict(records[r]['scalar']));late(r);assert before==(values(r),dict(records[r]['scalar']))
checks.append('Academies excluded; HS, valid hard commitments and Signed players preserved')
# Hook is scoped to the verified native fallback caller and uses the R8 recruit.
reset_recovery();r,p=rr(5550,stage=2,offers=())
ff=STACK+0x18000;fsp=ff+128;rrp=alloc(8);wq(rrp,r)
wq(ff+13*8,context);wq(ff+7*8,rrp);wq(ff+10*8,STACK+0x1a000)
out=alloc(8);wq(ff+12*8,out)
wq(fsp+0x108,GAME+0x081b58fa);assert call(hooks[12],ff)==1 and rq(out)==0 and records[r]['scalar'][9]==2
records[r]['scalar'][9]=2;wq(fsp+0x108,GAME+0x08103d10);call(hooks[12],ff);assert records[r]['scalar'][9]==2
checks.append('Actual compiled fallback-entry callback selects only at the verified NSD call site; absent offers leave stage unchanged')
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
assert call(exports['DriTestQualityFault'])==0
checks.append('All retained references balanced; no runtime fault')
(OUTPUT/'late_recovery_results.json').write_text(json.dumps({'checks':checks,'native_gate_calls':native_gate_calls,'mocked':['native dealbreaker predicate','SDF storage','Windows APIs'],'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
