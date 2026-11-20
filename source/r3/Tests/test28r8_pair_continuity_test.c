#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hs28_pairs.h"
static Hs28Choice c(int id,int score) {
 return (Hs28Choice){.table=1,.rid=(uint32_t)id,.pos=0,.ovr=70,.native_score=score,.legal=1,.viable=1,.existing=1,.offered=1};
}
static Hs28Prior prior(Hs28Choice *p) {
 return (Hs28Prior){p->table,p->rid,p->pos,p->slot,p->choice,p->order,p->native_score,0,0};
}
int main(void) {
 int v[22]={1},o[22]={0};Hs28Choice p[5]={c(10,500),c(20,400),c(30,600)};Hs28Prior old[5];
 hs28_plan(p,2,v,o,0,50);old[0]=prior(&p[0]);old[1]=prior(&p[1]);
 /* A new score leader cannot silently replace a valid funded primary. */
 assert(hs28_plan_prior(p,3,v,o,0,50,old,2)==2);
 assert(p[0].choice==1 && p[1].choice==2 && !p[2].selected);
 /* Lost A opens the role: compare existing B with stronger eligible C. */
 p[0].legal=0;hs28_plan_prior(p,3,v,o,0,50,old,2);
 assert(!p[0].selected && p[2].choice==1 && p[2].weight==40 && p[1].choice==2 && p[1].weight==20);
 old[0]=prior(&p[2]);old[1]=prior(&p[1]);p[2].legal=0;
 assert(hs28_plan_prior(p,3,v,o,0,50,old,2)==1 && p[1].choice==1 && p[1].weight==40);
 p[1].legal=0;p[2].legal=1;assert(hs28_plan_prior(p,3,v,o,0,50,old,2)==1 && p[2].choice==1);
 /* A filled opening does not replenish, even with an eligible fallback. */
 v[0]=0;assert(hs28_plan_prior(p,3,v,o,0,50,old,2)==0);
 /* Closing one of two seats preserves the OTHER pair and its fallback. */
 v[0]=2;p[0]=c(10,500);p[1]=c(20,400);p[2]=c(30,300);p[3]=c(40,200);p[4]=c(50,100);
 hs28_plan(p,5,v,o,0,50);for(int j=0;j<4;j++)old[j]=prior(&p[j]);
 assert(p[0].choice==1 && p[1].choice==1 && p[2].choice==2 && p[3].choice==2);
 for(int j=0;j<4;j++)if(old[j].slot==old[0].slot)old[j].closed=1;
 v[0]=1;p[0].legal=0;hs28_plan_prior(p,5,v,o,0,50,old,4);
 assert(p[1].choice==1 && p[3].choice==2 && !p[2].selected && p[1].slot==p[3].slot);
 /* Reload without a prior plan reconstructs native score ordering. */
 p[0].legal=1;p[0].native_score=100;p[1].native_score=500;
 hs28_plan_prior(p,2,v,o,0,50,0,0);assert(p[1].choice==1 && p[0].choice==2);
 /* Deadline ranking consumes the SAME native score, not a quality-only sort. */
 p[0].ovr=99;p[1].ovr=60;p[0].final=p[1].final=1;
 assert(hs28_better(&p[1],&p[0]));
 puts("PASS: primary continuity, fallback comparison/promotion/replenishment, no replacement, both lost, closed seat remap, reload scorer and deadline score consistency");
}
