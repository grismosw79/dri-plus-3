"""Compile production competition policy and shared primary/backup planner.
Recorded W5 values are a mechanism regression, not a simulated season result.
"""
from pathlib import Path
import subprocess,os,json
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test32-competition'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <assert.h>
#include <stdio.h>
#include "competition32_policy.h"
static void add(Market29Pressure *m,unsigned t,int fit,int points,int gain) {
 m->fit[t]=fit;m->interest[t]=points;m->gain[t]=gain;
 market29_bit(m->primary,t,1);market29_bit(m->planned,t,1);market29_bit(m->firm,t,1);market29_bit(m->active,t,1);
}
int main(void) {
 /* Recorded W5: Iowa State's Mekhi Edmond82/427/421 was primary; Damion
    Perkins79/437/260 was backup everywhere. Three other Edmond firm paths
    had points525,611,658 and fit487,497,487 respectively. No gain was
    inferred from the W5 report: this isolated fixture models a stagnant race. */
 Market29Pressure m={0};add(&m,10,427,421,0);add(&m,11,487,525,0);
 add(&m,12,497,611,0);add(&m,13,487,658,0);
 Hs28Choice p[2]={
 {.rid=2814,.pos=5,.ovr=82,.stars=4,.native_score=427,.points=421,.leader=658,.rank=4,
  .legal=1,.viable=1,.existing=1,.offered=1,.market_roles=1,.market_protected=1},
 {.rid=2366,.pos=5,.ovr=79,.stars=4,.native_score=437,.points=260,.leader=415,
  .legal=1,.viable=1,.existing=1,.offered=1,.market_roles=1,.market_protected=1}};
 assert(!competition32_close(&p[0],&m,10));
 assert(competition32_better(&m,10,p[0].rid,2026)==3);
 p[0].crowd_penalty=competition32_opportunity(&p[0],&p[1],3,0,3,0);
 assert(p[0].crowd_penalty==25);
 int v[22]={0},o[22]={0};v[5]=1;
 Hs28Prior old[2]={{.rid=2814,.pos=5,.slot=161,.choice=1,.order=1},
  {.rid=2366,.pos=5,.slot=161,.choice=2,.order=2}};
 assert(hs28_plan_prior(p,2,v,o,0,2,old,2)==2);
 assert(p[1].choice==1 && p[0].choice==2);
 /* Legitimate Alabama/Cavka close pursuit remains protected. */
 Hs28Choice close=p[0];close.points=627;close.leader=637;close.rank=6;
 Market29Pressure c={0};add(&c,10,477,627,40);add(&c,11,512,637,40);
 assert(competition32_close(&close,&c,10));
 assert(!competition32_opportunity(&close,&p[1],12,0,12,1));
 /* Earned progress protects outside-list contenders too. Mere positive gain
    slower than the leader does not represent closing the influence gap. */
 close.points=500;close.leader=700;close.rank=12;c.interest[10]=500;c.interest[11]=700;
 c.gain[10]=120;c.gain[11]=30;assert(competition32_close(&close,&c,10));
 c.gain[10]=10;assert(!competition32_close(&close,&c,10));
 /* A stale ineligible/withdrawn1000-point school cannot distort a real
    500-versus480 race or pretend its missing gain is zero progress. */
 c.interest[90]=1000;c.fit[90]=0;c.interest[10]=480;c.interest[11]=500;
 close.points=480;close.leader=1000;assert(competition32_close(&close,&c,10));
 /* Keep soft commitments, the first three credible rivals, sole suitable
    targets, distant-quality alternatives, and nonelite selection unchanged. */
 close=p[0];close.stage=4;assert(competition32_close(&close,&m,10));
 close.stage=0;assert(!competition32_opportunity(&close,&p[1],2,0,2,0));
 assert(!competition32_opportunity(&close,&p[1],8,2,8,0));
 Hs28Choice bad=p[1];bad.ovr=78;assert(!competition32_opportunity(&close,&bad,8,0,8,0));
 bad=p[1];bad.stars=2;assert(!competition32_opportunity(&close,&bad,8,0,8,0));
 bad=p[1];bad.legal=0;assert(!competition32_opportunity(&close,&bad,8,0,8,0));
 bad=p[1];bad.native_score=200;assert(!competition32_opportunity(&close,&bad,8,0,8,0));
 /* The original thresholds and portal resolver stay unchanged. */
 assert(market29_penalty(0,4)==0 && market29_penalty(0,8)==45);
 assert(market29_penalty(1,5)==45 && market30_limit(0)==16 && market30_limit(1)==10);
 puts("PASS: recorded W5 pair promotion; close and improving challengers; stale leaders; quality/eligibility/soft-commit safeguards; unchanged soft and hard limits");
}
'''
(O/'competition32_policy.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(S),str(O/'competition32_policy.c'),'-o',str(O/'competition32_policy')],check=True)
p=subprocess.run([str(O/'competition32_policy')],capture_output=True,text=True,check=True);print(p.stdout)
(O/'test32_competition_policy_results.json').write_text(json.dumps({'status':'PASS','method':'compiled production helper and planner; recorded W5 values with explicit synthetic gain assumptions','live_game_verified':False,'checks':['Iowa State Edmond/Perkins recorded W5 fit-quality-interest pair becomes firm/backup after marginal primary reassessment','Alabama-like close rival and rank12 improving contender retain race','stale eliminated leader ignored once valid feasible race exists','no automatic all-backup firmness or extra class place','legal quality boundaries and unchanged original thresholds']},indent=2)+'\n')
