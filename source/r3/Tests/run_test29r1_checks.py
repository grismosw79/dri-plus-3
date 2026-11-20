"""Integrated Test29 offline runner. No live season result is claimed.

Requires the supported native capture, Zig 0.13.0, GCC, Unicorn, pefile and
Capstone. Run after integration is frozen. A fixture compiled from the exact
current source is shared read-only across bounded parallel test processes.
Legacy policy expectations deliberately replaced by Test29 are reported as
superseded, never counted as old tests passing.

python run_test29_checks.py --zig /path/to/zig --capture /path/to/capture \
  --deps /path/to/python/deps --baseline /path/to/authoritative/R14
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
]
REQUIRED_COMPONENTS = {
    'market':('test29_market_core_test','test29_market_hs_test',
              'test29_market_portal_test','test29_market_portal_score_test'),
    'classes':('test29_class_policy_test','test29_class_r14_test',
               'test29_class_rosters_test','test29_class_source_test'),
    'portal':('test29_portal_policy_test','test29_portal_native_test',
              'test29_portal_witness_test','test29_portal_execution_test',
              'test29_portal_continuity_test','test29_portal_class_boundary_test'),
    'visits':('test29_visit_native_test','test29_visit_completion_test'),
    'supply':('test29_supply_native_test','test29_supply_model_test'),
}
CAPTURE_SHA='7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e'
MODULES=['retention.c','eps_core.c','eps_collect.c','eps_snapshot.c',
         'pt28_core.c','pp28_core.c','staff28_core.c','hooks.S']

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
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
    return {str(p.relative_to(package)):sha(p) for p in sorted(paths) if p.is_file()}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--zig',required=True);p.add_argument('--capture',required=True,type=Path)
    p.add_argument('--deps',type=Path);p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--jobs',type=int,default=3);p.add_argument('--timeout',type=int,default=900)
    p.add_argument('--only',action='append',default=[]);p.add_argument('--list',action='store_true')
    p.add_argument('--fresh-fixture',action='store_true')
    a=p.parse_args();assert 1<=a.jobs<=4 and 1<=a.timeout<=1800
    s=Path(__file__).resolve().parents[1];tests=s/'Tests';out=s/'build/test29r1-checks'
    new=sorted(x.stem for x in tests.glob('test29*_test.py'))
    names=LEGACY+new
    assert len(names)==len(set(names))
    missing={k:sorted(set(required)-set(new)) for k,required in REQUIRED_COMPONENTS.items()
             if set(required)-set(new)}
    if a.list:
        print(json.dumps({'checks':names,'superseded':SUPERSEDED,'missing_components':missing},indent=2));return
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
    before=source_fingerprints(s);input_before=release_input_fingerprints(s);out.mkdir(parents=True,exist_ok=True)
    fixture=out/'retention_fixture.dll';fixture_meta=out/'fixture_build.json'
    command=[str(zig),'cc','-target','x86_64-windows-gnu','-O2','-Wall','-Wextra','-Werror',
             '-I'+str(s),'-shared','-fms-extensions','-mno-avx','-DDRI_EMULATION_TEST']
    command += [str(s/n) for n in MODULES]+['-o',str(fixture),'-Wl,--dynamicbase','-Wl,--nxcompat']
    production={n:h for n,h in before.items() if not n.startswith('Tests/') and n!='dll_hash.h'}
    identity={'production_source':production,'zig':'0.13.0','compile_flags':command[2:command.index(str(s/'retention.c'))]}
    cached=json.loads(fixture_meta.read_text()) if fixture_meta.exists() else {}
    reuse=(not a.fresh_fixture and fixture.exists() and cached.get('identity')==identity
           and cached.get('fixture_sha256')==sha(fixture))
    if not reuse:
        r=subprocess.run(command,text=True,capture_output=True)
        (out/'fixture_build.log').write_text(r.stdout+r.stderr)
        assert r.returncode==0,('Fixture compile failed; see',out/'fixture_build.log')
        fixture_meta.write_text(json.dumps({'identity':identity,'fixture_sha256':sha(fixture)},indent=2)+'\n')
    fixture_hash=sha(fixture)
    hook_count=len(re.findall(r'\{0x[0-9a-f]+,\d+,\{[^}]+\},Hook\w+\}',(s/'retention.c').read_text()))
    saved_hook_count=len(re.findall(r'^\s*SAVE_STATE\s*$',(s/'hooks.S').read_text(),re.M))
    env=dict(os.environ,DRI_CAPTURE_DIR=str(capture),DRI_TEST_INSTRUCTION_LIMIT='250000000',
             DRI_TEST_ZIG=str(zig),DRI_BASELINE_DIR=str(baseline),DRI_PACKAGE_DIR=str(s.parent),
             DRI_TEST_EXPECTED_HOOK_COUNT=str(hook_count),DRI_TEST_EXPECTED_SAVED_HOOK_COUNT=str(saved_hook_count))
    if a.deps:env['PYTHONPATH']=str(a.deps.resolve())+os.pathsep+env.get('PYTHONPATH','')
    def run(name):
        target=out/name
        if target.exists():shutil.rmtree(target)
        target.mkdir();shutil.copy2(fixture,target/'retention_fixture.dll')
        start=time.monotonic();timed_out=False
        with (target/'test.log').open('w') as log:
            try:
                r=subprocess.run([sys.executable,'-u',str(tests/f'{name}.py')],
                    env=dict(env,DRI_TEST_OUTPUT_DIR=str(target)),stdout=log,stderr=subprocess.STDOUT,
                    text=True,timeout=a.timeout)
                code=r.returncode
            except subprocess.TimeoutExpired:
                code=124;timed_out=True
                log.write(f'\nTIMEOUT after {a.timeout}s\n')
        print(('PASS ' if code==0 else 'FAIL ')+name,flush=True)
        return {'name':name,'returncode':code,'timeout':timed_out,'seconds':round(time.monotonic()-start,3),
                'test_sha256':before[f'Tests/{name}.py'],'fixture_sha256':fixture_hash,'log_sha256':sha(target/'test.log')}
    started=time.time()
    with ThreadPoolExecutor(max_workers=a.jobs) as ex:
        results=[job.result() for job in as_completed([ex.submit(run,n) for n in names])]
    results.sort(key=lambda r:names.index(r['name']))
    after=source_fingerprints(s);changed=sorted(set(before)^set(after)|{n for n in before.keys()&after.keys() if before[n]!=after[n]})
    input_after=release_input_fingerprints(s)
    input_changed=sorted(set(input_before)^set(input_after)|{n for n in input_before.keys()&input_after.keys() if input_before[n]!=input_after[n]})
    report={'release':'Test29R1','status':'PASS_OFFLINE_LIVE_PENDING' if not changed and not input_changed and all(r['returncode']==0 for r in results) else 'FAIL',
            'scope':'partial' if a.only else 'coordinated','live_game_verified':False,'started_unix':started,
            'checks_passed':sum(r['returncode']==0 for r in results),'checks':results,'superseded_legacy_checks':SUPERSEDED,
            'source_fingerprints':before,'source_changed_during_run':changed,'fixture_sha256':fixture_hash,
            'release_input_fingerprints':input_before,'release_inputs_changed_during_run':input_changed,
            'fixture_reused':reuse,'captured_code_sha256':CAPTURE_SHA,'hook_count':hook_count,'saved_register_hook_count':saved_hook_count,
            'toolchain':{'zig':'0.13.0','target':'x86_64-windows-gnu','python':sys.version.split()[0],
                         'gcc':subprocess.check_output(['gcc','--version'],text=True).splitlines()[0]},
            'limits':['Compiled callbacks use controlled Windows/SDF services. Each component identifies captured-native paths separately.',
                      'A saved roster replay or modeled candidate case is not a native full-season simulation.',
                      'No live Test29R1 signings, visit UI behavior, Playing Style persistence or advance performance has been observed.']}
    report_path=out/('partial-results.json' if a.only else 'results.json')
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':str(report_path),'status':report['status'],'scope':report['scope'],
                     'passed':report['checks_passed'],'total':len(results),'source_changed':changed,'release_inputs_changed':input_changed},indent=2))
    raise SystemExit(report['status']=='FAIL')

if __name__=='__main__':main()
