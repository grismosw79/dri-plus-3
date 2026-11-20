/* Ordinary recruiting recovery at the native team's action dispatcher. This
   boundary precedes its first board iterator and follows CPU funding. All
   scholarship, hours, NIL, influence and commitment publication stay native. */
#include "test36r3_recovery.h"
typedef struct {
 int school,board_score,retained,merit,slot,role,ask,paid,wallet,hours,cost,action,preference,grade,required,owned_offer;
 int compare_qb,pair_round; unsigned rival;
 const char *why;
} R2Pursuit;
typedef struct {Rec team;int user;Hs15Snapshot *shot;PortalQuality *rooms;U cache[67];int cache_ok;} R2School;
typedef struct {U eval,franchise;unsigned rid,year,type,week;int active;} R2Pass;
static R2Pass r2_last;

static int r2_hours(Rec team) {
 Rec b=reference(team,224);if(!b)return -1;
 int total=(int)scalar(b,3,0x100),assigned=(int)scalar(b,1,0x100),processed=(int)scalar(b,2,0x100);
 release(b);
 return total<0 || total>100000 || assigned<0 || processed<0 || assigned>total || processed>total-assigned?
   0:total-assigned-processed;
}
/* Choose the cheapest enabled, positive-influence persistent action from
   the active FTC. The game verifies the player's own weekly allowance too. */
static int r2_action(U eval,int *cost) {
 Rec tuning=(Rec)q(eval+0xd0),a=tuning?array_field(tuning,49):0;
 U n=a?array_count(a):0;int best=0;*cost=0;
 for(U i=0;i<n && n<=64;i++){
  Rec t=array_at(a,i);if(!t)continue;
  int type=(int)scalar(t,0,0x108),price=(int)scalar(t,2,0x100);
  if(type>=5 && type<=9 && price>0 && price<=1000 && scalar(t,1,0x100)>0 &&
     scalar(t,5,0xf8) && !scalar(t,6,0xf8) && (!best || price<*cost)){best=type;*cost=price;}
  release(t);
 }
 release(a);return best;
}
/* The inventory already holds this exact board target. Keep it borrowed for
   this observation, including when a scorer cannot give a current answer. */
static int r2_existing_portal_offer(Rec recruit,Rec team,U eval,Rec target,PortalRoomIssue *optional) {
 if(!target || scalar(target,12,0x108)<2)return 0;
 U cache[67];int score=0;unsigned mask=0;
 if(!market29_portal_school_cache(team,eval,cache))return portal_room_issue(optional,"SCHOOL_SCORE_INPUT",-1);
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 int week=season?(int)scalar(season,4,0x100):-1;
 release(season);release(f);
 if(week<0 || !market29_portal_score_week(recruit,team,eval,cache,week,&score))
  return portal_room_issue(optional,"CURRENT_SCORE",week);
 if(score<=0 || !test29r2_portal_committable_checked(recruit,team,eval,"R2_LAST_OFFER",optional) || faulted)return 0;
 return test31_productive_allocated(target,team,eval,&mask)>0 ||
        (hs28_recovery_cost(eval,1)>0 && r2_hours(team)>=hs28_recovery_cost(eval,1));
}
static int r2_portal_live(Rec recruit,Rec team,U eval) {
 Rec user=reference(team,407);if(user){release(user);return 1;}
 if(!native_recruit_eligible(eval,team,recruit) || faulted || locked_out(recruit,team,eval))return 0;
 Rec target=t36_target(team,recruit);
 int ok=r2_existing_portal_offer(recruit,team,eval,target,0);
 release(target);return ok;
}
static int r2_load_room(R2School *s,U eval,int portal) {
 if(!s->shot) {
  s->shot=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*s->shot));
  if(!s->shot || !hs15_collect(s->shot,s->team,eval)){if(s->shot)HeapFree(GetProcessHeap(),0,s->shot);s->shot=0;return 0;}
 }
 if(portal && !s->rooms){
  s->rooms=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,22*sizeof(*s->rooms));
  int reserved[22]={0};
  if(!s->rooms || !hs15_portal_rooms(s->shot,s->rooms,s->shot->prestige,s->shot->projection_step) ||
     !test29_hs_reserved(s->team,eval,reserved)){if(s->rooms)HeapFree(GetProcessHeap(),0,s->rooms);s->rooms=0;return 0;}
  dri_portal_set_hs_reservations(s->rooms,reserved,s->shot->prestige);
 }
 return 1;
}
static void r2_forget_room(R2School *s) {
 if(s->shot)HeapFree(GetProcessHeap(),0,s->shot);
 if(s->rooms)HeapFree(GetProcessHeap(),0,s->rooms);
 s->shot=0;s->rooms=0;s->cache_ok=0;
}
/* Retain funded primary competitors and native HS source reservations. An
   unworked forecast is not a claim on a seat. Use the shipped vacancy and
   quality planner to select the actual slot, including documented exceptions. */
static int r2_portal_role(R2School *s,Rec r,U eval,R2Pursuit *v,int raw,int pos) {
 PortalChoice pool[MAX_BOARD+1]={0};int n=0,pick=-1,projected=portal_projected_ovr(raw,s->shot->projection_step);
 int prestige=s->shot->prestige;
 if(dri_quality_block_candidate(&s->rooms[pos],prestige,projected,raw))return 0;
 Rec a=board_array(s->team);U size=a?array_count(a):51;
 if(size>50){release(a);return 0;}
 int fixed=0;
 for(U j=0;j<size;j++){
  Rec t=array_at(a,j),other=t?reference(t,10):0;
  if(!other){release(t);release(a);return 0;}
  int same=portal_same_record(r,other);
  if(!transfer_recruit(other) || scalar(other,9,0x108)>=5){fixed++;release(other);release(t);continue;}
  unsigned mask=0;int held=scalar(t,12,0x108)>=2 && test31_productive_allocated(t,s->team,eval,&mask)>0;
  if(same || held){
   int p=position(other),rating=raw,base=v->retained;
   if(!same){
    Rec person=reference(other,5);rating=person?(int)scalar(person,191,0x100):-1;release(person);
    if(!market29_portal_score_cached(other,s->team,eval,s->cache,&base)){release(other);release(t);release(a);return 0;}
   }
   if(p>=0 && p<22 && rating>=0 && rating<=99 && base>0){
    int ovr=portal_projected_ovr(rating,s->shot->projection_step),rank=0;
    PortalChoice *c=pool+n;c->rec=(U)other;c->rid=row(other);c->pos=p;c->ovr=ovr;c->raw_ovr=rating;
    c->native_base=base;c->score=base+dri_quality_bonus_candidate(&s->rooms[p],prestige,ovr,rating,&rank);c->existing=1;
    c->role=dri_portal_role_candidate(&s->rooms[p],prestige,ovr,rating);
    if(!portal_choice_race(c,t,s->team,eval)){release(other);release(t);release(a);return 0;}
    if(same)pick=n;
    n++;
   }
  }
  release(other);release(t);
 }
 release(a);
 if(pick<0) {
  if(size>=50)return 0;
  pick=n++;int rank=0;
  pool[pick]=(PortalChoice){.rec=(U)r,.rid=row(r),.pos=pos,.raw_ovr=raw,.ovr=projected,.native_base=v->board_score,
   .score=v->board_score+dri_quality_bonus_candidate(&s->rooms[pos],prestige,projected,raw,&rank)};
  pool[pick].role=dri_portal_role_candidate(&s->rooms[pos],prestige,projected,raw);
  if(!portal_choice_race(pool+pick,0,s->team,eval))return 0;
 }
 /* A locked, abandoned race is reopened only after a read-only fit has
    passed. No other candidate is given relaxed access. */
 pool[pick].race_ok=1;
 dri_portal_order(pool,n,50-fixed,s->rooms,prestige);
 PortalChoice *chosen=0;
 for(int j=0;j<n;j++)if(pool[j].rid==row(r)){chosen=pool+j;break;}
 if(!chosen || !chosen->selected || chosen->slot<1)return 0;
 v->pair_round=0;v->rival=0;
 if(chosen->round!=1 && (!v->compare_qb || chosen->round>3))return 0;
 PortalCommitPlan old,now={0};
 AcquireSRWLockExclusive(&commitment_lock);old=commitment_plans[v->school];ReleaseSRWLockExclusive(&commitment_lock);
 portal_commit_scope(&now,s->team,eval);
 if(portal_commit_plan_matches(&old,&now))for(int i=0;i<old.count;i++){
  PortalCommitEntry *e=old.entries+i;
  if(e->pos!=pos || e->slot!=chosen->slot || e->round!=1 || (e->table==q((U)r+0x10) && e->rid==row(r)))continue;
  for(int j=0;j<n;j++)if(pool[j].rid==e->rid && pool[j].offered && pool[j].race_ok){
   if(!v->compare_qb)return 0;
   if(s->rooms[0].count+s->rooms[0].hs_reserved>=s->rooms[0].ideal && chosen->role!=1)return 0;
   if(!rc4_pair_free(v->school,e->slot,e->rid,row(r),now.week) && !rc4_same_pair(v->school,e->slot,e->rid,row(r),now.week))return 0;
   v->rival=e->rid;v->pair_round=chosen->round==1 && rc4_competes(raw,chosen->score,pool[j].raw_ovr,pool[j].score)?1:2;
  }
 }
 if(chosen->round!=1 && !v->pair_round)return 0;
 v->role=chosen->role;v->slot=chosen->slot;v->merit=chosen->score;return 1;
}
static int r2_preference(Rec r,unsigned tid) {
 Rec top=array_field(r,13);U n=top?array_count(top):0;int rank=0;
 for(U j=0;j<n && n<=64;j++){Rec x=array_at(top,j);int found=x && scalar(x,0,0x100)==tid;release(x);if(found){rank=(int)j+1;break;}}
 release(top);return rank;
}
static int r2_option(R2School *s,Rec r,U eval,int portal,int pos,int raw,int week,R2Pursuit *v) {
 Rec team=s->team;
 v->why="NATIVE_ELIGIBILITY";if(!native_recruit_eligible(eval,team,r))return 0;
 v->why="ACADEMY_POLICY";if(portal?portal_academy(v->school):academy29_hs_block(r,team))return 0;
 v->why="ROSTER_DATA";if(!r2_load_room(s,eval,portal))return 0;
 if(!s->cache_ok)s->cache_ok=market29_portal_school_cache(team,eval,s->cache);
 if(!s->cache_ok)return 0;
 v->why="NATIVE_SCORE";
 if(!market29_portal_score_week(r,team,eval,s->cache,week,&v->board_score))return 0;
 v->retained=v->board_score; /* Legacy field: current score uses the recruit source, not board state. */
 Rec target=t36_target(team,r);v->owned_offer=target && scalar(target,12,0x108)>=2;
 int base_ok=v->retained>0 && (v->owned_offer || v->board_score>0);
 if(!base_ok){release(target);return 0;}
 v->hours=r2_hours(team);v->why="HOURS";
 int effort=0;v->action=r2_action(eval,&effort);
 int both=hs28_recovery_cost(eval,0),one=hs28_recovery_cost(eval,1);
 v->cost=effort+(v->owned_offer?0:both-one);
 if(!v->action || both<=one || one<=0 || v->hours<v->cost){release(target);return 0;}
 v->why="NIL";
 int quoted=r2_quote(eval,team,r,target,&v->ask,&v->paid,&v->wallet);release(target);
 if(!quoted || (!v->owned_offer && v->ask-v->paid>v->wallet))return 0;
 v->why="ROLE_OR_CLASS";
 if(portal) {
  if(!r2_portal_role(s,r,eval,v,raw,pos))return 0;
 }else{
  int score=0;
  test36_hs_probe=r;
  int allowed=hs28_recovery_destination(r,team,eval,s->shot,&score,&v->slot,&v->role);
  test36_hs_probe=0;
  if(allowed<=0 || v->role!=1)return 0;
  v->merit=score;
 }
 v->preference=r2_preference(r,(unsigned)v->school);v->why="QUALIFIED";
 return 1;
}
static void r2_note_target(Rec target,Rec team,U eval) {
 unsigned tid=scalar(team,383,0x100);Hs28TeamPlan scope={0};
 if(!hs28_scope(&scope,team,eval))return;
 Test31TargetIdentity id={q((U)target+0x10),row(target)};
 AcquireSRWLockExclusive(&test31_funding_lock);
 Test31FundingPass *p=test31_funding_pass+tid;
 if(p->valid && p->eval==eval && hs28_scope_matches(&p->scope,&scope) && p->scope.week==scope.week){
  int i=0;while(i<p->count && (p->targets[i].table!=id.table || p->targets[i].row!=id.row))i++;
  if(i==p->count && i<MAX_BOARD)p->targets[p->count++]=id;
 }
 ReleaseSRWLockExclusive(&test31_funding_lock);
}
static void r2_assign_slot(Rec r,Rec team,U eval,const R2Pursuit *v,int pos) {
 PortalCommitPlan now={0};if(!portal_commit_scope(&now,team,eval))return;
 AcquireSRWLockExclusive(&commitment_lock);
 PortalCommitPlan *p=commitment_plans+v->school;
 if(!portal_commit_plan_matches(p,&now) || p->week!=now.week){*p=now;p->valid=1;}
 int keep=0;
 for(int j=0;j<p->count;j++){
  PortalCommitEntry *e=p->entries+j;
  if(v->pair_round && e->pos==0 && e->slot==v->slot && e->rid==v->rival){
   e->round=v->pair_round==1?2:1;p->entries[keep++]=*e;continue;
  }
  if((e->table==q((U)r+0x10) && e->rid==row(r)) || (e->pos==pos && e->slot==v->slot && e->round==1))continue;
  p->entries[keep++]=*e;
 }
 p->count=keep;
 if(keep<MAX_BOARD)p->entries[p->count++]=(PortalCommitEntry){q((U)r+0x10),row(r),pos,v->slot,v->pair_round?v->pair_round:1,keep+1};
 ReleaseSRWLockExclusive(&commitment_lock);
}
static int r2_fund(Rec r,Rec team,U eval,int portal,int pos,R2Pursuit *v) {
 if(portal && !rc4_live_window(eval) && !v->owned_offer)return 0;
 Rc4Pair pair={0};
 Rec t=t36_target(team,r);unsigned tid=(unsigned)v->school;
 int added=t!=0;release(t);
 if(!added)added=portal?portal29_admit(team,r,eval,v->board_score):r2_hs_add(team,r,eval,v->merit);
 if(added!=1 || faulted)return 0;
 t=t36_target(team,r);if(!t)return 0;
 int ok=0,offer_rc=0,action_rc=-1,existing=scalar(t,12,0x108)>=2;
 int ask=0,paid=0,wallet=0,remaining=r2_hours(team),before=0,after=0;
 unsigned mask=0;int effort=0,action=r2_action(eval,&effort);
 if(!action || remaining<v->cost || !r2_quote(eval,team,r,t,&ask,&paid,&wallet) ||
    (!existing && ask-paid>wallet) || test32_hs_offer_capacity(r,team,eval)!=1 ||
    locked_out(r,team,eval) || !native_recruit_eligible(eval,team,r))goto finish;
 before=(int)hs28_productive_allocated(t,eval,&mask);
 if(!before){
  U manager=q(eval+0xc8);
  int limit=((int(*)(U,Rec*,Rec*))(game_base+0x08180380))(manager,&team,&t);
  int used=((int(*)(U,Rec*,int))(game_base+0x08186200))(manager,&t,0);
  int native_cost=((int(*)(U,int,int))(game_base+0x081868b0))(manager,action,4);
  if(native_cost!=effort || used<0 || limit<0 || used>limit || effort>limit-used ||
     scalar(t,action+10,0xf8))goto finish;
 }
 if(v->pair_round){
  if(!rc4_live_window(eval) || (!rc4_pair_free(tid,v->slot,row(r),v->rival,rc4.week) && !rc4_same_pair(tid,v->slot,row(r),v->rival,rc4.week)))goto finish;
  pair=(Rc4Pair){(unsigned short)tid,(unsigned short)v->slot,(unsigned short)rc4.week,
     (unsigned short)(v->pair_round==1?row(r):v->rival),(unsigned short)(v->pair_round==2?row(r):v->rival)};
  Rc4Pair *prior=rc4_pair_for(tid,row(r),rc4.week);if(prior)pair=*prior;
  rc4_pending=pair;rc4_pending_live=1;
 }
 if(!existing){
  if(portal && !rc4_live_window(eval))goto finish;
  int price=ask>paid?ask:paid;
  offer_rc=((int(*)(U,Rec*,Rec*,int,void*,int,int))(game_base+0x0819ee10))(
       q(eval+0xc8),&team,&t,price,0,i32(eval+0x1a8),i32(eval+0x1a4));
  if(offer_rc || faulted || scalar(t,12,0x108)<2)goto finish;
 }
 if(portal)r2_assign_slot(r,team,eval,v,pos);else hs29_offer_invalidate();
 if(scalar(r,9,0x108)>=5){ok=2;goto finish;}
 if(!before && r2_hours(team)>=effort && !scalar(t,action+10,0xf8)){
  Rec b=reference(team,224);int assigned=b?(int)scalar(b,1,0x100):-1;release(b);
  /* Native add-action entry used by the title's recruiting interface. It
     validates the individual cap, charges board hours and sets the action. */
  action_rc=((int(*)(U,Rec*,Rec*,int))(game_base+0x08147960))(q(eval+0xc8),&team,&t,action);
  b=reference(team,224);int updated=b?(int)scalar(b,1,0x100):-1;release(b);
  if(!action_rc && (updated-assigned!=effort || !scalar(t,action+10,0xf8))){
   portal_fault("R2_action_hour_ledger");goto finish;
  }
 }
 after=(int)hs28_productive_allocated(t,eval,&mask);
 if(!action_rc && !before && after>=effort){r2_note_target(t,team,eval);ok=1;}
 if(before && test31_productive_allocated(t,team,eval,&mask)>0)ok=1;
 if(ok && portal)portal29_observe(team,eval,"POST_FUNDING_R2");
finish:
 if(pair.slot){
  rc4_pending_live=0;
  int live=(int)scalar(t,12,0x108),credited=(int)scalar(t,2,0x100);
  if(ok || (live>=2 && live<=4 && (existing || offer_rc==0))){
   if(!rc4_same_pair(tid,pair.slot,pair.primary,pair.backup,rc4.week)){rc4.disk.pair[rc4.disk.pairs++]=pair;rc4_save();}
   log_line("RC4_QB_PAIR_PAYMENT,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d",rc4.year,rc4.week,tid,pair.slot,pair.primary,pair.backup,ask,paid,credited,before,after,live,ok);
  }
 }
 log_line("R2_PURSUIT_PAYMENT,%s,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",portal?"PORTAL":"HS",row(r),tid,ask,paid,wallet,offer_rc,action,action_rc,before,after,ok,(int)scalar(t,12,0x108));
 release(t);return ok;
}
typedef struct {uint64_t offered[3],working[3];int unverified;} R2Offers;
static int r2_inventory_exception(unsigned tid,unsigned rid,unsigned slot,DWORD code) {
 log_line("RM18R22_RECOVERY_EXCEPTION,FUNDED_OFFER,%u,%u,%u",tid,rid,slot);
 return fail_exception("R9_funded_offer",code);
}
static int r2_observe_offer(Rec t,Rec r,Rec team,U eval,unsigned tid,unsigned rid,unsigned slot,
                            int user,R2Offers *offers,int *held) {
 __try {
  unsigned mask=0;market29_bit(offers->offered,tid,1);
  if(offers->unverified)return !faulted && mode==1;
  if(user){market29_bit(offers->working,tid,1);return 1;}
  if(test31_productive_allocated(t,team,eval,&mask)>0 && !faulted &&
     native_recruit_eligible(eval,team,r) && !faulted && !locked_out(r,team,eval) && !faulted){
   PortalRoomIssue issue={0,-1,rid};
   int working=transfer_recruit(r)?r2_existing_portal_offer(r,team,eval,t,&issue):hs29_offer_state(r,team,eval)==2;
   if(faulted || mode!=1)return 0;
   if(issue.source){
    if(!offers->unverified)(*held)++;
    offers->unverified=1;
    log_line("RM18R22_RECOVERY_OFFER_UNVERIFIED,%u,%u,%u,%s,%d,%u,KEEP_EXISTING_OFFERS_THIS_SWEEP",
             tid,rid,slot,issue.source,issue.member,issue.checked_rid);
   }else if(working)market29_bit(offers->working,tid,1);
  }
  return !faulted && mode==1;
 } __except(r2_inventory_exception(tid,rid,slot,GetExceptionCode())) {return 0;}
}
static int r2_inventory(R2School *schools,Rec teams,U eval,R2Offers *portal,R2Offers *hs,int *held) {
 U n=array_count(teams);
 for(U i=0;i<n;i++){
  Rec team=array_at(teams,i);unsigned tid=team?scalar(team,383,0x100):138;
  if(tid>=138 || schools[tid].team){
   log_line("RM18R22_RECOVERY_INVENTORY_INCOMPLETE,TEAM,%u,%u",tid,(unsigned)i);
   release(team);return 0;
  }
  R2School *s=schools+tid;s->team=team;Rec human=reference(team,407);s->user=human!=0;release(human);
  Rec a=board_array(team);U size=a?array_count(a):MAX_BOARD+1;
  if(size>50){
   log_line("RM18R22_RECOVERY_INVENTORY_INCOMPLETE,BOARD,%u",tid);
   release(a);return 0;
  }
  for(U j=0;j<size;j++){
   Rec t=array_at(a,j),r=t?reference(t,10):0;
   if(!r){
    log_line("RM18R22_RECOVERY_INVENTORY_INCOMPLETE,BOARD_TARGET,%u,%u",tid,(unsigned)j);
    release(t);release(a);return 0;
   }
   unsigned status=scalar(t,12,0x108);R2Offers *o=0;
   if(status>=2 && status<=4 && scalar(r,9,0x108)<5){
    if(portal && transfer_recruit(r)){
     Market29PortalRecruit *m=market29_portal_find(q((U)r+0x10),row(r));
     if(m && r3_recovery_stars(1,m->stars))o=portal+(m-market29_portal.recruits);
    }else if(hs && hs15_is_recruit(r)){
     Hs28MarketRecruit *m=hs28_market_find(q((U)r+0x10),row(r));
     if(m)o=hs+(m-hs28_market.recruits);
    }
   }
   if(o && !r2_observe_offer(t,r,team,eval,tid,row(r),(unsigned)j,s->user,o,held)){
    release(r);release(t);release(a);return 0;
   }
   release(r);release(t);
   if(faulted || mode!=1){release(a);return 0;}
  }
  release(a);
 }
 return 1;
}
static int r2_before_option(const R2Pursuit *a,const R2Pursuit *b) {
 if(a->compare_qb && b->compare_qb){
  if((a->pair_round==2)!=(b->pair_round==2))return a->pair_round!=2;
  if(a->pair_round==2 && a->owned_offer!=b->owned_offer)return a->owned_offer<b->owned_offer;
 }
 if(a->owned_offer!=b->owned_offer)return a->owned_offer>b->owned_offer;
 if(a->merit!=b->merit)return a->merit>b->merit;
 if(a->preference!=b->preference)return a->preference && (!b->preference || a->preference<b->preference);
 return a->school<b->school;
}
static struct {R2Pass scope;uint64_t seen[3],acted[3];int closed,post_closed;} r2_funding_batch;
static int r2_same_pass(const R2Pass *a,const R2Pass *b) {
 return a->eval==b->eval && a->franchise==b->franchise && a->rid==b->rid && a->year==b->year &&
    a->type==b->type && a->week==b->week;
}
static R2Pass r2_pass(U eval) {
 Rec f=context_ref(eval,6),s=f?reference(f,44):0;R2Pass p={0};
 if(s)p=(R2Pass){eval,q((U)f+0x10),row(f),scalar(s,2,0x100),scalar(s,5,0x108),scalar(s,4,0x100),1};
 release(s);release(f);return p;
}
static void r2_funding_tick(Rec team,U eval) {
 R2Pass p=r2_pass(eval);unsigned t=scalar(team,383,0x100);
 if(!p.active || t>=138)return;
 if(!r2_same_pass(&r2_funding_batch.scope,&p) || r2_funding_batch.closed) {
  memset(&r2_funding_batch,0,sizeof(r2_funding_batch));r2_funding_batch.scope=p;
  r2_last.active=0;
 }
 market29_bit(r2_funding_batch.seen,t,1);
}
static void r2_drop_hs_reservation(Rec r,unsigned tid) {
 int k=0;
 for(int i=0;i<hs28_market.recovery_count[tid];i++){
  Hs28MarketReservation v=hs28_market.recovery[tid][i];
  if(v.table!=q((U)r+0x10) || v.rid!=row(r))hs28_market.recovery[tid][k++]=v;
 }
 hs28_market.recovery_count[tid]=k;
}
static void r2_offer_set(Market29PortalRecruit *m,const uint64_t live[3],int school) {
 /* Every ordinary team has already had its funding turn. Only actual offers
    retain a consumed place. Snapshot caps are rebuilt from the real boards. */
 for(int j=0;j<3;j++){
  m->pressure.hard_authorized[j]=live[j];m->pressure.planned[j]&=live[j];
  m->pressure.primary[j]&=live[j];m->pressure.protected_offer[j]=live[j];
 }
 if(school>=0){
  market29_bit(m->pressure.hard_authorized,(unsigned)school,1);market29_bit(m->pressure.planned,(unsigned)school,1);
  market29_bit(m->pressure.primary,(unsigned)school,1);
 }
}
#include "rc4_qb_funding.inc"
static void r2_repair_closed(Rec r,R2School *schools,U eval,const R2Pass *p,int portal,R2Offers *o,unsigned stage) {
 Rec person=reference(r,5);if(!person)return;
 int pos=(int)scalar(person,3,0x108),raw=(int)scalar(person,191,0x100),stars=(int)scalar(person,231,0x108);
 int mot=(int)scalar(person,233,0x108),owner=-1;
 if(!r3_recovery_needed(portal,stars,(int)stage,0,0) || pos<0 || pos>=22 || raw<0 || raw>99 || portal_published_owner(r,eval,&owner)!=0){release(person);return;}
 Market29PortalRecruit *m=portal?market29_portal_find(q((U)r+0x10),row(r)):0;
 if(portal && (!m || (!rc4_live_window(eval) && !market30_count(o->offered)))){release(person);return;}
 int offered=market30_count(o->offered),limit=portal?market30_limit(1):hs33_limit_stars(stars);
 if(!portal && p->type!=6 && !stage && !offered){
  Hs28MarketRecruit *h=hs28_market_find(q((U)r+0x10),row(r));
  if(!h || !market30_count(h->ever_funded)){release(person);return;}
 }
 int compare_qb=portal && rc4.window && rc4_elite((U)r);
 R2Pursuit options[138];int count=0;
 Rec grade_leaving=0;
 __try {
 /* These two grades are diagnostics only; selection uses the existing
    eligibility/merit checks. Resolve their owner from the live evaluator. */
 if(portal && log30_deep && mot>=0 && mot<14){
  Rec current=context_ref(eval,6);
  __try {
   if(current && q((U)current+0x10)==p->franchise && row(current)==p->rid)
    grade_leaving=t36_current_leaving(current);
  } __finally { release(current); }
 }
 for(int i=0;i<138 && !faulted;i++){
  R2School *s=schools+i;if(!s->team || s->user)continue;
  R2Pursuit v={.school=i,.grade=-1,.required=-1,.compare_qb=compare_qb};
  if(portal && !rc4.window && !market29_has(o->offered,(unsigned)i))continue;
  if(offered>=limit && !market29_has(o->offered,(unsigned)i))continue;
  r2_late_recruit=r;
  int ok=r2_option(s,r,eval,portal,pos,raw,(int)p->week,&v);
  r2_late_recruit=0;
  if(faulted || mode!=1)return;
  if(portal && ok && grade_leaving && mot>=0 && mot<14){
   v.grade=t36_grade((U)grade_leaving,person,s->team,mot);
   v.required=((int(*)(U,int,Rec*,Rec*))(game_base+0x084330e0))((U)grade_leaving,mot,&person,&s->team);
  }
  if(log30_deep) {
   log_line("R2_CANDIDATE,%s,%u,%u,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
    portal?"PORTAL":"HS",p->week,row(r),i,v.why,v.board_score,v.retained,v.slot,v.role,v.hours,v.cost,v.ask,v.paid,v.wallet,v.preference,v.grade,v.required);
  }
  if(ok){int j=count++;while(j && r2_before_option(&v,options+j-1)){options[j]=options[j-1];j--;}options[j]=v;}
 }
 } __finally { release(grade_leaving);release(person); }
 /* First exhaust the player's current shortlist. Reopen at most once, with
    positive, eligible, affordable alternatives and zero live funded paths. */
 unsigned start=stage;int changed=0,success=0;uint64_t tried[3]={0};
 for(int outside=0;outside<=1 && !faulted && !success;outside++)for(int j=0;j<count && !success && !faulted;j++){
  R2Pursuit v=options[j];Rec team=schools[v.school].team;
  if(scalar(r,9,0x108)>=5)return;
  if(market30_count(o->offered)>=limit && !market29_has(o->offered,(unsigned)v.school))continue;
  if(market29_has(tried,(unsigned)v.school))continue;
  int locked=locked_out(r,team,eval);
  if(!outside && locked)continue;
  if(outside && !locked && !changed)continue;
  if(outside && locked && !changed && start>0){portal_recovery_scalar(r,9,0);changed=1;}
  if(locked_out(r,team,eval))continue;
  market29_bit(tried,(unsigned)v.school,1);
  r2_late_recruit=r;r2_late_team=team;
  int ok=r2_option(schools+v.school,r,eval,portal,pos,raw,(int)p->week,&v);
  if(ok && !faulted && mode==1 && portal)r2_offer_set(m,o->offered,v.school);
  if(ok && !faulted && mode==1 && !portal){
   ok=test36_hs_grant(r,team,eval,v.merit,o->offered);
   if(ok){test36_hs_probe=r;ok=hs28_recovery_reserve(team,r,eval,v.slot,v.role)>0;test36_hs_probe=0;}
  }
  if(ok && !faulted && mode==1)success=r2_fund(r,team,eval,portal,pos,&v);
  if(faulted || mode!=1){r2_late_recruit=0;r2_late_team=0;return;}
  int actual=portal_active_offer(r,team);
  if(actual>0)market29_bit(o->offered,(unsigned)v.school,1);
  if(!success && !portal){
   r2_drop_hs_reservation(r,(unsigned)v.school);
   if(actual==0)hs33_release_offer(r,team,eval);
  }
  if(portal)r2_offer_set(m,o->offered,-1);
  r2_late_recruit=0;r2_late_team=0;
  log_line("R2_RETRY,%s,%u,%u,%d,%d,%d,%d",portal?"PORTAL":"HS",p->week,row(r),v.school,v.slot,actual,success);
  r2_forget_room(schools+v.school);
  if(success)break;
 }
 if(changed && !success && !faulted && !market30_count(o->offered) && !scalar(r,9,0x108))
   portal_recovery_scalar(r,9,start);
 log_line("R2_RECOVERY_RESULT,%s,%u,%u,%d,%d,%d,%d",portal?"PORTAL":"HS",p->week,row(r),offered,count,market30_count(o->offered),success);
}
static void r2_repair_one(Rec r,R2School *schools,U eval,const R2Pass *p,int portal,R2Offers *o) {
 if(o->unverified || faulted || mode!=1 || market30_count(o->working))return;
 unsigned stage=scalar(r,9,0x108);
 if(stage<5)r2_repair_closed(r,schools,eval,p,portal,o,stage);
}
static void r2_sweep_mode(U eval,int closure) {
 R2Pass p=r2_pass(eval);
 if(!p.active || !r2_same_pass(&r2_funding_batch.scope,&p) ||
    (!closure && r2_last.active && r2_same_pass(&r2_last,&p)))return;
 int portal=p.type==6 && p.week>=2 && p.week<=6 && market29_portal.ready && market29_portal.scope.week==p.week && rc4_live_window(eval);
 int hs=!closure && hs28_market_scope(eval,0) && hs28_market.ready && hs28_pair_window(eval);
 if(!portal && !hs)return;
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;U n=teams?array_count(teams):0;
 if(!n || n>138){release(teams);release(f);return;}
 for(U i=0;i<n;i++){Rec t=array_at(teams,i);Rec human=t?reference(t,407):0;unsigned id=t?scalar(t,383,0x100):138;
  int done=id<138 && (human || market29_has(r2_funding_batch.seen,id));release(human);release(t);
  if(!done){release(teams);release(f);return;}
 }
 if(!closure){r2_last=p;r2_funding_batch.closed=1;}
 R2School *schools=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*sizeof(*schools));
 R2Offers *po=portal?HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(market29_portal.count+1)*sizeof(*po)):0;
 R2Offers *ho=hs?HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(hs28_market.count+1)*sizeof(*ho)):0;
 int held=0;
 if(schools && (!portal || po) && (!hs || ho) && r2_inventory(schools,teams,eval,po,ho,&held) && !faulted && mode==1){
  if(held)log_line("RM18R22_RECOVERY_HELD,%u,%u,%d,%d",p.year,p.week,closure,held);
  test31_offer_register(eval);
  if(portal && !closure)rc4_work_openings(schools,f,eval,&p,po);
  int complete=1;
  for(int source=0;source<2 && !faulted && mode==1;source++){
   if(source?!portal:!hs)continue;
   Rec a=array_field(f,source?46:39);U count=a?array_count(a):0;
   if(!a || count>12000)complete=0;
   else for(U i=0;i<count && !faulted && mode==1;i++){
    Rec r=array_at(a,i);if(!r){complete=0;break;}
    if(source){Market29PortalRecruit *m=market29_portal_find(q((U)r+0x10),row(r));
     if(m && r3_recovery_stars(1,m->stars) && (p.week>=3 || (rc4.window && rc4_elite((U)r))))r2_repair_one(r,schools,eval,&p,1,po+(m-market29_portal.recruits));
    }else{Hs28MarketRecruit *m=hs28_market_find(q((U)r+0x10),row(r));
     if(m && !m->owned)r2_repair_one(r,schools,eval,&p,0,ho+(m-hs28_market.recruits));}
    release(r);
   }
   release(a);
  }
  if(!faulted && mode==1 && complete)
   log_line(closure?"R3_CLOSED_RECOVERY_COMPLETE,%u,%u,%u,%d,%d":"R2_PRE_ACTION_COMPLETE,%u,%u,%u,%d,%d",p.year,p.type,p.week,hs,portal);
  else if(!faulted && mode==1)log_line("RM18R22_RECOVERY_POOL_INCOMPLETE,%u,%u,%d",p.year,p.week,closure);
 }
 if(schools){for(int i=0;i<138;i++){r2_forget_room(schools+i);release(schools[i].team);}HeapFree(GetProcessHeap(),0,schools);}
 if(po)HeapFree(GetProcessHeap(),0,po);if(ho)HeapFree(GetProcessHeap(),0,ho);
 release(teams);release(f);
}
static void r2_sweep(U eval){r2_sweep_mode(eval,0);}
static void r3_closed_recovery(U eval,const char *site) {
 if(mode!=1 || faulted)return;
 R2Pass p=r2_pass(eval);
 if(p.type!=6 || p.week<3 || p.week>6 || !r2_same_pass(&r2_funding_batch.scope,&p))return;
 log_line("R3_CLOSED_RECOVERY_BEGIN,%s,%u,%u",site,p.year,p.week);
 /* Rebuild offered/working inventory and all rooms. An earlier conditional
    path does not survive as authorization after its actual role closes. */
 r2_sweep_mode(eval,1);
}
static void r3_after_actions(U eval,Rec team) {
 R2Pass p=r2_pass(eval);unsigned tid=scalar(team,383,0x100);
 if(tid>=138 || p.type!=6 || p.week<3 || p.week>6 || !r2_funding_batch.closed ||
    !r2_same_pass(&r2_funding_batch.scope,&p) || r2_funding_batch.post_closed)return;
 market29_bit(r2_funding_batch.acted,tid,1);
 for(int j=0;j<3;j++)if((r2_funding_batch.acted[j]&r2_funding_batch.seen[j])!=r2_funding_batch.seen[j])return;
 r2_funding_batch.post_closed=1;
 r3_closed_recovery(eval,"AFTER_NATIVE_ACTION_BATCH");
}
static U r2_before_actions(U eval,Rec *team) {
 if(mode==1 && !faulted && eval && team && *team)
  __try {r2_sweep(eval);} __except(fail_exception("R2_pre_action",GetExceptionCode())){}
 U result=((U(*)(U,Rec*))(game_base+0x08152bd0))(eval,team);
 if(mode==1 && !faulted && eval && team && *team)
  __try {r3_after_actions(eval,*team);} __except(fail_exception("R3_post_action",GetExceptionCode())){}
 return result;
}
