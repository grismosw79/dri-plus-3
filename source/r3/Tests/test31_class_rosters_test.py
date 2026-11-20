"""Compiled calendar plan on all 138 supplied PRE scheme ideals over 100 years.
This models available normal class places; it does not predict live signings.
"""
from pathlib import Path
import os,subprocess,json,gzip,ctypes,hashlib
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
p=O/'class31_rosters.c';p.write_text('#include "class31_policy.h"\nint plan(const int*i,unsigned t,unsigned y,int*a){return class31_annual(i,t,y,a);}')
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared','-I'+str(S),str(p),'-o',str(O/'class31_rosters.so')],check=True)
f=ctypes.CDLL(str(O/'class31_rosters.so')).plan;A=ctypes.c_int*22;f.argtypes=[ctypes.POINTER(ctypes.c_int),ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_int)];f.restype=ctypes.c_int
fixture=S/'Tests/Fixtures/test31_pre_rosters.json.gz';data=json.loads(gzip.decompress(fixture.read_bytes()));pos='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
assert len(data['teams'])==138
national=[];bypos=[0]*22;teams=[]
for year in range(2026,2126):
 total=0
 for t in data['teams']:
  i=A(*(t['ideal'][p] for p in pos));out=A();n=f(i,t['tid'],year,out);values=list(out)
  assert 15<=n<=25 and n==sum(values) and not values[21],(t['tid'],year,n,values)
  assert all(v>=0 and (i[p]>0 or not v) for p,v in enumerate(values))
  check=A();assert f(i,t['tid'],year,check)==n and list(check)==values
  total+=n
  for p,v in enumerate(values):bypos[p]+=v
  if year==2026:teams.append({'tid':t['tid'],'team':t['name'],'target':n,'annual':dict(zip(pos,values)),'initial_roster':t['rosterCount'],'scheme_ideal':t['totalIdeal']})
 national.append(total)
expected=2667.3866666666668
assert abs(sum(national)/100-expected)<2.0
assert sum(x['target'] for x in teams)==national[0]
r={'passed':True,'input_provenance':data['inputs'],'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'teams':teams,'years':100,'national_mean':sum(national)/100,'school_mean':sum(national)/100/138,'national_range':[min(national),max(national)],'position_annual_means':dict(zip(pos,[v/100 for v in bypos])),'approved_model_annual_signings':expected,'scope':'Compiled normal-slot planning; initial rosters only reported, never used to set baseline demand','live_game_verified':False}
(O/'test31_class_rosters_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:v for k,v in r.items() if k!='teams'}))
