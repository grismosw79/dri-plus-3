/* R11: a recruit's stage may narrow only around an offer the school would
   currently accept. Read the existing acceptance policy; never manufacture
   interest, offers, roster places or a different commitment destination. */
#define HS29_OFFER_SLOTS 16384
typedef struct {U table;uint32_t rid;int used;uint64_t schools[3];} Hs29OfferEntry;
typedef struct {U table,eval;uint32_t rid,year,week,type;int valid;Hs29OfferEntry *entries;} Hs29OfferView;
static Hs29OfferView hs29_view;
/* Actual offer actions and completed funding can change this shortlist within
   one calendar week. Mark it dirty without freeing storage under a native
   callback; the next normal lookup rebuilds it from the actual board status. */
static void hs29_offer_invalidate(void){hs29_view.valid=0;}
static void hs29_offer_reset(void){
 if(hs29_view.entries)HeapFree(GetProcessHeap(),0,hs29_view.entries);
 memset(&hs29_view,0,sizeof(hs29_view));
}
static Hs29OfferEntry *hs29_offer_slot(Rec recruit,int create){
 if(!recruit || !hs29_view.entries)return 0;
 U table=q((U)recruit+0x10);uint32_t rid=row(recruit);
 unsigned at=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS29_OFFER_SLOTS-1);
 for(unsigned i=0;i<HS29_OFFER_SLOTS;i++,at=(at+1)&(HS29_OFFER_SLOTS-1)){
  Hs29OfferEntry *e=&hs29_view.entries[at];
  if(e->used && e->table==table && e->rid==rid)return e;
  if(!e->used){if(!create)return 0;e->used=1;e->table=table;e->rid=rid;return e;}
 }return 0;
}
/* This index is only a shortlist of boards to read. Offer validity and the
   school's acceptance decision are always rechecked live. No record pointers
   are retained. Reset before planning and after the native weekly batch. */
static int hs29_offer_index(U eval,Rec franchise,Rec teams){
 Rec season=reference(franchise,44);if(!season)return 0;
 Hs29OfferView now={q((U)franchise+0x10),eval,row(franchise),scalar(season,2,0x100),scalar(season,4,0x100),scalar(season,5,0x108),0,0};
 release(season);
 if(hs29_view.valid && hs29_view.table==now.table && hs29_view.eval==now.eval && hs29_view.rid==now.rid &&
    hs29_view.year==now.year && hs29_view.week==now.week && hs29_view.type==now.type)return 1;
 hs29_offer_reset();hs29_view=now;
 hs29_view.entries=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Hs29OfferEntry)*HS29_OFFER_SLOTS);
 if(!hs29_view.entries)return 0;
 U n=array_count(teams);if(!n || n>138)return 0;
 for(U i=0;i<n;i++){
  Rec team=array_at(teams,i),board=team?board_array(team):0;
  if(!team || scalar(team,383,0x100)!=i || !board){release(board);release(team);return 0;}
  U count=array_count(board);int ok=count<=MAX_BOARD;
  for(U j=0;j<count && ok;j++){
   Rec target=array_at(board,j),recruit=target?reference(target,10):0;
   if(!target || !recruit){release(recruit);release(target);ok=0;break;}
   unsigned status=scalar(target,12,0x108);
   if(status>4)ok=0;
   if(status>=2 && status<=4){
    Hs29OfferEntry *e=hs29_offer_slot(recruit,1);
    if(!e)ok=0;else e->schools[i>>6]|=UINT64_C(1)<<(i&63);
   }
   release(recruit);release(target);
  }
  release(board);release(team);if(!ok)return 0;
 }
 hs29_view.valid=1;return 1;
}
/* 0 unavailable, 1 conditional, 2 committable, -1 decoding failure.
   A primary designation alone is insufficient: the actual shared acceptance
   policy, native eligibility, live offer and current lockout must all pass. */
static int hs29_offer_state(Rec recruit,Rec team,U eval){
 int offered=portal_active_offer(recruit,team);if(offered<0)return -1;if(!offered)return 0;
 if(!native_recruit_eligible(eval,team,recruit) || locked_out(recruit,team,eval))return faulted?-1:0;
 int reason=cpu_recruit_decision(recruit,team,eval,"HS29_COMMITTABLE",1);
 if(faulted || mode!=1)return -1;
 return reason==17?1:reason?0:2;
}
/* Mirror the verified native lockout switch without temporarily changing the
   saved stage. Use live FTC cut sizes, including a retuned Top-8 cut. The two
   native helpers preserve rank/tie and Battle-threshold semantics exactly. */
static int hs29_next_allows(Rec recruit,Rec team,U eval,unsigned next){
 if(!next)return 1;
 if(next>4)return -1;
 if(hs15_is_recruit(recruit) && scalar(recruit,9,0x108)<5)return 1;
 Rec tuning=(Rec)q(eval+0xd0);if(!tuning)return -1;
 static const int fields[5]={0,0x3c,0x3b,0x36,0x39};
 unsigned count=scalar(tuning,fields[next],0x100);if(!count || count>64)return -1;
 Wrap w=wrap(recruit);
 int rank=((int(*)(Wrap*,Rec*,U))(game_base+0x0862cf60))(&w,&team,count);
 if(rank<0)return 0;
 if(next==3)return ((unsigned char(*)(Wrap*,Rec*,U))(game_base+0x0862dc10))(&w,&team,eval+0xd0)!=0;
 return 1;
}
typedef struct {int firm,conditional,unavailable,surviving,leader_firm;unsigned leader_tid,firm_tid,firm_points,firm_rank,active_points;} Hs29OfferResult;
static int hs29_offers(Rec recruit,U eval,unsigned next,Hs29OfferResult *result){
 memset(result,0,sizeof(*result));result->leader_tid=result->firm_tid=255;result->firm_rank=64;
 Rec franchise=context_ref(eval,6),teams=franchise?array_field(franchise,45):0;
 if(!franchise || !teams || !hs29_offer_index(eval,franchise,teams)){release(teams);release(franchise);return 0;}
 Hs29OfferEntry *entry=hs29_offer_slot(recruit,0);int ok=1;
 Rec top;U count;unsigned ids[64],earned[64],leader=255;
 unsigned stage=scalar(recruit,9,0x108);
 if(!hs_read_school_list(recruit,stage,&top,&count) || count>64 || (!count && stage))ok=0;
 for(U i=0;i<count && ok;i++){
  Rec school=array_at(top,i);if(!school){ok=0;break;}
  unsigned tid=scalar(school,0,0x100),points=scalar(school,1,0x100);release(school);
  if(tid>=138 || points>65535)ok=0;
  for(U j=0;j<i;j++)if(ids[j]==tid)ok=0;
  ids[i]=tid;earned[i]=points;if(!i)leader=tid;
 }
 release(top);result->leader_tid=leader;
 U n=array_count(teams);
 for(U tid=0;entry && tid<n && ok;tid++)if(entry->schools[tid>>6]&(UINT64_C(1)<<(tid&63))){
  Rec team=array_at(teams,tid);if(!team){ok=0;break;}
  int state=hs29_offer_state(recruit,team,eval);
  unsigned value=0,rank=64+(unsigned)tid;int listed=0;
  if(state>0){
   for(U k=0;k<count;k++)if(ids[k]==tid){value=earned[k];rank=(unsigned)k;listed=1;break;}
   if(!listed && portal_offer_total(recruit,team,&value)!=1)ok=0;
   if(value>65535)ok=0;
   if(ok && value>result->active_points)result->active_points=value;
  }
  if(state<0)ok=0;
  else if(state==2){
   result->firm++;if(tid==leader)result->leader_firm=1;
   if(ok && (result->firm_tid==255 || value>result->firm_points || (value==result->firm_points && rank<result->firm_rank))){
    result->firm_tid=(unsigned)tid;result->firm_points=value;result->firm_rank=rank;
   }
   int survives=next?hs29_next_allows(recruit,team,eval,next):1;
   if(survives<0)ok=0;else result->surviving+=survives;
  }else if(state==1)result->conditional++;else result->unavailable++;
  release(team);
 }
 release(teams);release(franchise);return ok;
}
/* R12 coordinator helpers. The effective leader is a LIVE firm or
   conditional offer; a withdrawn/ineligible school retains its earned points
   in the save but cannot make a newly accessible race look unwinnable. These
   helpers run outside hs28_choice, so acceptance never recursively plans. */
static int hs30_firm_count(Rec recruit,U eval){
 Hs29OfferResult result;
 return hs29_offers(recruit,eval,0,&result)?result.firm:-1;
}
static int hs30_offer_leader(Rec recruit,U eval){
 /* Same current actual offers and earned points as weekly destination choice.
    A funded outsider remains real competition for a new recovery pursuit. */
 Hs29OfferResult result;
 return hs29_offers(recruit,eval,0,&result)?(int)result.active_points:-1;
}
static int hs30_catchup_to(const Hs28Choice *choice,int leader){
 return choice->stage==0 && leader>=0 && (!leader ||
  (choice->commit_score>0 && (int64_t)leader*100<(int64_t)choice->commit_score*35) ||
  (int64_t)choice->points*4>=(int64_t)leader*3);
}
/* The captured weekly loop evaluates transition condition 08627910 before
   HookHsOwnership and calls native stage writer 0814c3c0 afterwards. The
   native writer/ownership uses the first school. Therefore a firm count alone
   cannot authorize soft commitment to a conditional first school.

   Re-evaluate the SAME transition expression for the best current firm
   alternative, using existing school/point pairs or actual funded Target
   influence for a school outside the bounded displayed list. An outside school
   enters at the front only for an accepted native soft commitment, displacing
   the final displayed pair. Always restore the temporary view before deciding. Only a successful native
   condition plus a fresh offer-state check selects the school for the actual
   soft-commit transition. Native code still writes stage and influence.
   Both boundaries require that selected school's own native expression to
   succeed; no fabricated condition, points, offers or owner is introduced. */
static int test30_hs_firm_choice(Rec recruit,U eval,Wrap *transition,unsigned tid){
 if(!transition || !transition->rec || !transition->schema || tid>=138)return 0;
 Rec top=array_field(recruit,13);U n=top?array_count(top):0;
 Rec entries[64]={0};unsigned ids[64],points[64],selected=0;int pick=-1,ok=top && n>0 && n<=64,allowed=0,wrote=0,outside=0;
 for(U i=0;i<n && ok;i++){
  entries[i]=array_at(top,i);if(!entries[i]){ok=0;break;}
  ids[i]=scalar(entries[i],0,0x100);points[i]=scalar(entries[i],1,0x100);
  if(ids[i]>=138 || points[i]>65535)ok=0;
  for(U j=0;j<i;j++)if(ids[j]==ids[i])ok=0;
  if(ids[i]==tid)pick=(int)i;
 }
 if(ok && pick<0){
  Rec team=portal_team_by_id(eval,tid);
  int offered=team?portal_offer_total(recruit,team,&selected):0;release(team);
  if(offered<0 || selected>65535)ok=0;
  else if(offered==1){outside=1;pick=(int)n-1;}
 }
 if(ok && (outside || pick>0)){
  if(!outside)selected=points[pick];
  __try { __try {
   wrote=1;
   for(int i=pick;i>0;i--){portal_recovery_scalar(entries[i],0,ids[i-1]);portal_recovery_scalar(entries[i],1,points[i-1]);}
   portal_recovery_scalar(entries[0],0,tid);portal_recovery_scalar(entries[0],1,selected);
   allowed=((unsigned char(*)(Wrap*,Rec*,U))(game_base+0x08627970))(transition,&recruit,eval+0xd0)!=0;
  } __finally {
   if(wrote)for(U i=0;i<n;i++){portal_recovery_scalar(entries[i],0,ids[i]);portal_recovery_scalar(entries[i],1,points[i]);}
  }} __except(fail_exception("HS31_native_condition",GetExceptionCode())){allowed=0;}
  Rec team=allowed?portal_team_by_id(eval,tid):0;
  if(allowed)allowed=team && hs29_offer_state(recruit,team,eval)==2 && !faulted;
  if(allowed && outside){unsigned current=0;allowed=portal_offer_total(recruit,team,&current)==1 && current==selected;}
  release(team);
  if(allowed){
   for(int i=pick;i>0;i--){portal_recovery_scalar(entries[i],0,ids[i-1]);portal_recovery_scalar(entries[i],1,points[i-1]);}
   portal_recovery_scalar(entries[0],0,tid);portal_recovery_scalar(entries[0],1,selected);
  }
  log_line("TEST31_HS_NATIVE_CHOICE,%u,%u,%u,%u,%s,%s",row(recruit),ids[0],tid,selected,
   outside?"ACTUAL_OUTSIDE_LIST_OFFER":"LISTED_OFFER",allowed?"NATIVE_CONDITION_AND_FIRM_PASS":"HOLD_NATIVE_CONDITION_OR_FIRM");
 }
 for(U i=0;i<n && i<64;i++)release(entries[i]);release(top);
 if(!ok)portal_fault("HS31_choice_identity");
 return allowed;
}
static int hs29_stage_block(Rec recruit,U eval,unsigned next,Wrap *transition){
 if(mode!=1 || faulted || !hs15_is_recruit(recruit) || !hs28_pair_window(eval))return 0;
 unsigned stage=scalar(recruit,9,0x108);
 if(stage>=5 || next<1 || next>4 || next<=stage)return 0;
 /* Preference milestones may advance without excluding any eligible school.
    Soft commitment still requires the actual interest leader to accept. */
 if(next<4){log_line("HS31_PREFERENCE,%u,%u,%u,ADVISORY_NO_LOCKOUT",row(recruit),stage,next);return 0;}
 Hs29OfferResult r;if(!hs29_offers(recruit,eval,next,&r)){portal_fault("HS29_offer_read");return 0;}
 if(r.leader_tid<138 && r.firm && r.surviving && !r.leader_firm && test30_hs_firm_choice(recruit,eval,transition,r.firm_tid)) {
  r.leader_firm=1;r.leader_tid=r.firm_tid;
 }
 /* This transition already failed its actual leader acceptance. A new
    alternative-evaluation fault must not turn that rejection into success. */
 if(faulted || mode!=1)return 1;
 int blocked=!r.firm || !r.surviving || (next==4 && !r.leader_firm);
 log_line("TEST30_HS_STAGE_LEADER,%u,%u,%u,%u,%u,%u",row(recruit),stage,next,r.leader_tid,r.firm_tid,r.firm_points);
 log_line("HS29_STAGE,%u,%u,%u,%d,%d,%d,%d,%d,%s",row(recruit),stage,next,r.firm,r.conditional,r.unavailable,r.surviving,r.leader_firm,
  !blocked?"ALLOW":!r.firm?"HOLD_NO_COMMITTABLE_OFFER":!r.surviving?"HOLD_COMMITTABLE_OFFER_OUTSIDE_CUT":"HOLD_CONDITIONAL_LEADER");
 return blocked;
}
/* Test31: the ordinary condition may fail only because its displayed leader
   has little influence while a real funded outsider has already earned enough.
   The new nonoverlapping hook replays the original condition exactly once,
   then calls here before its native false branch. An actual outside firm offer
   may qualify through the SAME native expression; no other false transition
   is changed. The existing post-condition acceptance hook still runs next. */
static int hs31_has_outside_offer(Rec recruit,U eval) {
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;
 if(!f || !teams || !hs29_offer_index(eval,f,teams)){release(teams);release(f);return -1;}
 Hs29OfferEntry *entry=hs29_offer_slot(recruit,0);
 release(teams);release(f);if(!entry)return 0;
 Rec top;U n;uint64_t listed[3]={0};unsigned stage=scalar(recruit,9,0x108);
 if(!hs_read_school_list(recruit,stage,&top,&n) || n>64 || (!n && stage)){release(top);return -1;}
 /* No displayed leader exists against which to re-evaluate a failed native
    commitment condition. An actual outside offer remains on its board. */
 if(!n){release(top);return 0;}
 for(U i=0;i<n;i++) {
  Rec school=array_at(top,i);unsigned tid=school?scalar(school,0,0x100):255;release(school);
  if(tid>=138 || (listed[tid>>6]&(UINT64_C(1)<<(tid&63)))){release(top);return -1;}
  listed[tid>>6]|=UINT64_C(1)<<(tid&63);
 }
 release(top);
 for(unsigned i=0;i<3;i++)if(entry->schools[i]&~listed[i])return 1;
 return 0;
}
void DriAfterHsCondition(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode!=1 || faulted || (r->rax&255))return;
 __try {
  U sp=(U)(r+1),eval=q(r->rbp+0x60);Rec recruit=(Rec)q(r->rbp+0x68);
  Wrap *transition=(Wrap *)(sp+0x30);
  if(!recruit || !hs15_is_recruit(recruit) || scalar(recruit,9,0x108)>=4 ||
     !transition->rec || scalar(transition->rec,1,0x108)!=4 || !hs28_pair_window(eval))return;
  /* Most native false conditions have no outsider. Use the bounded index to
     avoid rebuilding acceptance plans for their ordinary listed pursuits. */
  int outside=hs31_has_outside_offer(recruit,eval);
  if(outside<0){portal_fault("HS31_condition_offer_index");return;}
  if(!outside)return;
  Hs29OfferResult offers;
  if(!hs29_offers(recruit,eval,4,&offers)){portal_fault("HS31_condition_offers");return;}
  if(offers.firm && offers.surviving && offers.firm_tid<138 && offers.firm_rank>=64 &&
     test30_hs_firm_choice(recruit,eval,transition,offers.firm_tid) && !faulted && mode==1) {
   r->rax=(r->rax&~(U)255)|1;
   log_line("TEST31_HS_OUTSIDE_CONDITION,%u,%u,%u,%u,NATIVE_EXPRESSION_AND_FIRM_PASS",
    row(recruit),offers.leader_tid,offers.firm_tid,offers.firm_points);
  }
 } __except(fail_exception("HS31_condition_boundary",GetExceptionCode())){}
}
/* Repair already-closed races at a coordinator boundary, including a W6
   reload. A real acquired player is protected even if its stage lags. With no
   current committable offer the recruit must be open to new pursuits; a
   reservation for another conditional offer cannot establish a secure choice. */
/* A single firm offer is normally enough to keep a closed race intact.
   Reopen an elite's depleted race only after a lost offer/funded pursuit AND
   a distinct CPU destination with actual room, score and affordable actions
   have been proven. Native execution still has to create/fund that offer. */
static int hs30_competition_path(Hs28AbandonedMarket *market,Rec recruit,U eval,
 const Hs29OfferResult *offers,const char *site){
 if(offers->firm!=1 || offers->conditional)return 0;
 /* A recruit who has chosen a school that will accept him need not restart
    his search simply because the competing schools leave the race. */
 if(scalar(recruit,9,0x108)==4 && offers->leader_firm)return 0;
 Rec player=reference(recruit,5);int stars=player?(int)scalar(player,231,0x108):0;release(player);
 if(stars<3)return 0;
 int lost=offers->unavailable>0;
 Hs28MarketRecruit *m=hs28_market_scope(eval,0)?hs28_market_find(q((U)recruit+0x10),row(recruit)):0;
 if(m)for(unsigned tid=0;tid<138;tid++)
  if(hs28_market_has(m->ever_funded,tid) && !hs28_market_has(m->offered,tid))lost=1;
 if(!lost)return 0;
 hs28_recovery_cache_invalidate();
 int best=-1,best_score=0,best_slot=0,best_role=3;
 for(U tid=0;tid<market->team_count && !faulted;tid++){
  Rec team=market->teams[tid],user=reference(team,407);if(user){release(user);continue;}
  int state=hs29_offer_state(recruit,team,eval);if(state<0)return 0;if(state>0)continue;
  if(!native_recruit_eligible(eval,team,recruit))continue;
  Hs15Snapshot *shot=hs15_recovery_snapshot(&market->snapshots,team,eval);if(!shot)return 0;
  int score=0,slot=0,role=0;
  int result=hs28_recovery_destination(recruit,team,eval,shot,&score,&slot,&role);
  if(result<0)return 0;
  if(result>0 && (best<0 || role<best_role || (role==best_role && score>best_score))){
   best=(int)tid;best_score=score;best_slot=slot;best_role=role;
  }
 }
 if(best<0)return 0;
 Hs15Snapshot fresh;Rec team=market->teams[best];int score=0,slot=0,role=0;
 if(!hs15_collect(&fresh,team,eval) || hs28_recovery_destination(recruit,team,eval,&fresh,&score,&slot,&role)<=0 ||
   slot!=best_slot || role!=best_role || hs28_recovery_reserve(team,recruit,eval,slot,role)<=0)return 0;
 log_line("HS30_REOPEN_COMPETITION,%s,%u,%d,%d,%d,PENDING_NATIVE_PURSUIT",site,row(recruit),best,slot,role);
 return 1;
}
/* Test30: open races retain earned preference order. Acceptance is evaluated
   at the native commitment boundary, not by prematurely moving a weak firm
   school above a stronger conditional preference during every planning scan.
   Preserve stable ties and actual soft/hard choices. */
static void test29r1_hs_actionable_order(Rec recruit,U eval) {
 (void)eval;
 if(mode!=1 || faulted || !hs15_is_recruit(recruit) || scalar(recruit,9,0x108)>=4)return;
 Rec top=array_field(recruit,13);U n=top?array_count(top):0;
 if(!top || !n || n>64){release(top);return;}
 Rec entries[64]={0};unsigned tid[64],points[64],order[64];int ok=1,changed=0;
 for(U i=0;i<n;i++) {
  entries[i]=array_at(top,i);if(!entries[i]){ok=0;break;}
  tid[i]=scalar(entries[i],0,0x100);points[i]=scalar(entries[i],1,0x100);order[i]=(unsigned)i;
  if(tid[i]>=138 || points[i]>65535){ok=0;break;}
  for(U j=0;j<i;j++)if(tid[j]==tid[i])ok=0;
 }
 if(ok) {
  for(U i=1;i<n;i++) {
   unsigned x=order[i],j=(unsigned)i;
   while(j && points[x]>points[order[j-1]]) {order[j]=order[j-1];j--;}
   order[j]=x;
  }
  for(U i=0;i<n;i++)changed|=order[i]!=i;
  cache.pref_scope++; /* HS recruits below stage 4: writes no snapshot reads */
  if(changed)for(U i=0;i<n;i++) {
   portal_recovery_scalar(entries[i],0,tid[order[i]]);
   portal_recovery_scalar(entries[i],1,points[order[i]]);
  }
  cache.pref_scope--;
  if(changed)log_line("TEST30_HS_PREFERENCE_ORDER,%u,%u,%u,EARNED_POINTS_ORDER",row(recruit),tid[0],tid[order[0]]);
 }
 for(U i=0;i<n;i++)release(entries[i]);release(top);
 if(!ok)portal_fault("HS30_preference_identity");
}
static void hs29_reopen_uncommittable(U eval,const char *site){
 if(mode!=1 || faulted || !hs28_pair_window(eval))return;
 hs29_offer_reset();
 Rec franchise=context_ref(eval,6);
 Hs28AbandonedMarket *market=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*market));
 if(!franchise || !market || !hs28_abandoned_market_read(market,franchise)){
  if(market){hs28_abandoned_market_free(market);HeapFree(GetProcessHeap(),0,market);}
  release(franchise);portal_fault("HS29_reopen_pool");return;
 }
 unsigned reopened=0;U inspected=market->count;
 for(U i=0;i<market->count && !faulted;i++){
  Rec recruit=market->entries[i].recruit;unsigned stage=scalar(recruit,9,0x108);
  if(stage<5)test29r1_hs_actionable_order(recruit,eval);
  if(stage>0 && stage<5){
   Rec player=reference(recruit,5);
   PortalCleanupOwner *owner=player?portal_cleanup_slot(market->published,player,0):0;
   unsigned player_team=player?scalar(player,261,0x100):255;release(player);
   if(!owner)portal_fault("HS29_reopen_owner");
   else if(owner->owner<0 && player_team>=138){
    Hs29OfferResult r;
    if(!hs29_offers(recruit,eval,0,&r))portal_fault("HS29_reopen_offers");
    else if(!faulted && ((stage==4 && !r.leader_firm) || !r.firm || hs30_competition_path(market,recruit,eval,&r,site))){
     if(stage==4 && !r.leader_firm)log_line("HS31_SOFT_REASSESS,%s,%u,LEADER_NOT_COMMITTABLE_INTEREST_PRESERVED",site,row(recruit));
     portal_recovery_scalar(recruit,9,0);
     if(scalar(recruit,9,0x108)!=0)portal_fault("HS29_reopen_write");
     else {reopened++;log_line("HS29_REOPEN,%s,%u,%u,%d,%d,%s",site,row(recruit),stage,r.conditional,r.unavailable,r.firm?"LOST_COMPETITION_QUALIFIED_REPLACEMENT_PENDING":"NO_COMMITTABLE_OFFER_INTEREST_PRESERVED");}
    }
   }
  }
 }
 hs28_abandoned_market_free(market);HeapFree(GetProcessHeap(),0,market);
 release(franchise);hs29_offer_reset();
 if(!faulted)log_line("HS29_REOPEN_SCAN,%s,%llu,%u",site,(unsigned long long)inspected,reopened);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest29R1HsOrder(Rec r,U e){test29r1_hs_actionable_order(r,e);}
__declspec(dllexport) int DriTestHs30Catchup(Rec r,Rec t,U e){
 Hs15Snapshot s;Hs28Choice p;if(!hs15_collect(&s,t,e) || !hs28_choice(&p,r,0,t,e,&s,0))return -1;
 int firm=hs30_firm_count(r,e);
 return p.legal && firm>=0 && (p.viable || hs30_catchup_to(&p,firm?hs30_offer_leader(r,e):0));
}
__declspec(dllexport) void DriTestHs30Seed(U e){hs28_coverage_seed(e);}

__declspec(dllexport) int DriTestHs29Stage(Rec r,U e,unsigned next){return hs29_stage_block(r,e,next,0);}
__declspec(dllexport) int DriTestHs29Offer(Rec r,Rec t,U e){return hs29_offer_state(r,t,e);}
__declspec(dllexport) int DriTestHs29Next(Rec r,Rec t,U e,unsigned next){return hs29_next_allows(r,t,e,next);}
__declspec(dllexport) void DriTestHs29Reset(void){hs29_offer_reset();}
__declspec(dllexport) void DriTestHs29Reopen(U e){hs29_reopen_uncommittable(e,"FIXTURE");}
#endif
