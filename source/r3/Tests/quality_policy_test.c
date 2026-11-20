#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../portal_policy.h"
static PortalQuality room(int pos,int ideal,int starters,int n,const int *ratings) {
 PortalQuality q={0};q.pos=pos;q.ideal=ideal;q.starters=starters;q.count=q.base_count=n;q.incoming_best=-1;
 memcpy(q.rating,ratings,(size_t)n*sizeof(int));return q;
}
int main(void) {
 /* Regression: both OLB positions use the approved ROLB source bands. */
 for(int g=0;g<5;g++)for(int k=0;k<22;k++)assert(dri_quality_floor[g][13][k]==dri_quality_floor[g][15][k]);
 assert(dri_quality_floor[4][13][0]==324 && dri_quality_floor[4][13][1]==292);
 int lb[]={74,70,68,65},lbrank;
 for(int p=13;p<=15;p+=2) {
  PortalQuality lbq=room(p,4,1,4,lb);
  assert(!dri_quality_block(&lbq,9,80));
  assert(dri_quality_bonus(&lbq,9,80,&lbrank)==290 && lbrank==1);
  assert(dri_quality_bonus(&lbq,9,84,&lbrank)==350 && lbrank==1);
 }
 int ratings[]={74,73,71,69},rank;PortalQuality q=room(0,4,1,4,ratings);
 assert(!dri_quality_block(&q,9,83));assert(dri_quality_bonus(&q,9,83,&rank)==335 && rank==1);
 assert(dri_quality_bonus(&q,9,91,&rank)==455 && rank==1);
 assert(dri_quality_block(&q,9,76)==1);
 int signed_ratings[]={83,74,73,71,69};q=room(0,4,1,5,signed_ratings);q.base_count=4;q.incoming[0]=1;q.incoming_best=83;
 assert(dri_quality_block(&q,9,91)==4);assert(dri_quality_block(&q,9,82)==2);assert(dri_quality_block(&q,9,77)==4);
 int hb[]={87,76,75,73,69,65};q=room(1,6,1,6,hb);
 /* Backup gain is eight over HB2=76, independent of the 87 starter. */
 assert(!dri_quality_block(&q,8,84));assert(dri_quality_bonus(&q,8,84,&rank)==220 && rank==2);
 assert(!dri_quality_block(&q,8,78)); /* Weak matrix HB3: 75 -> 78. */
 assert(dri_quality_block(&q,8,77)==1); /* A two-point depth change is insufficient. */
 /* A strong room receives no extra priority solely for a +3 improvement. */
 int strong[]={92,86,81,77};q=room(0,4,1,4,strong);assert(dri_quality_bonus(&q,9,95,&rank)==0);
 /* Already over-ideal rooms may still replace a starter once. */
 int crowded[]={74,73,71,69,68};q=room(0,4,1,5,crowded);assert(!dri_quality_block(&q,9,83));
 q.incoming_best=83;q.count=6;q.rating[0]=83;q.incoming[0]=1;assert(dri_quality_block(&q,9,79)==4);
 /* Lower reserve can fill a real remaining vacancy after a starter signs. */
 int two[]={88,70};q=room(0,4,1,2,two);q.base_count=1;q.incoming_best=88;q.incoming[0]=1;
 assert(!dri_quality_block(&q,9,77));assert(dri_quality_block(&q,9,90)==2);
 q.ideal=0;assert(dri_quality_block(&q,9,99)==3);
 for(int year=0;year<4;year++)for(int ovr=0;ovr<=99;ovr++) {
  int adjusted=ovr+(year==0?2:year==1?1:0),expected=(adjusted>=70)+(adjusted>=75)+(adjusted>=82)+(adjusted>=90);
  assert(dri_portal_star(ovr,year)==expected);
 }
 puts("PASS: Oregon starter pursuit/closure, Missouri backup gap, strong-room priority, pre-existing overages, native vacancies, all 400 star/year combinations.");
}
