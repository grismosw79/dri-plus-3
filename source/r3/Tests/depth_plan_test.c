#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../portal_allocation.h"
static PortalChoice choice(int pos,int ovr,int score,unsigned id) {
 return (PortalChoice){.pos=pos,.role=3,.ovr=ovr,.raw_ovr=ovr,.score=score,.native_base=score,.rid=id,.rec=id,.index=(int)id};
}
static void setup(PortalQuality *q,int pos,int count,int ideal,int starters) {
 memset(q,0,sizeof(*q));q->pos=pos;q->count=q->base_count=count;q->ideal=ideal;q->starters=starters;q->incoming_best=-1;
 for(int j=0;j<count;j++)q->rating[j]=95-j;
}
int main(void) {
 PortalQuality q[22]={0};PortalChoice a[132];int n=0;
 /* Every native position, including scheme-enabled FB and LS. Each room has
    exactly one real vacancy. Huge second-choice scores cannot consume coverage. */
 for(int p=0;p<22;p++) {
  setup(&q[p],p,2,3,1);
  for(int j=0;j<3;j++)a[n++]=choice(p,75-j,9000-p*10-j,100+p*3+j);
 }
 dri_portal_order(a,n,22,q,9);
 int seen[22]={0};
 for(int j=0;j<22;j++){assert(a[j].selected && a[j].round==1 && a[j].slot==1);seen[a[j].pos]++;}
 for(int p=0;p<22;p++)assert(seen[p]==1);
 for(int j=22;j<n;j++)assert(!a[j].selected && a[j].round>=2);
 puts("PASS: all 22 positions receive one distinct vacancy pursuit before alternatives");
 /* QB 1/4: starter, usable backup, and lower depth simultaneously; three strong
    competing starters are not counted as three additions. */
 memset(q,0,sizeof(q));setup(&q[0],0,1,4,1);q[0].rating[0]=62;
 a[0]=choice(0,88,1000,1);a[1]=choice(0,87,990,2);a[2]=choice(0,86,980,3);a[3]=choice(0,78,800,4);a[4]=choice(0,70,700,5);a[5]=choice(0,69,650,6);
 dri_portal_order(a,6,3,q,9);
 assert(a[0].rid==1 && a[1].rid==4 && a[2].rid==5);
 assert(a[0].slot==1 && a[1].slot==2 && a[2].slot==3);
 assert(a[0].role==1 && a[1].role==2 && a[2].role==3);
 puts("PASS: starter, backup and depth coexist with separated QB roles");
 /* A recruit who improves the starter also fills a vacancy; one missing
    player with two deficient ranks needs at most two additions, not three. */
 PortalOpening needs[682];setup(&q[0],0,3,4,1);q[0].rating[0]=60;q[0].rating[1]=59;q[0].rating[2]=58;
 int nn=dri_portal_openings(q,9,needs);assert(nn==2);
 puts("PASS: quality upgrades and headcount vacancies are not double-counted");
 /* Coastal-shaped room: quality adequate but a genuine missing QB is real work. */
 setup(&q[0],0,3,4,1);q[0].rating[0]=71;q[0].rating[1]=70;q[0].rating[2]=69;
 a[0]=choice(0,69,30,1);dri_portal_order(a,1,1,q,0);assert(a[0].round==1 && a[0].selected);
 puts("PASS: adequate-quality Coastal-shaped QB room still gets vacancy coverage");
 /* Full room optional upgrade waits for another position's true missing player. */
 memset(q,0,sizeof(q));setup(&q[0],0,4,4,1);setup(&q[4],4,1,2,1);
 a[0]=choice(0,99,9999,1);a[1]=choice(4,75,10,2);dri_portal_order(a,2,1,q,9);assert(a[0].rid==2);
 puts("PASS: real TE vacancy precedes optional upgrade in a full adequate QB room");
 /* No candidates or no space is valid; do not force a signing or increase cap. */
 dri_portal_order(a,0,0,q,9);dri_portal_order(a,2,0,q,9);assert(!a[0].selected && !a[1].selected);
 puts("PASS: exhausted market and zero capacity remain valid");
 return 0;
}
