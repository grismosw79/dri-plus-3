"""The inherited-surplus regression must fail against the unchanged R4 DLL."""
from pathlib import Path
import os, subprocess, sys, shutil, json
output=Path(os.environ['DRI_TEST_OUTPUT_DIR'])
control=output/'control';control.mkdir(exist_ok=True)
source=Path(os.environ['DRI_TEST28R4_CONTROL_DLL'])
shutil.copy2(source,control/'retention_fixture.dll')
env=dict(os.environ,DRI_TEST_OUTPUT_DIR=str(control))
test=Path(__file__).with_name('test28r5_hs_budget_test.py')
r=subprocess.run([sys.executable,str(test)],env=env,capture_output=True,text=True)
(output/'r4-budget-negative.log').write_text(r.stdout+r.stderr)
assert r.returncode!=0,'Unchanged R4 unexpectedly passed the new budget regression'
assert "assert call(exports['DriTestHsDecision'],first,team,context)==0" in r.stderr,r.stderr
assert r.stderr.rstrip().endswith('AssertionError'),r.stderr
result={'unchanged_r4_fails_inherited_surplus_case':True,'live_game_verified':False}
(output/'test28r5_negative_control_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
