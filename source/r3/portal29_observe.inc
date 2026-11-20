/* Test29 read-only portal execution witnesses. POST_FUNDING is the verified
   native allocation convergence, before temporary budgets reset. Productive
   allocated actions are separate from raw schema ProspectHoursSpentCurrent and weekly influence delta.
   A plan weight alone is never reported as funding or a firm destination. */
typedef struct {U eval,team,table;uint32_t row;uint64_t start;int valid;} Portal29FundingClock;
static Portal29FundingClock portal29_clocks[PORTAL_FUNDING_EVALUATORS];
static SRWLOCK portal29_clock_lock=SRWLOCK_INIT;
/* One durable numeric stamp per school, not one per evaluator slot. Reusing
   an evaluator for another school cannot erase that school's funding proof. */
typedef struct {
 U eval,franchise_table,franchise_handle,team_table;
 uint32_t franchise_row,team_row;unsigned year,week,type,assigned,total,productive;int valid;
} Portal29BudgetStamp;
static Portal29BudgetStamp portal29_budget_stamps[138];
static SRWLOCK portal29_budget_lock=SRWLOCK_INIT;
static void portal29_budget_reset(void) {
 AcquireSRWLockExclusive(&portal29_budget_lock);memset(portal29_budget_stamps,0,sizeof(portal29_budget_stamps));ReleaseSRWLockExclusive(&portal29_budget_lock);
}
static void portal29_budget_store(unsigned tid,U eval,const PortalCommitPlan *scope,unsigned assigned,unsigned total,unsigned productive,int valid) {
 Portal29BudgetStamp stamp={eval,scope->franchise_table,scope->franchise_handle,scope->team_table,
  scope->franchise_row,scope->team_row,scope->year,scope->week,scope->type,assigned,total,productive,valid};
 AcquireSRWLockExclusive(&portal29_budget_lock);portal29_budget_stamps[tid]=stamp;ReleaseSRWLockExclusive(&portal29_budget_lock);
}
static int portal29_budget_matches(unsigned tid,U eval,const PortalCommitPlan *scope,unsigned assigned,unsigned total,unsigned allocated) {
 Portal29BudgetStamp stamp;AcquireSRWLockExclusive(&portal29_budget_lock);stamp=portal29_budget_stamps[tid];ReleaseSRWLockExclusive(&portal29_budget_lock);
 return stamp.valid && stamp.eval==eval && stamp.franchise_table==scope->franchise_table &&
  stamp.franchise_handle==scope->franchise_handle && stamp.franchise_row==scope->franchise_row &&
  stamp.team_table==scope->team_table && stamp.team_row==scope->team_row && stamp.year==scope->year &&
  stamp.week==scope->week && stamp.type==scope->type && stamp.assigned==assigned && stamp.total==total &&
  allocated<=stamp.productive && stamp.productive<=assigned && assigned<=total;
}

#ifdef DRI_EMULATION_TEST
static struct {unsigned rid,flags;} portal29_test_witness[MAX_BOARD];
static int portal29_test_witness_count;
#endif
static void portal29_funding_begin(Rec team,U eval) {
 unsigned tid=scalar(team,383,0x100);
 if(tid<138){AcquireSRWLockExclusive(&portal29_budget_lock);portal29_budget_stamps[tid].valid=0;ReleaseSRWLockExclusive(&portal29_budget_lock);}
 Portal29FundingClock clock={eval,(U)team,q((U)team+0x10),row(team),test29_clock_ms(),1};
 AcquireSRWLockExclusive(&portal29_clock_lock);
 int index=-1;
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(portal29_clocks[j].eval==eval){index=j;break;}
 if(index<0)for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(!portal29_clocks[j].valid){index=j;break;}
 if(index<0){index=0;for(int j=1;j<PORTAL_FUNDING_EVALUATORS;j++)if(portal29_clocks[j].start<portal29_clocks[index].start)index=j;}
 portal29_clocks[index]=clock;
 ReleaseSRWLockExclusive(&portal29_clock_lock);
}
static void portal29_funding_end(Rec team,U eval) {
 Portal29FundingClock clock={0};
 AcquireSRWLockExclusive(&portal29_clock_lock);
 for(int j=0;j<PORTAL_FUNDING_EVALUATORS;j++)if(portal29_clocks[j].valid && portal29_clocks[j].eval==eval &&
  portal29_clocks[j].team==(U)team && portal29_clocks[j].table==q((U)team+0x10) && portal29_clocks[j].row==row(team)) {
  clock=portal29_clocks[j];portal29_clocks[j].valid=0;break;
 }
 ReleaseSRWLockExclusive(&portal29_clock_lock);
 if(clock.valid)test29_phase_log("PORTAL_NATIVE_FUNDING",scalar(team,383,0x100),eval,clock.start);
}
static void portal29_observe(Rec team,U eval,const char *site) {
#ifdef DRI_EMULATION_TEST
 portal29_test_witness_count=0;
#endif
 if(mode!=1 || faulted || !team || !eval)return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 Rec user=reference(team,407);if(user){release(user);return;}
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 unsigned year=season?scalar(season,2,0x100):0,type=season?scalar(season,5,0x108):0,week=season?scalar(season,4,0x100):0;
 release(season);release(f);if(type!=6 || week<2 || week>6)return;
 PortalCommitPlan plan,now={0};
 AcquireSRWLockExclusive(&commitment_lock);plan=commitment_plans[tid];ReleaseSRWLockExclusive(&commitment_lock);
 int matched=portal_commit_scope(&now,team,eval) && portal_commit_plan_matches(&plan,&now);
 Rec budget=reference(team,224),board=board_array(team);
 unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;
 release(budget);
 U n=board?array_count(board):0;
 if(!board || n>MAX_BOARD){release(board);log_line("PORTAL29_EXECUTION_SKIP,%s,%u,BOARD_BOUNDS",site,tid);return;}
 /* Verify the complete board's productive assignment, including HS actions,
    before attributing any portal funding. Scholarship/scout spend is separate. */
 unsigned productive=0,raw_productive=0,excluded_targets=0;int complete=1;
 /* Keep one callback-local snapshot of board targets and productive action
    allocation. The same read must support both the complete-board budget sum
    and each witness. No records or decisions are cached across callbacks;
    eligibility, current status and firm acceptance are still evaluated below.
    Retained target references keep the snapshot alive until the final cleanup. */
 Rec observed_targets[MAX_BOARD]={0};
 unsigned observed_allocated[MAX_BOARD]={0},observed_mask[MAX_BOARD]={0};
 for(U j=0;j<n;j++) {
  Rec target=observed_targets[j]=array_at(board,j);
  if(!target){complete=0;continue;}
  unsigned raw=0;
  observed_allocated[j]=test31_productive_allocated_detail(target,team,eval,&observed_mask[j],&raw);
  productive+=observed_allocated[j];
  raw_productive+=raw;excluded_targets+=raw>observed_allocated[j];
 }
 int budget_ok=complete && productive<=assigned && assigned<=total;
 unsigned funded_count=0,firm_count=0,conditional_count=0;
 for(U j=0;j<n && !faulted;j++) {
  Rec target=observed_targets[j],recruit=target?reference(target,10):0;
  if(!target || !recruit){release(recruit);continue;}
  if(!transfer_recruit(recruit)){release(recruit);continue;}
  unsigned stage=scalar(recruit,9,0x108),status=scalar(target,12,0x108),target_hours_raw=scalar(target,6,0x100),mask=observed_mask[j];
  int32_t delta=(int32_t)scalar(target,7,0x100);
  unsigned allocated=observed_allocated[j];
  int index=matched?portal_commit_index(&plan,recruit):-1;
  int slot=index>=0?plan.entries[index].slot:0,round=index>=0?plan.entries[index].round:0;
  int offered=status>=2 && status<=4;
  int legal=stage<5 && offered && !portal_decision(recruit,team,eval,"EXECUTION_CHECK");
  int eligible=legal?native_recruit_eligible(eval,team,recruit):0;
  int funded=legal && eligible && budget_ok && allocated>0;
  int firm=funded && index>=0 && round==1 &&
   test29r2_portal_priority_clear(recruit,team,eval,"FIRM_FUNDING_CHECK");
  /* A paid primary can be conditional now: another live opening or stronger
     paid upgrade may own the capacity. Do not certify its old round number. */
  int conditional=funded && index>=0 && !firm;
#ifdef DRI_EMULATION_TEST
  if(portal29_test_witness_count<MAX_BOARD) {
   portal29_test_witness[portal29_test_witness_count].rid=row(recruit);
   portal29_test_witness[portal29_test_witness_count++].flags=(unsigned)(offered | (funded<<1) | (firm<<2) | (conditional<<3) | (budget_ok<<4) | (legal<<5));
  }
#endif
  if(matched && index>=0 && !faulted)t36_paid_witness(team,target,recruit,eval,&now,plan.entries[index].pos,slot,firm,conditional,site);
  if(!strcmp(site,"POST_FUNDING"))t36_recovery_witness(recruit,team,eval,funded,firm);
  funded_count+=funded;firm_count+=firm;conditional_count+=conditional;
  log_line("PORTAL29_FUNDING_WITNESS,%s,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",
   site,year,type,week,tid,row(recruit),stage,offered,target_hours_raw,allocated,mask,delta,slot,round,legal,eligible,funded,firm,conditional,budget_ok,
   strncmp(site,"POST_FUNDING",12)==0?"NATIVE_ALLOCATED_NOT_EXECUTION":"RECORDED_HOURS_DELTA_NOT_CAUSAL_PROOF");
  release(recruit);
 }
 for(U j=0;j<n;j++)release(observed_targets[j]);
 release(board);
 log_line("TEST31_FUNDING_PROVENANCE,%s,%u,%u,%u,%u,%u,%u,%u,RETAINED_ACTION_FLAGS_CURRENT_PASS_EXCLUSIONS",
  site,year,type,week,tid,raw_productive,productive,excluded_targets);
 if(budget_ok && !strcmp(site,"POST_FUNDING"))r2_ordinary_funding(eval,team,year,week,funded_count);
 if(strncmp(site,"POST_FUNDING",12)==0)portal29_budget_store(tid,eval,&now,assigned,total,productive,budget_ok && now.franchise_table!=0 && !faulted);
 log_line("PORTAL29_FUNDING_SUMMARY,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d",
  site,year,type,week,tid,total,assigned,productive,funded_count,firm_count,conditional_count,budget_ok);
}
static void portal29_observe_all(U eval,const char *site) {
 if(mode!=1 || faulted)return;
 for(unsigned tid=0;tid<138 && !faulted;tid++) {
  Rec team=portal_team_by_id(eval,tid);if(team)portal29_observe(team,eval,site);release(team);
 }
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestPortal29Observe(Rec team,U eval){portal29_observe(team,eval,"POST_FUNDING");}
__declspec(dllexport) unsigned DriTestPortal29Witness(unsigned rid){
 for(int i=0;i<portal29_test_witness_count;i++)if(portal29_test_witness[i].rid==rid)return portal29_test_witness[i].flags;
 return 0;
}
#endif
