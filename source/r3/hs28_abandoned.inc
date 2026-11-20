/* Test28R9: full-pool HS/JUCO abandonment repair. Run only at coordinator
   boundaries without live native board/candidate iterators. The read-only
   market snapshot finds zero-offer races too; a qualified, reserved pursuit
   must exist before the only save mutation (closed stage -> Top10).
   Interest identities, earned influence, offers and ownership are untouched. */
#define HS28_RECOVERY_LIMIT 12000
#define HS28_RECOVERY_MEMO_SLOTS 16384
typedef struct {
 Rec recruit;
 uint64_t offered[3];
} Hs28AbandonedEntry;
typedef struct {
 Rec teams[138];
 U team_count,count;
 Hs28AbandonedEntry *entries;
 PortalCleanupOwner *recruit_map,*published;
 Hs15RecoveryCache snapshots;
} Hs28AbandonedMarket;
typedef struct {
 unsigned stage,count;
 Rec entry[64];
 unsigned tid[64],points[64];
 uint64_t fingerprint;
} Hs28AbandonedState;
typedef struct {U table;uint32_t rid;uint64_t fingerprint,attempted[3],funded[3],retry[3];int used;} Hs28ReopenMemo;
static Hs28ReopenMemo hs28_reopen_memo[HS28_RECOVERY_MEMO_SLOTS];
static U hs28_reopen_scope_table;
static uint32_t hs28_reopen_scope_row,hs28_reopen_scope_year;
static uint64_t hs28_reopen_hash(uint64_t hash,uint64_t value) {
 /* FNV over complete integer fields, independent of pointers or padding. */
 for(unsigned i=0;i<8;i++){hash^=(value>>(i*8))&255;hash*=UINT64_C(1099511628211);}
 return hash;
}
static Hs28ReopenMemo *hs28_reopen_slot(Rec recruit,int create) {
 U table=q((U)recruit+0x10);uint32_t rid=row(recruit);
 unsigned at=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS28_RECOVERY_MEMO_SLOTS-1);
 for(unsigned i=0;i<HS28_RECOVERY_MEMO_SLOTS;i++,at=(at+1)&(HS28_RECOVERY_MEMO_SLOTS-1)) {
  Hs28ReopenMemo *m=&hs28_reopen_memo[at];
  if(m->used && m->table==table && m->rid==rid)return m;
  if(!m->used){if(!create)return 0;m->used=1;m->table=table;m->rid=rid;return m;}
 }
 return 0;
}
/* A successful rescued pursuit keeps continuity after the weekly reservation
   catalogue expires. Callers restrict this to an existing, offered unsigned
   target after native eligibility and acquisition checks. This numeric marker
   grants neither an offer nor influence and never bypasses native lockout. */
static int hs28_abandoned_continuity(Rec recruit,Rec team,U eval) {
 Rec franchise=context_ref(eval,6),season=franchise?reference(franchise,44):0;
 int matching=franchise && season && hs28_reopen_scope_table==q((U)franchise+0x10) &&
  hs28_reopen_scope_row==row(franchise) && hs28_reopen_scope_year==scalar(season,2,0x100);
 release(season);release(franchise);if(!matching)return 0;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 Hs28ReopenMemo *memo=hs28_reopen_slot(recruit,0);
 return memo && (memo->attempted[tid>>6]&(UINT64_C(1)<<(tid&63)))!=0;
}
/* No record is acquired here: the expired reservation contains numeric
   identities only. A new scan must revalidate the destination from scratch. */
static void hs31_release_unexecuted(U table,uint32_t rid,unsigned tid) {
 unsigned at=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS28_RECOVERY_MEMO_SLOTS-1);
 for(unsigned i=0;i<HS28_RECOVERY_MEMO_SLOTS;i++,at=(at+1)&(HS28_RECOVERY_MEMO_SLOTS-1)) {
  Hs28ReopenMemo *m=&hs28_reopen_memo[at];if(!m->used)return;
  if(m->table==table && m->rid==rid){
   uint64_t bit=UINT64_C(1)<<(tid&63);
   if(!(m->funded[tid>>6]&bit))m->attempted[tid>>6]&=~bit;
   return;
  }
 }
}
/* A failed native admission must not become a permanent attempted-school
   veto after its reservation is dropped. Retain its numeric retry marker
   until the NEXT weekly scope: no same-week board churn or fabricated offer.
   Successful native funding keeps continuity through later hour resets. */
static void hs31_retry_deferred(U table,uint32_t rid,unsigned tid,int funded) {
 if(tid>=138)return;
 unsigned at=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS28_RECOVERY_MEMO_SLOTS-1);
 for(unsigned i=0;i<HS28_RECOVERY_MEMO_SLOTS;i++,at=(at+1)&(HS28_RECOVERY_MEMO_SLOTS-1)) {
  Hs28ReopenMemo *m=&hs28_reopen_memo[at];if(!m->used)return;
  if(m->table!=table || m->rid!=rid)continue;
  uint64_t bit=UINT64_C(1)<<(tid&63);unsigned word=tid>>6;
  if(funded){m->funded[word]|=bit;m->retry[word]&=~bit;return;}
  if(!(m->funded[word]&bit) && (m->attempted[word]&bit) && !(m->retry[word]&bit)) {
   m->retry[word]|=bit;
   log_line("HS31_RESERVATION_RETRY,%u,%u,NATIVE_UNFUNDED_RECHECK_NEXT_WEEK",tid,rid);
  }
  return;
 }
}
static void hs31_retry_deferred_apply(void) {
 for(unsigned i=0;i<HS28_RECOVERY_MEMO_SLOTS;i++) {
  Hs28ReopenMemo *m=&hs28_reopen_memo[i];if(!m->used)continue;
  for(unsigned word=0;word<3;word++) {
   m->attempted[word]&=~(m->retry[word]&~m->funded[word]);m->retry[word]=0;
  }
 }
}
static void hs28_abandoned_state_free(Hs28AbandonedState *state) {
 for(unsigned i=0;i<state->count;i++)release(state->entry[i]);
 memset(state,0,sizeof(*state));
}
static void hs28_abandoned_market_free(Hs28AbandonedMarket *market) {
 if(market->entries) {
  for(U i=0;i<market->count;i++)release(market->entries[i].recruit);
  HeapFree(GetProcessHeap(),0,market->entries);
 }
 for(U i=0;i<market->team_count;i++)release(market->teams[i]);
 if(market->recruit_map)HeapFree(GetProcessHeap(),0,market->recruit_map);
 if(market->published)HeapFree(GetProcessHeap(),0,market->published);
 hs15_recovery_cache_free(&market->snapshots);
 memset(market,0,sizeof(*market));
}
static int hs28_abandoned_market_read(Hs28AbandonedMarket *market,Rec franchise) {
 Rec recruits=array_field(franchise,39),teams=array_field(franchise,45);
 if(!recruits || !teams){release(recruits);release(teams);return 0;}
 U count=array_count(recruits),team_count=array_count(teams);int ok=1;
 if(count>HS28_RECOVERY_LIMIT || !team_count || team_count>138){ok=0;goto done;}
 market->entries=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*market->entries));
 market->recruit_map=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*market->recruit_map));
 market->published=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*market->published));
 if(!market->entries || !market->recruit_map || !market->published){ok=0;goto done;}
 for(U i=0;i<count;i++) {
  Rec recruit=array_at(recruits,i);if(!recruit){ok=0;goto done;}
  unsigned stage=scalar(recruit,9,0x108);
  if(!hs15_is_recruit(recruit) || stage>=5){release(recruit);continue;}
  PortalCleanupOwner *entry=portal_cleanup_slot(market->recruit_map,recruit,1);
  if(!entry || entry->owner>=0){release(recruit);ok=0;goto done;}
  entry->owner=(int)market->count;
  market->entries[market->count++].recruit=recruit;
  Rec player=reference(recruit,5);
  if(!player){ok=0;goto done;}
  PortalCleanupOwner *acquisition=portal_cleanup_slot(market->published,player,1);release(player);
  if(!acquisition){ok=0;goto done;}
 }
 for(U i=0;i<team_count;i++) {
  Rec team=array_at(teams,i);market->teams[market->team_count++]=team;
  if(!team || scalar(team,383,0x100)!=i){ok=0;goto done;}
  Rec board=board_array(team);
  if(!board){ok=0;goto done;}
  U board_count=array_count(board);
  if(board_count>MAX_BOARD){release(board);ok=0;goto done;}
  /* Current roster membership also protects an acquired player during any
     publication interval. Index only this scan's recruit players, keeping the
     map bounded by recruit count rather than the full college population. */
  for(unsigned source=0;source<2 && ok;source++) {
   Rec acquired=array_field(team,source?239:33);
   if(!acquired){ok=0;break;}
   U acquired_count=array_count(acquired);
   if(acquired_count>192){release(acquired);ok=0;break;}
   for(U j=0;j<acquired_count;j++) {
    Rec player=array_at(acquired,j);
    if(!player){ok=0;break;}
    PortalCleanupOwner *owner=portal_cleanup_slot(market->published,player,0);release(player);
    if(owner) {
     if(owner->owner>=0 && owner->owner!=(int)i){ok=0;break;}
     owner->owner=(int)i;
    }
   }
   release(acquired);
  }
  for(U j=0;j<board_count && ok;j++) {
   Rec target=array_at(board,j),recruit=target?reference(target,10):0;
   if(!target || !recruit){release(recruit);release(target);ok=0;break;}
   unsigned status=scalar(target,12,0x108);
   PortalCleanupOwner *index=portal_cleanup_slot(market->recruit_map,recruit,0);
   if(status>4)ok=0;
   if(ok && index && status>=2) {
    Hs28AbandonedEntry *entry=&market->entries[index->owner];
    uint64_t bit=UINT64_C(1)<<(i&63);
    if(entry->offered[i>>6]&bit)ok=0;
    entry->offered[i>>6]|=bit;
   }
   release(recruit);release(target);
  }
  release(board);if(!ok)goto done;
 }
done:
 release(recruits);release(teams);return ok;
}
/* 1 genuinely abandoned, 0 protected/not a closed race, -1 unreadable.
   fresh=1 rechecks every board just before applying a selected recovery. */
static int hs28_abandoned_probe(Hs28AbandonedMarket *market,Hs28AbandonedEntry *entry,
                                U eval,Hs28AbandonedState *state,int fresh) {
 Rec recruit=entry->recruit;
 state->stage=scalar(recruit,9,0x108);
 if(state->stage>=5 || !hs15_is_recruit(recruit))return 0;
 /* A reload can retain reopened Top10 state but lose its numeric reservation.
    Reconstruct only a progressed, unoffered/abandoned open race, with
    bounded destination attempts. Ordinary early low-influence Top10 recruiting is left to
    the normal planner. No stage or interest write is needed for this resume. */
 /* Do not stack destinations before the reserved native pass has run.
    Failed native admissions release their reservation; the next safe scan or
    next weekly scope may then try a different school without a save write. */
 if(!state->stage && hs28_recovery_pending(recruit,eval))return 0;
 Rec player=reference(recruit,5);if(!player)return -1;
 PortalCleanupOwner *published=portal_cleanup_slot(market->published,player,0);
 unsigned player_team=scalar(player,261,0x100);release(player);
 /* A pre-transition permission ledger alone is not acquired ownership. Real
    hard/signed stages were excluded above; roster/publication or an assigned
    college team conservatively protects the remaining ambiguous cases. */
 int owner=-1;
 if((published && published->owner>=0) || player_team<138)return 0;
 if(fresh) {
  int acquired=portal_published_owner(recruit,eval,&owner);
  if(acquired)return acquired<0?-1:0;
 }
 /* TU1029R3: a fresh open recruit may have a null TopSchoolsList (the
    supplied PRE contains this shape). Like an allocated empty open list, it
    has no progressed race to recover. A narrowed race still requires a list;
    a non-null record that fails the array interface is always unreadable. */
 Rec top;U count;
 if(!hs_read_school_list(recruit,state->stage,&top,&count))return -1;
 
 if(!count || count>64){release(top);return !count && !state->stage?0:-1;}
 unsigned leader=0;
 /* Opening a shortlist is our own write, not a changed race. Normalize it
    so closed->Top10 cannot erase already attempted destination identities. */
 uint64_t hash=UINT64_C(1469598103934665603);
 for(U i=0;i<count;i++) {
  Rec school=array_at(top,i);if(!school){release(top);return -1;}
  state->entry[state->count++]=school;
  unsigned tid=scalar(school,0,0x100),points=scalar(school,1,0x100);
  if(tid>=138 || points>65535){release(top);return -1;}
  for(U j=0;j<i;j++)if(state->tid[j]==tid || portal_same_record(state->entry[j],school)){release(top);return -1;}
  state->tid[i]=tid;state->points[i]=points;if(points>leader)leader=points;
  hash=hs28_reopen_hash(hs28_reopen_hash(hash,tid),points);
 }
 release(top);
 if(!state->stage) {
  unsigned threshold=scalar(recruit,3,0x100);
  if(!threshold || threshold>1023 || (uint64_t)leader*100<(uint64_t)threshold*35)return 0;
 }
 int pos=position(recruit);if(pos<0 || pos>=POSITIONS)return -1;
 for(U tid=0;tid<market->team_count;tid++) {
  if(!fresh && !(entry->offered[tid>>6]&(UINT64_C(1)<<(tid&63))))continue;
  Rec team=market->teams[tid];unsigned points=0;
  int offered=portal_offer_total(recruit,team,&points);
  if(offered<0)return -1;
  if(!offered)continue;
  hash=hs28_reopen_hash(hs28_reopen_hash(hash,tid+256),points);
  int eligible=native_recruit_eligible(eval,team,recruit);
  if(!eligible)continue;
  Rec user=reference(team,407);if(user){release(user);return 0;}
  Hs15Snapshot *shot=hs15_recovery_snapshot(&market->snapshots,team,eval);
  if(!shot)return -1;
  int blocked=hs15_snapshot_block(shot,recruit);if(blocked<0)return -1;
  /* A valid live finalist is enough to protect the race. No effort threshold
     is imposed here: weekly hour reset must not manufacture abandonment. */
  if(!blocked && !hs28_recruit_opening_block(shot,recruit,pos) && !locked_out(recruit,team,eval)){
   int state=hs29_offer_state(recruit,team,eval);
   if(state<0)return -1;
   if(state==2)return 0;
  }
 }
 state->fingerprint=hash;return 1;
}
static void hs28_recover_market(U eval,const char *site) {
 if(mode!=1 || faulted || !eval)return;
 int window=hs15_recovery_window(eval);
 if(window<0){portal_fault("HS28_recovery_window");return;}
 if(!window)return;
 hs28_market_scope(eval,1);
 hs28_recovery_cache_invalidate();
 Rec franchise=context_ref(eval,6),season=franchise?reference(franchise,44):0;
 Hs28AbandonedMarket *market=0;const char *error=0;
 unsigned inspected=0,abandoned=0,reopened=0,resumed=0,constrained=0,unchanged=0;
 if(!franchise || !season || !portal_ownership_scope(franchise)){error="HS28_recovery_scope";goto done;}
 U table=q((U)franchise+0x10);uint32_t rid=row(franchise),year=scalar(season,2,0x100);
 if(hs28_reopen_scope_table!=table || hs28_reopen_scope_row!=rid || hs28_reopen_scope_year!=year) {
  memset(hs28_reopen_memo,0,sizeof(hs28_reopen_memo));
  hs28_reopen_scope_table=table;hs28_reopen_scope_row=rid;hs28_reopen_scope_year=year;
 }
 market=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*market));
 if(!market || !hs28_abandoned_market_read(market,franchise)){error="HS28_recovery_market";goto done;}
 for(U i=0;i<market->count && !faulted;i++) {
  Hs28AbandonedEntry *entry=&market->entries[i];Rec recruit=entry->recruit;
  Hs28AbandonedState state={0};int status=hs28_abandoned_probe(market,entry,eval,&state,0);inspected++;
  if(status<0){log_line("HS28_RECOVERY_READ_ERROR,%s,%u,%u,INITIAL_RACE",site,row(recruit),state.stage);hs28_abandoned_state_free(&state);error="HS28_recovery_race";break;}
  if(!status){hs28_abandoned_state_free(&state);continue;}abandoned++;
  Hs28ReopenMemo *memo=hs28_reopen_slot(recruit,0);
  /* Verified funding protects an existing offer through hour resets. Once
     the complete live board scan proves that offer is gone, it is a new
     recovery opportunity, not an eternal attempted-destination veto. */
  int withdrawn=0;
  if(memo)for(unsigned w=0;w<3;w++) {
   uint64_t gone=memo->funded[w]&~entry->offered[w];
   if(gone){memo->funded[w]&=~gone;memo->attempted[w]&=~gone;memo->retry[w]&=~gone;withdrawn=1;}
  }
  if(withdrawn)log_line("HS31_RECOVERY_REASSESS,%u,OFFER_WITHDRAWN_REVALIDATE",row(recruit));
  int best=-1,best_score=0,best_slot=0,best_role=0;
  for(U tid=0;tid<market->team_count && !faulted;tid++) {
   /* An unchanged race may try a different qualified school after a failed
      native pursuit, but never reopens repeatedly for the same destination. */
   if(memo && memo->fingerprint==state.fingerprint &&
      (memo->attempted[tid>>6]&(UINT64_C(1)<<(tid&63))))continue;
   Rec team=market->teams[tid],user=reference(team,407);
   if(user){release(user);continue;}
   /* Most schools fail the native dealbreaker/offer gate. Check that cheap
      read before constructing their complete projected roster snapshot. */
   if(!native_recruit_eligible(eval,team,recruit))continue;
   Hs15Snapshot *shot=hs15_recovery_snapshot(&market->snapshots,team,eval);
   if(!shot){error="HS28_recovery_projection";break;}
   int score=0,slot=0,role=0;
   test36_hs_probe=recruit;
   int qualifies=hs28_recovery_destination(recruit,team,eval,shot,&score,&slot,&role);
   test36_hs_probe=0;
   if(qualifies<0){error="HS28_recovery_destination";break;}
   if(qualifies && role==1 && (best<0 || score>best_score)) {
    best=(int)tid;best_score=score;best_slot=slot;best_role=role;
   }
  }
  if(error || faulted){hs28_abandoned_state_free(&state);break;}
  if(best<0) {
   constrained++;
   if(memo && memo->fingerprint==state.fingerprint)unchanged++;
   log_line("HS28_ABANDONED,%s,%u,%s",site,row(recruit),
    memo && memo->fingerprint==state.fingerprint?"UNCHANGED_NO_NEW_DESTINATION":"NO_FEASIBLE_DESTINATION");
   hs28_abandoned_state_free(&state);continue;
  }
  /* Reacquire current ownership/offers and verify all saved interest fields
     before reserving the destination or touching the recruit's stage. */
  Hs28AbandonedState fresh={0};status=hs28_abandoned_probe(market,entry,eval,&fresh,1);
  if(status<0){log_line("HS28_RECOVERY_READ_ERROR,%s,%u,%u,FINAL_RECHECK",site,row(recruit),fresh.stage);hs28_abandoned_state_free(&fresh);hs28_abandoned_state_free(&state);error="HS28_recovery_recheck";break;}
  int current=status && fresh.fingerprint==state.fingerprint;
  hs28_abandoned_state_free(&fresh);
  Rec team=market->teams[best];int score=0,slot=0,role=0;
  if(current) {
   Hs15Snapshot *shot=hs15_recovery_snapshot(&market->snapshots,team,eval);
   test36_hs_probe=recruit;
   int qualifies=shot?hs28_recovery_destination(recruit,team,eval,shot,&score,&slot,&role):-1;
   test36_hs_probe=0;
   if(qualifies<0){hs28_abandoned_state_free(&state);error="HS28_recovery_recheck_destination";break;}
   current=qualifies && score==best_score && slot==best_slot && role==best_role;
  }
  if(!current){hs28_abandoned_state_free(&state);continue;}
  if(!test36_hs_grant(recruit,team,eval,best_score,entry->offered)){constrained++;hs28_abandoned_state_free(&state);continue;}
  int reserved=hs28_recovery_reserve(team,recruit,eval,best_slot,best_role);
  if(reserved<0){hs28_abandoned_state_free(&state);error="HS28_recovery_reservation";break;}
  if(!reserved){hs33_release_offer(recruit,team,eval);constrained++;hs28_abandoned_state_free(&state);continue;}
  memo=hs28_reopen_slot(recruit,1);
  if(!memo){hs28_abandoned_state_free(&state);error="HS28_recovery_memo";break;}
  if(scalar(recruit,9,0x108)!=state.stage){hs28_abandoned_state_free(&state);continue;}
  for(unsigned j=0;j<state.count;j++)if(scalar(state.entry[j],0,0x100)!=state.tid[j] ||
      scalar(state.entry[j],1,0x100)!=state.points[j]){error="HS28_recovery_interest_changed";break;}
  if(error){hs28_abandoned_state_free(&state);break;}
  if(state.stage)portal_recovery_scalar(recruit,9,0);
  if(scalar(recruit,9,0x108)!=0){hs28_abandoned_state_free(&state);error="HS28_recovery_stage_write";break;}
  if(memo->fingerprint!=state.fingerprint) {
   memset(memo->attempted,0,sizeof(memo->attempted));
   memset(memo->retry,0,sizeof(memo->retry));
  }
  memo->fingerprint=state.fingerprint;
  memo->attempted[(unsigned)best>>6]|=UINT64_C(1)<<((unsigned)best&63);
  if(state.stage)reopened++;else resumed++;
  log_line("HS28_REOPEN,%s,%u,%u,%d,%d,%d,%d,%s",
           site,row(recruit),state.stage,best,best_score,best_slot,best_role,
           state.stage?"INFLUENCE_PRESERVED_NATIVE_PURSUIT_PENDING":"OPEN_RACE_RESERVATION_REBUILT_NO_SAVE_WRITE");
  hs28_abandoned_state_free(&state);
 }
 if(!faulted && !error)log_line("HS28_RECOVERY_SCAN,%s,%u,%u,%u,%u,%u,%u",site,inspected,abandoned,reopened,resumed,constrained,unchanged);
done:
 if(market){hs28_abandoned_market_free(market);HeapFree(GetProcessHeap(),0,market);}
 release(season);release(franchise);if(error)portal_fault(error);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestHs28RecoverMarket(U eval){hs28_recover_market(eval,"FIXTURE");}
__declspec(dllexport) int DriTestHs28WasRecovered(Rec r){return hs28_reopen_slot(r,0)!=0;}
__declspec(dllexport) int DriTestHs31RetryPending(Rec r,Rec t) {
 Hs28ReopenMemo *m=hs28_reopen_slot(r,0);unsigned tid=scalar(t,383,0x100);
 return m && tid<138 && (m->retry[tid>>6]&(UINT64_C(1)<<(tid&63)))!=0;
}
__declspec(dllexport) void DriTestHs28RecoveryReset(void) {
 memset(hs28_reopen_memo,0,sizeof(hs28_reopen_memo));
 hs28_reopen_scope_table=0;hs28_reopen_scope_row=hs28_reopen_scope_year=0;
}
#endif
