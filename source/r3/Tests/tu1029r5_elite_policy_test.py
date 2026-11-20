"""Actual compiled policy: saved Miami score comparisons and Georgia-shaped allocation.
Recorded numeric inputs are exact where stated; hypothetical controls are explicit.
No live spending/signing claim.
"""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
c=r'''
#include <assert.h>
#include <string.h>
#include "test29_class.h"
#include "hs28_pairs.h"
static Hs28Choice c(int id,int ovr,int score,int stars) {
 return (Hs28Choice){.table=1,.rid=(uint32_t)id,.pos=14,.ovr=ovr,.native_score=score,
  .stars=stars,.existing=1,.offered=1,.viable=1,.legal=1};
}
int main(void) {
 /* Miami actual W1, W2 and W5 scores; recruit80 vs73. */
 int elite_scores[]={395,455,470},primary_scores[]={420,460,455};
 for(int i=0;i<3;i++) {
  Hs28Choice a=c(1655,80,elite_scores[i],4),b=c(1790,73,primary_scores[i],3);
  assert(hs28_better(&a,&b));assert(hs28_material_upgrade(&a,&b));
  assert(a.native_score==elite_scores[i] && b.native_score==primary_scores[i]);
  b.stage=4;assert(!hs28_material_upgrade(&a,&b));
 }
 Hs28Choice p[2]={c(1790,73,420,3),c(1655,80,395,4)};
 Hs28Prior prior={.table=1,.rid=1790,.pos=14,.slot=449,.choice=1,.order=1};
 int v[22]={0},o[22]={0};v[14]=1;
 assert(hs28_plan_prior(p,2,v,o,0,50,&prior,1)==2);
 assert(p[1].choice==1 && p[0].choice==2 && p[1].slot==p[0].slot);
 /* Native fit can still outweigh talent. No upgrade on tiny net value,
    <four OVR, below-three-star, nonviable, illegal or market-blocked challenger. */
 for(int k=0;k<7;k++) {
  Hs28Choice a=c(2,80,395,4),b=c(1,73,420,3);
  if(k==0)a.native_score=300;
  if(k==1)a.native_score=368; /* net+4, below5 */
  if(k==2)a.ovr=76;
  if(k==3)a.stars=1;
  if(k==4)a.viable=0;
  if(k==5)a.legal=0;
  if(k==6)a.market_known=1;
  assert(!hs28_material_upgrade(&a,&b));
 }
 Test29ClassInput in={0};Test29ClassPlan out;
 for(int j=0;j<22;j++)in.ideal[j]=in.returning[j]=4;
 in.general=2;in.optional[0]=in.optional[4]=in.optional[12]=in.optional[14]=1;
 in.candidates[0]=in.candidates[4]=in.candidates[12]=in.candidates[14]=1;
 in.junior[12]=3;in.junior[4]=2;in.junior[14]=1;
 Hs28Choice mlb=c(1655,80,480,4),dt=c(2062,71,475,3),te=c(1945,76,425,3),qb=c(99999,69,300,2);
 in.optional_value[14]=(int)(hs28_selection_value(&mlb)/100);
 in.optional_value[12]=(int)(hs28_selection_value(&dt)/100);
 in.optional_value[4]=(int)(hs28_selection_value(&te)/100);
 in.optional_value[0]=(int)(hs28_selection_value(&qb)/100);
 test29_class_plan(&in,&out);
 assert(out.optional[14] && out.optional[12] && !out.optional[4] && !out.optional[0] && out.general==2);
 /* Negative control: historical allocation ignores prospect value. */
 memset(in.optional_value,0,sizeof(in.optional_value));test29_class_plan(&in,&out);
 assert(out.optional[12] && out.optional[4] && !out.optional[14]);
 in.general=0;test29_class_plan(&in,&out);assert(!out.planned);
 return 0;
}
'''
p=O/'elite_policy.c';p.write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(p),'-o',str(O/'elite_policy')],check=True)
subprocess.run([str(O/'elite_policy')],check=True)
result={'passed':True,'checks':['Miami actual three score pairs favor80over73 without rewriting native scores','uncommitted primary upgrade retains fallback in same place','soft commit, fit, improvement, legality, viability and market controls','Georgia-shaped optional allocation with actual MLB/DT/TE ratings and scores chooses MLB+DT; hypothetical QB','old cohort-only negative control chooses DT+TE','no new shared allowance'],'live_game_verified':False}
(O/'tu1029r5_elite_policy_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
