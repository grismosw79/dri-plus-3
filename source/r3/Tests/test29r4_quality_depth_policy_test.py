"""Compiled R4 quality-depth permission and planning policy; no live-game claim."""
from pathlib import Path
import json, os, subprocess
source=Path(__file__).resolve().parent
out=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',source.parent/'build/test29r4_quality_depth_policy'))
out.mkdir(parents=True,exist_ok=True)
binary=out/'test29r4_quality_depth_policy'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',str(source/'test29r4_quality_depth_policy_test.c'),'-o',str(binary)],check=True)
result=subprocess.run([str(binary)],text=True,capture_output=True,check=True)
(out/'portal_policy_results.json').write_text(json.dumps({'checks':result.stdout.splitlines(),'method':'compiled C actual policy/planner; controlled projections','live_game_verified':False},indent=2)+'\n')
print(result.stdout,end='')
