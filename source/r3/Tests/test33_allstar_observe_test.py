"""Compile the production all-star observer without native services.

This verifies schema and observational semantics only. Root integration uses
the existing complete-board observer; this test is not a game-week replay.
"""
from pathlib import Path
import csv,hashlib,json,os,subprocess
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test33-allstar-observe'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "hs28_pairs.h"
static void log_line(const char *fmt,...){va_list a;va_start(a,fmt);vprintf(fmt,a);va_end(a);putchar('\n');}
#include "test33_observe.inc"
static void emit_unchanged(Test33HsExecutionRow *r,const char *site,int board_valid){
 Test33HsExecutionRow before=*r;
 test33_execution_emit(r,site,2026,1,1,34,board_valid);
 assert(!memcmp(r,&before,sizeof(*r)));
}
int main(void){
 Hs28Choice p={.stars=0,.pos=16,.stage=0,.choice=1,.selected=1,.points=61,.leader=90};
 for(int stars=1;stars<=5;stars++){
  p.stars=stars-1;Hs28Choice before=p;
  Test33HsExecutionRow r=test33_execution_row(&p,1000+stars,2,50,8,66,12,100,1950,1);
  assert(!memcmp(&p,&before,sizeof(p)));
  emit_unchanged(&r,"POST_FUNDING",1);
 }
 /* Productive hours on an unoffered/cap-vetoed path remain visible but are
    not labelled currently funded, even if a caller supplied a valid plan. */
 p.stars=2;Test33HsExecutionRow r=test33_execution_row(&p,2484,0,50,8,109,48,1950,1950,1);
 emit_unchanged(&r,"POST_FUNDING",1);
 r.status=1;emit_unchanged(&r,"POST_FUNDING",1);
 /* All established native offer states2..4 can be observed. */
 r.status=3;emit_unchanged(&r,"POST_FUNDING",1);
 r.status=4;emit_unchanged(&r,"POST_FUNDING",1);
 /* Complete-board validity and both budget bounds are independent gates. */
 r.status=2;emit_unchanged(&r,"POST_FUNDING",0);
 r.assigned=1951;emit_unchanged(&r,"POST_FUNDING",1);
 r.assigned=49;emit_unchanged(&r,"POST_FUNDING",1);
 /* Role is explicit; selected backup is not falsely relabelled primary or
    firm. No firm/conditional classification is attempted in this helper. */
 p.choice=2;Hs28Choice backup_before=p;
 r=test33_execution_row(&p,2002,2,5,1,76,12,5,1950,1);
 assert(!memcmp(&p,&backup_before,sizeof(p)));
 emit_unchanged(&r,"POST_FUNDING",1);
 p.selected=0;r=test33_execution_row(&p,2002,2,5,1,76,12,5,1950,0);
 assert(r.role==0);emit_unchanged(&r,"POST_FUNDING",1);
 /* A genuine earlier allocation may reset atEND. Signed native loss is
    retained; there is intentionally no 'never funded' or historical flag. */
 p.selected=1;r=test33_execution_row(&p,2002,2,5,1,76,12,5,1950,1);
 emit_unchanged(&r,"POST_FUNDING",1);
 r.productive=0;r.actions=0;r.assigned=0;r.latest_gain=-9;
 emit_unchanged(&r,"END",1);
 /* Native target and displayed-school points need not match. */
 p.points=650;p.leader=500;r=test33_execution_row(&p,3000,2,5,1,625,7,5,1950,1);
 emit_unchanged(&r,"END",1);
 /* Invalid row families and phases are suppressed. */
 r.stars=0;emit_unchanged(&r,"END",1);
 r.stars=6;emit_unchanged(&r,"END",1);
 r.stars=3;r.status=5;emit_unchanged(&r,"END",1);
 r.status=2;emit_unchanged(&r,"BEGIN",1);
 emit_unchanged(&r,0,1);
 test33_execution_emit(0,"POST_FUNDING",2026,1,1,34,1);
 Test33HsExecutionRow absent=test33_execution_row(0,4000,2,5,1,625,7,5,1950,1);
 assert(absent.stars==0);emit_unchanged(&absent,"END",1);
 return 0;
}
'''
(O/'allstar-observe.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(S),str(O/'allstar-observe.c'),'-o',str(O/'allstar-observe')],check=True)
rows=list(csv.reader(subprocess.check_output([str(O/'allstar-observe')],text=True).splitlines()))
columns=['event','site','year','type','week','team','recruit','stars','position','stage','role','status','productive_hours','action_mask','native_target_total','native_choice_points','native_display_leader','latest_native_gain','assigned_hours','total_hours','current_plan_valid','complete_valid_budget','currently_funded']
assert len(rows)==17 and all(len(r)==len(columns) for r in rows)
data=[dict(zip(columns,r)) for r in rows]
assert all(r['event']=='TEST33_HS_EXECUTION' for r in data)
assert [r['stars'] for r in data[:5]]==['1','2','3','4','5']
assert all(r['currently_funded']=='1' for r in data[:5])
assert data[5]['status']=='0' and data[5]['productive_hours']=='50' and data[5]['currently_funded']=='0'
assert data[6]['status']=='1' and data[6]['currently_funded']=='0'
assert data[7]['status']=='3' and data[8]['status']=='4' and all(r['currently_funded']=='1' for r in data[7:9])
assert all(r['currently_funded']=='0' for r in data[9:12])
assert data[12]['role']=='2' and data[12]['currently_funded']=='1'
assert data[13]['role']=='0' and data[13]['current_plan_valid']=='0' and data[13]['currently_funded']=='0'
assert data[14]['currently_funded']=='1' and data[15]['site']=='END' and data[15]['currently_funded']=='0' and data[15]['latest_native_gain']=='-9'
assert data[16]['native_target_total']=='625' and data[16]['native_choice_points']=='650' and data[16]['native_display_leader']=='500'
assert 'funded_in_scope' not in columns and 'firm' not in columns
result={'status':'PASS','columns':columns,'checks':['All one-through-five-star HS rows observed, including two/three-star targets','Un offered status0/1 productive allocation visible but not currently funded','Native statuses2/3/4 preserved','Complete board and assigned/total bounds required','Explicit primary/secondary/unselected roles; no inferred firm state','POST_FUNDING versus END action reset distinguished without historical claim','Signed native gain and different native target/choice/displayed-leader values preserved','Null/invalid-star/invalid-status/BEGIN suppression','Choice and row inputs remain byte-identical'], 'production_sha256':hashlib.sha256((S/'test33_observe.inc').read_bytes()).hexdigest(),'sample_rows':data,'native_reads':0,'native_writes':0,'extra_board_scans':0,'live_game_verified':False}
(O/'test33_allstar_observe_results.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS Test33 all-star execution observer: '+str(len(result['checks']))+' checks; observational, not live outcomes')
