/* R4 intentionally supersedes the full-position/shared-cap veto only for
   prestige-qualified useful depth. Use the actual policy and allocator. */
#define main prior_policy_main
#include "test29_portal_policy_test.c"
#undef main
int main(void) {
 PortalQuality q[22]={0};
 q[1]=room(1,7,6,1,94,70);q[1].rating[1]=90;q[1].rating[2]=84;
 q[1].incoming[0]=q[1].incoming[1]=1;q[1].incoming_best=94;q[1].base_count=5;
 q[1].exception_states=DRI_EXCEPTION_READY; /* all shared routes exhausted */
 assert(dri_portal_quality_depth_rank(&q[1],10,89)==3);
 assert(!dri_quality_block(&q[1],10,89));
 assert(!dri_portal_quality_depth_rank(&q[1],10,84));
 assert(dri_quality_block(&q[1],10,84)==4);
 q[1].hs_reserved=2;q[1].hs_count=1;
 assert(!dri_quality_block(&q[1],10,89));
 assert(q[1].hs_reserved==2 && q[1].hs_count==1);
 puts("PASS full HB room, existing portal arrivals, exhausted shared allowance and paid HS reservations do not veto a qualifying HB3; equal redundant HB4 stays closed");

 q[3]=room(3,13,12,3,96,70);
 int wr[]={96,92,89,84,80,78};for(int i=0;i<6;i++)q[3].rating[i]=wr[i];
 q[3].incoming[0]=q[3].incoming[3]=1;q[3].base_count=11;q[3].incoming_best=96;
 q[3].exception_states=DRI_EXCEPTION_READY;
 assert(dri_portal_quality_depth_rank(&q[3],10,85)==4);
 assert(!dri_quality_block(&q[3],10,85));
 assert(dri_quality_block(&q[3],10,78)==4);
 assert(!dri_quality_block(&q[3],10,79));
 int rank=0;assert(dri_quality_bonus(&q[3],10,85,&rank)>0 && rank>0);
 PortalChoice pool[3]={choice(1,3,85,500),choice(2,3,78,600),choice(3,1,89,450)};
 dri_portal_order(pool,3,50,q,10);
 int selected=0;for(int j=0;j<3;j++) {
  assert(pool[j].score==pool[j].native_base);
  if(pool[j].rid==2)assert(!pool[j].selected);
  else {assert(pool[j].selected && pool[j].round==1);selected++;}
 }
 assert(selected==2);
 puts("PASS already-overfull WR4 and HB3 receive actual-pool primary plans; redundant reserve is not selected; native scoring inputs conserved");

 PortalQuality high=room(3,13,12,3,70,68);
 high.base_count=12;high.incoming[0]=1;
 assert(dri_portal_quality_depth_rank(&high,10,86)==0); /* P25 WR1=87 */
 assert(dri_quality_block(&high,10,86)==4);
 assert(dri_portal_quality_depth_rank(&high,10,87)==1);
 assert(!dri_quality_block(&high,10,87));
 assert(dri_portal_quality_depth_rank(&high,1,86)==1);
 for(int i=0;i<7;i++)dri_portal_project_add(&high,87);
 assert(!dri_portal_quality_depth_rank(&high,10,87));
 assert(dri_quality_block(&high,10,87)==4);
 puts("PASS exact quarter-OVR prestige floors and complete current rotation stop indefinite equal-quality stacking");

 memset(q,0,sizeof(q));
 /* Two ordinary HS overages consume the general pool. Quality portal
    arrivals leave that consumption unchanged, including after reload. */
 q[5]=room(5,4,3,1,75,70);q[5].hs_count=1;
 q[6]=room(6,4,3,1,75,70);q[6].hs_count=1;
 q[3]=room(3,12,12,3,86,70);
 dri_portal_refresh_budget(q,10);uint32_t state=q[0].exception_states;
 assert(state==(DRI_EXCEPTION_READY|(1u<<12)));
 for(int v=88;v<=90;v++) {
  assert(!dri_quality_block(&q[3],10,v));
  dri_portal_project_add(&q[3],v);dri_portal_refresh_budget(q,10);
  assert(q[0].exception_states==state);
 }
 PortalQuality again[22];memcpy(again,q,sizeof(q));dri_portal_refresh_budget(again,10);
 assert(again[0].exception_states==state && again[3].count==15);
 puts("PASS quality permission is independent of 2+2+1 and reload cannot refund ordinary HS charges or hide actual headcount");

 PortalQuality qb=room(0,4,4,1,86,80);
 assert(!dri_portal_quality_depth_rank(&qb,10,99) && dri_quality_block(&qb,10,99)==2);
 for(int pos=19;pos<=21;pos++) {
  PortalQuality specialist=room(pos,3,2,1,75,70);
  assert(!dri_portal_quality_depth_rank(&specialist,10,99));
 }
 high.count=192;assert(dri_quality_block(&high,10,99)==4);
 selection();
 puts("PASS QB/specialist guards, bounded arrays and retained market selection rules");
 return 0;
}
