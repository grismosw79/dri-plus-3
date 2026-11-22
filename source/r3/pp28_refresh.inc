/* Test28R1: first-preseason publication when the native creation path reuses
   cached grades. Reuse the native scorer, but skip exactly its history-aging
   call. Never use this path after the first preseason checkpoint. */
U ResumePPGroupGrade;
extern void HookPPGroupGrade(void);
extern int NativePPGroupGrade(U context,Rec *team,int kind,int group,Rec *player);
static SRWLOCK pp28_refresh_lock=SRWLOCK_INIT;
static const unsigned pp28_grade_fields[10]={30,31,33,32,28,25,27,24,26,29};
typedef struct {
    U context;
    Rec history,tracking[138],picks[PP28_MAX_HISTORY];
    uint32_t ages[PP28_MAX_HISTORY];
    unsigned count,held,skipped,published;
    unsigned char seen[138][10];
    int expected[138][10];
} PP28Refresh;
static PP28Refresh *pp28_refresh_work;
static volatile LONG pp28_refresh_owner;
/* A native SDF wrapper can outlive its backing dynasty store. Never keep a
   wrapper to compare/release on the next menu, practice or save-load callback.
   These addresses are identity VALUES only; old addresses are not read. */
typedef struct {U table;uint32_t row;} PP28RecordKey;
typedef struct {
    U context,tuning;
    PP28RecordKey evaluator,franchise,season,teams;
    unsigned league,year;
    LONG epoch;
} PP28RefreshKey;
static PP28RefreshKey pp28_refresh_done;
static int pp28_refresh_done_valid;
static volatile LONG pp28_refresh_epoch=1;
typedef struct {const char *phase;unsigned league,year,published;} PP28RefreshTrace;
/* The native school-grade update is an observed refresh boundary, even when
   an allocator reuses all the addresses. No lock or record access here:
   grade notifications can reenter synchronously during the native scorer. */
static void pp28_refresh_invalidate(void){InterlockedIncrement(&pp28_refresh_epoch);}
static PP28RecordKey pp28_record_key(Rec r){
    PP28RecordKey key={q((U)r+0x10),row(r)};return key;
}
static int pp28_key_equal(PP28RecordKey a,PP28RecordKey b){return a.table==b.table&&a.row==b.row;}
static PP28RefreshKey pp28_refresh_key(U context,Rec franchise,Rec season,Rec teams,unsigned league,unsigned year){
    PP28RefreshKey key={0};key.context=context;key.tuning=q(context+0x88);
    key.evaluator=pp28_record_key((Rec)context);key.franchise=pp28_record_key(franchise);
    key.season=pp28_record_key(season);key.teams=pp28_record_key(teams);
    key.league=league;key.year=year;key.epoch=InterlockedCompareExchange(&pp28_refresh_epoch,0,0);return key;
}
static int pp28_scope_equal(PP28RefreshKey a,PP28RefreshKey b){
    return a.context==b.context&&a.tuning==b.tuning&&a.league==b.league&&a.year==b.year&&a.epoch==b.epoch&&
        pp28_key_equal(a.evaluator,b.evaluator)&&pp28_key_equal(a.franchise,b.franchise)&&
        pp28_key_equal(a.season,b.season)&&pp28_key_equal(a.teams,b.teams);
}
static int pp28_refresh_exception(PP28RefreshTrace *trace,EXCEPTION_POINTERS *p){
    EXCEPTION_RECORD *e=p?p->ExceptionRecord:0;U at=e?(U)e->ExceptionAddress:0;
    if(pp28_refresh_work)trace->published=pp28_refresh_work->published;
    log_line("PP28_REFRESH_EXCEPTION,%s,league=%u,year=%u,published=%u,code=0x%08lx,address=0x%llx,game_rva=0x%llx,access=%llu,target=0x%llx",
        trace->phase,trace->league,trace->year,trace->published,e?e->ExceptionCode:0,
        (unsigned long long)at,(unsigned long long)(at>=game_base&&at-game_base<EXPECTED_IMAGE_SIZE?at-game_base:0),
        (unsigned long long)(e&&e->NumberParameters>0?e->ExceptionInformation[0]:0),
        (unsigned long long)(e&&e->NumberParameters>1?e->ExceptionInformation[1]:0));
    return fail_exception("PP28_refresh",e?e->ExceptionCode:0);
}
/* Cleanup faults still disable the runtime, but cannot bypass the outer lock
   release or cause a second release attempt of the same wrapper. */
static void pp28_refresh_drop(Rec *slot,PP28RefreshTrace *trace,const char *phase){
    Rec current=*slot;*slot=0;if(!current)return;trace->phase=phase;
    __try {release(current);}
    __except(pp28_refresh_exception(trace,GetExceptionInformation())){}
}
static int pp28_same(Rec a,Rec b){return a&&b&&q((U)a+0x10)==q((U)b+0x10)&&row(a)==row(b);}
static int pp28_refresh_owned(U context){
    return pp28_refresh_owner==(LONG)GetCurrentThreadId()&&pp28_refresh_work&&pp28_refresh_work->context==context;
}
static int pp28_refresh_skip_age(U context,Rec array){
    if(!pp28_refresh_owned(context))return 0;
    PP28Refresh *w=pp28_refresh_work;
    w->skipped++;
    if(!pp28_same(w->history,array)||w->skipped!=1)portal_fault("PP28_refresh_age_scope");
    return 1;
}
static void pp28_refresh_publication(U context,unsigned team,unsigned group,int grade){
    if(!pp28_refresh_owned(context))return;
    PP28Refresh *w=pp28_refresh_work;
    if(team>=138||group>=10||w->seen[team][group]||grade<0||grade>7){
        portal_fault("PP28_refresh_publication");return;
    }
    w->seen[team][group]=1;w->expected[team][group]=grade;w->published++;
}
static int pp28_refresh_inner(U context){
    Rec franchise=0,season=0;PP28Refresh *w=0;
    unsigned league=0,year=0;int okay=0,history_locked=0,completed=0;const char *why=0;
    PP28RefreshKey next={0};PP28RefreshTrace trace={"current_franchise",0,0,0};
    __try {
        franchise=context_ref(context,2);
        if(!franchise){pp28_refresh_done_valid=0;__leave;}
        trace.phase="current_calendar";
        season=reference(franchise,44);
        if(!season||!scalar(season,24,0xf8)){pp28_refresh_done_valid=0;__leave;}
        /* Proven native SeasonStage enum / CurrentYear / week accessors. */
        if(scalar(season,3,0x108)!=0||scalar(season,6,0x100)!=0||scalar(season,4,0x100)!=0){pp28_refresh_done_valid=0;__leave;}
        unsigned base=scalar(season,0,0x100);year=scalar(season,2,0x100);
        if(base<1900||base>2200||base!=year){why="PP28_refresh_year";__leave;}
        league=scalar(franchise,28,0x100);
        trace.league=league;trace.year=year;trace.phase="current_context";
        Rec teams=(Rec)q(context+0x80);
        if(!league||!teams||array_count(teams)!=138||!q(context+0x88)||
           *(unsigned char*)(context+0x10e8)!=0){why="PP28_refresh_context";__leave;}
        next=pp28_refresh_key(context,franchise,season,teams,league,year);
        if(pp28_refresh_done_valid&&pp28_scope_equal(pp28_refresh_done,next)){okay=1;__leave;}
        pp28_refresh_done_valid=0;trace.phase="current_curve";
        int prior=-1;
        for(int g=0;g<13;g++){
            int cut=i32(context+0x10ec+4*g);
            if(cut<0||cut<prior||cut>100000){why="PP28_refresh_curve";break;}prior=cut;
        }
        if(why)__leave;
        trace.phase="tracking_snapshot";
        w=calloc(1,sizeof(*w));if(!w){why="PP28_refresh_allocation";__leave;}
        w->context=context;
        for(unsigned i=0;i<138;i++){
            Rec team=array_at(teams,i);unsigned tid=team?scalar(team,383,0x100):255;
            if(tid>=138||w->tracking[tid]){release(team);why="PP28_refresh_teams";break;}
            w->tracking[tid]=reference(team,165);release(team);
            if(!w->tracking[tid]){why="PP28_refresh_tracking";break;}
            for(unsigned g=0;g<10;g++)if(scalar(w->tracking[tid],pp28_grade_fields[g],0x108)>12){why="PP28_refresh_saved_grade";break;}
            if(why)break;
        }
        if(why)__leave;
        trace.phase="history_snapshot";
        AcquireSRWLockExclusive(&pp28_history_lock);history_locked=1;
        w->history=array_field(franchise,8);
        if(!w->history||array_count(w->history)>PP28_MAX_HISTORY){why="PP28_refresh_history";__leave;}
        w->count=(unsigned)array_count(w->history);
        for(unsigned i=0;i<w->count;i++){
            Rec p=array_at(w->history,i);if(!p){why="PP28_refresh_pick";break;}
            w->picks[i]=p;w->held++;
            for(unsigned j=0;j<i;j++)if(pp28_same(p,w->picks[j])){why="PP28_refresh_duplicate_pick";break;}
            if(why)break;
            w->ages[i]=scalar(p,0,0x100);
            Rec team=reference(p,3);unsigned tid=team?scalar(team,383,0x100):255;release(team);
            if(w->ages[i]>4||scalar(p,1,0x100)>6||scalar(p,4,0x108)>=10||tid>=138){why="PP28_refresh_pick_fields";break;}
        }
        if(why)__leave;
        pp28_refresh_work=w;InterlockedExchange(&pp28_refresh_owner,(LONG)GetCurrentThreadId());
        log_line("PP28_REFRESH_BEGIN,%u,%u,%u,NON_AGING_FIRST_PRESEASON",league,year,w->count);
        trace.phase="native_scorer";
        ((void(*)(U))(game_base+0x08422650))(context);
        InterlockedExchange(&pp28_refresh_owner,0);pp28_refresh_work=0;
        trace.published=w->published;trace.phase="completion_check";
        if(faulted){why="PP28_refresh_native_fault";__leave;}
        if(w->skipped!=1||w->published!=1380||array_count(w->history)!=w->count){why="PP28_refresh_completion";__leave;}
        trace.phase="history_readback";
        for(unsigned i=0;i<w->count;i++){
            Rec p=array_at(w->history,i);
            int same=pp28_same(p,w->picks[i])&&scalar(p,0,0x100)==w->ages[i];release(p);
            if(!same){why="PP28_refresh_history_changed";break;}
        }
        if(why)__leave;
        trace.phase="grade_readback";
        for(unsigned t=0;t<138;t++)for(unsigned g=0;g<10;g++)
            if(!w->seen[t][g]||scalar(w->tracking[t],pp28_grade_fields[g],0x108)!=(unsigned)w->expected[t][g])why="PP28_refresh_readback";
        if(why)__leave;
        completed=1;okay=1;
    } __except(pp28_refresh_exception(&trace,GetExceptionInformation())){why="PP28_refresh_exception";}
    InterlockedExchange(&pp28_refresh_owner,0);pp28_refresh_work=0;
    if(history_locked)ReleaseSRWLockExclusive(&pp28_history_lock);
    unsigned history_count=w?w->count:0;
    if(w){
        for(unsigned i=0;i<w->held;i++)pp28_refresh_drop(&w->picks[i],&trace,"release_current_pick");
        for(unsigned i=0;i<138;i++)pp28_refresh_drop(&w->tracking[i],&trace,"release_current_tracking");
        pp28_refresh_drop(&w->history,&trace,"release_current_history");free(w);
    }
    pp28_refresh_drop(&season,&trace,"release_current_season");
    pp28_refresh_drop(&franchise,&trace,"release_current_franchise");
    if(why)portal_fault(why);
    if(faulted)pp28_refresh_done_valid=0;
    else if(completed){
        pp28_refresh_done=next;pp28_refresh_done_valid=1;
        log_line("PP28_REFRESH_READY,%u,%u,grades=1380,history=%u,history_unchanged=1",league,year,history_count);
    }
    return okay&&!faulted;
}
static int pp28_refresh(U context){
    if(!context||mode!=1||faulted)return 0;
    /* Native grade notifications may synchronously query this getter. */
    if(pp28_refresh_owner==(LONG)GetCurrentThreadId())return pp28_refresh_owned(context);
    AcquireSRWLockExclusive(&pp28_refresh_lock);
    int okay=0;
    __try {if(mode==1&&!faulted)okay=pp28_refresh_inner(context);}
    __finally {ReleaseSRWLockExclusive(&pp28_refresh_lock);}
    return okay;
}
int DriPPGroupGrade(U context,Rec *team,int kind,int group,Rec *player){
    cache_hook_entry(CACHE_HOOK_TEAM_GRADE);offer_cap_team(team);
    if(kind==6&&team&&*team&&group>=0&&group<10)pp28_refresh(context);
    return NativePPGroupGrade(context,team,kind,group,player);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestPPRefresh(U context){return pp28_refresh(context);}
__declspec(dllexport) void DriTestPPRefreshReset(void){
    memset(&pp28_refresh_done,0,sizeof(pp28_refresh_done));pp28_refresh_done_valid=0;
    pp28_refresh_invalidate();
}
__declspec(dllexport) void DriTestPPRefreshAddresses(U *out){out[0]=(U)HookPPGroupGrade;out[1]=(U)NativePPGroupGrade;}
#endif
