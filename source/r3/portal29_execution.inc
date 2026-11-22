/* Test29: observation at native SUCCESS branches after 08143f00 has called
   08144630 and published target ordinal8. These callbacks cannot select an
   action, spend hours, grant an offer, add influence or accept a commitment.
   Generic action input cost is distinct from its influence argument. Pitch
   native feedback carries zero hours; it is not interpreted as a free action. */
#ifdef DRI_EMULATION_TEST
static unsigned portal29_execution_test[8];
#endif
/* Cache stack: the observation after the calendar check runs inside a window (closed at its end, and by the hook
   bodies after an exception). */
static int exec_observe_window;
static void portal29_execution_observe(U eval,Rec team,Rec target,int action,int native_cost,int points,const char *site) {
 if(mode!=1 || faulted || !eval || !team || !target)return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 Rec user=reference(team,407);if(user){release(user);return;}
 Rec recruit=reference(target,10);if(!recruit)return;
 if(!transfer_recruit(recruit)){release(recruit);return;}
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 unsigned year=season?scalar(season,2,0x100):0,type=season?scalar(season,5,0x108):0,week=season?scalar(season,4,0x100):0;
 release(season);release(f);
 if(type!=6 || week<2 || week>6 || action<0 || action>12 || native_cost<0 || native_cost>10000){release(recruit);return;}
 exec_observe_window=cache_callback_window("EXEC_OBSERVE",eval);
 /* Native guards and the success-only hook prove an executed application.
    Re-read offered capacity and native assignment independently for firm-role
    attribution; allocation is never inferred from the influence result. */
 unsigned status=scalar(target,12,0x108),mask=0,total_points=scalar(target,8,0x100);
 unsigned allocated=hs28_productive_allocated(target,eval,&mask);
 Rec budget=reference(team,224);
 unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
 PortalCommitPlan plan,now={0};AcquireSRWLockExclusive(&commitment_lock);plan=commitment_plans[tid];ReleaseSRWLockExclusive(&commitment_lock);
 int scope_ok=portal_commit_scope(&now,team,eval);
 int matched=scope_ok && portal_commit_plan_matches(&plan,&now);
 /* The complete board was checked at POST_FUNDING. Do not rescan all targets
    for every executed action; missing/reused/reloaded scope remains unknown. */
 int budget_ok=scope_ok && portal29_budget_matches(tid,eval,&now,assigned,total,allocated);
 int index=matched?portal_commit_index(&plan,recruit):-1,slot=index>=0?plan.entries[index].slot:0,round=index>=0?plan.entries[index].round:0;
 int offered=status>=2 && status<=4;
 int legal=offered && scalar(recruit,9,0x108)<5 && !portal_decision(recruit,team,eval,"EXECUTED_CHECK");
 int eligible=legal?native_recruit_eligible(eval,team,recruit):0;
 int action_assigned=action==2?(mask&32u)!=0:(action>=5 && action<=9 && (mask&(1u<<(action-5)))!=0);
 int paid_assignment=budget_ok && action_assigned && allocated>0 && (action==2 || (native_cost>0 && (unsigned)native_cost<=allocated));
 /* Only paid planned primaries need the full arbitration scan here. The
    complete funding witnesses make the same check; backups are conditional
    until a subsequent plan promotes them. No acceptance cache is reused
    across an ownership, offer, point or roster change during native actions. */
 int firm=legal && eligible && paid_assignment && index>=0 && round==1 &&
  test29r2_portal_priority_clear(recruit,team,eval,"FIRM_EXECUTED_CHECK");
 int conditional=legal && eligible && paid_assignment && index>=0 && !firm;
#ifdef DRI_EMULATION_TEST
 portal29_execution_test[0]=row(recruit);portal29_execution_test[1]=(unsigned)action;
 portal29_execution_test[2]=(unsigned)native_cost;portal29_execution_test[3]=(unsigned)points;
 portal29_execution_test[4]=total_points;portal29_execution_test[5]=(unsigned)firm;
 portal29_execution_test[6]=(unsigned)conditional;portal29_execution_test[7]=(unsigned)paid_assignment;
#endif
 log_line("PORTAL29_ACTION_EXECUTED,%s,%u,%u,%u,%u,%u,%d,%d,%d,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%s",
  site,year,type,week,tid,row(recruit),action,native_cost,points,total_points,allocated,assigned,total,slot,round,offered,legal,firm,conditional,budget_ok,
  action==2?"PITCH_EXECUTED_COST_SEPARATE_FROM_CURRENT_ALLOCATION":"GENERIC_EXECUTED_NATIVE_COST_NOT_INFLUENCE");
 release(recruit);
 cache_callback_window_end(exec_observe_window);exec_observe_window=0;
}
void DriAfterPortal29GenericAction(Registers *r) {
 cache_hook_entry(CACHE_HOOK_TEAM);offer_cap_team_at(r->rbp+0x67);
 if(mode!=1 || faulted)return;
 __try {
  U th=q(r->rbp+0x67);Rec team=th?(Rec)q(th):0,target=r->rsi?(Rec)q(r->rsi):0;
  portal29_execution_observe(r->r14,team,target,(int)r->r15,(int)r->r12,*(int*)(r->rbp+0x7f),"GENERIC_SUCCESS");
 } __except(fail_exception("portal29_generic_witness",GetExceptionCode())){}
 if(exec_observe_window){cache_callback_window_end(exec_observe_window);exec_observe_window=0;}
}
void DriAfterPortal29Pitch(Registers *r) {
 cache_hook_entry(CACHE_HOOK_TEAM);offer_cap_team((const Rec *)r->r12);
 if(mode!=1 || faulted)return;
 __try {
  Rec team=r->r12?(Rec)q(r->r12):0,target=r->r13?(Rec)q(r->r13):0;
  portal29_execution_observe(r->rsi,team,target,2,0,*(int*)(r->rbp-0x7c),"PITCH_SUCCESS");
 } __except(fail_exception("portal29_pitch_witness",GetExceptionCode())){}
 if(exec_observe_window){cache_callback_window_end(exec_observe_window);exec_observe_window=0;}
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestPortal29GenericAction(Registers*r){DriAfterPortal29GenericAction(r);}
__declspec(dllexport) void DriTestPortal29Pitch(Registers*r){DriAfterPortal29Pitch(r);}
__declspec(dllexport) unsigned DriTestPortal29Execution(unsigned index){return index<8?portal29_execution_test[index]:0;}
#endif
