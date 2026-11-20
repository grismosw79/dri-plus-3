#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hs28_pairs.h"
static Hs28Choice c(int id,int native,int ovr,int stars) {
 return (Hs28Choice){.table=1,.rid=(uint32_t)id,.pos=10,.native_score=native,.ovr=ovr,
  .stars=stars,.legal=1,.viable=1,.existing=1,.offered=1};
}
static Hs28Prior prior(Hs28Choice x,int role,int slot,int order) {
 return (Hs28Prior){.table=x.table,.rid=x.rid,.pos=x.pos,.slot=slot,.choice=role,.order=order,.native_score=x.native_score};
}
static int plan(Hs28Choice *p,int n,Hs28Prior *old,int no,int seats) {
 int v[22]={0},o[22]={0};v[10]=seats;return hs28_plan_prior(p,n,v,o,0,50,old,no);
}
int main(void) {
 /* Saved Test30 Alabama LE shape. IDs are fixture-local; score/OVR are
    the saved audit's 72/447 incumbent and 82/382 eligible backup. */
 Hs28Choice p[5]={c(1,447,72,3),c(2,382,82,4),c(3,300,70,2)};
 Hs28Prior old[5]={prior(p[0],1,321,1),prior(p[1],2,321,2)};
 assert(hs28_selection_value(&p[0])==50300 && hs28_selection_value(&p[1])==51800);
 int native0=p[0].native_score,native1=p[1].native_score;
 plan(p,3,old,2,1);
 assert(p[0].native_score==native0 && p[1].native_score==native1);
#ifdef DRI_PREDECESSOR_NEGATIVE
 assert(p[0].choice==1 && p[1].choice==2);
 p[0]=c(10,500,70,2);p[1]=c(11,400,70,2);p[1].viable=0;
 assert(plan(p,2,0,0,1)==2 && p[1].choice==2);
 puts("PREDECESSOR_REPRODUCED: Alabama +10 OVR/+15 utility denied; nonviable sole backup selected");
 return 0;
#else
 assert(p[1].choice==1 && p[0].choice==2 && !p[2].selected);
 assert(p[0].native_score==native0 && p[1].native_score==native1);
 /* Three-stars can be useful upgrades; star label never repairs a poor
    net result, invalid race or failed market authorization. */
 p[0]=c(1,447,72,2);p[1]=c(2,382,82,2);
 assert(hs28_material_upgrade(&p[1],&p[0]));
 for(int k=0;k<9;k++) {
  p[0]=c(1,447,72,2);p[1]=c(2,382,82,3);
  if(k==0)p[1].native_score=371; /* +4 utility < five-point reassessment margin */
  if(k==1)p[1].ovr=75;
  if(k==2)p[1].viable=0;
  if(k==3)p[1].legal=0;
  if(k==4)p[1].market_known=1;
  if(k==5)p[1].crowd_penalty=10;
  if(k==6)p[0].stage=4;
  if(k==7)p[1].native_score=0;
  if(k==8)p[1].stars=1;
  assert(!hs28_material_upgrade(&p[1],&p[0]));
 }
 /* A lost primary's backup is evaluated as a contender for the vacant role.
    A one-OVR/one-utility improvement can win this NEW role without triggering
    incumbent displacement thresholds. The fallback remains a usable backup. */
 p[0]=c(10,500,72,2);p[1]=c(11,400,72,2);p[2]=c(12,393,73,2);
 old[0]=prior(p[0],1,321,1);old[1]=prior(p[1],2,321,2);p[0].legal=0;
 assert(plan(p,3,old,2,1)==2 && p[2].choice==1 && p[1].choice==2 && !p[0].selected);
 assert(p[2].slot==p[1].slot && p[2].weight==40 && p[1].weight==20);
 p[2].native_score=391;plan(p,3,old,2,1);assert(p[1].choice==1 && p[2].choice==2);
 p[2].native_score=600;p[1].stage=4;plan(p,3,old,2,1);assert(p[1].choice==1);
 /* LSU-shaped sole fallback has a positive score but no viable race.
    Neither it nor a zero/negative score can be a cosmetic backup. */
 for(int k=0;k<3;k++) {
  p[0]=c(20,500,75,3);p[1]=c(21,450,74,3);
  if(k==0)p[1].viable=0;
  if(k==1)p[1].native_score=0;
  if(k==2)p[1].native_score=-1;
  assert(plan(p,2,0,0,1)==1 && p[0].choice==1 && !p[1].selected);
  p[0].legal=0;assert(plan(p,2,0,0,1)==0);
 }
 /* Existing viable primary keeps continuity despite a score-only newcomer;
    this improvement does not entail a broad weekly wholesale board rewrite. */
 p[0]=c(30,400,75,2);p[1]=c(31,390,73,2);p[2]=c(32,800,75,3);
 old[0]=prior(p[0],1,321,1);old[1]=prior(p[1],2,321,2);
 for(int k=0;k<4;k++){plan(p,3,old,2,1);assert(p[0].choice==1 && p[1].choice==2 && !p[2].selected);}
 /* Separate live primary cannot be stolen as a replacement; when a seat
    closes, its former backup does not erase the remaining pair. */
 p[0]=c(40,500,72,2);p[1]=c(41,600,80,3);p[2]=c(42,300,70,2);p[3]=c(43,290,70,2);
 old[0]=prior(p[0],1,321,1);old[1]=prior(p[1],1,322,2);
 old[2]=prior(p[2],2,321,3);old[3]=prior(p[3],2,322,4);
 p[0].legal=0;plan(p,4,old,4,2);assert(p[1].slot==322 && p[1].choice==1 && p[2].slot==321 && p[2].choice==1);
 old[0].closed=old[2].closed=1;plan(p,4,old,4,1);
 assert(p[1].choice==1 && p[3].choice==2 && !p[2].selected && p[1].slot==p[3].slot);
 /* Soft cap may prefer an alternative, but does not erase a sole authorized,
    viable fallback when no close alternative exists. Hard authorization
    remains independent. */
 p[0]=c(50,500,80,3);p[1]=c(51,450,78,3);p[1].market_roles=1;p[1].backup_penalty=45;
 p[1].market_known=p[1].market_planned=1;
 assert(plan(p,2,0,0,1)==2 && p[1].choice==2);
 p[1].market_planned=0;assert(plan(p,2,0,0,1)==1 && !p[1].selected);
 /* Closing all annual seats creates no replacement and consumes no place. */
 assert(plan(p,2,old,4,0)==0);
 puts("PASS: Alabama saved shape, useful-three-star renewal, guarded continuity, lost-primary comparison, usable fallback, separate openings and market authorization");
 return 0;
#endif
}
