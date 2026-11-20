/* EPS-1.0 integration and shared Test28 dealbreaker dispatch. */
#include "eps_collect.h"
#include "eps_snapshot.h"
#include <stdlib.h>
#include <math.h>

U ResumeEpsGrade,ResumeEpsGeneric,ResumeEpsRequired,ResumeEpsUpdate;
extern void HookEpsGrade(void),HookEpsGeneric(void),HookEpsRequired(void),HookEpsUpdate(void);
extern int NativeEpsGrade(U,Rec *,int,Rec *);
extern int NativeEpsGeneric(U,Rec *,int,int);
extern int NativeEpsRequired(U,Rec *,Rec *,Rec *);
extern void NativeEpsUpdate(U);
static SRWLOCK eps_lock=SRWLOCK_INIT;
typedef struct {U table;uint32_t row;} EpsRecordKey;
typedef struct {
 EpsRecordKey dynasty,schools[EPS_TEAMS];uint32_t league;
 int year,index,stage,week,type,valid,final;
 uint64_t signature;
 EpsOutput output,previous;
 int previous_valid,unmapped_logged;
} EpsState;
static EpsState eps_state;
typedef struct {const char *caller,*phase;int team,profile;} EpsTrace;
static EpsRecordKey eps_record_key(Rec record){
 EpsRecordKey key={q((U)record+0x10),row(record)};
 return key;
}
static int eps_keys_equal(EpsRecordKey a,EpsRecordKey b){
 return a.table==b.table && a.row==b.row;
}
/* Preserve the fail-closed exception boundary, but retain the fault address.
   An EPS_grade code alone cannot distinguish an invalid context from a native
   integer division. This is diagnostic evidence, not a practice-mode fix. */
static int eps_exception(const char *where,EXCEPTION_POINTERS *p){
 EXCEPTION_RECORD *r=p?p->ExceptionRecord:0;
 U address=r?(U)r->ExceptionAddress:0;
 log_line("EPS_EXCEPTION,%s,0x%08lx,0x%llx,game_rva_0x%llx",where,
  r?r->ExceptionCode:0,(unsigned long long)address,
  (unsigned long long)(address>=game_base && address-game_base<EXPECTED_IMAGE_SIZE?address-game_base:0));
 return fail_exception(where,r?r->ExceptionCode:0);
}
static int eps_grade_exception(const EpsTrace *trace,EXCEPTION_POINTERS *p){
 log_line("EPS_GRADE_CONTEXT,%s,%s,team=%d,profile=%d,last_year=%d,last_type=%d,last_week=%d",
  trace->caller,trace->phase,trace->team,trace->profile,eps_state.year,eps_state.type,eps_state.week);
 return eps_exception("EPS_grade",p);
}
static EpsRec eps_ref(EpsRec r,int n){return (EpsRec)reference((Rec)r,n);}
static EpsRec eps_array(EpsRec r,int n){return (EpsRec)array_field((Rec)r,n);}
static EpsRec eps_at(EpsRec r,unsigned n){return (EpsRec)array_at((Rec)r,n);}
static unsigned eps_count(EpsRec r){return (unsigned)array_count((Rec)r);}
static int32_t eps_value(EpsRec r,int n,int method){return (int32_t)scalar((Rec)r,n,method);}
static uint64_t eps_key(EpsRec r){return row((Rec)r);}
static int eps_same(EpsRec a,EpsRec b){return a&&b&&q(a+0x10)==q(b+0x10)&&row((Rec)a)==row((Rec)b);}
static void eps_keep(EpsRec r){if(r)((void(*)(Rec))vfunc((Rec)r,0))((Rec)r);}
static void eps_drop(EpsRec r){release((Rec)r);}
/* Native field resolver 08051ff0 checks this signed schema field count. */
static int eps_stat_fields(EpsRec r){return *(int16_t*)(schema((Rec)r)+0xd8);}
static const EpsAPI eps_api={eps_ref,eps_array,eps_at,eps_count,eps_value,eps_key,eps_same,eps_keep,eps_drop,eps_stat_fields};
static int eps_path(wchar_t *path,size_t cap,unsigned league,int year,uint64_t signature,int final){
 DWORD n=GetModuleFileNameW(self_module,path,(DWORD)(cap-160));if(!n||n>=cap-160)return 0;
 while(n&&path[n-1]!=L'\\')n--;
 if(swprintf(path+n,cap-n,L"EPS1-Cache")<0)return 0;
 if(!CreateDirectoryW(path,0)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
 n+=(DWORD)wcslen(L"EPS1-Cache");
 int k=final?swprintf(path+n,cap-n,L"\\%08x-%d-final.bin",league,year):
  swprintf(path+n,cap-n,L"\\%08x-%d-%016llx.bin",league,year,(unsigned long long)signature);
 return k>0&&(size_t)k<cap-n;
}
static int eps_disk_read(EpsDisk *d,unsigned league,int year,int index,uint64_t signature,int final){
 wchar_t path[1024];if(!eps_path(path,1024,league,year,signature,final))return 0;
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
 if(h==INVALID_HANDLE_VALUE)return 0;
 LARGE_INTEGER size;DWORD got=0;int ok=GetFileSizeEx(h,&size)&&size.QuadPart==sizeof(*d)&&ReadFile(h,d,sizeof(*d),&got,0)&&got==sizeof(*d);
 CloseHandle(h);return ok&&eps_disk_valid(d,league,year,index,signature,final);
}
static int eps_disk_write(const EpsDisk *d,int final){
 wchar_t path[1024],temp[1100];if(!eps_path(path,1024,d->league,(int)d->year,d->signature,final))return 0;
 if(swprintf(temp,1100,L"%ls.%lu.tmp",path,(unsigned long)GetCurrentProcessId())<0)return 0;
 HANDLE h=CreateFileW(temp,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
 if(h==INVALID_HANDLE_VALUE)return 0;DWORD put=0;
 int ok=WriteFile(h,d,sizeof(*d),&put,0)&&put==sizeof(*d)&&FlushFileBuffers(h);CloseHandle(h);
 if(ok)ok=MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok)DeleteFileW(temp);return ok;
}
/* The native overview is keyed only by archetype. Its two shared K/P rows
   show kicker profiles. Player-specific calls use actual Position, including P. */
static int eps_index_school(Rec team,EpsRecordKey *schools,unsigned char *seen){
 int tid=team?(int)scalar(team,383,0x100):-1;
 if(tid<0||tid>=EPS_TEAMS||seen[tid])return -1;
 seen[tid]=1;
 if(schools)schools[tid]=eps_record_key(team);
 return tid;
}
static uint64_t eps_cache(Rec franchise,const EpsOutput *out,int publish,EpsRecordKey *schools){
 Rec teams=array_field(franchise,45);if(!teams||array_count(teams)!=138){release(teams);return 0;}
 uint64_t hash=UINT64_C(1469598103934665603);int ok=1;
 unsigned char seen[EPS_TEAMS]={0};
 for(unsigned i=0;i<138;i++){
  Rec team=array_at(teams,i);int tid=eps_index_school(team,schools,seen);
  Rec tracking=team?reference(team,165):0;
  if(!tracking||tid<0){release(tracking);release(team);ok=0;break;}
  Rec ranks=reference(tracking,21);if(!ranks){release(tracking);release(team);ok=0;break;}
  Wrap w={game_base+0x0bd4db18,tracking,schema(tracking),0};
  Wrap rw={game_base+0x0bdea1d8,ranks,schema(ranks),0};
  for(int a=0;a<63;a++){
   if(publish){const EpsGrade *g=&out->grade[tid][eps_profile(a,19)];
    ((void(*)(Wrap*,int,int))(game_base+0x08610de0))(&w,a,g->status?g->grade:13);
    ((void(*)(Wrap*,int,int))(game_base+0x08610ed0))(&w,a,g->status?(int)floor(g->rank-1):0);
   }
   int value=((int(*)(Wrap*,int))(game_base+0x0860f580))(&w,a);
   hash^=(uint64_t)(tid*64+a)*17+(unsigned)value;hash*=UINT64_C(1099511628211);
   int rank=((int(*)(Wrap*,int))(game_base+0x08597d80))(&rw,a);
   hash^=(unsigned)rank;hash*=UINT64_C(1099511628211);
  }
  release(ranks);release(tracking);release(team);
 }release(teams);return ok?hash:0;
}
/* Week 8 uses completed weeks 0..7, Week 16 uses 0..15. Postseason
   retains the Week-16 grade until completed-season publication. Preseason
   carries a validated prior final, or stays N/A in the first season. */
static int eps_cutoff(int stage,int type,int week){
 return stage==2?-1:type==0?0:type==1?(week/8)*8:16;
}
static void eps_reset(void){memset(&eps_state,0,sizeof(eps_state));}
/* Only scalar keys and grades survive a callback. Missing dynasty/season during
   menu initialization defers to native until the next valid request. */
static int eps_ensure(U context,int force,int publish,EpsTrace *trace){
 trace->phase="current_dynasty";
 Rec franchise=context?context_ref(context,2):0;
 if(!franchise){eps_state.valid=0;return 0;}
 trace->phase="current_season";
 Rec season=reference(franchise,44);
 if(!season){eps_state.valid=0;release(franchise);return 0;}
 if(!scalar(season,24,0xf8)){eps_state.valid=0;release(season);release(franchise);return 0;}
 int year=(int)scalar(season,2,0x100),index=(int)scalar(season,6,0x100),stage=(int)scalar(season,3,0x108),week=(int)scalar(season,4,0x100),type=(int)scalar(season,5,0x108);
 unsigned league=scalar(franchise,28,0x100);release(season);
 if(!league||year<1900||year>2200||index<0||index>100||stage<0||stage>8||week<0||week>60){release(franchise);portal_fault("EPS_scope");return 0;}
 /* Test30: a grade query can be the first refresh at PRE/W1, W8 or W16,
    without NativeEpsUpdate. Publish every verified refresh (including N/A
    and a prior-final carry) so saved tables agree with recruiting queries.
    The same-scope fast return below prevents rewriting on each query; the
    held-cutoff path republishes without consuming any new completed games.
    Dynasty enable/scope checks above still precede every publication. */
 publish=1;
 trace->phase="dynasty_identity";
 EpsRecordKey current=eps_record_key(franchise);
 int same=eps_state.valid&&eps_keys_equal(current,eps_state.dynasty)&&eps_state.league==league;
 if(!force&&same&&eps_state.year==year&&eps_state.index==index&&eps_state.stage==stage&&eps_state.week==week&&eps_state.type==type){release(franchise);return 1;}
 if(same && stage!=2 && eps_state.stage==stage && eps_state.year==year && eps_state.index==index &&
    eps_cutoff(stage,type,week)==eps_cutoff(eps_state.stage,eps_state.type,eps_state.week)){
  /* NativeEpsUpdate still runs. Republish the held grades after it, without
     fitting newly accumulated stats between the scheduled refreshes. */
  if(publish) {
   trace->phase="held_publication";
   uint64_t held_hash=eps_cache(franchise,&eps_state.output,1,eps_state.schools);
   if(!held_hash){eps_state.valid=0;release(franchise);portal_fault("EPS_held_publish");return 0;}
   log_line("EPS_PUBLICATION,%u,%d,%d,%d,%d,%016llx,HELD_READBACK",league,year,type,week,
    eps_cutoff(stage,type,week),(unsigned long long)held_hash);
  }
  eps_state.week=week;eps_state.type=type;eps_state.unmapped_logged=0;release(franchise);return 1;
 }
 if(!same)eps_reset();
 EpsDisk *disk=calloc(1,sizeof(*disk));EpsRow *rows=calloc(EPS_MAX_ROWS,sizeof(*rows));EpsOutput *next=calloc(1,sizeof(*next));
 if(!disk||!rows||!next){free(disk);free(rows);free(next);release(franchise);portal_fault("EPS_allocation");return 0;}
 int prior=0,ok=0,loaded=0;const char *error=0;
 if(same&&eps_state.year==year-1&&eps_state.final){eps_state.previous=eps_state.output;eps_state.previous_valid=1;prior=1;}
 else if(same&&eps_state.year==year&&eps_state.previous_valid)prior=1;
 else {
  eps_state.previous_valid=0;
  trace->phase="prior_publication";
  if(index>0&&eps_disk_read(disk,league,year-1,index-1,0,1)&&disk->cache_hash&&disk->cache_hash==eps_cache(franchise,0,0,0)){
   eps_state.previous=disk->output;eps_state.previous_valid=1;prior=1;
  }
 }
 trace->phase="collect";
 EpsReadResult result=eps_collect_through(&eps_api,(EpsRec)franchise,index,rows,eps_cutoff(stage,type,week));
 /* Finished-season grades are immutable across departures and position moves.
    Exact completed-game identity is required to reuse a saved snapshot. */
 if(result.success&&result.schedule_hash&&eps_disk_read(disk,league,year,index,result.schedule_hash,0)){
  *next=disk->output;loaded=1;ok=1;
 }else if(result.success){ok=eps_compute(rows,result.rows,result.teams,year,prior?&eps_state.previous:0,next);if(!ok)error="model";}
 else error=result.error;
 /* A signature is set only after complete traversal. Source differences are
    diagnostics, not refresh/recruiting failures. Invalid values retain their
    separate hard failure; never label them as tolerated source differences. */
 if(result.schedule_hash)for(int i=0;i<result.rows;i++){
  EpsRow *x=&rows[i];int reconciled=eps_reconcile_row(x);if(reconciled==1)continue;
  double *g=x->group[EPS_G_ALL];
  log_line("EPS_TEAM_RESIDUAL,%d,%d,%d,%d,%d,%s,pass_yards,%.0f,pass_attempts,%.0f,rush_yards,%.0f,rush_attempts,%.0f,sack_halves,%.0f",
   year,week,x->game,x->team,x->opponent,!eps_row_valid(x)?"INVALID_VALUES":loaded?"FROZEN_SOURCE_DIFFERENCE":"SOURCE_DIFFERENCE",
   x->pass_yards-g[EPS_S_PASSYARDS],x->pass_attempts-g[EPS_S_PASSATTEMPTS],
   x->rush_yards-g[EPS_S_RUSHYARDS],x->rush_attempts-g[EPS_S_RUSHATTEMPTS],
   2*x->sacks-2*g[EPS_S_DLINESACKS]-g[EPS_S_DLINEHALFSACK]);
 }
 if(ok){
  eps_state.dynasty=current;eps_state.unmapped_logged=0;
  eps_state.league=league;eps_state.year=year;eps_state.index=index;eps_state.stage=stage;eps_state.week=week;eps_state.type=type;
  eps_state.signature=result.schedule_hash;eps_state.output=*next;eps_state.valid=1;eps_state.final=stage==2&&next->rows>0;
  trace->phase="publish";
  uint64_t cache_hash=publish?eps_cache(franchise,next,1,eps_state.schools):0;
  if(publish&&!cache_hash){ok=0;error="cache_publish";}
  if(ok){
   memset(disk,0,sizeof(*disk));disk->magic=EPS_DISK_MAGIC;disk->version=EPS_DISK_VERSION;disk->bytes=sizeof(EpsOutput);
   disk->league=league;disk->year=(unsigned)year;disk->index=(unsigned)index;disk->signature=result.schedule_hash;disk->final=eps_state.final;
   disk->cache_hash=cache_hash;disk->output=*next;disk->output_hash=eps_hash(next,sizeof(*next));
   if(!eps_disk_write(disk,0)||(publish&&disk->final&&!eps_disk_write(disk,1))){ok=0;error="snapshot_write";}
  }
  if(ok){
   if(publish&&disk->final)log_line("EPS_FINAL_PUBLISHED,%u,%d,%d,%016llx,%016llx",league,year,result.rows/2,(unsigned long long)result.schedule_hash,(unsigned long long)cache_hash);
   int status[3]={0};for(int t=0;t<138;t++)for(int p=0;p<EPS_PROFILES;p++)status[next->grade[t][p].status]++;
   log_line("EPS_READY,%u,%d,%d,%d,%d,%d,%d,%d,%016llx,%s",league,year,week,result.rows/2,result.player_games,status[1],status[2],status[0],(unsigned long long)result.schedule_hash,loaded?"FROZEN_SNAPSHOT":"COMPUTED");
   log_line("EPS_CADENCE,%d,%d,%d,%d,EIGHT_WEEK_REFRESH",year,type,week,eps_cutoff(stage,type,week));
   if(publish)log_line("EPS_PUBLICATION,%u,%d,%d,%d,%d,%016llx,REFRESH_READBACK",league,year,type,week,
    eps_cutoff(stage,type,week),(unsigned long long)cache_hash);
   log_line("EPS_OPPONENT_PRIORS,%d,%.0f,%.0f,%.0f,%.0f",year,next->selected_prior_games[0],next->selected_prior_games[1],next->selected_prior_games[2],next->selected_prior_games[3]);
   for(int t=0;t<138;t++){
    for(int p=0;p<EPS_PROFILES;p++){EpsGrade *g=&next->grade[t][p];log_line("EPS_GRADE,%d,%d,%d,%.8f,%.8f,%d,%d",year,t,p,g->score,g->rank,g->status?g->grade:13,g->status);}
    for(int m=0;m<EPS_METRICS;m++){EpsMetric *v=&next->metric[t][m];log_line("EPS_METRIC,%d,%d,%d,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f",year,t,m,v->value,v->observed,v->numerator,v->denominator,v->prior_exposure,v->prior_value);}
   }
  }
 }
 free(next);free(rows);free(disk);release(franchise);
 if(!ok){eps_state.valid=0;portal_fault(error?error:"EPS_read");}return ok;
}
/* Look up the call's record identity in the verified 138-school publication.
   During normal lookups the previous school is an inexpensive collision-free
   hint. A different/unpublished record defers to native for this call. */
static int eps_school_id(EpsRecordKey key){
 static int last_school;
 if(!key.table)return -1;
 if(eps_keys_equal(key,eps_state.schools[last_school]))return last_school;
 for(int i=0;i<EPS_TEAMS;i++)if(eps_keys_equal(key,eps_state.schools[i])){
  last_school=i;return i;
 }
 return -1;
}
/* The game can recreate school records without advancing the calendar. Rebind
   from the active dynasty's complete list. Commit only a complete index that
   includes the requested school. This path performs no grade or save writes. */
static int eps_rebind_school(U context,EpsRecordKey wanted){
 Rec franchise=context_ref(context,2);
 if(!franchise)return -1;
 int matches=eps_keys_equal(eps_record_key(franchise),eps_state.dynasty)&&
             scalar(franchise,28,0x100)==eps_state.league;
 Rec teams=matches?array_field(franchise,45):0;
 release(franchise);
 if(!teams)return -1;
 EpsRecordKey current[EPS_TEAMS];unsigned char seen[EPS_TEAMS]={0};
 int found=-1,complete=array_count(teams)==EPS_TEAMS;
 for(unsigned i=0;complete&&i<EPS_TEAMS;i++){
  Rec school=array_at(teams,i);int tid=eps_index_school(school,current,seen);release(school);
  if(tid<0)complete=0;
  else if(eps_keys_equal(current[tid],wanted))found=tid;
 }
 release(teams);
 if(!complete||found<0)return -1;
 memcpy(eps_state.schools,current,sizeof(current));
 log_line("EPS_SCHOOLS_REBOUND,%u,%d,%d,%d",eps_state.league,eps_state.year,eps_state.type,eps_state.week);
 return found;
}
static int eps_read_grade(U context,Rec team,int archetype,int pos,int *grade,int *status,const char *caller){
 if(!context||!team)return 0;
 int profile=eps_profile(archetype,pos);if(profile<0)return 0;
 EpsTrace trace={caller,"current_dynasty",-1,profile};
 int ok=0;AcquireSRWLockExclusive(&eps_lock);
 __try {
  if(eps_ensure(context,0,0,&trace)){
   trace.phase="published_school";EpsRecordKey school=eps_record_key(team);
   int tid=eps_school_id(school);
   if(tid<0&&school.table){
    trace.phase="current_schools";tid=eps_rebind_school(context,school);
   }
   trace.team=tid;
   if(tid>=0){
    EpsGrade g=eps_state.output.grade[tid][profile];*grade=g.status?g.grade:13;*status=g.status;ok=1;
   }else if(!eps_state.unmapped_logged){
    eps_state.unmapped_logged=1;
    log_line("EPS_GRADE_DEFER,%s,unpublished_school,%d,%d,%d",caller,eps_state.year,eps_state.type,eps_state.week);
   }
  }
 } __except(eps_grade_exception(&trace,GetExceptionInformation())){eps_state.valid=0;ok=0;}
 ReleaseSRWLockExclusive(&eps_lock);return ok&&!faulted&&mode==1;
}
int DriEpsGrade(U context,Rec *team,int kind,Rec *player){
 if(mode==1&&!faulted&&team&&*team&&player&&*player){
  __try {
   int grade;
   if(kind==11&&staff28_grade(context,*team,*player,&grade))return grade;
   if(kind==1&&proximity28_grade(context,*team,*player,&grade))return grade;
  } __except(fail_exception("DB28_player_grade",GetExceptionCode())){}
 }
 if(mode==1&&!faulted&&kind==0&&team&&*team&&player&&*player){
  __try {int grade,status;if(eps_read_grade(context,*team,(int)scalar(*player,202,0x108),(int)scalar(*player,3,0x108),&grade,&status,"player"))return grade;}
  __except(eps_exception("EPS_player_grade",GetExceptionInformation())){}
 }
 return NativeEpsGrade(context,team,kind,player);
}
int DriEpsGeneric(U context,Rec *team,int kind,int archetype){
 if(mode==1&&!faulted&&kind==0&&team&&*team){int grade,status;if(eps_read_grade(context,*team,archetype,19,&grade,&status,"overview"))return grade;}
 return NativeEpsGeneric(context,team,kind,archetype);
}
static int eps_native_threshold(Rec info,int field_id,int value){
 Rec curve=reference(info,field_id);if(!curve){portal_fault("EPS_requirement_curve");return -1;}
 Wrap w={game_base+0x0bd2ee10,curve,schema(curve),0};int grade=((int(*)(Wrap*,int))(game_base+0x0860ddd0))(&w,value);release(curve);return grade;
}
static int t36_base_required(U eval,Rec *info,Rec *person,Rec *team){
 int native=NativeEpsRequired(eval,info,person,team),answer=native;
 if(mode!=1||faulted||!info||!*info||!person||!*person||!team||!*team)return native;
 __try {
  if(db28_required(eval,*info,*person,*team,native,&answer))return answer;
  if(scalar(*info,3,0x108)==8){
   int is_player=((unsigned char(*)(U,U))(game_base+0x08059c60))(schema(*person),q(eval+0x60));
   Rec player=is_player?*person:reference(*person,5);
   if(player){
    int grade,status,pos=(int)scalar(player,3,0x108),arch=(int)scalar(player,202,0x108);
    /* RecruitingEval's existing MySchoolGradeEval cache is used by the native
       motivation-to-grade evaluator at 084333c1 and 084334xx. */
    if(eps_read_grade(q(eval+0xc8),*team,arch,pos,&grade,&status,"requirement")){
     if(!status)answer=13; /* N/A versus N/A: no failure, neutral PLV grade gap. */
     else if(is_player){
      int ovr=(int)scalar(player,191,0x100),age=(int)scalar(player,242,0x108);
      if(ovr<0||ovr>99||age<0||age>3)portal_fault("EPS_player_quality");
      else {
       int before=eps_native_threshold(*info,4,ovr),after=eps_native_threshold(*info,6,dri_portal_star(ovr,age));
       if(before>=0&&after>=0&&before<=12&&after<=12)answer=native-before+after;
       else portal_fault("EPS_requirement_value");
      }
     }
    }
    if(!is_player)release(player);
   }
  }
 } __except(eps_exception("EPS_requirement",GetExceptionInformation())){answer=native;}
 return !faulted&&mode==1?answer:native;
}
int DriEpsRequired(U eval,Rec *info,Rec *person,Rec *team){
 int grade=t36_base_required(eval,info,person,team);
 if(mode==1 && !faulted && rc4.rec_table && info && *info && person && *person && team && *team){
  __try {Rec subject=rc4_evaluating?rc4_evaluating:*person;
         grade=rc4_grade(subject,*person,*info,grade);}
  __except(eps_exception("RC4_portal_grade",GetExceptionInformation())){}
 }
 if(t36_relation && t36_relation_team && person && team && info && *info && *person && *team &&
    portal_same_record(*team,t36_relation_team) && scalar(*info,3,0x108)<=13 && grade>=0 && grade<12){
  Rec player=reference(t36_relation,5);
  int related=portal_same_record(*person,t36_relation) || portal_same_record(*person,player);
  release(player);if(related)return grade+1;
 }
 return grade;
}
void DriEpsUpdate(U context){
 pt28_invalidate();
 pp28_refresh_invalidate();
 /* Capture a verified prior-season cache before the native updater changes it. */
 if(mode==1&&!faulted){AcquireSRWLockExclusive(&eps_lock);
  EpsTrace trace={"update","current_dynasty",-1,-1};
  __try {eps_ensure(context,0,0,&trace);}__except(eps_exception("EPS_before_update",GetExceptionInformation())){}
  ReleaseSRWLockExclusive(&eps_lock);
 }
 NativeEpsUpdate(context);
 pt28_invalidate();
 if(mode==1&&!faulted){AcquireSRWLockExclusive(&eps_lock);
  EpsTrace trace={"update","current_dynasty",-1,-1};
  __try {eps_ensure(context,1,1,&trace);}__except(eps_exception("EPS_publish",GetExceptionInformation())){}
  ReleaseSRWLockExclusive(&eps_lock);
 }
}
