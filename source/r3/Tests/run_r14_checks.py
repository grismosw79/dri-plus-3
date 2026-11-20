"""R14 regression runner. Requires Zig 0.13.0, GCC, Unicorn 2.1.4,
pefile 2024.8.26, capstone 5.0.9 and the original supported executable capture.
Usage: python run_r14_checks.py --zig /path/to/zig --capture /path/to/capture
The native capture remains external; its recorded code-section SHA is checked.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor,as_completed
import argparse,subprocess,os,sys,shutil,hashlib,json,time
p=argparse.ArgumentParser();p.add_argument('--zig',required=True);p.add_argument('--capture',required=True);p.add_argument('--deps');a=p.parse_args()
S=Path(__file__).resolve().parents[1];O=S/'build/r14-checks';O.mkdir(parents=True,exist_ok=True)
C=Path(a.capture).resolve()
assert hashlib.sha256((C/'section_00_00001000.bin').read_bytes()).hexdigest()=='7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e'
assert subprocess.check_output([a.zig,'version'],text=True).strip()=='0.13.0'
base=[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(S),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']
subprocess.run(base+[str(S/n)for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(O/'retention_fixture.dll'),'-Wl,--dynamicbase','-Wl,--nxcompat'],check=True)
names='''test28r14_crowding_test test28r14_offseason_test test28r14_native_admit_test test28r14_nsd_cleanup_test test28r14_saved_rosters_test test28r13r1_continuity_test test28r12_eps_cadence_test test28r9_native_pipeline_test test28r8_hs_rival_cleanup_test test28r8_funding_published_test test28r8_native_budget_test test28r8_postfunding_hook_test test28r6_nsd_pairs_test hs_ownership_test hard_commit_ownership_test test28r13_entry_abi_test hook_entry_test test28r14_policy_test'''.split()
env=dict(os.environ,DRI_CAPTURE_DIR=str(C),DRI_TEST_INSTRUCTION_LIMIT='250000000')
if a.deps:env['PYTHONPATH']=str(Path(a.deps).resolve())+os.pathsep+env.get('PYTHONPATH','')
def run(n):
 out=O/n;out.mkdir(exist_ok=True);shutil.copy2(O/'retention_fixture.dll',out/'retention_fixture.dll');start=time.time()
 r=subprocess.run([sys.executable,str(S/'Tests'/f'{n}.py')],env=dict(env,DRI_TEST_OUTPUT_DIR=str(out)),capture_output=True,text=True,timeout=900)
 (out/'test.log').write_text(r.stdout+r.stderr);print('PASS'if not r.returncode else'FAIL',n,flush=True)
 return {'name':n,'returncode':r.returncode,'seconds':round(time.time()-start,2)}
with ThreadPoolExecutor(max_workers=3)as ex:results=[j.result()for j in as_completed([ex.submit(run,n)for n in names])]
(O/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(r['returncode']for r in results))
