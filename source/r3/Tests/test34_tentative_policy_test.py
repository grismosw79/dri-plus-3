"""Compile the exact production planning header; no live-game outcome claimed."""
from pathlib import Path
import hashlib,json,os,subprocess
S=Path(__file__).resolve().parents[1]
O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
exe=O/'test34_tentative_policy'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),
 str(S/'Tests/test34_tentative_policy_test.c'),'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
print(r.stdout,end='')
(O/'test34_tentative_policy_results.json').write_text(json.dumps({'passed':True,
 'production_header_sha256':hashlib.sha256((S/'hs28_pairs.h').read_bytes()).hexdigest(),
 'checks':r.stdout.strip(),'live_game_verified':False},indent=2)+'\n')
