"""Reparse all delivered FTC routes and independently recompute full expectations."""
from pathlib import Path
import os,subprocess,json,tempfile
ROOT=Path(__file__).resolve().parents[4]
OUTPUT=Path(os.environ['DRI_TEST_OUTPUT_DIR']).resolve() if os.environ.get('DRI_TEST_OUTPUT_DIR') else Path(tempfile.mkdtemp(prefix='dri-test31-generator-model-'))
OUTPUT.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['DRI_GENERATOR_REPORT_DIR']=str(OUTPUT);env['DRI_SAVE_PARSER']=str(ROOT/'Runtime/Portal/Audit/Test29/Inputs/Parser/save_adapter.cjs')
subprocess.run(['node',str(ROOT/'Runtime/Portal/Source/test31_generator_verify.cjs'),str(ROOT/'Runtime/Portal/Audit/Test31/Generator/Baseline'),str(ROOT)],env=env,check=True)

# Verify the new report against the immutable packaged ledger as well.
for route in range(5):
    name=f'route{route}_verification.json'
    packaged=json.loads((ROOT/'Runtime/Portal/Audit/Test31/Generator'/name).read_text())
    fresh=json.loads((OUTPUT/name).read_text())
    assert fresh==packaged,f'Generator route {route} differs from its packaged verification ledger'
