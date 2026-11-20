/* RM12 all-CPU compatible position preparation. Independent accepted-R3 overlay.
   Six verified call sites; native XP remains unchanged. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

typedef uintptr_t U;
typedef void *Rec;
static U game_base;
static HMODULE self_module;
static volatile LONG enabled, installed, capture_fault;
static LONG64 sequence;
static DWORD tls_index=TLS_OUT_OF_INDEXES;
static HANDLE log_file=INVALID_HANDLE_VALUE;
static SRWLOCK log_lock=SRWLOCK_INIT;
static void rm_team(void*,Rec*);
static void rm_year(void*,Rec*);
static void dev_reset(void);
static void rm_dev_roster(void*,Rec*,Rec*,Rec*);
static void rm_dev_commit(void*,Rec*,Rec*,Rec*);
static int rm_score(void*,Rec*);
static int rm_chance(void*,int,Rec*,Rec*);
#include "native_profile.h"
#include "../log_io.h"

static int local_read(U address,void *out,size_t n) {
    SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),(void*)address,out,n,&got) && got==n;
}
static int code_pointer(U p) { return p>=game_base+0x1000 && p<game_base+0xa3b5000; }
static uint64_t hash_bytes(const void *p,size_t n) {
    const unsigned char *b=p;uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<n;i++)h=(h^b[i])*UINT64_C(1099511628211);return h;
}
static void emit(const char *body) {
    if(log_file==INVALID_HANDLE_VALUE)return;
    char line[32768];DWORD wrote=0;
    AcquireSRWLockExclusive(&log_lock);
    LONG64 seq=InterlockedIncrement64(&sequence);
    int n=snprintf(line,sizeof(line),"{\"seq\":%lld,\"thread\":%lu,%s}\r\n",(long long)seq,(unsigned long)GetCurrentThreadId(),body);
    if(n<0 || n>=(int)sizeof(line) || !WriteFile(log_file,line,(DWORD)n,&wrote,0) || wrote!=(DWORD)n)InterlockedExchange(&capture_fault,1);
    ReleaseSRWLockExclusive(&log_lock);
}
static __declspec(noinline) int logging_team(void) {
    return tls_index==TLS_OUT_OF_INDEXES ? -1 : (int)(intptr_t)TlsGetValue(tls_index)-1;
}
static __declspec(noinline) void log_line(const char *fmt,...) {
    char raw[1600],body[3400];va_list args;va_start(args,fmt);vsnprintf(raw,sizeof(raw),fmt,args);va_end(args);
    size_t k=(size_t)snprintf(body,sizeof(body),"\"event\":\"STATUS\",\"team\":%d,\"message\":\"",logging_team());
    for(size_t i=0;raw[i] && k+3<sizeof(body);i++) {
        unsigned char c=(unsigned char)raw[i];if(c=='"' || c=='\\')body[k++]='\\';
        body[k++]=c<32?' ':c;
    }
    body[k++]='"';body[k]=0;emit(body);
}
static void capture_error(const char *where) {
    InterlockedExchange(&capture_fault,1);log_line("CAPTURE_FAILED %s; native processing continues",where);FlushFileBuffers(log_file);
}
static int verify_build(void) {
    unsigned char b[32];
    for(size_t i=0;i<sizeof(code_guards)/sizeof(code_guards[0]);i++) {
        const RMCodeGuard *g=&code_guards[i];
        if(!local_read(game_base+g->rva,b,g->size) || hash_bytes(b,g->size)!=g->hash)return 0;
    }
    for(size_t i=0;i<sizeof(table_guards)/sizeof(table_guards[0]);i++) {
        const RMTableGuard *g=&table_guards[i];uint64_t h=UINT64_C(14695981039346656037);
        for(unsigned j=0;j<g->count;j++) {
            uint64_t pair[2];if(!local_read(game_base+g->rva+j*16,pair,sizeof(pair)) || pair[0]!=j || pair[1]<game_base)return 0;
            pair[1]-=game_base;
            for(size_t k=0;k<sizeof(pair);k++)h=(h^((unsigned char*)pair)[k])*UINT64_C(1099511628211);
        }
        if(h!=g->hash)return 0;
    }
    return 1;
}

/* Stored fields only: no speculative method invocation. Reference getters own
   one reference, which is released on the same game thread. */
static int field_context(Rec r,unsigned index,U *data,uint32_t *row,U *field) {
    U vt,getter,type,cache;int16_t count;
    if(!r || !local_read((U)r,&vt,8) || !local_read(vt+0x38,&getter,8) || !code_pointer(getter))return 0;
    type=((U(*)(Rec))getter)(r);
    if(!type || !local_read(type+0xd0,&cache,8) || !cache || !local_read(type+0xd8,&count,2) || count<=0 || index>=(unsigned)count)return 0;
    *field=((U(*)(U,unsigned))(game_base+0x8055d60))(type,index);
    return *field!=0xffffffffu && local_read((U)r+0x10,data,8) && *data && local_read((U)r+0x18,row,4);
}
static __declspec(noinline) int scalar(Rec r,unsigned index,unsigned kind,int *value) {
    U data,field,vt,fn;uint32_t row;
    if(!field_context(r,index,&data,&row,&field) || !local_read(data,&vt,8) ||
       !local_read(vt+(kind==1?0xf8:kind==2?0x100:0x108),&fn,8) || !code_pointer(fn))return 0;
    *value=kind==1?((unsigned char(*)(U,uint32_t,U))fn)(data,row,field)!=0:((int(*)(U,uint32_t,U))fn)(data,row,field);
    return 1;
}
static __declspec(noinline) void release(Rec r) {
    U vt,fn;if(!r)return;
    if(local_read((U)r,&vt,8) && local_read(vt+8,&fn,8) && code_pointer(fn))((void(*)(Rec))fn)(r);
    else capture_error("reference release");
}
static __declspec(noinline) Rec reference(Rec r,unsigned index,int *ok) {
    U data,field,vt,fn;uint32_t row;Rec out=0;*ok=0;
    if(!field_context(r,index,&data,&row,&field) || !local_read(data,&vt,8) || !local_read(vt+0x128,&fn,8) || !code_pointer(fn))return 0;
    ((void(*)(U,Rec*,uint32_t,U))fn)(data,&out,row,field);*ok=1;return out;
}
static __declspec(noinline) Rec array_field(Rec r,unsigned index,int *ok) {
    Rec raw=reference(r,index,ok),temp=0,out=0;U vt,fn;
    if(!*ok || !raw)return raw;
    if(!local_read((U)raw,&vt,8) || !local_read(vt+0x28,&fn,8) || !code_pointer(fn)){*ok=0;release(raw);return 0;}
    Rec *result=((Rec*(*)(Rec,Rec*,unsigned))fn)(raw,&temp,0x41525259);
    if(result){out=*result;*result=0;}else *ok=0;
    release(temp);release(raw);return out;
}
static __declspec(noinline) U array_count(Rec a) { return a?((U(*)(Rec))(game_base+0x37f4130))(a):0; }
static __declspec(noinline) Rec array_at(Rec a,U index,int *ok) {
    U data,vt,fn;uint32_t row;Rec out=0;*ok=0;
    if(!a || !local_read((U)a+0x10,&data,8) || !local_read((U)a+0x18,&row,4) ||
       !local_read(data,&vt,8) || !local_read(vt+0x128,&fn,8) || !code_pointer(fn))return 0;
    ((void(*)(U,Rec*,uint32_t,U))fn)(data,&out,row,index);*ok=1;return out;
}
static int identity(Rec r,U *store,uint32_t *row) {
    return r && local_read((U)r+0x10,store,8) && local_read((U)r+0x18,row,4);
}
static __declspec(noinline) int target(Rec team) {
    int id=-1,ok=0;Rec user;
    if(!enabled || capture_fault || !scalar(team,383,2,&id) || (id<0 || id>=138))return 0;
    user=reference(team,407,&ok);int is_user=user!=0;release(user);
    if(!ok){capture_error("team user boundary");return 0;}
    return !is_user;
}
static int append(char *b,size_t cap,size_t *len,const char *fmt,...) {
    if(*len>=cap)return 0;va_list args;va_start(args,fmt);int n=vsnprintf(b+*len,cap-*len,fmt,args);va_end(args);
    if(n<0 || (size_t)n>=cap-*len)return 0;*len+=(size_t)n;return 1;
}
static __declspec(noinline) void player_snapshot_values(const char *event,Rec p,int source,const int *values) {
    if(!rm18_deep())return;
    char b[28000];size_t len=0;U store;uint32_t row;
    if(!identity(p,&store,&row)){capture_error("player identity");return;}
    if(!append(b,sizeof(b),&len,"\"event\":\"%s\",\"team\":%d,\"store\":\"%llx\",\"row\":%u,\"source\":%d,\"fields\":{",event,logging_team(),(unsigned long long)store,row,source))return;
    for(size_t i=0;i<sizeof(fields_Player)/sizeof(fields_Player[0]);i++) {
        int v;const RMField *f=&fields_Player[i];
        if(values)v=values[i];
        else if(!scalar(p,f->index,f->kind,&v)){capture_error("player scalar capture");return;}
        if(!append(b,sizeof(b),&len,"%s\"%s\":%d",i?",":"",f->name,v)){capture_error("player output capacity");return;}
    }
    if(!append(b,sizeof(b),&len,"}")){capture_error("player output capacity");return;}emit(b);
}
static __declspec(noinline) void player_snapshot(const char *event,Rec p,int source,int unused1,int unused2) {
    (void)unused1;(void)unused2;player_snapshot_values(event,p,source,0);
}
static __declspec(noinline) void team_snapshot(const char *event,Rec team) {
    char b[26000];size_t len=0;int ok=0;
    append(b,sizeof(b),&len,"\"event\":\"%s\",\"team\":%d,\"charts\":{",event,logging_team());
    Rec chart=reference(team,63,&ok);
    if(!ok || !chart){release(chart);capture_error("chart missing");return;}
    for(size_t i=0;i<sizeof(charts)/sizeof(charts[0]);i++) {
        Rec a=array_field(chart,charts[i].index,&ok);U n=array_count(a);
        if(!ok || n>180){release(a);release(chart);capture_error("chart array");return;}
        if(!append(b,sizeof(b),&len,"%s\"%s\":[",i?",":"",charts[i].name)){release(a);release(chart);capture_error("chart output capacity");return;}
        for(U j=0;j<n;j++) {
            Rec p=array_at(a,j,&ok);U store=0;uint32_t row=UINT32_MAX;
            if(!ok || (p&&!identity(p,&store,&row)) || !append(b,sizeof(b),&len,"%s{\"store\":\"%llx\",\"row\":%u}",j?",":"",(unsigned long long)store,row)) {
                release(p);release(a);release(chart);capture_error("chart player");return;
            }
            release(p);
        }
        release(a);if(!append(b,sizeof(b),&len,"]")){release(chart);capture_error("chart close");return;}
    }
    release(chart);if(!append(b,sizeof(b),&len,"}")){capture_error("chart close");return;}emit(b);
}
static void roster_snapshot(const char *event,Rec team) {
    if(!rm18_deep())return;
    int ok=0;
    for(int source=0;source<2;source++) {
        Rec a=array_field(team,source?33:239,&ok);U n=array_count(a);
        if(!ok || n>180){release(a);capture_error("roster array");return;}
        log_line("TEAM_MEMBERS event=%s source=%d count=%llu",event,source,(unsigned long long)n);
        for(U j=0;j<n && !capture_fault;j++) {
            Rec p=array_at(a,j,&ok);if(ok && p)player_snapshot(event,p,source,0,0);else capture_error("roster player");release(p);
        }
        release(a);
    }
}
static __declspec(noinline) void season_record_snapshot(Rec season,const char *event) {
    char b[6000];size_t n=0;append(b,sizeof(b),&n,"\"event\":\"%s\",\"fields\":{",event);
    for(size_t i=0;i<sizeof(fields_SeasonInfo)/sizeof(fields_SeasonInfo[0]);i++) {
        int v;const RMField *f=&fields_SeasonInfo[i];
        if(!scalar(season,f->index,f->kind,&v) || !append(b,sizeof(b),&n,"%s\"%s\":%d",i?",":"",f->name,v)){capture_error("season context");return;}
    }
    append(b,sizeof(b),&n,"}");emit(b);
}
#include "rm_active.inc"
#include "rm_development.inc"
static void rm_year(void *module,Rec *team) {
    dev_start(module);
    /* Native rollover is forwarded exactly once on every path. */
    int id=-1;void *previous=TlsGetValue(tls_index);
    int scoped=team && target(*team) && scalar(*team,383,2,&id);
    if(scoped && TlsSetValue(tls_index,(void*)(intptr_t)(id+1))) {
        if(InterlockedCompareExchange(&prep_busy,1,0)==0) {
            ULONGLONG started=GetTickCount64();int ok=0,accepted=0,verified=0;
            Rec franchise=reference(module,2,&ok),season=0;
            if(ok && franchise)season=reference(franchise,44,&ok);
            if(ok && season && pilot_stage_guard(season) && league_census(franchise,season)) {
                if(prep_seen[id])log_line("TEAM_DUPLICATE_SKIPPED native rollover still forwarded");
                else {
                    prep_seen[id]=1;
                    season_record_snapshot(season,"PRE_YEAR_SEASON");
                    if(pilot_guard(*team,season)) {
                        accepted=1;team_snapshot("PRE_YEAR",*team);
                        if(!capture_fault)dev_prepare(*team,season);
                        if(!capture_fault)pilot_positions(module,*team,franchise);
                        if(!capture_fault)verified=pilot_finish(*team);
                    }
                    if(!accepted)log_line("TEAM_INPUT_REJECT no position writes");
                    int units=0,deferred=0;for(int i=0;i<6;i++){units+=unit_status[i]==1;deferred+=unit_status[i]!=1;}
                    completed_cpu++;
                    if(verified)prep_seen[id]=2;
                    log_line("TEAM_PREP_COMPLETE accepted=%d verified=%d units=%d deferred=%d conversions=%d players=%u fault=%ld elapsed_ms=%llu",accepted,verified,units,deferred,team_conversions,pilot_count,capture_fault,(unsigned long long)(GetTickCount64()-started));
                    log_line("CPU_COVERAGE visited=%d expected=%d",completed_cpu,expected_cpu);
                    if(completed_cpu==expected_cpu)log_line("CPU_COVERAGE_COMPLETE visited=%d expected=%d",completed_cpu,expected_cpu);
                }
            } else if(!ok || !season)capture_error("TeamManager franchise/season context");
            pilot_clear();release(season);release(franchise);
            InterlockedExchange(&prep_busy,0);FlushFileBuffers(log_file);
        } else log_line("TEAM_BUSY_DEFERRED no custom writes; native rollover still forwarded");
    } else if(scoped)capture_error("TLS context");
    ((void(*)(void*,Rec*))(game_base+0x859c1d0))(module,team);
    TlsSetValue(tls_index,previous);
}
static void rm_team(void *module,Rec *team) {
    /* Witness AFTER native training. Development is wrapped separately; XP is native. */
    int id=-1;void *previous=TlsGetValue(tls_index);
    int scoped=team && target(*team) && scalar(*team,383,2,&id);
    if(scoped && !TlsSetValue(tls_index,(void*)(intptr_t)(id+1))){capture_error("training TLS context");scoped=0;}
    ((void(*)(void*,Rec*))(game_base+0x8315c60))(module,team);
    if(scoped && !capture_fault && InterlockedCompareExchange(&prep_busy,1,0)==0) {
        if(league_valid && prep_seen[id]==2 && !training_seen[id]) {
            team_snapshot("TEAM_AFTER",*team);roster_snapshot("TEAM_AFTER",*team);
            training_seen[id]=1;trained_cpu++;
            if(!rm18_deep())
                log_line("DEV_TEAM_HOLDS season=%d team=%d unchanged=%ld",census_season,id,InterlockedExchange(&dev_held_by_team[id],0));
            log_line("NATIVE_TRAINING_COMPLETE fault=%ld observed=%d expected=%d custom_dev_enabled=1",capture_fault,trained_cpu,expected_cpu);
            if(trained_cpu==expected_cpu)log_line("TRAINING_COVERAGE_COMPLETE observed=%d expected=%d",trained_cpu,expected_cpu);
            FlushFileBuffers(log_file);
        }
        InterlockedExchange(&prep_busy,0);
    }
    TlsSetValue(tls_index,previous);
}
#include "rm_install.inc"

/* Read-only diagnostic addresses let offline verification exercise the exact
   shipped machine code, without replacing callbacks in a special test build. */
__declspec(dllexport) const void *DriDiagnosticInfo(void) {
    static const struct { char magic[16]; const void *p[45]; } info={"RM12-ABI-MAP-v1",{
        &game_base,(const void*)&enabled,(const void*)&capture_fault,&pilot_ready,pilot_players,&pilot_count,
        target,reference,release,scalar,read_all,set_scalar,cast_record,input_index,
        role_grade,array_replace,pilot_guard,pilot_positions,convert_preserving,
        pilot_finish,league_census,log_line,player_snapshot,team_snapshot,season_record_snapshot,pilot_clear,
        array_field,array_at,array_count,rm_inputs,rm_initial,rm_expected,prep_seen,(const void*)&prep_busy,logging_team,player_snapshot_values,position_targets,dev_prepare,dev_apply_down,dev_draw,dev_lookup,dev_state,&tls_index,&dev_tls_index,dev_records}};
    return &info;
}

__declspec(dllexport) DWORD WINAPI DriInitialize(void *unused) {
    (void)unused;
    if(InterlockedCompareExchange(&installed,1,0)!=0)return 17;
    game_base=(U)GetModuleHandleW(0);
    if(GetModuleHandleW(L"DRIRosterManagementRM07.dll")||GetModuleHandleW(L"DRIRosterManagementRM08.dll")||GetModuleHandleW(L"DRIRosterManagementRM08R1.dll")||GetModuleHandleW(L"DRIRosterManagementRM09.dll")||GetModuleHandleW(L"DRIRosterManagementRM09R1.dll")||GetModuleHandleW(L"DRIRosterManagementRM10.dll")||GetModuleHandleW(L"DRIRosterManagementRM11.dll"))return 19;
    /* Combined root installs the R3 component first. */
    wchar_t path[32768];DWORD n=GetModuleFileNameW(self_module,path,32000);
    if(!n || n>=32000)return 12;while(n && path[n-1]!=L'\\')n--;
    SYSTEMTIME t;GetSystemTime(&t);
    swprintf(path+n,32768-n,L"RM18R22_Positions_Development_%04u%02u%02u_%02u%02u%02u_%lu.jsonl",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,(unsigned long)GetCurrentProcessId());
    log_file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
    if(log_file==INVALID_HANDLE_VALUE)return 12;
    log_line("RM12 BALANCED DEVELOPMENT AND EMERGENCY K/P TEST; live roster inputs; user excluded; custom_dev_enabled=1; native XP unchanged; no transfer/cut/finance writes; accepted R3 overlay");
    tls_index=TlsAlloc();dev_tls_index=TlsAlloc();if(tls_index==TLS_OUT_OF_INDEXES||dev_tls_index==TLS_OUT_OF_INDEXES)return 12;
    if(!verify_build()){log_line("SIGNATURE_OR_SCHEMA_MISMATCH; no hooks installed");return 13;}
    if(!car35_install_all()){log_line("INSTALL_FAILED; restart game");return 15;}
    InterlockedExchange(&enabled,1);log_line("READY ideal-count CPU positions and annual balanced development; six call sites; native XP unchanged");FlushFileBuffers(log_file);return 0;
}
__declspec(dllexport) DWORD WINAPI DriDisable(void *unused) {
    (void)unused;InterlockedExchange(&enabled,0);log_line("DISABLED; pass-through hooks remain until game closes");FlushFileBuffers(log_file);return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID unused) {
    (void)unused;if(reason==DLL_PROCESS_ATTACH)self_module=h;return TRUE;
}

int Rm18PositionPreflight(void){game_base=(U)GetModuleHandleW(0);return verify_build();}

__declspec(dllexport) const struct {char tag[24];U p[4];} rm18_positions_dev_install_diag={"RM18R22-pos-INSTALL",{(U)&verify_build,(U)&car35_install_all,(U)&validate_image_padding,(U)&log_line}};

__declspec(dllexport) const struct {char tag[24];U p[9];} rm18_annual_diag={"RM18R22-ANNUAL-v1",{(U)&dev_type,(U)&dev_stat_values,(U)&dev_read_player,(U)&dev_freeze,(U)&dev_start,(U)&dev_count,(U)&dev_frozen,(U)&census_year,(U)&census_season}};
