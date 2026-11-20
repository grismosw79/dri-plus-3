#include <assert.h>
#include <stdio.h>
#include "../portal_allocation.h"
static PortalChoice candidate(PortalQuality *q,int ovr,int base,int id) {
 int rank=0;PortalChoice p={.pos=q->pos,.ovr=ovr,.raw_ovr=ovr-2,.native_base=base,.rid=(unsigned)id};
 p.score=base+dri_quality_bonus_candidate(q,4,ovr,ovr-2,&rank);return p;
}
int main(void) {
 /* A useful improvement at an already legal vacancy keeps its value above
    the soft floor. The same adequate full room remains closed. */
 int checked=0;
 for(int pos=1;pos<21;pos++) {
  PortalQuality q={.pos=pos,.ideal=6,.count=5,.base_count=5,.starters=1,.incoming_best=-1,.team_excess=9};
  for(int i=0;i<5;i++)q.rating[i]=80;
  int lo=0,hi=0;int a=dri_quality_bonus_candidate(&q,4,81,79,&lo),b=dri_quality_bonus_candidate(&q,4,89,87,&hi);
  assert(!dri_quality_block_candidate(&q,4,89,87));assert(b>a && hi>0);
  q.count=q.base_count=q.ideal;for(int i=0;i<6;i++)q.rating[i]=80;
  q.exception_states=DRI_EXCEPTION_READY;
  int blocked=dri_quality_block_candidate(&q,4,89,87);assert(blocked==1 || blocked==8);q.team_excess=0;q.exception_states=0;for(int i=0;i<6;i++)q.rating[i]=92;
  assert(dri_quality_block_candidate(&q,4,99,97)==1);checked++;
 }
 PortalQuality rooms[22]={0};PortalQuality *q=&rooms[18];
 *q=(PortalQuality){.pos=18,.ideal=4,.count=3,.base_count=3,.starters=1,.incoming_best=-1};
 q->rating[0]=77;q->rating[1]=70;q->rating[2]=68;
 PortalChoice p[2]={candidate(q,76,622,1),candidate(q,89,595,2)};
 dri_portal_order(p,2,2,rooms,4);assert(p[0].rid==2 && p[0].round==1);
 PortalChoice weak=p[0];weak.existing=weak.offered=weak.race_ok=1;weak.influence=20;weak.leader=100;
 int value=dri_portal_value(&p[0],q,4);assert(dri_portal_value(&weak,q,4)<value/2);
 weak.influence=75;assert(dri_portal_value(&weak,q,4)==value);
 weak.race_ok=0;assert(dri_portal_value(&weak,q,4)==0);
 PortalChoice fresh=p[0];fresh.offered=0;fresh.race_ok=1;fresh.commit_score=100;fresh.leader=50;fresh.influence=0;
 assert(dri_portal_value(&fresh,q,4)==value*100/150);
 fresh.influence=50;assert(dri_portal_value(&fresh,q,4)==value);
 assert(fresh.offered==0 && fresh.score==p[0].score);
 printf("PASS: %d non-QB positions value legal upgrades above soft floors without opening full rooms; useful 87 beats 74; losing races and seeded interest affect planning only\n",checked);
 return 0;
}
