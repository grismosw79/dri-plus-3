/* Phase-aware read-only acceptance. A verified cut may be disposed by native
   cleanup. Never dereference its retained record wrapper after pool removal. */
static int rm_opening_variance[RM_TEAMS];
static char rm_first_audit_issue[256];
static int rm_audit_error(int pid,int team,const char *check,const char *field,int expected,int actual){
 if(!rm_first_audit_issue[0])snprintf(rm_first_audit_issue,sizeof(rm_first_audit_issue),
    "team=%d %s.%s expected=%d actual=%d",team,check,field,expected,actual);
 char b[640];snprintf(b,sizeof(b),"\"event\":\"RM18R22_INVARIANT\",\"phase\":\"%s\",\"pid\":%d,\"team\":%d,\"check\":\"%s\",\"field\":\"%s\",\"expected\":%d,\"actual\":%d",rm_audit_phase,pid,team,check,field,expected,actual);emit(b);return 1;
}
static int rm_program_facilities(Rec t,int value[2]){
 return scalar(t,82,2,&value[0])&&scalar(t,83,2,&value[1]);
}
static int64_t rm_program_total(const PilotMoney *m,const int f[2]){
 /* Rollover is a separately protected balance, not another charge. The
    native reserved renewal cost is already withheld from remaining funds. */
 return (int64_t)m->value[0]+m->value[2]+m->value[3]+m->value[5]+f[0]+f[1];
}
static void rm_program_log(int t,const char *phase,const PilotMoney *m,const int f[2]){
 char b[800];snprintf(b,sizeof(b),"\"event\":\"RM18R22_PROGRAM_LEDGER\",\"phase\":\"%s\",\"team\":%d,\"NILProgramPointsSpent\":%d,\"ProgramPointBudget\":%d,\"RecruitProgramPointsSpent\":%d,\"RemainingProgramPoints\":%d,\"RolloverProgramPoints\":%d,\"StaffProgramPointsSpent\":%d,\"FacilitiesProgramPointsSpent\":%d,\"FacilitiesRenewalCostReserved\":%d,\"accounted_total\":%lld",phase,t,m->value[0],m->value[1],m->value[2],m->value[3],m->value[4],m->value[5],f[0],f[1],(long long)rm_program_total(m,f));emit(b);
}
static int rm_program_fields(int t,const char *check,const PilotMoney *m,const int f[2]){
 static const char *names[8]={"NILProgramPointsSpent","ProgramPointBudget","RecruitProgramPointsSpent","RemainingProgramPoints","RolloverProgramPoints","StaffProgramPointsSpent","FacilitiesProgramPointsSpent","FacilitiesRenewalCostReserved"};
 static const int maxima[8]={20000,30000,20000,30000,20000,20000,20000,10000};
 int errors=0;
 for(int k=0;k<8;k++){
  int v=k<6?m->value[k]:f[k-6];
  if(v<0||v>maxima[k])errors+=rm_audit_error(0,t,check,names[k],v<0?0:maxima[k],v);
 }
 return errors;
}
/* The recorded remaining balance and the budget less existing obligations are
   independent ceilings for new spending. Never repair or credit a game ledger. */
static int rm_program_open(int t,const PilotMoney *m,const int f[2]){
 rm_opening_variance[t]=0;
 int errors=rm_program_fields(t,"BASELINE_ACCOUNTING",m,f);
 if(errors)return errors;
 int gap=(int)(rm_program_total(m,f)-m->value[1]);
 if(!gap)return 0;
 /* For a pre-existing difference, corroborate the budget from published
    revenue sources before trusting it. Ordinals come from the game name table. */
 static const unsigned indexes[]={17,24,34,223,281};
 int funding=m->value[4];
 for(unsigned i=0;i<sizeof(indexes)/sizeof(indexes[0]);i++){
  int v;
  if(!scalar(rm_teams[t],indexes[i],2,&v))return rm_audit_error(0,t,"OPENING_BUDGET","income_field_readable",1,0);
  if(v<0||v>20000)return rm_audit_error(0,t,"OPENING_BUDGET","income_component_bounds",v<0?0:20000,v);
  funding+=v;
 }
 if(funding!=m->value[1])return rm_audit_error(0,t,"OPENING_BUDGET","funding_sources_equal_budget",m->value[1],funding);
 int backed=m->value[3]-gap;
 if(backed<0)return rm_audit_error(0,t,"OPENING_BUDGET","budget_covers_existing_charges",0,backed);
 int hold=rm_max(0,gap);
 rm_opening_variance[t]=gap;
 rm_state->t[t].budget-=hold;
 log_line("RM18R22 OPENING_BUDGET_VARIANCE team=%d recorded_budget=%d income_sources=%d recorded_cash=%d cash_backed_by_budget=%d opening_difference=%d cash_available_before_enrollment=%d",
          t,m->value[1],funding,m->value[3],backed,gap,rm_state->t[t].budget);
 return 0;
}
static int rm_audit_program(int t,int native_spending){
 PilotMoney m;int f[2],errors=0;
 if(!pilot_money(rm_teams[t],&m)||!rm_program_facilities(rm_teams[t],f))return rm_audit_error(0,t,"LEDGER_READ","readable",1,0);
 rm_program_log(t,rm_audit_phase,&m,f);
 const char *names[6]={"NILProgramPointsSpent","ProgramPointBudget","RecruitProgramPointsSpent","RemainingProgramPoints","RolloverProgramPoints","StaffProgramPointsSpent"};
 for(int k=0;k<6;k++)if((!native_spending||(k!=3&&k!=5))&&m.value[k]!=rm_expected_money[t].value[k])errors+=rm_audit_error(0,t,"FINAL_ACCOUNTING",names[k],rm_expected_money[t].value[k],m.value[k]);
 if(!native_spending)for(int k=0;k<2;k++)if(f[k]!=rm_expected_facilities[t][k])errors+=rm_audit_error(0,t,"FINAL_ACCOUNTING",k?"FacilitiesRenewalCostReserved":"FacilitiesProgramPointsSpent",rm_expected_facilities[t][k],f[k]);
 errors+=rm_program_fields(t,"FINAL_ACCOUNTING",&m,f);
 if(errors)return errors;
 int gap=(int)(rm_program_total(&m,f)-m.value[1]),opening=rm_opening_variance[t];
 if(gap==opening)return 0;
 /* Custom transfers and settlement have already passed an exact audit before
    native staff/facility processing. Only in that later native window can a
    previously observed discrepancy close in full. Partial drift still fails. */
 if(native_spending&&opening&&gap==0){
  log_line("RM18R22 NATIVE_OPENING_VARIANCE_CLOSED team=%d previous=%d current=0",t,opening);
  return 0;
 }
 return rm_audit_error(0,t,"FINAL_ACCOUNTING","program_variance_from_opening",opening,gap);
}
static void rm_program_checkpoint(const char *phase){
 /* Diagnostic boundaries only: native work may be incomplete at this instant.
    The two complete audits retain the acceptance decisions. */
 if(!rm_audit_ready||!rm_state)return;
 for(int t=0;t<RM_TEAMS;t++){PilotMoney m;int f[2];if(pilot_money(rm_teams[t],&m)&&rm_program_facilities(rm_teams[t],f))rm_program_log(t,phase,&m,f);else log_line("RM18R22 PROGRAM_CHECKPOINT_UNREADABLE phase=%s team=%d",phase,t);}
}
static int rm_live_identity(Rec p,int i){U store=0;uint32_t row=0;return identity(p,&store,&row)&&store==rm_refs[i].store&&row==rm_refs[i].row;}
#include "enrollment_accounting.inc"
static int rm_football_field(const RMField *f){
 /* Immediate transactions still compare all 268 scalars. Across the native
    year transition protect earned football state, not year/health/role or
    the cached OverallRating / dynamic ConfidenceRating. Report those changes. */
 unsigned k=f->index;
 return k==0||k==3||(k>=4&&k<=97)||k==142||(k>=178&&k<=183)||
        (k>=196&&k<=200)||k==202||(k>=245&&k<=251)||k==271||
        (strstr(f->name,"Rating")&&k!=138&&k!=191);
}
typedef struct {int records,fields;} RMBookkeepingCounts;
static int rm_audit_football(Rec p,int i,RMBookkeepingCounts *counts){
 PilotPlayerState now;int errors=0,bookkeeping=0;
 if(!rm_snapshot_quiet(p,&now))return rm_audit_error(rm_state->p[i].id,rm_state->p[i].team,"PLAYER_READ","stored_fields",1,0);
 for(unsigned k=0;k<PILOT_SCALARS;k++)if(now.value[k]!=rm_expected_states[i].value[k]){
  if(rm_football_field(&pilot_player_fields[k]))errors+=rm_audit_error(rm_state->p[i].id,rm_state->p[i].team,"FOOTBALL_PRESERVATION",pilot_player_fields[k].name,rm_expected_states[i].value[k],now.value[k]);
  else bookkeeping++;
 }
 if(bookkeeping) {
  if(rm18_deep())
   log_line("RM18R22 NATIVE_BOOKKEEPING phase=%s pid=%d changed_fields=%d; immediate transaction preservation checked separately",rm_audit_phase,rm_state->p[i].id,bookkeeping);
  else {
   counts->records++;
   counts->fields+=bookkeeping;
  }
 }
 return errors;
}
static int rm_audit_prepare(void){
 rm_first_audit_issue[0]=0;
 if(!rm_state||!rm_refs||rm_audit_ready)return 0;
 rm_expected_states=calloc(RM_PLAYERS,sizeof(*rm_expected_states));rm_expected_nil=calloc(RM_PLAYERS,sizeof(*rm_expected_nil));
 if(!rm_expected_states||!rm_expected_nil)return 0;
 int64_t salary[RM_TEAMS]={0},commits[RM_TEAMS]={0};int errors=0;rm_audit_phase="BEFORE_CUSTOM_CHANGES";
 for(int i=0;i<rm_state->n;i++){
  RMPlayer*p=&rm_state->p[i];if(!rm_snapshot_quiet(rm_refs[i].player,&rm_expected_states[i]))return 0;
  rm_expected_nil[i]=pilot_get(rm_refs[i].player,139,2);if(rm_expected_nil[i]<0)return 0;
  if(p->original>=0){if(p->arrival)commits[p->original]+=rm_expected_nil[i];else salary[p->original]+=rm_expected_nil[i];}
 }
 for(int t=0;t<RM_TEAMS;t++){
  if(!pilot_money(rm_teams[t],&rm_expected_money[t])||!rm_program_facilities(rm_teams[t],rm_expected_facilities[t])){errors+=rm_audit_error(0,t,"BASELINE_ACCOUNTING","readable",1,0);continue;}
  rm_program_log(t,rm_audit_phase,&rm_expected_money[t],rm_expected_facilities[t]);
  errors+=rm_program_open(t,&rm_expected_money[t],rm_expected_facilities[t]);
  if(salary[t]>20000||commits[t]>20000){errors+=rm_audit_error(0,t,"BASELINE_ACCOUNTING","contract_totals_within_native_bounds",1,0);continue;}
  if(rm_expected_money[t].value[0]!=salary[t])errors+=rm_audit_error(0,t,"BASELINE_ACCOUNTING","NILSpent",(int)salary[t],rm_expected_money[t].value[0]);
  errors+=rm_enrollment_prepare(t,(int)commits[t]);
 }
 return !errors&&!capture_fault;
}
static int rm_audit_seal(void){
 if(!rm_expected_states||!rm_expected_nil||rm_executed!=rm_state->na)return 0;
 /* Compute from the captured baseline and verified receipts, never accept a
    post-mutation ledger as the expected answer. Completed prefix survives replan. */
 for(int j=0;j<rm_state->na;j++){
  RMAction*a=&rm_state->a[j];int i=a->player,old=rm_expected_nil[i];
  if(a->src>=0){rm_expected_money[a->src].value[0]-=old;rm_expected_money[a->src].value[3]+=old;}
  if(a->phase==RM_FINAL_CUT){if(!rm_cut_verified[j])return 0;rm_expected_nil[i]=0;}
  else{rm_expected_money[a->dst].value[0]+=a->quote;rm_expected_money[a->dst].value[3]-=a->quote;rm_expected_nil[i]=a->quote;}
 }
 for(int t=0;t<RM_TEAMS;t++){PilotMoney actual;int f[2];if(!pilot_money(rm_teams[t],&actual)||!rm_program_facilities(rm_teams[t],f)||memcmp(&actual,&rm_expected_money[t],sizeof(actual))||memcmp(f,rm_expected_facilities[t],sizeof(f)))return 0;rm_program_log(t,"AFTER_CUSTOM_TRANSACTIONS",&actual,f);}
 rm_audit_phase="AFTER_CUSTOM_TRANSACTIONS";
 if(!rm_enrollment_seal())return 0;
 rm_audit_ready=1;return !capture_fault;
}
static int rm_audit_final(void){
 if(!rm_audit_ready||!rm_expected_states||!rm_expected_nil)return !rm_audit_error(0,-1,"BASELINE_MISSING","ready",1,0);
 int errors=0,below=0,above=0,gaps=0,label_gaps=0,disposed=0,retained=0;
 RMBookkeepingCounts bookkeeping={0};
 int pending_fb[RM_TEAMS]={0};
 int pending_kicking[RM_TEAMS][2]={{0}};
 unsigned char seen[RM_PLAYERS]={0},retired[RM_PLAYERS]={0},freeagents[RM_PLAYERS]={0};
 int action[RM_PLAYERS];for(int i=0;i<RM_PLAYERS;i++)action[i]=-1;
 for(int j=0;j<rm_state->na;j++){int i=rm_state->a[j].player;if(i<0||i>=rm_state->n||action[i]>=0){errors+=rm_audit_error(0,-1,"ACTION_IDENTITY","unique_index",1,0);continue;}action[i]=j;}
 for(int t=0;t<RM_TEAMS;t++){
  int ok=0;Rec a=array_field(rm_teams[t],239,&ok);U size=array_count(a);int counts[RM_POS]={0},n=(int)size;
  if(!ok||size>RM_ROOM){release(a);errors+=rm_audit_error(0,t,"ROSTER_ARRAY","readable",1,0);continue;}
  if(n!=rm_state->t[t].count)errors+=rm_audit_error(0,t,"ROSTER_COUNT","count",rm_state->t[t].count,n);
  int active=pilot_get(rm_teams[t],3,2);if(active!=n)errors+=rm_audit_error(0,t,"ACTIVE_ROSTER_SIZE","ActiveRosterSize",n,active);
  for(int j=0;j<n;j++){
   Rec p=array_at(a,j,&ok);if(!ok||!p){release(p);errors+=rm_audit_error(0,t,"ROSTER_ENTRY","nonnull",1,0);continue;}
   int id=pilot_get(p,228,2),i=rm_lookup(id),pos=pilot_get(p,3,3),owner=pilot_get(p,261,2);
   if(pos<0||pos>=RM_POS)errors+=rm_audit_error(id,t,"POSITION","valid",1,0);else counts[pos]++;
   if(owner!=t)errors+=rm_audit_error(id,t,"SCALAR_OWNERSHIP","TeamIndex",t,owner);
   if(i<0||!rm_live_identity(p,i)){errors+=rm_audit_error(id,t,"RECORD_IDENTITY","expected_record",1,0);release(p);continue;}
   if(seen[i]++)errors+=rm_audit_error(id,t,"DUPLICATE_OWNERSHIP","roster_occurrences",1,seen[i]);
   if(rm_state->p[i].team!=t)errors+=rm_audit_error(id,t,"PLAN_OWNERSHIP","team",rm_state->p[i].team,t);
   int nil=pilot_get(p,139,2);if(nil!=rm_expected_nil[i])errors+=rm_audit_error(id,t,"PLAYER_NIL","CurrentNILCompensation",rm_expected_nil[i],nil);
   errors+=rm_audit_football(p,i,&bookkeeping);release(p);
  }release(a);
  a=array_field(rm_teams[t],33,&ok);size=array_count(a);if(!ok||size)errors+=rm_audit_error(0,t,"COMMIT_ENROLLMENT","remaining_commits",0,ok?(int)size:-1);release(a);
  errors+=rm_audit_program(t,rm_end_complete&&rm_encourage_complete&&rm_validation_count==RM_TEAMS);
  if(rm_managed(rm_state,t)){below+=n<85;above+=n>90;}rm_event("FINAL_ROSTER_COUNT",0,t,t,n);
  for(int pos=0;pos<RM_POS;pos++){RMConfig*c=&rm_state->t[t].config[pos];int missing=rm_max(0,c->minimum-counts[pos]);if(rm_managed(rm_state,t))label_gaps+=missing;char b[400];snprintf(b,sizeof(b),"\"event\":\"RM17R3_ROOM\",\"team\":%d,\"position\":%d,\"count\":%d,\"minimum\":%d,\"ideal\":%d,\"missing\":%d",t,pos,counts[pos],c->minimum,c->ideal,missing);emit(b);}
  if(rm_managed(rm_state,t))for(int pos=0;pos<RM_POS;pos++)if(rm_unit(pos)==pos) {
   int count=0,required=rm_cfg(rm_state,t,pos,3);
   for(int j=0;j<RM_POS;j++)if(rm_unit(j)==pos)count+=counts[j];
   if(count>=required)continue;
   if(pos==RM_FB_POS)pending_fb[t]=required-count;
   else if(count==0 && rm_kicking_unit(pos))pending_kicking[t][pos-RM_K_POS]=required;
   else {gaps+=required-count;rm_coverage_detail(rm_audit_phase,t,pos,count,required,NULL,-1);}
  }
 }
 for(int i=0;i<rm_state->n;i++)if(rm_state->p[i].team>=0&&seen[i]!=1)errors+=rm_audit_error(rm_state->p[i].id,rm_state->p[i].team,"MISSING_EXPECTED_PLAYER","roster_occurrences",1,seen[i]);
 for(int pool=0;pool<2;pool++){
  int ok=0,nulls=0;Rec a=array_field(rm_franchise,pool?42:14,&ok);U n=array_count(a);
  if(!ok||n>10000){release(a);errors+=rm_audit_error(0,-1,"POOL_ARRAY","readable",1,0);continue;}
  for(U k=0;k<n;k++){
   Rec p=array_at(a,k,&ok);if(!ok){errors+=rm_audit_error(0,-1,"POOL_ENTRY","readable",1,0);release(p);continue;}if(!p){nulls++;continue;}
   int id=pilot_get(p,228,2),i=rm_lookup(id);if(i>=0){
    unsigned char *list=pool?retired:freeagents;if(list[i]<255)list[i]++;
    if(!rm_live_identity(p,i))errors+=rm_audit_error(id,-1,"POOL_IDENTITY","expected_record",1,0);
    if(list[i]>1)errors+=rm_audit_error(id,-1,"POOL_DUPLICATE",pool?"retired":"free_agent",1,list[i]);
    if(rm_state->p[i].team>=0)errors+=rm_audit_error(id,rm_state->p[i].team,"ACTIVE_POOL_OVERLAP",pool?"retired":"free_agent",0,list[i]);
    int j=action[i];if(j>=0&&rm_state->a[j].phase==RM_FINAL_CUT){
     if(!pool)errors+=rm_audit_error(id,-1,"CUT_FREE_AGENT","free_agent",0,list[i]);
     else if(rm_live_identity(p,i)){
      int owner=pilot_get(p,261,2),nil=pilot_get(p,139,2);
      if(owner!=255)errors+=rm_audit_error(id,-1,"CUT_OWNERSHIP","TeamIndex",255,owner);
      if(nil)errors+=rm_audit_error(id,-1,"CUT_NIL","CurrentNILCompensation",0,nil);
      errors+=rm_audit_football(p,i,&bookkeeping);
     }
    }
   }release(p);
  }release(a);if(nulls)log_line("RM18R22 SPARSE_POOL phase=%s field=%d null_entries=%d",rm_audit_phase,pool?42:14,nulls);
 }
 int primary[RM_ACTIONS]={0},specialty[RM_ACTIONS]={0},rank[RM_ACTIONS]={0};
 for(int t=0;t<RM_TEAMS;t++){
  RMFullbackRoom fb_room;fb_room.count=0;
  int kicking[2]={-1,-1};
  if(pending_fb[t]) {
   rm_fullback_room(rm_state,t,-1,seen,&fb_room);
   int healthy=0;
   for(int j=0;j<fb_room.count;j++) {
    int i=fb_room.member[j].player;
    int status=pilot_get(rm_refs[i].player,155,3);
    if(status<0)errors+=rm_audit_error(rm_state->p[i].id,t,"FB_CHART_HEALTH","readable",1,0);
    if(status==1)fb_room.member[healthy++]=fb_room.member[j];
   }
   fb_room.count=healthy;
  }
  int ok=0;Rec chart=reference(rm_teams[t],63,&ok);if(!ok||!chart){release(chart);gaps+=pending_fb[t]+pending_kicking[t][0]+pending_kicking[t][1];errors+=rm_audit_error(0,t,"DEPTH_CHART","readable",1,0);continue;}
  for(int pos=0;pos<RM_CHARTS;pos++){
   Rec a=array_field(chart,pos,&ok);U n=array_count(a);if(!ok||n>30){release(a);errors+=rm_audit_error(0,t,"CHART_ARRAY","readable",1,0);continue;}
   for(U k=0;k<n;k++){
    Rec p=array_at(a,k,&ok);if(!ok){errors+=rm_audit_error(0,t,"CHART_ENTRY","readable",1,0);release(p);continue;}if(!p)continue;
    int id=pilot_get(p,228,2),i=rm_lookup(id);
    if(i<0||!rm_live_identity(p,i)||rm_state->p[i].team!=t||seen[i]!=1){errors+=rm_audit_error(id,t,"CHART_OWNERSHIP","active_team_member",1,0);release(p);continue;}
    if(k==0 && rm_kicking_unit(pos) && pending_kicking[t][pos-RM_K_POS]) {
     int status=pilot_get(p,155,3);
     if(status<0)errors+=rm_audit_error(id,t,"KICKING_CHART_HEALTH","readable",1,0);
     if(rm_kicking_incumbent(rm_state,t,pos,&rm_state->p[i],status==1) && pilot_get(p,3,3)==rm_state->p[i].pos)
      kicking[pos-RM_K_POS]=i;
    }
    if(fb_room.count && pos<RM_OFFENSE_POSITIONS)
     rm_fullback_chart_entry(&fb_room,i,pos,(int)k+1,rm_state->t[t].config[pos].starters);
    int j=action[i];if(j>=0){RMAction*x=&rm_state->a[j];int relevant=pos<21&&rm_job_unit(pos)==rm_job_unit(rm_state->p[i].pos);
     if(relevant&&(!rank[j]||(int)k+1<rank[j]))rank[j]=(int)k+1;
     if(pos<21&&(int)k<rm_state->t[t].config[pos].starters)primary[j]=1;
     if(pos>=21&&k==0)specialty[j]=1;
     char b[400];snprintf(b,sizeof(b),"\"event\":\"RM17R3_ACTUAL_ROLE\",\"pid\":%d,\"team\":%d,\"chart\":%d,\"rank\":%d,\"promised_role\":%d",id,t,pos,(int)k+1,x->role);emit(b);
    }release(p);
   }release(a);
  }release(chart);
  if(pending_fb[t]) {
   RMFullbackCover cover;int required=rm_state->t[t].config[RM_FB_POS].minimum;
   rm_assign_fullback_cover(rm_state,t,pending_fb[t],&fb_room,&cover);
   gaps+=pending_fb[t]-cover.count;
   rm_coverage_detail(rm_audit_phase,t,RM_FB_POS,required-pending_fb[t],required,&cover,-1);
  }
  for(int k=0;k<2;k++)if(pending_kicking[t][k]) {
   gaps+=pending_kicking[t][k]-(kicking[k]>=0);
   rm_coverage_detail(rm_audit_phase,t,RM_K_POS+k,0,pending_kicking[t][k],NULL,kicking[k]);
  }
 }
 int role_reviews=0;
 for(int j=0;j<rm_state->na;j++){
  RMAction*a=&rm_state->a[j];int i=a->player,id=rm_state->p[i].id;
  if(a->phase==RM_FINAL_CUT){
   if(!rm_cut_verified[j])errors+=rm_audit_error(id,-1,"CUT_RECEIPT","verified_before_cleanup",1,0);
   if(seen[i]||freeagents[i]||retired[i]>1)errors+=rm_audit_error(id,-1,"CUT_LIFECYCLE","inactive_and_not_free_agent",1,0);
   if(retired[i]){retained++;rm_event("FINAL_CUT_RETAINED",id,a->src,-1,retired[i]);}
   else{disposed++;rm_event("FINAL_CUT_DISPOSED_BY_NATIVE_CLEANUP",id,a->src,-1,0);}
  }else{
   int source_depth=rm_state->p[i].chart_depth;
   int review=(a->role==RM_STARTER&&!primary[j])||(a->role==RM_ROTATION&&source_depth>0&&(!rank[j]||rank[j]>=source_depth));
   role_reviews+=review;
   log_line("RM18R22 ROLE_REVIEW phase=%s pid=%d destination=%d projected_role=%d primary_starter=%d specialty_first=%d source_depth=%d destination_depth=%d review=%d; projections do not force native chart selections",rm_audit_phase,id,a->dst,a->role,primary[j],specialty[j],source_depth,rank[j],review);
  }
 }
 if(above)errors+=rm_audit_error(0,-1,"FINAL_ROSTER_CEILING","over90",0,above);
 if(!rm18_deep())
  log_line("RM18R22 BOOKKEEPING_TOTAL phase=%s records=%d changed_fields=%d",rm_audit_phase,bookkeeping.records,bookkeeping.fields);
 log_line("RM18R22 FINAL_RUNTIME_CHECK phase=%s transactions=%d errors=%d under85_exceptions=%d over90=%d compatible_unit_minimum_gaps=%d listed_minimum_gaps=%d cuts_retained=%d cuts_disposed=%d role_reviews=%d; %s; actual-role and saved-state acceptance remain separate",rm_audit_phase,rm_executed,errors,below,above,gaps,label_gaps,retained,disposed,role_reviews,errors?"INTEGRITY_FAILED":gaps?"ROSTER_INTEGRITY_PASS_WITH_POSITION_SHORTAGES":below?"90_CEILING_COVERAGE_PASS_WITH_DOCUMENTED_FLOOR_EXCEPTIONS":"85_90_COUNTS_AND_COMPATIBLE_MINIMUM_COVERAGE_PASS");
 return !errors&&!capture_fault;
}
