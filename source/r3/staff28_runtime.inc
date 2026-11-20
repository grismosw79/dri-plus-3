/* Test28 Coach Prestige and Proximity. Include after portal.inc and before EPS.
   No saved coach scores, team grade tables, or player city references are edited. */
#include "staff28_core.h"
#include <math.h>

U ResumeStaff28Update;
extern void HookStaff28Update(void);
extern void NativeStaff28Update(U context);
U ResumeProximity28Grade;
extern void HookProximity28Grade(void);
extern int NativeProximity28Grade(U context,Rec *team,Rec *player);
typedef struct {U table;uint32_t row,reserved;} Staff28Key;
typedef struct {
    Staff28Scores scores;
    Staff28Key coaches[3],team;
} Staff28Team;
typedef struct {
    int valid,waiting_for_teams;
    U context;
    Staff28Key franchise,teams;
    unsigned league,generation;
    Staff28Team team[STAFF28_TEAMS];
    int grades[STAFF28_TEAMS][2];
} Staff28State;
typedef struct {const char *phase;int team,role,ordinal;} Staff28Trace;
static Staff28State staff28_state;
static SRWLOCK staff28_lock=SRWLOCK_INIT;
static volatile LONG proximity28_neutral_logged,proximity28_alaska_logged;
static Staff28Key staff28_key(Rec r){
    Staff28Key k={r?q((U)r+0x10):0,r?row(r):0,0};return k;
}
static int staff28_equal(Staff28Key a,Staff28Key b){return a.table==b.table&&a.row==b.row;}
static void staff28_defer(const char *reason,int count,int index){
    staff28_state.valid=0;
    if(!staff28_state.waiting_for_teams)
        log_line("STAFF28_DEFER,%s,count=%d,index=%d,generation=%u",
                 reason,count,index,staff28_state.generation);
    staff28_state.waiting_for_teams=1;
}
static int staff28_read(Rec team,Staff28Team *out,Staff28Trace *trace){
    static const int refs[3]={111,183,58};
    memset(out,0,sizeof(*out));out->team=staff28_key(team);
    for(int k=0;k<3;k++){
        trace->role=k;
        Rec coach=reference(team,refs[k]);
        if(coach){
            out->coaches[k]=staff28_key(coach);
            out->scores.present[k]=1;
            out->scores.score[k]=(int32_t)scalar(coach,113,0x100);
            release(coach);
            if(out->scores.score[k]<0)return 0;
        }
    }
    trace->role=-1;
    return 1;
}
/* The cache owns scores and identities. Every game record comes from the
   current grade request and is released in that request. */
static int staff28_rebuild(U context,Staff28Key franchise,Rec teams,unsigned league,Staff28Trace *trace){
    Staff28Team collected[STAFF28_TEAMS];Staff28Scores scores[STAFF28_TEAMS];
    int grades[STAFF28_TEAMS][2];unsigned char seen[STAFF28_TEAMS]={0};
    int missing[3]={0};
    for(unsigned i=0;i<STAFF28_TEAMS;i++){
        trace->ordinal=(int)i;trace->team=-1;trace->role=-1;
        Rec team=array_at(teams,i);
        if(!team){staff28_defer("team_record",STAFF28_TEAMS,(int)i);return -1;}
        int tid=(int)scalar(team,383,0x100);
        trace->team=tid;
        if(tid<0||tid>=STAFF28_TEAMS||seen[tid]||!staff28_read(team,&collected[tid],trace)){
            release(team);return 0;
        }
        seen[tid]=1;scores[tid]=collected[tid].scores;
        for(int k=0;k<3;k++)missing[k]+=!scores[tid].present[k];
        release(team);
    }
    trace->phase="rank";trace->ordinal=-1;trace->team=-1;
    if(!staff28_compute(scores,STAFF28_TEAMS,grades))return 0;
    staff28_state.franchise=franchise;
    staff28_state.context=context;staff28_state.teams=staff28_key(teams);
    staff28_state.league=league;
    memcpy(staff28_state.team,collected,sizeof(collected));
    memcpy(staff28_state.grades,grades,sizeof(grades));
    staff28_state.valid=1;staff28_state.waiting_for_teams=0;staff28_state.generation++;
    log_line("STAFF28_READY,generation=%u,league=%u,teams=138,HC=75,side=25,missing_HC=%d,missing_OC=%d,missing_DC=%d,ties=best_rank",
             staff28_state.generation,league,missing[0],missing[1],missing[2]);
    return 1;
}
static int staff28_grade_locked(U context,Rec team,Rec player,int *out,Staff28Trace *trace){
    /* Grade UI initialization can precede a fully created Dynasty. Match the
       verified SeasonInfo.LeagueStarted readiness gate used by EPS/PT. */
    Rec franchise=context_ref(context,2);
    if(!franchise){staff28_state.valid=0;return 0;}
    trace->phase="season";
    Rec season=reference(franchise,44);
    int ready=season&&scalar(season,24,0xf8);release(season);
    if(!ready){release(franchise);staff28_state.valid=0;return 0;}
    trace->phase="player";int pos=(int)scalar(player,3,0x108),side=staff28_side(pos);
    if(pos<0||pos>=22){release(franchise);portal_fault("staff28_position");return 0;}
    if(side<0){release(franchise);return 0;} /* K/P/LS preserve native 75/13/12. */
    trace->phase="team_list";Rec teams=array_field(franchise,45);
    U count=teams?array_count(teams):0;
    if(count<STAFF28_TEAMS){
        release(teams);release(franchise);staff28_defer("team_list",(int)count,-1);return 0;
    }
    if(count>STAFF28_TEAMS){
        release(teams);release(franchise);staff28_state.valid=0;portal_fault("staff28_snapshot");return 0;
    }
    unsigned league=scalar(franchise,28,0x100);
    Staff28Key current_franchise=staff28_key(franchise);
    release(franchise);
    trace->phase="team";int tid=(int)scalar(team,383,0x100);trace->team=tid;
    if(tid<0||tid>=STAFF28_TEAMS){release(teams);portal_fault("staff28_team");return 0;}
    trace->phase="school_coaches";
    Staff28Team current;
    if(!league||!staff28_read(team,&current,trace)){release(teams);staff28_state.valid=0;portal_fault("staff28_scores");return 0;}
    int same=staff28_state.valid&&staff28_state.context==context&&staff28_state.league==league&&
        staff28_equal(staff28_state.teams,staff28_key(teams))&&
        staff28_equal(staff28_state.franchise,current_franchise)&&
        memcmp(&current,&staff28_state.team[tid],sizeof(current))==0;
    trace->phase="snapshot";
    if(!same){
        int result=staff28_rebuild(context,current_franchise,teams,league,trace);
        if(result!=1){
            release(teams);staff28_state.valid=0;
            if(result==0)portal_fault("staff28_snapshot");
            return 0;
        }
    }
    trace->phase="finish";release(teams);*out=staff28_state.grades[tid][side];return 1;
}
static int staff28_exception(const Staff28Trace *trace,DWORD code){
    log_line("STAFF28_CONTEXT,%s,team=%d,role=%d,index=%d,generation=%u",
             trace->phase,trace->team,trace->role,trace->ordinal,staff28_state.generation);
    return fail_exception("staff28_grade",code);
}
/* Root generic grade router passes the actual Player record, for HS, portal,
   roster/PLV, and player-specific UI. Return 0 asks it to preserve native. */
static int staff28_grade(U context,Rec team,Rec player,int *out){
    if(!context||!team||!player||!out||faulted||mode!=1)return 0;
    Staff28Trace trace={"dynasty",-1,-1,-1};
    int handled=0;AcquireSRWLockExclusive(&staff28_lock);
    __try {handled=staff28_grade_locked(context,team,player,out,&trace);}
    __except(staff28_exception(&trace,GetExceptionCode())){staff28_state.valid=0;}
    ReleaseSRWLockExclusive(&staff28_lock);return handled&&!faulted&&mode==1;
}
/* Hook native staff rank refresh, not the EPS update. This catches a changed
   coach at another school, including same-week coaching-carousel changes. */
void DriStaff28Update(U context){
    AcquireSRWLockExclusive(&staff28_lock);staff28_state.valid=0;ReleaseSRWLockExclusive(&staff28_lock);
    NativeStaff28Update(context);
}
static float proximity28_float(Rec record,int n){
    U t=q((U)record+0x10),f=field(schema(record),n);
    return ((float(*)(U,uint32_t,U))q(q(t)+0x110))(t,row(record),f);
}
static int proximity28_grade_inner(U context,Rec team,Rec player,int *out){
    unsigned state=scalar(player,214,0x100);
    if(state==50||state==51){
        /* Native neutral B- would block every school at the new B requirement. */
        *out=4;
        if(InterlockedCompareExchange(&proximity28_neutral_logged,1,0)==0)
            log_line("PROXIMITY28_NEUTRAL,states=50|51,grade=B,preserve_neutral_eligibility");
        return 1;
    }
    if(state!=1)return 0; /* Hawaii's native LA exception remains untouched. */
    Rec city=reference(team,23);
    if(!city){*out=13;return 1;}
    if(scalar(city,24,0x100)==1){release(city);return 0;}
    float lat=proximity28_float(city,17),lon=proximity28_float(city,18);release(city);
    if(!isfinite(lat)||!isfinite(lon)||lat < -90||lat>90||lon < -180||lon>180){
        portal_fault("proximity28_coordinates");return 0;
    }
    float miles=staff28_distance(47.62110137939453f,-122.32440185546875f,lat,lon,
                               (float(*)(float))(game_base+0x078cd808));
    if(!isfinite(miles)||miles<0||miles>50000){portal_fault("proximity28_distance");return 0;}
    Rec tuning=(Rec)q(context+0x88),raw=tuning?reference(tuning,14):0;
    if(!raw){*out=13;return 1;}
    Rec temp=0;
    Rec *result=((Rec*(*)(Rec,Rec*,uint32_t))vfunc(raw,0x28))(raw,&temp,0x53504c4e);
    Rec spline=result?*result:0;if(result)*result=0;release(temp);release(raw);
    if(!spline){*out=13;return 1;}
    int grade=((int(*)(Rec,int,int))(game_base+0x07e0f080))(spline,(int)miles,0);
    release(spline);
    if(grade<0||grade>12){portal_fault("proximity28_spline_grade");return 0;}
    *out=grade;
    if(InterlockedCompareExchange(&proximity28_alaska_logged,1,0)==0)
        log_line("PROXIMITY28_ALASKA,anchor=Seattle,latitude=47.62110138,longitude=-122.32440186,live_native_spline=1");
    return 1;
}
static int proximity28_grade(U context,Rec team,Rec player,int *out){
    if(!context||!team||!player||!out||faulted||mode!=1)return 0;
    int handled=0;
    __try {handled=proximity28_grade_inner(context,team,player,out);}
    __except(fail_exception("proximity28_grade",GetExceptionCode())){}
    return handled&&!faulted&&mode==1;
}
/* CPU target-cost evaluation calls the native geographic getter directly,
   bypassing the generic grade router. Intercept that entry as well so Alaska
   and neutral-state grades agree in eligibility, UI and CPU target cost. */
int DriProximity28Grade(U context,Rec *team,Rec *player){
    if(mode==1&&!faulted&&team&&player){
        __try {int grade;if(proximity28_grade(context,*team,*player,&grade))return grade;}
        __except(fail_exception("proximity28_direct",GetExceptionCode())){}
    }
    return NativeProximity28Grade(context,team,player);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestStaff28(U context,Rec team,Rec player,int *out){return staff28_grade(context,team,player,out);}
__declspec(dllexport) int DriTestProximity28(U context,Rec team,Rec player,int *out){return proximity28_grade(context,team,player,out);}
__declspec(dllexport) int DriTestProximity28Direct(U context,Rec *team,Rec *player){return DriProximity28Grade(context,team,player);}
__declspec(dllexport) void DriTestStaff28Reset(void){memset(&staff28_state,0,sizeof(staff28_state));}
__declspec(dllexport) unsigned DriTestStaff28Generation(void){return staff28_state.generation;}
__declspec(dllexport) unsigned DriTestStaff28Presence(int team,int index){return staff28_state.team[team].scores.present[index];}
__declspec(dllexport) void DriTestStaff28Addresses(U *out){out[0]=(U)NativeStaff28Update;out[1]=(U)DriStaff28Update;out[2]=(U)HookStaff28Update;out[3]=(U)NativeProximity28Grade;out[4]=(U)HookProximity28Grade;}
#endif
