#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../portal_allocation.h"
static PortalQuality room(int pos,int retained,int ideal) {
 PortalQuality q={0};q.pos=pos;q.ideal=ideal;q.starters=1;q.count=q.base_count=1;
 q.incoming_best=-1;q.rating[0]=retained;return q;
}
static PortalChoice candidate(const PortalQuality *q,int prestige,int ovr,int base,unsigned rid) {
 int rank=0;return (PortalChoice){.pos=q->pos,.ovr=ovr,.raw_ovr=ovr,.native_base=base,.score=base>0 && !dri_quality_block(q,prestige,ovr)?base+dri_quality_bonus(q,prestige,ovr,&rank):0,.rid=rid};
}
int main(void) {
 int cases=0;
 /* The duplicate predicate must depend on the final room, not commitment order.
    Three slots remain, so neither order encounters a headcount limit. */
 int positions[]={0,19,20};
 for(int p=0;p<3;p++)for(int retained=60;retained<=95;retained++)
 for(int a=60;a<=99;a++)for(int b=60;b<=99;b++) {
  PortalQuality qa=room(positions[p],retained,4),qb=qa;
  /* Only compare signing orders when either first signing is legal. */
  if(dri_quality_block(&qa,9,a) || dri_quality_block(&qa,9,b))continue;
  dri_portal_project_add(&qa,a);dri_portal_project_add(&qb,b);
  int ab=dri_quality_block(&qa,9,b),ba=dri_quality_block(&qb,9,a);
  assert(ab==ba);
  int high=a>b?a:b,gap=a-b;if(gap<0)gap=-gap;
  assert(ab==(((gap<=3 && high>=retained-3) || (positions[p]==0 && a>=85 && b>=85))?2:0));cases++;
 }
 printf("PASS: %d QB/K/P final-room cases are symmetric across signing order\n",cases);
 PortalQuality q=room(0,76,4);q.count=q.base_count=2;q.rating[1]=65;
 dri_portal_project_add(&q,76);assert(!dri_quality_block(&q,9,81));
 dri_portal_project_add(&q,81);assert(q.count==4 && q.rating[0]==81 && q.rating[1]==76);
 assert(dri_quality_block(&q,9,83)==2);
 dri_portal_project_add(&q,65);assert(dri_quality_block(&q,9,95)==4);
 puts("PASS: SMU depth-first 76 then 81 stays open; close duplicate and unchanged cap still bind");
 q=room(20,76,2);dri_portal_project_add(&q,81);
 assert(dri_quality_block(&q,9,76)==1);
 puts("PASS: when a starter fills the only vacancy, a redundant lower reserve cannot force an overage");
 /* Monotone quality within every position and across backup-to-starter and
    starter-rank transitions, including matrix-adequate starters. */
 int monotone=0;
 for(int pos=0;pos<22;pos++)for(int prestige=0;prestige<10;prestige++)
 for(int starters=1;starters<=3;starters++) {
  q=room(pos,80,8);q.starters=starters;q.count=q.base_count=3;
  q.rating[1]=70;q.rating[2]=60;int previous=-1;
  for(int ovr=0;ovr<=99;ovr++) {
   int rank=0,bonus=dri_quality_bonus(&q,prestige,ovr,&rank);
   assert(bonus>=previous);previous=bonus;monotone++;
  }
 }
 printf("PASS: %d quality evaluations stay monotone across all 22 positions, five matrix bands and 1-3 starters\n",monotone);
 PortalQuality rooms[22]={0};rooms[20]=room(20,77,2);
 PortalChoice pool[3];pool[0]=candidate(&rooms[20],8,75,467,1);pool[1]=candidate(&rooms[20],8,82,517,2);
 assert(pool[1].score>pool[0].score);dri_portal_order(pool,2,1,rooms,8);
 assert(pool[0].rid==2 && pool[0].selected && pool[0].role==1);
 assert(pool[1].rid==1 && pool[1].round==2 && pool[1].slot==1);
 puts("PASS: empty backup slot does not outscore the better punter; the addition fills only one vacancy");
 memset(rooms,0,sizeof(rooms));rooms[16]=room(16,79,4);rooms[16].count=rooms[16].base_count=4;
 rooms[16].rating[1]=78;rooms[16].rating[2]=77;rooms[16].rating[3]=76;
 rooms[4]=room(4,95,2);
 pool[0]=candidate(&rooms[16],0,91,100,1);pool[1]=candidate(&rooms[4],0,70,1,2);
 dri_portal_order(pool,2,2,rooms,0);
 assert(pool[0].rid==2 && pool[0].selected && pool[1].rid==1 && !pool[1].selected);
 pool[0]=candidate(&rooms[16],0,99,0,3);assert(pool[0].score==0);
 puts("PASS: full adequate CB room stays closed while genuine TE vacancy is filled; native zero remains zero");
 return 0;
}
