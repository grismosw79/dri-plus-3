/* Test28 Pro Potential. Included after portal helpers in retention.c.
   Native publication retains the exact native mock-draft evidence/sort and
   writes through its original setter and change-notification path. */
#include "pp28_core.h"
#include <stdlib.h>

U ResumePPBeforeAge,NativePPAge,ResumePPPublish,NativePPSetGrade;
extern void HookPPBeforeAge(void),HookPPPublish(void);
static SRWLOCK pp28_history_lock=SRWLOCK_INIT;
static int pp28_refresh_skip_age(U context,Rec array);
static void pp28_refresh_publication(U context,unsigned team,unsigned group,int grade);

typedef struct {
    Rec records[PP28_MAX_HISTORY];
    uint32_t before[PP28_MAX_HISTORY],after[PP28_MAX_HISTORY];
    size_t count,held,touched;
} PP28HistoryWork;

static void pp28_set_year(Rec record,uint32_t value) {
    U table=q((U)record+0x10),f=field(schema(record),0);
    ((void(*)(U,uint32_t,U,uint32_t))q(q(table)+0x148))(table,row(record),f,value);cache_note_write();
    /* Exact native YearOffset writer also marks the table dirty here. */
    ((void(*)(U))(game_base+0x080595e0))(table);
}

/* Actual annual caller only: native initialization also calls the updater,
   but must not consume the one-time repair. Newly created current draft picks
   are already in this array with age0 before the native increment. */
static int pp28_normalize_history(U context,Rec array,U caller) {
    if(caller!=game_base+0x08430aed)return 0;
    Rec franchise=0,season=0;PP28HistoryWork *work=0;int result=0;
    size_t changed=0;unsigned current=0,base=0,year=0,zeros=0;
    const char *why=0,*skip=0;
    __try {
        franchise=context_ref(context,2);
        if(!franchise){why="PP28_history_franchise";__leave;}
        season=reference(franchise,44);
        if(!season){why="PP28_history_season";__leave;}
        base=scalar(season,0,0x100);year=scalar(season,2,0x100);current=scalar(season,6,0x100);
        if(current!=0 || base!=year || base<1900 || base>2200){skip="not_first_season";__leave;}
        if(!array){why="PP28_history_array";__leave;}
        U n=array_count(array);
        if(n>PP28_MAX_HISTORY){why="PP28_history_capacity";__leave;}
        if(!n){skip="empty_history";__leave;}
        work=calloc(1,sizeof(*work));
        if(!work){why="PP28_history_allocation";__leave;}
        work->count=(size_t)n;
        for(size_t i=0;i<work->count;i++) {
            Rec pick=array_at(array,i);
            if(!pick){why="PP28_history_record";__leave;}
            work->records[i]=pick;work->held++;
            U table=q((U)pick+0x10);uint32_t id=row(pick);
            for(size_t j=0;j<i;j++)if(q((U)work->records[j]+0x10)==table&&row(work->records[j])==id){why="PP28_history_duplicate";break;}
            if(why)break;
            work->before[i]=scalar(pick,0,0x100);
            unsigned round=scalar(pick,1,0x100),group=scalar(pick,4,0x108);
            Rec team=reference(pick,3);
            unsigned tid=team?scalar(team,383,0x100):255;release(team);
            if(round>6||group>=PP28_GROUPS||tid>=138){why="PP28_history_fields";break;}
            zeros+=(work->before[i]==0);
        }
        if(why)__leave;
        /* A new global draft class is required to distinguish this annual
           boundary from any initialization or read-only grade refresh. */
        if(!zeros){skip="no_current_draft_class";__leave;}
        int plan=pp28_history_plan(work->before,work->count,1,work->after,&changed);
        if(plan<0){why="PP28_history_plan";__leave;}
        if(!plan){skip="already_normal_or_unrecognized_window";__leave;}
        /* Complete read and plan succeeded before the first field write. */
        for(size_t i=0;i<work->count;i++) {
            if(work->before[i]==work->after[i])continue;
            work->touched=i+1;
            pp28_set_year(work->records[i],work->after[i]);
            if(scalar(work->records[i],0,0x100)!=work->after[i]){why="PP28_history_write";break;}
        }
        if(why)__leave;
        result=1;
        log_line("PP28_HISTORY_NORMALIZED,%u,%u,%u,%llu,%llu,await_native_age",base,year,current,
                 (unsigned long long)changed,(unsigned long long)zeros);
    } __except(fail_exception("PP28_history",GetExceptionCode())) {why="PP28_history_exception";}
    if(why && work && work->touched) {
        int restored=1;
        __try {
            for(size_t i=0;i<work->touched;i++)if(work->before[i]!=work->after[i]) {
                pp28_set_year(work->records[i],work->before[i]);
                if(scalar(work->records[i],0,0x100)!=work->before[i])restored=0;
            }
        } __except(fail_exception("PP28_history_rollback",GetExceptionCode())) {restored=0;}
        log_line("PP28_HISTORY_ROLLBACK,%s,%s",why,restored?"RESTORED":"FAILED_STOP_TEST");
    }
    if(work){for(size_t i=0;i<work->held;i++)release(work->records[i]);free(work);}
    release(season);release(franchise);
    if(why){portal_fault(why);return -1;}
    if(skip)log_line("PP28_HISTORY_SKIP,%s,%u,%u,%u",skip,base,year,current);
    return result;
}

int DriPPBeforeAge(Registers *r) {
    cache_hook_entry(CACHE_HOOK_OTHER);
    /* Our explicit non-aging refresh is scoped to its own thread, context,
       and exact history array. Even a fault must not age that refresh. */
    if(pp28_refresh_skip_age(r->r13,(Rec)q(r->rbp+0x6f)))return 1;
    if(mode!=1||faulted)return 0;
    AcquireSRWLockExclusive(&pp28_history_lock);
    __try {pp28_normalize_history(r->r13,(Rec)q(r->rbp+0x6f),q(r->rbp+0x5f));}
    __except(fail_exception("PP28_before_age",GetExceptionCode())){}
    ReleaseSRWLockExclusive(&pp28_history_lock);
    return 0;
}

void DriPPPublish(Registers *r) {
    cache_hook_entry(CACHE_HOOK_OTHER);
    if(mode!=1||faulted)return;
    __try {
        U vector=q(r->rbp-0x59),end=q(r->rbp-0x51),context=r->r13;
        size_t count=(size_t)r->rbx,index=(size_t)r->r15;unsigned group=(unsigned)r->r12;
        if(!vector||!count||count>PP28_MAX_SCHOOLS||index>=count||group>=PP28_GROUPS||
           end<vector||end-vector!=count*0xb0 || *(unsigned char*)(context+0x10e8)!=0 ||
           q(vector+index*0xb0)!=q(r->rbp-0x71)) {
            portal_fault("PP28_publish_layout");return;
        }
        PP28Evidence evidence[PP28_MAX_SCHOOLS];int32_t cutoffs[13];PP28Grade grade;
        for(size_t i=0;i<count;i++) {
            U entry=vector+i*0xb0;
            evidence[i]=(PP28Evidence){i32(entry+0x34+4*group),i32(entry+0x5c+4*group),i32(entry+0x84+4*group)};
        }
        for(int g=0;g<13;g++)cutoffs[g]=i32(context+0x10ec+4*g);
        if(!pp28_grade(evidence,count,index,cutoffs,&grade)){portal_fault("PP28_grade_evidence");return;}
        Rec team=(Rec)q(vector+index*0xb0+8);unsigned tid=team?scalar(team,383,0x100):255;
        if(tid>=138){portal_fault("PP28_grade_team");return;}
        pp28_refresh_publication(context,tid,group,grade.grade);
        log_line("PP28_GRADE,%u,%u,%u,%d,%d,%d,%d,%d",tid,group,(unsigned)r->rsi,
                 grade.grade,grade.composite,grade.projected_points,grade.better,grade.tied);
        /* The unchanged setter AND following native change notification read
           ESI. Replacing it here preserves both operations' agreement. */
        r->rsi=(U)grade.grade;
    } __except(fail_exception("PP28_publish",GetExceptionCode())){}
}

#include "pp28_refresh.inc"

#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestPPPublish(Registers *r){DriPPPublish(r);}
__declspec(dllexport) int DriTestPPHistory(U context,Rec array,U caller){return pp28_normalize_history(context,array,caller);}
__declspec(dllexport) void DriTestPPBeforeAge(Registers *r){DriPPBeforeAge(r);}
__declspec(dllexport) void DriTestPPAddresses(U *out){
    out[0]=(U)HookPPBeforeAge;out[1]=(U)HookPPPublish;
    out[2]=NativePPAge;out[3]=NativePPSetGrade;
}
#endif
