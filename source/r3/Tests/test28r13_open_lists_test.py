"""Compiled R13 detour + captured native lockout; controlled SDF services.
Exercises full week-boundary recovery and preserves earned interest/ownership.
"""
from pathlib import Path
import os,json,struct
os.environ.setdefault('DRI_TEST_INSTRUCTION_LIMIT','100000000')
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'shared fixture','exec'))
checks=[]
if 'DriTestHsShortlistHook' not in exports:
 reset29();r,p=make(95000,((10,950),(11,900),(12,850),(13,800)),[13],stage=2)
 assert locked(r,13)==1
 reset8();r,p=recruit8(95001);recover8();records[season]['scalar'][4]+=1;recover8()
 assert call(exports['DriTestHsRecoveryPending'],r,context)==0
 print('R12_NEGATIVE_REPRODUCED: shortlist lockout and unexecuted reservation expiry')
 raise SystemExit(0)

# Install the actual production detour in captured native text. All shared
# native/UI-shaped callers now enter it; fallback runs its displaced prologue.
entry=call(exports['DriTestHsShortlistHook'])
uc.mem_write(GAME+0x0862d990,b'\xff\x25\0\0\0\0'+struct.pack('<Q',entry)+b'\x90\x90')
cases=0
for cls in range(4):
 reset29();r,p=make(95010+cls,tuple((tid,1000-(tid-10)*50) for tid in range(10,20)),[10])
 records[r]['scalar'][2]=cls
 for stage in range(5):
  records[r]['scalar'][9]=stage;before=snap8(r)
  for tid in [10,12,14,17,19,20,96]:assert locked(r,tid)==0;cases+=1
  assert snap8(r)==before
checks.append(f'{cases} captured-entry HS/JUCO Top10/5/3/Battle/Soft checks allow listed and unlisted schools without record mutation')

# Original portal, acquired-player and secured-commit behavior is exact.
native_cases=0
for cls in range(8):
 reset29();r,p=make(95100+cls,((10,1000),(11,800),(12,500)),[10])
 records[r]['scalar'][2]=cls
 for stage in range(7):
  if cls<4 and stage<5:continue
  records[r]['scalar'][9]=stage
  for tid in [10,12,96]:
   configure(0);want=locked(r,tid);configure(1);assert locked(r,tid)==want;native_cases+=1
reset29();r,p=make(95110,((10,1000),(11,800)),[10],stage=2)
records[p]['scalar'][261]=10
configure(0);want=locked(r,96);configure(1);assert locked(r,96)==want==1
for mode_value in [0,-1]:
 configure(mode_value);records[p]['scalar'][261]=255;assert locked(r,96)==1
configure(1)
checks.append(f'{native_cases} portal/hard/signed cases match captured native fallback; acquired players and disabled/diagnostic modes remain native')

# Preferences can advance while a real soft/hard commitment still needs
# the leader's actual, legal, committable offer.
reset29();r,p=make(95200,((10,950),(11,900),(12,850),(13,800)),[10,11,12,13])
for tid in [10,11,12]:conditional(95300+tid,tid)
assert attempt(r,2)==0 and attempt(r,4)==1
records[r]['scalar'][9]=2
assert locked(r,13)==0 and call(exports['DriTestHs29Offer'],r,allteams[13],context)==2
records[r]['scalar'][9]=0
before=snap8(r);assert attempt(r,1)==0 and attempt(r,3)==0 and snap8(r)==before
reset29();r,p=make(95201,((10,1000),),[])
for stage in [1,2,3]:assert attempt(r,stage)==0
assert attempt(r,4)==1 and attempt(r,5)==1
valid_offer(r,destination=allteams[10]);call(exports['DriTestHs29Reset'])
assert attempt(r,4)==0 and attempt(r,5)==0
allowed8.remove(10);assert attempt(r,4)==1 and attempt(r,5)==1
checks.append('Advisory cuts preserve outside firm offers; soft/hard transitions still reject missing, conditional or native-ineligible leader offers')

# Stack-shaped saved soft state: Illinois offer already withdrawn; California
# remains a real, viable primary. Reassessment must not need a third school.
reset29();r,p=make(1318,((33,672),(16,116)),[16],ovr=72,stage=4)
records[p]['scalar'][3]=3;records[ideal_entries[3]]['scalar'][2]=1
allowed8.clear();allowed8.add(16)
before=snap8(r);call(exports['DriTestHs29Reopen'],context)
assert records[r]['scalar'][9]==0 and values(r)==list(before[1]) and offer_status(r,16)
assert records[r]['scalar']==before[0]|{9:0}
assert call(exports['DriTestHs29Offer'],r,allteams[16],context)==2
records[r]['scalar'][9]=2
assert attempt(r,5)==1 # no offer from the stale leader
reset29();r,p=make(95203,((10,1000),(11,800)),[10],stage=4)
before=snap8(r);call(exports['DriTestHs29Reopen'],context);assert snap8(r)==before
for kind in ['hard','published','roster']:
 reset29();r,p=make(95210+len(kind),((10,1000),),[],stage=5 if kind=='hard' else 4)
 if kind!='hard':records[records[allteams[10]]['ref'][33 if kind=='published' else 239]]['array']=[p]
 before=snap8(r);call(exports['DriTestHs29Reopen'],context);assert snap8(r)==before
checks.append('Withdrawn soft leader reopens with the alternate offer and all influence intact; valid soft and acquired/hard commitments stay protected')

# Unexecuted post-week reservation: next week revalidates the same sole school.
# Observed failed admission: next week does not repeat the failed destination.
for observed in [False,True]:
 reset8();r,p=recruit8(95400+observed);recover8();before=snap8(r)
 assert call(exports['DriTestHsRecoveryReservedCost'],r,allteams[96],context,0)==15
 if observed:call(exports['DriTestHsCoverageObserve'],allteams[96],context)
 records[season]['scalar'][4]+=1;recover8()
 assert snap8(r)==before and not native_events and not offer_status(r,96)
 assert call(exports['DriTestHsRecoveryPending'],r,context)==int(not observed)
 checks.append(('Observed failed' if observed else 'Unexecuted')+' reservation: correct next-week retry decision without manufactured offer, points or spending')
 if not observed:
  # Materialize the next native offer, verify role, then the following week.
  target=valid_offer(r,destination=allteams[96]);records[target]['scalar'][8]=0
  assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==1
  call(exports['DriTestHsCoverageObserve'],allteams[96],context)
  records[season]['scalar'][4]+=1;recover8()
  assert call(exports['DriTestHsPlanRole'],r,allteams[96],context,0)==1
  assert values(r)==list(before[1])
checks.append('Revalidated next-week reservation becomes an offered primary and keeps continuity for another weekly pass')
for loss in ['eligibility','budget','capacity','committed']:
 reset8();r,p=recruit8(95500+len(loss));recover8();records[season]['scalar'][4]+=1
 if loss=='eligibility':allowed8.clear()
 if loss=='budget':records[records[allteams[96]]['ref'][224]]['scalar'][3]=0
 if loss=='capacity':
  fill(96)
  secured,sp=rr(95600,stage=5,schools=((96,1000),),offers=(96,),portal=False)
  records[sp]['scalar'][231]=3
 if loss=='committed':records[r]['scalar'][9]=5
 before=snap8(r);recover8();assert snap8(r)==before
 assert call(exports['DriTestHsRecoveryPending'],r,context)==0,loss
checks.append('Expired proposals never bypass changed eligibility, budget, roster capacity or completed commitment')
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
(OUTPUT/'test28r13_open_lists_results.json').write_text(json.dumps({'checks':checks,'open_cases':cases,'native_fallback_cases':native_cases,'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
