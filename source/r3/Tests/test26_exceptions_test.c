#include <assert.h>
#include <stdio.h>
#include "../portal_allocation.h"
static int fit(const unsigned *m,int n,int g,int e,int r) {
 if(!n)return 1;
 return ((m[0]&1) && g && fit(m+1,n-1,g-1,e,r)) ||
        ((m[0]&2) && e && fit(m+1,n-1,g,e-1,r)) ||
        ((m[0]&4) && r && fit(m+1,n-1,g,e,r-1));
}
static void oracle(void) {
 unsigned masks[6];int cases=0;
 for(int n=0;n<=6;n++)for(unsigned code=0;code<(1u<<(2*n));code++) {
  unsigned x=code;uint32_t states=1;
  for(int i=0;i<n;i++){masks[i]=1u|((x&3u)<<1);x>>=2;states=dri_exception_step(states,masks[i]);}
  assert(!!states==fit(masks,n,2,2,1));cases++;
 }
 printf("PASS %d independent bucket-assignment cases, including overlap and sixth-addition rejection\n",cases);
}
static PortalQuality room(int pos,int count,int ideal,int best,int backup) {
 PortalQuality q={.pos=pos,.count=count,.base_count=count,.ideal=ideal,.starters=1,.incoming_best=-1};
 for(int i=0;i<count;i++)q.rating[i]=i?backup:best;
 return q;
}
static void budget(void) {
 PortalQuality q[22]={0};
 for(int p=0;p<22;p++)q[p]=room(p,0,0,0,0);
 q[5]=room(5,3,3,78,71);q[6]=room(6,3,3,79,73); /* Small gaps: general only. */
 q[7]=room(7,3,3,70,60);q[8]=room(8,3,3,68,60); /* Emergency starter. */
 q[1]=room(1,6,6,94,75); /* Adequate HB1, useful backup upgrade. */
 q[17]=room(17,7,4,90,85); /* Inherited overage is not a new addition. */
 q[9]=room(9,3,3,65,60);q[10]=room(10,1,4,60,50);
 dri_portal_refresh_budget(q,10);
 assert(dri_portal_exception_mask(&q[5],10,83)==1);
 assert(dri_portal_exception_mask(&q[7],10,75)==3);
 assert(dri_portal_exception_mask(&q[1],10,80)==5);
 assert(!(dri_portal_exception_mask(&q[7],10,74)&2));
 int positions[5]={5,6,7,8,1},ratings[5]={83,84,75,73,80};
 for(int i=0;i<5;i++) {
  int p=positions[i];assert(!dri_quality_block_candidate(&q[p],10,ratings[i],ratings[i]-2));
  dri_portal_project_add(&q[p],ratings[i]);dri_portal_refresh_budget(q,10);
  uint32_t state=q[0].exception_states;dri_portal_refresh_budget(q,10);assert(state==q[0].exception_states);
 }
 assert(q[0].exception_states==(DRI_EXCEPTION_READY|(1u<<17)));
 assert(dri_quality_block_candidate(&q[9],10,90,88)==8);
 assert(!dri_quality_block_candidate(&q[10],10,99,97)); /* Vacancy remains free. */
 dri_portal_project_add(&q[10],99);dri_portal_refresh_budget(q,10);
 assert(q[0].exception_states==(DRI_EXCEPTION_READY|(1u<<17)));
 assert(dri_quality_block_candidate(&q[1],10,90,88)==4); /* One extra per position. */
 puts("PASS 2 general + 2 emergency + 1 RB; inherited overages exempt; recount stable; sixth extra rejected; vacancies free");
}
static void emergency_positions(void) {
 for(int prestige=0;prestige<=10;prestige++)for(int pos=0;pos<22;pos++) {
  int floor=dri_quality_floor[dri_group(prestige)][pos][0];if(!floor)continue;
  int incumbent=floor/4-5;PortalQuality q=room(pos,4,4,incumbent,incumbent-5);
  assert(dri_portal_exception_mask(&q,prestige,incumbent+5)&2);
  assert(!(dri_portal_exception_mask(&q,prestige,incumbent+4)&2));
  q.rating[0]=(floor+3)/4-4;assert(!(dri_portal_exception_mask(&q,prestige,99)&2));
  q.incoming_best=90;assert(dri_portal_exception_mask(&q,prestige,99)==1);
 }
 PortalQuality qb=room(0,4,4,86,60);assert(dri_quality_block_candidate(&qb,10,90,88)==2);
 PortalQuality rb=room(1,7,6,94,70);assert(!(dri_portal_exception_mask(&rb,10,80)&4));
 puts("PASS emergency gaps across all rated positions/prestiges; five-point gain boundary; secured-role, elite QB and overfull RB protection");
}
int main(void){oracle();budget();emergency_positions();return 0;}
