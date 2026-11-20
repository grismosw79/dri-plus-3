"""Production battle diagnostic preserves distinctions needed for live review.

Pure C logging code is compiled directly. These tests do not simulate recruiting
outcomes; integration is the existing completed-board coverage observer.
"""
from pathlib import Path
import csv,hashlib,json,os,subprocess
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test32-battles'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "hs28_pairs.h"
static void log_line(const char *fmt,...){va_list a;va_start(a,fmt);vprintf(fmt,a);va_end(a);putchar('\n');}
#include "test32_battles.inc"
int main(void){
 Hs28Choice p={.stars=4,.pos=9,.stage=2,.choice=1,.selected=1,.points=410,.leader=500,.rank=2,.commit_score=800};
 Hs28Choice old=p;
 Test32BattleRow r=test32_battle_row(&p,1234,2,40,32,390,2,21,300,800,1),before=r;
 test32_battle_emit(&r,"POST_FUNDING",2026,1,5,2,1);
 assert(!memcmp(&p,&old,sizeof(p)) && !memcmp(&r,&before,sizeof(r)));
 /* Partial/over-budget observation cannot count as currently funded. */
 test32_battle_emit(&r,"POST_FUNDING",2026,1,5,2,0);
 r.assigned=801;test32_battle_emit(&r,"POST_FUNDING",2026,1,5,2,1);
 /* END may retain a genuine earlier funding witness after action reset. */
 r.assigned=0;r.productive=0;r.latest_gain=-4;
 test32_battle_emit(&r,"END",2026,1,5,2,1);
 /* Conditional and unavailable offers remain distinguishable. */
 r.assigned=40;r.productive=40;r.role=2;r.state=1;
 test32_battle_emit(&r,"POST_FUNDING",2026,1,6,2,1);
 r.role=0;r.state=0;r.funded_in_scope=0;
 test32_battle_emit(&r,"POST_FUNDING",2026,1,6,2,1);
 /* Outside displayed list: retain native target total and rank=0. */
 p.points=650;p.rank=0;r=test32_battle_row(&p,1234,2,40,32,650,2,25,40,800,1);
 test32_battle_emit(&r,"END",2026,1,6,3,1);
 /* New, lower-star and BEGIN candidates are not offered elite witnesses. */
 r.status=1;test32_battle_emit(&r,"END",2026,1,6,3,1);
 r.status=2;r.stars=3;test32_battle_emit(&r,"END",2026,1,6,3,1);
 r.stars=5;test32_battle_emit(&r,"BEGIN",2026,1,6,3,1);
 return 0;
}
'''
(O/'battles.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(S),str(O/'battles.c'),'-o',str(O/'battles')],check=True)
rows=list(csv.reader(subprocess.check_output([str(O/'battles')],text=True).splitlines()))
columns=['event','site','year','type','week','team','recruit','stars','position','stage','role','offer_state','status','productive_hours','action_mask','target_total','choice_points','display_leader','display_gap','display_rank','threshold','latest_native_gain','assigned_hours','total_hours','complete_valid_budget','currently_funded','funded_in_scope']
assert len(rows)==7 and all(len(r)==len(columns) for r in rows)
data=[dict(zip(columns,r)) for r in rows]
assert data[0]['currently_funded']=='1' and data[0]['display_gap']=='90'
assert data[0]['target_total']=='390' and data[0]['choice_points']=='410','retain both observed native quantities; do not silently equate'
assert data[1]['currently_funded']=='0' and data[2]['currently_funded']=='0'
assert data[3]['currently_funded']=='0' and data[3]['funded_in_scope']=='1' and data[3]['latest_native_gain']=='-4'
assert data[4]['offer_state']=='1' and data[4]['role']=='2' and data[4]['currently_funded']=='1'
assert data[5]['offer_state']=='0' and data[5]['currently_funded']=='0'
assert data[6]['display_rank']=='0' and data[6]['target_total']=='650' and data[6]['display_gap']=='-150'
result={'status':'PASS','columns':columns,'checks':['Read-only input preservation','Complete-board budget required for current funding','Current versus earlier-scope funding separated','Firm, conditional and unavailable roles explicit','Outside-list earned influence retained','Native gain may be negative; displayed gap may be negative','POST_FUNDING versus END and year/type/week scoped','Nonoffered/nonelite/BEGIN suppressed'], 'production_sha256':hashlib.sha256((S/'test32_battles.inc').read_bytes()).hexdigest(),'sample_rows':data,'live_game_verified':False}
(O/'test32_battles_results.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS Test32 battle evidence: '+str(len(result['checks']))+' checks; observational, not live outcomes')
