/* RM17R3 dynamic CPU roster prep at Encourage Transfers. */
#include "read_core.inc"
static LONG64 invocation;
static volatile LONG site_visits[CAR35_CALL_COUNT];
static volatile LONG observer_thread;
static int safe_rec_ptr(U address,Rec *out) { return address && local_read(address,out,sizeof(*out)); }
static void trace_clock(Rec franchise,LONG64 token,int after) {
    int ok=0;Rec season=reference(franchise,44,&ok);
    if(!ok || !season){release(season);capture_error("season reference");return;}
    char b[2000];size_t len=0;
    append(b,sizeof(b),&len,"\"event\":\"CLOCK\",\"token\":%lld,\"after\":%d,\"fields\":{",(long long)token,after);
    for(unsigned j=0;j<sizeof(fields_SeasonInfo)/sizeof(RMField);j++) {
        int v;const RMField *f=&fields_SeasonInfo[j];
        if(!scalar(season,f->index,f->kind,&v) || !append(b,sizeof(b),&len,"%s\"%s\":%d",j?",":"",f->name,v)){release(season);capture_error("clock read");return;}
    }
    release(season);append(b,sizeof(b),&len,"}");emit(b);
}
static void snapshot_player(Rec p,LONG64 token,int after,int team,int source) {
    U store;uint32_t row;char b[1800];size_t len=0;
    if(!identity(p,&store,&row)){capture_error("player identity");return;}
    if(!append(b,sizeof(b),&len,"\"event\":\"PLAYER\",\"token\":%lld,\"after\":%d,\"team\":%d,\"source\":%d,\"store\":\"%llx\",\"row\":%u,\"values\":[",(long long)token,after,team,source,(unsigned long long)store,row)){capture_error("player output");return;}
    for(unsigned j=0;j<sizeof(fields_Player)/sizeof(RMField);j++) {
        int v;const RMField *f=&fields_Player[j];
        if(!scalar(p,f->index,f->kind,&v) || !append(b,sizeof(b),&len,"%s%d",j?",":"",v)){capture_error("player read");return;}
    }
    append(b,sizeof(b),&len,"]");emit(b);
}
static void snapshot_team(Rec team,LONG64 token,int after) {
    int id,academy,ok=0;
    if(!scalar(team,383,2,&id) || id<0 || id>=138 || !scalar(team,132,1,&academy)){capture_error("team boundary");return;}
    Rec user=reference(team,407,&ok);int is_user=user!=0;release(user);
    if(!ok){capture_error("user reference");return;}
    char b[512];
    snprintf(b,sizeof(b),"\"event\":\"TEAM\",\"token\":%lld,\"after\":%d,\"team\":%d,\"academy\":%d,\"user\":%d",(long long)token,after,id,academy,is_user);emit(b);
    for(int source=0;source<2 && !capture_fault;source++) {
        Rec arr=array_field(team,source?33:239,&ok);U n=array_count(arr);
        if(!ok || n>180){release(arr);capture_error("roster length");return;}
        snprintf(b,sizeof(b),"\"event\":\"ARRAY\",\"token\":%lld,\"after\":%d,\"team\":%d,\"source\":%d,\"count\":%llu",(long long)token,after,id,source,(unsigned long long)n);emit(b);
        for(U i=0;i<n && !capture_fault;i++) {
            Rec p=array_at(arr,i,&ok);
            if(!ok || !p){release(p);capture_error("roster member");break;}
            snapshot_player(p,token,after,id,source);release(p);
        }
        release(arr);
    }
}
static Rec franchise_for(unsigned site,U module) {
    unsigned kind=flow_sites[site].context;int ok=0;Rec result=0;
    if(kind==1) {
        Rec manager=reference((Rec)module,26,&ok);
        if(ok && manager)result=reference(manager,2,&ok);
        release(manager);
    } else if(kind==2) result=reference((Rec)module,5,&ok);
    else return 0;
    if(!ok || !result){release(result);capture_error("franchise context");return 0;}
    return result;
}
static void trace_capture(unsigned site,U module,U tp,U pp,LONG64 token,int after) {
    if(!enabled || capture_fault || !rm18_deep())return;
    if((DWORD)observer_thread!=GetCurrentThreadId()) {
        /* No unsynchronized shared capture state across native processing threads. */
        capture_error("native callback changed thread; observation disabled");return;
    }
    Rec franchise=0;
    __try {
        franchise=franchise_for(site,module);
        if(franchise && !capture_fault)trace_clock(franchise,token,after);
        if(capture_fault)__leave;
        if(tp) {
            Rec team=0;
            if(!safe_rec_ptr(tp,&team) || !team)capture_error("team argument");
            else snapshot_team(team,token,after);
            if(pp && !capture_fault) {
                Rec player=0;
                if(!safe_rec_ptr(pp,&player) || !player)capture_error("player argument");
                else snapshot_player(player,token,after,-1,2);
            }
        } else if(franchise) {
            int ok=0;Rec teams=array_field(franchise,45,&ok);U n=array_count(teams);
            if(!ok || n!=138){release(teams);capture_error("league length (expected 138)");__leave;}
            unsigned char seen[138]={0};
            for(U i=0;i<n && !capture_fault;i++) {
                Rec team=array_at(teams,i,&ok);int id=-1;
                if(!ok || !team || !scalar(team,383,2,&id) || id<0 || id>=138 || seen[id])capture_error("league team identity");
                else {seen[id]=1;snapshot_team(team,token,after);}
                release(team);
            }
            release(teams);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {capture_error("read-only snapshot exception; native call still forwarded");}
    release(franchise);
}
static __declspec(noinline) LONG64 trace_begin(unsigned site,U module,U team,U player) {
    if(!enabled || capture_fault || site>=CAR35_CALL_COUNT)return 0;
    InterlockedCompareExchange(&observer_thread,(LONG)GetCurrentThreadId(),0);
    if(site==4) { for(unsigned k=0;k<CAR35_CALL_COUNT;k++)InterlockedExchange(&site_visits[k],0); }
    LONG visits=InterlockedIncrement(&site_visits[site]);
    if(visits>2048){capture_error("callback budget exceeded");return 0;}
    LONG64 token=InterlockedIncrement64(&invocation);
    char b[700];snprintf(b,sizeof(b),"\"event\":\"BEGIN\",\"site\":\"%s\",\"site_id\":%u,\"token\":%lld,\"visit\":%ld,\"native_rva\":%u",flow_sites[site].name,site,(long long)token,visits,flow_sites[site].target);emit(b);
    trace_capture(site,module,team,player,token,0);if(rm18_deep())FlushFileBuffers(log_file);return token;
}
static __declspec(noinline) void trace_end(unsigned site,U module,U team,U player,LONG64 token) {
    if(!token)return;
    trace_capture(site,module,team,player,token,1);
    char b[512];snprintf(b,sizeof(b),"\"event\":\"END\",\"site\":\"%s\",\"site_id\":%u,\"token\":%lld,\"capture_fault\":%ld",flow_sites[site].name,site,(long long)token,capture_fault);emit(b);if(rm18_deep())FlushFileBuffers(log_file);
}
#include "native_actions.inc"
#include "flow_wrappers.inc"
#include "rm_install.inc"
__declspec(dllexport) const void *DriDiagnosticInfo(void) {
    static const struct {char magic[16];const void *p[13];} info={"RM16-FLOW-v1",{&game_base,(const void*)&enabled,(const void*)&capture_fault,trace_begin,trace_end,verify_build,car35_install_all,car35_calls,trace_capture,&log_file,(const void*)&observer_thread,&tls_index,snapshot_team}};
    return &info;
}
__declspec(dllexport) const void *DriRM17R3Info(void) {
 static const struct {char magic[16];const void *p[21];} info={"RM17R3-FULL-v1",{rm_cycle,rm_before_enroll,rm_native_gate,rm_native_execute,rm_pay,(const void*)&rm_failed,(const void*)&rm_done,(const void*)&rm_year,&rm_manager,&rm_state,&rm_refs,rm_teams,&rm_active,rm_block,rm_clock,rm_configs,pilot_preserved,pilot_rotate,rm_lookup,rm_index_put,rm_pid}};
 return &info;
}
static volatile LONG initialize_result=17;
static __declspec(noinline) DWORD initialize_once(void) {
    game_base=(U)GetModuleHandleW(0);
    if(GetModuleHandleW(L"DRIRosterManagementRM17R2.dll") || GetModuleHandleW(L"DRIRosterManagementRM17R1.dll") || GetModuleHandleW(L"DRIRosterManagementRM17.dll") || GetModuleHandleW(L"DRIRosterManagementRM16R2.dll") || GetModuleHandleW(L"DRIRosterManagementRM16R1.dll") || GetModuleHandleW(L"DRIRosterManagementRM16.dll") || GetModuleHandleW(L"DRIRosterManagementRM15.dll") || GetModuleHandleW(L"DRIRosterManagementRM12.dll") || GetModuleHandleW(L"DRIRosterManagementRM13.dll"))return 19;
    accepted_r3_base=(U)self_module;
    if(!accepted_r3_base)return 18;
    unsigned char cap=0;
    if(!local_read(game_base+0x81b4031,&cap,1) || cap!=89)return 20;
    wchar_t path[32768];DWORD n=GetModuleFileNameW(self_module,path,32000);
    if(!n || n>=32000)return 12;while(n && path[n-1]!=L'\\')n--;
    SYSTEMTIME t;GetSystemTime(&t);
    swprintf(path+n,32768-n,L"RM18R22_Roster_%04u%02u%02u_%02u%02u%02u_%lu.jsonl",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,(unsigned long)GetCurrentProcessId());
    log_file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
    if(log_file==INVALID_HANDLE_VALUE)return 12;
    log_line("RM18R22 CPU roster prep; native paid NIL; source-first transfers; academies unsigned HS only; bounded depth flexibility; documented under85 exceptions; last-resort native cuts only above90; LIVE UNVERIFIED");
    tls_index=TlsAlloc();if(tls_index==TLS_OUT_OF_INDEXES)return 12;
    char fields[1200];size_t len=0;append(fields,sizeof(fields),&len,"\"event\":\"PLAYER_COLUMNS\",\"columns\":[");
    for(unsigned i=0;i<sizeof(fields_Player)/sizeof(RMField);i++)append(fields,sizeof(fields),&len,"%s\"%s\"",i?",":"",fields_Player[i].name);
    append(fields,sizeof(fields),&len,"]");emit(fields);
    if(!verify_build()){log_verify_failure();log_line("SIGNATURE_OR_SCHEMA_MISMATCH; no RM17R3 hooks installed");return 13;}
    log_line("R3_COMPAT_PASS grade_rva=0x08407000 owned_combined_symbol=HookEpsGrade; exact detour and unchanged tail verified");
    if(capture_fault)return 12;
    if(!car35_install_all()){log_verify_failure();log_line("INSTALL_FAILED; restart game");return 15;}
    if(capture_fault)return 12;
    InterlockedExchange(&enabled,1);log_line("READY RM18R22 roster module; combined enrollment90 active; disposable ENC branch only; live validation pending");
    if(capture_fault){InterlockedExchange(&enabled,0);return 12;}
    return 0;
}
__declspec(dllexport) DWORD WINAPI DriInitialize(void *unused) {
    (void)unused;
    if(InterlockedCompareExchange(&installed,1,0)!=0) {
        LONG previous=InterlockedCompareExchange(&initialize_result,17,17);
        return previous? (DWORD)previous:21; /* Never mask a failure or claim reactivation. */
    }
    DWORD result=initialize_once();
    if(log_file!=INVALID_HANDLE_VALUE && !FlushFileBuffers(log_file)) {
        InterlockedExchange(&capture_fault,1);InterlockedExchange(&enabled,0);if(!result)result=12;
    }
    InterlockedExchange(&initialize_result,(LONG)result);return result;
}
__declspec(dllexport) const void *DriActivationInfo(void) {
    static const struct {char magic[16];const void *p[6];} info={"RM17R3-INIT-v1",{
        &accepted_r3_base,&verify_failure,(const void*)&installed,(const void*)&initialize_result,initialize_once,log_line}};
    return &info;
}
__declspec(dllexport) DWORD WINAPI DriDisable(void *unused) {
    (void)unused;InterlockedExchange(&enabled,0);log_line("DISABLED; native pass-through hooks remain until game closes");FlushFileBuffers(log_file);return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID unused) {
    (void)unused;if(reason==DLL_PROCESS_ATTACH)self_module=h;return TRUE;
}

__declspec(dllexport) const struct {char tag[24];U pointers[7];} rm_failure_diagnostics={"RM17R3-FAILURE-v1",{(U)&rm_failed_route,(U)&rm_fallback,(U)&rm_mutation_started,(U)&rm_gate_inflight,(U)&rm_native_diverged,(U)&rm_after_validate,(U)&rm_validation_count}};

__declspec(dllexport) const struct {char tag[24];U pointers[3];} rm_role_read_diagnostics={"RM17R3-ROLE-READ-v1",{(U)&rm_charts,(U)&rm_finish,(U)&rm_add_player}};

int Rm18RosterPreflight(void){game_base=(U)GetModuleHandleW(0);accepted_r3_base=0;return verify_build();}

__declspec(dllexport) const struct {char tag[24];U p[4];} rm18_roster_install_diag={"RM18R22-ros-INSTALL",{(U)&verify_build,(U)&car35_install_all,(U)&validate_image_padding,(U)&log_line}};

__declspec(dllexport) const struct {char tag[24];U p[4];} rm18_cut_diag={"RM18R22-CUT-v1",{(U)&rm_native_cut,(U)&rm_cut_states,(U)&rm_franchise,(U)&rm_array_occurrences}};

__declspec(dllexport) const struct {char tag[24];U p[3];} rm18_user_diag={"RM18R22-USER-v1",{(U)&rm_select_user_fallback,(U)&rm_managed,(U)&rm_open}};

__declspec(dllexport) const struct {char tag[24];U p[6];} rm18_replan_diag={"RM18R22-REPLAN-v1",{(U)&rm_rewind_pending,(U)&rm_replan_remaining,(U)&rm_run_plan,(U)&rm_plan_ready,(U)&rm_executed,(U)&rm_plan_remaining}};

__declspec(dllexport) const struct {char tag[24];U p[14];} rm18_audit_diag={"RM18R22-AUDIT-v1",{(U)&rm_audit_prepare,(U)&rm_audit_seal,(U)&rm_expected_states,(U)&rm_expected_nil,(U)&rm_expected_money,(U)&rm_audit_ready,(U)&rm_cut_verified,(U)&rm_team_phase,(U)&rm_end_complete,(U)&rm_encourage_started,(U)&rm_encourage_complete,(U)&rm_phase_enter,(U)&rm_phase_leave,(U)&rm_audit_phase}};
__declspec(dllexport) const struct {char tag[24];U p[1];} rm18_cycle_diag={"RM18R22-CYCLE-v1",{(U)&rm_planned}};
__declspec(dllexport) const struct {char tag[24];U p[10];} rm18_record_diag={"RM18R22-RECORD-v1",{(U)&scalar,(U)&reference,(U)&array_field,(U)&array_count,(U)&array_at,(U)&release,(U)&identity,(U)&rm_snapshot_quiet,(U)&emit,(U)&log_line}};
__declspec(dllexport) const struct {char tag[24];U p[3];} rm18_program_diag={"RM18R22-PROGRAM-v1",{(U)&rm_audit_program,(U)&rm_expected_facilities,(U)&rm_program_checkpoint}};
__declspec(dllexport) const struct {char tag[24];U p[5];} rm18_enrollment_diag={"RM18R22-ENROLL-v1",{(U)&rm_enrollment_prepare,(U)&rm_enrollment_seal,(U)&rm_enrollment_contracts,(U)&rm_enrollment_holdback,(U)&rm_enrollment_reserve}};
