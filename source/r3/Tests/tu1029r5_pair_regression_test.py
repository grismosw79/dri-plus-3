"""Compile retained pair-capacity/continuity tests and updated upgrade contract."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
checks=['test28r6_pairs_test','test28r8_pair_continuity_test','test28r12_primary_test']
for n in checks:
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(S/'Tests'/f'{n}.c'),'-o',str(O/n)],check=True)
 subprocess.run([str(O/n)],check=True)
(O/'tu1029r5_pair_regression_results.json').write_text(json.dumps({'passed':True,'checks':checks,'policy_revision':'Test31 material upgrade uses positive native fit, four OVR and five net utility points without redundant 90% fit ratio; negative utility case 294vs330. Lost-primary backup is compared with eligible alternatives. Nonviable/nonpositive placeholders excluded.','live_game_verified':False},indent=2)+'\n')
