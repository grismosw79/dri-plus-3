/* Test29: one global, pre-execution portal market. Native scoring/eligibility
   and the normal role planner create affordable intentions; synchronous
   rejections redistribute initial demand before school callbacks consume it.
   This snapshot writes no native score, offer, influence, hours or ownership.
   Live new-entry authorization is consumed by board admission, retained-board
   scoring, and the Test31 universal scholarship-action preflight. */
#include "market29_core.h"
#define MARKET29_PORTAL_LIMIT 12000
#define MARKET29_PORTAL_HASH 16384
typedef struct {
 U table;uint32_t rid;int pos,raw_ovr,stars,best_score;
 Market29Pressure pressure;
 int16_t forecast_board[138],forecast_retained[138];
 int forecast_rejected[7],forecast_fit;
 uint64_t recovery_primary[3],recovery_attempted[3],recovery_executed[3];
 unsigned char pursuit_percent[138]; /* H3 diagnostic/priority metadata only */
} Market29PortalRecruit;
typedef struct {
 Rec team;PortalChoice *pool;int n,capacity,remaining,prestige;
 PortalQuality rooms[22];
 U native_cache[67];
} Market29PortalSchool;
static struct {
 int ready,building,reconciling,count,rounds;U eval;
 Hs28TeamPlan scope;int hash[MARKET29_PORTAL_HASH];
 Market29PortalRecruit recruits[MARKET29_PORTAL_LIMIT];
} market29_portal;
static int portal29_admit(Rec team,Rec recruit,U eval,int verified_score);
static Market29PortalRecruit *market29_portal_find(U table,uint32_t rid) {
 unsigned h=(unsigned)((table>>4)^((U)rid*2654435761u))&(MARKET29_PORTAL_HASH-1);
 for(int n=0;n<MARKET29_PORTAL_HASH;n++,h=(h+1)&(MARKET29_PORTAL_HASH-1)) {
  int k=market29_portal.hash[h];if(!k)return 0;
  Market29PortalRecruit *m=&market29_portal.recruits[k-1];if(m->table==table && m->rid==rid)return m;
 }return 0;
}
static int market29_portal_scope(Rec team,U eval) {
 Hs28TeamPlan s={0};if(!hs28_scope(&s,team,eval))return 0;
 Hs28TeamPlan *p=&market29_portal.scope;
 return market29_portal.eval==eval && p->franchise_table==s.franchise_table &&
  p->franchise_handle==s.franchise_handle && p->franchise_row==s.franchise_row &&
  p->year==s.year && p->type==s.type && p->week==s.week;
}
static int market29_portal_sum(const uint64_t bits[3],unsigned own) {
 int n=0;for(unsigned t=0;t<138;t++)if(t!=own)n+=market29_has(bits,t);return n;
}
/* Captured native board prepare 08172cce/08172ce4 constructs this 0x218-byte
   inline school cache, then passes it to both candidate routes. A null cache
   is NOT equivalent: the needs component falls back to aggregate team needs,
   rather than the per-position array. That mismatch nominated schools whose
   real native board score was nonpositive. These native routines only fill
   the temporary cache; their inline vectors never allocate at these sizes. */
static int market29_portal_school_cache(Rec team,U eval,U cache[67]) {
 U scorer=q(eval+0xf8);if(!scorer)return 0;
 memset(cache,0,67*sizeof(U));
 ((void(*)(U*))(game_base+0x08142690))(cache);
 ((void(*)(U,Rec*,U*))(game_base+0x0815c640))(scorer,&team,cache);
 U start=(U)cache;
 return q(start)==start+0x18 && q(start+0xa8)==start+0xc0 && q(start+0x150)==start+0x168;
}
/* H2: native argument DL selects the recruit SOURCE (0 HS / 1 transfer),
   not new-board versus retained-board state. Applying both profiles made
   normal HS one-star penalties veto otherwise positive portal candidates.
   R2's linked-player user-control flag remains correct and is preserved.
   Native board callers also convert the season-local week to calendar week
   through 085daba0 before scoring; use that same read-only conversion. */
static int market29_portal_score_week(Rec recruit,Rec team,U eval,U *cache,int week,int *out) {
 U scorer=q(eval+0xf8);if(!scorer || !out)return 0;
 struct {U rec;int32_t values[10];} candidate={(U)recruit,{0}};
 _Static_assert(sizeof(candidate)==48,"Test29 portal native candidate ABI");
 Rec player=reference(recruit,5);if(!player)return 0;
 unsigned flag=scalar(player,0,0xf8);release(player);
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;release(f);
 if(!season || week<0 || week>100){release(season);return 0;}
 unsigned type=scalar(season,5,0x108);if(type>6){release(season);return 0;}
 Wrap sw=wrap(season);
 int calendar=((int(*)(Wrap*,unsigned,int))(game_base+0x085dac00))(&sw,type,week);
 release(season);if(calendar<0 || calendar>100)return 0;
 ((void(*)(U,unsigned char,unsigned char,Rec*,U*,void*,int))(game_base+0x081858a0))
  (scorer,(unsigned char)(transfer_recruit(recruit)!=0),(unsigned char)flag,&team,cache,&candidate,calendar);
 if(candidate.rec!=(U)recruit)return 0;
 *out=candidate.values[8];return 1;
}
static int market29_portal_score_cached(Rec recruit,Rec team,U eval,U *cache,int *out) {
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 int week=season?(int)scalar(season,4,0x100):-1;release(season);release(f);
 return market29_portal_score_week(recruit,team,eval,cache,week,out);
}
#ifdef DRI_EMULATION_TEST
static int market29_portal_native_score(Rec recruit,Rec team,U eval,int *out) {
 U cache[67];if(!market29_portal_school_cache(team,eval,cache))return 0;
 return market29_portal_score_cached(recruit,team,eval,cache,out);
}
#endif
/* Frozen authorization concerns new attention only. Native scoring/eligibility
   still decides whether an authorized pursuit is usable and affordable now. */
static int market29_portal_admission_block(Rec recruit,Rec team,U eval) {
 if(mode!=1 || faulted || !recruit || !team || !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 if(!market29_portal.ready || !market29_portal_scope(team,eval))return 1;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 1;
 Market29PortalRecruit *m=market29_portal_find(q((U)recruit+0x10),row(recruit));
 if(!m)return 1;
 if(r2_late_recruit==recruit && r2_late_team==team)return 0;
 return !market29_has(m->pressure.hard_authorized,tid) ||
  (!market29_has(m->pressure.protected_offer,tid) && !market29_has(m->pressure.planned,tid));
}
static void market29_portal_apply(PortalChoice *pool,int n,Rec team,U eval,const PortalQuality rooms[22],int prestige) {
 for(int i=0;i<n;i++) {
  PortalChoice *p=&pool[i];p->crowd_penalty=p->crowd_defer=p->crowd_admitted=p->coverage_priority=0;
  p->backup_penalty=p->market_known=p->market_planned=p->market_primary=p->market_protected=0;
 }
 if((!market29_portal.ready && !market29_portal.reconciling) || !market29_portal_scope(team,eval))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 for(int i=0;i<n;i++) {
  PortalChoice *p=&pool[i];
  if(p->pos<0 || p->pos>=22 || p->stage>=5 || p->native_base<=0 || !p->race_ok ||
    dri_quality_block_candidate(&rooms[p->pos],prestige,p->ovr,p->raw_ovr))continue;
  Market29PortalRecruit *m=market29_portal_find(q(p->rec+0x10),p->rid);if(!m)continue;
  p->market_protected=market29_has(m->pressure.protected_offer,tid);
  p->market_known=market29_portal.ready;
  p->market_planned=market29_has(m->pressure.planned,tid);
  p->market_primary=market29_has(m->pressure.primary,tid);
  /* At most two globally nominated recovery partners compete for a real
     opening. Zero firm destinations never promotes every school at once. */
  p->coverage_priority=market29_has(m->recovery_primary,tid);
  if(p->market_protected)continue;
  if((market29_portal.ready && !market29_has(m->pressure.hard_authorized,tid)) ||
     (market29_portal.reconciling && market29_has(m->pressure.hard_rejected,tid))) {
   p->market_known=1;p->market_planned=p->market_primary=p->coverage_priority=0;
   p->crowd_penalty=p->backup_penalty=100;p->crowd_defer=1;continue;
  }
  int pressure=market29_primary_pressure(&m->pressure,tid);
  int rejected=market29_has(m->pressure.rejected,tid);
  p->crowd_admitted=!rejected && market29_has(m->pressure.admitted,tid);
  int penalty=p->crowd_admitted?0:rejected?45:market29_penalty(1,pressure);
  int backup_rejected=market29_has(m->pressure.backup_rejected,tid);
  p->backup_penalty=backup_rejected?45:market29_has(m->pressure.backup_admitted,tid)?0:
   market29_backup_penalty(1,market29_pressure(&m->pressure,tid));
  if(!penalty)continue;
  int better=0,own_fit=m->pressure.fit[tid],best_fit=0;
  for(unsigned t=0;t<138;t++) {
   if(m->pressure.fit[t]>best_fit)best_fit=m->pressure.fit[t];
   if(m->pressure.fit[t]>own_fit ||
    (m->pressure.fit[t]==own_fit && market29_tie(m->rid,t,market29_portal.scope.year)<market29_tie(m->rid,tid,market29_portal.scope.year)))better++;
  }
  int strong=best_fit>0 && (int64_t)own_fit*100>=(int64_t)best_fit*90 && better<2;
  int competitive=p->influence>0 && p->leader>0 && (int64_t)p->influence*100>=(int64_t)p->leader*85;
  int urgent=rooms[p->pos].ideal>0 && rooms[p->pos].count*2<rooms[p->pos].ideal;
  if(strong || competitive || urgent || p->coverage_priority) {
   market29_bit(m->pressure.exception,tid,1);continue;
  }
  p->crowd_penalty=penalty;p->crowd_defer=penalty>=45;
 }
}
/* Stable, bounded nomination before school replanning. This is a preference,
   never a fabricated offer or a waiver of room, eligibility, or budget. */
static void market29_portal_seed_recovery(Market29PortalSchool *schools,Rec *held) {
 int nominated[138]={0},positions[138][22]={{0}},replaceable[138][22]={{0}};
 int cost=hs28_recovery_cost(market29_portal.eval,0);if(cost<=0)return;
 /* Reserve nomination budgets across the entire cohort, not independently
    per recruit. Only an actual selected unprotected primary opening can be
    redirected; a full fixed board or protected earned race is not capacity. */
 for(unsigned t=0;t<138;t++)if(schools[t].team && schools[t].capacity>0)
  for(int j=0;j<schools[t].n;j++) {
   PortalChoice *p=&schools[t].pool[j];
   if(p->selected && p->round==1 && !dri_portal_competitive(p))replaceable[t][p->pos]++;
  }
 for(int i=0;i<market29_portal.count;i++)for(unsigned t=0;t<138;t++)
  if(market29_has(market29_portal.recruits[i].recovery_primary,t)) {
   nominated[t]++;positions[t][market29_portal.recruits[i].pos]++;
  }
 for(int i=0;i<market29_portal.count;i++) {
  Market29PortalRecruit *m=&market29_portal.recruits[i];
  if(market29_portal_sum(m->pressure.firm,138) || market29_portal_sum(m->pressure.primary,138))continue;
  for(int seat=market29_portal_sum(m->recovery_primary,138);seat<2;seat++) {
   int best=-1;
   for(unsigned t=0;t<138;t++)if(schools[t].team && market29_recovery_capacity(schools[t].capacity,schools[t].remaining,cost,
       nominated[t],replaceable[t][m->pos],positions[t][m->pos]) && m->pressure.fit[t]>0 &&
      !market29_has(m->recovery_primary,t) && !market29_has(m->recovery_attempted,t) &&
      (m->stars>=3 || (portal_academy(t) && portal_academy_allowed(held[i],t))) &&
      (best<0 || market29_before(&m->pressure,t,(unsigned)best,m->rid,market29_portal.scope.year)))best=(int)t;
   if(best<0)break;
   market29_bit(m->recovery_primary,(unsigned)best,1);nominated[best]++;positions[best][m->pos]++;
  }
 }
}
static int market29_portal_retire_failed_recovery(void) {
 int retired=0;
 for(int i=0;i<market29_portal.count;i++)for(unsigned t=0;t<138;t++) {
  Market29PortalRecruit *m=&market29_portal.recruits[i];
  if(market29_has(m->recovery_primary,t) && !market29_has(m->pressure.primary,t)) {
   market29_bit(m->recovery_primary,t,0);market29_bit(m->recovery_attempted,t,1);retired++;
  }
 }
 return retired;
}
static void market29_portal_propose(Market29PortalSchool *s,U eval) {
 unsigned tid=scalar(s->team,383,0x100);int remaining=s->remaining;
 PortalChoice primaries[MAX_BOARD];int np=0;
 for(int j=0;j<s->n && np<MAX_BOARD;j++)
  if(s->pool[j].selected && s->pool[j].round==1)primaries[np++]=s->pool[j];
 /* Reuse the local role value after all coexisting additions are selected.
    Only forecast preference/fit are weighted. Native influence, source score,
    permissions, offers, funds and commitment thresholds remain untouched. */
 for(int j=0;j<s->n;j++) {
  PortalChoice *p=&s->pool[j];Market29PortalRecruit *m=market29_portal_find(q(p->rec+0x10),p->rid);if(!m)continue;
  int percent=dri_portal_planned_pursuit_percent(p,primaries,np,s->rooms,s->prestige);
  m->pressure.fit[tid]=dri_portal_pursuit_scale(p->native_base,percent);
  m->pressure.interest[tid]=dri_portal_pursuit_scale(p->influence,percent);
  if(percent!=m->pursuit_percent[tid] && (percent<100 || m->pursuit_percent[tid]))
   log_line("R2H3_PURSUIT,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,FORECAST_ONLY",
    market29_portal.scope.week,m->rid,tid,p->pos,p->raw_ovr,percent,p->native_base,
    m->pressure.fit[tid],p->influence,m->pressure.interest[tid],p->selected);
  m->pursuit_percent[tid]=(unsigned char)percent;
 }
 for(int order=1;order<=50;order++)for(int j=0;j<s->n;j++)if(s->pool[j].selected && s->pool[j].priority==order) {
  PortalChoice *p=&s->pool[j];Market29PortalRecruit *m=market29_portal_find(q(p->rec+0x10),p->rid);if(!m)continue;
  int active=market29_has(m->pressure.active,tid),cost=hs28_recovery_cost(eval,p->offered);
  if(!active && (cost<=0 || remaining<cost))continue;
  if(!active)remaining-=cost;
  market29_bit(m->pressure.planned,tid,1);
  if(p->round==1)market29_bit(m->pressure.primary,tid,1);
 }
}
static int market29_portal_existing(Rec team,unsigned tid) {
 Rec board=board_array(team);if(!board)return 0;
 U n=array_count(board);if(n>MAX_BOARD){release(board);return 0;}
 for(U i=0;i<n;i++) {
  Rec target=array_at(board,i),recruit=target?reference(target,10):0;
  if(!target || !recruit){release(recruit);release(target);release(board);return 0;}
  unsigned status=scalar(target,12,0x108);
  Market29PortalRecruit *m=market29_portal_find(q((U)recruit+0x10),row(recruit));
  /* Every existing scholarship consumes hard-cap space, including a dormant
     conditional opportunity. Its validity/funding are separate checks. */
  if(m && status>=2 && status<=4)market29_bit(m->pressure.protected_offer,tid,1);
  release(recruit);release(target);
 }
 release(board);return 1;
}
/* Previous unfunded board nominations should yield to a different viable
   school next week. A complete frozen pass still owns all authorizations. */
typedef struct {U franchise,table;unsigned rid,year,week;uint64_t failed[3];} T36PortalRetry;
static T36PortalRetry t36_retry[MARKET29_PORTAL_HASH];
static T36PortalRetry *t36_retry_slot(Market29PortalRecruit *m,int create) {
 unsigned k=(unsigned)((m->table>>4)^(m->rid*2654435761u))&(MARKET29_PORTAL_HASH-1);
 for(unsigned j=0;j<MARKET29_PORTAL_HASH;j++,k=(k+1)&(MARKET29_PORTAL_HASH-1)){
  T36PortalRetry *s=&t36_retry[k];
  if(s->table==m->table && s->rid==m->rid && s->franchise==market29_portal.scope.franchise_table && s->year==market29_portal.scope.year)return s;
  if(!s->table || s->franchise!=market29_portal.scope.franchise_table || s->year!=market29_portal.scope.year){
   if(!create)return 0;
   *s=(T36PortalRetry){.table=m->table,.rid=m->rid,.franchise=market29_portal.scope.franchise_table,.year=market29_portal.scope.year};return s;
  }
 }
 return 0;
}
static void t36_recovery_witness(Rec r,Rec team,U eval,int funded,int firm) {
 if(!market29_portal.ready || !market29_portal_scope(team,eval))return;
 unsigned t=scalar(team,383,0x100);Market29PortalRecruit *m=market29_portal_find(q((U)r+0x10),row(r));
 if(!m || t>=138 || !market29_has(m->recovery_primary,t))return;
 if(funded){T36PortalRetry *s=t36_retry_slot(m,1);if(s)market29_bit(s->failed,t,0);
  market29_bit(m->recovery_executed,t,1);}
 log_line("TEST36_PORTAL_RECOVERY,%u,%u,%u,%d,%d,POST_SCHOLARSHIP_AND_ACTION",market29_portal.scope.week,t,row(r),funded,firm);
}
static void market29_portal_begin(U eval) {
 if(mode!=1 || faulted || market29_portal.building || !eval)return;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 unsigned type=season?scalar(season,5,0x108):99,week=season?scalar(season,4,0x100):99;
 if(type==6 && week<=6){Rec pool=f?array_field(f,46):0;rc4_bind(f,season,pool,0);release(pool);}
 else rc4_inactive();
 release(season);
 if(type!=6 || (week<2 && !(week==1 && t36_active_eval==eval)) || week>6){release(f);return;}
 Rec list=f?array_field(f,46):0,teams=f?array_field(f,45):0;release(f);
 Rec probe=teams && array_count(teams)?array_at(teams,0):0;
 if(probe && market29_portal.ready && market29_portal_scope(probe,eval)){release(probe);release(list);release(teams);return;}
 uint64_t phase_start=test29_clock_ms();
 market29_portal.ready=0;market29_portal.building=1;market29_portal.eval=eval;market29_portal.count=0;
 memset(market29_portal.hash,0,sizeof(market29_portal.hash));
 Rec *held=0;Market29PortalSchool *schools=0;PortalChoice *pool=0;int retained=0,complete=0;
 if(!probe || !hs28_scope(&market29_portal.scope,probe,eval) || !list || !teams ||
   array_count(list)>MARKET29_PORTAL_LIMIT || array_count(teams)>138)goto done;
 U count=array_count(list),nt=array_count(teams);
 held=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*held));
 schools=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*sizeof(*schools));
 pool=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*pool));
 if(!held || !schools || !pool)goto done;
 for(U i=0;i<count;i++) {
  Rec r=array_at(list,i);if(!r)goto done;
  if(!transfer_recruit(r) || scalar(r,9,0x108)>=5){release(r);continue;}
  int owner=-1,published=portal_published_owner(r,eval,&owner);
  if(published<0){release(r);goto done;}if(published){release(r);continue;}
  Rec player=reference(r,5);if(!player){release(r);goto done;}
  int pos=(int)scalar(player,3,0x108),ovr=(int)scalar(player,191,0x100),stars=(int)scalar(player,231,0x108);release(player);
  if(pos<0 || pos>=22 || ovr<0 || ovr>99 || stars<0 || stars>4){release(r);goto done;}
  Market29PortalRecruit *m=&market29_portal.recruits[retained];memset(m,0,sizeof(*m));
  m->table=q((U)r+0x10);m->rid=row(r);
  T36PortalRetry *previous=t36_retry_slot(m,0);
  if(previous && previous->week+1==week)memcpy(m->recovery_attempted,previous->failed,sizeof(m->recovery_attempted));
  m->pos=pos;m->raw_ovr=ovr;m->stars=stars;held[retained++]=r;
  unsigned h=(unsigned)((m->table>>4)^((U)m->rid*2654435761u))&(MARKET29_PORTAL_HASH-1);
  while(market29_portal.hash[h])h=(h+1)&(MARKET29_PORTAL_HASH-1);
  market29_portal.hash[h]=++market29_portal.count;
 }
 for(U t=0;t<nt && !faulted;t++) {
  Rec team=array_at(teams,t);if(!team)goto done;
  unsigned tid=scalar(team,383,0x100);if(tid>=138){release(team);goto done;}
  Rec user=reference(team,407);int skip=user!=0;release(user);
  if(skip){release(team);continue;}
  Market29PortalSchool *s=&schools[tid];s->team=team;
  if(!market29_portal_existing(team,tid))goto done;
  Rec budget=reference(team,224);unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
  if(total>100000 || assigned>total)goto done;s->remaining=(int)(total-assigned);
  /* Zero total and zero assigned hours cannot support either an allocated
     productive offer or a new affordable proposal. */
  if(!total){release(s->team);s->team=0;continue;}
  if(!market29_portal_school_cache(team,eval,s->native_cache))goto done;
  int actual_valid=market29_allocations_valid(team,eval,assigned,total);if(actual_valid<0)goto done;
  for(int pos=0;pos<22;pos++) {
   PortalRoster room;if(!portal_roster(&room,team,eval,pos))goto done;
   s->rooms[pos]=room.projected;s->prestige=room.prestige;
  }
  int reserved[22]={0};test29_hs_reserved(team,eval,reserved);if(faulted)goto done;
  dri_portal_set_hs_reservations(s->rooms,reserved,s->prestige);
  Rec board=board_array(team);if(!board || array_count(board)>MAX_BOARD){release(board);goto done;}
  Rec targets[MAX_BOARD]={0};int target_count=0,fixed=0;
  for(U b=0;b<array_count(board);b++) {
   Rec target=array_at(board,b),r=target?reference(target,10):0;
   if(!target || !r){release(target);release(r);for(int z=0;z<target_count;z++)release(targets[z]);release(board);goto done;}
   if(!transfer_recruit(r) || scalar(r,9,0x108)>=5)fixed++;
   release(r);targets[target_count++]=target;
  }
  release(board);s->capacity=50-fixed;if(s->capacity<0)s->capacity=0;
  int n=0,ok=1;
  for(int i=0;i<retained && ok;i++) {
   Rec r=held[i];Market29PortalRecruit *m=&market29_portal.recruits[i];Rec target=0;
   if(portal_academy(tid) || !portal_academy_allowed(r,tid)){m->forecast_rejected[0]++;continue;}
   for(int b=0;b<target_count;b++) {
    Rec linked=reference(targets[b],10);int match=portal_same_record(linked,r);release(linked);
    if(match){target=targets[b];break;}
   }
   int projected=portal_projected_ovr(m->raw_ovr,portal_projection_step(eval));
   if(dri_quality_block_candidate(&s->rooms[m->pos],s->prestige,projected,m->raw_ovr)){m->forecast_rejected[1]++;continue;}
   if(!native_recruit_eligible(eval,team,r)){m->forecast_rejected[2]++;continue;}
   if(!target && s->remaining<hs28_recovery_cost(eval,0)){m->forecast_rejected[3]++;continue;}
   int base=0;
   if(!market29_portal_score_cached(r,team,eval,s->native_cache,&base)){ok=0;break;}
   m->forecast_board[tid]=(int16_t)(base>32767?32767:base< -32767?-32767:base);
   /* Preserve the legacy permission ledger's two columns. Both refer to the
      same source-correct native score; ownership does not change the source. */
   m->forecast_retained[tid]=m->forecast_board[tid];
   if(base<=0){m->forecast_rejected[4]++;continue;}
   int offered=target && scalar(target,12,0x108)>=2;
   if(!offered && m->stars>=3){
    int ask=0,paid=0,wallet=0;
    if(!r2_quote(eval,team,r,target,&ask,&paid,&wallet) || ask-paid>wallet){
     log_line("R2_FORECAST_NIL,%u,%u,%d,%d,%d,RELEASE_BEFORE_FREEZE",m->rid,tid,ask,paid,wallet);
     m->forecast_rejected[5]++;
     continue;
    }
   }
   int rank=0,bonus=dri_quality_bonus_candidate(&s->rooms[m->pos],s->prestige,projected,m->raw_ovr,&rank);
   PortalChoice *p=&pool[n];*p=(PortalChoice){.pos=m->pos,.score=base+bonus,.native_base=base,.ovr=projected,.raw_ovr=m->raw_ovr,
    .existing=target!=0,.rid=m->rid,.rec=(U)r,.index=-1};
   if(!portal_choice_race(p,target,team,eval)){ok=0;break;}if(!p->race_ok){m->forecast_rejected[6]++;continue;}
   p->role=dri_portal_role_candidate(&s->rooms[m->pos],s->prestige,projected,m->raw_ovr);
   m->pressure.fit[tid]=base;m->pressure.interest[tid]=p->influence;if(base>m->best_score)m->best_score=base;
   m->forecast_fit++;
   if(p->existing && p->offered) {
    market29_bit(m->pressure.protected_offer,tid,1);
    unsigned actions=0,hours=test31_productive_allocated(target,team,eval,&actions);
    if(actual_valid && hours>0 && hours<=assigned)market29_bit(m->pressure.active,tid,1);
   }
   n++;
  }
  for(int b=0;b<target_count;b++)release(targets[b]);if(!ok || faulted)goto done;
  s->n=n;s->pool=HeapAlloc(GetProcessHeap(),0,(n+1)*sizeof(*pool));if(!s->pool)goto done;
  memcpy(s->pool,pool,(size_t)n*sizeof(*pool));
  rc4_compare_team=(int)tid;rc4_compare_site="MARKET";
  dri_portal_order(s->pool,n,s->capacity,s->rooms,s->prestige);rc4_compare_site=0;
  for(int i=0;i<n;i++)if(s->pool[i].selected && s->pool[i].round==1) {
   Market29PortalRecruit *m=market29_portal_find(q(s->pool[i].rec+0x10),s->pool[i].rid);
   if(m && market29_has(m->pressure.active,tid) && test29r2_portal_committable((Rec)s->pool[i].rec,team,eval,"MARKET_FIRM"))market29_bit(m->pressure.firm,tid,1);
  }
  market29_portal_propose(s,eval);
 }
 if(faulted)goto done;
 market29_portal.reconciling=1;int proposals_changed=1,last_rejections=0;
 for(market29_portal.rounds=0;market29_portal.rounds<MARKET29_ROUNDS && !faulted;market29_portal.rounds++) {
  market29_portal_seed_recovery(schools,held);
  int changed=0;
  for(int i=0;i<retained;i++) {
   Market29PortalRecruit *m=&market29_portal.recruits[i];
   changed+=market29_reconcile(&m->pressure,1,m->rid,market29_portal.scope.year);
   changed+=market30_hard_reconcile(&m->pressure,1,m->rid,market29_portal.scope.year);
  }
  last_rejections=changed;if(!changed && !proposals_changed)break;
  for(int i=0;i<retained;i++)market29_save_plan(&market29_portal.recruits[i].pressure);
  for(unsigned t=0;t<138;t++)if(schools[t].team) {
   Market29PortalSchool *s=&schools[t];market29_portal_apply(s->pool,s->n,s->team,eval,s->rooms,s->prestige);
   dri_portal_order(s->pool,s->n,s->capacity,s->rooms,s->prestige);
  }
  for(int i=0;i<retained;i++){memset(market29_portal.recruits[i].pressure.planned,0,24);memset(market29_portal.recruits[i].pressure.primary,0,24);}
  for(unsigned t=0;t<138;t++)if(schools[t].team)market29_portal_propose(&schools[t],eval);
  proposals_changed=market29_portal_retire_failed_recovery();
  for(int i=0;i<retained;i++)proposals_changed+=market29_plan_changed(&market29_portal.recruits[i].pressure);
 }
 market29_portal.reconciling=0;
 for(int i=0;i<retained;i++)market30_hard_freeze(&market29_portal.recruits[i].pressure,1,market29_portal.recruits[i].rid,market29_portal.scope.year);
 log_line("PORTAL29_MARKET_RECONCILE,%d,%d,%d,BOUNDED_SYNCHRONOUS_FORECAST",market29_portal.rounds,market29_portal.count,!last_rejections && !proposals_changed);
 if(last_rejections || proposals_changed)log_line("PORTAL29_MARKET_BOUND,%d,%d,%d,NONCONVERGED_FORECAST_NOT_EXECUTION",market29_portal.rounds,last_rejections,proposals_changed);
 if(faulted)goto done;
 for(int i=0;i<retained;i++) {
  Market29PortalRecruit *m=&market29_portal.recruits[i];int primary=market29_portal_sum(m->pressure.primary,138),firm=market29_portal_sum(m->pressure.firm,138);
  log_line("R2H2_FORECAST,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,FIT_ACADEMY_ROOM_ELIGIBILITY_HOURS_SCORE_NIL_RACE",
   week,m->rid,m->stars+1,m->forecast_fit,m->forecast_rejected[0],m->forecast_rejected[1],m->forecast_rejected[2],
   m->forecast_rejected[3],m->forecast_rejected[4],m->forecast_rejected[5],m->forecast_rejected[6]);
  memset(m->recovery_primary,0,sizeof(m->recovery_primary));
  if(!firm)for(unsigned t=0;t<138;t++)if(!market29_has(m->recovery_attempted,t) && (m->stars>=3 || (portal_academy(t) && portal_academy_allowed(held[i],t))) && market29_has(m->pressure.primary,t))market29_bit(m->recovery_primary,t,1);
  if(m->stars>=3)for(unsigned t=0;t<138;t++)if(market29_has(m->pressure.hard_authorized,t))
   log_line("R2_PORTAL_PERMISSION,%u,%u,%u,%d,%d,%d,%d",week,m->rid,t,m->forecast_board[t],m->forecast_retained[t],
    market29_has(m->pressure.primary,t),market29_has(m->pressure.protected_offer,t));
  log_line("TEST30_MARKET_AUTH,PORTAL,%u,6,%d,%d,%d,%d,EXISTING_PLANNED_AUTHORIZED_REJECTED",m->rid,
   market30_count(m->pressure.protected_offer),market30_count(m->pressure.planned),
   market30_count(m->pressure.hard_authorized),market30_count(m->pressure.hard_rejected));
  log_line("PORTAL29_MARKET,%u,%d,%d,%d,%d,%d,%d,%d,ALLOCATED_OFFER_ROLE_AND_PLANNED_NOT_EXECUTED",m->rid,
   market29_portal_sum(m->pressure.active,138),firm,market29_portal_sum(m->pressure.planned,138),primary,market29_pressure(&m->pressure,138),market29_portal_sum(m->pressure.exception,138),market29_portal_sum(m->pressure.rejected,138));
 }
 log_line("TEST36_PORTAL_MARKET_POLICY,4,6,6,PRIMARY_TOTAL_SOFT_AND_HARD_CPU_AUTHORIZATION");
 complete=1;
done:
 if(schools){for(unsigned t=0;t<138;t++){release(schools[t].team);if(schools[t].pool)HeapFree(GetProcessHeap(),0,schools[t].pool);}HeapFree(GetProcessHeap(),0,schools);}
 for(int i=0;i<retained;i++)release(held[i]);if(held)HeapFree(GetProcessHeap(),0,held);if(pool)HeapFree(GetProcessHeap(),0,pool);
 release(probe);release(list);release(teams);market29_portal.building=0;market29_portal.reconciling=0;market29_portal.ready=complete;
 log_line("PORTAL29_SNAPSHOT,%d,%d,%d,FROZEN_BEFORE_TEAM_PASSES",complete,market29_portal.count,market29_portal.rounds);
 test29_phase_log("PORTAL_MARKET",255,eval,phase_start);
 if(!complete && !faulted)portal_fault("market29_portal_snapshot");
}
/* Only native board admission is requested. All legal/funding checks run
   again, and neither this request nor the forecast is logged as a paid offer. */
static void market29_portal_recovery(Rec team,U eval) {
 if(mode!=1 || faulted || !market29_portal.ready || !market29_portal_scope(team,eval))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 Rec f=context_ref(eval,6),list=f?array_field(f,46):0;release(f);if(!list)return;
 U n=array_count(list);if(n>MARKET29_PORTAL_LIMIT){release(list);portal_fault("market29_recovery_list");return;}
 U cache[67];if(!market29_portal_school_cache(team,eval,cache)){release(list);portal_fault("market29_recovery_cache");return;}
 for(U i=0;i<n && !faulted;i++) {
  Rec r=array_at(list,i);if(!r){portal_fault("market29_recovery_record");break;}
  Market29PortalRecruit *m=market29_portal_find(q((U)r+0x10),row(r));
  if(m && market29_has(m->recovery_primary,tid) && !market29_has(m->recovery_executed,tid) && !market29_has(m->recovery_attempted,tid) && scalar(r,9,0x108)<5) {
   T36PortalRetry *attempt=t36_retry_slot(m,1);
   if(attempt){if(attempt->week!=market29_portal.scope.week){memset(attempt->failed,0,sizeof(attempt->failed));attempt->week=market29_portal.scope.week;}market29_bit(attempt->failed,tid,1);}
   market29_bit(m->recovery_attempted,tid,1);
   int base=0;
   if(!market29_portal_score_cached(r,team,eval,cache,&base)){release(r);portal_fault("market29_recovery_score");break;}
   int result=base>0?portal29_admit(team,r,eval,base):0;
   if(base<=0) {
    market29_bit(m->recovery_primary,tid,0);market29_bit(m->recovery_attempted,tid,1);
    log_line("TEST31_PORTAL_RECOVERY_RETIRED,%u,%u,%d,%d,NATIVE_SCORE_UNEXECUTABLE",tid,row(r),base,base);
   }
   else if(!result)log_line("TEST31_PORTAL_RECOVERY_PENDING,%u,%u,NATIVE_BOARD_SELECTION_STILL_REQUIRED",tid,row(r));
   log_line("PORTAL29_RECOVERY_ADMISSION,%u,%u,%d,NATIVE_FUNDING_REQUIRED",tid,row(r),result);
  }
  release(r);
 }
 release(list);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestPortal30AdmissionBlock(Rec r,Rec t,U e){return market29_portal_admission_block(r,t,e);}
__declspec(dllexport) int DriTestMarket29PortalNativeScore(Rec r,Rec t,U eval,int *out){if(!hs28_scope(&market29_portal.scope,t,eval))return 0;return market29_portal_native_score(r,t,eval,out);}
__declspec(dllexport) void DriTestMarket29PortalBegin(U eval){market29_portal_begin(eval);}
__declspec(dllexport) void DriTestMarket29PortalReset(void){memset(&market29_portal,0,sizeof(market29_portal));}
__declspec(dllexport) int DriTestMarket29PortalFlags(Rec r,unsigned tid){
 Market29PortalRecruit *m=market29_portal_find(q((U)r+0x10),row(r));if(!m || tid>=138)return -1;
 return market29_has(m->pressure.planned,tid) | (market29_has(m->pressure.primary,tid)<<1) |
  (market29_has(m->pressure.rejected,tid)<<2) | (market29_has(m->recovery_primary,tid)<<3);
}
__declspec(dllexport) int DriTestMarket29PortalCount(Rec r,int kind){Market29PortalRecruit *m=market29_portal_find(q((U)r+0x10),row(r));return !m?-1:market29_portal_sum(kind==0?m->pressure.active:kind==1?m->pressure.firm:kind==2?m->pressure.planned:m->pressure.primary,138);}
#endif
