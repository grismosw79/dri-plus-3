/* Test11: remove only confirmed rival portal commitments through the native
   RemoveFromBoard service. Include after portal ownership helper definitions.
   Never run a sweep while a caller is iterating a target board. The post-weekly
   portal-loop boundary and pre-board preparation boundary are safe callers. */
#define PORTAL_CLEANUP_SLOTS 16384
typedef struct {U table;uint32_t rid;int owner,stage,used;} PortalCleanupOwner;
typedef struct {Rec team,target,recruit;unsigned tid,held,stage;int owner;} PortalCleanupRemoval;
typedef struct {int active;U recruit_table,team_table;uint32_t rid,team_row;unsigned owner;} PortalCleanupAuthorization;
static DWORD cleanup_tls=TLS_OUT_OF_INDEXES;

typedef struct {
 const char *phase;
 unsigned year,type,week;
 int team,recruit,index;
} HsCleanupTrace;
static __declspec(thread) HsCleanupTrace *hs_cleanup_trace;

static void hs_cleanup_step(const char *phase,int team,int recruit,int index) {
 HsCleanupTrace *trace=hs_cleanup_trace;
 if(trace){
  trace->phase=phase;trace->team=team;trace->recruit=recruit;trace->index=index;
 }
}
static int hs_cleanup_exception(const HsCleanupTrace *trace,DWORD code) {
 log_line("HS_CLEANUP_CONTEXT,%s,year=%u,type=%u,week=%u,team=%d,recruit=%d,index=%d",
          trace->phase,trace->year,trace->type,trace->week,trace->team,trace->recruit,trace->index);
 return fail_exception("HS_rival_sweep",code);
}

static int portal_cleanup_same(Rec a,Rec b) {
 return a && b && q((U)a+0x10)==q((U)b+0x10) && row(a)==row(b);
}
static PortalCleanupOwner *portal_cleanup_slot(PortalCleanupOwner *map,Rec rec,int create) {
 U table=q((U)rec+0x10);uint32_t rid=row(rec);
 unsigned at=(unsigned)((table>>4)^((U)rid*2654435761u))&(PORTAL_CLEANUP_SLOTS-1);
 for(unsigned i=0;i<PORTAL_CLEANUP_SLOTS;i++,at=(at+1)&(PORTAL_CLEANUP_SLOTS-1)) {
  PortalCleanupOwner *s=&map[at];
  if(s->used && s->table==table && s->rid==rid)return s;
  if(!s->used) {
   if(!create)return 0;
   s->used=1;s->table=table;s->rid=rid;s->owner=-1;return s;
  }
 }
 return 0;
}
static PortalCleanupAuthorization *portal_cleanup_authorization(int create) {
 if(cleanup_tls==TLS_OUT_OF_INDEXES)return 0;
 PortalCleanupAuthorization *s=TlsGetValue(cleanup_tls);
 if(!s && create) {
  s=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*s));
  if(s && !TlsSetValue(cleanup_tls,s)){HeapFree(GetProcessHeap(),0,s);return 0;}
 }
 return s;
}
/* Native RemoveFromBoard rejects the stored leader of HardCommitted/Signed.
   A redirected Signed recruit may still have the losing school at rank one.
   Override that check ONLY for the exact removal already authorized by the
   ownership snapshot below. All ordinary native/UI removal calls are unchanged.
   Hook: 081A409E, immediately after the native protected-owner predicate. */
void DriOnPortalCleanupProtection(Registers *r) {
 if(mode!=1 || faulted || !(r->rax&255))return;
 __try {
  PortalCleanupAuthorization *s=portal_cleanup_authorization(0);
  if(!s || !s->active || !r->r12 || !r->r14)return;
  Rec recruit=(Rec)q(r->r12),team=(Rec)q(r->r14);
  if(!recruit || !team || q((U)recruit+0x10)!=s->recruit_table || row(recruit)!=s->rid ||
     q((U)team+0x10)!=s->team_table || row(team)!=s->team_row ||
     scalar(team,383,0x100)==s->owner || (!transfer_recruit(recruit) && !hs15_is_recruit(recruit)) || scalar(recruit,9,0x108)<5)return;
  r->rax&=~(U)255;
 } __except(fail_exception("cleanup_protection",GetExceptionCode())){}
}

/* filter==NULL sweeps the entire current portal. An optional filter is useful
   after a user instant commitment; expected_owner must already be confirmed.
   Returns 1 when complete/no-op; 0 on failed validation. Unknown/unowned records
   are deliberately not removed, and never become ownership by implication. */
static int portal_cleanup_run_kind(U eval,Rec filter,int expected_owner,int hs,Rec only_team) {
 if(mode!=1 || faulted)return 1;
 if(!eval || (filter && (!transfer_recruit(filter) || scalar(filter,9,0x108)<5)))return 1;
 hs_cleanup_step("current_context",-1,-1,-1);
 if(hs && only_team) {Rec user=reference(only_team,407);if(user){release(user);return 1;}}
 U manager=q(eval+0xc8);
 if(!manager){portal_fault("cleanup_manager");return 0;}
 Rec franchise=0,team_array=0,portal_array=0,teams[138]={0};
 PortalCleanupOwner *published=0,*owners=0;
 PortalCleanupRemoval *pending=0;PortalCleanupAuthorization *auth=0;
 unsigned counts[138]={0},removed[138]={0};U team_count=0;unsigned pending_count=0;
 int ok=0;const char *failure=0;
 franchise=context_ref(eval,6);
 hs_cleanup_step("ownership_scope",-1,-1,-1);
 if(!franchise || !portal_ownership_scope(franchise)){failure="scope";goto done;}
 hs_cleanup_step("team_and_recruit_lists",-1,-1,-1);
 team_array=array_field(franchise,45);portal_array=array_field(franchise,hs?39:46);
 if(!team_array || !portal_array){failure="class_or_teams";goto done;}
 team_count=array_count(team_array);U portal_count=array_count(portal_array);
 if(!team_count || team_count>138 || portal_count>12000){failure="array_bounds";goto done;}
 published=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*published));
 owners=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*owners));
 pending=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,team_count*MAX_BOARD*sizeof(*pending));
 auth=portal_cleanup_authorization(1);
 if(!published || !owners || !pending || !auth){failure="memory";goto done;}
 if(auth->active){failure="reentrant";goto done;}
 /* Resolve published owners once. This also detects duplicate published
    ownership before touching any board. Team index equals franchise-array index
    in the supported build; do not guess when that invariant changes. */
 for(U i=0;i<team_count;i++) {
  hs_cleanup_step("published_team",(int)i,-1,-1);
  teams[i]=array_at(team_array,i);
  if(!teams[i] || scalar(teams[i],383,0x100)!=i){failure="team_identity";goto done;}
  Rec committed=array_field(teams[i],33);
  if(!committed){failure="committed_array";goto done;}
  U count=array_count(committed);
  if(count>192){release(committed);failure="committed_bounds";goto done;}
  for(U j=0;j<count;j++) {
   hs_cleanup_step("published_player",(int)i,-1,(int)j);
   Rec player=array_at(committed,j);
   if(!player){release(committed);failure="committed_player";goto done;}
   PortalCleanupOwner *owner=portal_cleanup_slot(published,player,1);release(player);
   if(!owner || (owner->owner>=0 && owner->owner!=(int)i)) {
    release(committed);failure="published_owner_conflict";goto done;
   }
   owner->owner=(int)i;
  }
  release(committed);
 }
 /* Stage-5 validity includes a real current offer via portal_hard_owner.
    Signed ownership uses published acquisition first and the same-advance
    signing ledger second. Stored school rank is never Signed ownership. */
 for(U i=0;i<portal_count;i++) {
  hs_cleanup_step("recruit_list",-1,-1,(int)i);
  Rec recruit=array_at(portal_array,i);
  if(!recruit){failure="recruit";goto done;}
  if(hs_cleanup_trace)hs_cleanup_step("recruit_owner",-1,(int)row(recruit),(int)i);
  unsigned stage=scalar(recruit,9,0x108);
  if((hs?!hs15_is_recruit(recruit):(!transfer_recruit(recruit) || stage<5)) || (filter && !portal_cleanup_same(filter,recruit))) {
   release(recruit);continue;
  }
  Rec player=reference(recruit,5);
  if(!player){release(recruit);failure="player";goto done;}
  PortalCleanupOwner *saved=portal_cleanup_slot(published,player,0);
  int owner=-1,ledger=-1,has_ledger=portal_signed_destination(recruit,player,&ledger);
  if(hs) {
   /* Published HS acquisitions can retain a soft-looking stage. Resolve the
      actual Player owner before considering any recruit stage or old leader. */
   if(stage>6){release(player);release(recruit);failure="hs_stage";goto done;}
   if(saved)owner=saved->owner;
   else if(stage>=5 && has_ledger)owner=ledger;
   if(saved && stage>=5 && has_ledger && saved->owner!=ledger) {
    release(player);release(recruit);failure="hs_ledger_conflict";goto done;
   }
   if(owner<0 && stage==5) {
    if(hs_cleanup_trace)hs_cleanup_step("hard_commit_leader",-1,(int)row(recruit),(int)i);
    Rec top=array_field(recruit,13),leading=0;
    if(!top){release(player);release(recruit);failure="hs_hard_interest";goto done;}
    U count=array_count(top);
    if(count>64){release(top);release(player);release(recruit);failure="hs_hard_bounds";goto done;}
    if(count)leading=array_at(top,0);
    release(top);
    if(count && !leading){release(player);release(recruit);failure="hs_hard_leader";goto done;}
    unsigned tid=leading?scalar(leading,0,0x100):138;release(leading);
    if(count && tid>=team_count){release(player);release(recruit);failure="hs_hard_team";goto done;}
    if(tid<team_count) {
     if(hs_cleanup_trace)hs_cleanup_step("hard_commit_offer",(int)tid,(int)row(recruit),(int)i);
     int offered=portal_active_offer(recruit,teams[tid]);
     if(offered<0){release(player);release(recruit);failure="hs_hard_offer";goto done;}
     if(hs_cleanup_trace)hs_cleanup_step("hard_commit_owner",(int)tid,(int)row(recruit),(int)i);
     if(offered && own_commit(recruit,teams[tid]))owner=(int)tid;
    }
   }
  } else if(stage==5) {
   int valid=portal_hard_owner(recruit,eval,&owner);
   if(valid<0){release(player);release(recruit);failure="hard_owner";goto done;}
   if(!valid)owner=-1;
   if(owner>=0 && saved && saved->owner!=owner){release(player);release(recruit);failure="hard_published_conflict";goto done;}
  } else if(stage==6) {
   if(saved)owner=saved->owner;
   else if(has_ledger)owner=ledger;
   if(saved && has_ledger && saved->owner!=ledger){release(player);release(recruit);failure="signed_ledger_conflict";goto done;}
  }
  release(player);
  if(owner>=0 && ((U)owner>=team_count || (!hs && !portal_academy_allowed(recruit,(unsigned)owner))))owner=-1;
  if(owner>=0 && filter && expected_owner!=owner){release(recruit);failure="filter_owner_conflict";goto done;}
  if(owner>=0) {
   PortalCleanupOwner *entry=portal_cleanup_slot(owners,recruit,1);
   if(!entry){release(recruit);failure="owner_capacity";goto done;}
   entry->owner=owner;entry->stage=(int)stage;
  }
  release(recruit);
 }
 /* Complete every board read before native mutations. Snapshot strong refs,
    never retain an array iterator across RemoveFromBoard compaction. */
 for(U i=0;i<team_count;i++) {
  hs_cleanup_step("school_board",(int)i,-1,-1);
  if(hs) {
   if(only_team && !portal_cleanup_same(only_team,teams[i]))continue;
   Rec user=reference(teams[i],407);if(user){release(user);continue;}
  }
  Rec board=board_array(teams[i]);
  if(!board){failure="board";goto done;}
  U count=array_count(board);counts[i]=(unsigned)count;
  if(count>MAX_BOARD){release(board);failure="board_bounds";goto done;}
  for(U j=0;j<count;j++) {
   hs_cleanup_step("board_target",(int)i,-1,(int)j);
   Rec target=array_at(board,j),recruit=target?reference(target,10):0;
   if(!target || !recruit){release(recruit);release(target);release(board);failure="target";goto done;}
   PortalCleanupOwner *entry=portal_cleanup_slot(owners,recruit,0);
   unsigned stage=scalar(recruit,9,0x108);
   int academy_open=!hs && !filter && portal_academy((unsigned)i) && transfer_recruit(recruit) && stage<5;
   if(academy_open || (entry && entry->owner>=0 && entry->owner!=(int)i)) {
    for(unsigned k=0;k<pending_count;k++)if(pending[k].tid==i && portal_cleanup_same(pending[k].recruit,recruit)) {
     release(recruit);release(target);release(board);failure="duplicate_target";goto done;
    }
    unsigned status=scalar(target,12,0x108);
    if(status>4){release(recruit);release(target);release(board);failure="scholarship_status";goto done;}
    pending[pending_count++]=(PortalCleanupRemoval){teams[i],target,recruit,(unsigned)i,status>=2 && status<=4,stage,academy_open?-1:entry->owner};
   } else {release(recruit);release(target);}
  }
  release(board);
 }
 for(unsigned i=0;i<pending_count;i++) {
  PortalCleanupRemoval *item=&pending[i];
  hs_cleanup_step("removal_check",(int)item->tid,-1,(int)i);
  Rec current=reference(item->target,10);
  PortalCleanupOwner *owner=portal_cleanup_slot(owners,item->recruit,0);
  int allowed=item->owner<0 ? portal_academy(item->tid) && item->stage<5 && transfer_recruit(item->recruit) :
             owner && owner->owner==item->owner && owner->owner!=(int)item->tid;
  int stable=portal_cleanup_same(current,item->recruit) && allowed &&
             scalar(item->recruit,9,0x108)==item->stage;
  release(current);
  Rec board=board_array(item->team);U before=board?array_count(board):0;release(board);
  if(!stable || before!=counts[item->tid]-removed[item->tid]){failure="snapshot_changed";goto done;}
  unsigned offers_before=scalar(item->recruit,14,0x100);
  *auth=(PortalCleanupAuthorization){item->owner>=0,q((U)item->recruit+0x10),q((U)item->team+0x10),row(item->recruit),row(item->team),(unsigned)item->owner};
  U candidate[6]={(U)item->recruit,0,0,0,0,0};Rec team=item->team;
  int rc;
  if(hs_cleanup_trace)hs_cleanup_step("remove_target",(int)item->tid,(int)row(item->recruit),(int)i);
  __try {rc=((int(*)(U,Rec*,void*))(game_base+0x081a7b10))(manager,&team,candidate);}
  __finally {auth->active=0;}
  hs_cleanup_step("verify_removal",(int)item->tid,-1,(int)i);
  unsigned offers_after=scalar(item->recruit,14,0x100);
  current=reference(item->target,10);int cleared=!current;release(current);
  board=board_array(item->team);U after=board?array_count(board):0;release(board);
  int success=rc==0 && cleared && after+1==before &&
              offers_after==offers_before-(item->held && offers_before>0?1:0) &&
              scalar(item->recruit,9,0x108)==item->stage;
  log_line(hs?"HS28_RIVAL_DROP,%u,%u,%d,%u,%d,%u,%u,%s":"PORTAL_RIVAL_DROP,%u,%u,%d,%u,%d,%u,%u,%s",item->tid,row(item->recruit),item->owner,item->stage,rc,offers_before,offers_after,success?"REMOVED":"FAILED");
  if(!success){failure="native_remove_check";goto done;}
  removed[item->tid]++;
 }
 log_line(hs?"HS28_RIVAL_CLEANUP,%u,%s,ACTIVE":"PORTAL_CLEANUP,%u,%s,ACTIVE",pending_count,filter?"COMMIT":only_team?"TEAM":"SWEEP");ok=1;
done:
 hs_cleanup_step("release_sweep_records",-1,-1,-1);
 if(auth)auth->active=0;
 for(unsigned i=0;i<pending_count;i++){release(pending[i].recruit);release(pending[i].target);}
 for(U i=0;i<team_count && i<138;i++)release(teams[i]);
 release(portal_array);release(team_array);release(franchise);
 if(pending)HeapFree(GetProcessHeap(),0,pending);
 if(owners)HeapFree(GetProcessHeap(),0,owners);
 if(published)HeapFree(GetProcessHeap(),0,published);
 if(failure){log_line(hs?"HS28_CLEANUP_REFUSED,%s":"PORTAL_CLEANUP_REFUSED,%s",failure);portal_fault(hs?"HS_rival_cleanup_validation":"cleanup_validation");}
 return ok;
}
static int portal_cleanup_all(U eval) {return portal_cleanup_run_kind(eval,0,-1,0,0);}
static int portal_cleanup_rivals(Rec recruit,unsigned owner,U eval) {return portal_cleanup_run_kind(eval,recruit,(int)owner,0,0);}
/* Safe-boundary callers only. HS user boards retain manual control. */
static int hs28_cleanup_rivals(Rec team,U eval) {return portal_cleanup_run_kind(eval,0,-1,1,team);}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestHsRivalCleanup(Rec team,U eval){return hs28_cleanup_rivals(team,eval);}
#endif
