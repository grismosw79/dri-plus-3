"""Replay the actual rejected R21 rows through compiled R22 grade policy.
This proves policy admission only; native checks were not called in that log.
"""
from pathlib import Path
import argparse,ctypes,json,subprocess,collections
ap=argparse.ArgumentParser();ap.add_argument('--evidence',required=True);ap.add_argument('--source',required=True);ap.add_argument('--out',required=True);a=ap.parse_args()
out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
shim=out/'grade_probe.c';lib=out/'grade_probe.so'
shim.write_text('#include "car35_policy.h"\nint normal(int r,int p,int g){return car35_promotion_grade_allowed(0,r,p,g);}\nint tier(int r,int p,int g,int f){return car35_promotion_grade_at_floor(0,r,p,g,f);}\n')
subprocess.run(['cc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I',str(Path(a.source).resolve()),str(shim),'-o',str(lib)],check=True)
native=ctypes.CDLL(str(lib.resolve()))
for name,n in [('normal',3),('tier',4)]:getattr(native,name).argtypes=[ctypes.c_int]*n;getattr(native,name).restype=ctypes.c_int
data=json.loads(Path(a.evidence).read_text());counts=collections.Counter();builds=[]
for b in data['builds']:
    eligible={6:[],7:[],8:[]}
    for r in b['candidates']:
        args=(r['current_role'],r['prev_role'],r['prestige'])
        assert native.normal(*args)==r['logged_result']==0
        for floor in eligible:
            if native.tier(*args,floor):eligible[floor].append(r['coach_row']);counts[str(floor)]+=1
    builds.append({'build':b['build'],'school':b['school'],'completed':'returned_count'in b,'eligible_grade_only':eligible})
assert counts['6']==0 and counts['7']==101 and counts['8']==280
for floor in [-1,5,9,12,13]:assert native.tier(1,1,8,floor)==0
result={'passed':True,'observed_rejections':3459,'ordinary_policy_changes':0,'C_checks_newly_reaching_native':101,'Cminus_additional_checks_if_C_tier_empty':179,'counts_are_repeated_checks_not_unique_coaches':True,'native_outcome_known':False,'live_acceptance':False,'builds':builds}
(out/'logged-grade-replay.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='builds'},indent=2))
