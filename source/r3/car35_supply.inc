/* R8 annual free-agent retention. Native retirement cleanup is untouched.
   Reuse the native per-coach remover only for unused, historyless entrants;
   do not pass a deliberately nonempty pool to native ClearCoachArray's
   empty-array assertion. All references are collected before any removal. */
static DWORD car35_generation_tls=TLS_OUT_OF_INDEXES;
typedef struct {
    void *generator,*manager,*pool;
    LONG event;
    int cleanup_done;
} Car35GenerationContext;
static int car35_same_record(void *a,void *b) {
    U da=0,db=0;uint32_t ra=0,rb=0;
    return a && b && (a==b ||
        (car35_local_read((U)a+0x10,&da,8) && car35_local_read((U)b+0x10,&db,8) &&
         car35_local_read((U)a+0x18,&ra,4) && car35_local_read((U)b+0x18,&rb,4) && da && da==db && ra==rb));
}
static int car35_unused_entrant(void *coach) {
    int status=-1,user=-1,years=-1;
    return car35_record_field(coach,12,0,&status) && status==7 &&
        car35_record_field(coach,0,1,&user) && user==0 &&
        car35_record_field(coach,135,2,&years) && years==0 &&
        car35_entrant_empty_results(coach,29,1) && car35_entrant_empty_results(coach,61,0);
}
static int car35_preserve_pool(void *manager,void *array) {
    Car35GenerationContext *c=TlsGetValue(car35_generation_tls);
    if(mode!=1 || !c || c->manager!=manager || !car35_same_record(c->pool,array))return 0;
    size_t n=0;void **rows=NULL;int valid=car35_contract_count(array,&n) && n<=4096;
    if(valid && n) {
        rows=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,n*sizeof(*rows));valid=rows!=NULL;
        for(size_t i=0;valid && i<n;i++) {
            rows[i]=car35_contract_at(array,i);if(!rows[i])valid=0;
            for(size_t j=0;valid && j<i;j++)if(car35_same_record(rows[i],rows[j]))valid=0;
        }
    }
    size_t removed=0,retained=0;
    if(valid) {
        for(size_t i=0;i<n;i++) {
            int discard=car35_unused_entrant(rows[i]);
            log_line("CAR35_POOL_DECISION event=%ld coach_row=%lu status=%d years=%d remove=%d reason=%s",
                c->event,(unsigned long)car35_row(rows[i]),car35_get(rows[i],12,0),car35_get(rows[i],135,2),discard,
                discard?"unused_historyless_native_cleanup":"preserve_experience_or_protected_record");
            if(discard) {
                ((void(*)(void*,void**))(game_base+0x08607700))(manager,&rows[i]);removed++;
            } else retained++;
        }
        c->cleanup_done=1;
    }
    if(rows) {
        for(size_t i=0;i<n;i++)car35_release_reference(rows[i]);
        HeapFree(GetProcessHeap(),0,rows);
    }
    /* An unreadable pool is protected, not bulk-deleted as recovery. The
       count hook reads actual surviving population, including failed native
       removals. Thus no guessed retained count can overfill the target. */
    log_line("CAR35_POOL_RETENTION event=%ld valid=%d before=%llu retained=%llu remove_calls=%llu policy=preserve_veterans_native_remove_unused",
        c->event,valid,(unsigned long long)n,(unsigned long long)retained,(unsigned long long)removed);
    return 1;
}
/* Replace mov edx,[r15+18]; call r9 as one guarded seven-byte window.
   Preserve the native descriptor/reader arguments and pass generator as
   argument five. The native caller uses our returned count consistently
   for reserve, generation and its vector-size assertion, including zero. */
__attribute__((naked,used)) static void car35_generation_count_adapter(void) {
    __asm__ volatile("mov 0x18(%r15), %edx\n\tsub $0x28, %rsp\n\t"
      "mov %r15, 0x20(%rsp)\n\tcall car35_generation_count\n\t"
      "add $0x28, %rsp\n\tret\n\t");
}
__declspec(dllexport) int car35_generation_count(void *data,uint32_t row,U field,U reader,void *generator) {
    int native=((int(*)(void*,uint32_t,U))reader)(data,row,field);
    Car35GenerationContext *c=TlsGetValue(car35_generation_tls);
    int result=native;size_t retained=0;int valid=0;
    if(mode==1 && c && c->generator==generator && c->pool && native>=0) {
        valid=car35_contract_count(c->pool,&retained) && retained<=4096;
        /* If the protected pool cannot be counted, do not create an
           unbounded extra batch. Existing staff and the pool are untouched. */
        result=valid && retained<(size_t)native?native-(int)retained:0;
        log_line("CAR35_REPLENISH event=%ld native_target=%d retained_actual=%llu valid=%d cleanup_done=%d new_count=%d over_target=%d settings_written=0",
            c->event,native,(unsigned long long)retained,valid,c->cleanup_done,result,valid && retained>(size_t)native);
    }
    return result;
}
