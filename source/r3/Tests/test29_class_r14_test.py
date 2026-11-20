"""ActualR14 W0/NSD source-quantity feasibility through compiledclasspolicy.
This records all137CPU schools including28small/3largeNSDclasses; notlivewins.
"""
from pathlib import Path
import os,subprocess,json,gzip,ctypes as C
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
source='#include "test29_class.h"\nvoid calculate(const Test29ClassInput *in,Test29ClassPlan *out){test29_class_plan(in,out);}\n'
(O/'class-bridge.c').write_text(source)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(S),str(O/'class-bridge.c'),'-o',str(O/'class-bridge.so')],check=True)
class Input(C.Structure):
 _fields_=[(x,C.c_int*22)for x in ['ideal','returning','secured','junior','vacancy','optional','candidates','protected_vacancy','protected_optional','optional_value']]+[(x,C.c_int)for x in ['general','academy','annual_valid','annual_target']]
class Output(C.Structure):
 _fields_=[(x,C.c_int)for x in ['target','secured','replacement','remaining','available','planned','shortfall','replacement_exception','protected_over_target','general']]+[(x,C.c_int*22)for x in ['vacancy','optional']]
dll=C.CDLL(str(O/'class-bridge.so'));dll.calculate.argtypes=[C.POINTER(Input),C.POINTER(Output)]
fixture=json.loads(gzip.decompress((S/'Tests/Fixtures/test29_class_r14.json.gz').read_bytes()));results=[]
assert len(fixture['cases'])==274
for case in fixture['cases']:
 i=Input();o=Output();i.academy=case['tid']in[0,7,55]
 for k in ['ideal','returning','secured','junior']:
  for p,v in enumerate(case[k]):getattr(i,k)[p]=v
 for p in range(22):i.vacancy[p]=i.candidates[p]=max(0,case['ideal'][p]-case['count'][p])
 dll.calculate(C.byref(i),C.byref(o))
 assert o.secured==sum(case['secured']) and o.available==sum(i.vacancy)
 assert o.planned<=o.available and o.planned<=o.remaining
 assert o.shortfall==max(0,o.remaining-o.planned)
 assert sum(o.vacancy)+sum(o.optional)==o.planned
 assert (15<=o.target<=25)or(i.academy and o.replacement_exception)
 results.append({k:case[k]for k in ['phase','tid','school','actual_R14_HS_class','actual_R14_projected_total']}|{k:getattr(o,k)for k in ['target','secured','replacement','remaining','available','planned','shortfall','replacement_exception']})
nsd=[x for x in results if x['phase']=='NSD'];small=[x for x in nsd if x['actual_R14_HS_class']<15];large=[x for x in nsd if x['actual_R14_HS_class']>25]
assert len(small)==28 and len(large)==3
assert sum(x['actual_R14_projected_total']>=85 for x in small)==20
assert sum(x['available']<15 for x in results if x['phase']=='W0')==24
(O/'test29_class_r14_results.json').write_text(json.dumps({'inputs':fixture['inputs'],'method':fixture['method'],'cases':results,'all28_small_and3_large_included':True,'W0_teams_with_less_than15_quantity_vacancies':24,'live_game_verified':False},indent=2)+'\n')
print('PASS actualR14 source-feasibility:274phase/schoolcases;28small3large;24startwithfewerthan15quantityvacancies')
