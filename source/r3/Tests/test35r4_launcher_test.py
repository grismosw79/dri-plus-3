"""Regression for R5 producer/monitor naming and real monitor control flow.
Win32 I/O is modeled; no claim of live Windows GUI or game execution.
"""
from pathlib import Path
import subprocess,json,os,re,hashlib,resource
ROOT=Path(__file__).resolve().parents[1]
OUT=Path(os.environ['DRI_TEST_OUTPUT_DIR']);OUT.mkdir(parents=True,exist_ok=True)
producer=re.search(r'L"(DRI_PortalTest[^_]+)_%',(ROOT/'retention.c').read_text()).group(1)
consumer=re.search(r'L"(DRI_PortalTest[^_]+)_\*_',(ROOT/'launcher_monitor.inc').read_text()).group(1)
assert producer==consumer=='DRI_PortalTest35R5'
checks=[{'name':'Actual DLL source and launcher include agree on R5 CSV prefix','passed':True}]
fixture=(ROOT/'Tests/test15r1_launcher_monitor_test.c').read_text().replace('Test28R12','Test35R5').replace('Test28R7_previous_version','Test35R1_previous_version')
source=OUT/'launcher.c';source.write_text(fixture)
def compile_run(source,include,exe):
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(include),str(source),'-o',str(exe)],check=True)
 return subprocess.run([str(exe)],capture_output=True,text=True,preexec_fn=lambda:resource.setrlimit(resource.RLIMIT_CORE,(0,0)))
bad=OUT/'original-r1-monitor';bad.mkdir(exist_ok=True)
(bad/'launcher_monitor.inc').write_text((ROOT/'launcher_monitor.inc').read_text().replace('Test35R5','Test35R1'))
(bad/'runtime_log_monitor.h').write_bytes((ROOT/'runtime_log_monitor.h').read_bytes())
(bad/'car35_generation_monitor.h').write_bytes((ROOT/'car35_generation_monitor.h').read_bytes())
negative_source=OUT/'negative.c'
negative_source.write_text('\n'.join(line for line in fixture.splitlines() if 'Test35R1_previous_version' not in line)+'\n')
r=compile_run(negative_source,bad,OUT/'negative-control')
assert r.returncode!=0 and 'find_runtime_log' in r.stderr
checks.append({'name':'Original R1 monitor reproduces failure when the current session writes R5 CSV','passed':True})
r=compile_run(source,ROOT,OUT/'launcher');assert r.returncode==0,r.stderr
checks.append({'name':'Corrected actual monitor selects fresh R5 for correct PID; ignores newer R1, old timestamps, directories and other PIDs; checks shared opening, split faults, intentional mode changes, read/process failures, handle cleanup and final flush','passed':True})
source=OUT/'log.c';source.write_bytes((ROOT/'Tests/test15r1_log_monitor_test.c').read_bytes())
r=compile_run(source,ROOT,OUT/'log');assert r.returncode==0,r.stderr
checks.append({'name':'Streaming log parser regression passes','passed':True})
exe=(ROOT.parent/'DRIRetentionTest.exe').read_bytes();dll=(ROOT.parent/'DRIChallengerRetention.dll').read_bytes();digest=hashlib.sha256(dll).hexdigest()
assert digest==re.search(r'DRI_DLL_HASH "([a-f0-9]{64})"',(ROOT/'dll_hash.h').read_text()).group(1)
assert digest.encode()in exe
assert 'DRI_PortalTest35R5_*_%lu.csv'.encode('utf-16le')in exe
assert 'DRI_PortalTest35R1_*_%lu.csv'.encode('utf-16le')not in exe
assert b'launcher_revision=R5_Quality_Repair'in exe
checks.append({'name':'Shipped Windows launcher embeds correct R5 pattern and original gameplay DLL hash; stale R1 pattern absent','passed':True})
source=OUT/'generation-monitor.c';source.write_bytes((ROOT/'Tests/test35r4_generation_monitor_test.c').read_bytes())
r=compile_run(source,ROOT,OUT/'generation-monitor');assert r.returncode==0,r.stderr
checks.append({'name':'Generation notice recognizes complete boundary-end rows across split reads and truncates unrelated long rows safely','passed':True})
result={'passed':True,'checks':checks,'DLL_sha256':digest,'launcher_sha256':hashlib.sha256(exe).hexdigest(),'gameplay_changes':True,'Win32_services_modeled':True,'live_Windows_acceptance':False}
(OUT/'test35r5_launcher_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
