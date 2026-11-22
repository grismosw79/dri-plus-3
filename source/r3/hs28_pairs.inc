/* Test28R9: one numeric plan connects allocation, funding and commitment.
   Native records are reacquired for every use. Reloads reconstruct scores;
   current ownership and room capacity remain authoritative. */
#include "hs28_pairs.h"
static Rec hs28_nsd_destination(Rec recruit,U eval);
static void hs28_nsd_reset(void);
static int hs28_coverage_prior(Rec team,U eval,Hs28Prior *old,int no);
static int hs28_coverage_recovered(Rec recruit,Rec team,U eval);
static int hs28_abandoned_continuity(Rec recruit,Rec team,U eval);
static int hs28_native_score(Rec recruit,Rec team,U eval,int *out);
static unsigned hs28_productive_allocated(Rec target,U eval,unsigned *mask);
static void hs32_note_unseeded(Rec recruit);
static int hs32_admission_block(Rec recruit,Rec team,U eval);
static int hs32_recovery_block(Rec recruit,Rec team,U eval);
static void hs32_crowding_apply(Hs28Choice *pool,int n,Rec team,U eval,const Hs15Snapshot *snapshot);
static void hs33_prepare_native_pool(const Hs28Choice *pool,int n,Rec team,U eval);
static int hs33_plan_published(const Hs28Choice *pool,int n,Rec team,U eval);
static void hs33_plan_strict(Rec team,U eval,int strict);
typedef struct {
 Hs28Prior prior;U player_table;uint32_t player_row;
} Hs28PlanEntry;
typedef struct {
 U franchise_table,franchise_handle,eval,team_table;uint32_t franchise_row,team_row;
 unsigned year,week,type;int valid,count;
 Hs28PlanEntry entry[50];
} Hs28TeamPlan;
static Hs28TeamPlan hs28_team_plans[138];
static SRWLOCK hs28_plan_lock=SRWLOCK_INIT;
static int hs28_scope(Hs28TeamPlan *p,Rec team,U eval) {
 Rec f=context_ref(eval,6),s=0;uint32_t year=0,week=0,type=0;
 if(!cache_season_get(eval,f,&year,&week,&type)) { /* cache stack: the season memo of the window */
  s=f?reference(f,44):0;
  if(!f || !s){release(s);release(f);return 0;}
  year=scalar(s,2,0x100);week=scalar(s,4,0x100);type=scalar(s,5,0x108);
  cache_season_check(eval,f,year,week,type,"HS28_SCOPE");
 }
 p->franchise_table=q((U)f+0x10);p->franchise_handle=(U)f;p->franchise_row=row(f);p->eval=eval;
 p->team_table=q((U)team+0x10);p->team_row=row(team);
 p->year=year;p->week=week;p->type=type;
 release(s);release(f);return 1;
}
static int hs28_scope_matches(const Hs28TeamPlan *a,const Hs28TeamPlan *b) {
 return a->valid && a->franchise_table==b->franchise_table && a->franchise_handle==b->franchise_handle &&
 a->franchise_row==b->franchise_row && a->eval==b->eval && a->team_table==b->team_table &&
 a->team_row==b->team_row && a->year==b->year &&
 (b->type>a->type || (b->type==a->type && b->week>=a->week));
}
#include "test29_class_runtime.inc"
static int hs28_plan_load(Hs28TeamPlan *p,Rec team,U eval) {
 unsigned tid=scalar(team,383,0x100);Hs28TeamPlan now={0};
 if(tid>=138 || !hs28_scope(&now,team,eval))return 0;
 AcquireSRWLockExclusive(&hs28_plan_lock);*p=hs28_team_plans[tid];ReleaseSRWLockExclusive(&hs28_plan_lock);
 return hs28_scope_matches(p,&now);
}
#include "hs34r3_continuity.inc"
static void hs28_plan_store(const Hs28Choice *pool,int n,Rec team,U eval) {
 unsigned tid=scalar(team,383,0x100);Hs28TeamPlan plan={0};
 if(tid>=138 || !hs28_scope(&plan,team,eval))return;
 for(int i=0;i<n;i++)if(pool[i].selected) {
  if(plan.count>=50){portal_fault("HS_plan_store_capacity");return;}
  const Hs28Choice *p=&pool[i];Hs28PlanEntry *e=&plan.entry[plan.count++];
  e->prior=(Hs28Prior){p->table,p->rid,p->pos,p->slot,p->choice,p->order,p->native_score,0,0};
  e->player_table=p->player_table;e->player_row=p->player_row;
 }
 plan.valid=1;int changed=0;
 AcquireSRWLockExclusive(&hs28_plan_lock);
 Hs28TeamPlan *old=&hs28_team_plans[tid];
 changed=!hs28_scope_matches(old,&plan) || old->week!=plan.week || old->type!=plan.type || old->count!=plan.count;
 for(int i=0;i<plan.count && !changed;i++) {
  const Hs28Prior *p=&plan.entry[i].prior;int found=0;
  for(int j=0;j<old->count;j++) {
   const Hs28Prior *q=&old->entry[j].prior;
   if(p->table==q->table && p->rid==q->rid && p->slot==q->slot && p->choice==q->choice && p->native_score==q->native_score){found=1;break;}
  }
  if(!found)changed=1;
 }
 hs28_team_plans[tid]=plan;ReleaseSRWLockExclusive(&hs28_plan_lock);
 if(changed)test29_class_invalidate(team);
 if(changed)for(int i=0;i<n;i++)if(pool[i].selected)
  log_line("HS28_PLAN,%u,%u,%u,%d,%d,%d,%d",tid,plan.week,pool[i].rid,pool[i].slot,pool[i].choice,pool[i].weight,pool[i].native_score);
 if(changed)for(int i=0;i<n;i++)if(pool[i].selected && pool[i].choice==1 && pool[i].stars>=3)
  log_line("HS29_QUALITY_PLAN,%u,%u,%d,%d,%d,%lld,PLANNED_NOT_FUNDED",tid,pool[i].rid,
   pool[i].pos,pool[i].ovr,pool[i].native_score,(long long)hs28_selection_value(&pool[i]));
 if(changed && test29_class_academy(tid))for(int i=0;i<n;i++)if(pool[i].selected && pool[i].academy_preference)
  log_line("TEST29_ACADEMY_TARGET,%u,%u,%d,%d,%d,%d,NATIVE_SCORE_UNCHANGED",tid,pool[i].rid,
   pool[i].stars+1,pool[i].native_score,pool[i].academy_preference,pool[i].choice);
}
static void hs28_close_prior(Hs28TeamPlan *plan,int slot) {
 for(int i=0;i<plan->count;i++)if(plan->entry[i].prior.slot==slot)plan->entry[i].prior.closed=1;
}
static void hs28_prior_ownership(Hs28TeamPlan *plan,const Hs15Snapshot *snapshot) {
 /* The same snapshot already includes hard ownership and published incoming
    players. Close their old pair before remapping the reduced vacancies. */
 for(int j=0;j<snapshot->used;j++) {
  const Hs15Player *p=&snapshot->players[j];if(!p->hs && !p->published)continue;
  for(int i=0;i<plan->count;i++)if(plan->entry[i].player_table==p->table && plan->entry[i].player_row==p->rid)
   hs28_close_prior(plan,plan->entry[i].prior.slot);
 }
}
static int hs28_plan_team(Hs28Choice *pool,int n,const int vacancy[22],const int optional[22],
 int general,int capacity,Rec team,U eval,const Hs15Snapshot *snapshot) {
 Hs28TeamPlan plan;Hs28Prior prior[50];int np=0;
 if(hs28_plan_load(&plan,team,eval) || hs34r3_restore(&plan,pool,n,team,eval,snapshot)) {
  hs28_prior_ownership(&plan,snapshot);
  for(int i=0;i<plan.count;i++)prior[np++]=plan.entry[i].prior;
 }
 np=hs28_coverage_prior(team,eval,prior,np);
 test29_class_preferences(pool,n,team,snapshot);
 hs32_crowding_apply(pool,n,team,eval,snapshot);
 np=hs34r3_reconstruct(pool,n,vacancy,optional,general,capacity,prior,np);
 int approved[22],extra[22];
 general=test29_class_prepare(pool,n,vacancy,optional,general,team,eval,snapshot,prior,np,approved,extra);
 if(general<0)return 0;
 return hs28_plan_prior(pool,n,approved,extra,general,capacity,prior,np);
}
static int hs28_regular(U eval) {
 Rec f=context_ref(eval,6),s=f?reference(f,44):0;
 int yes=s && scalar(s,5,0x108)<=1;
 release(s);release(f);return yes;
}
static int hs28_pair_window(U eval) {return hs15_window(eval)>0;}
static int hs28_identity(const Hs28Choice *p,Rec r) {
 return r && p->table==q((U)r+0x10) && p->rid==row(r);
}
static int hs28_choice_issue(Rec recruit,Rec team,const char *field_name) {
 log_line("HS28_CHOICE_INPUT_FAIL,%u,%u,%s",scalar(team,383,0x100),row(recruit),field_name);
 return 0;
}
static int hs28_choice(Hs28Choice *p,Rec rec,Rec target,Rec team,U eval,const Hs15Snapshot *s,int deadline) {
 memset(p,0,sizeof(*p));p->index=-1;p->final=deadline;p->pursuit_state=1;
 p->table=q((U)rec+0x10);p->rid=row(rec);p->rec=(U)rec;p->target=(U)target;p->existing=target!=0;
 p->stage=(int)scalar(rec,9,0x108);
 if(p->stage<0 || p->stage>6)return hs28_choice_issue(rec,team,"STAGE");
 if(!hs15_is_recruit(rec) || p->stage>=5)return 1;
 Rec player=reference(rec,5);if(!player)return hs28_choice_issue(rec,team,"PLAYER");
 p->player_table=q((U)player+0x10);p->player_row=row(player);
 int acquired=hs15_has(s,player);
 p->pos=(int)scalar(player,3,0x108);p->ovr=(int)scalar(player,191,0x100);p->stars=(int)scalar(player,231,0x108);release(player);
 if(p->pos<0 || p->pos>=22 || p->ovr<0 || p->ovr>99 || p->stars<0 || p->stars>4)return hs28_choice_issue(rec,team,"PLAYER_FIELDS");
 /* Published ownership can coexist with a soft-looking native stage and a
    positive native candidate score. Preserve that physical candidate, but
    never assign the already acquired player another active pair opening. */
 if(acquired){hs34_trace_note(team,rec,HS34_ALREADY_OWNED);return 1;}
 if(target && !hs28_native_score(rec,team,eval,&p->native_score))return hs28_choice_issue(rec,team,"BOARD_SCORE");
 int blocked=hs15_snapshot_values(s,p->pos,p->stars,p->ovr);
 hs34_trace_block(team,rec,s,p->pos,blocked);
 if(blocked<0)return hs28_choice_issue(rec,team,"ROOM_POLICY");
 p->commit_score=(int)scalar(rec,3,0x100);
 if(p->commit_score<0 || p->commit_score>1023)return hs28_choice_issue(rec,team,"COMMIT_SCORE");
 if(target){
  Rec linked=reference(target,10);int match=linked && p->table==q((U)linked+0x10) && p->rid==row(linked);release(linked);
  unsigned status=scalar(target,12,0x108);p->points=(int)scalar(target,8,0x100);
  if(!match || status>4 || p->points<0 || p->points>65535)return hs28_choice_issue(rec,team,"BOARD_TARGET");
  p->offered=status>=2;
 }
 Rec top;U count;
 if(!hs_read_school_list(rec,(unsigned)p->stage,&top,&count))return hs28_choice_issue(rec,team,"SCHOOL_LIST");
 if(!top)hs32_note_unseeded(rec);
 if(count>64){release(top);return hs28_choice_issue(rec,team,"SCHOOL_LIST_COUNT");}
 int cutoff=p->stage==0?5:3,values[64],seen[138]={0},tid=(int)scalar(team,383,0x100),listed=0;
 for(U j=0;j<count;j++) {
  Rec e=array_at(top,j);if(!e){release(top);return hs28_choice_issue(rec,team,"SCHOOL_LIST_ENTRY");}
  int t=(int)scalar(e,0,0x100),v=(int)scalar(e,1,0x100);release(e);
  if(t<0 || t>=138 || v<0 || v>65535 || seen[t]++){release(top);return hs28_choice_issue(rec,team,"SCHOOL_LIST_VALUE");}
  values[j]=v;if(v>p->leader)p->leader=v;
  if(t==tid){p->rank=(int)j+1;p->points=v;listed=1;}
 }
 release(top);
 for(U j=1;j<count;j++){int v=values[j];U k=j;while(k>0 && values[k-1]<v){values[k]=values[k-1];k--;}values[k]=v;}
 p->cut_points=count>=(U)cutoff?values[cutoff-1]:0;
 p->legal=!blocked;
 if(!p->legal)return 1;
 if(academy29_hs_block(rec,team)){p->legal=0;hs34_trace_note(team,rec,HS34_ACADEMY);return 1;}
 if(!deadline && locked_out(rec,team,eval)){p->legal=0;hs34_trace_note(team,rec,HS34_CLOSED_RACE);return 1;}
 /* Eligibility must survive the native dealbreaker/offer gate too. */
 p->legal=native_recruit_eligible(eval,team,rec);
 if(!p->legal)return 1;
 int early=p->stage==0 && (p->commit_score<=0 || (int64_t)p->leader*100<(int64_t)p->commit_score*35);
 p->viable=early || (listed && (p->rank<=cutoff || (int64_t)p->points*4>=(int64_t)p->leader*3));
 /* R13R1: a qualified recovery survives advisory preference milestones.
    Legal/eligibility/acquisition checks above still apply. Pending proposals
    remain scoped reservations; continuity additionally needs a live offer. */
 if(hs28_coverage_recovered(rec,team,eval))p->viable=1;
 if(p->existing && p->offered && hs28_abandoned_continuity(rec,team,eval))p->viable=1;
 /* A restart has no recovery memo. A real offered pursuit with native earned
    influence or a productive assigned action is reconstructible from the
    save. This is credibility only: current legal/eligibility checks above,
    pairing, capacity and productive budget accounting remain authoritative. */
 if(!p->viable && p->existing && p->offered) {
  unsigned actions=0;
  if((int32_t)scalar(target,7,0x100)>0 || test31_productive_allocated(target,team,eval,&actions)>0)p->viable=1;
 }
 if(deadline)p->viable=p->offered;
 /* Test34: seeded/unoffered interest alone is not a paid pursuit. Retain
    actual offered progress through weekly action reset and cold reload. A
    current productive allocation must be backed by the native hour ledger;
    old action flags with an empty assigned budget do not establish payment. */
 if(p->stage>=4)p->pursuit_state=2;
 else if(p->existing && p->offered) {
  if(p->points>0 || (int32_t)scalar(target,7,0x100)>0)p->pursuit_state=2;
  else {
   unsigned mask=0,paid=test31_productive_allocated(target,team,eval,&mask);
   Rec budget=reference(team,224);
   unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;
   release(budget);
   if(paid && paid<=assigned && assigned<=total)p->pursuit_state=2;
  }
 }
 p->urgent=p->stage<=1 && p->rank>=cutoff && p->cut_points>0 &&
  (int64_t)p->points*100>=(int64_t)p->cut_points*70;
 return 1;
}
static int hs28_rooms(const Hs15Snapshot *s,int vacancy[22],int optional[22]) {
 PortalQuality rooms[22];if(!hs15_portal_rooms(s,rooms,s->prestige,s->projection_step))return -1;
 uint32_t states=rooms[0].exception_states & ~DRI_EXCEPTION_READY;int general=0;
 while(states && general<2){states=dri_exception_step(states,DRI_EXCEPTION_GENERAL);if(states)general++;}
 for(int p=0;p<22;p++) {
  vacancy[p]=s->annual_valid?(s->annual[p]>s->hs[p]?s->annual[p]-s->hs[p]:0):
   (s->ideal[p]>s->count[p]?s->ideal[p]-s->count[p]:0);
  int annual=dri_annual_limit(p,s->ideal[p])-s->hs[p];
  int first=!s->hs[p] && annual>0;
  /* Candidate legality below narrows the second branch to a material elite
     upgrade. One shared optional slot per position; no fictitious vacancy. */
  int quality=p>=1 && p<=18 && s->hs[p]>0 && s->count[p]==s->ideal[p];
  optional[p]=p!=19 && p!=20 && s->ideal[p]>0 &&
   s->count[p]>=s->ideal[p] && (first || quality);
  if(hs34_vacancy_extra(s,p))optional[p]=1;
  if(s->annual_valid && vacancy[p]>0)optional[p]=0;
 }
 return general;
}
static int hs28_board(Rec team,U eval,const Hs15Snapshot *s,Hs28Choice pool[MAX_BOARD],int *n,int *fixed,int deadline) {
 *n=0;*fixed=0;Rec a=board_array(team);if(!a)return 0;
 U count=array_count(a);if(count>MAX_BOARD){release(a);return 0;}
 for(U j=0;j<count;j++) {
  Rec target=array_at(a,j),rec=target?reference(target,10):0;
  if(!target || !rec){release(rec);release(target);release(a);return 0;}
  Rec player=hs15_is_recruit(rec)?reference(rec,5):0;
  int acquired=player?hs15_has(s,player):0;release(player);
  if(hs15_is_recruit(rec) && scalar(rec,9,0x108)<5 && !acquired) {
   if(!hs28_choice(&pool[*n],rec,target,team,eval,s,deadline)){release(rec);release(target);release(a);return 0;}
   (*n)++;
  } else (*fixed)++;
  release(rec);release(target);
 }
 release(a);return 1;
}
static int hs28_make_board_plan(Rec team,U eval,const Hs15Snapshot *s,Hs28Choice pool[MAX_BOARD],int *n,int deadline) {
 int fixed=0,vacancy[22],optional[22],general=hs28_rooms(s,vacancy,optional);
 if(general<0 || !hs28_board(team,eval,s,pool,n,&fixed,deadline))return 0;
 /* Reuse exact allocation scores within this weekly pass; on reload or a
    new week the native scorer above reconstructs the current values. */
 Hs28TeamPlan old,now={0};
 if(hs28_plan_load(&old,team,eval) && hs28_scope(&now,team,eval) && old.week==now.week && old.type==now.type)
  for(int i=0;i<*n;i++)for(int j=0;j<old.count;j++)
   if(pool[i].table==old.entry[j].prior.table && pool[i].rid==old.entry[j].prior.rid)
    pool[i].native_score=old.entry[j].prior.native_score;
 hs28_plan_team(pool,*n,vacancy,optional,general,50-fixed,team,eval,s);
 if(faulted)return 0;hs28_plan_store(pool,*n,team,eval);return !faulted;
}
/* A shared source reservation is only a legal offered HS primary with native
   productive action assignment. It is not an incoming player or guarantee. */
static int test29_hs_reserved(Rec team,U eval,int reserve[22]) {
 memset(reserve,0,22*sizeof(*reserve));
 if(!team || !eval || mode!=1 || faulted || !hs28_pair_window(eval))return 1;
 unsigned tid=scalar(team,383,0x100);if(tid>=138){portal_fault("class_source_team");return 0;}
 Rec user=reference(team,407);if(user){release(user);return 1;}
 Hs28TeamPlan scope={0};if(!hs28_scope(&scope,team,eval)){portal_fault("class_source_scope");return 0;}
 Test29ClassCache *c=&test29_class_cache[tid];
 if(c->building)return 1; /* Native read-only eligibility may reenter. */
 if(c->valid && hs28_scope_matches(&c->scope,&scope) && c->scope.week==scope.week && c->scope.type==scope.type) {
  memcpy(reserve,c->reserve,sizeof(c->reserve));return 1;
 }
 c->valid=0;c->building=1;Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int n=0,ok=0;
 if(!hs15_collect(&shot,team,eval) || !hs28_make_board_plan(team,eval,&shot,pool,&n,0)){portal_fault("class_source_plan");goto done;}
 Rec budget=reference(team,224);unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
 if(assigned>total || total>100000){portal_fault("class_source_budget");goto done;}
 /* HS and portal share this native assigned-hour counter. Validate every
    board action, including non-HS and secured/stale entries, before using
    any HS flag as evidence of allocated attention. */
 Rec board=board_array(team);U board_count=board?array_count(board):MAX_BOARD+1;
 if(!board || board_count>MAX_BOARD){release(board);portal_fault("class_source_board");goto done;}
 unsigned board_allocated=0;int complete=1;
 for(U i=0;i<board_count;i++) {
  Rec target=array_at(board,i);unsigned mask=0;
  if(!target){complete=0;continue;}
  board_allocated+=test31_productive_allocated(target,team,eval,&mask);release(target);
 }
 release(board);
 unsigned allocated_total=0;int slots[50],used=0;
 for(int i=0;i<n;i++)if(pool[i].target) {
  unsigned mask=0,allocated=test31_productive_allocated((Rec)pool[i].target,team,eval,&mask);
  allocated_total+=allocated;
  Hs28Choice *p=&pool[i];if(!p->selected || p->choice!=1 || !p->legal || !p->viable || !p->offered || !allocated || allocated>assigned)continue;
  int seen=0;for(int j=0;j<used;j++)if(slots[j]==p->slot)seen=1;
  if(!seen && used<50){slots[used++]=p->slot;reserve[p->pos]++;}
 }
 if(!complete || board_allocated>assigned || allocated_total>board_allocated) {
  /* Saved action flags can outlive the native assigned-hour reset. Do not
     certify them as funded, and cache that inconclusive observation until
     the next explicit funding/ownership invalidation rather than rescan
     the whole class for every portal candidate. */
  memset(reserve,0,22*sizeof(*reserve));
  scope.valid=1;c->scope=scope;memcpy(c->reserve,reserve,sizeof(c->reserve));c->valid=1;ok=1;
  log_line("TEST29_HS_SOURCE_SKIP,%u,%u,%u,%s",tid,board_allocated,assigned,
   complete?"ASSIGNED_BUDGET_MISMATCH":"BOARD_ASSIGNMENT_INCOMPLETE");goto done;
 }
 int reserved_count=0;
 for(int p=0;p<22;p++) {
  /* A legal portal quality upgrade can use the existing general allowance
     while this paid HS primary keeps its source seat. Portal arrivals must
     not erase that reservation merely by occupying the projected vacancy.
     Annual renewal authority is not a physical portal-seat reservation:
     clamp only these paid-source reservations to real remaining vacancy. */
  int base=shot.returning[p]+shot.hs[p];
  int free=shot.ideal[p]>base?shot.ideal[p]-base:0;
  if(reserve[p]>free)reserve[p]=free;
  reserved_count+=reserve[p];
 }
 scope.valid=1;c->scope=scope;memcpy(c->reserve,reserve,sizeof(c->reserve));c->valid=1;ok=1;
 log_line("TEST29_HS_SOURCE_RESERVE,%u,%d,%u,%u,%u,NATIVE_ASSIGNED_PRIMARY",tid,reserved_count,allocated_total,board_allocated,assigned);
done:
 c->building=0;return ok;
}
/* Filled quantity openings stay closed to ordinary backups. A material
   quality challenger must instead earn a new optional plan and pass the
   same prospective shared-budget check; it never inherits the old seat. */
static int hs28_opening_block(const Hs15Snapshot *s,int pos,int stars,int ovr) {
 if(s->annual_valid && s->annual[pos]>s->hs[pos])return 0;
 return s->count[pos]>=s->ideal[pos] && s->hs[pos]>0 &&
  (!hs29_quality_extra(s,pos,stars,ovr) || hs15_snapshot_values(s,pos,stars,ovr)!=0);
}
static int hs28_recruit_opening_block(const Hs15Snapshot *s,Rec recruit,int pos) {
 if(s->annual_valid && s->annual[pos]>s->hs[pos])return 0;
 if(s->count[pos]<s->ideal[pos] || !s->hs[pos])return 0;
 Rec player=reference(recruit,5);if(!player)return 1;
 int stars=(int)scalar(player,231,0x108),ovr=(int)scalar(player,191,0x100);release(player);
 return hs28_opening_block(s,pos,stars,ovr);
}
static int hs28_commit_block(Rec recruit,Rec team,U eval,const char *site) {
 if(mode!=1 || faulted || !hs15_is_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 Hs15Snapshot shot;if(!hs15_collect(&shot,team,eval)){portal_fault("HS_pair_projection");return 0;}
 int pos=position(recruit);if(pos<0 || pos>=22){portal_fault("HS_pair_position");return 0;}
 if(hs28_recruit_opening_block(&shot,recruit,pos))return 16;
 int deadline=strncmp(site,"SIGN_",5)==0 || strncmp(site,"SELECT_",7)==0 || strncmp(site,"NSD_",4)==0;
 Hs28Choice pool[MAX_BOARD];int n=0;
 if(!hs28_make_board_plan(team,eval,&shot,pool,&n,deadline)){portal_fault("HS_pair_plan");return 0;}
 int own=-1;for(int i=0;i<n;i++)if(hs28_identity(&pool[i],recruit)){own=i;break;}
 if(own<0)return 0; /* No fabricated board identity; native validation follows. */
 if(!pool[own].selected)return deadline?17:16;
 if(pool[own].choice!=2)return 0;
 for(int i=0;i<n;i++)if(pool[i].selected && pool[i].slot==pool[own].slot && pool[i].choice==1) {
  int viable=pool[i].legal && pool[i].offered && pool[i].viable;
  if(deadline && viable) {
   Rec a=board_array(team),primary=0;if(!a){portal_fault("HS_pair_deadline_board");return 0;}
   U count=array_count(a);
   for(U j=0;j<count;j++) {
    Rec t=array_at(a,j),r=t?reference(t,10):0;release(t);
    if(hs28_identity(&pool[i],r)){primary=r;break;}release(r);
   }
   release(a);Rec winner=primary?hs28_nsd_destination(primary,eval):0;
   viable=winner && q((U)winner+0x10)==q((U)team+0x10) && row(winner)==row(team);
   release(winner);release(primary);
  }
  if(viable){log_line("HS28_PAIR_DEFER,%s,%u,%u,%u,%d,%d",site,scalar(team,383,0x100),row(recruit),pool[i].rid,pool[own].slot,deadline);return 17;}
 }
 return 0;
}
/* Only called before native board iterators exist or after the weekly batch.
   Reacquire each selected record before a removal; no stale array iterators. */
static const char *hs31_withdraw_reason(const Hs15Snapshot *s,const Hs28Choice *p,
 Rec recruit,Rec team,U eval,int closed) {
 if(closed)return s->annual_valid && s->annual[p->pos]<=s->hs[p->pos]?
  "ANNUAL_INTAKE_FILLED":"OPENING_FILLED";
 if(academy29_hs_block(recruit,team))return "ACADEMY_SOURCE_OR_STAR_POLICY";
 if(hs15_snapshot_values(s,p->pos,p->stars,p->ovr)!=0)return "ROSTER_OR_EXCEPTION_CAPACITY";
 if(locked_out(recruit,team,eval))return "RACE_ACCESS";
 if(!native_recruit_eligible(eval,team,recruit))return "NATIVE_ELIGIBILITY";
 if(!p->viable)return "NONVIABLE_RACE";
 if(p->native_score<=0)return "NONPOSITIVE_NATIVE_SCORE";
 if(p->market_known && !p->market_protected && !p->market_planned)return "MARKET_NOT_AUTHORIZED";
 if(p->crowd_penalty>=45)return "CROWDED_RACE_CLOSE_ALTERNATIVE";
 return "LOWER_CURRENT_PLAN_PRIORITY";
}
static int hs28_prune(Rec team,U eval,int closed_only) {
 if(mode!=1 || faulted || !hs28_pair_window(eval))return 1;
 Rec user=reference(team,407);if(user){release(user);return 1;}
 Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int n=0;
 if(!hs15_collect(&shot,team,eval) || !hs28_make_board_plan(team,eval,&shot,pool,&n,0)){portal_fault("HS_pair_prune_read");return 0;}
 for(int i=0;i<n && !faulted;i++) {
  int closed=hs28_opening_block(&shot,pool[i].pos,pool[i].stars,pool[i].ovr);
  /* Quality alone does not preserve an unselected/ineligible extra after
     its prior quantity opening fills. The current optional plan is required. */
  if((!shot.annual_valid || shot.annual[pool[i].pos]<=shot.hs[pool[i].pos]) &&
     shot.count[pool[i].pos]>=shot.ideal[pool[i].pos] && shot.hs[pool[i].pos]>0 &&
     (!pool[i].selected || !pool[i].legal))closed=1;
  if(!closed && (closed_only || pool[i].selected))continue;
  Rec a=board_array(team),rec=0,target=0;if(!a){portal_fault("HS_pair_prune_board");return 0;}
  U count=array_count(a);
  for(U j=0;j<count;j++) {
   Rec t=array_at(a,j),r=t?reference(t,10):0;
   if(hs28_identity(&pool[i],r)){rec=r;target=t;break;}
   release(r);release(t);
  }
  release(a);if(!rec){release(target);continue;}
  if(scalar(rec,9,0x108)>=5){release(rec);release(target);continue;}
  U manager=q(eval+0xc8);if(!manager){release(rec);release(target);portal_fault("HS_pair_prune_manager");return 0;}
  unsigned before=scalar(rec,14,0x100),status=scalar(target,12,0x108);
  const char *reason=hs31_withdraw_reason(&shot,&pool[i],rec,team,eval,closed);
  U candidate[6]={(U)rec,0,0,0,0,0};
  int rc=((int(*)(U,Rec*,U*))(game_base+0x081a7b10))(manager,&team,candidate);cache_note_removal();
  test29_class_invalidate(team);
  unsigned after=scalar(rec,14,0x100);Rec still=reference(target,10);
  int ok=!rc && !still && after==before-((status>=2 && status<=4 && before>0)?1:0);
  log_line("HS28_PAIR_WITHDRAW,%u,%u,%d,%s",scalar(team,383,0x100),row(rec),pool[i].pos,closed?"OPENING_FILLED":"PAIR_REASSESSMENT");
  log_line("HS31_PAIR_WITHDRAW_REASON,%u,%u,%d,%s,%d,%d,%d",scalar(team,383,0x100),row(rec),
   pool[i].pos,reason,pool[i].legal,pool[i].viable,pool[i].native_score);
  release(still);release(rec);release(target);
  if(!ok){portal_fault("HS_pair_prune_result");return 0;}
 }
 return !faulted;
}
static unsigned char hs28_changed[138];
static void hs28_note(Rec recruit,Rec team) {
 test29_class_invalidate(team);
 if(hs15_is_recruit(recruit)){hs28_nsd_reset();unsigned tid=scalar(team,383,0x100);if(tid<138)hs28_changed[tid]=1;}
}
static void hs28_after_week(U eval) {
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;release(f);
 if(!teams)return;
 U count=array_count(teams);if(count>138){release(teams);portal_fault("HS_pair_team_bounds");return;}
 for(U i=0;i<count && !faulted;i++)if(hs28_changed[i]) {
  Rec team=array_at(teams,i);if(!team){portal_fault("HS_pair_team");break;}
  hs28_prune(team,eval,1);release(team);hs28_changed[i]=0;
 }
 release(teams);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest29ClassPlan(Rec team,U eval,int *out) {
 Hs15Snapshot s;Test29ClassInput in={0};Test29ClassPlan plan;
 if(!hs15_collect(&s,team,eval) || !test29_class_cohorts(team,&s,in.junior))return 0;
 in.annual_valid=s.annual_valid;in.annual_target=s.annual_target;
 in.academy=test29_class_academy(scalar(team,383,0x100));
 in.general=hs28_rooms(&s,in.vacancy,in.optional);if(in.general<0)return 0;
 for(int p=0;p<22;p++) {
  in.ideal[p]=s.ideal[p];in.returning[p]=s.returning[p];in.secured[p]=s.hs[p];
  in.candidates[p]=in.vacancy[p]+in.optional[p];
 }
 test29_class_plan(&in,&plan);
 out[0]=plan.target;out[1]=plan.secured;out[2]=plan.replacement;out[3]=plan.available;
 out[4]=plan.planned;out[5]=plan.shortfall;out[6]=plan.replacement_exception;
 for(int p=0;p<22;p++){out[7+p]=plan.vacancy[p];out[29+p]=plan.optional[p];out[51+p]=in.junior[p];}
 return 1;
}
__declspec(dllexport) int DriTest29HsReserved(Rec team,U eval,int *out){return test29_hs_reserved(team,eval,out);}
__declspec(dllexport) void DriTest29ClassInvalidate(Rec team){test29_class_invalidate(team);}
__declspec(dllexport) void DriTestHsPlanReset(void){hs31_native_score_invalidate(0);memset(hs28_team_plans,0,sizeof(hs28_team_plans));memset(test29_class_cache,0,sizeof(test29_class_cache));hs34r4_preload_reset();}
__declspec(dllexport) int DriTestHs34PursuitState(Rec r,Rec target,Rec team,U eval) {
 Hs15Snapshot s;Hs28Choice p;
 if(!hs15_collect(&s,team,eval) || !hs28_choice(&p,r,target,team,eval,&s,0))return -1;
 return p.pursuit_state;
}
__declspec(dllexport) void DriTestHsPlanStore(const Hs28Choice *p,int n,Rec t,U e){hs28_plan_store(p,n,t,e);}
__declspec(dllexport) int DriTestHsPlanRole(Rec r,Rec t,U e,int field) {
 Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int n=0;
 if(!hs15_collect(&shot,t,e) || !hs28_make_board_plan(t,e,&shot,pool,&n,0))return -1;
 for(int i=0;i<n;i++)if(hs28_identity(&pool[i],r))return field==0?pool[i].choice:field==1?pool[i].weight:field==2?pool[i].slot:pool[i].native_score;
 return 0;
}
/* Test34R2 numeric observation: one real board-plan evaluation, no writes to
   offers, budgets, influence, ownership or native commitment state. */
__declspec(dllexport) int DriTest34R2PlanSnapshot(Rec t,U e,int *out) {
 Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int n=0;
 if(!hs15_collect(&shot,t,e) || !hs28_make_board_plan(t,e,&shot,pool,&n,0))return -1;
 unsigned tid=scalar(t,383,0x100);if(tid>=138)return -1;
 const Test29ClassCache *c=&test29_class_cache[tid];
 out[0]=c->target;out[1]=c->secured;out[2]=c->planned;out[3]=c->shortfall;
 for(int i=0;i<n;i++) {
  out[4+i*4]=(int)pool[i].rid;out[5+i*4]=pool[i].choice;
  out[6+i*4]=pool[i].slot;out[7+i*4]=pool[i].native_score;
 }
 return n;
}
__declspec(dllexport) int DriTestHsPairDecision(Rec r,Rec t,U e,int deadline){return hs28_commit_block(r,t,e,deadline?"SIGN_FIXTURE":"WEEKLY");}
__declspec(dllexport) int DriTestHsPairPrune(Rec t,U e,int closed){return hs28_prune(t,e,closed);}
#endif
