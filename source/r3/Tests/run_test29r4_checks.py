"""Test29R4 release runner. --only runs scoped checks; release validation requires
a single coordinated frozen-source attempt. No live season result is claimed.

Requires the supported native capture, Zig 0.13.0, GCC, Unicorn, pefile and
Capstone. Run after integration is frozen. A fixture compiled from the exact
current source is shared read-only across bounded parallel test processes.
Legacy policy expectations deliberately replaced by Test29 are reported as
superseded, never counted as old tests passing.

python run_test29r2_checks.py --zig /path/to/zig --capture /path/to/capture \
  --deps /path/to/python/deps --baseline /path/to/authoritative/Test29R1
Use --list without compiling, or --only NAME for a clearly marked partial run.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
import argparse, hashlib, json, os, re, shutil, subprocess, sys, time

LEGACY = '''test28r15_recovery_retention_test test28r14_offseason_test
test28r14_native_admit_test test28r14_nsd_cleanup_test test28r14_saved_rosters_test
test28r13r1_continuity_test test28r12_eps_cadence_test test28r9_native_pipeline_test
test28r8_hs_rival_cleanup_test test28r8_funding_published_test
test28r8_native_budget_test test28r8_postfunding_hook_test test28r6_nsd_pairs_test
hs_ownership_test hard_commit_ownership_test test28r13_entry_abi_test
hook_entry_test'''.split()
SUPERSEDED = [
    {'name':'test28r14_crowding_test', 'reason':'R14 raw first-wave forecast and 15-entry expectations intentionally replaced by Test29 reconciled HS12 / portal6–8 market.', 'successor_pattern':'test29_market*_test.py'},
    {'name':'test28r14_policy_test', 'reason':'R14 6/10/15 tiers intentionally replaced by separate Test29 HS and portal tiers.', 'successor_pattern':'test29_market*_test.py'},
    {'name':'test29_portal_policy_test', 'reason':'The old hs_reservation assertion rejected every material portal upgrade at a reserved last HS vacancy. R2 permits only an independently qualifying upgrade using an existing unused shared allowance while keeping the HS reservation. The successor preserves the old material/capacity/selection regressions and replaces the reservation expectation.', 'successor_pattern':'test29r2_acceptance_policy_test.py'},
]
SUPERSEDED += [
 {'name':'test29_portal_native_test','reason':'The old marginal-starter veto is intentionally replaced by R4 prestige-qualified useful depth. The successor retains all 15 identity/native admission cases and adds a below-useful-depth rejection.','successor_pattern':'test29r4_quality_depth_native_test.py'},
 {'name':'test29r2_acceptance_policy_test','reason':'R2 required all non-QB quality arrivals to use the normal position/shared allowance. R4 replaces that restriction for prestige-qualified useful depth; successor covers full/overfull rooms, HS reservations, shared accounting/reload, equal-depth stopping, QB/specialists and native-score selection. Historical R2 assertions retained as historical source.','successor_pattern':'test29r4_quality_depth_policy_test.py'},
 {'name':'test29r2_acceptance_test','reason':'Its same-position and cross-position conditional capacity assertions assume the scarce shared allowance is required by quality-qualified arrivals. R4 intentionally allows coexistence. The successor retains read-only acceptance/witness/execution/native-eligibility checks, secured counting/reload and an explicit redundant-depth rejection.','successor_pattern':'test29r4_quality_depth_runtime_test.py'},
 {'name':'test29r3_launcher_test','reason':'Same monitor regressions compiled with R4 current-release and R3 previous-release labels.','successor_pattern':'test29r4_launcher_test.py'},
]
POLICY_REVISIONS = [
    {'tests':['test29_market_core_test','test29_market_hs_test','test29_market_portal_test'],
     'previous_policy':'Test29/R1 counted primary and backup demand together toward HS12 / portal8.',
     'current_policy':'R2 separates HS8 / portal5 committable-primary targets from secondary HS12 / portal8 total-attention guidance.',
     'replacement_coverage':['test29r2_market_roles_test','test29r2_market_execution_test'],
     'claim':'These components run with deliberately updated policy expectations; they are not claimed as unchanged R1-test passes.'},
    {'tests':['test29_market_hs_test'],
     'previous_policy':'The old admission callback scenario expected [False, True] for the crowded recruit and its close alternative, dropping the sole remaining conditional backup.',
     'current_policy':'When the close alternative becomes primary, its sole credible conditional backup may remain under the secondary soft guide; the scenario changes from [False, True] to [True, True]. This does not authorize unfunded or unplanned admission.',
     'replacement_coverage':['test29r2_market_roles_test','test29r2_market_execution_test'],
     'claim':'The old sole-backup rejection expectation is deliberately superseded within a retained test; native admission and funding constraints remain tested.'},
]
POLICY_REVISIONS += [{'tests':['test29r1_capacity_test','test29r2_class_continuity_test'],
 'previous_policy':'Own-academy portal pursuits were allowed; paid-HS continuity fixture used a four-star Army target.',
 'current_policy':'All academy portal pursuits are blocked. Continuity uses a permitted three-star Army target; new R3 tests separately reject four/five-star HS and exercise end-only return.',
 'replacement_coverage':['test29r3_academy_test','test29r3_return_native_test'],
 'claim':'Intentional user-authorized academy policy changes; other capacity/ownership assertions retained.'}]
POLICY_REVISIONS += [{'tests':['portal_runtime_test','signing_ownership_test','hard_commit_ownership_test','hs_ownership_test','test29r1_capacity_test'],
 'previous_policy':'Marginal/full-room WR/HB and other position arrivals could reject despite being prestige-qualified useful contributors.',
 'current_policy':'R4 permits quality-qualified non-QB useful depth. The 78 archived ownership rooms retain original inputs/count checks, with static case-specific rank/floor permission expectations in Fixtures/test29r4_archive_quality_permissions.json. All other ownership and QB rejections remain. R1 paid-HB priority now explicitly permits the same quality coexistence proven by R4 runtime coverage; academy ownership assertions remain.',
 'replacement_coverage':['test29r4_quality_depth_policy_test','test29r4_quality_depth_runtime_test','test29r4_quality_depth_native_test','hard_commit_ownership_test'],
 'claim':'Intentional policy expectation changes, not unchanged historical passes or predicted live signings.'}]
HARNESS_REVISIONS = [
    {'tests':['test28r13r1_continuity_test'],
     'files':['Tests/runtime_fixture_test.py','Tests/runtime_heap_fixture.py','Tests/test28r13r1_continuity_test.py'],
     'previous_harness':'The original fixed 64 MiB bump allocator never reclaimed HeapFree calls. Coordinated attempt 1 exhausted that synthetic heap during repeated continuity cases; this was not a timeout or failed recruiting-policy assertion.',
     'current_harness':'Continuity alone opts into bounded reusable HeapAlloc/HeapFree modeling. The same total 64 MiB mapping is partitioned into a 48 MiB SDF record arena and a separate 16 MiB native allocation arena; freed blocks are poisoned, valid blocks are tracked, reuse honors HEAP_ZERO_MEMORY, and invalid or duplicate frees fail.',
     'claim':'Native allocation/free tracing established balanced production lifetimes before this fixture correction. All continuity policy assertions remain; production DLL/source and fixture bytes are unchanged. A complete new coordinated attempt reruns every check, preserving the first failure.'},
    {'tests':['test29_portal_execution_test'],
     'previous_harness':'Synthetic caller RBP was placed approximately 96 KiB below live RSP, allowing a deeper valid callback stack to overlap the synthetic frame.',
     'current_harness':'Caller RBP uses the independently confirmed captured native prologue deltas: RSP + 0x91 for generic execution and RSP + 0x100 for pitch execution. The test asserts the prologue bytes.',
     'claim':'All register, stack, branch and record assertions are retained. This corrects fixture placement and does not change production behavior; the historical failed run remains development evidence.'},
    {'tests':['test29_portal_witness_test'],
     'previous_harness':'The ctypes PortalChoice mirror matches the R1 internal structure.',
     'current_harness':'The mirror includes the five R2 internal market-role integers.',
     'claim':'The native candidate ABI and behavioral witness assertions remain unchanged.'},
]
HISTORICAL_NOT_RUN = [
    {'tests':['test25_commitment_test','market_race_runtime_test','test28r8_palko_test'],
     'reason':'Historical ctypes mirrors already omit internal structure fields present in the preserved R1 baseline. These files were outside the R1 coordinated suite and remain outside R2; they are not valid against the current fixture and are not claimed as passes.',
     'current_coverage':['test29_portal_witness_test','test29r2_acceptance_test','test29r2_market_execution_test','test29r2_class_continuity_test']},
]
REQUIRED_COMPONENTS = {
    'r4_publication':('test29r4_academy_publication_test',),
    'r4_launcher':('test29r4_launcher_test',),
    'r2_market':('test29r2_market_execution_test','test29r2_market_roles_test'),
    'r4_acceptance':('test29r4_quality_depth_policy_test','test29r4_quality_depth_runtime_test'),
    'r2_classes':('test29r2_class_recovery_test','test29r2_class_continuity_test'),
    'r2_native_retry':('test29r2_portal_retry_native_test',),
    'market':('test29_market_core_test','test29_market_hs_test',
              'test29_market_portal_test','test29_market_portal_score_test'),
    'classes':('test29_class_policy_test','test29_class_r14_test',
               'test29_class_rosters_test','test29_class_source_test'),
    'portal':('test29r4_quality_depth_native_test',
              'test29_portal_witness_test','test29_portal_execution_test',
              'test29_portal_continuity_test','test29_portal_class_boundary_test'),
    'visits':('test29_visit_native_test','test29_visit_completion_test'),
    'supply':('test29_supply_native_test','test29_supply_model_test'),
}
CAPTURE_SHA='7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e'
MODULES=['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c',
         'pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def discover_new_checks(tests):
    excluded={x['name']for x in SUPERSEDED}
    return sorted(x.stem for x in tests.glob('test29*_test.py')if x.stem not in excluded)
def discover_checks(tests):
    new=discover_new_checks(tests)
    return [n for n in new if n.startswith('test29r4_')]+LEGACY+[n for n in new if not n.startswith('test29r4_')]
def source_fingerprints(source):
    return {str(p.relative_to(source)):sha(p) for p in sorted(source.rglob('*'))
            if p.is_file() and not {'build','__pycache__','.zig-cache'} & set(p.relative_to(source).parts)
            and p.suffix in {'.c','.h','.inc','.S','.py','.json','.cjs','.gz'}}
def release_input_fingerprints(source):
    package=source.parent.parent.parent
    paths=list((package/'frantkdata').glob('*/*.FTC'))
    audit=source.parent/'Audit/Test29'
    paths += list(audit.parent.glob('*.json'))  # Restored historical ownership inputs.
    paths += [audit/n for n in ['ftc_changes.json','generator_changes.json','production_build.json',
                              'Inputs/punter-cohort-evidence.json']]
    paths += list((audit/'Inputs/GeneratorBaseline').rglob('*'))
    paths += list((audit.parent/'Test29R4/Inputs').rglob('*'))
    paths += [audit.parent/'TU1029/ftc_identity.json']
    paths += [audit.parent/'Test29R4/production_build.json']
    return {str(p.relative_to(package)):sha(p) for p in sorted(paths) if p.is_file()}
def baseline_fingerprints(package):
    source=package/'Runtime/Portal/Source'
    out={'Runtime/Portal/Source/'+n:h for n,h in source_fingerprints(source).items()}
    out.update({str(p.relative_to(package)):sha(p) for p in sorted((package/'frantkdata').glob('*/*.FTC'))})
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--zig');p.add_argument('--capture',type=Path)
    p.add_argument('--deps',type=Path);p.add_argument('--baseline',type=Path)
    p.add_argument('--jobs',type=int,default=3);p.add_argument('--timeout',type=int,default=900)
    p.add_argument('--continuity-timeout',type=int,default=1800,
                   help='Known long compiled continuity check; default retains its validated R1 1800s allowance')
    p.add_argument('--only',action='append',default=[]);p.add_argument('--list',action='store_true')
    p.add_argument('--fresh-fixture',action='store_true')
    a=p.parse_args();assert 1<=a.jobs<=4 and 1<=a.timeout<=1800 and 1<=a.continuity_timeout<=1800
    s=Path(__file__).resolve().parents[1];tests=s/'Tests';out=s/'build/tu1029-checks'
    new=discover_new_checks(tests)
    names=discover_checks(tests)
    assert len(names)==len(set(names))
    missing={k:sorted(set(required)-set(new)) for k,required in REQUIRED_COMPONENTS.items()
             if set(required)-set(new)}
    if a.list:
        print(json.dumps({'checks':names,'superseded':SUPERSEDED,'policy_expectation_revisions':POLICY_REVISIONS,'harness_revisions':HARNESS_REVISIONS,'historical_fixtures_not_run':HISTORICAL_NOT_RUN,'missing_components':missing},indent=2));return
    if not all((a.zig,a.capture,a.baseline)):
        p.error('--zig, --capture and --baseline are required unless --list is used')
    if a.only:
        assert set(a.only)<=set(names),('unknown requested test',set(a.only)-set(names))
        names=list(dict.fromkeys(a.only))
    else:
        assert not missing,('coordinated release missing component checks',missing)
        for old in SUPERSEDED:assert list(tests.glob(old['successor_pattern'])),old
    for name in names:assert (tests/f'{name}.py').is_file(),name
    capture=a.capture.resolve();baseline=a.baseline.resolve()
    assert (baseline/'Runtime/Portal/Source/retention.c').is_file()
    assert sha(capture/'section_00_00001000.bin')==CAPTURE_SHA,'Unsupported native capture'
    zig=Path(a.zig).resolve()
    assert subprocess.check_output([str(zig),'version'],text=True).strip()=='0.13.0','Zig 0.13.0 required'
    before=source_fingerprints(s);input_before=release_input_fingerprints(s);baseline_before=baseline_fingerprints(baseline);out.mkdir(parents=True,exist_ok=True)
    # A later run never destroys the earlier failures, timeout logs, or fixtures.
    attempt=1
    while (out/f'attempt-{attempt:04d}').exists():attempt+=1
    attempt_out=out/f'attempt-{attempt:04d}';attempt_out.mkdir()
    (out/'latest-run.json').write_text(json.dumps({'attempt':attempt,'directory':attempt_out.name,'status':'RUNNING'},indent=2)+'\n')
    fixture=out/'retention_fixture.dll';fixture_meta=out/'fixture_build.json'
    command=[str(zig),'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror',
             '-I'+str(s),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']
    command += [str(s/n) for n in MODULES]+['-o',str(fixture),'-Wl,--dynamicbase','-Wl,--nxcompat']
    production={n:h for n,h in before.items() if not n.startswith('Tests/') and n!='dll_hash.h'}
    identity={'production_source':production,'zig':'0.13.0','compile_flags':command[2:command.index(str(s/'retention.c'))]}
    cached=json.loads(fixture_meta.read_text()) if fixture_meta.exists() else {}
    reuse=(not a.fresh_fixture and fixture.exists() and cached.get('identity')==identity
           and cached.get('fixture_sha256')==sha(fixture)
           and (out/'fixture_build.log').is_file()
           and cached.get('fixture_build_log_sha256')==sha(out/'fixture_build.log'))
    if not reuse:
        r=subprocess.run(command,text=True,capture_output=True)
        (out/'fixture_build.log').write_text(r.stdout+r.stderr)
        shutil.copy2(out/'fixture_build.log',attempt_out/'fixture_build.log')
        if r.returncode!=0:
            failure={'release':'Test29R4-TU1029','status':'FAIL_FIXTURE_COMPILE','scope':'partial'if a.only else'coordinated',
                     'attempt':attempt,'returncode':r.returncode,'source_fingerprints':before,
                     'release_input_fingerprints':input_before,'baseline_source_and_ftc_fingerprints':baseline_before,
                     'zig':'0.13.0','command':command,'live_game_verified':False}
            (attempt_out/'compile_failure.json').write_text(json.dumps(failure,indent=2)+'\n')
            archive=s.parent/'Audit/TU1029/ValidationAttempts'/attempt_out.name;archive.mkdir(parents=True,exist_ok=False)
            for name in ['compile_failure.json','fixture_build.log']:shutil.copy2(attempt_out/name,archive/name)
            (out/'latest-run.json').write_text(json.dumps({'attempt':attempt,'directory':attempt_out.name,'report':'compile_failure.json','status':failure['status'],'scope':failure['scope']},indent=2)+'\n')
            raise SystemExit('Fixture compile failed; preserved log: '+str(archive/'fixture_build.log'))
        fixture_meta.write_text(json.dumps({'identity':identity,'fixture_sha256':sha(fixture),
            'fixture_build_log_sha256':sha(out/'fixture_build.log')},indent=2)+'\n')
    fixture_hash=sha(fixture)
    for name in ['retention_fixture.dll','fixture_build.json','fixture_build.log']:
        if (out/name).exists():shutil.copy2(out/name,attempt_out/name)
    hook_count=len(re.findall(r'\{0x[0-9a-f]+,\d+,\{[^}]+\},Hook\w+\}',(s/'retention.c').read_text()))
    saved_hook_count=len(re.findall(r'^\s*SAVE_STATE\s*$',(s/'hooks.S').read_text(),re.M))
    env=dict(os.environ,DRI_CAPTURE_DIR=str(capture),DRI_TEST_INSTRUCTION_LIMIT='250000000',
             DRI_TEST_ZIG=str(zig),DRI_BASELINE_DIR=str(baseline),DRI_PACKAGE_DIR=str(s.parent),
             DRI_TEST_EXPECTED_HOOK_COUNT=str(hook_count),DRI_TEST_EXPECTED_SAVED_HOOK_COUNT=str(saved_hook_count))
    if a.deps:env['PYTHONPATH']=str(a.deps.resolve())+os.pathsep+env.get('PYTHONPATH','')
    def run(name):
        target=attempt_out/name
        target.mkdir();shutil.copy2(fixture,target/'retention_fixture.dll')
        start=time.monotonic();timed_out=False
        timeout=a.continuity_timeout if name=='test28r13r1_continuity_test' else a.timeout
        with (target/'test.log').open('w') as log:
            try:
                r=subprocess.run([sys.executable,'-u',str(tests/f'{name}.py')],
                    env=dict(env,DRI_TEST_OUTPUT_DIR=str(target)),stdout=log,stderr=subprocess.STDOUT,
                    text=True,timeout=timeout)
                code=r.returncode
            except subprocess.TimeoutExpired:
                code=124;timed_out=True
                log.write(f'\nTIMEOUT after {timeout}s\n')
        print(('PASS ' if code==0 else 'FAIL ')+name,flush=True)
        return {'name':name,'returncode':code,'timeout':timed_out,'timeout_seconds':timeout,'seconds':round(time.monotonic()-start,3),
                'test_sha256':before[f'Tests/{name}.py'],'fixture_sha256':fixture_hash,'log_sha256':sha(target/'test.log')}
    started=time.time()
    # Preserve provenance before starting children and each completed exit
    # receipt immediately. An interrupted monitor must not erase evidence.
    (attempt_out/'attempt_snapshot.json').write_text(json.dumps({
        'release':'Test29R4-TU1029','attempt':attempt,'status':'RUNNING',
        'started_unix':started,'checks':names,'fixture_sha256':fixture_hash,
        'source_fingerprints':before,'release_input_fingerprints':input_before,
        'baseline_source_and_ftc_fingerprints':baseline_before},indent=2)+'\n')
    with ThreadPoolExecutor(max_workers=a.jobs) as ex:
        results=[]
        for job in as_completed([ex.submit(run,n) for n in names]):
            result=job.result();results.append(result)
            (attempt_out/result['name']/'completed.json').write_text(json.dumps(result,indent=2)+'\n')
    results.sort(key=lambda r:names.index(r['name']))
    after=source_fingerprints(s);changed=sorted(set(before)^set(after)|{n for n in before.keys()&after.keys() if before[n]!=after[n]})
    input_after=release_input_fingerprints(s)
    input_changed=sorted(set(input_before)^set(input_after)|{n for n in input_before.keys()&input_after.keys() if input_before[n]!=input_after[n]})
    baseline_after=baseline_fingerprints(baseline)
    baseline_changed=sorted(set(baseline_before)^set(baseline_after)|{n for n in baseline_before.keys()&baseline_after.keys() if baseline_before[n]!=baseline_after[n]})
    report={'release':'Test29R4-TU1029','status':'PASS_OFFLINE_LIVE_PENDING' if not changed and not input_changed and not baseline_changed and all(r['returncode']==0 for r in results) else 'FAIL',
            'scope':'partial' if a.only else 'coordinated','live_game_verified':False,'started_unix':started,
            'attempt':attempt,'execution_mode':'single_frozen_source_attempt','jobs':a.jobs,
            'checks_passed':sum(r['returncode']==0 for r in results),'checks':results,'superseded_legacy_checks':SUPERSEDED,
            'policy_expectation_revisions':POLICY_REVISIONS,
            'harness_revisions':HARNESS_REVISIONS,
            'historical_fixtures_not_run':HISTORICAL_NOT_RUN,
            'source_fingerprints':before,'source_changed_during_run':changed,'fixture_sha256':fixture_hash,
            'release_input_fingerprints':input_before,'release_inputs_changed_during_run':input_changed,
            'baseline_source_and_ftc_fingerprints':baseline_before,'baseline_changed_during_run':baseline_changed,
            'fixture_reused':reuse,'captured_code_sha256':CAPTURE_SHA,'hook_count':hook_count,'saved_register_hook_count':saved_hook_count,
            'toolchain':{'zig':'0.13.0','target':'x86_64-windows-gnu','python':sys.version.split()[0],
                         'gcc':subprocess.check_output(['gcc','--version'],text=True).splitlines()[0]},
            'limits':['Compiled callbacks use controlled Windows/SDF services. Each component identifies captured-native paths separately.',
                      'A saved roster replay or modeled candidate case is not a native full-season simulation.',
                      'No live Test29R4 signings, visit UI behavior, Playing Style persistence or advance performance has been observed.']}
    report_path=attempt_out/('partial-results.json' if a.only else 'results.json')
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    (out/'latest-run.json').write_text(json.dumps({'attempt':attempt,'directory':attempt_out.name,'report':report_path.name,'status':report['status'],'scope':report['scope']},indent=2)+'\n')
    # Preserve portable evidence for every completed attempt, including failures.
    # Exact fixture hashes are recorded; each final release also includes its fixture.
    archive=s.parent/'Audit/TU1029/ValidationAttempts'/attempt_out.name
    archive.mkdir(parents=True,exist_ok=False)
    for file in sorted(attempt_out.rglob('*')):
        if file.is_file() and file.suffix in {'.json','.log'}:
            dest=archive/file.relative_to(attempt_out);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(file,dest)
    print(json.dumps({'result':str(report_path),'status':report['status'],'scope':report['scope'],
                     'passed':report['checks_passed'],'total':len(results),'source_changed':changed,'release_inputs_changed':input_changed,'baseline_changed':baseline_changed},indent=2))
    raise SystemExit(report['status']=='FAIL')

if __name__=='__main__':main()
