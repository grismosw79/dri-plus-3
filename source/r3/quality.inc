/* Candidate-phase snapshot is thread-local and used only by the two verified
   scoring call sites. Signing decisions always reconstruct the current room. */
static __declspec(thread) int rc4_compare_team=-1;
static __declspec(thread) const char *rc4_compare_site;
#define DRI_RC4_CHANGED(need,old,nv,ov,val) do { if(rc4_compare_site)log_line("RC4_QB_COMPARE,%s,%u,%u,%d,%d,%u,%d,%d,%u,%d,%d",rc4_compare_site,rc4.year,rc4.week,rc4_compare_team,(need)->ordinal,(old)->rid,(old)->raw_ovr,(ov),(nv)->rid,(nv)->raw_ovr,(val)); } while(0)
#define DRI_RC4_ELITE(rec) (rc4.window && rc4_elite(rec))
#include "portal_allocation.h"
static void market29_portal_begin(U eval);
static int market29_portal_admission_block(Rec recruit,Rec team,U eval);
static int hs32_admission_block(Rec recruit,Rec team,U eval);
static int test29r2_portal_committable(Rec recruit,Rec team,U eval,const char *site);
static void market29_portal_apply(PortalChoice *pool,int n,Rec team,U eval,const PortalQuality rooms[22],int prestige);
static void market29_portal_recovery(Rec team,U eval);
#define PORTAL_SCORE_SLOTS 16384
#define PORTAL_PLAN_LIMIT 12000
/* Board and funding are separate native reactions sharing evaluator+0x340.
   They need not share an OS thread. Retain exact scoring provenance in a
   bounded shared ledger; only numeric identities/values are used under lock.
   At most eight evaluator ledgers are allocated lazily (2 MiB of entries).
   Each evaluator's next prepare invalidates its preceding team/generation. */
#define PORTAL_FUNDING_EVALUATORS 8
typedef struct {U rec;int base,score;} PortalFundingScore;
typedef struct {
 U eval,team,generation;PortalFundingScore *scores;
} PortalFundingLedger;
static PortalFundingLedger funding_ledgers[PORTAL_FUNDING_EVALUATORS];
static SRWLOCK funding_provenance_lock=SRWLOCK_INIT;
static U funding_provenance_generation;
static void portal_funding_invalidate(U eval) {
 AcquireSRWLockExclusive(&funding_provenance_lock);
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(funding_ledgers[j].eval==eval) {
  funding_ledgers[j].team=0;funding_ledgers[j].generation=++funding_provenance_generation;
 }
 ReleaseSRWLockExclusive(&funding_provenance_lock);
}
static PortalFundingLedger *portal_funding_begin(U eval,U team,U *generation) {
 PortalFundingLedger *ledger=0;
 AcquireSRWLockExclusive(&funding_provenance_lock);
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(funding_ledgers[j].eval==eval){ledger=&funding_ledgers[j];break;}
 if(!ledger)for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(!funding_ledgers[j].eval){ledger=&funding_ledgers[j];break;}
 /* Repeated save loads may create new evaluators. Reuse the oldest generation
    instead of permanently exhausting the bounded registry. An evicted pass
    safely falls back to native funding; its old writers cannot repopulate it. */
 if(!ledger) {
  ledger=&funding_ledgers[0];
  for(int j=1;j<PORTAL_FUNDING_EVALUATORS;j++)if(funding_ledgers[j].generation<ledger->generation)ledger=&funding_ledgers[j];
 }
 if(ledger && !ledger->scores)ledger->scores=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(PortalFundingScore)*PORTAL_SCORE_SLOTS);
 if(ledger && ledger->scores) {
  memset(ledger->scores,0,sizeof(PortalFundingScore)*PORTAL_SCORE_SLOTS);
  ledger->eval=eval;ledger->team=team;ledger->generation=++funding_provenance_generation;*generation=ledger->generation;
 } else ledger=0;
 ReleaseSRWLockExclusive(&funding_provenance_lock);return ledger;
}
static PortalFundingScore *portal_funding_slot(PortalFundingLedger *ledger,U rec,int create) {
 unsigned k=(unsigned)((rec>>4)*2654435761u)&(PORTAL_SCORE_SLOTS-1);
 for(unsigned j=0;j<PORTAL_SCORE_SLOTS;j++,k=(k+1)&(PORTAL_SCORE_SLOTS-1)) {
  PortalFundingScore *s=&ledger->scores[k];
  if(s->rec==rec)return s;
  if(!s->rec){if(create)s->rec=rec;return create?s:0;}
 }
 return 0;
}
static void portal_funding_store(PortalFundingLedger *ledger,U eval,U team,U generation,U rec,int base,int score) {
 if(!ledger)return;
 AcquireSRWLockExclusive(&funding_provenance_lock);
 if(ledger->eval==eval && ledger->team==team && ledger->generation==generation) {
  PortalFundingScore *s=portal_funding_slot(ledger,rec,1);
  if(s){s->base=base;s->score=score;}
 }
 ReleaseSRWLockExclusive(&funding_provenance_lock);
}
/* Return the first unmatched pool index plus one. Validate the entire snapshot
   under one lock; this function never dereferences a game record or logs. */
static int portal_funding_bases(U eval,U team,PortalChoice *pool,int n) {
 int missing=0;PortalFundingLedger *ledger=0;
 AcquireSRWLockExclusive(&funding_provenance_lock);
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(funding_ledgers[j].eval==eval && funding_ledgers[j].team==team){ledger=&funding_ledgers[j];break;}
 for(int j=0;j<n;j++) {
  PortalFundingScore *s=ledger?portal_funding_slot(ledger,pool[j].rec,0):0;
  if(!s || s->base<=0 || s->score!=pool[j].score){missing=j+1;break;}
  pool[j].native_base=s->base;
 }
 ReleaseSRWLockExclusive(&funding_provenance_lock);return missing;
}
typedef struct {
 U rec;int score,pos,ovr,role,rank,deficit,native_base,raw_ovr;
} PortalScore;
typedef struct {
 int valid,tid,prestige;U team,eval,frame,fallback_context,fallback_frame;
 PortalFundingLedger *funding;U funding_generation;
 PortalQuality rooms[POSITIONS],current[POSITIONS];int projection_step;int quality_open[POSITIONS];
 int encoded,allocated,hs_valid;Hs15Snapshot hs_snapshot;
 Hs28Choice hs_choices[50];int hs_count;
 PortalScore scores[PORTAL_SCORE_SLOTS];
} PortalCache;
static DWORD quality_tls=TLS_OUT_OF_INDEXES;
static PortalCache *portal_cache(int create) {
 if(quality_tls==TLS_OUT_OF_INDEXES)return 0;
 PortalCache *c=TlsGetValue(quality_tls);
 if(!c && create) {
  c=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*c));
  if(c && !TlsSetValue(quality_tls,c)){HeapFree(GetProcessHeap(),0,c);return 0;}
 }
 return c;
}
static PortalScore *portal_score_entry(PortalCache *c,U rec,int create) {
 unsigned k=(unsigned)((rec>>4)*2654435761u)&(PORTAL_SCORE_SLOTS-1);
 for(unsigned i=0;i<PORTAL_SCORE_SLOTS;i++,k=(k+1)&(PORTAL_SCORE_SLOTS-1)) {
  PortalScore *s=&c->scores[k];
  if(s->rec==rec)return s;
  if(!s->rec){if(create)s->rec=rec;return create?s:0;}
 }
 if(create)portal_fault("score_cache_full");return 0;
}
void DriOnPortalPrepare(Registers *r) {
 if(mode<0 || faulted)return;
 uint64_t phase_start=test29_clock_ms();
 const char *volatile phase="prepare_context";
 __try {
  test31_offer_register(r->r13);
  PortalCache *c=portal_cache(1);if(!c){portal_fault("quality_cache");return;}
  c->valid=0;c->encoded=0;c->allocated=0;c->hs_valid=0;c->hs_count=0;
  c->funding=0;c->funding_generation=0;portal_funding_invalidate(r->r13);
  memset(c->scores,0,sizeof(c->scores));
  U sp=(U)(r+1);
  int portal_pass=*(unsigned char *)(sp+0x42)!=0;
  if(portal_pass){phase="prepare_portal_maintenance";portal_maintenance(r->r13);if(faulted)return;}
  phase="prepare_cpu_school";
  Rec team=(Rec)q(r->r12);if(!team)return;
  Rec user=reference(team,407);if(user){release(user);return;}
  hs31_native_score_invalidate(team);
  phase="prepare_HS_maintenance";
  hs28_maintenance(r->r13);if(faulted)return;
  phase="prepare_school_cleanup";
  if(hs28_pair_window(r->r13))hs28_cleanup_rivals(team,r->r13);if(faulted)return;
  if(portal_pass){
   phase="prepare_portal_market";
   /* Freeze all schools before this school's recovery mutates its board. */
   market29_portal_begin(r->r13);if(faulted)return;
   hs32_offseason_admit(team,r->r13);if(faulted)return;
   market29_portal_recovery(team,r->r13);if(faulted)return;
  }
  if(!portal_pass) {
   phase="prepare_HS_board";
   if(hs28_pair_window(r->r13)) {
    hs32_offseason_admit(team,r->r13);if(faulted)return;
    if(!hs28_prune(team,r->r13,0))return;
    if(!hs15_collect(&c->hs_snapshot,team,r->r13)){portal_fault("HS_coverage_projection");return;}
    c->hs_valid=1;c->team=(U)team;c->eval=r->r13;c->frame=r->rbp;
   }
   test29_phase_log("HS_PREPARE",scalar(team,383,0x100),r->r13,phase_start);
   return;
  }
  phase="prepare_portal_projection";
  c->tid=(int)scalar(team,383,0x100);if(c->tid<0 || c->tid>=138)return;
  c->team=(U)team;c->eval=r->r13;c->frame=r->rbp;c->projection_step=portal_projection_step(c->eval);
  c->funding=portal_funding_begin(c->eval,c->team,&c->funding_generation);
  if(!portal_quality_rooms(c->rooms,c->current,&c->prestige,team,c->eval)){
   portal_fault("prepare_roster");return;
  }
  int hs_reserved[POSITIONS]={0};
  test29_hs_reserved(team,c->eval,hs_reserved);if(faulted)return;
  dri_portal_set_hs_reservations(c->rooms,hs_reserved,c->prestige);
  log_line("PORTAL_EXCEPTION_BUDGET,%d,2,2,1,%u,%s",c->tid,c->rooms[0].exception_states,mode?"ACTIVE":"DIAGNOSTIC");
  log_line("PORTAL_TEAM_PROJECTION,%d,%d,%d,%s",c->tid,c->rooms[0].team_excess,
   dri_portal_flex_budget(c->rooms),mode?"ACTIVE":"DIAGNOSTIC");
  for(int pos=0;pos<POSITIONS;pos++) {
   PortalQuality *room=&c->rooms[pos];
   int rank=0,bonus=0;
   for(int ovr=0;ovr<=99;ovr++) {
    int k=0,b=dri_quality_bonus_candidate(room,c->prestige,portal_projected_ovr(ovr,c->projection_step),ovr,&k);
    if(b>bonus){bonus=b;rank=k;}
   }
   c->quality_open[pos]=bonus>0 || room->flex_allowed;
   int count=room->count,starters=room->starters<count?room->starters:count;
   int best=count?c->current[pos].rating[0]:0,role=starters?c->current[pos].rating[starters-1]:0;
   log_line("PORTAL_PROJECTION,%d,%d,%d,%d,%d,%d,NEXT_SEASON_ESTIMATE",c->tid,pos,count,best,count?room->rating[0]:0,c->projection_step);
   log_line("PORTAL_NEED,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",c->tid,pos,room->ideal,count,room->base_count,role,best,rank,bonus,mode?"ACTIVE":"DIAGNOSTIC");
  }
  c->valid=1;
  test29_phase_log("PORTAL_PREPARE",(unsigned)c->tid,c->eval,phase_start);
 } __except(fail_exception(phase,GetExceptionCode())){}
}
void DriOnPortalScore(Registers *r) {
 if(mode<0 || faulted)return;
 __try {
  U sp=(U)(r+1),caller=q(sp+0x88);
  if(caller!=game_base+0x0818c2e2 && caller!=game_base+0x0817345a)return;
  PortalCache *c=portal_cache(0);if(!c || (!c->valid && !c->hs_valid))return;
  U tr=q(sp+0xa8);if(!tr || q(tr)!=c->team)return;
  Rec recruit=(Rec)q(r->r15);if(!recruit)return;
  /* Retained-board scoring vetoes excess attention before candidate totals.
     This route alone did not cover actual scholarship creation. Test31's
     universal action preflight also enforces the same frozen authorization;
     reweighting a formed vector to minimum1 cannot implement a hard cap. */
  if(hs15_is_recruit(recruit)) {
   if(mode==1 && caller==game_base+0x0817345a && scalar(recruit,9,0x108)<5 &&
      hs32_admission_block(recruit,(Rec)c->team,c->eval)) {
    r->r14=(U)(-(int64_t)(int32_t)r->r12);
    log_line("TEST30_HS_FUNDING_VETO,%u,%u,BEFORE_NATIVE_STAGE_TOTALS",scalar((Rec)c->team,383,0x100),row(recruit));
   }
   return;
  }
  if(!c->valid || !transfer_recruit(recruit))return;
  if(scalar(recruit,9,0x108)>=5)return;
  Rec p=reference(recruit,5);if(!p){portal_fault("score_player");return;}
  int pos=(int)scalar(p,3,0x108),ovr=(int)scalar(p,191,0x100);release(p);
  if(pos<0 || pos>=POSITIONS || ovr<0 || ovr>99){portal_fault("score_values");return;}
  int base=(int32_t)(r->r12+r->r14),rank=0,projected=portal_projected_ovr(ovr,c->projection_step);
  int reason=!portal_academy_allowed(recruit,(unsigned)c->tid)?5:dri_quality_block_candidate(&c->rooms[pos],c->prestige,projected,ovr);
  /* R2 new-entry authorization precedes native funding-vector formation.
     Keep the native base intact in the ledger; only the selection result is
     zero. No formed stage total, budget, action, or existing offer is edited. */
  if(!reason && caller==game_base+0x0817345a && market29_portal_admission_block(recruit,(Rec)c->team,c->eval))reason=6;
  int bonus=base>0?dri_quality_bonus_candidate(&c->rooms[pos],c->prestige,projected,ovr,&rank):0;
  int score=reason?0:base+bonus;
  if(ovr>=80)log_line("R2_NATIVE_BASE,%u,%u,%s,%d,%d,%d",c->tid,row(recruit),
    caller==game_base+0x0818c2e2?"NEW_BOARD":"RETAINED",base,rank,reason);
  if(log30_deep && (base>0 || ovr>=80))
   log_line("PORTAL_SCORE,%d,%u,%d,%d,%d,%d,%d,%d,%d,%s",c->tid,row(recruit),pos,ovr,base,bonus,score,rank,reason,mode?"ACTIVE":"DIAGNOSTIC");
  if(caller==game_base+0x0818c2e2) {
   PortalScore *s=portal_score_entry(c,(U)recruit,1);if(!s)return;
   int role=base>0 && score>0?dri_portal_role_candidate(&c->rooms[pos],c->prestige,projected,ovr):0;
   if(base>0 && score>=1000000){portal_fault("priority_score_range");return;}
   s->score=base>0?score:0;s->native_base=base;s->pos=pos;s->ovr=projected;s->raw_ovr=ovr;s->role=role;s->rank=rank;s->deficit=0;
   if(rank) {
    int cur=rank<=c->rooms[pos].count?c->rooms[pos].rating[rank-1]:0;
    s->deficit=dri_quality_floor[dri_group(c->prestige)][pos][rank-1]-cur*4;
   }
   if(mode==1 && base>0) {
    score=dri_portal_shortlist_score(score,role);c->encoded=1;
   }
  }
  /* The shortlist route may encode its return value. Funding receives the
     original restored total, retained here together with its exact base. */
  if(mode==1) {
   PortalScore *s=caller==game_base+0x0818c2e2?portal_score_entry(c,(U)recruit,0):0;
   portal_funding_store(c->funding,c->eval,c->team,c->funding_generation,(U)recruit,base,s?s->score:score);
  }
  if(mode==1 && base>0)r->r14=(U)(int64_t)(score-(int32_t)r->r12);
 } __except(fail_exception("portal_score",GetExceptionCode())){}
}

/* After all native retention decisions, before the existing-target prefix
   search. Let every unsecured CPU portal target compete for priority again;
   the allocator below protects the chosen existing targets without deleting
   and re-adding them. HS targets and secured commitments keep native rules. */
void DriOnPortalRetain(Registers *r) {
 if(mode!=1 || faulted)return;
 __try {
  PortalCache *c=portal_cache(0);U sp=(U)(r+1),tr=q(r->rbp+0x28);
  if(!c || c->frame!=r->rbp || !tr || q(tr)!=c->team)return;
  Rec rec=(Rec)q(r->rbp-0x58);
  if(c->hs_valid && !*(unsigned char *)(sp+0x42)) {
   if(rec && hs15_is_recruit(rec) && scalar(rec,9,0x108)<5) {
    *(unsigned char *)(sp+0x44)=1;*(unsigned char *)(sp+0x40)=1;
   }
   return;
  }
  if(!c->valid || !*(unsigned char *)(sp+0x42))return;
  if(rec && hs15_is_recruit(rec)) {
   if(hs33_offseason_retain(rec,(Rec)q(r->rbp-0x38),(Rec)c->team,c->eval)) {
    *(unsigned char *)(sp+0x44)=0;*(unsigned char *)(sp+0x40)=0;
   }
   return;
  }
  if(!rec || !transfer_recruit(rec) || scalar(rec,9,0x108)>=5)return;
  *(unsigned char *)(sp+0x44)=1;
  *(unsigned char *)(sp+0x40)=1;
 } __except(fail_exception("portal_retain",GetExceptionCode())){}
}

typedef struct {U rec;int32_t values[10];} PortalNativeCandidate;
_Static_assert(sizeof(PortalNativeCandidate)==48 && offsetof(PortalNativeCandidate,values[8])==40,"Candidate ABI");
static int portal_choice_index(PortalChoice *pool,int n,U rec) {
 for(int j=0;j<n;j++)if(pool[j].rec==rec)return j;
 return -1;
}
static void portal_choice_set(PortalChoice *p,PortalScore *s,int index,int existing) {
 *p=(PortalChoice){.pos=s->pos,.role=s->role,.score=s->score,.deficit=s->deficit,
  .existing=existing,.index=index,.rid=row((Rec)s->rec),.rec=s->rec,.ovr=s->ovr,
  .native_base=s->native_base,.plan_score=s->score,.raw_ovr=s->raw_ovr};
 p->stage=(int)scalar((Rec)s->rec,9,0x108);
 p->commit_score=(int)scalar((Rec)s->rec,3,0x100);
}
/* Read only. Initial school-list interest is not evidence of an actual offer.
   Keep the conservative strongest listed influence as the opponent measure;
   a real offer outside a reopened list uses its already-earned target total.
   Recruit/RecruitTarget ordinals come from the captured native schema. */
static int portal_choice_race(PortalChoice *p,Rec target,Rec team,U eval) {
 p->offered=0;p->influence=0;p->leader=0;p->race_ok=0;
 p->stage=(int)scalar((Rec)p->rec,9,0x108);
 p->commit_score=(int)scalar((Rec)p->rec,3,0x100);
 if(p->stage<0 || p->stage>6 || p->commit_score<0 || p->commit_score>1023)return 0;
 if(p->stage>=5)return 1;
 unsigned earned=0;
 if(target && p->existing) {
 Rec linked=reference(target,10);int match=portal_same_record(linked,(Rec)p->rec);release(linked);
 if(!match)return 0;
 unsigned status=scalar(target,12,0x108);earned=scalar(target,8,0x100);
 if(status>4 || earned>65535)return 0;
 p->offered=status>=2;
 }
 unsigned tid=scalar(team,383,0x100),seen[64];int found=0;
 Rec top=array_field((Rec)p->rec,13);if(!top)return 0;
 U count=array_count(top);if(count>64){release(top);return 0;}
 for(U j=0;j<count;j++) {
  Rec entry=array_at(top,j);if(!entry){release(top);return 0;}
  unsigned school=scalar(entry,0,0x100),points=scalar(entry,1,0x100);release(entry);
  if(school>=138 || points>65535){release(top);return 0;}
  for(U k=0;k<j;k++)if(seen[k]==school){release(top);return 0;}
  seen[j]=school;
  if(points>(unsigned)p->leader)p->leader=(int)points;
  if(school==tid){p->influence=(int)points;found=1;}
 }
 release(top);
 if(!found && p->offered)p->influence=(int)earned;
 if(p->influence>p->leader)p->leader=p->influence;
 p->race_ok=!locked_out((Rec)p->rec,team,eval);
 return 1;
}
/* Scan the team's board once. Target identities can use distinct retained
   handles for the same table row, so compare native record identities. */
static int portal_board_races(PortalChoice *pool,int n,Rec team,U eval) {
 Rec targets=board_array(team);if(!targets)return 0;
 U count=array_count(targets);if(count>MAX_BOARD){release(targets);return 0;}
 int seen[MAX_BOARD],matched=0,ok=1;
 for(U j=0;j<count;j++) {
  Rec target=array_at(targets,j),rec=target?reference(target,10):0;
  if(!target || !rec){release(rec);release(target);ok=0;break;}
  int k=portal_choice_index(pool,n,(U)rec);
  if(k<0)for(int z=0;z<n;z++)if(pool[z].existing && portal_same_record((Rec)pool[z].rec,rec)){k=z;break;}
  if(k>=0 && pool[k].existing) {
   for(int z=0;z<matched;z++)if(seen[z]==k)ok=0;
   if(ok){seen[matched++]=k;ok=portal_choice_race(&pool[k],target,team,eval);}
  }
  release(rec);release(target);if(!ok)break;
 }
 release(targets);
 if(ok)for(int j=0;j<n;j++)if(!pool[j].existing && !portal_choice_race(&pool[j],0,team,eval)){ok=0;break;}
 return ok;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestPortalChoiceRace(PortalChoice *p,Rec target,Rec team,U eval) {
 return portal_choice_race(p,target,team,eval);
}
#endif
#include "market29_portal.inc"
#include "portal_commitment.inc"
#include "test29r1_capacity.inc"
#include "hs_coverage.inc"
/* Executes after native builds/sorts the replacement vector, before consuming
   candidates. All transient scores are restored even if behavior was disabled.
   Native add/remove operations, capacity and eligibility remain in control. */
void DriOnPortalAllocate(Registers *r) {
 PortalChoice *pool=0;PortalNativeCandidate *copy=0;
 __try {
  PortalCache *c=portal_cache(0);U sp=(U)(r+1),tr=q(r->rbp+0x28);
  if(!*(unsigned char *)(sp+0x42)){hs25_allocate(r);return;}
  if(!c || !c->valid || c->frame!=r->rbp || !tr || q(tr)!=c->team)return;
  U vec=q(r->rbp-0x68);if(!vec){portal_fault("allocation_vector");return;}
  U begin=q(vec),end=q(vec+8),count=q(sp+0x68);
  if(end<begin || (end-begin)%48 || count!=(end-begin)/48 || count>PORTAL_PLAN_LIMIT) {
   portal_fault("allocation_vector_bounds");return;
  }
  PortalNativeCandidate *a=(void *)begin;
  if(c->encoded) {
   for(U i=0;i<count;i++) {
    PortalScore *s=portal_score_entry(c,a[i].rec,0);
    if(s && a[i].values[8]>0)a[i].values[8]=s->score;
   }
   c->encoded=0;
  }
  if(mode!=1 || faulted || c->allocated)return;
  U rem_begin=q(sp+0x78),rem_end=q(r->rbp-0x80),board_count=q(sp+0x50);
  int capacity=i32(r->rbp+0x30);
  if(rem_end<rem_begin || (rem_end-rem_begin)%48 || (rem_end-rem_begin)/48>MAX_BOARD ||
     board_count>MAX_BOARD || capacity<0 || capacity>MAX_BOARD || (rem_end-rem_begin)/48>board_count) {
   portal_fault("allocation_board_bounds");return;
  }
  int nr=(int)((rem_end-rem_begin)/48),fixed=(int)board_count-nr,available=capacity-fixed;
  if(available<0)available=0;
  PortalNativeCandidate *rem=(void *)rem_begin;
  pool=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+nr+1)*sizeof(*pool));
  copy=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*copy));
  if(!pool || !copy){portal_fault("allocation_memory");goto cleanup;}
  int n=0;
  for(U i=0;i<count;i++) {
   PortalScore *s=portal_score_entry(c,a[i].rec,0);
   if(!s || s->score<=0 || !s->role || a[i].values[8]<=0)continue;
   if(portal_choice_index(pool,n,s->rec)>=0){portal_fault("allocation_duplicate");goto cleanup;}
   portal_choice_set(&pool[n++],s,(int)i,0);
  }
  for(int i=0;i<nr;i++) {
   PortalScore *s=portal_score_entry(c,rem[i].rec,0);
   if(!s || s->score<=0 || !s->role)continue;
   int k=portal_choice_index(pool,n,s->rec);
   if(k>=0)pool[k].existing=1;
   else portal_choice_set(&pool[n++],s,-1,1);
  }
  if(!portal_board_races(pool,n,(Rec)c->team,c->eval)){portal_fault("allocation_race");goto cleanup;}
  market29_portal_apply(pool,n,(Rec)c->team,c->eval,c->rooms,c->prestige);if(faulted)goto cleanup;
  dri_portal_order(pool,n,available,c->rooms,c->prestige);
  portal_commit_plan_store(pool,n,(Rec)c->team,c->eval,"BOARD");
  /* Plan all operations before mutating either vector. Main-vector reordering
     is a permutation: references and all score components remain owned once. */
  U written=0;int chosen=0,retained=0,chosen_role[4]={0};
  for(int j=0;j<n;j++) {
   PortalChoice *p=&pool[j];
   if(p->index>=0)copy[written++]=a[p->index];
   if(p->selected){chosen++;retained+=p->existing;chosen_role[p->role]++;}
  }
  /* Native HS candidates retain their relative order and eligibility. */
  for(U i=0;i<count;i++)if(a[i].values[8]>0 && portal_choice_index(pool,n,a[i].rec)<0)copy[written++]=a[i];
  for(U i=0;i<count;i++)if(a[i].values[8]<=0)copy[written++]=a[i];
  if(written!=count){portal_fault("allocation_permutation");goto cleanup;}
  memcpy(a,copy,count*sizeof(*a));
  int kept=0;
  for(int i=0;i<nr;i++) {
   int k=portal_choice_index(pool,n,rem[i].rec);
   if(k>=0 && pool[k].selected)release((Rec)rem[i].rec);
   else {if(kept!=i)rem[kept]=rem[i];kept++;}
  }
  /* Preserve native ordering of replacement candidates. Chosen portal targets
     are now protected; every remaining replacement is outside the selected set. */
  memset(rem+kept,0,(nr-kept)*sizeof(*rem));
  *(U *)(r->rbp-0x80)=rem_begin+(U)kept*48;
  c->allocated=1;
  log_line("PORTAL_ALLOCATION,%d,%llu,%d,%d,%d,%d,%d,%d,%d,%d,ACTIVE",c->tid,(unsigned long long)board_count,capacity,fixed,n,chosen,retained,chosen_role[1],chosen_role[2],chosen_role[3]);
  if(log30_deep) {
   for(int j=0;j<n;j++) {
    PortalChoice *p=&pool[j];
    log_line("PORTAL_PRIORITY,%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%s",c->tid,p->rid,p->pos,p->role,p->score,p->deficit,p->round,p->existing,p->selected,j+1,p->selected?"SELECTED":"CAPACITY");
    log_line("PORTAL_SLOT,%d,%u,%d,%d,%d,%d,%d,%d",c->tid,p->rid,p->pos,p->slot,p->role,p->round,p->ovr,p->selected);
    log_line("PORTAL_PLAN,BOARD,%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",c->tid,p->rid,p->pos,p->score,p->native_base,p->plan_score,p->offered,p->influence,p->leader,p->commit_score,p->stage,p->race_ok,p->existing,p->selected);
   }
  }
  for(int pos=0;pos<POSITIONS;pos++) {
   int scored[4]={0},shortlisted[4]={0},selected[4]={0};
   for(int j=0;j<PORTAL_SCORE_SLOTS;j++) {
    PortalScore *s=&c->scores[j];if(s->rec && s->pos==pos && s->role && s->score>0)scored[s->role]++;
   }
   for(int j=0;j<n;j++)if(pool[j].pos==pos) {shortlisted[pool[j].role]++;if(pool[j].selected)selected[pool[j].role]++;}
   for(int role=1;role<=3;role++)log_line("PORTAL_COVERAGE,%d,%d,%d,%d,%d,%d,%s",c->tid,pos,role,scored[role],shortlisted[role],selected[role],selected[role]?"COVERED":shortlisted[role]?"CAPACITY":scored[role]?"SHORTLIST":"NO_ELIGIBLE_CANDIDATE");
  }
cleanup:;
 } __except(fail_exception("portal_allocate",GetExceptionCode())){}
 if(copy)HeapFree(GetProcessHeap(),0,copy);
 if(pool)HeapFree(GetProcessHeap(),0,pool);
}
void DriOnPortalQuota(Registers *r) {
 if(mode<0 || faulted)return;
 __try {
  PortalCache *c=portal_cache(0);if(!c || (!c->valid && !c->hs_valid))return;
  U tr=q(r->rbp+0x38);if(!tr || q(tr)!=c->team || q(r->rbp+0x30)!=c->eval)return;
  int pos=(int)r->r8;if(pos<0 || pos>=POSITIONS)return;
  /* Shortlisting used to discard eligible depth before the all-position plan.
     Keep the bounded native pool; the allocator still enforces board capacity.
     This is a transient candidate quota, not offers or funded pursuits. */
  if(c->hs_valid && c->hs_snapshot.count[pos]>=c->hs_snapshot.ideal[pos])return;
  if(mode==1 && (int32_t)r->rcx<PORTAL_PLAN_LIMIT)r->rcx=PORTAL_PLAN_LIMIT;
 } __except(fail_exception("portal_quota",GetExceptionCode())){}
}
void DriOnPortalSelect(Registers *r) {
 if(mode<0 || faulted || !(r->rax&255))return;
 __try {
  U context=q(r->rbp+0x67),rr=q(r->rbp+0x77);
  if(!rr)return;
  Rec recruit=(Rec)q(rr),team=(Rec)q(r->rbp-0x39);
  int blocked=cpu_recruit_decision(recruit,team,context,"SELECT_LIST",1);
  U caller=q(r->rbp+0x5f);
  if(mode==1 && !faulted && recruit && scalar(recruit,2,0x108)<=7 && scalar(recruit,9,0x108)<5 &&
     (caller==game_base+0x081b592f || caller==game_base+0x081b462b)) {
   Rec winner=portal_nsd_winner(recruit,context,0);
   if(!faulted)blocked=!winner || !portal_same_record(winner,team);
   release(winner);
  }
  if(blocked && !faulted && mode==1)r->rax&=~(U)255;
 } __except(fail_exception("portal_select",GetExceptionCode())){}
}
int DriOnPortalFallbackStart(Registers *r) {
 if(mode<0 || faulted)return 0;
 __try {
  PortalCache *c=portal_cache(1);if(!c){portal_fault("fallback_cache");return 0;}
  c->fallback_context=r->rcx;c->fallback_frame=r->rbp;
  U sp=(U)(r+1);
  U caller=q(sp+0x108);
  if(mode==1 && r->r8 && r->rdx && (caller==game_base+0x081b595a || caller==game_base+0x081b4662)) {
   Rec recruit=(Rec)q(r->r8);
   if(recruit && scalar(recruit,2,0x108)<=7 && scalar(recruit,9,0x108)<5) {
    Rec winner=portal_nsd_winner(recruit,r->rcx,1);
    if(faulted || mode!=1){release(winner);return 0;}
    *(Rec*)r->rdx=winner;r->rax=r->rdx;return 1;
   }
  }
  return 0;
 } __except(fail_exception("portal_fallback_start",GetExceptionCode())){return 0;}
}
int DriOnPortalFallback(Registers *r) {
 if(mode<0 || faulted)return 0;
 __try {
  PortalCache *c=portal_cache(0);if(!c || c->fallback_frame!=r->rbp)return 0;
  U array=q(r->rbp-0x69);Rec recruit=(Rec)q(r->rbp-0x21);
  Rec team=(Rec)q(array+r->rsi*8);
  int blocked=cpu_recruit_decision(recruit,team,c->fallback_context,"SELECT_OFFER",1);
  return !faulted && mode==1 && blocked!=0;
 } __except(fail_exception("portal_fallback",GetExceptionCode())){return 0;}
}

void DriOnHsQuota(Registers *r) {
 if(mode!=1 || faulted)return;
 __try {
  PortalCache *c=portal_cache(0);U sp=(U)(r+1);
  if(!c || !c->hs_valid || c->valid || c->allocated)return;
  U tr=q(r->rbp-0x60);
  if(!tr || q(tr)!=c->team || q(sp+0x60)!=c->eval)return;
  int pos=(int)r->r8;if(pos<0 || pos>=POSITIONS)return;
  /* The physical native pool is already bounded by PORTAL_PLAN_LIMIT. This
     changes candidate visibility only; scores, eligibility and capacity stay. */
  if((int32_t)r->rcx<PORTAL_PLAN_LIMIT)r->rcx=PORTAL_PLAN_LIMIT;
 } __except(fail_exception("HS_shortlist_quota",GetExceptionCode())){}
}
