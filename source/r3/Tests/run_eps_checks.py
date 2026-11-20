"""Compile and replay EPS against the independent, extracted season fixtures."""
from pathlib import Path
import subprocess,os,sys,json,hashlib
S=Path(__file__).resolve().parents[1];A=S.parent/'Audit/EPS1';R=A/'Replay/test27-build'
env=dict(os.environ,DRI_EPS_SOURCE=str(S))
def run(cmd):subprocess.run(cmd,env=env,check=True)
run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',str(S/'eps_core.c'),str(S/'eps_collect.c'),'-o',str(R/'eps_core.so'),'-lm'])
run([sys.executable,str(R/'test_eps_collect.py')])
run([sys.executable,str(R/'test_eps_core.py')])
run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(S/'eps_snapshot.c'),str(S/'Tests/eps_snapshot_test.c'),'-o',str(R/'eps_snapshot_test')])
run([str(R/'eps_snapshot_test'),str(R/'eps-output.bin')])
result={'status':'PASS_OFFLINE_LIVE_PENDING','core':json.loads((R/'eps-core-validation.json').read_text()),'collector':json.loads((R/'eps-collector-validation.json').read_text()),'snapshotIdentityAndCorruptionChecks':True,'live_game_verified':False}
assert result['core']['nativeReaderPipelineGrades']==3174
(A/'portable-validation.json').write_text(json.dumps(result,indent=2));print('PASS: portable numerical core, record reader, full reader/model pipeline, and snapshot validation')
