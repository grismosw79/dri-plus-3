/* Full-market work runs at the existing preplanning boundary, before any
   native candidate/board iterators exist. Per-team admission remains native. */
typedef struct {
 U franchise_table,team_table,eval;uint32_t franchise_row,year,week,type;
 int valid;
} Hs28MaintenanceScope;
static Hs28MaintenanceScope hs28_maintenance_scope;
static void hs28_maintenance(U eval) {
 if(mode!=1 || faulted || !hs28_pair_window(eval))return;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0,teams=f?array_field(f,45):0;
 if(!f || !season || !teams){release(teams);release(season);release(f);return;}
 Hs28MaintenanceScope now={q((U)f+0x10),q((U)teams+0x10),eval,row(f),
  scalar(season,2,0x100),scalar(season,4,0x100),scalar(season,5,0x108),1};
 release(teams);release(season);release(f);
 Hs28MaintenanceScope *old=&hs28_maintenance_scope;
 if(old->valid && old->franchise_table==now.franchise_table && old->team_table==now.team_table &&
    old->eval==now.eval && old->franchise_row==now.franchise_row && old->year==now.year &&
    old->week==now.week && old->type==now.type)return;
 /* A later school's board may contain a recruit reopened by an earlier
    scan. Capture all matching roles before the first such native change. */
 hs34r4_preload(eval);if(faulted)return;
 hs28_nsd_reset();
 HsCleanupTrace trace={"begin",now.year,now.type,now.week,-1,-1,-1};
 HsCleanupTrace *previous=hs_cleanup_trace;
 hs_cleanup_trace=&trace;
 __try {
  __try {hs28_cleanup_rivals(0,eval);}
  __except(hs_cleanup_exception(&trace,GetExceptionCode())) {}
 } __finally {hs_cleanup_trace=previous;}
 if(faulted)return;
 hs29_reopen_uncommittable(eval,"PRE_PLAN");if(faulted)return;
 hs28_coverage_begin(eval);if(faulted)return;
 hs28_recover_market(eval,"PRE_PLAN");if(faulted)return;
 hs32_crowding_begin(eval);if(faulted)return;
 *old=now;
 log_line("HS28_MAINTENANCE,%u,%u,%u,COMPLETE",now.year,now.type,now.week);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestHs28Maintenance(U eval){hs28_maintenance(eval);}
__declspec(dllexport) void DriTestHs28MaintenanceReset(void){memset(&hs28_maintenance_scope,0,sizeof(hs28_maintenance_scope));}
#endif
