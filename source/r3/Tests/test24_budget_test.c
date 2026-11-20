#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../portal_allocation.h"
static PortalQuality room(int pos,int ideal,int count,int rating) {
 PortalQuality q={0};q.pos=pos;q.ideal=ideal;q.starters=1;
 q.count=q.base_count=count;q.incoming_best=-1;
 for(int i=0;i<count;i++)q.rating[i]=rating;
 return q;
}
static void boundaries(void) {
 unsigned checks=0;
 for(int p=0;p<22;p++)for(int prestige=0;prestige<=10;prestige++) {
  PortalQuality q=room(p,4,3,60);q.team_excess=30;
  for(int c=1;c<=99;c++) {
   assert(!dri_quality_block_candidate(&q,prestige,c,c));checks++;
  }
  q.count=q.base_count=4;q.exception_states=DRI_EXCEPTION_READY;
  for(int c=1;c<=99;c++) {assert(dri_quality_block_candidate(&q,prestige,c,c));checks++;}
  q.team_excess=0;q.exception_states=0;
  for(int i=0;i<4;i++)q.rating[i]=99;
  for(int c=1;c<=99;c++){assert(dri_quality_block_candidate(&q,prestige,c,c));checks++;}
  q.ideal=0;assert(dri_quality_block_candidate(&q,prestige,99,99)==3);
 }
 printf("PASS %u vacancy / exhausted budget / adequate full-room checks across all positions and prestige\n",checks);
 /* Exact Test22 failure: 67 HS RT, prestige 1, ideal 3. No ceiling at 66. */
 PortalQuality q=room(9,3,1,67);q.team_excess=20;
 for(int c=68;c<=99;c++)assert(!dri_quality_block_candidate(&q,1,c,c-2));
 /* Useful depth remains valuable, independent of the quality of HB1. */
 q=room(1,6,6,69);q.rating[0]=94;q.rating[1]=78;q.rating[2]=77;
 int rank=0;assert(dri_quality_bonus_candidate(&q,8,83,81,&rank)==175 && rank==2);
 q.rating[0]=99;assert(dri_quality_bonus_candidate(&q,8,83,81,&rank)==175 && rank==2);
 q.rating[1]=88;assert(dri_quality_bonus_candidate(&q,8,83,81,&rank)==140 && rank==3);
 q.incoming[2]=1;assert(dri_quality_block_candidate(&q,8,83,81)==1);
 /* A filled adequate role no longer reopens for a better player. */
 q=room(6,3,3,74);q.rating[0]=79;
 assert(!dri_quality_block_candidate(&q,10,84,82));
 dri_portal_project_add(&q,84);
 assert(dri_quality_block_candidate(&q,10,99,97));
}
static void useful_roles(void) {
 for(int pos=0;pos<21;pos++) {
  PortalQuality q=room(pos,10,10,pos==0?84:99);
  int seat=dri_portal_useful_depth(&q)-1;
  int floor=dri_quality_floor[4][pos][seat];if(!floor)continue;
  q.rating[seat]=(floor+3)/4-5;
  int candidate=(floor+3)/4+1,rank=0;
  assert(!dri_quality_block_candidate(&q,10,candidate,candidate-2));
  assert(dri_quality_bonus_candidate(&q,10,candidate,candidate-2,&rank)>0 && rank==seat+1);
  q.incoming[seat]=1;assert(dri_quality_block_candidate(&q,10,candidate,candidate-2));
 }
 puts("PASS meaningful backup/rotation gaps across all applicable positions and secured-role closure");
}
static void global_plans(void) {
 for(int existing=0;existing<4;existing++)for(int cap=1;cap<=16;cap++) {
  PortalQuality q[22]={0};PortalChoice a[16]={0};
  for(int p=0;p<22;p++)q[p]=room(p,0,0,0);
  for(int p=5;p<13;p++)q[p]=room(p,3,3,60);
  q[18]=room(18,4,4+existing,90);
  dri_portal_set_flex(q);assert(q[5].team_excess==existing);
  for(int i=0;i<16;i++) {
   a[i].rid=i+1;a[i].rec=i+1;a[i].pos=5+i/2;a[i].ovr=85-i%2;a[i].raw_ovr=a[i].ovr-2;
   a[i].native_base=600-i;a[i].score=600-i;a[i].influence=12;a[i].leader=100;a[i].commit_score=160;a[i].race_ok=1;
  }
  dri_portal_order(a,16,cap,q,10);
  int primaries=0,selected=0;PortalQuality final[22];memcpy(final,q,sizeof(q));
  for(int i=0;i<16;i++) {
   assert(a[i].native_base==601-(int)a[i].rid && a[i].score==a[i].native_base);
   assert(a[i].influence==12 && a[i].commit_score==160);
   if(a[i].selected){selected++;if(a[i].round==1){primaries++;dri_portal_project_add(&final[a[i].pos],a[i].ovr);}}
  }
  assert(selected<=cap);assert(primaries<=4);
  if(cap>=4)assert(primaries==4);
  dri_portal_set_flex(final);assert(final[5].team_excess==existing+primaries);
  for(int i=0;i<16;i++)if(a[i].selected) {
   PortalQuality alternative[22];memcpy(alternative,q,sizeof(q));
   for(int j=0;j<16;j++)if(a[j].selected && a[j].round==1 &&
     !(a[j].pos==a[i].pos && a[j].slot==a[i].slot))
      dri_portal_project_add(&alternative[a[j].pos],a[j].ovr);
   dri_portal_refresh_budget(alternative,10);
   assert(!dri_quality_block_candidate(&alternative[a[i].pos],10,a[i].ovr,a[i].raw_ovr));
  }
 }
 puts("PASS team-wide primary/fallback compatibility, 64 capacity/surplus combinations, native scores and race inputs unchanged");
}
static void shared_flex(void) {
 PortalQuality q[22];for(int p=0;p<22;p++)q[p]=room(p,4,4,90);
 q[21]=room(21,0,0,0);q[2]=room(2,3,3,90); /* 83 configured places */
 assert(dri_portal_set_flex(q)==2 && q[1].flex_allowed);
 dri_portal_project_add(&q[5],91);dri_portal_set_flex(q);
 assert(q[1].team_excess==1 && q[1].flex_allowed);
 dri_portal_project_add(&q[7],91);dri_portal_set_flex(q);
 assert(q[1].team_excess==2 && !q[1].flex_allowed);
 q[9].count=1;dri_portal_set_flex(q);
 assert(!dri_quality_block_candidate(&q[9],1,95,93));
 puts("PASS flex and optional upgrades share one budget; shortages stay open after exhaustion");
}
int main(void){boundaries();useful_roles();global_plans();shared_flex();return 0;}
