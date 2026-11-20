"""Compile unchanged monitor assertions with current release labels."""
from pathlib import Path
import subprocess,json
ROOT=Path(__file__).resolve().parents[1]
import os
OUTPUT=Path(os.environ['DRI_TEST_OUTPUT_DIR'])
checks=[]
for name,original in [('launcher','test15r1_launcher_monitor_test.c'),('log','test15r1_log_monitor_test.c')]:
 text=(ROOT/'Tests'/original).read_text()
 if name=='launcher':text=text.replace('Test28R12','Test29R4-TU1029R5').replace('Test28R7_previous_version','Test29R4_previous_version')
 source=OUTPUT/(name+'.c');source.write_text(text);exe=OUTPUT/name
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(source),'-o',str(exe)],check=True)
 result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
 print(result.stdout);checks.append({'name':name,'returncode':result.returncode})
(OUTPUT/'test29r4_launcher_results.json').write_text(json.dumps({'checks':checks,'passed':True},indent=2))
