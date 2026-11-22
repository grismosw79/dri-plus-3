/* Test11: global maintenance only at native boundaries without live board
   iterators. Recruiting is serialized by the supported native game loop. */
typedef struct {U table;uint32_t rid,year,week,type;int valid,active;} PortalMaintenanceScope;
static PortalMaintenanceScope maintenance_scope;
static struct {U table,eval;unsigned row,year,week;} r2_first_paid;
static void r2_ordinary_funding(U eval,Rec team,unsigned year,unsigned week,unsigned funded) {
 if(!funded || week<2 || week>6 || !team)return;
 Rec f=context_ref(eval,6);
 if(f && (r2_first_paid.eval!=eval || r2_first_paid.table!=q((U)f+0x10) ||
     r2_first_paid.row!=row(f) || r2_first_paid.year!=year || week<r2_first_paid.week)) {
  r2_first_paid.table=q((U)f+0x10);r2_first_paid.eval=eval;r2_first_paid.row=row(f);
  r2_first_paid.year=year;r2_first_paid.week=week;
  log_line("R2_FIRST_ORDINARY_FUNDING,%u,%u,%u,%u",year,week,scalar(team,383,0x100),funded);
 }
 release(f);
}
static int r2_seed_grace(U eval,U table,unsigned rid,unsigned year,unsigned week,int after_batch) {
 if(week>3)return 0;
 int paid=r2_first_paid.eval==eval && r2_first_paid.table==table &&
    r2_first_paid.row==rid && r2_first_paid.year==year && r2_first_paid.week<=week;
 return !paid || (!after_batch && r2_first_paid.week==week);
}
static void portal_maintenance(U eval) {
 if(mode<0 || faulted || !eval)return;
 Rec franchise=context_ref(eval,6);
 if(!franchise){portal_fault("maintenance_franchise");return;}
 Rec season=reference(franchise,44);
 if(!season){release(franchise);portal_fault("maintenance_season");return;}
 PortalMaintenanceScope next={q((U)franchise+0x10),row(franchise),scalar(season,2,0x100),
                             scalar(season,4,0x100),scalar(season,5,0x108),1,mode};
 release(season);release(franchise);
 if(next.type!=6 || next.week>6)rc4_inactive();else rc4_restore_if_cold(eval);
 if(mode==1 && next.type==6 && next.week==2)t36_open(eval,0,"OPENING_PREPARE");
 if(next.type!=6 || next.week<2 || next.week>5)return;
 if(maintenance_scope.valid && maintenance_scope.table==next.table && maintenance_scope.rid==next.rid &&
    maintenance_scope.year==next.year && maintenance_scope.week==next.week && maintenance_scope.type==next.type &&
    maintenance_scope.active==next.active)return;
 int grace=r2_seed_grace(eval,next.table,next.rid,next.year,next.week,0);
 recover_window_site="PORTAL_MAINT_RECOVER_MARKET";portal_recover_market(eval,grace);recover_window_site=0;
 log_line("R2_SEEDED_INTEREST,%u,%u,%d,PRE_PLAN",next.year,next.week,grace);
 if(!faulted)portal_cleanup_all(eval);
 if(!faulted){maintenance_scope=next;log_line("PORTAL_MAINTENANCE,%u,%u,%u,COMPLETE",next.year,next.type,next.week);}
}
void DriAfterPortalWeekly(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 pt28_invalidate();
 if(mode!=1 || faulted)return;
 /* Cache stack: the HS block below is one window (paused around t36_open), each step ending with a sub-phase
    boundary; it closes before the portal part. The early exits leave the __try so the window always closes. */
 volatile int window=0;U eval=0;
 __try {
  eval=q(r->rbp+0x60);
  window=cache_season_window_read(eval,"POST_WEEK");
  cache_pause(1);t36_open(eval,0,"WEEK_PUBLICATION");cache_pause(0);cache_phase_boundary();
  /* Native weekly outcomes may have filled an HS-reserved positional seat. */
  for(unsigned tid=0;tid<138;tid++) {
   Rec team=portal_team_by_id(eval,tid);test29_class_invalidate(team);release(team);
  }
  cache_phase_boundary();
  portal29_observe_all(eval,"POST_WEEK");cache_phase_boundary();if(faulted)__leave;
  hs28_nsd_reset();
  hs28_coverage_end(eval);cache_phase_boundary();if(faulted)__leave;
  hs28_after_week(eval);cache_phase_boundary();if(faulted)__leave;
  /* Native HS outcomes occur outside the allocation calendar as well. */
  hs28_cleanup_rivals(0,eval);cache_phase_boundary();if(faulted)__leave;
  if(hs28_pair_window(eval)) {
   hs29_reopen_uncommittable(eval,"POST_WEEK");cache_phase_boundary();if(faulted)__leave;
   hs28_recover_market(eval,"POST_WEEK");cache_phase_boundary();if(faulted)__leave;
   hs28_coverage_seed(eval);cache_phase_boundary();if(faulted)__leave;
   /* Test34: revisit ordinary class deficits after wins, losses and cleanup.
      Reservations still require the current native budget; this does not
      fabricate offers or borrow future-week hours. */
   test34_class_refill(eval);cache_phase_boundary();if(faulted)__leave;
  }
  cache_season_window_end(window);window=0;
  Rec franchise=context_ref(eval,6),season=franchise?reference(franchise,44):0;
  int portal=season && scalar(season,5,0x108)==6;
  int planned=portal && maintenance_scope.valid && maintenance_scope.active==1 &&
              maintenance_scope.table==q((U)franchise+0x10) && maintenance_scope.rid==row(franchise) &&
              maintenance_scope.year==scalar(season,2,0x100);
  int grace=planned && r2_seed_grace(eval,q((U)franchise+0x10),row(franchise),scalar(season,2,0x100),scalar(season,4,0x100),1);
  release(season);release(franchise);
  /* New secured players can exhaust other races after pre-planning recovery.
     Rescan at this safe end-of-batch boundary, including the last portal week.
     Do not mistake the initial seeded-interest transition for a spent round:
     at least one active planning pass must have happened in this campaign. */
  if(planned){recover_window_site="POST_PORTAL_RECOVER_MARKET";portal_recover_market(eval,grace);recover_window_site=0;}
  if(portal && !faulted)portal_cleanup_all(eval);
  if(!faulted)hs34r3_checkpoint(eval);
 }
 __except(fail_exception("post_weekly_cleanup",GetExceptionCode())){}
 cache_season_window_end(window);
 cache_summary("POST_WEEK",eval);
}
void DriAfterPortalInstant(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 pt28_invalidate();
 if(mode!=1 || faulted)return;
 __try {
  Rec recruit=(Rec)q(r->rbp-0x41);U eval=t36_active_eval && t36_designated && portal_same_record(t36_designated,recruit)?t36_active_eval:q(r->rbp+0x67);
  if(!recruit || !transfer_recruit(recruit) || scalar(recruit,9,0x108)!=5)return;
  int owner=-1,valid=portal_hard_owner(recruit,eval,&owner);
  if(valid<0){portal_fault("instant_final_owner");return;}
  Rec team=(Rec)q(q(r->rbp+0x77));
  if(valid && (!team || scalar(team,383,0x100)!=(unsigned)owner)){portal_fault("instant_winner_mismatch");return;}
  if(valid){test29_class_invalidate(team);portal_cleanup_rivals(recruit,(unsigned)owner,eval);}
 } __except(fail_exception("post_instant_cleanup",GetExceptionCode())){}
}
void DriAfterPortalNsd(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 pt28_invalidate();
 if(mode!=1 || faulted){academy29_pending_clear();return;}
 __try {academy29_verify_publication(r->r13);if(!faulted)hs_finalize_published(r->r13);if(!faulted)portal_cleanup_all(r->r13);if(!faulted)hs28_cleanup_rivals(0,r->r13);if(!faulted)test30_reconcile_offer_counters(r->r13);if(!faulted)hs34r3_checkpoint(r->r13);}
 __except(fail_exception("post_nsd_cleanup",GetExceptionCode())){}
 cache_summary("POST_NSD",r->r13);
}
