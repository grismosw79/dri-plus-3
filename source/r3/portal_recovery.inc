/* Test29: recover when current finalists have no independent actionable
   opening. A paid conditional backup cannot close reconsideration merely by
   retaining a displayed scholarship. Called before live board iterators
   or candidate-score snapshots exist. The caller owns once-per-advance scope.

   This never invents influence, schools, offers, or signing ownership. Real
   offers outside the closed finalist set retain their earned influence. A
   withdrawn/illegal finalist's stale influence is cleared; the same school
   records are stably reordered before returning the recruit to Top10.
   All candidate records and mutations are validated before the first write. */
#define PORTAL_RECOVERY_TOP 10
typedef struct {
 Rec record;
 unsigned tid,influence,keep,drop;
} PortalRecoverySchool;
typedef struct {
 Rec recruit;
 unsigned stage,count,kept,cleared,dropped;
 PortalRecoverySchool schools[PORTAL_RECOVERY_TOP];
 unsigned order[PORTAL_RECOVERY_TOP];
} PortalRecoveryPlan;
typedef struct {int valid,prestige,flex_ready,flex_allowed,team_excess,hs_reserved;uint32_t exception_states;PortalQuality quality;} PortalRecoveryRoom;

/* Native setters in 081486b0 (stage) and 081a4273 (integer offer count)
   use the same table vfunc +148 and then 08055870(table). +140 is boolean. */
static void portal_recovery_scalar(Rec record,int ordinal,unsigned value) {
 /* Cache stack: a recruit stage write that stays below 5, or an HS preference re-ordering write, is one that no
    HS snapshot reads (classified before the write; one extra stage read, inside a window only). */
 int excused=cache.active && ((ordinal==9 && value<5 && scalar(record,9,0x108)<5) || (ordinal<=1 && cache.pref_scope));
 U table=q((U)record+0x10),key=field(schema(record),ordinal);
 if(ordinal==9)cache_note_stage_write();
 ((void(*)(U,uint32_t,U,unsigned))q(q(table)+0x148))(table,row(record),key,value);
 ((void(*)(U))(game_base+0x080595e0))(table);cache_note_write();
 if(excused)cache.excused++;
}
static void portal_recovery_release(PortalRecoveryPlan *p) {
 for(unsigned i=0;i<p->count;i++)release(p->schools[i].record);
 release(p->recruit);memset(p,0,sizeof(*p));
}
static int portal_recovery_room_ok(Rec recruit,Rec team,U eval,PortalRecoveryRoom *cache) {
 Rec user=reference(team,407);
 if(user){release(user);return 1;} /* Existing CPU role policy does not bind users. */
 Rec player=reference(recruit,5);if(!player)return -1;
 unsigned pos=scalar(player,3,0x108),ovr=scalar(player,191,0x100);release(player);
 unsigned tid=scalar(team,383,0x100);
 if(pos>=POSITIONS || ovr>99 || tid>=138)return -1;
 PortalRecoveryRoom *slot=cache+tid*POSITIONS+pos;
 if(!slot->valid) {
  PortalRoster roster;
  if(!portal_roster(&roster,team,eval,(int)pos))return -1;
  slot->quality=roster.projected;slot->prestige=roster.prestige;slot->valid=1;
 }
 int projected=portal_projected_ovr((int)ovr,portal_projection_step(eval));
 if(faulted)return -1;
 int reserved[22]={0};test29_hs_reserved(team,eval,reserved);if(faulted)return -1;
 if(slot->quality.ideal>0 && (slot->quality.count>=slot->quality.ideal || reserved[pos]>0) && !slot->flex_ready) {
  Hs15Snapshot shot;if(!hs15_collect(&shot,team,eval))return -1;
  PortalQuality rooms[22];
  if(!hs15_portal_rooms(&shot,rooms,slot->prestige,portal_projection_step(eval)))return -1;
  dri_portal_set_hs_reservations(rooms,reserved,slot->prestige);
  /* A recovery pass cannot restore an offer to a full room whose shared
     budget is already exhausted. Cache the same team snapshot for all rooms. */
  for(unsigned p=0;p<POSITIONS;p++) {
   PortalRecoveryRoom *other=cache+tid*POSITIONS+p;
   other->flex_ready=1;other->team_excess=rooms[p].team_excess;
   other->flex_allowed=rooms[p].flex_allowed;other->exception_states=rooms[p].exception_states;
   other->hs_reserved=rooms[p].hs_reserved;
  }
 }
 slot->quality.flex_allowed=slot->flex_allowed;
 slot->quality.team_excess=slot->team_excess;
 slot->quality.exception_states=slot->exception_states;
 slot->quality.hs_reserved=slot->hs_reserved;
 int blocked=dri_quality_block_candidate(&slot->quality,slot->prestige,projected,(int)ovr);
 return blocked==0;
}
static int portal_recovery_plan(Rec recruit,U eval,int opening_grace,
                                PortalRecoveryRoom *cache,PortalRecoveryPlan *plan) {
 unsigned stage=scalar(recruit,9,0x108);
 if(!transfer_recruit(recruit) || stage==0 || stage>=6)return 0;
 if(stage<5) {
  int owner=-1,published=portal_published_owner(recruit,eval,&owner);
  if(published<0)return -1;
  if(published)return 0;
 }
 if(opening_grace && stage<5 && scalar(recruit,14,0x100)==0)return 0;
 if(stage==5) {
  int owner=-1,valid=portal_hard_owner(recruit,eval,&owner);
  if(valid<0)return -1;
  if(valid)return 0;
  /* An illegal academy destination may already have a published acquisition
     in a loaded save. Do not reopen that player into duplicate ownership;
     leave the inconsistent save untouched for explicit repair. */
  int published=portal_published_owner(recruit,eval,&owner);
  if(published!=0)return -1;
 }
 Rec top=array_field(recruit,13);if(!top)return -1;
 U count=array_count(top);
 if(count==0 || count>PORTAL_RECOVERY_TOP){release(top);return -1;}
 plan->stage=stage;int viable=0,ok=1;
 for(U i=0;i<count;i++) {
  Rec entry=array_at(top,i);if(!entry){ok=0;break;}
  PortalRecoverySchool *school=&plan->schools[plan->count++];school->record=entry;
  school->tid=scalar(entry,0,0x100);school->influence=scalar(entry,1,0x100);
  if(school->tid>=138 || school->influence>65535){ok=0;break;}
  for(U j=0;j<i;j++) {
   if(plan->schools[j].tid==school->tid || portal_same_record(plan->schools[j].record,entry))ok=0;
  }
  if(!ok)break;
  Rec team=portal_team_by_id(eval,school->tid);if(!team){ok=0;break;}
  int offered=portal_active_offer(recruit,team),allowed=0;
  if(offered<0){release(team);ok=0;break;}
  if(offered && portal_academy_allowed(recruit,school->tid)) {
   allowed=portal_recovery_room_ok(recruit,team,eval,cache);
   if(allowed<0){release(team);ok=0;break;}
  }
  school->keep=(unsigned)(offered && allowed);
  school->drop=(unsigned)(offered && !allowed);
  if(school->keep) {
   plan->kept++;
   if(stage<5 && !locked_out(recruit,team,eval)) {
    int committable=test29r2_portal_committable(recruit,team,eval,"RECOVERY_CHECK");
    if(committable && (scalar(recruit,14,0x100)!=1 || r2_portal_live(recruit,team,eval)))viable++;
    else log_line("PORTAL29_CONDITIONAL_PATH,%u,%u,0,CURRENT_ACCEPTANCE_UNAVAILABLE_RECONSIDER_BEFORE_FUNDING",row(recruit),school->tid);
   }
  } else if(school->influence)plan->cleared++;
  if(school->drop)plan->dropped++;
  release(team);
 }
 release(top);
 if(!ok)return -1;
 if(viable)return 0;
 /* Preserve record identity/capacity: reorder the two verified scalar values,
    not reference-array membership. All influence values decrease or stay put. */
 for(unsigned i=0;i<plan->count;i++)plan->order[i]=i;
 for(unsigned i=1;i<plan->count;i++) {
  unsigned candidate=plan->order[i],j=i;
  unsigned value=plan->schools[candidate].keep?plan->schools[candidate].influence:0;
  while(j) {
   unsigned previous=plan->order[j-1];
   unsigned old=plan->schools[previous].keep?plan->schools[previous].influence:0;
   if(old>=value)break;
   plan->order[j]=previous;j--;
  }
  plan->order[j]=candidate;
 }
 ((void(*)(Rec))vfunc(recruit,0))(recruit);plan->recruit=recruit;
 return 1;
}
static void portal_recover_market(U eval,int opening_grace) {
 if(mode<0 || faulted)return;
 Rec franchise=context_ref(eval,6),transfers=0;
 PortalRecoveryPlan *plans=0;PortalRecoveryRoom *rooms=0;
 U used=0,count=0,manager=q(eval+0xc8);const char *error=0;
 if(!franchise){error="recovery_franchise";goto cleanup;}
 transfers=array_field(franchise,46);
 if(!transfers){error="recovery_transfers";goto cleanup;}
 count=array_count(transfers);
 if(count>12000){error="recovery_count";goto cleanup;}
 if(!count)goto cleanup;
 plans=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*plans));
 rooms=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*POSITIONS*sizeof(*rooms));
 if(!plans || !rooms){error="recovery_memory";goto cleanup;}
 for(U i=0;i<count;i++) {
  Rec recruit=array_at(transfers,i);
  if(!recruit){error="recovery_recruit";goto cleanup;}
  int result=portal_recovery_plan(recruit,eval,opening_grace,rooms,&plans[used]);
  release(recruit);
  if(result<0){error="recovery_plan";goto cleanup;}
  if(result) {
   for(U j=0;j<used;j++)if(portal_same_record(plans[j].recruit,plans[used].recruit)) {
    error="recovery_duplicate";goto cleanup;
   }
   used++;
  } else portal_recovery_release(&plans[used]);
 }
 /* Do not apply a partially read or stale plan. The coordinator must ensure
    no native iterator is live while it invokes this maintenance pass. */
 for(U i=0;i<used;i++) {
  PortalRecoveryPlan *p=&plans[i];
  if(p->dropped && !manager){error="recovery_manager";goto cleanup;}
  if(scalar(p->recruit,9,0x108)!=p->stage){error="recovery_stage_changed";goto cleanup;}
  for(unsigned j=0;j<p->count;j++) {
   PortalRecoverySchool *v=&p->schools[j];
   if(scalar(v->record,0,0x100)!=v->tid || scalar(v->record,1,0x100)!=v->influence) {
    error="recovery_interest_changed";goto cleanup;
   }
  }
 }
 for(U i=0;i<used;i++) {
  PortalRecoveryPlan *p=&plans[i];
  log_line("PORTAL_REOPEN,%u,%u,0,%u,%u,%u,%s",row(p->recruit),p->stage,p->kept,p->cleared,p->dropped,mode?"ACTIVE":"WOULD_REOPEN");
  if(mode!=1)continue;
  /* Stage 5 without accepted ownership is an orphan, not a secured player.
     Reopen before native removal: its own-hard-commit protection must not keep
     an academy's invalid offer alive. Every plan was validated above. */
  portal_recovery_scalar(p->recruit,9,0);portal_forget_owner(p->recruit);
  if(scalar(p->recruit,9,0x108)!=0){error="recovery_stage_write";goto cleanup;}
  /* Remove only an actually offered school which is now prohibited or room
     blocked. Native removal performs offer-count/cache/visit/hour cleanup. */
  for(unsigned j=0;j<p->count;j++)if(p->schools[j].drop) {
   Rec team=portal_team_by_id(eval,p->schools[j].tid);
   if(!team){error="recovery_drop_team";goto cleanup;}
   U candidate[6]={(U)p->recruit,0,0,0,0,0};
   int result=((int(*)(U,Rec*,void*))(game_base+0x081a7b10))(manager,&team,candidate);cache_note_removal();
   int offered=portal_active_offer(p->recruit,team);
   if(!result && offered==0)test29_class_invalidate(team);
   release(team);
   if(result || offered!=0){error="recovery_drop";goto cleanup;}
  }
  for(unsigned j=0;j<p->count;j++) {
   PortalRecoverySchool *dest=&p->schools[j],*src=&p->schools[p->order[j]];
   unsigned influence=src->keep?src->influence:0;
   if(dest->tid!=src->tid)portal_recovery_scalar(dest->record,0,src->tid);
   if(dest->influence!=influence)portal_recovery_scalar(dest->record,1,influence);
   if(scalar(dest->record,0,0x100)!=src->tid || scalar(dest->record,1,0x100)!=influence) {
    error="recovery_interest_write";goto cleanup;
   }
  }
 }
cleanup:
 if(plans) {
  /* The extra slot owns any incomplete plan from the validation loop. */
  for(U i=0;i<=used;i++)portal_recovery_release(&plans[i]);
  HeapFree(GetProcessHeap(),0,plans);
 }
 if(rooms)HeapFree(GetProcessHeap(),0,rooms);
 release(transfers);release(franchise);
 if(error)portal_fault(error);
}

/* Test12: only at the NSD offered-team fallback ENTRY, before it constructs its
   native target iterator. Recheck after preceding deadline signings. Include a
   real offered school omitted from TopSchoolsList using its earned target total.
   No new offer, points, automatic commitment or relaxed dealbreaker is created. */
static int portal_offer_total(Rec recruit,Rec team,unsigned *total) {
 Rec board=reference(team,224),a=board?array_field(board,0):0;release(board);
 if(!a)return 0;U n=array_count(a);if(n>MAX_BOARD){release(a);return -1;}
 int found=0,matched=0;
 for(U i=0;i<n;i++) {
  Rec target=array_at(a,i),r=target?reference(target,10):0;
  if(!target || !r){release(r);release(target);found=-1;break;}
  if(portal_same_record(r,recruit)) {
   unsigned status=scalar(target,12,0x108),influence=scalar(target,8,0x100);
   if(matched++ || status>4 || influence>65535){release(r);release(target);found=-1;break;}
   if(status>=2){found=1;*total=influence;}
  }
  release(r);release(target);
 }
 release(a);return found;
}
/* Test15 HS/JUCO recovery is limited to existing, earned CPU pursuits at
   genuine future-roster vacancies. Native eligibility, offers, funding and
   commitment thresholds are unchanged. A viable finalist or any offered user
   school keeps the entire race intact. Only the shortlist and its closed stage
   can change; no scholarship or signing is created or removed here. */
typedef struct {Hs15Snapshot *team[138];} Hs15RecoveryCache;
static void hs15_recovery_cache_free(Hs15RecoveryCache *cache) {
 for(unsigned i=0;i<138;i++)if(cache->team[i])HeapFree(GetProcessHeap(),0,cache->team[i]);
 memset(cache,0,sizeof(*cache));
}
static Hs15Snapshot *hs15_recovery_snapshot(Hs15RecoveryCache *cache,Rec team,U eval) {
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 if(!cache->team[tid]) {
  Hs15Snapshot *shot=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*shot));
  if(!shot)return 0;
  if(!hs15_collect(shot,team,eval)){HeapFree(GetProcessHeap(),0,shot);return 0;}
  cache->team[tid]=shot;
 }
 return cache->team[tid];
}
static int hs15_recovery_window(U eval) {
 return hs15_window(eval);
}
/* Historical Test15 earned-offer recovery remains in the emulator only.
   Production R8 uses hs28_recover_market and preserves the entire interest list. */
#ifdef DRI_EMULATION_TEST
static void hs15_reopen(Rec recruit,U eval,Hs15RecoveryCache *cache,const char *site) {
 if(mode!=1 || faulted || !hs15_is_recruit(recruit))return;
 unsigned stage=scalar(recruit,9,0x108);
 if(!stage || stage>=5)return;
 int owner=-1,published=portal_published_owner(recruit,eval,&owner);
 if(published<0){portal_fault("HS_recovery_published_owner");return;}
 if(published)return;
 PortalRecoveryPlan plan={0};Rec top=array_field(recruit,13);const char *error=0;
 unsigned tids[138],points[138];int n=0;
 if(!top){error="HS_recovery_top";goto cleanup;}
 U count=array_count(top);
 if(!count || count>PORTAL_RECOVERY_TOP){error="HS_recovery_top_count";goto cleanup;}
 plan.stage=stage;
 for(U i=0;i<count;i++) {
  Rec entry=array_at(top,i);if(!entry){error="HS_recovery_interest";goto cleanup;}
  PortalRecoverySchool *s=&plan.schools[plan.count++];s->record=entry;
  s->tid=scalar(entry,0,0x100);s->influence=scalar(entry,1,0x100);
  if(s->tid>=138 || s->influence>65535){error="HS_recovery_interest_value";goto cleanup;}
  for(U j=0;j<i;j++)if(plan.schools[j].tid==s->tid || portal_same_record(plan.schools[j].record,entry)) {
   error="HS_recovery_duplicate";goto cleanup;
  }
 }
 for(unsigned tid=0;tid<138;tid++) {
  Rec team=portal_team_by_id(eval,tid);if(!team){error="HS_recovery_team";goto cleanup;}
  unsigned influence=0;int offered=portal_offer_total(recruit,team,&influence);
  if(offered<0){release(team);error="HS_recovery_offer";goto cleanup;}
  if(!offered){release(team);continue;}
  Rec user=reference(team,407);
  if(user){release(user);release(team);goto cleanup;}
  int listed=0;
  for(unsigned j=0;j<plan.count;j++)if(plan.schools[j].tid==tid) {
   listed=1;influence=plan.schools[j].influence;break;
  }
  /* The same native predicate is used by original HS and portal board scoring.
     It deliberately leaves the separate shortlist lockout to this recovery. */
  int eligible=native_recruit_eligible(eval,team,recruit);
  if(eligible) {
   Hs15Snapshot *shot=hs15_recovery_snapshot(cache,team,eval);
   if(!shot){release(team);error="HS_recovery_projection";goto cleanup;}
   int blocked=hs15_snapshot_block(shot,recruit);
   if(blocked<0){release(team);error="HS_recovery_candidate";goto cleanup;}
   /* Ordinary annual developmental recruiting is a viable finalist even if
      this school does not qualify for a vacancy recovery exception. */
   if(!blocked && listed && !locked_out(recruit,team,eval)){release(team);goto cleanup;}
   int vacancy=hs15_snapshot_recovery(shot,recruit);
   if(vacancy<0){release(team);error="HS_recovery_vacancy";goto cleanup;}
   if(!blocked && vacancy && influence){tids[n]=tid;points[n++]=influence;}
  }
  release(team);if(faulted)goto cleanup;
 }
 /* Do not disturb a closed race unless a real earned, eligible vacancy pursuit
    can continue. Schools with no offer or no earned points receive no rescue. */
 if(!n)goto cleanup;
 for(int i=1;i<n;i++) {
  unsigned tid=tids[i],point=points[i];int j=i;
  while(j>0 && (points[j-1]<point || (points[j-1]==point && tids[j-1]>tid))) {
   tids[j]=tids[j-1];points[j]=points[j-1];j--;
  }
  tids[j]=tid;points[j]=point;
 }
 unsigned keep=n<(int)plan.count?(unsigned)n:plan.count,used=keep;
 for(unsigned i=0;i<plan.count && used<plan.count;i++) {
  unsigned tid=plan.schools[i].tid;int duplicate=0;
  for(unsigned j=0;j<used;j++)if(tids[j]==tid)duplicate=1;
  if(!duplicate){tids[used]=tid;points[used++]=0;}
 }
 if(used!=plan.count){error="HS_recovery_permutation";goto cleanup;}
 if(scalar(recruit,9,0x108)!=stage){error="HS_recovery_stage_changed";goto cleanup;}
 for(unsigned j=0;j<plan.count;j++) {
  PortalRecoverySchool *s=&plan.schools[j];
  if(scalar(s->record,0,0x100)!=s->tid || scalar(s->record,1,0x100)!=s->influence) {
   error="HS_recovery_interest_changed";goto cleanup;
  }
 }
 portal_recovery_scalar(recruit,9,0);
 if(scalar(recruit,9,0x108)!=0){error="HS_recovery_stage_write";goto cleanup;}
 for(unsigned j=0;j<plan.count;j++) {
  portal_recovery_scalar(plan.schools[j].record,0,tids[j]);
  portal_recovery_scalar(plan.schools[j].record,1,points[j]);
  if(scalar(plan.schools[j].record,0,0x100)!=tids[j] || scalar(plan.schools[j].record,1,0x100)!=points[j]) {
   error="HS_recovery_interest_write";goto cleanup;
  }
 }
 log_line("HS15_REOPEN,%s,%u,%u,0,%u,EXISTING_EARNED_OFFERS",site,row(recruit),stage,keep);
 for(unsigned j=0;j<keep;j++)log_line("HS15_REOPEN_SUITOR,%u,%u,%u",row(recruit),tids[j],points[j]);
cleanup:
 release(top);portal_recovery_release(&plan);if(error)portal_fault(error);
}
/* Called before native board iterators exist. Snapshot offered recruits first;
   all team roster summaries can then be reused because reopening itself never
   accepts a recruit or changes a roster. The next callback starts fresh. */
static void hs15_recover_team(Rec team,U eval) {
 if(mode!=1 || faulted || !team)return;
 Rec user=reference(team,407);if(user){release(user);return;}
 int window=hs15_recovery_window(eval);
 if(window<0){portal_fault("HS_recovery_season");return;}if(!window)return;
 Hs15RecoveryCache cache={0};Rec pending[MAX_BOARD]={0};unsigned used=0;
 Rec board=reference(team,224),a=board?array_field(board,0):0;release(board);
 const char *error=0;
 if(!a)goto cleanup;
 U n=array_count(a);if(n>MAX_BOARD){error="HS_recovery_board_count";goto cleanup;}
 for(U i=0;i<n;i++) {
  Rec target=array_at(a,i),r=target?reference(target,10):0;
  if(!target || !r){release(r);release(target);error="HS_recovery_target";goto cleanup;}
  unsigned status=scalar(target,12,0x108),stage=scalar(r,9,0x108);
  if(status>4){release(r);release(target);error="HS_recovery_status";goto cleanup;}
  if(status>=2 && hs15_is_recruit(r) && stage>0 && stage<5) {
   for(unsigned j=0;j<used;j++)if(portal_same_record(pending[j],r))error="HS_recovery_board_duplicate";
   if(error){release(r);release(target);goto cleanup;}
   pending[used++]=r;r=0;
  }
  release(r);release(target);
 }
 release(a);a=0;
 for(unsigned i=0;i<used && !faulted;i++) {
  Rec r=pending[i];
  if(!locked_out(r,team,eval))continue;
  Hs15Snapshot *shot=hs15_recovery_snapshot(&cache,team,eval);
  if(!shot){error="HS_recovery_projection";break;}
  int allowed=hs15_snapshot_recovery(shot,r);
  if(allowed<0){error="HS_recovery_candidate";break;}
  if(allowed)hs15_reopen(r,eval,&cache,"PREBOARD");
 }
cleanup:
 for(unsigned i=0;i<used;i++)release(pending[i]);
 release(a);hs15_recovery_cache_free(&cache);if(error)portal_fault(error);
}
/* Historical recovery retained only for regression fixtures. Test26's native
   deadline entry selects directly from active offers without rewriting rank. */
static void hs15_deadline_recover(Rec recruit,U eval) {
 if(mode!=1 || faulted || !hs15_is_recruit(recruit))return;
 int window=hs15_recovery_window(eval);
 if(window<0){portal_fault("HS_recovery_season");return;}if(!window)return;
 Hs15RecoveryCache cache={0};hs15_reopen(recruit,eval,&cache,"NATIVE_FALLBACK");
 hs15_recovery_cache_free(&cache);
}
static void portal_deadline_recover(Rec recruit,U eval) {
 if(recruit && !transfer_recruit(recruit)){hs15_deadline_recover(recruit,eval);return;}
 if(mode!=1 || faulted || !recruit || !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return;
 PortalRecoveryPlan plan={0};Rec top=array_field(recruit,13);const char *error=0;
 unsigned tids[138],points[138];int drop[138]={0},n=0,viable=0;
 if(!top){error="deadline_top";goto cleanup;}
 U count=array_count(top);
 if(!count || count>PORTAL_RECOVERY_TOP){error="deadline_top_count";goto cleanup;}
 plan.stage=scalar(recruit,9,0x108);
 for(U i=0;i<count;i++) {
  Rec entry=array_at(top,i);if(!entry){error="deadline_interest";goto cleanup;}
  PortalRecoverySchool *s=&plan.schools[plan.count++];s->record=entry;
  s->tid=scalar(entry,0,0x100);s->influence=scalar(entry,1,0x100);
  if(s->tid>=138 || s->influence>65535){error="deadline_interest_value";goto cleanup;}
  for(U j=0;j<i;j++)if(plan.schools[j].tid==s->tid || portal_same_record(plan.schools[j].record,entry)){error="deadline_duplicate";goto cleanup;}
 }
 for(unsigned tid=0;tid<138;tid++) {
  Rec team=portal_team_by_id(eval,tid);if(!team){error="deadline_team";goto cleanup;}
  unsigned influence=0;int offered=portal_offer_total(recruit,team,&influence);
  if(offered<0){release(team);error="deadline_offer";goto cleanup;}
  if(!offered){release(team);continue;}
  int ok=portal_academy_allowed(recruit,tid) && !portal_decision(recruit,team,eval,"DEADLINE_RECOVERY");
  if(faulted || mode!=1){release(team);goto cleanup;}
  /* Same native dealbreaker predicate used before original portal scoring.
     Rank lockout is deliberately separate: that is what recovery repairs. */
  if(ok)ok=native_recruit_eligible(eval,team,recruit);
  if(faulted){release(team);goto cleanup;}
  drop[tid]=!ok;
  int listed=0;
  for(unsigned j=0;j<plan.count;j++)if(plan.schools[j].tid==tid) {
   listed=1;influence=plan.schools[j].influence;break;
  }
  if(ok && listed && !locked_out(recruit,team,eval))viable++;
  if(ok && influence){tids[n]=tid;points[n++]=influence;}
  release(team);
 }
 if(viable)goto cleanup;
 for(int i=1;i<n;i++) {
  unsigned tid=tids[i],point=points[i];int j=i;
  while(j>0 && (points[j-1]<point || (points[j-1]==point && tids[j-1]>tid))){tids[j]=tids[j-1];points[j]=points[j-1];j--;}
  tids[j]=tid;points[j]=point;
 }
 unsigned keep=n< (int)plan.count?(unsigned)n:plan.count,used=keep;
 for(unsigned i=0;i<plan.count && used<plan.count;i++) {
  unsigned tid=plan.schools[i].tid;int duplicate=0;
  for(unsigned j=0;j<used;j++)if(tids[j]==tid)duplicate=1;
  if(!duplicate){tids[used]=tid;points[used++]=0;}
 }
 if(used!=plan.count){error="deadline_permutation";goto cleanup;}
 int changed=plan.stage!=0;
 for(unsigned j=0;j<plan.count;j++)if(plan.schools[j].tid!=tids[j] || plan.schools[j].influence!=points[j])changed=1;
 if(!changed)goto cleanup;
 U manager=q(eval+0xc8);if(!manager){error="deadline_manager";goto cleanup;}
 portal_recovery_scalar(recruit,9,0);portal_forget_owner(recruit);
 if(scalar(recruit,9,0x108)!=0){error="deadline_stage_write";goto cleanup;}
 for(unsigned tid=0;tid<138;tid++)if(drop[tid]) {
  Rec team=portal_team_by_id(eval,tid);if(!team){error="deadline_drop_team";goto cleanup;}
  U candidate[6]={(U)recruit,0,0,0,0,0};
  int result=((int(*)(U,Rec*,void*))(game_base+0x081a7b10))(manager,&team,candidate);cache_note_removal();
  int offered=portal_active_offer(recruit,team);release(team);
  if(result || offered){error="deadline_drop";goto cleanup;}
 }
 for(unsigned j=0;j<plan.count;j++) {
  portal_recovery_scalar(plan.schools[j].record,0,tids[j]);
  portal_recovery_scalar(plan.schools[j].record,1,points[j]);
  if(scalar(plan.schools[j].record,0,0x100)!=tids[j] || scalar(plan.schools[j].record,1,0x100)!=points[j]){error="deadline_interest_write";goto cleanup;}
 }
 log_line("PORTAL_LATE_REOPEN,%u,%u,0,%u,BEFORE_NATIVE_FALLBACK",row(recruit),plan.stage,keep);
 for(unsigned j=0;j<keep;j++)log_line("PORTAL_LATE_SUITOR,%u,%u,%u,EXISTING_OFFER",row(recruit),tids[j],points[j]);
cleanup:
 release(top);portal_recovery_release(&plan);if(error)portal_fault(error);
}
#endif
