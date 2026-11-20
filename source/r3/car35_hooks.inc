static void car35_module_trace(void *m);
/* Native ABI boundaries are guarded by captured bytes. These hooks never
   alter saved record fields directly. Candidate scores and native return
   values are changed; original engine code owns offer/contract assignments. */
typedef struct {
    void *coach;
    int raw,raw_valid,native,proposed;
    int64_t exact;
    int merit,rank,role,status,level,history,user;
    int broad_seen,broad_pass,qualified,support_applied,post_pass,tier;
    int grade,unit,games,blacklist,qualification_path;
    int hc_wins,hc_losses;
    const char *hc_reason;
    const char *competition_reason;
    uint32_t row;
} car35_CandidateMeta;
typedef struct car35_BuildContext {
    void *module,*team;
    int destination,week,current_week,cap,count,bad,sort_active,merit_enabled,coordinator_enabled;
    int population,parity_enabled,team_user,destination_conference;
    LONG id;
    car35_CandidateMeta entries[1024];
} car35_BuildContext;
static DWORD car35_tls_index=TLS_OUT_OF_INDEXES;
static DWORD car35_gate_tls=TLS_OUT_OF_INDEXES;
typedef struct { void *module,*coach,*team;int destination; } Car35GateContext;
static volatile LONG car35_build_events,car35_length_events;
static car35_CandidateMeta *car35_meta_for(car35_BuildContext *c,void *coach,int create) {
    for(int i=0;i<c->count;i++) if(c->entries[i].coach==coach) return &c->entries[i];
    if(!create || c->count>=1024) { if(create)c->bad=1; return NULL; }
    car35_CandidateMeta *m=&c->entries[c->count++];memset(m,0,sizeof(*m));
    m->coach=coach;m->rank=-1;return m;
}
/* R9: seven exact evaluation-only reads use CPU semantics for both control
   modes. The saved user flag, ownership, offer acceptance and signing remain
   untouched. The paired FTCs remove the two user-only scoring bonuses. */
__declspec(dllexport) unsigned char car35_parity_bool(void *data,uint32_t row,void *field,U reader) {
    unsigned char original=((unsigned char(*)(void*,uint32_t,void*))reader)(data,row,field);
    car35_BuildContext *c=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    int active=mode==1 && c && c->parity_enabled && !c->bad;
    if(original && active)log_line("CAR35_USER_PARITY build=%ld caller_rva=0x%llx stored_user=1 evaluation_user=0 ownership_unchanged=1",
        c->id,(unsigned long long)((U)__builtin_return_address(0)-game_base));
    return active?0:original;
}
__attribute__((naked,used)) static void car35_parity_rbx_adapter(void) {
    __asm__ volatile("mov 0x18(%rbx), %edx\n\tjmp car35_parity_bool\n\t");
}
__attribute__((naked,used)) static void car35_parity_rdi_adapter(void) {
    __asm__ volatile("mov 0x18(%rdi), %edx\n\tjmp car35_parity_bool\n\t");
}
__attribute__((naked,used)) static void car35_parity_rsi_adapter(void) {
    __asm__ volatile("mov 0x18(%rsi), %edx\n\tjmp car35_parity_bool\n\t");
}
static int car35_phase(void *module,int *week,int *current_week) {
    void *season=NULL;int stage=-1,active=-1;
    return car35_local_read((uintptr_t)module+0x138,&season,8) && season &&
        car35_record_field(season,3,0,&stage) && car35_record_field(season,5,0,week) &&
        car35_record_field(season,4,2,current_week) && car35_record_field(season,12,1,&active) &&
        car35_active_phase(stage,*week,active);
}
static uint32_t car35_row_of(void *record) {
    uint32_t row=UINT32_MAX;
    if(record)car35_local_read((uintptr_t)record+0x18,&row,4);
    return row&0x1ffffu;
}
#include "car35_competition.inc"
/* R7: an established HC's current team-performance rank gets the same
   bounded merit calculation already used for coordinator promotions.
   Read real current-season results before using the rank; a missing or
   unplayed season retains R6 scoring. This never revives a native zero,
   changes FTC eligibility, or creates coach history. */
static int car35_hc_performance_rank(car35_CandidateMeta *m) {
    int rank=-1;
    m->hc_wins=-1;m->hc_losses=-1;
    void *season=car35_retain_reference(m->coach,61);
    int valid=car35_record_field(season,1,2,&m->hc_wins) &&
        car35_record_field(season,0,2,&m->hc_losses);
    car35_release_reference(season);
    if(!valid || m->hc_wins<0 || m->hc_wins>32 || m->hc_losses<0 || m->hc_losses>100 ||
       m->hc_wins+m->hc_losses<=0) {
        m->hc_reason="no_verified_current_season_native_merit_retained";return -1;
    }
    if(!car35_record_field(m->coach,120,2,&rank) || rank<0 || rank>137) {
        m->hc_reason="unverified_rank_native_merit_retained";return -1;
    }
    m->hc_reason="current_HC_team_rank_same_merit_formula";return rank;
}
/* The shared builder invokes this adapter before any fallback insertion.
   Copy stack arguments into our own call frame; never overwrite the native
   builder's live stack slots. RBP+0xb0 is guarded destination role. */
__attribute__((naked,used)) static void car35_broad_adapter(void) {
    __asm__ volatile("sub $0x48, %rsp\n\t"
      "mov 0x70(%rsp), %rax\n\tmov %rax, 0x20(%rsp)\n\t"
      "mov 0x78(%rsp), %rax\n\tmov %rax, 0x28(%rsp)\n\t"
      "mov 0xb0(%rbp), %eax\n\tmov %eax, 0x30(%rsp)\n\t"
      "call car35_broad_hook\n\tadd $0x48, %rsp\n\tret\n\t");
}
__declspec(dllexport) unsigned char car35_broad_hook(void *module,void **coach_ref,void **team_ref,void **old_team,int cap,unsigned char flag,int destination) {
    typedef unsigned char(*Broad)(void*,void**,void**,void**,int,unsigned char);
    void *coach=coach_ref?*coach_ref:NULL,*team=team_ref?*team_ref:NULL;
    int floor=car35_emergency_candidate_floor(module,team,coach,destination,TlsGetValue(car35_tls_index));
    if(coach && !car35_eligible_at_floor(coach,old_team?*old_team:NULL,destination,"candidate_before_fallback",floor)) {
        car35_BuildContext *build=(car35_BuildContext*)TlsGetValue(car35_tls_index);
        int role=car35_get(coach,16,0),previous_role=car35_get(coach,17,0),prestige=car35_get(coach,112,0);
        log_line("CAR35_CANDIDATE_GATE build=%ld coach_row=%lu team_row=%lu destination=%d result=0 status=%d current_role=%d prev_role=%d side=%d prestige=%d native_called=0 reason=%s",
            build?build->id:0,(unsigned long)car35_row(coach),(unsigned long)car35_row(team),destination,
            car35_get(coach,12,0),role,previous_role,car35_get(coach,134,0),prestige,
            car35_promotion_grade_at_floor(destination,role,previous_role,prestige,floor)?"structural_identity_reject":"promotion_below_floor_or_invalid_grade");
        return 0;
    }
    car35_BuildContext *scope=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    int conference=-2;
    if(coach && car35_get(coach,16,0)==0 && (destination==1 || destination==2) &&
       (car35_get(coach,12,0)==0 || car35_get(coach,12,0)==4) &&
       scope && scope->module==module && scope->team==team && scope->destination==destination) {
        if(scope->destination_conference==-2)scope->destination_conference=car35_hc_destination_conference(module,team);
        conference=scope->destination_conference;
    }
    if(coach && !car35_hc_move_allowed(module,coach,old_team?*old_team:NULL,team,destination,conference,"candidate_before_fallback")) {
        log_line("CAR35_CANDIDATE_GATE build=%ld coach_row=%lu team_row=%lu destination=%d result=0 status=%d current_role=0 native_called=0 reason=employed_HC_nonP4_coordinator",
            scope?scope->id:0,(unsigned long)car35_row(coach),(unsigned long)car35_row(team),destination,car35_get(coach,12,0));
        return 0;
    }
    Car35GateContext context={module,coach,team,destination};
    void *previous=TlsGetValue(car35_gate_tls);
    int stored=TlsSetValue(car35_gate_tls,&context)!=0;
    unsigned char result=((Broad)(game_base+0x08330e30))(module,coach_ref,team_ref,old_team,cap,flag);
    if(stored)TlsSetValue(car35_gate_tls,previous);
    car35_BuildContext *build=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    if(build && build->coordinator_enabled && build->module==module && build->team==team &&
       build->destination==destination && coach) {
        /* Rejected pool rows do not consume the candidate-vector capacity. */
        car35_CandidateMeta *meta=car35_meta_for(build,coach,result!=0);
        if(meta) {meta->broad_seen=1;meta->broad_pass=result!=0;}
    }
    log_line("CAR35_CANDIDATE_GATE build=%ld coach_row=%lu team_row=%lu destination=%d result=%d status=%d current_role=%d side=%d prestige=%d years=%d tenure=%d offers=%d offer_cap=%d reason=%s",
      ((car35_BuildContext*)TlsGetValue(car35_tls_index))?((car35_BuildContext*)TlsGetValue(car35_tls_index))->id:0,
      (unsigned long)car35_row(coach),(unsigned long)car35_row(team),destination,result,
      car35_get(coach,12,0),car35_get(coach,16,0),car35_get(coach,134,0),car35_get(coach,112,0),
      car35_get(coach,135,2),car35_get(coach,19,2),car35_get(coach,128,2),cap,result?"native_broad_pass":"native_broad_reject");
    return result;
}
__declspec(dllexport) unsigned char car35_gate_hook(void *module,void **coach_ref) {
    typedef unsigned char (*Gate)(void*,void**);
    unsigned char original=((Gate)(game_base+CAR35_GATE_RVA))(module,coach_ref);
    if(original)return original;
    Car35GateContext *c=TlsGetValue(car35_gate_tls);
    void *coach=coach_ref?*coach_ref:NULL;int status=-1,role=-1,user=-1,team_user=-1,week=-1,now=-1,old=-1,target=-1;
    if(!c || c->module!=module || c->coach!=coach || !coach || !c->team ||
       !car35_record_field(coach,12,0,&status) || status!=4 ||
       !car35_record_field(coach,16,0,&role) || (role!=1 && role!=2) ||
       !car35_record_field(coach,0,1,&user) ||
       !car35_record_reference_present(c->team,407,&team_user) ||
       !((c->destination==0 && !team_user) || c->destination==role) ||
       !car35_record_field(coach,8,2,&old) || !car35_record_field(c->team,383,2,&target) || old==target ||
       !car35_phase(module,&week,&now))return original;
    car35_BuildContext *build=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    if(user && (!build || !build->parity_enabled || build->bad))return original;
    /* R8: renewing lateral candidates obey the loaded coordinator tenure
       threshold. Preserve the distinct, tested HC promotion exception. */
    if(c->destination==role) {
        void *tuning=NULL;int tenure=-1,threshold=-1;
        int allowed=car35_local_read((U)module+0x148,&tuning,8) && tuning &&
            car35_record_field(tuning,94,2,&threshold) && threshold>=0 &&
            car35_record_field(coach,19,2,&tenure) && tenure>threshold;
        log_line("CAR35_COORD_TENURE coach_row=%lu team_row=%lu destination=%d status=%d tenure=%d threshold=%d passed=%d rule=strict_greater_than_loaded_FTC mode=%ld",
            (unsigned long)car35_row(coach),(unsigned long)car35_row(c->team),role,status,tenure,threshold,allowed,mode);
        if(!allowed)return original;
    }
    int capacity=((unsigned char(*)(void*))(game_base+CAR35_CAPACITY_RVA))(module)!=0;
    int result=mode==1 && capacity;
    log_line("%s coach_row=%lu team_row=%lu status=%d role=%d current_week=%d week_type=%d original=0 capacity=%d returned=%d mode=%ld",
        c->destination==0?"CAR35_RENEWAL_PROMOTION":"CAR35_RENEWAL_MOBILITY",(unsigned long)car35_row(coach),(unsigned long)car35_row(c->team),status,role,now,week,capacity,result,mode);
    return (unsigned char)result;
}
/* R9: normal CPU and manual-school coordinator searches may use the same
   native per-vacancy available funds. This is a ceiling, not a forced offer:
   native coach demands, negotiation, outstanding reservations and spending
   still determine actual terms. No money, allocations or FTC posture records
   are written. HC, zero-budget and emergency searches keep their native path. */
static int car35_coordinator_budget(car35_BuildContext *c,int original,U caller) {
    int allocated=-1,available=-1,proposed=original;
    const char *reason="native_scope_retained";
    if(c && c->coordinator_enabled && (caller==0x08352cdd || caller==0x0892df4e) && original>0) {
        car35_record_field(c->team,c->destination==1?184:59,2,&allocated);
        int values[3]={0,0,0};
        struct {int *begin,*end,*capacity;} vector={values,values+3,values+3};
        void *team=c->team;
        ((void(*)(void*,void**,void*))(game_base+0x0833bd00))(c->module,&team,&vector);
        if(vector.begin==values && vector.end==values+3 && vector.capacity==values+3) {
            available=values[c->destination];
            if(available>=0) {
                if(available>original)proposed=available;
                reason=proposed>original?"shared_native_available_funds":"native_funds_limit";
            } else reason="invalid_available_native_retained";
        } else reason="invalid_vector_native_retained";
    }
    if(c)log_line("CAR35_COORD_BUDGET build=%ld team_row=%lu destination=%d caller_rva=0x%llx native_ceiling=%d role_allocation=%d native_available=%d proposed=%d returned=%d reason=%s mode=%d",
      c->id,(unsigned long)car35_row(c->team),c->destination,(unsigned long long)caller,original,allocated,available,proposed,mode==1?proposed:original,reason,mode);
    return mode==1?proposed:original;
}
__declspec(dllexport) void car35_builder_hook(void *module,void **team_ref,int destination,
                                               void *out,unsigned char a5,unsigned char a6,int a7) {
    typedef void (*Build)(void*,void**,int,void*,unsigned char,unsigned char,int);
    Build original=(Build)(game_base+0x08345870u);
    U caller=(U)__builtin_return_address(0)-game_base;
    car35_module_trace(module);
    log_line("CAR35_BUILD_CALL caller_rva=0x%llx destination=%d flag5=%d flag6=%d points=%d",(unsigned long long)((U)__builtin_return_address(0)-game_base),destination,a5,a6,a7);
    void *team=NULL;int user=-1,week=-1,now=-1,cap=-1;
    car35_BuildContext *previous=(car35_BuildContext*)TlsGetValue(car35_tls_index),*c=NULL;
    /* A nested out-of-scope build must not reuse its caller's context. */
    if(!TlsSetValue(car35_tls_index,NULL)) {
        if(previous)previous->bad=1;
        original(module,team_ref,destination,out,a5,a6,a7);return;
    }
    if(destination>=0 && destination<=2 && car35_local_read((uintptr_t)team_ref,&team,8) && team) {
        car35_record_reference_present(team,407,&user);car35_phase(module,&week,&now);
        car35_local_read((uintptr_t)module+0x1f8,&cap,4);
        c=(car35_BuildContext*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*c));
        if(c) {
            c->module=module;c->team=team;c->destination=destination;c->week=week;c->current_week=now;c->cap=cap;
            c->destination_conference=-2;
            c->team_user=user;
            void *tuning=NULL;int prestige_bonus=-1,expressed_bonus=-1;
            int phase=car35_phase(module,&week,&now) && cap==240 && user>=0;
            int paired=car35_local_read((U)module+0x148,&tuning,8) && tuning &&
                car35_record_field(tuning,119,2,&prestige_bonus) &&
                car35_record_field(tuning,118,2,&expressed_bonus) &&
                prestige_bonus==0 && expressed_bonus==0;
            c->parity_enabled=phase && paired;
            c->merit_enabled=destination==0 && user==0 && c->parity_enabled;
            c->coordinator_enabled=(destination==1 || destination==2) && c->parity_enabled;
            if(phase && !paired)log_line("FAULT CAR35_PARITY_FTC UserPrestigeBonus=%d UserExpressedInterestScoreBonus=%d required=0 import_paired_R9_FTCs_and_restart=1",prestige_bonus,expressed_bonus);
            if(c->coordinator_enabled)c->population=car35_competition_population(module);
            c->id=InterlockedIncrement(&car35_build_events);
            log_line("CAR35_PARITY_SCOPE build=%ld destination=%d team_user=%d enabled=%d prestige_bonus=%d expressed_bonus=%d",
                c->id,destination,user,c->parity_enabled,prestige_bonus,expressed_bonus);
            if(!TlsSetValue(car35_tls_index,c)) { HeapFree(GetProcessHeap(),0,c);c=NULL; }
        }
    }
    log_line("BUILD_SCOPE team_row=%lu destination=%d current_week=%d week_type=%d team_user=%d cap=%d enabled=%d mode=%d",
        (unsigned long)car35_row_of(team),destination,now,week,user,cap,c!=NULL,mode);
    int points=car35_coordinator_budget(c,a7,caller);
    Car35Emergency *emergency=car35_emergency_context();
    U empty_begin=0,empty_end=0;
    int staged=mode==1 && c && !c->bad && emergency && !emergency->building && !emergency->selected &&
        emergency->module==module && emergency->team==team && destination==0 && caller==0x083362d8 &&
        a5==0 && a6==1 && car35_emergency_owner(emergency) && car35_emergency_vacancy(team) &&
        car35_emergency_vector(out,&empty_begin,&empty_end) && empty_begin==empty_end;
    if(staged) {
        /* The captured builder appends to its output. Retry only while it is
           EMPTY: never clear/drop a live native-owned candidate or offer.
           Native emergency selection uses the first entry, even at score 0;
           normal affordability/interest gates are not grafted onto it. */
        emergency->building=1;emergency->build=c;
        for(int floor=6;floor<=8;floor++) {
            emergency->floor=floor;
            original(module,team_ref,destination,out,a5,a6,points);
            U b=0,e=0;int valid=car35_emergency_vector(out,&b,&e);
            log_line("CAR35_EMERGENCY_TIER build=%ld team_row=%lu minimum_grade=%d count=%lld native_rules_retained=1",
                c->id,(unsigned long)car35_row(team),floor,valid?(long long)((e-b)/48):-1LL);
            if(!valid || c->bad || !car35_emergency_owner(emergency))break;
            if(e>b) {car35_emergency_retain_selected(emergency,out);break;}
            if(!car35_emergency_vacancy(team))break;
            c->count=0; /* no output records own these per-build observations */
        }
        emergency->building=0;emergency->build=NULL;
    } else original(module,team_ref,destination,out,a5,a6,points);
    U begin=0,end=0;
    if(car35_local_read((U)out,&begin,8) && car35_local_read((U)out+8,&end,8) && end>=begin && (end-begin)%48==0 && (end-begin)/48<=4096) {
        log_line("CAR35_CANDIDATE_RESULT build=%ld team_row=%lu destination=%d count=%llu",c?c->id:0,(unsigned long)car35_row(team),destination,(unsigned long long)((end-begin)/48));
        for(U p=begin;p<end;p+=48) {
            void *coach=NULL;int score=-1;car35_local_read(p,&coach,8);car35_local_read(p+24,&score,4);
            log_line("CAR35_FINAL_ORDER build=%ld team_row=%lu destination=%d slot=%llu coach_row=%lu score=%d side=%d",c?c->id:0,(unsigned long)car35_row(team),destination,(unsigned long long)((p-begin)/48+1),(unsigned long)car35_row(coach),score,car35_get(coach,134,0));
            int expected=-1,offered=-1,interest=-1,final_interest=-1,comparison=-1,cutoff=-1;
            car35_local_read(p+16,&expected,4);car35_local_read(p+20,&offered,4);
            car35_local_read(p+28,&interest,4);car35_local_read(p+32,&final_interest,4);
            car35_local_read(p+40,&comparison,4);car35_local_read((U)module+0x23c,&cutoff,4);
            car35_CandidateMeta *meta=c?car35_meta_for(c,coach,0):NULL;
            log_line("CAR35_DECISION build=%ld team_row=%lu destination=%d coach_row=%lu status=%d grade=%d level=%d years=%d score_called=%d school_score=%d job_comparison=%d expected_points=%d offered_points=%d initial_interest=%d final_interest=%d interest_cutoff=%d point_gate=%d interest_gate=%d final_score=%d",
              c?c->id:0,(unsigned long)car35_row(team),destination,(unsigned long)car35_row(coach),car35_get(coach,12,0),car35_get(coach,112,0),car35_get(coach,14,2),car35_get(coach,135,2),meta?meta->raw_valid:0,meta?meta->raw:0,comparison,expected,offered,interest,final_interest,cutoff,expected>=0 && offered>=expected,cutoff>=0 && final_interest>=cutoff,score);
        }
    }
    if(!TlsSetValue(car35_tls_index,previous)) {
        if(c)c->bad=1;
        log_line("TLS_RESTORE_FAILED context retained and scoring suppressed; restart game");
        return;
    }
    if(c)HeapFree(GetProcessHeap(),0,c);
}
__declspec(dllexport) int car35_score_hook(void *module,void **team_ref,int destination,void **coach_ref,int points) {
    typedef int (*Score)(void*,void**,int,void**,int);
    int original=((Score)(game_base+CAR35_SCORE_RVA))(module,team_ref,destination,coach_ref,points);
    car35_BuildContext *c=(car35_BuildContext*)TlsGetValue(car35_tls_index);void *coach=NULL,*team=NULL;int result=original;
    if(c && c->module==module && c->destination==destination &&
       car35_local_read((uintptr_t)team_ref,&team,8) && c->team==team &&
       car35_local_read((uintptr_t)coach_ref,&coach,8) && coach) {
        car35_CandidateMeta *m=car35_meta_for(c,coach,1);
        if(m) {
            m->raw=original;m->raw_valid=1;m->support_applied=0;m->qualified=0;m->post_pass=0;
            m->competition_reason=NULL;
            if(c->coordinator_enabled && original>=0) {
                m->competition_reason=car35_competition_qualify(c,m);
                if(original==0 && m->qualified && mode==1) {result=1;m->support_applied=1;}
                log_line("CAR35_COMPETITION_RAW build=%ld coach_row=%lu team_row=%lu destination=%d broad_seen=%d broad_pass=%d population=%d grade=%d unit_rank=%d career_games=%d blacklist=%d qualified=%d native_raw=%d returned_raw=%d support_applied=%d reason=%s mode=%d",
                  c->id,(unsigned long)car35_row(coach),(unsigned long)car35_row(team),destination,m->broad_seen,m->broad_pass,c->population,m->grade,m->unit,m->games,m->blacklist,m->qualified,original,result,m->support_applied,m->competition_reason,mode);
            }
        }
    }
    log_line("CAR35_NATIVE_SCORE build=%ld coach_row=%lu team_row=%lu destination=%d native_raw=%d base_valuation=%d returned_raw=%d",
      c?c->id:0,(unsigned long)car35_row(coach),(unsigned long)car35_row(team),destination,original,points,result);
    return result;
}
/* Observe the actual loaded school-fit curve at its native evaluation site.
   Values come from the active FTC, not a package banner or hardcoded policy. */
__declspec(dllexport) int car35_school_curve_hook(void *spline,int prestige,int flag) {
    int result=((int(*)(void*,int,int))(game_base+0x07e0f080))(spline,prestige,flag);
    car35_BuildContext *c=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    if(c)log_line("CAR35_SCHOOL_CURVE build=%ld team_row=%lu destination=%d school_prestige=%d fit_points=%d",c->id,(unsigned long)car35_row(c->team),c->destination,prestige,result);
    return result;
}
static unsigned char car35_compare(const void *a,const void *b) {
    car35_BuildContext *c=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    if(!c || !c->sort_active)return ((unsigned char(*)(const void*,const void*))(game_base+0x08353fe0u))(a,b);
    void *ca=*(void*const*)a,*cb=*(void*const*)b;
    car35_CandidateMeta *x=car35_meta_for(c,ca,0),*y=car35_meta_for(c,cb,0);
    if(!x || !y)return ((unsigned char(*)(const void*,const void*))(game_base+0x08353fe0u))(a,b);
    if(c->coordinator_enabled) {
        if(x->tier!=y->tier)return x->tier>y->tier;
        if(x->tier==4) {
            if(x->grade!=y->grade)return x->grade<y->grade;
            if(x->unit!=y->unit)return x->unit<y->unit;
            if(x->level!=y->level)return x->level>y->level;
            return x->row<y->row;
        }
    }
    if(x->exact!=y->exact)return x->exact>y->exact;
    if(x->merit!=y->merit)return x->merit>y->merit;
    if(x->raw!=y->raw)return x->raw>y->raw;
    if(x->level!=y->level)return x->level>y->level;
    return x->row<y->row;
}
/* R8: qualified experienced coordinators compete together ahead of weak
   native fits and entrants. Use the same merit and native post gates for
   positive fits and supported raw-zero candidates. No status bonus. */
static void car35_coordinator_sort(void *begin,void *end,void *compare,car35_BuildContext *c) {
    typedef void (*Sort)(void*,void*,void*);
    Sort native_sort=(Sort)(game_base+0x083238f0u);
    U first=(U)begin,last=(U)end;const char *reason=NULL;size_t count=0;
    if(c->bad || compare!=(void*)(game_base+0x08353fe0u) || last<first || (last-first)%48 || (last-first)/48>1024)reason="invalid_context_or_vector";
    if(!reason)count=(last-first)/48;
    for(size_t i=0;!reason && i<count;i++) {
        U entry=first+i*48;void *coach=NULL;
        if(!car35_local_read(entry,&coach,8) || !coach) {reason="invalid_coach";break;}
        car35_CandidateMeta *m=car35_meta_for(c,coach,1);
        if(!m || !car35_local_read(entry+24,&m->native,4) || m->native<0 || m->native>c->cap ||
           !car35_record_field(coach,12,0,&m->status) || !car35_record_field(coach,0,1,&m->user) ||
           !car35_record_field(coach,14,2,&m->level)) {reason="invalid_metadata";break;}
        m->row=car35_row(coach);m->proposed=m->native;m->merit=0;
        if(m->status==7 && m->native>0 && m->raw_valid && m->raw>m->native && m->raw<=c->cap)m->proposed=m->raw;
        m->tier=m->proposed>0?(m->status==7 && m->raw==0?1:3):0;
        if(m->qualified) {
            if(car35_competition_post(c,m,entry))m->tier=4;
            else if(m->support_applied) {
                if(m->status!=7)m->proposed=0;
                m->tier=m->proposed>0?1:0;
            }
        }
        log_line("CAR35_QUALIFICATION build=%ld coach_row=%lu user=%d source_role=%d path=%d grade=%d comparison_unit_rank=%d games=%d qualified=%d post_pass=%d",
            c->id,(unsigned long)m->row,m->user,car35_get(m->coach,16,0),m->qualification_path,m->grade,m->unit,m->games,m->qualified,m->post_pass);
        m->exact=(int64_t)m->proposed*274;
    }
    if(reason || c->bad) {
        log_line("CAR35_COORD_SKIP build=%ld reason=%s",c->id,reason?reason:"metadata_overflow");
        car35_competition_withdraw(begin,end,c);
        native_sort(begin,end,compare);return;
    }
    for(size_t i=0;i<count;i++) {
        unsigned char *entry=(unsigned char*)begin+i*48;
        car35_CandidateMeta *m=car35_meta_for(c,*(void**)entry,0);
        if(mode==1)memcpy(entry+24,&m->proposed,4);
        log_line("CAR35_COORD_SCORE build=%ld coach_row=%lu native_final=%d raw_valid=%d native_raw=%d returned=%d status=%d level=%d mode=%d",
          c->id,(unsigned long)m->row,m->native,m->raw_valid,m->raw,mode==1?m->proposed:m->native,m->status,m->level,mode);
        log_line("CAR35_COORD_POLICY build=%ld coach_row=%lu grade=%d unit_rank=%d reason=%s native=%d proposed=%d returned=%d",
          c->id,(unsigned long)m->row,car35_get(m->coach,112,0),car35_get(m->coach,120,2),
          m->tier==4?"qualified_experienced_priority":m->support_applied?"support_failed_post_native_gates":
          m->status==2 && m->native==0?"zero_fit_pending_fire_not_rescued":
          m->proposed>m->native?"positive_free_agent_fit_restored":m->native>0?"native_positive_retained":"native_zero_retained",
          m->native,m->proposed,mode==1?m->proposed:m->native);
        log_line("CAR35_COMPETITION_FINAL build=%ld coach_row=%lu tier=%d qualified=%d support_applied=%d post_gate_pass=%d blacklist=%d native_raw=%d native_post=%d returned=%d reason=%s",
          c->id,(unsigned long)m->row,m->tier,m->qualified,m->support_applied,m->post_pass,m->blacklist,m->raw,m->native,mode==1?m->proposed:m->native,
          m->competition_reason?m->competition_reason:"native_path_retained");
    }
    c->sort_active=mode==1;native_sort(begin,end,mode==1?(void*)&car35_compare:compare);c->sort_active=0;
}
__declspec(dllexport) void car35_sort_hook(void *begin,void *end,void *original_compare) {
    typedef void (*Sort)(void*,void*,void*);
    Sort original=(Sort)(game_base+0x083238f0u);
    car35_BuildContext *c=(car35_BuildContext*)TlsGetValue(car35_tls_index);
    if(c && c->coordinator_enabled) {car35_coordinator_sort(begin,end,original_compare,c);return;}
    uintptr_t first=(uintptr_t)begin,last=(uintptr_t)end;
    const char *reason=NULL;size_t count=0;int fallback_count=0,fallback_min=0,fallback_max=0;
    if(!c || !c->merit_enabled)reason="merit_scope_disabled";
    else if(c->bad)reason="invalid_context";
    else if(original_compare!=(void*)(game_base+0x08353fe0u))reason="unexpected_comparator";
    else if(last<first)reason="reversed_vector";
    else if((last-first)%0x30)reason="invalid_vector_stride";
    else if((last-first)/0x30>1024)reason="candidate_limit";
    if(reason) {
        log_line("SORT_SKIP build=%ld team_row=%lu candidates=0 reason=%s",c?c->id:0,
            (unsigned long)(c?car35_row_of(c->team):UINT32_MAX),reason);
        original(begin,end,original_compare);return;
    }
    count=(last-first)/0x30;
    for(size_t i=0;i<count;i++) {
        uintptr_t entry=first+i*0x30;void *coach=NULL;int old=-1,target=-1;
        if(!car35_local_read(entry,&coach,8) || !coach) { reason="invalid_candidate";break; }
        car35_CandidateMeta *m=car35_meta_for(c,coach,1);
        if(!m || !car35_local_read(entry+0x18,&m->native,4) ||
           !car35_record_field(coach,0,1,&m->user) || !car35_record_field(coach,12,0,&m->status) ||
           !car35_record_field(coach,16,0,&m->role) || !car35_record_field(coach,14,2,&m->level) ||
           !car35_record_field(coach,8,2,&old) || !car35_record_field(c->team,383,2,&target)) { reason="candidate_read_failed";break; }
        m->row=car35_row_of(coach);m->proposed=m->native;m->exact=(int64_t)m->native*274;
        m->merit=0;m->rank=-1;m->history=0;
        m->hc_wins=-1;m->hc_losses=-1;m->hc_reason="native_path_retained";
        /* R9: the same merit calculation and bounds apply to either owner. */
        if(m->native<0 || m->native>c->cap) { reason="native_score_out_of_range";break; }
        if(m->status==7 && m->native>0) {
            if(!car35_record_reference_present(coach,29,&m->history)) { reason="career_reference_failed";break; }
            if(m->history && m->raw_valid && m->raw>0 && m->raw<=c->cap) {
                m->exact=car35_merit_numerator(m->raw,-1);m->proposed=car35_merit_round(m->exact);
            } else if(!m->history) {
                if(!fallback_count || m->level<fallback_min)fallback_min=m->level;
                if(!fallback_count || m->level>fallback_max)fallback_max=m->level;
                ++fallback_count;
            }
        } else if(m->native>0 && (m->status==0 || m->status==2 || m->status==4)) {
            if(m->role<0 || m->role>2) { reason="unknown_role";break; }
            if((m->role==1 || m->role==2) && old>=0 && old<255 && old!=target) {
                if(!car35_record_field(coach,120,2,&m->rank) || m->rank<0 || m->rank>137) { reason="rank_read_failed";break; }
                m->merit=80*(137-2*m->rank);
            }
            if(m->role==0 && old>=0 && old<255 && old!=target) {
                m->rank=car35_hc_performance_rank(m);
                if(m->rank>=0)m->merit=80*(137-2*m->rank);
            }
            m->exact=car35_merit_numerator(m->native,m->rank);m->proposed=car35_merit_round(m->exact);
        }
    }
    if(reason || c->bad) {
        log_line("SORT_SKIP build=%ld team_row=%lu candidates=%llu reason=%s",c->id,(unsigned long)car35_row_of(c->team),(unsigned long long)count,reason?reason:"metadata_overflow");
        original(begin,end,original_compare);return;
    }
    /* Validate the complete list before changing any score. Sorting uses the
       native entry move/release operations with a replacement comparator. */
    log_line("FALLBACK_POOL build=%ld team_row=%lu candidates=%d minimum_level=%d maximum_level=%d equal_level_score=5",
        c->id,(unsigned long)car35_row_of(c->team),fallback_count,fallback_min,fallback_max);
    for(size_t i=0;i<count;i++) {
        unsigned char *entry=(unsigned char*)begin+i*0x30;void *coach=*(void**)entry;
        car35_CandidateMeta *m=car35_meta_for(c,coach,0);
        if(m->status==7 && m->native>0 && !m->history) {
            m->exact=car35_fallback_numerator(m->level,fallback_min,fallback_max);
            m->proposed=car35_merit_round((int)m->exact);
        }
        if(mode==1)memcpy(entry+0x18,&m->proposed,4);
        log_line("SCORE build=%ld coach_row=%lu team_row=%lu role=%d status=%d user=%d current_week=%d week_type=%d rank0=%d level=%d career=%d raw_valid=%d raw=%d native_final=%d proposed=%d exact=%lld returned=%d mode=%d",
            c->id,(unsigned long)m->row,(unsigned long)car35_row_of(c->team),m->role,m->status,m->user,c->current_week,c->week,m->rank,m->level,m->history,m->raw_valid,m->raw,m->native,m->proposed,(long long)m->exact,mode==1?m->proposed:m->native,mode);
        if(m->role==0)log_line("CAR35_HC_MERIT build=%ld coach_row=%lu team_row=%lu status=%d user=%d wins=%d losses=%d rank0=%d native_final=%d prior_merit_score=%d proposed=%d returned=%d reason=%s",
            c->id,(unsigned long)m->row,(unsigned long)car35_row_of(c->team),m->status,m->user,
            m->hc_wins,m->hc_losses,m->rank,m->native,
            (m->native>0 && (m->status==0 || m->status==2 || m->status==4))?car35_merit_round(car35_merit_numerator(m->native,-1)):m->native,
            m->proposed,mode==1?m->proposed:m->native,m->native==0?"native_final_zero_not_revived":m->hc_reason);
    }
    c->sort_active=mode==1;
    original(begin,end,mode==1?(void*)&car35_compare:original_compare);
    c->sort_active=0;
    for(size_t i=0;i<count;i++) {
        unsigned char *entry=(unsigned char*)begin+i*0x30;
        log_line("ORDER build=%ld team_row=%lu slot=%llu coach_row=%lu score=%d",c->id,(unsigned long)car35_row_of(c->team),(unsigned long long)i+1,(unsigned long)car35_row_of(*(void**)entry),*(int*)(entry+0x18));
    }
}
__declspec(dllexport) int car35_length_hook(void *module,int kind,void **coach_ref,void **team_ref,int destination,unsigned char flag) {
    typedef int (*Length)(void*,int,void**,void**,int,unsigned char);
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    car35_module_trace(module);
    int original=((Length)(game_base+0x08344b50u))(module,kind,coach_ref,team_ref,destination,flag);
    void *coach=NULL,*team=NULL;int user=-1,status=-1,result=original;
    int valid=original>0 && kind>=0 && kind<=2 && destination>=0 && destination<=2 &&
        car35_local_read((uintptr_t)coach_ref,&coach,8) && coach && car35_record_field(coach,0,1,&user) &&
        car35_record_field(coach,12,0,&status);
    car35_local_read((uintptr_t)team_ref,&team,8);
    if(valid && !user && mode==1)result=car35_contract_years(destination);
    log_line("LENGTH event=%ld caller_rva=0x%llx coach_row=%lu team_row=%lu kind=%d destination=%d status=%d user=%d native=%d returned=%d valid=%d mode=%d",
        InterlockedIncrement(&car35_length_events),(unsigned long long)(caller-game_base),(unsigned long)car35_row_of(coach),(unsigned long)car35_row_of(team),kind,destination,status,user,original,result,valid,mode);
    return result;
}
__declspec(dllexport) void car35_sign_hook(void *manager,void **team_ref,void **offer_ref,unsigned char flag) {
    typedef void (*Sign)(void*,void**,void**,unsigned char);
    void *offer=NULL,*coach=NULL,*team=NULL;int offered=-1,role=-1,points=-1,length=-1,remaining=-1,status=-1;
    car35_local_read((uintptr_t)team_ref,&team,8);
    if(car35_local_read((uintptr_t)offer_ref,&offer,8) && offer) {
        car35_record_field(offer,6,2,&offered);car35_record_field(offer,3,0,&role);car35_record_field(offer,7,2,&points);
        coach=car35_retain_reference(offer,9);
    }
    LONG sign_id=InterlockedIncrement(&car35_event_count);
    int before_side=coach?car35_identity(coach,NULL,"before_sign"):-1;
    int side_ok=car35_eligible_side(role,before_side);
    int source_team=car35_get(coach,8,2);
    int source_role=car35_get(coach,16,0),previous_role=car35_get(coach,17,0),prestige=car35_get(coach,112,0);
    int promotion_floor=car35_emergency_sign_floor(manager,team,coach,offer,role,(U)__builtin_return_address(0)-game_base);
    int promotion_ok=car35_promotion_grade_at_floor(role,source_role,previous_role,prestige,promotion_floor);
    Car35Emergency *emergency=car35_emergency_context();
    if(promotion_floor>6 && promotion_ok && emergency) {
        emergency->consumed=1;
        log_line("CAR35_EMERGENCY_SIGN sign_id=%ld team_row=%lu coach_row=%lu grade=%d minimum_grade=%d",
            sign_id,(unsigned long)car35_row(team),(unsigned long)car35_row(coach),prestige,promotion_floor);
    }
    log_line("CAR35_SIGN_BEGIN sign_id=%ld coach_row=%lu team_row=%lu offer_row=%lu role=%d length=%d points=%d status=%d side=%d side_ok=%d caller_rva=0x%llx source_role=%d prev_role=%d prestige=%d promotion_ok=%d",
      sign_id,(unsigned long)car35_row(coach),(unsigned long)car35_row(team),(unsigned long)car35_row(offer),role,offered,points,car35_get(offer,11,0),before_side,side_ok,(unsigned long long)((U)__builtin_return_address(0)-game_base),source_role,previous_role,prestige,promotion_ok);
    /* Candidate/reconstruction guards own rejection. The native signer is
       void and must run to preserve an already-started native transaction. */
    if(mode==1 && !promotion_ok)
        log_line("FAULT,CAR35_PROMOTION_INVARIANT,coach_row=%lu,source_role=%d,prestige=%d,minimum_Cplus,stop_and_retain_checkpoint",
            (unsigned long)car35_row(coach),source_role,prestige);
    ((Sign)(game_base+0x085aeda0u))(manager,team_ref,offer_ref,flag);
    log_line("CAR35_SIGN_END sign_id=%ld coach_row=%lu role=%d team=%d length=%d remaining=%d salary=%d side=%d side_ok=%d",
      sign_id,(unsigned long)car35_row(coach),car35_get(coach,16,0),car35_get(coach,8,2),car35_get(coach,10,2),car35_get(coach,13,2),car35_get(coach,11,2),car35_get(coach,134,0),car35_eligible_side(car35_get(coach,16,0),car35_get(coach,134,0)));
    if(coach) {
        car35_record_field(coach,10,2,&length);car35_record_field(coach,13,2,&remaining);car35_record_field(coach,12,0,&status);
        if(mode==1 && (!side_ok || !car35_eligible_side(car35_get(coach,16,0),car35_get(coach,134,0)) ||
           car35_get(coach,16,0)!=role || (offered>0 && (length!=offered || remaining!=offered))))
            log_line("FAULT,CAR35_SIGN_INVARIANT,coach_row=%lu,role=%d,offered=%d,final=%d,remaining=%d,side=%d,stop_and_retain_checkpoint",
              (unsigned long)car35_row(coach),role,offered,length,remaining,car35_get(coach,134,0));
    }
    log_line("SIGN coach_row=%lu team_row=%lu role=%d supplied_length=%d supplied_points=%d final_length=%d final_remaining=%d status=%d match=%d",
        (unsigned long)car35_row_of(coach),(unsigned long)car35_row_of(team),role,offered,points,length,remaining,status,
        coach && offered>0 && offered==length && offered==remaining);
    log_line("CAR35_MOVE sign_id=%ld coach_row=%lu source_team=%d source_role=%d target_team=%d target_role=%d changed_job=%d",
      sign_id,(unsigned long)car35_row(coach),source_team,source_role,car35_get(coach,8,2),car35_get(coach,16,0),source_team!=car35_get(coach,8,2) || source_role!=car35_get(coach,16,0));
    car35_release_reference(coach);
}
