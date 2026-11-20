"""Execute compiled grade gate at pre-fallback and reconstruction boundaries."""
from car35_fixture import *
checks.clear()
count=0
for current in [1,2,255]:
 for previous in [1,2]:
  for status in [0,2,4,7]:
   for user in [0,1]:
    for grade in list(range(14))+[-1,90]:
     source=current if current in [1,2]else previous
     c=coach(**{'0':user,'12':status,'16':current,'17':previous,'112':grade,'134':source-1})
     snapshot=dict(records[c]['scalar'])
     assert call('DriTestCar35Eligible',c,0,0)==int(0<=grade<=6)
     assert call('DriTestCar35Eligible',c,0,source)==1
     assert call('DriTestCar35Eligible',c,0,3-source)==0
     assert records[c]['scalar']==snapshot
     count+=1
check(f'{count} grade/status/user/current-or-last-role combinations: C+ inclusive, C and worse/invalid rejected; coordinator jobs remain side-specific; no record writes')
for grade in [-1,6,7,12,13]:
 for previous in [1,2,255]:
  c=coach(**{'16':0,'17':previous,'112':grade,'134':0})
  assert call('DriTestCar35Eligible',c,0,0)==1
check('Existing HCs remain eligible for HC mobility regardless of previous coordinator role or grade')
config(0);c=coach(**{'16':1,'112':12,'134':0});snapshot=dict(records[c]['scalar'])
assert call('DriTestCar35Eligible',c,0,0)==1 and records[c]['scalar']==snapshot
config();check('Observe mode does not apply the proposed eligibility restriction')

native_calls=[]
api(lambda:(native_calls.append('broad')or 1),G+0x8330e30)
api(lambda:(native_calls.append('reconstruct')or 1),G+0x834a8e0)
m=record('module');t=record('destination',{383:20});tr=ref(t);old=ref(0)
reconstruct=call('DriTestCar35Address',1)
for status in [0,2,4,7]:
 for user in [0,1]:
  c=coach(**{'0':user,'12':status,'16':2,'112':7,'134':1});cr=ref(c)
  for emergency in [0,1]:
   before=len(native_calls)
   assert call('car35_broad_hook',m,cr,tr,old,6,emergency,0)==0
   assert len(native_calls)==before
  before=len(native_calls)
  assert call(reconstruct,m,tr,cr,0,0x111,0x222,0x333)==0 and len(native_calls)==before
  records[c]['scalar'][112]=6
  for emergency in [0,1]:assert call('car35_broad_hook',m,cr,tr,old,6,emergency,0)==1
  assert call(reconstruct,m,tr,cr,0,0x111,0x222,0x333)==1
check('Normal/emergency builder flags cannot bypass the gate; CPU/user, signed/pending-fire/pending-renewal/free-agent reconstruction rejects before native mutation and passes C+')

# A serialized copy does not lose the promotion restriction.
records[c]['scalar'].update({12:7,8:255,16:2,17:2,112:8})
copy=coach(**json.loads(json.dumps(records[c]['scalar'])))
assert call('DriTestCar35Eligible',copy,0,0)==0
assert call('DriTestCar35Eligible',copy,0,2)==1
check('Firing/free agency and record reload preserve the C+ rule without a process-local registry')

# The direct assignment route signs the current role; it does not promote by
# changing a destination argument. It must still permit an F-grade OC as OC
# and a current HC as HC. Actual user promotion uses role=HC reconstruction.
api(lambda:(native_calls.append('direct_user')or 1),G+0x8830ae0)
direct=call('DriTestCar35Address',2)
records[c]['scalar'].update({0:1,16:1,17:1,112:12,134:0})
assert call(direct,m,cr,tr)==1
before=len(native_calls)
assert call(reconstruct,m,tr,cr,0,0,0,0)==0 and len(native_calls)==before
records[c]['scalar'][16]=0
assert call(direct,m,cr,tr)==1
check('Direct same-role user assignment remains valid; a user coordinator cannot obtain an HC reconstruction below C+')
assert_refs()
result={'passed':True,'checks':checks,'compiled_callbacks':True,'new_minimum':'Cplus','minimum_enum':6,
 'accepted_offer_retention_unchanged':True,'no_hiring_score_changes':True,'live_acceptance':False}
(ROOT/'promotion_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
