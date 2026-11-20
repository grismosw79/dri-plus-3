"""Rebuild Test32 and run checks against a single frozen source/fixture set.

Use the exact Zig 0.13.0 and supported TU1029 captures. --baseline is the
preserved Test30 package; --legacy-baseline is preserved TU1029R5 for historical
negative controls. --test31-baseline is immutable Test31R1; all 20 FTCs must match it.
The optional secondary capture reuses the same compiled fixture without rebuilding.
No recorded or emulated pass establishes live game behavior.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
import argparse, os, sys, subprocess, hashlib, json, shutil, time
A=Path(__file__).resolve().parent
S=A.parent
R=S.parents[2]
DEFAULT='''tu1029r5_elite_policy_test tu1029r5_elite_runtime_test tu1029r5_pool_test
test31_class_policy_test test29_class_r14_test test29_class_rosters_test test29_class_source_test
test29r2_class_continuity_test test29r2_class_recovery_test
test29_market_core_test test30_market_core_test test30_market_hs_test test30_market_portal_test test29r2_market_roles_test test29r2_market_execution_test
test28r8_score_abi_test test28r9_native_pipeline_test test28r8_native_budget_test test28r8_postfunding_hook_test
test28r8_funding_published_test test28r14_native_admit_test test28r14_offseason_test
tu1029r5_accounting_test tu1029r5_pair_regression_test hs_ownership_test hard_commit_ownership_test test29r1_hs_flex_test
test29r1_capacity_test test28r6_nsd_pairs_test test28r14_nsd_cleanup_test test28r8_hs_rival_cleanup_test portal_cleanup_test
test29r4_quality_depth_runtime_test test29r4_quality_depth_policy_test test29_portal_class_boundary_test
test29r3_academy_test test29r3_return_native_test
test31_compat_test hook_entry_test tu1029r3_open_list_test test28r12_eps_cadence_test test31_launcher_test
test29_market_portal_score_test test29r1_qb_test test29r1_recovery_funding_test
test29r2_acceptance_policy_test test29r4_quality_depth_native_test
test29_portal_execution_test test29_portal_witness_test test29r2_portal_retry_native_test
tu1029r4_eps_publication_test tu1029r4_eps_runtime_test tu1029r4_eps_sources_test'''.split()

SUPERSEDED={
 'test29_market_hs_test': {'replacement':['test30_market_hs_test'], 'reason':'Test30 hard16 cap with actual-offer grandfather controls; retained unchanged'},
 'test29_market_portal_test': {'replacement':['test30_market_portal_test'], 'reason':'Test30 hard10 cap with actual-offer grandfather controls; retained unchanged'},
 'test29r4_launcher_test': {'replacement':['test31_launcher_test'], 'reason':'Same monitor protections under current Test31 label'},
 'test30_launcher_test': {'replacement':['test31_launcher_test'], 'reason':'Current release identity and monitor protections'},
 'test30_compat_test': {'replacement':['test31_compat_test'], 'reason':'Test30 preserved signatures plus explicitly approved Test31 guards/hooks'},
 'test29_class_policy_test': {'replacement':['test31_class_policy_test','test31_class_runtime_test','test31_class_rosters_test'], 'reason':'Annual class composition intentionally replaces the full-room two-recruit plan and academy40 rule'},
 'test29_class_rosters_test': {'replacement':['test31_class_rosters_test','test31_class_roster_runtime_test'], 'reason':'Annual composition replaces vacancy-only ceiling; 138-school scheme census and actual saved-roster collector checks retained'},
 'test29_class_source_test': {'replacement':['test31_class_source_test'], 'reason':'Annual source map replaces 20-primary cap; native funding conservation, byte identity, index permutation, complete-board accounting and user controls retained'},
 'test29r2_class_recovery_test': {'replacement':['test31_class_recovery_test'], 'reason':'Annual demand replaces full-room/W8-only veto; native eligibility, score, hours, user, deadline, admission and no-fabricated-offer controls retained'},
 'test29r2_class_continuity_test': {'replacement':['test31_class_continuity_test'], 'reason':'Annual HS slot stays normal after portal arrival; exact native score totals, eligibility refusal, paid-source persistence and reset-hour controls retained'},
 'tu1029r5_elite_runtime_test': {'replacement':['test31_elite_runtime_test'], 'reason':'Explicit annual-quota fixtures replace shared-budget assumptions; native eligibility, user, quality thresholds, capacity, firmness, cleanup and NSD no-write controls retained'},
}
SUPERSEDED['test31_launcher_test']={
 'replacement':['test32_launcher_test'],
 'reason':'Same monitor assertions with Test32 identity; previous-release refusal remains tested'}
SUPERSEDED['test31_offer_action_native_test']={
 'replacement':['test32_offer_action_native_test'],
 'reason':'Unused forecast identities no longer consume executed HS offer capacity; same native ABI/payment, live16 CPU offers, user and portal hard10 boundaries retained'}
SUPERSEDED['test30_market_hs_test']={
 'replacement':['test32_market_hs_test'],
 'reason':'Executed live offers replace frozen standby exclusivity for HS entry; revised whole-market tests retain eligible selection, scarcity, grandfathered offers, capacity, conservation, reload/order and user controls'}
SUPERSEDED['test31_generator_model_test']={
 'replacement':['test31r1_generator_model_test'],
 'reason':'FTC-Master-R03 includes the documented header-offset repair; retained R1 verifier checks exact one-word delta plus unchanged full decoded model against released Test31. Old Test31 packed-hash ledger is intentionally obsolete.'}
for _spec in SUPERSEDED.values():
 _spec['replacement']=[{'test31_launcher_test':'test32_launcher_test','test30_market_hs_test':'test32_market_hs_test'}.get(n,n) for n in _spec['replacement']]
SUFFIXES={'.c','.h','.inc','.s','.py','.json','.gz','.cjs'}
EXCLUDED={'.git','build','__pycache__'}
def sha(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    return digest.hexdigest()
def tree_hashes(root, suffixes=None):
    return {str(p.relative_to(root)):sha(p) for p in sorted(root.rglob('*'))
            if p.is_file() and not EXCLUDED.intersection(p.relative_to(root).parts)
            and (suffixes is None or p.suffix.lower() in suffixes)}
def source_hashes():return tree_hashes(S,SUFFIXES)
def ftc_hashes():
    return {str(p.relative_to(R)):sha(p) for p in sorted(R.rglob('*'))
            if p.is_file() and p.suffix.lower()=='.ftc' and 'Audit' not in p.relative_to(R).parts}
def checked_directory(path,label):
    path=path.resolve()
    if not path.is_dir():raise SystemExit(label+' directory not found: '+str(path))
    return path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--zig',type=Path,required=True)
p.add_argument('--capture',type=Path,required=True)
p.add_argument('--baseline',type=Path,required=True,help='Preserved Test30 package root')
p.add_argument('--test31-baseline',type=Path,required=True,help='Preserved Test31R1 package root; FTC byte-preservation control')
p.add_argument('--secondary-capture',type=Path,help='Optional second supported native capture, using the identical fixture')
p.add_argument('--secondary-tests',nargs='+',help='Explicit second-capture tests; default is ten inherited checks plus four Test32 native/market checks')
p.add_argument('--legacy-baseline',type=Path,help='Preserved TU1029R5 package root; required for the full suite')
p.add_argument('--output',type=Path,required=True,help='New directory outside the package and both baselines')
p.add_argument('--deps',type=Path)
p.add_argument('--jobs',type=int,default=4)
p.add_argument('--tests',nargs='+',help='Explicit subset; never reported as full-suite validation')
p.add_argument('--expected-hook-count',type=int,required=True)
p.add_argument('--expected-saved-hook-count',type=int,required=True)
p.add_argument('--log-csv',type=Path,help='Optional actual Test30 CSV for logging replay')
a=p.parse_args()
if not 1<=a.jobs<=4:p.error('--jobs must be between 1 and 4')
zig=a.zig.resolve()
if subprocess.check_output([str(zig),'version'],text=True).strip()!='0.13.0':
    raise SystemExit('Only Zig 0.13.0 is supported; no build attempted')
cap=checked_directory(a.capture,'Capture')
base=checked_directory(a.baseline,'Test30 baseline')
base31=checked_directory(a.test31_baseline,'Test31R1 baseline')
secondary=checked_directory(a.secondary_capture,'Secondary capture') if a.secondary_capture else None
legacy=checked_directory(a.legacy_baseline,'TU1029R5 baseline') if a.legacy_baseline else None
if not a.tests and legacy is None:p.error('Full validation requires --legacy-baseline TU1029R5')
tests=list(dict.fromkeys(a.tests or DEFAULT+[q.stem for q in sorted((S/'Tests').glob('test30_*_test.py'))]
                         +[q.stem for q in sorted((S/'Tests').glob('test31_*_test.py'))]
                         +[q.stem for q in sorted((S/'Tests').glob('test32_*_test.py'))]
                         +['test31r1_generator_model_test']))
if not a.tests:tests=[n for n in tests if n not in SUPERSEDED]
missing=[n for n in tests if not (S/'Tests'/f'{n}.py').is_file()]
if missing:raise SystemExit('Missing required checks: '+repr(missing))
if not a.tests:
    missing_replacements=sorted({n for v in SUPERSEDED.values() for n in v['replacement'] if n not in tests})
    if missing_replacements:raise SystemExit('Missing explicit replacement checks: '+repr(missing_replacements))
out=a.output.resolve()
for protected in [R,base,base31]+([legacy] if legacy else []):
    if out==protected or protected in out.parents:
        p.error('--output must be outside package and baseline directories to freeze their contents')
out.mkdir(parents=True,exist_ok=False)
approved={
 '8247e6dcc68b78672ac1c79e2deb41900925e6303a71c1e0ace02ef8602b3be2':'7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e',
 '99ac5da70511c0c8ec5f8677d0e847e6443e1bfa56b22842228edae4723387f8':'f04870ba6c6490b6353157d0bd7fddbbb615413635b7861cfe357b28308899cf',
}
report=json.loads((cap/'report.json').read_text(encoding='utf-8-sig'))
if not report['Complete'] or report['ExecutableSha256'] not in approved:
    raise SystemExit('Capture is incomplete or unsupported; no compatibility guard may be bypassed')
code_hash=sha(cap/'section_00_00001000.bin')
report_hash=sha(cap/'report.json')
if code_hash!=approved[report['ExecutableSha256']]:raise SystemExit('Capture executable/code mismatch')
before_build=source_hashes()
ftcs=ftc_hashes()
baseline_hashes=tree_hashes(base)
legacy_hashes=tree_hashes(legacy) if legacy else {}
baseline31_hashes=tree_hashes(base31)
with (out/'production-build.log').open('w') as log:
    subprocess.run([sys.executable,str(S/'build.py'),str(zig)],stdout=log,stderr=subprocess.STDOUT,check=True)
cmd=[str(zig),'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror','-I'+str(S),
     '-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']
cmd += [str(S/n) for n in ['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c','pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']]
cmd += ['-o',str(out/'retention_fixture.dll'),'-Wl,--dynamicbase','-Wl,--nxcompat']
with (out/'fixture-build.log').open('w') as log:
    subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
before=source_hashes()
if {k:v for k,v in before_build.items() if k!='dll_hash.h'}!={k:v for k,v in before.items() if k!='dll_hash.h'}:
    raise SystemExit('Source changed during compilation; discard mixed build and rerun on frozen input')
env=dict(os.environ,DRI_CAPTURE_DIR=str(cap),DRI_TEST30_BASELINE_DIR=str(base),
         DRI_TEST31_BASE_SOURCE=str(base/'Runtime/Portal/Source'),DRI_TEST31R1_BASELINE_DIR=str(base31),DRI_TEST_SECONDARY_CAPTURE_DIR=str(secondary or cap),DRI_TEST_ZIG=str(zig),DRI_TEST_EXPECTED_HOOK_COUNT=str(a.expected_hook_count),
         DRI_TEST_EXPECTED_SAVED_HOOK_COUNT=str(a.expected_saved_hook_count),
         DRI_TEST_INSTRUCTION_LIMIT='250000000',DRI_TEST_REUSE_NATIVE_HEAP='1')
env.pop('DRI_EXPECT_PREVIOUS_FAULT',None)
env.pop('DRI_TEST32_FUNDING_BASELINE',None)
for key in ['DRI31_SIGN_NEGATIVE','DRI31_HS_SCORE_NEGATIVE']:env.pop(key,None)
env.pop('DRI31_PRE_TRACE_ONLY',None)
env.pop('DRI31_SIGN_NEGATIVE',None)
env.pop('DRI31_HS_SCORE_NEGATIVE',None)
env.pop('DRI_BASELINE_DIR',None)
if legacy:env['DRI_BASELINE_DIR']=str(legacy)
if a.deps:env['PYTHONPATH']=str(checked_directory(a.deps,'Dependencies'))+os.pathsep+env.get('PYTHONPATH','')
if a.log_csv:env['DRI_TEST_LOG_CSV']=str(a.log_csv.resolve())
else:env.pop('DRI_TEST_LOG_CSV',None)
identity={'release':'Test32','source_sha256':before,'ftc_sha256':ftcs,'baseline_sha256':baseline_hashes,
          'legacy_baseline_sha256':legacy_hashes,'test31_baseline_sha256':baseline31_hashes,'fixture_sha256':sha(out/'retention_fixture.dll'),
          'checks':tests,'superseded_checks':SUPERSEDED,'full_suite':not bool(a.tests),'zig':'0.13.0',
          'expected_hook_count':a.expected_hook_count,'expected_saved_hook_count':a.expected_saved_hook_count,
          'native_capture_executable_sha256':report['ExecutableSha256'],'native_capture_code_sha256':code_hash,
          'live_game_verified':False}
if a.log_csv:identity['log_replay_input_sha256']=sha(a.log_csv.resolve())
(out/'frozen-inputs.json').write_text(json.dumps(identity,indent=2)+'\n')
def run(name):
    dest=out/name;dest.mkdir();shutil.copy2(out/'retention_fixture.dll',dest/'retention_fixture.dll')
    start=time.monotonic()
    with (dest/'run.log').open('w') as log:
        try:
            result=subprocess.run([sys.executable,'-u',str(S/'Tests'/f'{name}.py')],
                                  env=dict(env,DRI_TEST_OUTPUT_DIR=str(dest)),stdout=log,stderr=subprocess.STDOUT,timeout=900)
            code=result.returncode
        except subprocess.TimeoutExpired:
            code=124;log.write('\nTIMEOUT 900 seconds\n')
    result={'name':name,'returncode':code,'seconds':round(time.monotonic()-start,2),'log_sha256':sha(dest/'run.log'),'fixture_sha256':sha(dest/'retention_fixture.dll')}
    if result['fixture_sha256']!=identity['fixture_sha256']:result['returncode']=125;result['fixture_mutated']=True
    (dest/'completed.json').write_text(json.dumps(result,indent=2)+'\n')
    print(('PASS ' if code==0 else 'FAIL ')+name,flush=True)
    return result
results=[]
# Start the known long native catalogue replay first. Check definitions and
# results order stay unchanged; this avoids an idle tail after shorter tests.
schedule=sorted(tests,key=lambda n:0 if n=='tu1029r3_open_list_test' else 1)
with ThreadPoolExecutor(max_workers=a.jobs) as pool:
    for task in as_completed([pool.submit(run,n) for n in schedule]):results.append(task.result())
results.sort(key=lambda x:tests.index(x['name']))
secondary_result=None
if secondary:
    secondary_report=json.loads((secondary/'report.json').read_text(encoding='utf-8-sig'))
    secondary_code=sha(secondary/'section_00_00001000.bin')
    secondary_report_hash=sha(secondary/'report.json')
    if not secondary_report['Complete'] or approved.get(secondary_report['ExecutableSha256'])!=secondary_code:
        raise SystemExit('Unsupported secondary capture; original checks retained, release not passed')
    second_tests=a.secondary_tests or (
      'test31_compat_test hook_entry_test test31_native_hs_context_test '
      'test31_native_portal_context_test test32_offer_action_native_test '
      'test31_initial_interest_test test31_weekly_destination_test '
      'test31_signing_test test31_academy_queue_test test31_generator_order_test '
      'test32_native_funding_order_test test32_hs_minimum_share_test '
      'test32_native_funding_spend_test test32_market_hs_test').split()
    for name in second_tests:
        if not (S/'Tests'/f'{name}.py').is_file():raise SystemExit('Missing secondary check: '+name)
    # Reuse exactly the first fixture, never rebuild it between captures.
    primary_env=env.copy();primary_out=out
    primary_fixture=out/'retention_fixture.dll';fixture_hash=sha(primary_fixture)
    secondary_out=out/'secondary';secondary_out.mkdir()
    shutil.copy2(primary_fixture,secondary_out/'retention_fixture.dll')
    assert sha(secondary_out/'retention_fixture.dll')==fixture_hash==identity['fixture_sha256']
    env['DRI_CAPTURE_DIR']=str(secondary);out=secondary_out
    with ThreadPoolExecutor(max_workers=a.jobs) as pool:
        secondary_checks=[task.result() for task in as_completed([pool.submit(run,n) for n in second_tests])]
    secondary_checks.sort(key=lambda x:second_tests.index(x['name']))
    secondary_result={
      'native_capture_executable_sha256':secondary_report['ExecutableSha256'],
      'native_capture_code_sha256':secondary_code,'fixture_sha256':fixture_hash,
      'fixture_rebuilt':False,'checks':secondary_checks,
      'total':len(secondary_checks),'passed_count':sum(r['returncode']==0 for r in secondary_checks),
      'passed':all(r['returncode']==0 for r in secondary_checks) and sha(primary_fixture)==fixture_hash and
        sha(secondary/'section_00_00001000.bin')==secondary_code and sha(secondary/'report.json')==secondary_report_hash,
      'full_suite':False,'live_game_verified':False}
    (secondary_out/'results.json').write_text(json.dumps(secondary_result,indent=2)+'\n')
    out=primary_out;env=primary_env
unchanged=before==source_hashes()
ftcs_unchanged=ftcs==ftc_hashes()
baseline_unchanged=baseline_hashes==tree_hashes(base)
legacy_unchanged=legacy_hashes==(tree_hashes(legacy) if legacy else {})
baseline31_unchanged=baseline31_hashes==tree_hashes(base31)
capture_unchanged=sha(cap/'section_00_00001000.bin')==code_hash and sha(cap/'report.json')==report_hash
result={'release':'Test32','passed':all(r['returncode']==0 for r in results) and unchanged and ftcs_unchanged and baseline_unchanged and legacy_unchanged and baseline31_unchanged and capture_unchanged,
        'full_suite':not bool(a.tests),'checks':results,'passed_count':sum(r['returncode']==0 for r in results),'total':len(results),
        'source_unchanged':unchanged,'ftcs_unchanged':ftcs_unchanged,'baseline_unchanged':baseline_unchanged,'legacy_baseline_unchanged':legacy_unchanged,'test31_baseline_unchanged':baseline31_unchanged,
        'fixture_sha256':identity['fixture_sha256'],'production_dll_sha256':sha(S.parent/'DRIChallengerRetention.dll'),
        'launcher_sha256':sha(S.parent/'DRIRetentionTest.exe'),'native_capture_executable_sha256':report['ExecutableSha256'],
        'capture_unchanged':capture_unchanged,'toolchain':'Zig 0.13.0','live_game_verified':False,'secondary_capture':secondary_result}
if secondary_result is not None:result['passed']=result['passed'] and secondary_result['passed']
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='checks'},indent=2))
sys.exit(not result['passed'])
