/* Test14 planner regressions. These fixtures test selection outcomes and the
   original-budget invariant; they are not a season or commitment simulation. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../portal_allocation.h"

static void empty_rooms(PortalQuality rooms[22]) {
 memset(rooms,0,22*sizeof(*rooms));
 for(int p=0;p<22;p++){rooms[p].pos=p;rooms[p].incoming_best=-1;}
}
static void room_set(PortalQuality *q,int pos,int count,int ideal,int starters,const int *ratings) {
 memset(q,0,sizeof(*q));q->pos=pos;q->count=q->base_count=count;
 q->ideal=ideal;q->starters=starters;q->incoming_best=-1;
 for(int j=0;j<count;j++)q->rating[j]=ratings[j];
}
static PortalChoice choice(const PortalQuality *q,int prestige,int ovr,int base,unsigned id) {
 PortalChoice p={0};int rank=0;
 p.pos=q->pos;p.ovr=p.raw_ovr=ovr;p.rid=id;p.rec=id;p.index=(int)id;
 p.native_base=base;
 p.score=base>0 && !dri_quality_block(q,prestige,ovr)?base+dri_quality_bonus(q,prestige,ovr,&rank):0;
 p.plan_score=p.score;p.role=p.score?dri_portal_role(q,prestige,ovr):0;
 return p;
}
static void competitive(PortalChoice *p) {
 p->existing=1;p->offered=1;p->influence=89;p->leader=89;
 p->commit_score=120;p->stage=2;p->race_ok=1;
}
static PortalChoice *by_id(PortalChoice *p,int n,unsigned id) {
 for(int j=0;j<n;j++)if(p[j].rid==id)return &p[j];
 assert(!"Candidate disappeared from permutation");return 0;
}
static void unchanged_scores(PortalChoice *a,int n,const PortalChoice *before) {
 for(int j=0;j<n;j++) {
  const PortalChoice *old=0;
  for(int k=0;k<n;k++)if(before[k].rid==a[j].rid)old=&before[k];
  assert(old && old->score==a[j].score && old->native_base==a[j].native_base);
 }
}
static int selected_primaries(const PortalChoice *a,int n) {
 int k=0;for(int j=0;j<n;j++)k+=a[j].selected && a[j].round==1;return k;
}

static void dynamic_qb_room(void) {
 PortalQuality rooms[22],original[22];PortalChoice a[3],old[3];
 const int ratings[]={72,65};empty_rooms(rooms);room_set(&rooms[0],0,2,4,1,ratings);
 a[0]=choice(&rooms[0],9,90,800,1);
 a[1]=choice(&rooms[0],9,85,650,2);
 a[2]=choice(&rooms[0],9,76,680,3);competitive(&a[2]);
 assert(a[1].score>a[2].score); /* Static Test13 ordering favors the second star. */
 memcpy(old,a,sizeof(a));memcpy(original,rooms,sizeof(rooms));
 dri_portal_order(a,3,2,rooms,9);
 assert(by_id(a,3,1)->selected && by_id(a,3,3)->selected && !by_id(a,3,2)->selected);
 assert(by_id(a,3,1)->round==1 && by_id(a,3,3)->round==1);
 assert(by_id(a,3,3)->slot==2 && by_id(a,3,3)->role==2);
 /* The 76 QB improves the revised room's 72 backup by four: 100+4*15. */
 assert(by_id(a,3,3)->plan_score==840);
 unchanged_scores(a,3,old);assert(!memcmp(rooms,original,sizeof(rooms)));
 puts("PASS: a stronger QB primary and attainable lower backup coexist; stale starter bonus cannot select the second star");
}

static void protected_one_vacancy(void) {
 PortalQuality rooms[22],original[22];PortalChoice a[3],old[3];
 const int ratings[]={72,68,65};empty_rooms(rooms);room_set(&rooms[0],0,3,4,1,ratings);
 a[0]=choice(&rooms[0],9,90,800,10);
 a[1]=choice(&rooms[0],9,85,750,11);
 a[2]=choice(&rooms[0],9,76,500,12);competitive(&a[2]);
 memcpy(old,a,sizeof(a));memcpy(original,rooms,sizeof(rooms));
 dri_portal_order(a,3,2,rooms,9);
 assert(by_id(a,3,10)->selected && by_id(a,3,10)->round==1);
 assert(by_id(a,3,12)->selected && by_id(a,3,12)->round==2);
 assert(by_id(a,3,12)->slot==by_id(a,3,10)->slot);
 assert(!by_id(a,3,11)->selected && selected_primaries(a,3)==1);
 unchanged_scores(a,3,old);assert(!memcmp(rooms,original,sizeof(rooms)));
 puts("PASS: competitive progress keeps a one-vacancy fallback without displacing the better primary or inventing another opening");
}

static void vacancy_fallback_before_upgrade(void) {
 PortalQuality rooms[22];PortalChoice a[3];
 const int te[]={85},qb[]={84,80,70,65};empty_rooms(rooms);
 room_set(&rooms[4],4,1,2,1,te);room_set(&rooms[0],0,4,4,1,qb);
 a[0]=choice(&rooms[4],9,76,600,20);
 a[1]=choice(&rooms[4],9,74,500,21);
 a[2]=choice(&rooms[0],9,95,5000,22);
 dri_portal_order(a,3,2,rooms,9);
 assert(by_id(a,3,20)->selected && by_id(a,3,20)->round==1);
 assert(by_id(a,3,21)->selected && by_id(a,3,21)->round==2);
 assert(!by_id(a,3,22)->selected);
 puts("PASS: usable fallback for a genuine TE vacancy precedes an optional full-room QB upgrade regardless of its raw score");
}

static void protection_is_conditional(void) {
 PortalQuality rooms[22];const int ratings[]={72,68,65};
 for(int variant=0;variant<8;variant++) {
  PortalChoice a[3];empty_rooms(rooms);room_set(&rooms[0],0,3,4,1,ratings);
  a[0]=choice(&rooms[0],9,90,800,30);
  a[1]=choice(&rooms[0],9,85,750,31);
  a[2]=choice(&rooms[0],9,76,500,32);competitive(&a[2]);
  switch(variant) {
   case 0:a[2].offered=0;break;
   case 1:a[2].race_ok=0;break;
   case 2:a[2].influence=0;break;
   case 3:a[2].existing=0;break;
   case 4:a[2].influence=79;a[2].leader=100;break;
   case 5:a[2].influence=59;a[2].leader=59;break;
   case 6:a[2].stage=5;break;
   case 7:a[2].commit_score=0;break;
  }
  dri_portal_order(a,3,2,rooms,9);
  assert(by_id(a,3,30)->selected && by_id(a,3,31)->selected);
  assert(!by_id(a,3,32)->selected);
 }
 /* Exact boundaries are inclusive; no arbitrary tie behavior at the cutoff. */
 PortalChoice a[3];empty_rooms(rooms);room_set(&rooms[0],0,3,4,1,ratings);
 a[0]=choice(&rooms[0],9,90,800,30);a[1]=choice(&rooms[0],9,85,750,31);
 a[2]=choice(&rooms[0],9,76,500,32);competitive(&a[2]);
 a[2].influence=80;a[2].leader=100;a[2].commit_score=160;
 dri_portal_order(a,3,2,rooms,9);
 assert(by_id(a,3,32)->selected && by_id(a,3,32)->round==2);
 puts("PASS: protection requires an existing live offer, eligible unsecured race, meaningful progress and competitive influence; threshold boundaries are inclusive");
}

static void multiple_starters_repriced(void) {
 for(int p=3;p<=16;p+=13) {
  PortalQuality rooms[22],revised,original[22];PortalChoice a[3],old[3];
  const int ratings[]={80,70};empty_rooms(rooms);room_set(&rooms[p],p,2,4,2,ratings);
  a[0]=choice(&rooms[p],9,95,1000,40);
  a[1]=choice(&rooms[p],9,86,600,41);
  a[2]=choice(&rooms[p],9,79,500,42);
  memcpy(old,a,sizeof(a));memcpy(original,rooms,sizeof(rooms));
  revised=rooms[p];dri_portal_project_add(&revised,95);int rank=0;
  int expected=600+dri_quality_bonus(&revised,9,86,&rank);
  assert(expected<old[1].score && rank==2);
  dri_portal_order(a,3,2,rooms,9);
  PortalChoice *second=by_id(a,3,41);
  assert(by_id(a,3,40)->selected && second->selected && second->round==1);
  assert(second->role==1 && second->slot==2 && second->plan_score==expected);
  unchanged_scores(a,3,old);assert(!memcmp(rooms,original,sizeof(rooms)));
 }
 puts("PASS: WR and CB second-starter values use the revised multi-starter room while original funding scores stay unchanged");
}

static void alternatives_are_not_additions(void) {
 PortalQuality rooms[22],original[22];PortalChoice a[3],old[3];
 const int ratings[]={90,89};empty_rooms(rooms);room_set(&rooms[3],3,2,3,1,ratings);
 for(int j=0;j<3;j++)a[j]=choice(&rooms[3],9,75-j,600-j*50,50+j);
 memcpy(old,a,sizeof(a));memcpy(original,rooms,sizeof(rooms));
 dri_portal_order(a,3,3,rooms,9);
 assert(selected_primaries(a,3)==1);
 for(int j=0;j<3;j++) {
  PortalChoice *p=by_id(a,3,50+j);
  assert(p->selected && p->slot==1 && p->round==j+1 && p->plan_score==p->native_base);
 }
 unchanged_scores(a,3,old);assert(!memcmp(rooms,original,sizeof(rooms)));
 /* Once an actual acquisition fills that room, the same lower alternatives
    are redundant. Protection never bypasses this existing policy. */
 PortalQuality secured=rooms[3];dri_portal_project_add(&secured,75);
 assert(dri_quality_block(&secured,9,74)==1);
 puts("PASS: first and second fallbacks remain alternatives for one vacancy; a real signing can close that need");
}

static void optional_upgrade_preserves_fallback(void) {
 PortalQuality rooms[22];const int ratings[]={72,68,65};
 for(int cap=2;cap<=3;cap++) {
  PortalChoice a[4];empty_rooms(rooms);room_set(&rooms[0],0,3,4,1,ratings);
  a[0]=choice(&rooms[0],9,90,800,60);
  a[1]=choice(&rooms[0],9,76,500,61);competitive(&a[1]);
  a[2]=choice(&rooms[0],9,75,700,62);
  a[3]=choice(&rooms[0],9,83,600,63);
  dri_portal_order(a,4,cap,rooms,9);
  assert(by_id(a,4,60)->selected && by_id(a,4,61)->selected);
  assert(by_id(a,4,61)->round==2 && by_id(a,4,61)->slot==1);
  /* The 75 would be valid alongside the 90, but would invalidate the retained
     76's fallback race if the 90 went elsewhere. The 83 can coexist in either
     outcome. Later optional work must not break earlier vacancy coverage. */
  assert(!by_id(a,4,62)->selected);
  assert(by_id(a,4,63)->selected==(cap==3));
  PortalQuality alternative=rooms[0];
  /* The established QB planner keeps the 83 as a third choice for this
     very opening. It never occupies a second spot alongside the 76. */
  if(cap==3){assert(by_id(a,4,63)->round==3 && by_id(a,4,63)->slot==1);}
  assert(!dri_quality_block(&alternative,9,76));
  int rank=0,expected=500+dri_quality_bonus(&alternative,9,76,&rank);
  assert(by_id(a,4,61)->plan_score==expected);
 }
 puts("PASS: optional additions preserve a selected vacancy fallback; unselected optional primaries never change its final room or value");
}

static void all_position_coverage_and_caps(void) {
 PortalQuality rooms[22];PortalChoice a[66],old[66];int n=0;
 const int ratings[]={95,94};empty_rooms(rooms);
 for(int p=0;p<22;p++) {
  room_set(&rooms[p],p,2,3,1,ratings);
  for(int j=0;j<3;j++)a[n++]=choice(&rooms[p],9,75-j,9000-p*10-j,100+p*3+j);
 }
 memcpy(old,a,sizeof(a));dri_portal_order(a,n,22,rooms,9);int seen[22]={0};
 for(int j=0;j<n;j++)if(a[j].selected){assert(a[j].round==1 && a[j].slot==1);seen[a[j].pos]++;}
 for(int p=0;p<22;p++)assert(seen[p]==1);
 unchanged_scores(a,n,old);
 /* Original admission caps and near-equal QB guard remain binding. */
 const int qb[]={88,84,74,68};room_set(&rooms[0],0,4,4,1,qb);
 rooms[0].incoming[0]=1;rooms[0].incoming_best=88;
 assert(dri_quality_block(&rooms[0],9,90)==2);
 dri_portal_project_add(&rooms[0],95);
 assert(dri_quality_block(&rooms[0],9,99)==2); /* Existing elite QB blocks before headcount. */
 /* Zero board capacity does not create a selected fallback or primary. */
 memcpy(a,old,sizeof(a));dri_portal_order(a,n,0,rooms,9);
 for(int j=0;j<n;j++)assert(!a[j].selected);
 puts("PASS: all 22 positions retain vacancy coverage; zero capacity, duplicate guard and existing room cap remain effective");
}

int main(void) {
 dynamic_qb_room();protected_one_vacancy();vacancy_fallback_before_upgrade();
 protection_is_conditional();multiple_starters_repriced();alternatives_are_not_additions();
 optional_upgrade_preserves_fallback();
 all_position_coverage_and_caps();
 puts("PASS: Test14 focused market-planning regressions");return 0;
}
