"""Actual C crowding tiers and shared pair planner, compiled natively."""
from pathlib import Path
import subprocess,re,os,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR'])
source=(S/'hs32_crowding.inc').read_text()
tier=re.search(r'static int hs32_penalty\(int count\) \{[^}]+\}',source).group()
c='#include <assert.h>\n#include "hs28_pairs.h"\n'+tier+r'''
int main(void) {
 int counts[]={0,5,6,9,10,14,15,137},want[]={0,0,10,10,25,25,45,45};
 for(int i=0;i<8;i++)assert(hs32_penalty(counts[i])==want[i]);
 int v[22]={1},o[22]={0};
 Hs28Choice p[3]={{.rid=1,.pos=0,.native_score=500,.ovr=78,.legal=1,.viable=1},
 {.rid=2,.pos=0,.native_score=480,.ovr=77,.legal=1,.viable=1},
 {.rid=3,.pos=0,.native_score=470,.ovr=77,.legal=1,.viable=1}};
 assert(hs28_plan(p,3,v,o,0,2)==2 && p[0].choice==1);
 p[0].crowd_penalty=10;
 assert(hs28_plan(p,3,v,o,0,2)==2 && !p[0].selected && p[1].choice==1);
 p[0].crowd_penalty=25;
 assert(hs28_plan(p,2,v,o,0,2)==2 && p[1].choice==1 && p[0].choice==2);
 p[0].crowd_penalty=45;
 assert(hs28_plan(p,2,v,o,0,2)==1 && !p[0].selected && p[1].choice==1);
 Hs28Prior old={.table=0,.rid=1,.pos=0,.slot=1,.choice=1,.order=1};
 p[0].existing=1;
 assert(hs28_plan_prior(p,2,v,o,0,2,&old,1)==1 && !p[0].selected);
 p[0].crowd_penalty=0;p[0].offered=1;
 assert(hs28_plan_prior(p,2,v,o,0,2,&old,1)==2 && p[0].choice==1);
 return 0;
}
'''
path=O/'crowding-policy.c';path.write_text(c)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(S),str(path),'-o',str(O/'crowding-policy')],check=True)
subprocess.run([str(O/'crowding-policy')],check=True)
(O/'test28r14_policy_results.json').write_text(json.dumps({'threshold_boundaries':[0,5,6,9,10,14,15,137],'native_score_unchanged':True,'lower_tiers_reorder':True,'highest_tier_defers_new_entry':True,'unoffered_old_plan_cannot_bypass':True,'existing_offered_primary_retained':True,'live_game_verified':False},indent=2)+'\n')
print('PASS crowding tier boundaries and shared pair selection')
