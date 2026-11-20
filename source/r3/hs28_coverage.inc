/* Test28R9: observed execution and coordinated elite-market alternatives.
   No offers, hours, points, or capacity are written. The native board and
   interaction passes execute every reserved pursuit; their outcome is audited. */
#include "hs28_coverage.h"
static int hs29_offer_state(Rec recruit,Rec team,U eval);
static int hs30_firm_count(Rec recruit,U eval);
static int hs30_offer_leader(Rec recruit,U eval);
static int hs30_catchup_to(const Hs28Choice *choice,int leader);
#define HS28_MARKET_LIMIT 12000
#define HS28_MARKET_HASH 16384
typedef struct {
 U table,player_table;uint32_t rid,pid;int stars,pos,owned;
 uint64_t offered[3],funded[3],planned[3],firm[3],conditional[3],ever_funded[3];
 unsigned candidates,legal,viable,selected,trace_passes;
 int best_team,best_score;
} Hs28MarketRecruit;
typedef struct {U table;uint32_t rid;int slot,role,cost,new_board,observed;} Hs28MarketReservation;
typedef struct {
 U table,handle;uint32_t row,year,week,type;int valid,ready,count;
 Hs28MarketRecruit recruits[HS28_MARKET_LIMIT];int hash[HS28_MARKET_HASH];
 Hs28MarketReservation recovery[138][50];int recovery_count[138];
 unsigned team_seen[138];
} Hs28Market;
static Hs28Market hs28_market;
typedef struct {int valid,n,count;unsigned total,assigned,processed;Hs28Choice plan[MAX_BOARD];} Hs28RecoveryTeam;
static Hs28RecoveryTeam hs28_recovery_teams[138];
static void hs28_coverage_seed(U eval);
static void test29r2_class_seed(U eval);
static void test34_class_market_reset(int previous_valid);
static int hs28_recovery_cost(U eval,int offered);
static int hs28_recovery_destination(Rec recruit,Rec team,U eval,const Hs15Snapshot *shot,int *score,int *slot,int *role);
static int hs28_recovery_reserve(Rec team,Rec recruit,U eval,int slot,int role);

static void hs31_release_unexecuted(U table,uint32_t rid,unsigned tid);
static void hs31_retry_deferred_apply(void);
static void hs31_retry_deferred(U table,uint32_t rid,unsigned tid,int funded);
static int hs28_market_scope(U eval,int reset) {
 Rec f=context_ref(eval,6),s=f?reference(f,44):0;
 if(!f || !s){release(s);release(f);return 0;}
 U table=q((U)f+0x10);uint32_t rid=row(f),year=scalar(s,2,0x100),week=scalar(s,4,0x100),type=scalar(s,5,0x108);
 int match=hs28_market.valid && hs28_market.table==table && hs28_market.handle==(U)f &&
  hs28_market.row==rid && hs28_market.year==year && hs28_market.week==week && hs28_market.type==type;
 if(!match && reset) {
  hs34_trace_end();test34_class_market_reset(hs28_market.valid);
  hs31_retry_deferred_apply();
  for(unsigned tid=0;tid<138;tid++)for(int i=0;i<hs28_market.recovery_count[tid];i++) {
   Hs28MarketReservation *r=&hs28_market.recovery[tid][i];
   if(!r->observed){hs31_release_unexecuted(r->table,r->rid,tid);
    log_line("HS31_RESERVATION_RETRY,%u,%u,UNEXECUTED_SCOPE_EXPIRED_REVALIDATE",tid,r->rid);}
  }
  memset(&hs28_market,0,sizeof(hs28_market));memset(hs28_recovery_teams,0,sizeof(hs28_recovery_teams));hs28_market.table=table;hs28_market.handle=(U)f;
  hs28_market.row=rid;hs28_market.year=year;hs28_market.week=week;hs28_market.type=type;
 }
 release(s);release(f);return match;
}
static Hs28MarketRecruit *hs28_market_find(U table,uint32_t rid) {
 unsigned h=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS28_MARKET_HASH-1);
 for(unsigned n=0;n<HS28_MARKET_HASH;n++,h=(h+1)&(HS28_MARKET_HASH-1)) {
  int i=hs28_market.hash[h];if(!i)return 0;
  Hs28MarketRecruit *m=&hs28_market.recruits[i-1];if(m->table==table && m->rid==rid)return m;
 }
 return 0;
}
static int hs28_recovery_pending(Rec recruit,U eval) {
 if(!hs28_market_scope(eval,0))return 0;
 U table=q((U)recruit+0x10);uint32_t rid=row(recruit);
 for(unsigned tid=0;tid<138;tid++)for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];if(r->table==table && r->rid==rid)return 1;
 }
 return 0;
}
static int hs28_coverage_recovered(Rec recruit,Rec team,U eval) {
 if(!hs28_market_scope(eval,0))return 0;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];
  if(r->table==q((U)recruit+0x10) && r->rid==row(recruit))return 1;
 }
 return 0;
}
static int hs28_coverage_prior(Rec team,U eval,Hs28Prior *old,int no) {
 if(!hs28_market_scope(eval,0) || no<0 || no>50)return no;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return no;
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];int found=-1,duplicate=-1;
  for(int j=0;j<no;j++) {
   if(old[j].table==r->table && old[j].rid==r->rid){duplicate=j;}
   if(old[j].slot==r->slot && old[j].choice==r->role)found=j;
  }
  if(duplicate>=0){
   if(old[duplicate].choice==r->role && old[duplicate].slot==r->slot)continue;
   /* A backup can become the verified new primary; remove its old identity
      before inserting it so a recruit never occupies both roles. */
   old[duplicate].closed=1;
  }
  /* Reservation was checked against the current board plan. A surviving old
     primary is protected; only a closed stale entry can be overwritten here. */
  if(found>=0 && !old[found].closed){
   if(r->role!=1)continue;
   Hs28Prior displaced=old[found];int backup=-1;
   for(int j=0;j<no;j++)if(old[j].slot==r->slot && old[j].choice==2){backup=j;break;}
   if(backup<0 && no<50)backup=no++;
   if(backup>=0){displaced.choice=2;old[backup]=displaced;}
  }
  if(found<0){if(no>=50)continue;found=no++;}
  old[found]=(Hs28Prior){.table=r->table,.rid=r->rid,.pos=r->slot/32,.slot=r->slot,
   .choice=r->role,.order=found+1,.reserved=1};
 }
 return no;
}
static void hs28_market_bit(uint64_t bits[3],unsigned tid,int value) {
 uint64_t mask=(uint64_t)1<<(tid%64);if(value)bits[tid/64]|=mask;else bits[tid/64]&=~mask;
}
static int hs28_market_has(const uint64_t bits[3],unsigned tid) {return (int)((bits[tid/64]>>(tid%64))&1);}
static int hs28_market_count(const Hs28MarketRecruit *m,int kind,int exclude) {
 int count=0;
 for(unsigned t=0;t<138;t++)if((int)t!=exclude) {
  int actual=hs28_market_has(m->offered,t) && hs28_market_has(m->funded,t);
  count+=kind==0?actual:kind==1?hs28_market_has(m->planned,t):actual || hs28_market_has(m->planned,t);
 }
 return count;
}
static void hs28_recovery_cache_invalidate(void){hs31_native_score_invalidate(0);memset(hs28_recovery_teams,0,sizeof(hs28_recovery_teams));}
/* Flags are set by the native funded-action path only after affordability and
   board-hour assignment. Scholarship/scouting spend alone is not pursuit. */
static unsigned hs28_productive_allocated(Rec target,U eval,unsigned *mask) {
 *mask=0;
 /* Test29R1: retained pitches on acquired recruits are history, not current
    recruiting demand. Never clear native records to make accounting fit. */
 Rec recruit=target?reference(target,10):0;
 if(!recruit)return 0;
 unsigned stage=scalar(recruit,9,0x108);
 Rec player=reference(recruit,5);int owner=-1;
 int acquired=stage>=5 || (player && portal_signed_destination(recruit,player,&owner));
 release(player);release(recruit);if(acquired)return 0;
 Rec tuning=(Rec)q(eval+0xd0),actions=tuning?array_field(tuning,49):0;
 if(!actions)return 0;
 U n=array_count(actions);if(n>64){release(actions);return 0;}unsigned hours=0,seen=0;
 for(U i=0;i<n;i++) {
  Rec a=array_at(actions,i);if(!a)continue;
  unsigned type=scalar(a,0,0x108),cost=scalar(a,2,0x100),gain=scalar(a,1,0x100);
  if(type>=5 && type<=9 && !(seen&(1u<<type)) && scalar(a,5,0xf8) && !scalar(a,6,0xf8) &&
    cost>0 && cost<=1000 && gain>0 && scalar(target,(int)type+10,0xf8)) {
   seen|=1u<<type;*mask|=1u<<(type-5);hours+=cost;
  }
  release(a);
 }
 Rec pitches=array_field(target,0);
 if(pitches && array_count(pitches)<=16)for(U j=0;j<array_count(pitches);j++) {
  Rec pitch=array_at(pitches,j);if(!pitch)continue;
  unsigned intensity=scalar(pitch,0,0x108);release(pitch);
  for(U i=0;i<n;i++) {
   Rec a=array_at(actions,i);if(!a)continue;
   unsigned type=scalar(a,0,0x108),cost=scalar(a,2,0x100),gain=scalar(a,1,0x100);
   int match=type==2 && scalar(a,4,0x108)==intensity && scalar(a,5,0xf8) && !scalar(a,6,0xf8) && gain>0 && cost>0 && cost<=1000;
   release(a);if(match){hours+=cost;*mask|=32u;break;}
  }
 }
 release(pitches);release(actions);return hours;
}
/* Capture completed native funding assignment before weekly action execution.
   Allocated productive hours and executed hours/influence remain distinct. */
static void hs28_coverage_observe(Rec team,U eval,const char *site) {
 test29_class_invalidate(team);
 if(mode!=1 || faulted || !hs28_market_scope(eval,0))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 hs28_recovery_teams[tid].valid=0;
 Rec user=reference(team,407);if(user){release(user);return;}
 Hs15Snapshot shot;Hs28Choice plan[MAX_BOARD];int n=0;
 if(!hs15_collect(&shot,team,eval) || !hs28_make_board_plan(team,eval,&shot,plan,&n,0)) {
  log_line("HS28_COVERAGE_SKIP,%s,%u,PLAN_UNAVAILABLE",site,tid);return;
 }
 int post=strncmp(site,"POST_FUNDING",12)==0;
 for(int i=0;i<hs28_market.count;i++) {
  Hs28MarketRecruit *m=&hs28_market.recruits[i];hs28_market_bit(m->offered,tid,0);hs28_market_bit(m->firm,tid,0);hs28_market_bit(m->conditional,tid,0);
  if(post)hs28_market_bit(m->planned,tid,0);
  for(int j=0;j<shot.used;j++)if(shot.players[j].published && shot.players[j].table==m->player_table && shot.players[j].rid==m->pid)m->owned=1;
 }
 Rec budget=reference(team,224);unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
 unsigned productive_total=0;
 Test32BattleRow battle_rows[MAX_BOARD];int battle_count=0;
 Test33HsExecutionRow execution_rows[MAX_BOARD];int execution_count=0;
 int materialized[50]={0},offered[50]={0},funded_reservation[50]={0},secured[50]={0};
 Rec board=board_array(team);if(!board)return;
 U count=array_count(board);if(count>MAX_BOARD){release(board);log_line("HS28_COVERAGE_SKIP,%s,%u,BOARD_BOUNDS",site,tid);return;}
 int complete=1;
 for(U i=0;i<count;i++) {
  Rec target=array_at(board,i),rec=target?reference(target,10):0;
  if(!target || !rec){release(rec);release(target);complete=0;continue;}
  U table=q((U)rec+0x10);uint32_t rid=row(rec);
  Hs28MarketRecruit *m=hs28_market_find(table,rid);
  int reservation=-1;
  for(int j=0;j<hs28_market.recovery_count[tid];j++)if(hs28_market.recovery[tid][j].table==table && hs28_market.recovery[tid][j].rid==rid){reservation=j;break;}
  unsigned stage=scalar(rec,9,0x108);
  int acquired=stage>=5;
  if(reservation>=0) {
   int owner=-1,published=portal_published_owner(rec,eval,&owner);
   if(published<0){complete=0;release(rec);release(target);continue;}
   acquired|=published>0;materialized[reservation]=1;secured[reservation]=acquired;
  }
  if(m && acquired)m->owned=1;
  if(m && !m->owned) {
   for(int j=0;j<shot.used;j++)if(shot.players[j].published && shot.players[j].table==m->player_table && shot.players[j].rid==m->pid)m->owned=1;
  }
  if(hs15_is_recruit(rec) && !acquired && stage<5) {
   int k=-1;for(int j=0;j<n;j++)if(hs28_identity(&plan[j],rec)){k=j;break;}
   unsigned status=scalar(target,12,0x108),hours=scalar(target,6,0x100);int32_t delta=(int32_t)scalar(target,7,0x100);
   unsigned actions=0,allocated=test31_productive_allocated(target,team,eval,&actions);productive_total+=allocated;
   int valid=k>=0 && plan[k].selected && plan[k].legal && plan[k].viable && status>=2 && status<=4;
   int funded=valid && allocated>0 && assigned>=allocated && assigned<=total;
   if(k>=0 && execution_count<MAX_BOARD)execution_rows[execution_count++]=test33_execution_row(&plan[k],rid,
    status,allocated,actions,scalar(target,8,0x100),delta,assigned,total,
    plan[k].selected && plan[k].legal && plan[k].viable);
   if(reservation>=0){offered[reservation]=valid;funded_reservation[reservation]=funded;}
   if(m && !m->owned) {
    int state=valid?hs29_offer_state(rec,team,eval):0;
    if(state<0){complete=0;state=0;}
    hs28_market_bit(m->firm,tid,state==2);
    hs28_market_bit(m->conditional,tid,state==1);
    valid=valid && state>0;funded=funded && valid;
    hs28_market_bit(m->offered,tid,valid);
    if(strncmp(site,"BEGIN",5)==0 && valid)hs28_market_bit(m->planned,tid,1);
    if(post && funded){hs28_market_bit(m->ever_funded,tid,1);hs28_market_bit(m->funded,tid,1);hs28_market_bit(m->planned,tid,1);}
    if(k>=0 && battle_count<MAX_BOARD)battle_rows[battle_count++]=test32_battle_row(&plan[k],m->rid,
     status,allocated,actions,scalar(target,8,0x100),state,delta,assigned,total,
     valid && hs28_market_has(m->funded,tid));
    log_line("HS28_COVERAGE_EXECUTION,%s,%u,%u,%d,%u,%u,%u,%u,%d,%d,%d",site,tid,m->rid,
     k>=0 && plan[k].selected?plan[k].weight:0,status,hours,allocated,actions,delta,valid,
     valid && hs28_market_has(m->funded,tid));
   }
  }
  release(rec);release(target);
 }
 release(board);
 int budget_ok=productive_total<=assigned && assigned<=total;
 if(post && !budget_ok) {
  for(int i=0;i<hs28_market.count;i++){hs28_market_bit(hs28_market.recruits[i].funded,tid,0);hs28_market_bit(hs28_market.recruits[i].planned,tid,0);}
  log_line("HS28_COVERAGE_SKIP,%s,%u,ASSIGNED_BUDGET_MISMATCH,%u,%u",site,tid,productive_total,assigned);
 }
 for(int i=0;i<battle_count;i++)test32_battle_emit(&battle_rows[i],site,hs28_market.year,
  hs28_market.type,hs28_market.week,tid,complete && budget_ok);
 for(int i=0;i<execution_count;i++)test33_execution_emit(&execution_rows[i],site,hs28_market.year,
  hs28_market.type,hs28_market.week,tid,complete && budget_ok);
 /* Recovery is available to every HS/JUCO star level. Reconcile its pending
    costs independently of the elite coverage catalogue. A failed native
    offer releases only our reservation; the safe recovery pass may try a
    different qualified school without spending this team's budget twice. */
 if(post && complete) {
  int kept=0;
  for(int i=0;i<hs28_market.recovery_count[tid];i++) {
   Hs28MarketReservation r=hs28_market.recovery[tid][i];
   if(!materialized[i] || !offered[i] || secured[i]) {
    if(!secured[i])hs31_retry_deferred(r.table,r.rid,tid,0);
    log_line("HS28_RECOVERY_RESERVATION,%u,%u,%s",tid,r.rid,secured[i]?"SECURED_CLOSED":"NATIVE_OFFER_NOT_COMPLETED");
    continue;
   }
   r.new_board=0;r.observed=funded_reservation[i] && budget_ok;
   hs31_retry_deferred(r.table,r.rid,tid,r.observed);
   if(budget_ok)r.cost=funded_reservation[i]?0:hs28_recovery_cost(eval,1);
   hs28_market.recovery[tid][kept++]=r;
   log_line("HS28_RECOVERY_RESERVATION,%u,%u,%s,%d",tid,r.rid,funded_reservation[i] && budget_ok?"FUNDED":"OFFERED_AWAITING_FUNDING",r.cost);
  }
  hs28_market.recovery_count[tid]=kept;
 }
}
static void hs28_coverage_begin(U eval) {
 if(mode!=1 || faulted || !hs28_pair_window(eval))return;
 int matching=hs28_market_scope(eval,1);if(matching && hs28_market.ready)return;
 Rec f=context_ref(eval,6),a=f?array_field(f,39):0;
 if(!a){release(f);log_line("HS28_COVERAGE_SKIP,BEGIN,POOL_UNAVAILABLE");return;}
 U n=array_count(a);if(n>HS28_MARKET_LIMIT){release(a);release(f);log_line("HS28_COVERAGE_SKIP,BEGIN,POOL_BOUNDS");return;}
 for(U i=0;i<n;i++) {
  Rec r=array_at(a,i);if(!r)continue;
  if(hs15_is_recruit(r) && scalar(r,9,0x108)<5) {
   Rec p=reference(r,5);if(p) {
    int stars=(int)scalar(p,231,0x108),pos=(int)scalar(p,3,0x108);
    if(stars>=3 && stars<=4 && pos>=0 && pos<22) {
     Hs28MarketRecruit *m=&hs28_market.recruits[hs28_market.count];
     m->table=q((U)r+0x10);m->rid=row(r);m->player_table=q((U)p+0x10);m->pid=row(p);
     m->stars=stars;m->pos=pos;m->best_team=-1;
     unsigned h=(unsigned)((m->table>>4)^((U)m->rid*2654435761u))&(HS28_MARKET_HASH-1);
     while(hs28_market.hash[h])h=(h+1)&(HS28_MARKET_HASH-1);
     hs28_market.hash[h]=++hs28_market.count;
    }
    release(p);
   }
  }
  release(r);
 }
 release(a);
 /* Published ownership can coexist with a soft-looking recruit stage. Build
    this exclusion once from all acquiring teams, rather than trusting stage. */
 Rec teams=array_field(f,45);release(f);
 if(!teams || array_count(teams)>138){release(teams);log_line("HS28_COVERAGE_SKIP,BEGIN,TEAMS_UNAVAILABLE");return;}
 U nt=array_count(teams);
 for(U t=0;t<nt;t++) {
  Rec team=array_at(teams,t),commits=team?array_field(team,33):0;
  if(commits && array_count(commits)<=192)for(U j=0;j<array_count(commits);j++) {
   Rec p=array_at(commits,j);if(!p)continue;
   for(int k=0;k<hs28_market.count;k++)if(hs28_market.recruits[k].player_table==q((U)p+0x10) && hs28_market.recruits[k].pid==row(p))hs28_market.recruits[k].owned=1;
   release(p);
  }
  release(commits);release(team);
 }
 hs28_market.valid=1;hs28_market.ready=1;
 for(U t=0;t<nt;t++){Rec team=array_at(teams,t);if(team)hs28_coverage_observe(team,eval,"BEGIN");release(team);}
 release(teams);
 hs28_coverage_seed(eval);
 test29r2_class_seed(eval);
 log_line("HS28_COVERAGE_BEGIN,%u,%u,%u,%d",hs28_market.year,hs28_market.type,hs28_market.week,hs28_market.count);
}
static void hs28_coverage_repair(Hs28Choice *pool,int n,Rec team,U eval) {
 if(mode!=1 || faulted || !hs28_market_scope(eval,0) || n<=0)return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 int *counts=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(size_t)n*sizeof(int));if(!counts)return;
 for(int i=0;i<n;i++) {
  Hs28MarketRecruit *m=hs28_market_find(pool[i].table,pool[i].rid);
  counts[i]=m && !m->owned?hs28_market_count(m,2,(int)tid):2;
  if(m && !m->owned) {
   m->candidates++;m->legal+=pool[i].legal!=0;m->viable+=pool[i].legal && pool[i].viable;
   if(pool[i].legal && pool[i].viable && pool[i].native_score>m->best_score){m->best_score=pool[i].native_score;m->best_team=(int)tid;}
  }
 }
 int changed=hs28_coverage_swap(pool,n,counts);
 if(changed)log_line("HS28_COVERAGE_REPAIR,%u,%d,SCORE_PRESERVING_BACKUP_SUBSTITUTION",tid,changed);
 HeapFree(GetProcessHeap(),0,counts);
}
static void hs28_coverage_reserve_plan(const Hs28Choice *pool,int n,Rec team,U eval) {
 if(mode!=1 || faulted || !hs28_market_scope(eval,0))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 for(int i=0;i<hs28_market.count;i++)hs28_market_bit(hs28_market.recruits[i].planned,tid,0);
 for(int i=0;i<n;i++)if(pool[i].selected && pool[i].legal && pool[i].viable && pool[i].weight>=20) {
  Hs28MarketRecruit *m=hs28_market_find(pool[i].table,pool[i].rid);
  if(m && !m->owned){hs28_market_bit(m->planned,tid,1);m->selected++;}
 }
 hs28_market.team_seen[tid]=1;hs28_recovery_teams[tid].valid=0;
}
static void hs28_coverage_end(U eval) {
 if(mode!=1 || faulted || !hs28_market_scope(eval,0))return;
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;release(f);
 if(!teams || array_count(teams)>138){release(teams);return;}
 U n=array_count(teams);
 for(U i=0;i<n;i++){Rec team=array_at(teams,i);if(team)hs28_coverage_observe(team,eval,"END");release(team);}
 release(teams);
 for(int i=0;i<hs28_market.count;i++) {
  Hs28MarketRecruit *m=&hs28_market.recruits[i];if(m->owned)continue;
  int actual=hs28_market_count(m,0,-1),planned=hs28_market_count(m,1,-1);
  int firm=0,conditional=0,lost=0;
  for(unsigned t=0;t<138;t++){
   firm+=hs28_market_has(m->firm,t) && hs28_market_has(m->funded,t);
   conditional+=hs28_market_has(m->conditional,t) && hs28_market_has(m->funded,t);
   lost+=hs28_market_has(m->ever_funded,t) && !hs28_market_has(m->offered,t);
  }
  log_line("HS30_OFFER_COVERAGE,%u,%d,%d,%d,%d",m->rid,firm,conditional,lost,planned);
  const char *why=!firm?"NO_FUNDED_COMMITTABLE_OFFER":actual>=2?"COVERED":lost?"FUNDED_PURSUIT_BECAME_UNAVAILABLE":!m->candidates?"NATIVE_CANDIDATE_NOT_ADMITTED":!m->legal?"NO_LEGAL_OPENING_OR_ELIGIBILITY":!m->viable?"NO_CREDIBLE_ACCESSIBLE_RACE":planned>=2?"OFFER_OR_FUNDING_NOT_EXECUTED":"STRONGER_EXISTING_OBLIGATION_OR_CAPACITY";
  log_line("HS28_COVERAGE_RESULT,%u,%d,%d,%d,%u,%u,%u,%u,%d,%d,%s",m->rid,m->stars+1,actual,planned,m->candidates,m->legal,m->viable,m->selected,m->best_team,m->best_score,why);
 }
}
/* Abandonment may remove ONLY the stale shortlist gate. This probe proves a
   real pair path and positive native score before any recruit state changes.
   Resources here mean available native hours, not a promised weight-to-hours
   conversion. The normal action path remains responsible for affordability. */
static int hs28_recovery_reserved(unsigned tid,int slot,int role,Rec recruit) {
 for(int j=0;j<hs28_market.recovery_count[tid];j++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][j];
  if(r->slot==slot && r->role==role && !(r->table==q((U)recruit+0x10) && r->rid==row(recruit)))return 1;
 }
 return 0;
}
/* Native RecruitingTunables49 is RecruitingActions. The captured action
   descriptor has Cost2, BaseInfluenceGranted1, enabled5, immediate6. Native
   scholarship execution requests ActionType3; resolve by type, never array index. */
static int hs28_recovery_cost(U eval,int offered) {
 Rec tuning=(Rec)q(eval+0xd0),actions=tuning?array_field(tuning,49):0;
 if(!actions)return 0;
 U n=array_count(actions);if(n<2 || n>64){release(actions);return 0;}
 unsigned scholarship=0,effort=0;
 for(U i=0;i<n;i++) {
  Rec action=array_at(actions,i);if(!action)continue;
  unsigned type=scalar(action,0,0x108),cost=scalar(action,2,0x100),gain=scalar(action,1,0x100);
  int enabled=scalar(action,5,0xf8)!=0,immediate=scalar(action,6,0xf8)!=0;
  if(enabled && immediate && type==3 && !gain && cost>0 && cost<=1000)scholarship=cost;
  if(enabled && !immediate && cost>0 && cost<=1000 && gain>0 && (!effort || cost<effort))effort=cost;
  release(action);
 }
 int valid=scholarship>0;
 release(actions);return valid && effort?(int)(effort+(offered?0:scholarship)):0;
}
/* Only an unoffered HS board target with no paid work, visit, earned lead,
   annual role, offer permission or pending recovery may yield a board place.
   Inspect live handles. The forecast never removes anything. */
static int r2_hs_spares(Rec team,U eval,const Hs28Choice *plan,int n,Rec *victim) {
 if(victim)*victim=0;
 Rec board=board_array(team);U size=board?array_count(board):MAX_BOARD+1;
 if(size>50){release(board);return 0;}
 int count=0,best=0;unsigned tid=scalar(team,383,0x100);
 for(U j=0;j<size;j++){
  Rec t=array_at(board,j),r=t?reference(t,10):0;
  int k=-1;for(int i=0;r && i<n;i++)if(hs28_identity(&plan[i],r)){k=i;break;}
  if(k>=0 && hs15_is_recruit(r) && plan[k].stars<3 && !plan[k].selected && !plan[k].market_protected &&
      !plan[k].market_planned && !plan[k].offered && !plan[k].points && !scalar(t,8,0x100) && !scalar(t,9,0x100) &&
      scalar(r,9,0x108)<4 && scalar(t,12,0x108)<2 && !scalar(t,1,0x100) &&
      !scalar(t,2,0x100) && !scalar(t,6,0x100) && (*(int16_t *)(schema(t)+0xd8)<=21 || !scalar(t,21,0xf8))) {
   Rec visit=reference(t,11);unsigned mask=0;
   int safe=!visit && !hs28_productive_allocated(t,eval,&mask);release(visit);
   for(int z=0;safe && tid<138 && z<hs28_market.recovery_count[tid];z++){
    Hs28MarketReservation *p=&hs28_market.recovery[tid][z];
    if(p->table==q((U)r+0x10) && p->rid==row(r))safe=0;
   }
   if(safe){int owner=-1;safe=portal_published_owner(r,eval,&owner)==0;}
   if(safe){
    count++;int cost=plan[k].stars*100000+(int)scalar(t,8,0x100);
    if(victim && (!*victim || cost<best)){
     release(*victim);*victim=r;r=0;best=cost;
    }
   }
  }
  release(r);release(t);
 }
 release(board);return count;
}
static int r2_hs_requested(Rec recruit,Rec team,U eval) {
 if(!hs28_market_scope(eval,0))return 0;
 Hs28MarketRecruit *m=hs28_market_find(q((U)recruit+0x10),row(recruit));
 if(!m || m->owned || m->stars<3 || m->stars>4)return 0;
 unsigned tid=scalar(team,383,0x100);
 return tid<138 && !hs28_market_count(m,0,-1) && scalar(recruit,14,0x100)==0;
}
static int hs28_recovery_destination(Rec recruit,Rec team,U eval,const Hs15Snapshot *shot,int *score,int *slot,int *role) {
 *score=*slot=*role=0;
 if(academy29_hs_block(recruit,team)){hs34_trace_note(team,recruit,HS34_ACADEMY);return 0;}
 if(hs32_recovery_block(recruit,team,eval)){hs34_trace_note(team,recruit,HS34_PERMIT);return 0;}
 if(!hs28_market_scope(eval,1))hs28_market.valid=1;
 unsigned tid=scalar(team,383,0x100);if(tid>=138){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
 Rec user=reference(team,407);if(user){release(user);hs34_trace_note(team,recruit,HS34_USER);return 0;}
 int blocked=hs15_snapshot_block(shot,recruit);
 hs34_trace_block(team,recruit,shot,position(recruit),blocked);if(blocked)return blocked<0?-1:0;
 if(!native_recruit_eligible(eval,team,recruit))return 0;
 if(!hs28_native_score(recruit,team,eval,score)){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
 if(hs34_trace_matches(team,recruit))hs34_trace.score=*score;
 if(*score<=0){hs34_trace_note(team,recruit,HS34_SCORE);return 0;}
 Hs28RecoveryTeam *cache=&hs28_recovery_teams[tid];
 if(!cache->valid) {
  if(!hs28_make_board_plan(team,eval,shot,cache->plan,&cache->n,0)){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
  Rec board=reference(team,224),a=board?array_field(board,0):0;
  if(!board || !a){release(a);release(board);hs34_trace_note(team,recruit,HS34_DATA);return -1;}
  cache->count=(int)array_count(a);cache->total=scalar(board,3,0x100);
  cache->assigned=scalar(board,1,0x100);cache->processed=scalar(board,2,0x100);
  release(a);release(board);
  if(cache->count>50 || cache->total>100000 || cache->assigned>100000 || cache->processed>100000){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
  cache->valid=1;
 }
 Rec live_board=reference(team,224),live_array=live_board?array_field(live_board,0):0;
 if(!live_board || !live_array){release(live_array);release(live_board);hs34_trace_note(team,recruit,HS34_DATA);return -1;}
 cache->count=(int)array_count(live_array);cache->total=scalar(live_board,3,0x100);cache->assigned=scalar(live_board,1,0x100);
 release(live_array);release(live_board);if(cache->count>50 || cache->total>100000 || cache->assigned>100000){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
 Hs28Choice *plan=cache->plan;int n=cache->n;
 int own=-1;for(int i=0;i<n;i++)if(hs28_identity(&plan[i],recruit)){own=i;break;}
 int cost=hs28_recovery_cost(eval,own>=0 && plan[own].offered),reserved=0,new_board=0;
 if(hs34_trace_matches(team,recruit))hs34_trace.cost=cost;
 if(cost<=0){hs34_trace_note(team,recruit,HS34_ACTION_COST);return 0;}
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];
  if(r->table==q((U)recruit+0x10) && r->rid==row(recruit))continue;
  reserved+=r->cost;new_board+=r->new_board;
 }
 if(hs34_trace_matches(team,recruit))hs34_trace.available=(int)cache->total-(int)cache->assigned-reserved;
 if((int64_t)cache->total-cache->assigned-reserved<cost){hs34_trace_note(team,recruit,HS34_HOURS);return 0;}
 if(own<0 && cache->count+new_board>=50) {
  if(!r2_hs_requested(recruit,team,eval) || r2_hs_spares(team,eval,plan,n,0)<=cache->count+new_board-50){
   hs34_trace_note(team,recruit,HS34_BOARD);return 0;
  }
 }
 if(r2_hs_requested(recruit,team,eval)){
  Rec target=t36_target(team,recruit);int ask=0,paid=0,wallet=0;
  int affordable=r2_quote(eval,team,recruit,target,&ask,&paid,&wallet) && ask-paid<=wallet;
  release(target);
  if(!affordable){hs34_trace_note(team,recruit,HS34_NIL);return 0;}
 }
 int pos=position(recruit),vacancy[22],optional[22],general=hs28_rooms(shot,vacancy,optional);
 if(pos<0 || pos>=22 || general<0){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
 if(own>=0 && plan[own].selected && plan[own].choice==1){
  if(hs28_recovery_reserved(tid,plan[own].slot,1,recruit)){hs34_trace_note(team,recruit,HS34_RESERVED);return 0;}
  *slot=plan[own].slot;*role=1;return 1;
 }
 Hs28Choice challenger={0};
 challenger.pos=pos;challenger.legal=challenger.viable=1;challenger.native_score=*score;
 Rec player=reference(recruit,5);
 if(!player){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
 challenger.ovr=(int)scalar(player,191,0x100);challenger.stars=(int)scalar(player,231,0x108);release(player);
 for(int i=0;i<n;i++)if(i!=own && plan[i].selected && plan[i].choice==1 && plan[i].pos==pos &&
   hs34_primary_upgrade(&challenger,&plan[i]) && !hs28_recovery_reserved(tid,plan[i].slot,1,recruit)){
  *slot=plan[i].slot;*role=1;return 1;
 }
 if(own>=0 && plan[own].selected) {
  if(hs28_recovery_reserved(tid,plan[own].slot,plan[own].choice,recruit)){hs34_trace_note(team,recruit,HS34_RESERVED);return 0;}
  *slot=plan[own].slot;*role=plan[own].choice;return 1;
 }
 /* A new recovery primary must fit the same annual source plan used by
    admission. Distinct already planned/reserved primary seats count once;
    replacing a primary or becoming its conditional backup consumes no new
    source seat. Never promise a raw vacancy that admission cannot honor. */
 unsigned char primary_seat[22*32+32]={0};int primary_count=0;
 for(int i=0;i<n;i++)if(plan[i].selected && plan[i].choice==1) {
  int s=plan[i].slot;if(s<1 || s>=22*32+32){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
  if(!primary_seat[s]){primary_seat[s]=1;primary_count++;}
 }
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];
  /* Revalidating a reservation does not request a second source seat. */
  if(r->role!=1 || (r->table==q((U)recruit+0x10) && r->rid==row(recruit)))continue;
  if(r->slot<1 || r->slot>=22*32+32){hs34_trace_note(team,recruit,HS34_DATA);return -1;}
  if(!primary_seat[r->slot]){primary_seat[r->slot]=1;primary_count++;}
 }
 int pending_repair=0;
 for(int p=1;p<=18;p++)if(primary_seat[p*32+31] && hs34_vacancy_extra(shot,p))pending_repair++;
 if(hs34_vacancy_extra(shot,pos) && !primary_seat[pos*32+31])pending_repair++;
 if(pending_repair>general)pending_repair=general;
 int source_room=test29_class_new_primary_room(shot,tid,primary_count,pending_repair);
 /* Complete another real primary opening before using its candidate as a
    conditional backup of the first opening. This matters when an academy
    has several genuine replacements in the same position. */
 for(int r=1;r<=2;r++)for(int seat=0;seat<vacancy[pos];seat++) {
  int s=pos*32+seat+1,occupied=0;
  if((r==1 && !source_room) || (r==2 && !primary_seat[s]))continue;
  for(int i=0;i<n;i++)if(i!=own && plan[i].selected && plan[i].slot==s && plan[i].choice==r)occupied=1;
  if(!occupied && !hs28_recovery_reserved(tid,s,r,recruit)){*slot=s;*role=r;return 1;}
 }
 if(optional[pos] && general>0) {
  int used=0;for(int i=0;i<n;i++)if(plan[i].selected && plan[i].choice==1 && plan[i].slot%32==31)used++;
  for(int i=0;i<hs28_market.recovery_count[tid];i++) {
   Hs28MarketReservation *r=&hs28_market.recovery[tid][i];int in_plan=0;
   if(r->table==q((U)recruit+0x10) && r->rid==row(recruit))continue;
   for(int j=0;j<n;j++)if(plan[j].selected && plan[j].slot==r->slot && plan[j].choice==1)in_plan=1;
   if(r->role==1 && r->slot%32==31 && !in_plan)used++;
  }
  int s=pos*32+31,primary=0,backup=0;
  for(int i=0;i<n;i++)if(i!=own && plan[i].selected && plan[i].slot==s){primary|=plan[i].choice==1;backup|=plan[i].choice==2;}
  for(int i=0;i<hs28_market.recovery_count[tid];i++) {
   Hs28MarketReservation *r=&hs28_market.recovery[tid][i];
   if(r->table==q((U)recruit+0x10) && r->rid==row(recruit))continue;
   if(r->slot==s){primary|=r->role==1;backup|=r->role==2;}
  }
  if(!primary && used<general && source_room && !hs28_recovery_reserved(tid,s,1,recruit)){*slot=s;*role=1;return 1;}
  if(primary && !backup && !hs28_recovery_reserved(tid,s,2,recruit)){*slot=s;*role=2;return 1;}
 }
 hs34_trace_note(team,recruit,source_room?HS34_ROLE_OCCUPIED:HS34_CLASS_TARGET);
 return 0;
}
static int hs28_recovery_reserve(Rec team,Rec recruit,U eval,int slot,int role) {
 if(hs32_recovery_block(recruit,team,eval))return 0;
 if(!hs28_market_scope(eval,0)) {
  /* Recovery precedes the coverage catalogue at the safe weekly boundary. */
  hs28_market_scope(eval,1);hs28_market.valid=1;
 }
 unsigned tid=scalar(team,383,0x100);if(tid>=138 || slot<1 || slot>=22*32+32 || role<1 || role>2)return -1;
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];
  if(r->table==q((U)recruit+0x10) && r->rid==row(recruit))return r->slot==slot && r->role==role;
  if(r->slot==slot && r->role==role)return 0;
 }
 int n=hs28_market.recovery_count[tid];if(n>=50)return 0;
 Hs28RecoveryTeam *cache=&hs28_recovery_teams[tid];int existing=0,offered=0;
 for(int i=0;i<cache->n;i++)if(hs28_identity(&cache->plan[i],recruit)){existing=1;offered=cache->plan[i].offered;break;}
 int cost=hs28_recovery_cost(eval,offered);if(cost<=0)return 0;
 if(cache->valid)hs28_plan_store(cache->plan,cache->n,team,eval);
 hs28_market.recovery[tid][n]=(Hs28MarketReservation){q((U)recruit+0x10),row(recruit),slot,role,cost,!existing,0};hs28_market.recovery_count[tid]++;
 Hs28MarketRecruit *m=hs28_market_find(q((U)recruit+0x10),row(recruit));if(m)hs28_market_bit(m->planned,tid,1);
 return 1;
}
static Hs34Trace hs34_coverage_trace[138];
static void hs28_coverage_seed(U eval) {
 if(!hs28_market_scope(eval,0) || !hs28_market.ready)return;
 hs28_recovery_cache_invalidate();
 Hs15RecoveryCache cache={0};Rec f=context_ref(eval,6),pool=f?array_field(f,39):0;release(f);
 if(!pool)return;
 U n=array_count(pool);if(n>HS28_MARKET_LIMIT){release(pool);return;}
 /* Global zero-before-one sweep, including CPU schools not on an existing
    interest list. Closed active races keep their native access restrictions. */
 for(int objective=-1;objective<2;objective++)for(U i=0;i<n && !faulted;i++) {
  Rec recruit=array_at(pool,i);if(!recruit)continue;
  Hs28MarketRecruit *m=hs28_market_find(q((U)recruit+0x10),row(recruit));
  if(!m || m->owned || scalar(recruit,9,0x108)>=5){release(recruit);continue;}
  int firm=hs30_firm_count(recruit,eval);
  if(firm<0){release(recruit);portal_fault("HS30_coverage_offers");break;}
  int pending_primary=0;
  for(unsigned t=0;t<138;t++)for(int j=0;j<hs28_market.recovery_count[t];j++){
   Hs28MarketReservation *r=&hs28_market.recovery[t][j];
   if(r->table==m->table && r->rid==m->rid && r->role==1)pending_primary=1;
  }
  if(objective<0 ? firm>0 || pending_primary : hs28_market_count(m,2,-1)!=objective){release(recruit);continue;}
  /* One live snapshot per recruit/sweep, never a fresh global scan for
     every candidate school. It does not survive this coordinator iteration. */
  int effective_leader=firm?hs30_offer_leader(recruit,eval):0;
  if(effective_leader<0){release(recruit);portal_fault("HS30_coverage_leader");break;}
  int best=-1,best_score=0,best_slot=0,best_role=0,eligible=0,accessible=0,capacity=0;
  int trace=objective<0 && m->trace_passes<2;
  if(trace)memset(hs34_coverage_trace,0,sizeof(hs34_coverage_trace));
  for(unsigned tid=0;tid<138;tid++) {
   if(objective>=0 && (hs28_market_has(m->planned,tid) ||
      (hs28_market_has(m->offered,tid) && hs28_market_has(m->funded,tid))))continue;
   Rec team=portal_team_by_id(eval,tid);if(!team)continue;
   Rec user=reference(team,407);if(user){release(user);release(team);continue;}
   if(trace)hs34_trace_begin(team,recruit);
   if(locked_out(recruit,team,eval)){
    if(trace){hs34_trace_note(team,recruit,HS34_CLOSED_RACE);hs34_coverage_trace[tid]=hs34_trace;hs34_trace_end();}
    release(team);continue;
   }accessible++;
   Hs15Snapshot *shot=hs15_recovery_snapshot(&cache,team,eval);
   if(!shot){if(trace){hs34_coverage_trace[tid]=hs34_trace;hs34_trace_end();}release(team);continue;}
   Hs28Choice choice;int score=0,slot=0,role=0;
   int valid=hs28_choice(&choice,recruit,0,team,eval,shot,0);
   int viable=valid && choice.legal && (choice.viable || hs30_catchup_to(&choice,effective_leader));
   if(!viable){
    if(trace){if(valid && choice.legal)hs34_trace_note(team,recruit,HS34_VIABILITY);hs34_coverage_trace[tid]=hs34_trace;hs34_trace_end();}
    release(team);continue;
   }eligible++;
   int result=hs28_recovery_destination(recruit,team,eval,shot,&score,&slot,&role);
   if(result>0 && (objective>=0 || role==1)){capacity++;if(score>best_score || (score==best_score && (best<0 || tid<(unsigned)best))){best=(int)tid;best_score=score;best_slot=slot;best_role=role;}}
   if(trace){
    if(result>0)hs34_trace_note(team,recruit,role==1?HS34_ACCEPTED:HS34_BACKUP_ONLY);
    hs34_trace.score=score;hs34_trace.slot=slot;hs34_trace.role=role;
    hs34_coverage_trace[tid]=hs34_trace;hs34_trace_end();
   }
   release(team);
  }
  if(best>=0) {
   Rec team=portal_team_by_id(eval,(unsigned)best);Hs15Snapshot shot;int score=0,slot=0,role=0;
   int ok=team && hs15_collect(&shot,team,eval) && !locked_out(recruit,team,eval) &&
    hs28_recovery_destination(recruit,team,eval,&shot,&score,&slot,&role)>0 && slot==best_slot && role==best_role &&
    hs28_recovery_reserve(team,recruit,eval,slot,role)>0;
   log_line("HS28_COVERAGE_SEED,%u,%d,%d,%d,%d,%d,%s",m->rid,best,best_score,best_slot,best_role,objective,ok?"RESERVED_NATIVE_PURSUIT":"REVALIDATION_REJECTED");release(team);
  } else {
   log_line("HS28_COVERAGE_SHORTFALL,%u,%d,%d,%d,%d,%s",m->rid,objective,accessible,eligible,capacity,
    !accessible?"PROTECTED_CLOSED_RACE":!eligible?"NO_ELIGIBLE_CREDIBLE_OPENING":"NO_PAIR_CAPACITY_OR_AFFORDABLE_ACTION");
   if(trace){
    for(int tid=0;tid<138;tid++)if(hs34_coverage_trace[tid].active)hs34_trace_emit(&hs34_coverage_trace[tid],"ELITE_PRIMARY_SEARCH");
    m->trace_passes++;
   }
  }
  release(recruit);
 }
 release(pool);hs15_recovery_cache_free(&cache);
}
#include "test29r2_class_recovery.inc"
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestHs34RecoveryGate(Rec recruit,Rec team,U eval) {
 Hs15Snapshot shot;if(!hs15_collect(&shot,team,eval))return HS34_DATA;
 if(!hs28_market_scope(eval,1))hs28_market.valid=1;
 hs34_trace_begin(team,recruit);Hs28Choice choice;int score=0,slot=0,role=0;
 int valid=hs28_choice(&choice,recruit,0,team,eval,&shot,0);
 if(valid && choice.legal) {
  if(!choice.viable)hs34_trace_note(team,recruit,HS34_VIABILITY);
  else {
   int result=hs28_recovery_destination(recruit,team,eval,&shot,&score,&slot,&role);
   if(result>0)hs34_trace_note(team,recruit,role==1?HS34_ACCEPTED:HS34_BACKUP_ONLY);
  }
 }
 hs34_trace.score=score;hs34_trace.slot=slot;hs34_trace.role=role;
 hs34_trace_emit(&hs34_trace,"FIXTURE_GATE");hs34_trace_end();return hs34_trace.code;
}
__declspec(dllexport) int DriTestHs34NativeEligibility(Rec team,Rec recruit,U eval) {
 hs34_trace_begin(team,recruit);int allowed=native_recruit_eligible(eval,team,recruit);hs34_trace_end();return allowed;
}
__declspec(dllexport) unsigned DriTestHs34ReasonByte(unsigned index){return index<96?(unsigned char)hs34_trace.native_reason[index]:0;}
__declspec(dllexport) int DriTestHsRecoveryPending(Rec r,U e){return hs28_recovery_pending(r,e);}
__declspec(dllexport) int DriTestHsRecoveryReservedCost(Rec r,Rec t,U e,int field) {
 if(!hs28_market_scope(e,0))return -1;unsigned tid=scalar(t,383,0x100);if(tid>=138)return -1;
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {Hs28MarketReservation *v=&hs28_market.recovery[tid][i];if(v->table==q((U)r+0x10) && v->rid==row(r))return field?v->new_board:v->cost;}
 return -1;
}
__declspec(dllexport) void DriTestHsCoverageReset(void){hs34_trace_end();memset(&hs28_market,0,sizeof(hs28_market));memset(hs28_recovery_teams,0,sizeof(hs28_recovery_teams));}
__declspec(dllexport) void DriTestHsCoverageBegin(U eval){hs28_coverage_begin(eval);}
__declspec(dllexport) void DriTestHsCoverageObserve(Rec team,U eval){hs28_coverage_observe(team,eval,"POST_FUNDING");}
__declspec(dllexport) void DriTestHsCoverageEnd(U eval){hs28_coverage_end(eval);}
__declspec(dllexport) int DriTestHsCoverageCount(Rec r,int kind){Hs28MarketRecruit *m=hs28_market_find(q((U)r+0x10),row(r));return m?hs28_market_count(m,kind,-1):-1;}
__declspec(dllexport) int DriTestHsRecoveryDestination(Rec r,Rec t,U e,int field){Hs15Snapshot s;if(!hs15_collect(&s,t,e))return -1;int score,slot,role;int result=hs28_recovery_destination(r,t,e,&s,&score,&slot,&role);return result<=0?result:field==1?slot:field==2?role:score;}
__declspec(dllexport) int DriTestHsRecoveryReserve(Rec r,Rec t,U e,int slotrole){return hs28_recovery_reserve(t,r,e,slotrole/4,slotrole%4);}
#endif
