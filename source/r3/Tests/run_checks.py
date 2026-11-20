"""Build and validate the exact Test28 tree. No live game is started.
Usage: python run_checks.py --zig /path/to/zig --capture /path/to/native-capture --deps /path/to/python-modules --baseline-source /path/to/original-Test15/Source --r7-package /path/to/DRI-Test28R8-HS-Competition
"""
from pathlib import Path
import argparse,subprocess,os,sys,json,hashlib,time
p=argparse.ArgumentParser();p.add_argument('--zig',required=True);p.add_argument('--capture',required=True);p.add_argument('--deps');p.add_argument('--r7-package',required=True,help='Immutable delivered R8 package for unchanged FTC/save comparison (legacy argument name)');p.add_argument('--baseline-source',required=True,help='Original Test15 Runtime/Portal/Source directory for the negative control');a=p.parse_args()
S=Path(__file__).resolve().parents[1];R=S.parent;B=S/'build';B.mkdir(exist_ok=True);A=R/'Audit';A.mkdir(exist_ok=True);PACKAGE=R.parents[1]
R7PACKAGE=Path(a.r7_package).resolve();BASELINE=Path(a.baseline_source).resolve();CAPTURE=Path(a.capture).resolve();CONTROL=B/'baseline';CONTROL.mkdir(exist_ok=True)
assert (BASELINE/'retention.c').is_file() and (BASELINE/'hooks.S').is_file()
env=dict(os.environ,DRI_CAPTURE_DIR=str(Path(a.capture).resolve()),DRI_TEST15_BASELINE_DLL=str(CONTROL/'retention_fixture.dll'))
if a.deps:env['PYTHONPATH']=str(Path(a.deps).resolve())+os.pathsep+env.get('PYTHONPATH','')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def inputs():
 files=[p for p in S.rglob('*') if p.is_file() and not any(x in ('build','__pycache__','deps') for x in p.relative_to(S).parts) and p.name!='dll_hash.h']
 files += list((PACKAGE/'Audit/Test28R6').glob('*.json'))
 files += list((PACKAGE/'Audit/Test28R7').glob('*.json'))
 files += [p for p in (PACKAGE/'Audit/Test28R8').rglob('*') if p.is_file() and '__pycache__' not in p.parts]
 files += [PACKAGE/'Prepared-Save/DYNASTY-TEST28R8START',PACKAGE/'Runtime/Start-Portal-Checked.ps1',PACKAGE/'Runtime/Reserve20/ReservationPatch.cs']
 files += list(A.glob('*cases.json'))
 files += list((A/'Test27Fault').glob('*.json'))
 files += list((A/'Test27R2').glob('*.json'))
 files += [p for p in (A/'Test28R2').glob('*') if p.is_file()]
 files += [p for p in (A/'Test28R3').glob('*') if p.is_file()]
 files += [p for p in (A/'Test28R4').rglob('*') if p.is_file()]
 files += [p for p in (A/'Test28R5').rglob('*') if p.is_file()]
 files += [p for p in (A/'Test28R6').rglob('*') if p.is_file()]
 files += [p for p in (A/'Test28R7').rglob('*') if p.is_file()]
 files += [p for p in (A/'Test28R8').rglob('*') if p.is_file()]
 files += [p for p in (A/'PT28').rglob('*') if p.is_file() and '__pycache__' not in p.parts]
 files += list((A/'Test26NegativeControlSource').glob('*'))
 files += [p for p in (A/'Test25Replay').rglob('*') if p.is_file() and '__pycache__' not in p.parts]
 files += [p for p in (A/'Test24Replay').rglob('*') if p.is_file() and not any(x in ('build','__pycache__') for x in p.relative_to(A/'Test24Replay').parts)]
 files += [p for p in (A/'Test28R10').rglob('*') if p.is_file() and '__pycache__' not in p.parts]
 files += [p for p in (A/'Test28R11/NegativeControlSource').iterdir() if p.is_file()]
 files += [p for p in (A/'Test28R12/NegativeControlSource').iterdir() if p.is_file()]
 files += [A/'Test28R9/verify_build.py']
 files += [A/'BuildSupport/verify_build.py',A/'BuildSupport/toolchain.json',A/'BuildSupport/capture_verification.json']
 files += [p for p in (A/'EPS1/Replay').rglob('*') if p.is_file() and p.suffix in ('.py','.json') and not p.name.endswith('-validation.json')]
 # FTC inputs and captured requirement fixtures are tested with this DLL and
 # must remain frozen even though they live at the package level.
 files += [PACKAGE/f'frantkdata/{route}/dynasty-tuning-binary.FTC' for route in range(5)]
 files += [PACKAGE/f'Audit/Test28/ftc-route-{route}.json' for route in range(5)]
 files += [p for p in (PACKAGE/'Audit/Test28/NativeRequirementFixture').rglob('*') if p.is_file()]
 files += [p for p in (PACKAGE/'Audit/Test28/BooleanABI').rglob('*') if p.is_file()]
 return {os.path.relpath(p,R):sha(p) for p in sorted(files)}
def baseline_inputs():
 return {p.name:sha(p) for p in sorted(BASELINE.iterdir()) if p.is_file() and p.suffix in ('.c','.h','.inc','.S')}
def r7_inputs():
 return {f'frantkdata/{route}/dynasty-tuning-binary.FTC':sha(R7PACKAGE/f'frantkdata/{route}/dynasty-tuning-binary.FTC') for route in range(5)}
def capture_inputs():
 metadata=json.loads((A/'BuildSupport/capture_verification.json').read_text())
 expected={x['file']:x['sha256'] for x in metadata['sections']}
 expected['pe_headers.bin']=metadata['pe_header_sha256']
 actual={name:sha(CAPTURE/name) for name in expected}
 assert actual==expected,'Native capture does not match verified original bytes'
 report=json.loads((CAPTURE/'report.json').read_text(encoding='utf-8-sig'))
 assert report['ExecutableSha256']==metadata['executable_sha256']
 actual['report.json']=sha(CAPTURE/'report.json')
 return actual
started=time.time_ns();fingerprints=inputs();baseline_fingerprints=baseline_inputs();capture_fingerprints=capture_inputs();r7_fingerprints=r7_inputs();steps=[]
(A/'validation_run.json').write_text(json.dumps({'release':'Test28R12','started_ns':started,'fingerprints':fingerprints,'baseline_source':str(BASELINE),'baseline_fingerprints':baseline_fingerprints,'capture_fingerprints':capture_fingerprints,'r7_package':str(R7PACKAGE),'r7_fingerprints':r7_fingerprints,'live_game_verified':False},indent=2)+'\n')
(A/'offline_checks.json').unlink(missing_ok=True)
(A/'offline_failure.json').unlink(missing_ok=True)
(A/'build_verification.json').unlink(missing_ok=True)
for p in B.glob('*results.json'):p.unlink()
def run(name,cmd):
 print('CHECK',name,flush=True)
 t=time.time();x=subprocess.run(cmd,env=env,capture_output=True,text=True);(A/(name+'.log')).write_text(x.stdout+x.stderr)
 if x.returncode:
  (A/'offline_failure.json').write_text(json.dumps({'failed':name,'returncode':x.returncode,'completed':steps,'live_game_verified':False},indent=2)+'\n')
  print(x.stdout[-4000:]+x.stderr[-4000:]);raise SystemExit(x.returncode)
 steps.append({'check':name,'passed':True,'seconds':round(time.time()-t,3),'log_sha256':sha(A/(name+'.log'))})
run('r9_ftc_unchanged',[sys.executable,str(S/'Tests/test28r9_ftc_unchanged.py'),str(R7PACKAGE),str(PACKAGE)])
run('build',[sys.executable,str(S/'build.py'),a.zig])
run('eps_portable',[sys.executable,str(S/'Tests/run_eps_checks.py')])
run('test28r2_eps_reconciliation',[sys.executable,str(S/'Tests/test28r2_eps_reconciliation.py')])
run('pt28_core_test',[sys.executable,str(S/'Tests/pt28_core_test.py')])
run('staff28_core_test',[sys.executable,str(S/'Tests/staff28_core_test.py')])
run('test28_ftc_native_requirements',[sys.executable,str(S/'test28_ftc_native_requirements.py')])
run('pp28_core_build',['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(S/'pp28_core.c'),str(S/'Tests/pp28_core_test.c'),'-o',str(B/'pp28_core_test')])
run('pp28_core_test',[str(B/'pp28_core_test')])
run('fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(S),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST',str(S/'retention.c'),str(S/'eps_core.c'),str(S/'eps_collect.c'),str(S/'eps_snapshot.c'),str(S/'pt28_core.c'),str(S/'pp28_core.c'),str(S/'staff28_core.c'),str(S/'hooks.S'),'-o',str(B/'retention_fixture.dll')])
R3=A/'Test28R4/NegativeControlSource';R3OUT=B/'r3_control';R3OUT.mkdir(exist_ok=True)
run('r3_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R3),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R3/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R3OUT/'retention_fixture.dll')])
env['DRI_TEST28R3_CONTROL_DLL']=str(R3OUT/'retention_fixture.dll')
R4=A/'Test28R5/NegativeControlSource';R4OUT=B/'r4_control';R4OUT.mkdir(exist_ok=True)
run('r4_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R4),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R4/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R4OUT/'retention_fixture.dll')])
env['DRI_TEST28R4_CONTROL_DLL']=str(R4OUT/'retention_fixture.dll')
R6=A/'Test28R7/NegativeControlSource';R6OUT=B/'r6_control';R6OUT.mkdir(exist_ok=True)
run('r6_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R6),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R6/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R6OUT/'retention_fixture.dll')])
env['DRI_TEST28R6_CONTROL_DLL']=str(R6OUT/'retention_fixture.dll')
R7=A/'Test28R8/NegativeControlSource';R7OUT=B/'r7_control';R7OUT.mkdir(exist_ok=True)
run('r7_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R7),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R7/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R7OUT/'retention_fixture.dll')])
env['DRI_TEST28R7_CONTROL_DLL']=str(R7OUT/'retention_fixture.dll')

R9=A/'Test28R10/NegativeControlSource';R9OUT=B/'r9_control';R9OUT.mkdir(exist_ok=True)
run('r9_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R9),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R9/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R9OUT/'retention_fixture.dll')])
env['DRI_TEST28R9_CONTROL_DLL']=str(R9OUT/'retention_fixture.dll')

R10=A/'Test28R11/NegativeControlSource';R10OUT=B/'r10_control';R10OUT.mkdir(exist_ok=True)
run('r10_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R10),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R10/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R10OUT/'retention_fixture.dll')])
env['DRI_TEST28R10_CONTROL_DLL']=str(R10OUT/'retention_fixture.dll')

R11=A/'Test28R12/NegativeControlSource';R11OUT=B/'r11_control';R11OUT.mkdir(exist_ok=True)
run('r11_negative_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(R11),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']+[str(R11/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]+['-o',str(R11OUT/'retention_fixture.dll')])
env['DRI_TEST28R11_CONTROL_DLL']=str(R11OUT/'retention_fixture.dll')

run('baseline_fixture_build',[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(BASELINE),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST',str(BASELINE/'retention.c'),str(BASELINE/'hooks.S'),'-o',str(CONTROL/'retention_fixture.dll')])
cases=[('test26_exceptions','test26_exceptions_test.c'),('quality_policy','quality_policy_test.c'),('depth_plan','depth_plan_test.c'),('quality_order','quality_order_test.c'),('market_plan','market_plan_test.c'),('hs_policy','policy_test.c'),('test21_depth','test21_depth_test.c'),('hs15_policy_test','hs15_policy_test.c'),('test15r1_log_monitor_test','test15r1_log_monitor_test.c'),('test15r1_launcher_monitor_test','test15r1_launcher_monitor_test.c'),('test24_budget','test24_budget_test.c'),('test25_policy','test25_policy_test.c')]
cases.insert(0,('test28r6_pairs','test28r6_pairs_test.c'))
cases.insert(0,('test28r5_shared_policy','test28r5_shared_policy_test.c'))
cases.insert(0,('db28_requirements_policy','db28_requirements_policy_test.c'))
for t in sorted(list((S/'Tests').glob('test28r8*_test.c'))+list((S/'Tests').glob('test28r9*_test.c'))+list((S/'Tests').glob('test28r10*_test.c'))+list((S/'Tests').glob('test28r12*_test.c'))):
 cases.append((t.stem.removesuffix('_test'),t.name))
for name,source in cases:
 run(name+'_build',['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(S),str(S/'Tests'/source),'-o',str(B/name)])
 run(name,[str(B/name)])
python_cases=['test27r2_hs_finalize_test','test27r1_published_owner_test','eps_runtime_test','eps_cache_test','test26r1_nsd_native_test','test26_fallback_machine_test','test26_nsd_test','hook_entry_test','retention_loop_test','branch_join_test','hook_machine_test','portal_hook_test','integrity_hook_test','quality_hook_test','allocation_hook_test','destination_search_test','allocation_runtime_test','signing_ownership_test','hard_commit_ownership_test','hs_ownership_test','bulk_ownership_test','bulk_guard_test','recovery_runtime_test','portal_cleanup_test','funding_runtime_test','late_recovery_test','projection_runtime_test','quality_order_runtime_test','market_race_runtime_test','hs_recovery_test','test24_budget_runtime_test','test25_commitment_test','test25_hs_coverage_test']
python_cases[:0]=['test28r7_permutation_test','test28r6_pairs_runtime_test','test28r6_nsd_pairs_test','test28r5_negative_control','test28r5_hs_budget_test','test28r4_nsd_collection_test','test28r3_eps_publication_test','pp28_refresh_test','pt28_calendar_test','db28_requirements_runtime_test','pt28_runtime_test','pp28_runtime_test','staff28_runtime_test']
for t in sorted(list((S/'Tests').glob('test28r8*_test.py'))+list((S/'Tests').glob('test28r9*_test.py'))+list((S/'Tests').glob('test28r10*_test.py'))+list((S/'Tests').glob('test28r11*_test.py'))+list((S/'Tests').glob('test28r12*_test.py'))):
 if t.stem not in python_cases:python_cases.append(t.stem)
for t in sorted((S/'Tests').glob('*15*test.py')):
 if t.stem not in python_cases:python_cases.append(t.stem)
# Long saved-roster replay is isolated; overlap it with the other independent gates.
python_cases.remove('test15r1_real_rosters_test')
python_cases.insert(0,'test15r1_real_rosters_test')
from concurrent.futures import ThreadPoolExecutor,as_completed
import shutil

def python_check(name):
 directory=B/'isolated'/name;directory.mkdir(parents=True,exist_ok=True)
 shutil.copy2(B/'retention_fixture.dll',directory/'retention_fixture.dll')
 childenv=dict(env,DRI_TEST_OUTPUT_DIR=str(directory));started_at=time.time()
 x=subprocess.run([sys.executable,str(S/'Tests'/(name+'.py'))],env=childenv,capture_output=True,text=True)
 return name,directory,x,started_at
python_failures=[]
with ThreadPoolExecutor(max_workers=6) as executor:
 for future in as_completed([executor.submit(python_check,name) for name in python_cases]):
  name,directory,x,started_at=future.result();(A/(name+'.log')).write_text(x.stdout+x.stderr)
  if x.returncode:
   (A/'offline_failure.json').write_text(json.dumps({'failed':name,'returncode':x.returncode,'completed':steps,'live_game_verified':False},indent=2)+'\n')
   print(name,x.stdout[-3000:]+x.stderr[-3000:],flush=True)
   python_failures.append(name);continue
  for result in directory.glob('*results.json'):shutil.copy2(result,B/result.name)
  steps.append({'check':name,'passed':True,'seconds':round(time.time()-started_at,3),'log_sha256':sha(A/(name+'.log'))})
  print('PASS',name,flush=True)
if python_failures:raise SystemExit('Failed runtime gates: '+', '.join(python_failures))
run('allocation_replay_bridge',['gcc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',str(S/'Tests/allocation_replay_bridge.c'),'-o',str(B/'allocation_replay.so')])
run('replay_bridge',['gcc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',str(S/'Tests/quality_replay_bridge.c'),'-o',str(B/'quality_replay.so')])
run('baseline23_bridge',['gcc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',str(A/'Test24Replay/Baseline23/Tests/allocation_replay_bridge.c'),'-o',str(B/'baseline23.so')])
run('test23_frozen_replay',[sys.executable,str(A/'Test24Replay/compare.py'),'--old',str(B/'baseline23.so'),'--new',str(B/'allocation_replay.so'),'--out',str(A)])
run('baseline24_bridge',['gcc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',str(A/'Test25Replay/Baseline24/Tests/allocation_replay_bridge.c'),'-o',str(B/'baseline24.so')])
run('test24_frozen_replay',[sys.executable,str(A/'Test25Replay/compare.py'),'--old',str(B/'baseline24.so'),'--new',str(B/'allocation_replay.so'),'--out',str(A)])
assert fingerprints==inputs(),'Source, tests or replay inputs changed during validation; rerun exact final tree'
assert baseline_fingerprints==baseline_inputs(),'Original Test15 negative-control source changed during validation'
assert capture_fingerprints==capture_inputs(),'Native capture changed during validation'
assert r7_fingerprints==r7_inputs(),'R8 FTC baseline changed during validation'
(A/'offline_failure.json').unlink(missing_ok=True)
(A/'offline_checks.json').write_text(json.dumps({'steps':steps,'status':'PASS_OFFLINE_LIVE_PENDING','release':'Test28R12','live_game_verified':False,'fingerprints':fingerprints,'baseline_source':str(BASELINE),'baseline_fingerprints':baseline_fingerprints,'capture_fingerprints':capture_fingerprints,'r7_package':str(R7PACKAGE),'r7_fingerprints':r7_fingerprints,'baseline_fixture_sha256':sha(CONTROL/'retention_fixture.dll'),'started_ns':started,'completed_ns':time.time_ns()},indent=2)+'\n')
print('PASS: all offline gates; live game test still required',flush=True)
