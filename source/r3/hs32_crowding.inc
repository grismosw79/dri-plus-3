/* Test29: frozen market pressure on NEW HS/JUCO pursuits only. Allocated
   offers and affordable planning intentions are separate evidence. Neither
   writes native candidate scores, hours, offers, influence or eligibility.
   All schools use the same pre-pass snapshot, including on a saved reload. */
#include "market29_core.h"
#include "competition32_policy.h"
#include "market33_policy.h"
#define HS32_LIMIT 12000
#define HS32_HASH 16384
typedef struct {
 U table;uint32_t rid;int active,firm,demand,primary,best_score,authorized,unseeded_seen;
 Market29Pressure pressure;uint64_t scarce_primary[3];
 Market33Cache cached;Market33Admissions admissions;
} Hs32Recruit;
static struct {int ready,building,reconciling,count,unseeded;U eval;Hs28TeamPlan scope;int hash[HS32_HASH];Hs32Recruit recruits[HS32_LIMIT];uint64_t completed[3],strict[3];} hs32_market;
static Hs32Recruit *hs32_find(U table,uint32_t rid) {
 unsigned h=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS32_HASH-1);
 for(int n=0;n<HS32_HASH;n++,h=(h+1)&(HS32_HASH-1)) {
  int k=hs32_market.hash[h];if(!k)return 0;
  Hs32Recruit *m=&hs32_market.recruits[k-1];if(m->table==table && m->rid==rid)return m;
 }return 0;
}
static void hs32_note_unseeded(Rec recruit) {
 if(!hs32_market.building)return;
 Hs32Recruit *m=hs32_find(q((U)recruit+0x10),row(recruit));
 if(!m || m->unseeded_seen)return;
 m->unseeded_seen=1;hs32_market.unseeded++;
 if(hs32_market.unseeded<=3)
  log_line("HS32_OPEN_SCHOOL_LIST,%u,%u,%u,%u,UNSEEDED",hs32_market.scope.year,
   hs32_market.scope.type,hs32_market.scope.week,m->rid);
}
/* A provisional permit belongs to a currently feasible nomination. Native
   actions still recheck actual capacity; unused/failed nominations release
   their place for schools whose native pass has not completed. */
static int hs33_scope_valid(Rec team,U eval) {
 if(!hs32_market.ready || hs32_market.building || hs32_market.eval!=eval)return 0;
 Hs28TeamPlan scope={0};if(!hs28_scope(&scope,team,eval))return 0;
 Hs28TeamPlan *old=&hs32_market.scope;
 return scope.franchise_table==old->franchise_table && scope.franchise_handle==old->franchise_handle &&
  scope.franchise_row==old->franchise_row && scope.year==old->year && scope.type==old->type && scope.week==old->week;
}
static void hs33_rank(Hs32Recruit *m) {
 market33_cache(&m->pressure,&m->cached,m->rid,hs32_market.scope.year);
 market33_select(&m->pressure,&m->admissions,&m->cached,hs32_market.completed,m->rid,hs32_market.scope.year);
 m->authorized=market30_count(m->pressure.hard_authorized);
 memcpy(m->pressure.admitted,m->pressure.hard_authorized,24);
 memcpy(m->pressure.backup_admitted,m->pressure.hard_authorized,24);
 memcpy(m->pressure.rejected,m->pressure.hard_rejected,24);
 memcpy(m->pressure.backup_rejected,m->pressure.hard_rejected,24);
}
static void hs33_plan_strict(Rec team,U eval,int strict) {
 if(!team || !hs33_scope_valid(team,eval))return;
 unsigned tid=scalar(team,383,0x100);if(tid<138)market29_bit(hs32_market.strict,tid,strict);
}
static int hs33_unreserved(Hs32Recruit *m,const Hs28Choice *p,unsigned tid) {
 if(market29_has(hs32_market.strict,tid))return 0;
 if(market29_has(m->admissions.failed,tid) || market29_has(hs32_market.completed,tid))return 0;
 int used=m->authorized;
 if(used>=hs33_limit_stars(m->admissions.stars))return 0;
 if(used<hs33_soft_stars(m->admissions.stars))return 1;
 return market29_has(m->admissions.need,tid) || market29_has(m->admissions.recovered,tid) ||
  market33_strong_fit(&m->pressure,&m->cached,tid) ||
  ((m->cached.earned || p->stage>=4) && market33_close(p,&m->pressure,&m->cached,tid));
}
static int hs32_authorization_block(Rec recruit,Rec team,U eval,int execution) {
 if(mode!=1 || faulted || !recruit || !team || !hs15_is_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 if(!hs33_scope_valid(team,eval))return execution;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 1;
 Hs32Recruit *m=hs32_find(q((U)recruit+0x10),row(recruit));if(!m)return 1;
 if(market29_has(m->pressure.hard_authorized,tid))return 0;
 if(execution && r2_late_recruit==recruit && r2_late_team==team)return 0;
 if(!execution && test36_hs_probe==recruit &&
    m->admissions.stars>=3 && m->admissions.stars<=4 && (!market29_has(hs32_market.completed,tid) || r2_late_recruit==recruit))return 0;
 if(!execution && !market29_has(m->admissions.failed,tid) && !market29_has(hs32_market.completed,tid) &&
   m->authorized<hs33_soft_stars(m->admissions.stars))return 0;
 return 1;
}
static int hs32_admission_block(Rec recruit,Rec team,U eval) {return hs32_authorization_block(recruit,team,eval,1);}
static int hs32_recovery_block(Rec recruit,Rec team,U eval) {return hs32_authorization_block(recruit,team,eval,0);}
static int test36_hs_grant(Rec recruit,Rec team,U eval,int score,const uint64_t live[3]) {
 if(!hs33_scope_valid(team,eval))return 1;
 Hs32Recruit *m=hs32_find(q((U)recruit+0x10),row(recruit));unsigned t=scalar(team,383,0x100);
 if(!m || t>=138)return 0;
 if(market29_has(m->pressure.hard_authorized,t))return 1;
 if(m->admissions.stars<3 || m->admissions.stars>4)
  return !hs32_recovery_block(recruit,team,eval);
 if(market29_has(hs32_market.completed,t) && r2_late_recruit!=recruit)return 0;
 /* The capture has no live firm destination. A real recovery beats an
    unfilled speculative permission, never an existing scholarship. */
 if(score<=0 || !live)return 0;
 unsigned used=0;
 for(int w=0;w<3;w++){
  m->pressure.protected_offer[w]&=live[w];
  uint64_t filled=live[w]|(m->admissions.inflight[w]&m->admissions.eligible[w]&~m->admissions.failed[w]&~hs32_market.completed[w]);
  if((unsigned)w==t/64)filled&=~(UINT64_C(1)<<(t%64));
  while(filled){used++;filled&=filled-1;}
 }
 if(used>=(unsigned)hs33_limit_stars(m->admissions.stars))return 0;
 m->pressure.fit[t]=score;
 market29_bit(m->admissions.failed,t,0);market29_bit(m->admissions.recovered,t,1);
 market29_bit(m->admissions.eligible,t,1);market29_bit(m->pressure.planned,t,1);
 market29_bit(m->pressure.primary,t,1);market29_bit(m->admissions.inflight,t,1);
 hs33_rank(m);
 int allowed=market29_has(m->pressure.hard_authorized,t) || (r2_late_recruit==recruit && r2_late_team==team);
 log_line("TEST36_HS_REOPEN,%u,%u,%d,ABANDONED_FIRM_POSITION_COST_RECHECK",t,row(recruit),allowed);
 return allowed;
}
static void hs33_release_offer(Rec recruit,Rec team,U eval) {
 if(!recruit || !team || !hs33_scope_valid(team,eval))return;
 unsigned tid=scalar(team,383,0x100);Hs32Recruit *m=hs32_find(q((U)recruit+0x10),row(recruit));
 if(tid>=138 || !m || market29_has(m->pressure.protected_offer,tid))return;
 market29_bit(m->admissions.failed,tid,1);market29_bit(m->admissions.eligible,tid,0);market29_bit(m->admissions.inflight,tid,0);
 market29_bit(m->pressure.planned,tid,0);market29_bit(m->pressure.primary,tid,0);hs33_rank(m);
 log_line("TEST33_HS_PERMIT_RELEASE,%u,%u,FAILED_BEFORE_PAYMENT",tid,m->rid);
}
static int hs33_plan_published(const Hs28Choice *pool,int n,Rec team,U eval) {
 if(!pool || !team || !hs33_scope_valid(team,eval))return 0;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 unsigned char selected[HS32_LIMIT],updated[HS32_LIMIT];memset(selected,0,sizeof(selected));memset(updated,0,sizeof(updated));
 for(int j=0;j<n;j++)if(pool[j].selected && pool[j].legal && pool[j].viable && pool[j].native_score>0) {
  const Hs28Choice *p=&pool[j];Hs32Recruit *m=hs32_find(p->table,p->rid);if(!m)continue;
  int k=(int)(m-hs32_market.recruits);selected[k]=(unsigned char)p->choice;
  /* Refresh only this selected native nomination. A recruit with a formerly
     zero native fit can now compete; ineligible paths cannot become legal. */
  int gain=p->offered && p->target?(int32_t)scalar((Rec)p->target,7,0x100):0;
  if(m->pressure.fit[tid]!=p->native_score || m->pressure.interest[tid]!=p->points || m->pressure.gain[tid]!=gain) {
   m->pressure.fit[tid]=p->native_score;m->pressure.interest[tid]=p->points;m->pressure.gain[tid]=gain;updated[k]=1;
  }
 }
 /* O(pool+recruits), not their product. Only changed nominations rerank. */
 for(int i=0;i<hs32_market.count;i++) {
  Hs32Recruit *m=&hs32_market.recruits[i];int was=market29_has(m->admissions.eligible,tid),role=selected[i];
  if(market29_has(m->admissions.failed,tid))role=0;
  int wanted=role!=0,primary=role==1;
  int lock=wanted && market29_has(m->pressure.hard_authorized,tid);
  market29_bit(m->admissions.inflight,tid,lock);
  if(!updated[i] && was==wanted && market29_has(m->pressure.primary,tid)==primary)continue;
  market29_bit(m->admissions.eligible,tid,wanted);market29_bit(m->pressure.planned,tid,wanted);
  market29_bit(m->pressure.primary,tid,primary);hs33_rank(m);
 }
 int denied=0;for(int j=0;j<n;j++)if(pool[j].selected && !pool[j].offered) {
  Hs32Recruit *m=hs32_find(pool[j].table,pool[j].rid);
  if(!m || !market29_has(m->pressure.hard_authorized,tid))denied++;
 }
 return denied;
}
/* Release forecast choices absent or unusable in the actual native pool
   before current selection. A positive-fit unselected candidate never owns a
   provisional slot merely by being present in this pool. */
static void hs33_prepare_native_pool(const Hs28Choice *pool,int n,Rec team,U eval) {
 if(!pool || !team || !hs33_scope_valid(team,eval))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 unsigned char present[HS32_LIMIT];memset(present,0,sizeof(present));
 for(int j=0;j<n;j++)if(pool[j].legal && pool[j].viable && pool[j].native_score>0) {
  Hs32Recruit *m=hs32_find(pool[j].table,pool[j].rid);if(m)present[m-hs32_market.recruits]=1;
 }
 for(int i=0;i<hs32_market.count;i++) {
  Hs32Recruit *m=&hs32_market.recruits[i];
  if(present[i] || !market29_has(m->admissions.eligible,tid) || market29_has(m->pressure.protected_offer,tid))continue;
  market29_bit(m->admissions.eligible,tid,0);market29_bit(m->admissions.inflight,tid,0);market29_bit(m->pressure.planned,tid,0);market29_bit(m->pressure.primary,tid,0);hs33_rank(m);
 }
}
static void hs33_update_school(Rec team,U eval,int finished) {
 if(!team)return;Rec user=reference(team,407);int human=user!=0;release(user);if(human)return;
 if(!team || !hs33_scope_valid(team,eval))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138 || market29_has(hs32_market.completed,tid))return;
 unsigned char observed[HS32_LIMIT];memset(observed,0,sizeof(observed));Rec board=board_array(team);if(!board)return;
 U count=array_count(board);if(count>MAX_BOARD){release(board);return;}
 for(U j=0;j<count;j++) {
  Rec target=array_at(board,j),recruit=target?reference(target,10):0;
  if(!target || !recruit){release(recruit);release(target);release(board);return;}
  unsigned status=scalar(target,12,0x108);
  if(hs15_is_recruit(recruit) && status>=2 && status<=4) {
   Hs32Recruit *m=hs32_find(q((U)recruit+0x10),row(recruit));if(m)observed[m-hs32_market.recruits]=1;
  }release(recruit);release(target);
 }release(board);
 if(finished)market29_bit(hs32_market.completed,tid,1);int released=0;
 for(int i=0;i<hs32_market.count;i++) {
  Hs32Recruit *m=&hs32_market.recruits[i];int eligible=market29_has(m->admissions.eligible,tid),old=market29_has(m->pressure.protected_offer,tid);
  if(!eligible && !old && !observed[i])continue;
  if(!finished && old==observed[i])continue;
  if(finished && eligible && !observed[i])released++;
  market29_bit(m->pressure.protected_offer,tid,observed[i]);
  if(finished || observed[i])market29_bit(m->admissions.inflight,tid,0);
  if(!observed[i]) {
   market29_bit(m->pressure.active,tid,0);market29_bit(m->pressure.firm,tid,0);m->pressure.gain[tid]=0;
   if(finished){market29_bit(m->admissions.eligible,tid,0);market29_bit(m->pressure.planned,tid,0);market29_bit(m->pressure.primary,tid,0);}
  }
  hs33_rank(m);
 }if(finished)log_line("TEST33_HS_SCHOOL_RELEASE,%u,%d,UNEXECUTED_TO_UNPROCESSED_SCHOOLS",tid,released);
}
static void hs33_refresh_school(Rec team,U eval){hs33_update_school(team,eval,0);}
static void hs33_release_school(Rec team,U eval){hs33_update_school(team,eval,1);}
static void hs32_crowding_apply(Hs28Choice *pool,int n,Rec team,U eval,const Hs15Snapshot *snapshot) {
 for(int i=0;i<n;i++) {
  pool[i].crowd_penalty=pool[i].backup_penalty=pool[i].market_known=0;
  pool[i].market_roles=pool[i].market_planned=pool[i].market_primary=pool[i].market_protected=0;
 }
 if((!hs32_market.ready && !hs32_market.reconciling) ||
    (hs32_market.building && !hs32_market.reconciling) || hs32_market.eval!=eval)return;
 Hs28TeamPlan scope={0};if(!hs28_scope(&scope,team,eval))return;
 if(scope.franchise_table!=hs32_market.scope.franchise_table || scope.franchise_handle!=hs32_market.scope.franchise_handle ||
    scope.franchise_row!=hs32_market.scope.franchise_row || scope.year!=hs32_market.scope.year ||
    scope.type!=hs32_market.scope.type || scope.week!=hs32_market.scope.week)return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 for(int i=0;i<n;i++) {
  Hs28Choice *p=&pool[i];if(p->final || !p->legal || !p->viable || p->native_score<=0)continue;
  Hs32Recruit *m=hs32_find(p->table,p->rid);if(!m)continue;
  p->market_roles=1;
  p->market_protected=market29_has(m->pressure.protected_offer,tid);
  /* Actual existing scholarships consume the hard ceiling; speculative
     standby authorizations do not. A full snapshot may defer a new school
     until the next safe plan, while the native action checks live capacity. */
  if(!p->market_protected && (m->cached.offers>=hs33_limit_stars(p->stars) ||
     (!hs32_market.building && (market29_has(m->admissions.failed,tid) ||
      (!market29_has(m->pressure.hard_authorized,tid) && !hs33_unreserved(m,p,tid)))))) {
   p->market_known=1;p->market_planned=0;p->market_primary=0;
   p->crowd_penalty=p->backup_penalty=100;continue;
  }
  /* Do not freeze the old forecast role into a second admission barrier.
     Current native shortlists and loss/win changes can produce a better
     feasible primary than the prepass. Selection/funding still use one plan. */
  p->market_known=!hs32_market.building;
  p->market_planned=p->market_primary=1; /* membership permits either current role */
  int count=market33_primary_pressure(&m->pressure,&m->cached,tid);
  int rejected=market29_has(m->pressure.rejected,tid);
  p->backup_penalty=p->market_protected?0:market29_has(m->pressure.backup_rejected,tid)?45:
   market29_has(m->pressure.backup_admitted,tid)?0:hs33_backup_penalty_stars(p->stars,market33_attention(&m->pressure,&m->cached,tid));
  /* Native fit, credible interest/progress and acute quantity need preserve
     genuine competition. Displayed offers alone no longer freeze primaries. */
  int strong_fit=market33_strong_choice(p,&m->pressure,&m->cached,tid,m->rid,(unsigned)hs32_market.scope.year);
  int competitive=market33_close(p,&m->pressure,&m->cached,tid);
  int need=snapshot->ideal[p->pos]>0 && snapshot->count[p->pos]*2<snapshot->ideal[p->pos];
  int preserve=strong_fit || competitive || need || hs28_coverage_recovered((Rec)p->rec,team,eval);
  if(preserve){market29_bit(m->pressure.exception,tid,1);continue;}
  int ranked_better=p->stars>=3 && p->stage<4 && count>=3?competition32_better(&m->pressure,tid,p->rid,hs32_market.scope.year):0;
  int penalty=rejected?45:hs33_penalty_stars(p->stars,count),opportunity=0,alternative=0;
  int may_opportunity=p->stars>=3 && p->stage<4 && count>=3 && ranked_better>=3;
  int may_alternative=!p->market_protected && penalty && (rejected || !market29_has(m->pressure.admitted,tid));
  if(!may_opportunity && !may_alternative)continue;
  for(int j=0;j<n && ((may_opportunity && opportunity<25) || (may_alternative && !alternative));j++)if(j!=i && pool[j].legal && pool[j].viable && pool[j].pos==p->pos &&
    pool[j].native_score>0 && pool[j].ovr>=p->ovr-3 && (int64_t)pool[j].native_score*100>=(int64_t)p->native_score*70) {
   Hs32Recruit *other=hs32_find(pool[j].table,pool[j].rid);if(!other)continue;
   if(!market29_has(other->pressure.protected_offer,tid) &&
      other->cached.offers>=hs33_limit_stars(pool[j].stars))continue;
   int pressure=market33_primary_pressure(&other->pressure,&other->cached,tid);
   int value=may_opportunity?competition32_opportunity(p,&pool[j],count,pressure,ranked_better,0):0;
   if(value>opportunity)opportunity=value;
   if(!market29_has(other->pressure.rejected,tid) &&
      ((pressure<count && hs33_penalty_stars(p->stars,pressure)<penalty) || market29_has(other->pressure.admitted,tid)))alternative=1;
  }
  /* Existing races are only reassessed for the demonstrated elite
     opportunity-cost case; the generic soft thresholds still apply to new
     attention. Keep the former primary usable as a conditional fallback. */
  if(opportunity)p->crowd_penalty=opportunity;
  else if(!p->market_protected && alternative && penalty &&
    (rejected || !market29_has(m->pressure.admitted,tid)))p->crowd_penalty=penalty;
 }
}
typedef struct {
 Rec team;Hs28Choice *pool;Hs15Snapshot snapshot;
 Rec held_targets[MAX_BOARD],held_recruits[MAX_BOARD];int held;
 int n,fixed,general,remaining,vacancy[22],optional[22];
} Hs32SchoolForecast;
/* A forecast spans several synchronous rounds. Keep record wrappers alive
   throughout those rounds; only the published frozen result is numeric. */
static int hs32_board(Hs32SchoolForecast *s,U eval,const Hs15Snapshot *shot,Hs28Choice *pool,int *n,int *fixed) {
 *n=0;*fixed=0;Rec board=board_array(s->team);
 if(!board){log_line("HS32_BOARD_INPUT_FAIL,%u,-1,BOARD",scalar(s->team,383,0x100));return 0;}
 U count=array_count(board);if(count>MAX_BOARD){
  log_line("HS32_BOARD_INPUT_FAIL,%u,%llu,COUNT",scalar(s->team,383,0x100),(unsigned long long)count);
  release(board);return 0;
 }
 for(U j=0;j<count;j++) {
  Rec target=array_at(board,j),rec=target?reference(target,10):0;
  if(!target || !rec){
   log_line("HS32_BOARD_INPUT_FAIL,%u,%llu,%s",scalar(s->team,383,0x100),(unsigned long long)j,target?"TARGET_RECRUIT":"TARGET");
   release(target);release(rec);release(board);return 0;
  }
  Rec player=hs15_is_recruit(rec)?reference(rec,5):0;int acquired=player?hs15_has(shot,player):0;release(player);
  if(hs15_is_recruit(rec) && scalar(rec,9,0x108)<5 && !acquired) {
   if(!hs28_choice(&pool[*n],rec,target,s->team,eval,shot,0)){release(target);release(rec);release(board);return 0;}
   s->held_targets[s->held]=target;s->held_recruits[s->held++]=rec;(*n)++;
  }else {(*fixed)++;release(target);release(rec);}
 }
 release(board);return 1;
}
/* Validate productive allocation flags against the complete native board,
   including the other recruiting source. Inconsistent old flags are not
   evidence of current allocated attention; affordable forecasts stay separate. */
static int market29_allocations_valid(Rec team,U eval,unsigned assigned,unsigned total) {
 Rec board=board_array(team);if(!board)return -1;
 U count=array_count(board);if(count>MAX_BOARD){release(board);return -1;}
 uint64_t allocated=0;
 for(U i=0;i<count;i++) {
  Rec target=array_at(board,i);if(!target){release(board);return -1;}
  unsigned mask=0;allocated+=test31_productive_allocated(target,team,eval,&mask);release(target);
 }
 release(board);
 int valid=assigned<=total && allocated<=assigned;
 if(!valid)log_line("MARKET29_ALLOCATION_COUNTER_MISMATCH,%u,%llu,%u,%u,EXCLUDED_FROM_ACTUAL_PRESSURE",
  scalar(team,383,0x100),(unsigned long long)allocated,assigned,total);
 return valid;
}
static void hs32_counts(void) {
 for(int i=0;i<hs32_market.count;i++) {
  Hs32Recruit *m=&hs32_market.recruits[i];m->active=m->firm=m->demand=m->primary=0;
  for(unsigned t=0;t<138;t++){m->active+=market29_has(m->pressure.active,t);m->firm+=market29_has(m->pressure.firm,t);
   m->demand+=market29_has(m->pressure.planned,t);m->primary+=market29_has(m->pressure.primary,t);}
  market33_cache(&m->pressure,&m->cached,m->rid,hs32_market.scope.year);
 }
}
static void hs32_propose(Hs32SchoolForecast *s,U eval) {
 unsigned tid=scalar(s->team,383,0x100);int remaining=s->remaining;
 for(int order=1;order<=50;order++)for(int j=0;j<s->n;j++)if(s->pool[j].selected && s->pool[j].order==order) {
  Hs28Choice *p=&s->pool[j];Hs32Recruit *m=hs32_find(p->table,p->rid);if(!m)continue;
  int active=market29_has(m->pressure.active,tid),spend=hs28_recovery_cost(eval,p->offered);
  if(!active && (spend<=0 || remaining<spend))continue;
  if(!active)remaining-=spend;
  market29_bit(m->pressure.planned,tid,1);if(p->choice==1) {
   market29_bit(m->pressure.primary,tid,1);
   if(p->crowd_penalty>=45){market29_bit(m->scarce_primary,tid,1);market29_bit(m->pressure.exception,tid,1);}
  }
 }
}
static void hs32_crowding_begin(U eval) {
 if(mode!=1 || faulted || hs32_market.building || !hs28_pair_window(eval))return;
 uint64_t phase_start=test29_clock_ms();
 hs32_market.ready=0;hs32_market.building=1;hs32_market.count=0;hs32_market.unseeded=0;hs32_market.eval=eval;
 memset(hs32_market.hash,0,sizeof(hs32_market.hash));memset(hs32_market.completed,0,sizeof(hs32_market.completed));memset(hs32_market.strict,0,sizeof(hs32_market.strict));
 Rec f=context_ref(eval,6),list=f?array_field(f,39):0,teams=f?array_field(f,45):0;release(f);
 Rec *recruits=0;Hs28Choice *pool=0;Hs32SchoolForecast *schools=0;int retained=0,complete=0;
 const char *failure=0;int team_id=-1,slot=-1,recruit_row=-1,have_scope=0;
 if(!list || !teams){failure="MARKET_COLLECTION";goto done;}
 if(array_count(list)>HS32_LIMIT || array_count(teams)>138){failure="COLLECTION_SIZE";goto done;}
 U count=array_count(list),nt=array_count(teams);
 recruits=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*recruits));
 pool=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+MAX_BOARD+1)*sizeof(*pool));
 schools=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*sizeof(*schools));
 if(!recruits || !pool || !schools){failure="MARKET_ALLOCATION";goto done;}
 for(U i=0;i<count;i++) {
  Rec r=array_at(list,i);if(!r){failure="RECRUIT_RECORD";slot=(int)i;goto done;}
  if(!hs15_is_recruit(r) || scalar(r,9,0x108)>=5){release(r);continue;}
  recruits[retained++]=r;Hs32Recruit *m=&hs32_market.recruits[hs32_market.count];memset(m,0,sizeof(*m));
  m->table=q((U)r+0x10);m->rid=row(r);
  Rec player=reference(r,5);m->admissions.stars=player?(int)scalar(player,231,0x108):0;release(player);
  m->admissions.stage=(int)scalar(r,9,0x108);
  unsigned h=(unsigned)((m->table>>4)^((U)m->rid*2654435761u))&(HS32_HASH-1);
  while(hs32_market.hash[h])h=(h+1)&(HS32_HASH-1);
  hs32_market.hash[h]=++hs32_market.count;
 }
 for(U t=0;t<nt && !faulted;t++) {
  Rec team=array_at(teams,t);if(!team){failure="TEAM_RECORD";slot=(int)t;team_id=-1;goto done;}
  unsigned tid=scalar(team,383,0x100);team_id=(int)tid;
  if(tid>=138){failure="TEAM_ID";slot=(int)t;release(team);goto done;}
  if(t==0) {
   have_scope=hs28_scope(&hs32_market.scope,team,eval);
   if(!have_scope){failure="SEASON_SCOPE";release(team);goto done;}
  }
  Rec user=reference(team,407);if(user){release(user);release(team);continue;}
  Hs32SchoolForecast *s=&schools[tid];s->team=team;
  Hs15Snapshot shot;int n=0,fixed=0,vacancy[22],optional[22];
  if(!hs15_collect(&shot,team,eval)){failure="TEAM_ROSTER";goto done;}
  if(!hs32_board(s,eval,&shot,pool,&n,&fixed)){failure="TEAM_BOARD";goto done;}
  Rec budget=reference(team,224);unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
  if(total>100000 || assigned>total){
   failure="TEAM_BUDGET";log_line("HS32_MARKET_BUDGET,%u,%u,%u",tid,assigned,total);goto done;
  }
  int actual_valid=market29_allocations_valid(team,eval,assigned,total);if(actual_valid<0){failure="BOARD_ALLOCATIONS";goto done;}
  int general=hs28_rooms(&shot,vacancy,optional);if(general<0){failure="POSITION_ROOMS";goto done;}
  hs28_plan_team(pool,n,vacancy,optional,general,50-fixed,team,eval,&shot);
  /* Reconstruct actual attention from saved/native assigned actions, not
     TotalScholarshipOffers or historical interest. */
  for(int j=0;j<n;j++)if(actual_valid && pool[j].selected && pool[j].legal && pool[j].viable && pool[j].offered) {
   unsigned mask=0,hours=test31_productive_allocated((Rec)pool[j].target,team,eval,&mask);
   Hs32Recruit *m=hs32_find(pool[j].table,pool[j].rid);
   if(m && hours>0 && hours<=assigned) {
    int state=hs29_offer_state((Rec)pool[j].rec,team,eval);
    if(state>0)hs28_market_bit(m->pressure.active,tid,1);
    if(state==2)hs28_market_bit(m->pressure.firm,tid,1);
   }
  }
  /* Forecast only legal, positively scored, credible choices with an
     affordable native scholarship + productive action. It is demand, not
     an executed offer; actual admission still belongs to the native pass. */
  int cost=hs28_recovery_cost(eval,0),remaining=(int)total-(int)assigned;
  if(cost>0 && remaining>=cost)for(int j=0;j<retained;j++) {
   Rec r=recruits[j];int exists=0;for(int k=0;k<n;k++)if(hs28_identity(&pool[k],r)){exists=1;break;}
   if(exists)continue;
   int score=0;if(!hs28_native_score(r,team,eval,&score)){
    failure="CANDIDATE_SCORE";slot=j;recruit_row=(int)row(r);goto done;
   }
   if(score<=0)continue;
   if(!hs28_choice(&pool[n],r,0,team,eval,&shot,0)){
    failure="CANDIDATE_CHOICE";slot=j;recruit_row=(int)row(r);goto done;
   }
   pool[n].native_score=score;if(pool[n].legal && pool[n].viable)n++;
  }
  hs28_plan_team(pool,n,vacancy,optional,general,50-fixed,team,eval,&shot);
  s->pool=HeapAlloc(GetProcessHeap(),0,(n+1)*sizeof(*pool));if(!s->pool){failure="TEAM_ALLOCATION";goto done;}
  memcpy(s->pool,pool,(size_t)n*sizeof(*pool));s->snapshot=shot;s->n=n;s->fixed=fixed;s->general=general;s->remaining=remaining;s->team=team;
  memcpy(s->vacancy,vacancy,sizeof(vacancy));memcpy(s->optional,optional,sizeof(optional));
  for(int j=0;j<n;j++) {
   Hs32Recruit *m=hs32_find(pool[j].table,pool[j].rid);if(!m)continue;
   m->pressure.fit[tid]=pool[j].legal && pool[j].viable?pool[j].native_score:0;
   m->pressure.interest[tid]=pool[j].points;
   market29_bit(m->admissions.need,tid,shot.ideal[pool[j].pos]>0 && shot.count[pool[j].pos]*2<shot.ideal[pool[j].pos]);
   if(hs28_coverage_recovered((Rec)pool[j].rec,team,eval))market29_bit(m->admissions.recovered,tid,1);
   if(pool[j].target && pool[j].offered && pool[j].legal && pool[j].viable)
    m->pressure.gain[tid]=(int32_t)scalar((Rec)pool[j].target,7,0x100);
   if(pool[j].existing && pool[j].offered)market29_bit(m->pressure.protected_offer,tid,1);
   if(m->pressure.fit[tid]>m->best_score)m->best_score=m->pressure.fit[tid];
   /* The recruit list retains these wrappers throughout all forecast rounds. */
   int k=(int)(m-hs32_market.recruits);s->pool[j].rec=(U)recruits[k];
  }
  hs32_propose(s,eval);
 }
 if(faulted)goto done;
 hs32_counts();
 hs32_market.reconciling=1;
 int rounds=0,proposals_changed=1,last_rejections=0;
 for(;rounds<MARKET29_ROUNDS && !faulted;rounds++) {
  int changed=0;
  for(int i=0;i<hs32_market.count;i++) {
   Hs32Recruit *m=&hs32_market.recruits[i];
   uint64_t old[3];memcpy(old,m->pressure.hard_authorized,24);
   memcpy(m->admissions.eligible,m->pressure.planned,24);hs33_rank(m);
   changed+=memcmp(old,m->pressure.hard_authorized,24)!=0;
  }
  last_rejections=changed;if(!changed && !proposals_changed)break;
  for(int i=0;i<hs32_market.count;i++)market29_save_plan(&hs32_market.recruits[i].pressure);
  /* Every school plans against the same completed previous-round demand. */
  for(unsigned t=0;t<138;t++)if(schools[t].team) {
   Hs32SchoolForecast *s=&schools[t];
   hs28_plan_team(s->pool,s->n,s->vacancy,s->optional,s->general,50-s->fixed,s->team,eval,&s->snapshot);
  }
  for(int i=0;i<hs32_market.count;i++){memset(hs32_market.recruits[i].pressure.planned,0,24);memset(hs32_market.recruits[i].pressure.primary,0,24);memset(hs32_market.recruits[i].scarce_primary,0,24);}
  for(unsigned t=0;t<138;t++)if(schools[t].team)hs32_propose(&schools[t],eval);
  hs32_counts();proposals_changed=0;
  for(int i=0;i<hs32_market.count;i++)proposals_changed+=market29_plan_changed(&hs32_market.recruits[i].pressure);
 }
 hs32_market.reconciling=0;
 if(faulted)goto done;
 for(int i=0;i<hs32_market.count;i++) {Hs32Recruit *m=&hs32_market.recruits[i];memcpy(m->admissions.eligible,m->pressure.planned,24);hs33_rank(m);}
 hs32_counts();
 log_line("HS29_MARKET_RECONCILE,%d,%d,%d,BOUNDED_SYNCHRONOUS_FORECAST",rounds,hs32_market.count,!last_rejections && !proposals_changed);
 if(last_rejections || proposals_changed)log_line("HS29_MARKET_BOUND,%d,%d,%d,NONCONVERGED_FORECAST_NOT_EXECUTION",rounds,last_rejections,proposals_changed);
 for(int i=0;i<hs32_market.count;i++) {
  Hs32Recruit *m=&hs32_market.recruits[i];
  if(m->active || m->demand || market30_count(m->pressure.protected_offer)) {
   log_line("TEST30_MARKET_AUTH,HS,%u,%d,%d,%d,%d,%d,EXISTING_PLANNED_AUTHORIZED_REJECTED",
    m->rid,hs33_limit_stars(m->admissions.stars),market30_count(m->pressure.protected_offer),m->demand,
    market30_count(m->pressure.hard_authorized),market30_count(m->pressure.hard_rejected));
   log_line("HS32_MARKET,%u,%d,%d,%d,%d",m->rid,m->active,m->firm,m->demand,m->primary);
   int exceptions=0,rejected=0,scarcity=0;for(unsigned t=0;t<138;t++){exceptions+=market29_has(m->pressure.exception,t);rejected+=market29_has(m->pressure.rejected,t);scarcity+=market29_has(m->scarce_primary,t);}
   if(scarcity)log_line("HS29_MARKET_SCARCITY,%u,%d,QUANTITY_PRIMARY_AFTER_ALTERNATIVE_CONSUMED",m->rid,scarcity);
   log_line("HS29_MARKET_EVIDENCE,%u,%d,%d,%d,%d,%d,%d,%d,ALLOCATED_OFFER_ROLE_NOT_EXECUTION",m->rid,m->active,m->firm,m->demand,m->primary,market29_pressure(&m->pressure,138),exceptions,rejected);
  }
 }
 log_line("TEST33_HS_MARKET_POLICY,ELITE_8_12,LOWER_3_6,ONE_STAR_8_16,RANKED_PROVISIONAL_ACTUAL_CPU_OFFERS");
 complete=1;
done:
 if(!complete)log_line("HS32_MARKET_INPUT_FAIL,%d,%d,%d,%s,%d,%d,%d",
  have_scope?(int)hs32_market.scope.year:-1,have_scope?(int)hs32_market.scope.type:-1,
  have_scope?(int)hs32_market.scope.week:-1,failure?failure:"NESTED_RUNTIME_FAULT",team_id,slot,recruit_row);
 if(hs32_market.unseeded)log_line("HS32_OPEN_SCHOOL_LIST_TOTAL,%d",hs32_market.unseeded);
 if(schools){for(unsigned t=0;t<138;t++){for(int j=0;j<schools[t].held;j++){release(schools[t].held_targets[j]);release(schools[t].held_recruits[j]);}release(schools[t].team);if(schools[t].pool)HeapFree(GetProcessHeap(),0,schools[t].pool);}HeapFree(GetProcessHeap(),0,schools);}
 for(int i=0;i<retained;i++)release(recruits[i]);
 if(recruits)HeapFree(GetProcessHeap(),0,recruits);if(pool)HeapFree(GetProcessHeap(),0,pool);
 release(teams);release(list);hs32_market.building=0;hs32_market.reconciling=0;hs32_market.ready=complete;
 log_line("HS32_SNAPSHOT,%d,%d,FROZEN_BEFORE_TEAM_PASSES",complete,hs32_market.count);
 test29_phase_log("HS_MARKET",255,eval,phase_start);
 if(!complete && !faulted)portal_fault("HS30_market_snapshot");
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestHs30AdmissionBlock(Rec r,Rec t,U e){return hs32_admission_block(r,t,e);}
__declspec(dllexport) int DriTestHs30Authorized(Rec r,unsigned tid){Hs32Recruit *m=hs32_find(q((U)r+0x10),row(r));return m?market29_has(m->pressure.hard_authorized,tid):-1;}
__declspec(dllexport) void DriTestHs33ReleaseOffer(Rec r,Rec t,U e){hs33_release_offer(r,t,e);}
__declspec(dllexport) void DriTestHs33ReleaseSchool(Rec t,U e){hs33_release_school(t,e);}
__declspec(dllexport) int DriTestHs33PlanPublished(const Hs28Choice *p,int n,Rec t,U e){return hs33_plan_published(p,n,t,e);}
__declspec(dllexport) void DriTestHs32Begin(U eval){hs32_crowding_begin(eval);}
__declspec(dllexport) void DriTestHs32Reset(void){memset(&hs32_market,0,sizeof(hs32_market));}
__declspec(dllexport) int DriTestHs32Count(Rec r,int kind){Hs32Recruit *m=hs32_find(q((U)r+0x10),row(r));return !m?-1:kind==0?m->active:kind==1?m->firm:kind==2?m->demand:m->primary;}
__declspec(dllexport) int DriTestHs32Penalty(Rec r,Rec t,U eval){Hs15Snapshot s;Hs28Choice p[MAX_BOARD];int n=0;if(!hs15_collect(&s,t,eval)||!hs28_make_board_plan(t,eval,&s,p,&n,0))return -1;for(int i=0;i<n;i++)if(hs28_identity(&p[i],r))return p[i].crowd_penalty;return -1;}
__declspec(dllexport) int DriTestHs32Pick(Rec a,Rec b,Rec t,U eval) {
 Hs15Snapshot s;Hs28Choice p[2];int vacancy[22],optional[22];
 if(!hs15_collect(&s,t,eval)||!hs28_choice(&p[0],a,0,t,eval,&s,0)||!hs28_choice(&p[1],b,0,t,eval,&s,0))return -1;
 if(!hs28_native_score(a,t,eval,&p[0].native_score)||!hs28_native_score(b,t,eval,&p[1].native_score))return -1;
 int general=hs28_rooms(&s,vacancy,optional);if(general<0)return -1;
 hs28_plan_team(p,2,vacancy,optional,general,50,t,eval,&s);
 return p[0].choice==1?0:p[1].choice==1?1:-1;
}
__declspec(dllexport) int DriTestHs32Selected(Rec a,Rec b,Rec t,U eval) {
 Hs15Snapshot s;Hs28Choice p[2];int vacancy[22],optional[22];
 if(!hs15_collect(&s,t,eval)||!hs28_choice(&p[0],a,0,t,eval,&s,0)||!hs28_choice(&p[1],b,0,t,eval,&s,0))return -1;
 if(!hs28_native_score(a,t,eval,&p[0].native_score)||!hs28_native_score(b,t,eval,&p[1].native_score))return -1;
 int general=hs28_rooms(&s,vacancy,optional);if(general<0)return -1;
 hs28_plan_team(p,2,vacancy,optional,general,50,t,eval,&s);
 return (p[0].selected?1:0)|(p[1].selected?2:0);
}
#endif
