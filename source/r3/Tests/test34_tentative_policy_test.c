#include <assert.h>
#include <stdio.h>
#include "hs28_pairs.h"
static Hs28Choice c(int id,int fit,int ovr) {
 return (Hs28Choice){.table=1,.rid=(unsigned)id,.pos=1,.ovr=ovr,.stars=3,
  .native_score=fit,.legal=1,.viable=1,.existing=1,.pursuit_state=1};
}
static Hs28Prior prior(Hs28Choice p,int role,int slot,int order) {
 return (Hs28Prior){.table=p.table,.rid=p.rid,.pos=p.pos,.slot=slot,
  .choice=role,.order=order,.native_score=p.native_score};
}
static void plan(Hs28Choice *p,int n,Hs28Prior *old,int no,int seats) {
 int v[22]={0},o[22]={0};v[1]=seats;
 hs28_plan_prior(p,n,v,o,0,50,old,no);
}
int main(void) {
 Hs28Choice p[4]={c(1,400,74),c(2,450,74),c(3,300,73)};
 Hs28Prior old[4]={prior(p[0],1,33,1),prior(p[2],2,33,2)};
 assert(!hs28_material_upgrade(&p[1],&p[0]));
 assert(hs34_primary_upgrade(&p[1],&p[0]));
 plan(p,3,old,2,1);
 assert(p[1].choice==1 && p[0].choice==2 && !p[2].selected);
 assert(p[0].slot==p[1].slot && p[1].slot==33);
 assert(p[0].native_score==400 && p[1].native_score==450);
 /* Paid, earned, and unknown evidence preserve historical +4 protection. */
 for(int state=0;state<=2;state+=2) {
  p[0]=c(1,400,74);p[0].offered=1;p[0].pursuit_state=state;
  p[1]=c(2,800,74);p[2]=c(3,300,73);
  plan(p,3,old,2,1);assert(p[0].choice==1 && p[1].choice!=1);
  p[1].ovr=78;assert(hs34_primary_upgrade(&p[1],&p[0]));
 }
 /* Never use tentative status to displace soft ownership or bypass gates. */
 for(int k=0;k<7;k++) {
  p[0]=c(1,400,74);p[1]=c(2,800,74);
  if(k==0)p[0].stage=4;
  if(k==1)p[1].legal=0;
  if(k==2)p[1].viable=0;
  if(k==3)p[1].native_score=0;
  if(k==4)p[1].market_known=1;
  if(k==5)p[1].crowd_penalty=5;
  if(k==6)p[1].pos=2;
  assert(!hs34_primary_upgrade(&p[1],&p[0]));
 }
 /* Earned interest remains the existing tie-breaker; ratings/native scores
    are not fabricated to force an upgrade. */
 p[0]=c(1,400,74);p[1]=c(2,400,74);p[1].points=10;
 assert(hs34_primary_upgrade(&p[1],&p[0]));
 p[1].points=0;p[1].native_score=390;assert(!hs34_primary_upgrade(&p[1],&p[0]));
 /* Another live primary's seat cannot be stolen in a local reconsideration. */
 p[0]=c(1,400,74);p[1]=c(2,800,74);p[2]=c(3,300,73);p[3]=c(4,290,73);
 old[0]=prior(p[0],1,33,1);old[1]=prior(p[1],1,34,2);
 old[2]=prior(p[2],2,33,3);old[3]=prior(p[3],2,34,4);
 plan(p,4,old,4,2);assert(p[0].choice==1 && p[0].slot==33 && p[1].choice==1 && p[1].slot==34);
 puts("PASS: tentative renewal, paid/unknown continuity, soft protection, legality/admission, native interest and distinct seats");
 return 0;
}
