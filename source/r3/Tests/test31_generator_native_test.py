"""Captured weighted-draw and recruit-pool maximum consumer checks.

SDF getters/RNG draws are fixtures; captured instructions execute the boundaries.
The distinct RandomEnum property dispatch and full cohort creation are not run.
"""
from pathlib import Path
import os,sys,json,importlib.util,contextlib,io
ROOT=Path(__file__).resolve().parents[4]
CAPTURE=os.environ.get('DRI_CAPTURE_DIR');assert CAPTURE,'DRI_CAPTURE_DIR required'
OUTPUT=Path(os.environ.get('DRI_TEST_OUTPUT_DIR','.'));OUTPUT.mkdir(parents=True,exist_ok=True)
oldargv=sys.argv;sys.argv=[__file__,CAPTURE]
spec=importlib.util.spec_from_file_location('native29',Path(__file__).with_name('test29_supply_native.py'));n=importlib.util.module_from_spec(spec)
with contextlib.redirect_stdout(io.StringIO()):spec.loader.exec_module(n)
sys.argv=oldargv
vectors={}
for route in range(5):
 s=json.loads((ROOT/f'Runtime/Portal/Audit/Test31/Generator/route{route}_solution.json').read_text())
 g=json.loads((ROOT/f'Runtime/Portal/Audit/Test31/Generator/route{route}_graph.json').read_text());w=s['weight_changes']
 for key,v in s['nonelite_position_overrides'].items():vectors[tuple(v['weights'])]=f'route{route}:{key}:integer CDF algorithm'
 b=max(g['branches'],key=lambda b:b['weight']);vectors[tuple(w.get(str(z['ref']),z['weight'])for z in b['children'][0]['stars'])]=f'route{route}:star selector'
 vectors[tuple(w.get(str(b['pipelineRef']),b['weight'])for b in g['branches'])]=f'route{route}:pipeline selector'
draws=[dict(n.weighted_selection(list(v)),context=label)for v,label in vectors.items()]
# Captured RecruitingTunables maximum getter then 64-bit maximum used as cap.
def maximum_consumer(value):
 u=n.machine();record=0x10001000;table=0x10003000;vt=0x10004000;arr=0x10005000;mgr=0x10006000
 n.q(u,record+0x10,table);n.q(u,table,vt);n.q(u,vt+0x100,0x10009000)
 n.q(u,arr,0x1000a000);n.q(u,arr+8,0x1000a008);n.q(u,mgr+0x30,0x1000b000)
 u.reg_write(n.UC_X86_REG_RBX,record);u.reg_write(n.UC_X86_REG_R13,arr);u.reg_write(n.UC_X86_REG_RSI,mgr);u.reg_write(n.UC_X86_REG_RBP,0x1000c000)
 def hook(u,a,size,data):
  if a==0x8179c89:
   assert n.rr(u,n.UC_X86_REG_RDX)==62;u.reg_write(n.UC_X86_REG_RAX,62);u.reg_write(n.UC_X86_REG_RIP,a+5)
  elif a==0x8179ca2:
   assert n.rr(u,n.UC_X86_REG_R8)==62;u.reg_write(n.UC_X86_REG_RAX,value);u.reg_write(n.UC_X86_REG_RIP,a+3)
 u.hook_add(n.UC_HOOK_CODE,hook);u.emu_start(0x8179c81,0x8179cbc,count=100);assert n.rr(u,n.UC_X86_REG_R14)==value
 return {'value':value,'runtime_field':62,'register_max':n.rr(u,n.UC_X86_REG_R14)}
def pool_boundary(index,maximum):
 u=n.machine();u.reg_write(n.UC_X86_REG_RDX,index);u.reg_write(n.UC_X86_REG_R14,maximum);seen=[]
 def hook(u,a,size,data):
  if a in (0x8179e8d,0x817a30d):seen.append(a);u.emu_stop()
 u.hook_add(n.UC_HOOK_CODE,hook);u.emu_start(0x8179e84,0,count=3);assert seen==[0x8179e8d if index<maximum else 0x817a30d]
 return {'index':index,'maximum':maximum,'admitted_to_native_publication':index<maximum}
r={'status':'PASS_CAPTURED_NATIVE','native_code_sha256':n.CAPTURE_HASH,'weighted_vectors':draws,'pool_maximum_getter':[maximum_consumer(v)for v in(3200,3000)],'pool_boundaries':[pool_boundary(i,3000)for i in(0,2999,3000,3001)],'limits':['Record getters and RNG draw values are modeled services.','The captured weighted-selection algorithm is replayed with delivered integer weights; RandomEnum property-dispatch itself is not replayed here.','Full generation/quantile ordering/final star relabeling/live cohorts are not executed.','Minimum3000 and maximum3000 tuning fields are statically verified; this captured consumer independently proves the maximum publication bound.'],'live_game_verified':False}
(OUTPUT/'test31_generator_native_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'status':r['status'],'draws':sum(x['all_rng_draws']for x in draws),'vectors':len(draws),'pool_boundary_cases':4}))
