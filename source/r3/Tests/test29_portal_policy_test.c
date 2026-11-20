#include <assert.h>
#include <stdio.h>
#include "../portal_allocation.h"
static PortalQuality room(int pos,int count,int ideal,int starters,int first,int other) {
 PortalQuality q={.pos=pos,.count=count,.base_count=count,.ideal=ideal,.starters=starters,.incoming_best=-1};
 for(int i=0;i<count;i++)q.rating[i]=i?other:first;
 return q;
}
static PortalChoice choice(unsigned id,int pos,int ovr,int score) {
 return (PortalChoice){.rid=id,.pos=pos,.ovr=ovr,.raw_ovr=ovr-2,.score=score,.native_base=score,.race_ok=1,.commit_score=140};
}
static void material(void) {
 /* R14 Auburn/Luke-shaped projected RT: 78+adequate room, recruit83+2.
    Boise State/Luke: projected best75; both had native-positive R14 scores. */
 for(int incumbent=75;incumbent<=78;incumbent+=3) {
  PortalQuality q=room(9,3,3,1,incumbent,incumbent);
  assert(dri_portal_upgrade_minimum(&q,7,0)==incumbent+5);
  assert(!dri_portal_seat_upgrade(&q,7,incumbent+4,0,0));
  assert(dri_portal_seat_upgrade(&q,7,incumbent+5,0,0));
  assert(!dri_quality_block_candidate(&q,7,85,83));
  q.incoming[0]=q.incoming[1]=q.incoming[2]=1;q.incoming_best=incumbent;
  assert(dri_quality_block_candidate(&q,7,99,97)==1);
 }
 PortalQuality rotation=room(3,10,10,3,95,90);
 rotation.rating[1]=rotation.rating[2]=95;
 assert(dri_portal_upgrade_minimum(&rotation,7,3)==94);
 assert(!dri_portal_seat_upgrade(&rotation,7,93,91,3));
 assert(dri_portal_seat_upgrade(&rotation,7,94,92,3));
 rotation.incoming[3]=1;assert(!dri_portal_seat_upgrade(&rotation,7,99,97,3));
 PortalQuality qb=room(0,4,4,1,86,80);assert(dri_quality_block_candidate(&qb,7,95,93)==2);
 qb=room(0,4,4,1,83,76);assert(dri_quality_block_candidate(&qb,5,85,85)==1);
 PortalQuality wr=room(3,10,10,3,91,79);wr.rating[1]=86;wr.rating[2]=81;
 assert(dri_quality_block_candidate(&wr,5,84,84)==1);
 puts("PASS material starter+5 and rotation+4 boundaries, same projected horizon, secured arrivals and QB protections");
}
static void capacity(void) {
 PortalQuality q[22]={0};
 q[5]=room(5,3,3,1,82,82);q[6]=room(6,3,3,1,82,82);q[9]=room(9,3,3,1,82,82);
 dri_portal_refresh_budget(q,7);
 for(int p=5;p<=6;p++) {
  assert(!dri_quality_block_candidate(&q[p],7,87,85));
  dri_portal_project_add(&q[p],87);dri_portal_refresh_budget(q,7);
  assert(dri_quality_block_candidate(&q[p],7,99,97)==4);
 }
 assert(dri_quality_block_candidate(&q[9],7,87,85)==8);
 uint32_t state=q[0].exception_states;dri_portal_refresh_budget(q,7);assert(q[0].exception_states==state);
 PortalChoice a[3]={choice(3202,5,87,480),choice(4783,6,87,450),choice(3817,9,87,400)};
 q[5]=room(5,3,3,1,82,82);q[6]=room(6,3,3,1,82,82);q[9]=room(9,3,3,1,82,82);
 dri_portal_order(a,3,50,q,7);int primaries=0;
 for(int i=0;i<3;i++){assert(a[i].score==a[i].native_base);primaries+=a[i].selected && a[i].round==1;}
 assert(primaries==2);
 puts("PASS optional actual-pool openings, two general extras, one-per-position cap and stable restart reconstruction");
}
static void hs_reservation(void) {
 PortalQuality q[22]={0};int reserved[22]={0};q[9]=room(9,2,3,1,78,73);reserved[9]=1;
 dri_portal_set_hs_reservations(q,reserved,7);
 assert(q[9].count==2 && q[9].hs_reserved==1);
 assert(!dri_portal_seat_upgrade(&q[9],7,85,83,2));
 assert(dri_quality_block_candidate(&q[9],7,85,83)==18);
 PortalChoice p=choice(3202,9,85,480);dri_portal_order(&p,1,50,q,7);assert(!p.selected);
 reserved[9]=0;dri_portal_set_hs_reservations(q,reserved,7);assert(!dri_quality_block_candidate(&q[9],7,85,83));
 puts("PASS paid HS last vacancy protected even against material portal upgrade until HS resolves; actual count unchanged");
}
static void selection(void) {
 PortalQuality q[22]={0};q[9]=room(9,2,3,1,75,70);
 PortalChoice p[2]={choice(1,9,85,500),choice(2,9,83,450)};
 p[0].crowd_penalty=45;p[0].crowd_defer=1;
 dri_portal_order(p,2,50,q,7);assert(p[0].rid==2 && p[0].selected && p[0].round==1);
 for(int i=0;i<2;i++)assert(p[i].score==p[i].native_base);
 p[0]=choice(1,9,85,500);p[0].crowd_defer=1;p[1]=choice(2,9,79,450);
 assert(!dri_portal_crowd_defer(&p[0],p,2,&q[9],7,0,0,1));
 p[1].ovr=83;p[1].raw_ovr=81;assert(dri_portal_crowd_defer(&p[0],p,2,&q[9],7,0,0,1));
 p[1].priority=1;p[1].round=1;p[1].slot=1;
 assert(dri_portal_crowd_defer(&p[0],p,2,&q[9],7,0,0,1)); /* Close primary still redirects a redundant hot backup. */
 p[1].slot=2;assert(!dri_portal_crowd_defer(&p[0],p,2,&q[9],7,0,0,1));
 p[1].slot=1;
 p[0].crowd_admitted=1;assert(!dri_portal_crowd_defer(&p[0],p,2,&q[9],7,0,0,1));
 p[0]=choice(1,9,85,500);p[1]=choice(3202,9,83,400);p[1].coverage_priority=1;
 dri_portal_order(p,2,50,q,7);assert(p[0].rid==3202 && p[0].round==1);
 p[0]=choice(1,9,85,500);p[0].existing=p[0].offered=1;p[0].influence=p[0].leader=100;
 p[1]=choice(3202,9,83,400);p[1].coverage_priority=1;
 dri_portal_order(p,2,50,q,7);assert(p[0].rid==1 && p[0].round==1);
 puts("PASS close-alternative deferral, frozen admitted protection, early coverage and earned competitive race protection");
}
int main(void){material();capacity();hs_reservation();selection();return 0;}
