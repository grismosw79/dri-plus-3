"""Compiled planning regressions and optional immutable Test30 negative control.
No native score, interest, budget or ownership writer is exercised here; the
runtime suite separately checks admission/funding. These are not live outcomes.
"""
from pathlib import Path
import json, os, subprocess, hashlib
S=Path(__file__).resolve().parents[1]
O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
source=S/'Tests/test31_hs_selection_test.c'
results=[]
def run(name,include,negative=False):
 exe=O/name
 cmd=['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(include)]
 if negative:cmd+=['-DDRI_PREDECESSOR_NEGATIVE']
 subprocess.run(cmd+[str(source),'-o',str(exe)],check=True)
 result=subprocess.run([str(exe)],check=True,text=True,capture_output=True)
 print(result.stdout,end='')
 results.append({'test':name,'stdout':result.stdout.strip(),'header_sha256':hashlib.sha256((include/'hs28_pairs.h').read_bytes()).hexdigest()})
run('test31_hs_selection',S)
base=os.environ.get('DRI_TEST31_BASE_SOURCE')
if base:run('test30_hs_selection_negative',Path(base),True)
result={'passed':True,'method':'compiled exact production selection header; Test30 negative when supplied','checks':results,'live_game_verified':False,'native_funding_verified_by_this_test':False}
(O/'test31_hs_selection_results.json').write_text(json.dumps(result,indent=2)+'\n')
