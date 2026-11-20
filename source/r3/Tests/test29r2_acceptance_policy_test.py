"""Current acceptance policy: ordinary R2 budget plus existing R4 quality permission."""
from pathlib import Path
import json, os, subprocess
source=Path(__file__).resolve().parent
out=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',source.parent/'build/test29r2_acceptance_policy'))
out.mkdir(parents=True,exist_ok=True)
binary=out/'test29r2_acceptance_policy'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',str(source/'test29r2_acceptance_policy_test.c'),'-o',str(binary)],check=True)
result=subprocess.run([str(binary)],text=True,capture_output=True,check=True)
(out/'r2_acceptance_policy.json').write_text(json.dumps({'checks':result.stdout.splitlines(),
 'supersedes':'Historical R1/R2 material/capacity blanket vetoes also fail against preserved R5; current R4 quality-depth coverage runs separately. This test retains market selection and isolates ordinary non-quality shared-budget paths.',
 'method':'compiled actual policy and planner, controlled floor-boundary projections','live_game_verified':False},indent=2)+'\n')
print(result.stdout,end='')
