#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../portal_allocation.h"
static PortalQuality room(int pos,int starters,int ideal,int count,const int *ratings) {
 PortalQuality q={0};q.pos=pos;q.starters=starters;q.ideal=ideal;q.count=q.base_count=count;q.incoming_best=-1;
 for(int i=0;i<count;i++)q.rating[i]=ratings[i];
 return q;
}
static void matched_seat_values(void) {
 int r[]={90,77,77,75,72,69},rank=0;PortalQuality q=room(1,1,6,6,r);
 assert(dri_quality_bonus_candidate(&q,8,84,82,&rank)==205 && rank==2);
 assert(dri_quality_bonus_candidate(&q,8,83,81,&rank)==190 && rank==2);
 q.rating[0]=99;
 assert(dri_quality_bonus_candidate(&q,8,84,82,&rank)==205 && rank==2);
 assert(dri_quality_bonus_candidate(&q,8,83,81,&rank)==190 && rank==2);
 q.count=q.base_count=5;
 assert(dri_quality_bonus_candidate(&q,8,84,82,&rank)==205 && rank==2);
 puts("PASS: Missouri-shaped backup gains are valued against HB2, independent of elite starter and full/vacant room");
}
static void useful_horizons(void) {
 int r[22];for(int i=0;i<22;i++)r[i]=99;
 const int one_starter[]={2,3,2,5,3,2,2,2,2,2,3,3,4,2,2,2,5,2,2,2,2,2};
 for(int pos=0;pos<22;pos++) {
  PortalQuality q=room(pos,1,10,10,r);
  assert(dri_portal_useful_depth(&q)==one_starter[pos]);
  q.ideal=1;assert(dri_portal_useful_depth(&q)==1);
  q.ideal=0;assert(dri_portal_useful_depth(&q)==0);
  assert(dri_quality_block_candidate(&q,8,99,99)==3);
 }
 PortalQuality q=room(3,3,10,10,r);assert(dri_portal_useful_depth(&q)==6);
 q.starters=4;assert(dri_portal_useful_depth(&q)==8);
 q=room(16,3,8,8,r);assert(dri_portal_useful_depth(&q)==6);
 q=room(12,2,6,6,r);assert(dri_portal_useful_depth(&q)==4);
 puts("PASS: all 22 positions have bounded scheme-aware horizons; zero-ideal positions stay closed");
}
static void third_back_and_limits(void) {
 int r[]={91,85,80,77,77,76,70,60,50},rank=0;PortalQuality q=room(1,1,6,6,r);
 assert(!dri_quality_block_candidate(&q,8,84,82));
 assert(dri_quality_bonus_candidate(&q,8,84,82,&rank)==110 && rank==3);
 assert(dri_quality_bonus_candidate(&q,8,83,81,&rank)==95 && rank==3);
 assert(dri_quality_block_candidate(&q,8,82,80)==1);
 assert(!dri_quality_block_candidate(&q,8,83,79));
 q.count=7;assert(dri_quality_block_candidate(&q,8,83,80)==4);
 q.count=8;assert(dri_quality_block_candidate(&q,8,99,99)==4);
 q.count=6;q.rating[2]=85;q.rating[3]=60;
 assert(dri_quality_block_candidate(&q,8,84,82)==1);
 puts("PASS: HB3 +3 qualifies, +2 and fourth-back-only upgrades do not; quality gaps need no raw-80 gate; normal overage limit holds");
}
static void all_position_bonus_and_monotonicity(void) {
 unsigned checks=0;
 for(int pos=0;pos<22;pos++)for(int starters=1;starters<=4;starters++) {
  if((pos==0 || pos==19 || pos==20) && starters!=1)continue;
  int r[22];for(int i=0;i<22;i++)r[i]=i<starters?99:60;
  PortalQuality q=room(pos,starters,12,12,r);int rank=0;
  int expected=dri_quality_floor[4][pos][starters]?400:0;
  assert(dri_quality_bonus_candidate(&q,8,80,80,&rank)==expected && rank==(expected?starters+1:0));
  for(int prestige=0;prestige<=10;prestige++)for(int count=0;count<=12;count++) {
   q.count=q.base_count=count;int previous=-1;
   for(int candidate=0;candidate<=99;candidate++) {
    if(dri_quality_block_candidate(&q,prestige,candidate,candidate))continue;
    int b=dri_quality_bonus_candidate(&q,prestige,candidate,candidate,&rank);
    assert(b>=previous);previous=b;checks++;
   }
  }
 }
 printf("PASS: matched-seat bonus across all positions; %u admissible quality comparisons remain monotone\n",checks);
}
static void vacancy_and_budget_priority(void) {
 PortalQuality rooms[22]={0};int hb[]={91,85,80,77,77,76},te[]={85};
 rooms[1]=room(1,1,6,6,hb);rooms[4]=room(4,1,2,1,te);
 PortalChoice a[2]={{0}};
 a[0]=(PortalChoice){.pos=1,.ovr=84,.raw_ovr=82,.native_base=5000,.rid=1};
 a[1]=(PortalChoice){.pos=4,.ovr=70,.raw_ovr=68,.native_base=1,.rid=2};
 int original[2];
 for(int i=0;i<2;i++){int rank=0;a[i].score=a[i].native_base+dri_quality_bonus_candidate(&rooms[a[i].pos],8,a[i].ovr,a[i].raw_ovr,&rank);original[i]=a[i].score;}
 dri_portal_order(a,2,1,rooms,8);
 assert(a[0].rid==2 && a[0].selected && !a[1].selected);
 for(int i=0;i<2;i++)assert(a[i].score==original[a[i].rid-1]);
 puts("PASS: real TE vacancy still precedes optional HB3 upgrade; planner preserves native input scores");
}
int main(void) {
 matched_seat_values();useful_horizons();third_back_and_limits();
 all_position_bonus_and_monotonicity();vacancy_and_budget_priority();
 return 0;
}
