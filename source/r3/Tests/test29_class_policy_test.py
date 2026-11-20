"""Compile the actual class/source policy; no live signing outcome is inferred."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
c=r'''
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "test29_class.h"
#include "hs28_pairs.h"
static Test29ClassInput regular(void) {
 Test29ClassInput in={0};
 for(int p=0;p<20;p++){in.ideal[p]=4;in.returning[p]=3;in.vacancy[p]=1;in.candidates[p]=3;}
 return in;
}
int main(void) {
 Test29ClassInput in=regular();Test29ClassPlan out;
 test29_class_plan(&in,&out);assert(out.target==20 && out.planned==20 && out.shortfall==0);
 /* Already secured signings reduce pursuit places, not roster headcount. */
 for(int p=0;p<10;p++){in.secured[p]=1;in.vacancy[p]=0;}
 test29_class_plan(&in,&out);assert(out.secured==10 && out.planned==10);
 /* No late forced top-up on a full roster. */
 memset(&in,0,sizeof(in));
 for(int p=0;p<22;p++){in.ideal[p]=4;in.returning[p]=4;in.candidates[p]=3;}
 in.general=2;in.optional[3]=in.optional[4]=in.optional[5]=1;
 test29_class_plan(&in,&out);assert(out.target==15 && out.planned==2 && out.shortfall==13);
 assert(out.general==2);
 /* A large ordinary replacement class reserves the rest for the portal. */
 in=regular();for(int p=0;p<20;p++){in.returning[p]=2;in.vacancy[p]=2;}
 test29_class_plan(&in,&out);assert(out.target==25 && out.planned==25 && !out.replacement_exception);
 /* HS-only academies must expose genuine replacement exceptions. */
 in.academy=1;test29_class_plan(&in,&out);assert(out.target==40 && out.planned==40 && out.replacement_exception);
 /* Current-cohort exposure cannot invent headcount. */
 in=regular();in.junior[5]=28;in.vacancy[5]=0;
 test29_class_plan(&in,&out);assert(out.vacancy[5]==0 && out.planned==19 && out.shortfall==1);
 /* Scarce candidates limit modeled attainability. */
 in=regular();in.candidates[19]=0;test29_class_plan(&in,&out);assert(!out.vacancy[19] && out.planned==19);
 /* Existing credible funded primaries survive target reduction. */
 in=regular();in.secured[21]=25;in.protected_vacancy[3]=1;
 test29_class_plan(&in,&out);assert(out.planned==1 && out.protected_over_target==1 && out.vacancy[3]==1);
 /* Only the3 academies get the contextual native-positive preference. */
 assert(test29_class_academy(0) && test29_class_academy(7) && test29_class_academy(55));
 assert(!test29_class_academy(67));
 assert(test29_academy_preference(1,2,2,80,100,1,200)==20);
 assert(test29_academy_preference(1,1,9,0,100,1,200)==10);
 assert(!test29_academy_preference(1,3,2,80,100,1,200));
 assert(!test29_academy_preference(1,4,2,80,100,1,200));
 assert(!test29_academy_preference(0,2,2,80,100,1,200));
 assert(!test29_academy_preference(1,2,2,80,100,1,0));
 Hs28Choice p[2]={{.rid=1,.legal=1,.viable=1,.native_score=500,.ovr=78},
 {.rid=2,.legal=1,.viable=1,.native_score=480,.ovr=77,.academy_preference=20}};
 assert(hs28_better(&p[1],&p[0]) && p[1].native_score==480);
 p[0].native_score=600;assert(hs28_better(&p[0],&p[1]));
 /* New field occupies tail padding; existing numeric/player offsets intact. */
 assert(offsetof(Hs28Choice,academy_preference)+4==sizeof(Hs28Choice));
 /* Mixed funding: prioritize HS primaries onlywithin the same nativebucket. */
 uint64_t indices[6]={0,1,2,3,4,5};int bucket[6]={0,1,0,1,0,1},priority[6]={0,0,1,1,0,0};
 assert(test29_source_prioritize(6,indices,bucket,priority)==4);
 uint64_t expected[6]={2,3,0,1,4,5};assert(!memcmp(indices,expected,sizeof(expected)));
 for(int i=0;i<6;i++)assert(bucket[indices[i]]==i%2);
 uint64_t bad[3]={0,0,2},before[3]={0,0,2};assert(test29_source_prioritize(3,bad,bucket,priority)==-1);
 assert(!memcmp(bad,before,sizeof(bad)));
 /* Compose class capacity and the paired planner: two real vacancies can
    use the crowded candidate only after the sole close alternative fills. */
 in=regular();memset(in.vacancy,0,sizeof(in.vacancy));memset(in.candidates,0,sizeof(in.candidates));
 in.vacancy[0]=in.candidates[0]=2;test29_class_plan(&in,&out);
 Hs28Choice scarce[2]={{.rid=1,.legal=1,.viable=1,.native_score=500,.ovr=78,.crowd_penalty=45},
 {.rid=2,.legal=1,.viable=1,.native_score=480,.ovr=77}};
 assert(hs28_plan_prior(scarce,2,out.vacancy,out.optional,out.general,50,0,0)==2);
 assert(scarce[0].choice==1 && scarce[1].choice==1 && scarce[1].order<scarce[0].order);
 in.vacancy[0]=1;test29_class_plan(&in,&out);
 assert(hs28_plan_prior(scarce,2,out.vacancy,out.optional,out.general,50,0,0)==1);
 assert(!scarce[0].selected && scarce[1].choice==1);
 memset(in.vacancy,0,sizeof(in.vacancy));in.optional[0]=1;in.general=1;
 in.candidates[0]=1;scarce[1].legal=0;test29_class_plan(&in,&out);
 assert(hs28_plan_prior(scarce,2,out.vacancy,out.optional,out.general,50,0,0)==0);
 return 0;
}
'''
(O/'class-policy.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(O/'class-policy.c'),'-o',str(O/'class-policy')],check=True)
subprocess.run([str(O/'class-policy')],check=True)
(O/'test29_class_policy_results.json').write_text(json.dumps({'passed':True,'checks':['normal20','secured-signing-accounting','no-forced-full-roster-topup','normal25-source-cap','academy-replacement-exception','cohort-cannot-invent-seat','candidate-scarcity','existing-funded-primary-continuity','academy-only-context-preference','native-score-unchanged','choice-layout-tail-padding','mixed-source-funding-stable-native-buckets','bad-permutation-no-write','crowding-fallback-only-for-unfilled-quantity-primary','crowded-backup-and-optional-still-deferred'],'live_game_verified':False},indent=2)+'\n')
print('PASS Test29 class/source and academy policy: capacity, cohorts, continuity, targeting')
