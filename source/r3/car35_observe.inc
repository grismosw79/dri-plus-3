/* Lifecycle boundaries, with R8's scoped retention and entrant profiles. */
static volatile LONG car35_generation_events;
static void car35_coach_trace(void *c,const char *event,int slot,int team) {
    if(!c)return;
    int history=-1;car35_record_reference_present(c,29,&history);
    log_line("CAR35_COACH event=%s coach_row=%lu slot=%d team=%d status=%d role=%d previous_role=%d side=%d age=%d years=%d career=%d prestige=%d prestige_score=%d rank=%d level=%d experience=%d tenure=%d salary=%d length=%d remaining=%d",
        event,(unsigned long)car35_row(c),slot,team,car35_get(c,12,0),car35_get(c,16,0),car35_get(c,17,0),car35_get(c,134,0),car35_get(c,1,2),car35_get(c,135,2),history,
        car35_get(c,112,0),car35_get(c,113,2),car35_get(c,120,2),car35_get(c,14,2),car35_get(c,41,2),car35_get(c,19,2),car35_get(c,11,2),car35_get(c,10,2),car35_get(c,13,2));
    log_line("CAR35_TRAITS event=%s coach_row=%lu backstory=%d specialty=%d offense=%d defense=%d qb=%d rb=%d wr=%d ol=%d dl=%d lb=%d db=%d safety=%d rating=%d",
        event,(unsigned long)car35_row(c),car35_get(c,110,0),car35_get(c,106,0),car35_get(c,93,2),car35_get(c,80,2),car35_get(c,99,2),car35_get(c,101,2),car35_get(c,109,2),car35_get(c,96,2),car35_get(c,85,2),car35_get(c,91,2),car35_get(c,79,2),car35_get(c,105,2),car35_get(c,100,2));
    void *off=car35_retain_reference(c,51),*def=car35_retain_reference(c,39),*talents=car35_retain_reference(c,9);
    log_line("CAR35_PROFILE event=%s coach_row=%lu points=%d archetype=%d offensive_scheme_row=%lu defensive_scheme_row=%lu talent_tree_row=%lu generation_event=%ld",
      event,(unsigned long)car35_row(c),car35_get(c,111,2),car35_get(c,121,0),(unsigned long)car35_row(off),(unsigned long)car35_row(def),(unsigned long)car35_row(talents),car35_generation_events);
    car35_release_reference(off);car35_release_reference(def);car35_release_reference(talents);
}
static void car35_pool_trace(void *franchise,unsigned field,const char *event) {
    void *a=car35_contract_array(franchise,field);size_t n=0;
    int valid=car35_contract_count(a,&n);
    log_line("CAR35_POOL event=%s field=%u count=%llu valid=%d",event,field,(unsigned long long)n,valid);
    for(size_t i=0;valid && i<n;i++) {
        void *coach=car35_contract_at(a,i);car35_coach_trace(coach,event,-1,-1);car35_release_reference(coach);
    }
    car35_release_reference(a);
}
static void car35_franchise_trace(void *f,const char *event,int normalize) {
    if(!f)return;
    car35_pool_trace(f,12,event);car35_pool_trace(f,41,event);
    void *teams=car35_contract_array(f,45);size_t n=0;
    int valid=car35_contract_count(teams,&n),filled=0;
    for(size_t i=0;valid && i<n;i++) {
        void *team=car35_contract_at(teams,i);unsigned fields[3]={111,183,58};
        int index=car35_get(team,383,2);
        for(int r=0;team && r<3;r++) {
            void *coach=car35_retain_reference(team,fields[r]);
            if(coach) {
                filled++;
                if(normalize)car35_identity(coach,team,event);
                car35_coach_trace(coach,event,r,index);
            } else log_line("CAR35_VACANCY event=%s team=%d role=%d",event,index,r);
            car35_release_reference(coach);
        }
        car35_release_reference(team);
    }
    log_line("CAR35_STAFFING event=%s teams=%llu filled=%d valid=%d",event,(unsigned long long)n,filled,valid);
    car35_release_reference(teams);
}
static SRWLOCK car35_observe_lock=SRWLOCK_INIT;
static void *car35_last_module;
static int car35_last_year=-1,car35_last_week=-1,car35_last_type=-1;
static void car35_module_trace(void *m) {
    void *season=NULL;car35_local_read((U)m+0x138,&season,8);
    int year=car35_get(season,6,2),week=car35_get(season,4,2),type=car35_get(season,5,0),changed;
    AcquireSRWLockExclusive(&car35_observe_lock);
    changed=car35_last_module!=m || year!=car35_last_year || week!=car35_last_week || type!=car35_last_type;
    if(changed) {car35_last_module=m;car35_last_year=year;car35_last_week=week;car35_last_type=type;}
    ReleaseSRWLockExclusive(&car35_observe_lock);
    if(!changed)return;
    log_line("CAR35_PHASE year=%d week=%d type=%d stage=%d active=%d",year,week,type,car35_get(season,3,0),car35_get(season,12,1));
    void *tuning=NULL;car35_local_read((U)m+0x148,&tuning,8);
    log_line("CAR35_HC_TUNING fired_min=%d fired_max=%d hc_poach_threshold=%d coord_poach_threshold=%d tenure_comparison=strict_greater_than loaded_values=1",
      car35_get(tuning,47,2),car35_get(tuning,46,2),car35_get(tuning,95,2),car35_get(tuning,94,2));
    log_line("CAR35_LOADED_TUNING row=%lu extension_bonus=%d candidate_cutoff=%d max_candidates=%d max_offers=%d max_rating=%d min_rating=%d coord_poach=%d hc_poach=%d hc_only_level=%d interest_offer=%d job_interest=%d recent_years=%d route=compare_to_manifest_not_assumed",
      (unsigned long)car35_row(tuning),car35_get(tuning,38,2),car35_get(tuning,2,2),car35_get(tuning,77,2),car35_get(tuning,79,2),car35_get(tuning,75,2),car35_get(tuning,86,2),car35_get(tuning,94,2),car35_get(tuning,95,2),car35_get(tuning,53,2),car35_get(tuning,55,2),car35_get(tuning,57,2),car35_get(tuning,121,2));
    void *f=car35_retain_reference(m,20);car35_franchise_trace(f,"carousel_phase",1);car35_release_reference(f);
}
static void car35_generation(void *m) {
    void *manager=NULL;car35_local_read((U)m+0x60,&manager,8);
    void *f=manager?car35_retain_reference(manager,2):NULL;
    LONG event=InterlockedIncrement(&car35_generation_events);
    void *pool=car35_contract_array(f,12);
    Car35GenerationContext *context=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*context));
    if(context) {context->generator=m;context->manager=manager;context->pool=pool;context->event=event;}
    void *previous=TlsGetValue(car35_generation_tls);
    int stored=context && TlsSetValue(car35_generation_tls,context)!=0;
    log_line("CAR35_GENERATION boundary=begin event=%ld configured_batch=%d observation_only=%d retention_context=%d",event,car35_get(m,3,2),mode!=1,stored);
    car35_franchise_trace(f,"generation_before",0);
    if(mode!=1 || (stored && manager && pool))((void(*)(void*))(game_base+0x0817a9b0))(m);
    else log_line("CAR35_SUPPLY_GUARD event=%ld reason=missing_safe_generation_context native_generation_skipped=1",event);
    int restored=!stored || TlsSetValue(car35_generation_tls,previous)!=0;
    if(context) {
        if(restored)HeapFree(GetProcessHeap(),0,context);
        else {
            /* Keep a disabled object alive if TLS restoration fails. */
            context->generator=NULL;context->manager=NULL;context->pool=NULL;
            log_line("CAR35_SUPPLY_GUARD event=%ld reason=TLS_restore_failed_context_disabled restart_required=1",event);
        }
    }
    car35_franchise_trace(f,"generation_after",0);
    log_line("CAR35_GENERATION boundary=end event=%ld observation_only=%d",event,mode!=1);
    car35_release_reference(pool);car35_release_reference(f);
}
static void car35_cleanup_pool(void *m,void **array) {
    size_t n=0;void *a=array?*array:NULL;int valid=car35_contract_count(a,&n);
    log_line("CAR35_CLEANUP boundary=begin count=%llu valid=%d caller_rva=0x%llx policy=scope_checked_below",(unsigned long long)n,valid,(unsigned long long)((U)__builtin_return_address(0)-game_base));
    for(size_t i=0;valid && i<n;i++) {
        void *c=car35_contract_at(a,i);car35_coach_trace(c,"cleanup_scheduled",-1,-1);car35_release_reference(c);
    }
    int preserved=car35_preserve_pool(m,a);
    if(!preserved)((void(*)(void*,void**))(game_base+0x08606e80))(m,array);
    size_t after=0;void *remaining=array?*array:NULL;int after_valid=car35_contract_count(remaining,&after);
    log_line("CAR35_CLEANUP boundary=end count=%llu valid=%d before_count=%llu generation_event=%ld observation_only=%d retained_policy=%d",
      (unsigned long long)after,after_valid,(unsigned long long)n,car35_generation_events,!preserved,preserved);
    for(size_t i=0;after_valid && i<after;i++) {
        void *c=car35_contract_at(remaining,i);car35_coach_trace(c,"cleanup_remaining",-1,-1);car35_release_reference(c);
    }
}

/* This vector is the generator's explicit new batch, not a set difference of
   row numbers (deleted rows can be reused). Observe native initialization. */
static void car35_generated_batch(void *m,void **template_ref,void *vector) {
    U begin=0,end=0;int valid=car35_local_read((U)vector,&begin,8) && car35_local_read((U)vector+8,&end,8) &&
      end>=begin && (end-begin)%8==0 && (end-begin)/8<=4096;
    log_line("CAR35_GENERATED_BATCH boundary=before_initialize event=%ld count=%llu valid=%d",car35_generation_events,(unsigned long long)(valid?(end-begin)/8:0),valid);
    for(U p=begin;valid && p<end;p+=8) {
        void *c=NULL;if(car35_local_read(p,&c,8))car35_coach_trace(c,"generated_raw",-1,-1);
    }
    ((void(*)(void*,void**,void*))(game_base+0x08177450))(m,template_ref,vector);
    /* Re-read vector bounds; do not rely on borrowed pointers across native. */
    valid=car35_local_read((U)vector,&begin,8) && car35_local_read((U)vector+8,&end,8) &&
      end>=begin && (end-begin)%8==0 && (end-begin)/8<=4096;
    log_line("CAR35_GENERATED_BATCH boundary=after_initialize event=%ld count=%llu valid=%d",car35_generation_events,(unsigned long long)(valid?(end-begin)/8:0),valid);
    for(U p=begin;valid && p<end;p+=8) {
        void *c=NULL;if(car35_local_read(p,&c,8)) {
            car35_entrant_profile(c,1);car35_coach_trace(c,"generated_initialized",-1,-1);
        }
    }
}
/* All five direct CreateJobOpening callers use this eight-argument ABI.
   Native code alone creates the record and assigns the vacancy reason. */
static void **car35_opening(void *m,void **out,int reason,int role,void **previous,void **team_ref,unsigned char emergent,unsigned char flag) {
    LONG event=InterlockedIncrement(&car35_event_count);
    void *team=team_ref?*team_ref:NULL,*old=previous?*previous:NULL;
    log_line("CAR35_OPENING_BEGIN event=%ld team_row=%lu team=%d role=%d reason=%d previous_coach_row=%lu previous_status=%d emergent=%d flag=%d caller_rva=0x%llx",
      event,(unsigned long)car35_row(team),car35_get(team,383,2),role,reason,(unsigned long)car35_row(old),car35_get(old,12,0),emergent,flag,(unsigned long long)((U)__builtin_return_address(0)-game_base));
    void **result=((void**(*)(void*,void**,int,int,void**,void**,unsigned char,unsigned char))(game_base+0x08327050))(m,out,reason,role,previous,team_ref,emergent,flag);
    void *job=out?*out:NULL;
    log_line("CAR35_OPENING_END event=%ld job_row=%lu returned=%d role=%d reason=%d emergent=%d filled=%d",
      event,(unsigned long)car35_row(job),job!=NULL,car35_get(job,6,0),car35_get(job,8,0),car35_get(job,5,1),car35_get(job,1,1));
    return result;
}
