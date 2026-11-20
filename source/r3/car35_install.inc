/* Prepare relays before freezing threads; the caller installs all R4 and
   carousel sites in one transaction. No allocation or logging while frozen. */
typedef struct { const char *reason;DWORD code,subject;U address; } Car35InstallError;
static Car35InstallError car35_install_error;
static void car35_install_failure(const char *reason,DWORD code,DWORD subject,U address) {
 if(!car35_install_error.reason) {car35_install_error.reason=reason;car35_install_error.code=code;car35_install_error.subject=subject;car35_install_error.address=address;}
}
static void *car35_relay_pool;
typedef struct { U address;unsigned size;unsigned char original[32],replacement[32]; } Car35Patch;
static Car35Patch car35_patch_plan[HOOK_COUNT+CAR35_CALL_COUNT];
typedef struct { DWORD queries,free_regions,attempts,last_error;ULONGLONG start; } RelaySearch;
static void *search_relay_range(uintptr_t low,uintptr_t high,const SYSTEM_INFO *info,RelaySearch *s) {
    uintptr_t granularity=info->dwAllocationGranularity;
    uintptr_t cursor=(low+granularity-1)&~(granularity-1);
    while(cursor<=high) {
        if(s->queries>=65536 || GetTickCount64()-s->start>=5000) {
            car35_install_failure("relay_search_budget",ERROR_TIMEOUT,s->queries,cursor);return NULL;
        }
        MEMORY_BASIC_INFORMATION memory;s->queries++;
        if(!VirtualQuery((void*)cursor,&memory,sizeof(memory))) {
            s->last_error=GetLastError();
            car35_install_failure("relay_virtual_query",s->last_error,s->queries,cursor);return NULL;
        }
        uintptr_t base=(uintptr_t)memory.BaseAddress;
        if(memory.RegionSize==0 || base>UINTPTR_MAX-memory.RegionSize || base+memory.RegionSize<=cursor) {
            car35_install_failure("relay_invalid_region",ERROR_INVALID_DATA,s->queries,cursor);return NULL;
        }
        uintptr_t end=base+memory.RegionSize;
        if(memory.State==MEM_FREE) {
            s->free_regions++;
            if(end-cursor>=info->dwPageSize) {
                s->attempts++;
                void *p=VirtualAlloc((void*)cursor,info->dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
                if(p)return p;
                s->last_error=GetLastError();
                /* Another thread can win this address. Continue within the free
                   range; the global time/query budget also bounds contention. */
                if(cursor>UINTPTR_MAX-granularity)break;
                cursor+=granularity;continue;
            }
        }
        /* Skip an occupied/reserved region in one query, rather than querying
           every 64 KiB address inside the same allocation. */
        if(end>UINTPTR_MAX-(granularity-1))break;
        cursor=(end+granularity-1)&~(granularity-1);
    }
    return NULL;
}
static void *allocate_relay_pool(const uintptr_t sites[CAR35_CALL_COUNT],const SYSTEM_INFO *info) {
    uintptr_t low=(uintptr_t)info->lpMinimumApplicationAddress;
    uintptr_t high=(uintptr_t)info->lpMaximumApplicationAddress-info->dwPageSize+1;
    if(CAR35_CALL_COUNT*16>info->dwPageSize) { car35_install_failure("relay_pool_too_small",ERROR_INSUFFICIENT_BUFFER,0,0);return NULL; }
    for(int j=0;j<CAR35_CALL_COUNT;j++) {
        int64_t anchor=(int64_t)(sites[j]+5)-(int64_t)(j*16);
        int64_t lo=anchor+INT32_MIN,hi=anchor+INT32_MAX;
        if(lo>0 && (uintptr_t)lo>low)low=(uintptr_t)lo;
        if(hi<0) { car35_install_failure("relay_no_common_range",ERROR_INVALID_ADDRESS,j,sites[j]);return NULL; }
        if((uintptr_t)hi<high)high=(uintptr_t)hi;
    }
    if(low>high) { car35_install_failure("relay_no_common_range",ERROR_INVALID_ADDRESS,0,low);return NULL; }
    uintptr_t preferred=sites[0]&~((uintptr_t)info->dwAllocationGranularity-1);
    if(preferred<low || preferred>high)preferred=low;
    RelaySearch s={0};s.start=GetTickCount64();
    log_line("INSTALL_STAGE relay_search low=0x%llx high=0x%llx shared_slots=%d budget_ms=5000",
        (unsigned long long)low,(unsigned long long)high,CAR35_CALL_COUNT);
    void *p=search_relay_range(preferred,high,info,&s);
    if(!p && !car35_install_error.reason && preferred>low)p=search_relay_range(low,preferred-1,info,&s);
    if(!p && !car35_install_error.reason)car35_install_failure("relay_no_space",s.last_error?s.last_error:ERROR_NOT_ENOUGH_MEMORY,s.attempts,low);
    log_line("INSTALL_STAGE relay_search_done pool=0x%llx queries=%lu free_regions=%lu attempts=%lu last_allocation_error=%lu elapsed_ms=%llu",
        (unsigned long long)(uintptr_t)p,s.queries,s.free_regions,s.attempts,s.last_error,(unsigned long long)(GetTickCount64()-s.start));
    return p;
}
static int car35_install_all(void) {
    enum { TOTAL=HOOK_COUNT+CAR35_CALL_COUNT };
    U sites[CAR35_CALL_COUNT],pages[TOTAL*2];DWORD protection[TOTAL*2];
    HANDLE threads[512],snapshot=INVALID_HANDLE_VALUE;
    int page_count=0,count=0,suspended=0,writable=0,written=0,ok=0,rollback_ok=1;
    DWORD ignored=0;SYSTEM_INFO info;GetSystemInfo(&info);
    if(info.dwPageSize<16*CAR35_CALL_COUNT)return 0;
    for(int j=0;j<CAR35_CALL_COUNT;j++)sites[j]=game_base+car35_calls[j].rva;
    car35_relay_pool=allocate_relay_pool(sites,&info);if(!car35_relay_pool)return 0;
    for(int j=0;j<TOTAL;j++) {
        Car35Patch *p=&car35_patch_plan[j];
        if(j<HOOK_COUNT) {
            p->address=game_base+hooks[j].rva;p->size=hooks[j].size;
            memcpy(p->original,hooks[j].expected,p->size);memset(p->replacement,0x90,p->size);
            p->replacement[0]=0xff;p->replacement[1]=0x25;memset(p->replacement+2,0,4);
            U target=(U)hooks[j].entry;memcpy(p->replacement+6,&target,8);
        } else {
            int k=j-HOOK_COUNT;const Car35Call *c=&car35_calls[k];
            U relay=(U)car35_relay_pool+k*16;int64_t delta=(int64_t)relay-(int64_t)(sites[k]+5);
            if(delta<INT32_MIN || delta>INT32_MAX)goto cleanup;
            p->address=sites[k];p->size=c->size;memcpy(p->original,c->expected,p->size);
            memset(p->replacement,0x90,p->size);p->replacement[0]=0xe8;
            int32_t displacement=(int32_t)delta;memcpy(p->replacement+1,&displacement,4);
            unsigned char code[14]={0xff,0x25,0,0,0,0};memcpy(code+6,&c->entry,8);memcpy((void*)relay,code,14);
        }
        for(int edge=0;edge<2;edge++) {
            U page=(p->address+(edge?p->size-1:0))&~((U)info.dwPageSize-1);int found=0;
            for(int k=0;k<page_count;k++)if(pages[k]==page)found=1;
            if(!found)pages[page_count++]=page;
        }
    }
    if(!VirtualProtect(car35_relay_pool,info.dwPageSize,PAGE_EXECUTE_READ,&ignored) ||
       !FlushInstructionCache(GetCurrentProcess(),car35_relay_pool,info.dwPageSize))goto cleanup;
    snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(snapshot==INVALID_HANDLE_VALUE)goto cleanup;
    THREADENTRY32 t={0};t.dwSize=sizeof(t);
    DWORD pid=GetCurrentProcessId(),self=GetCurrentThreadId();
    if(!Thread32First(snapshot,&t))goto cleanup;
    do {
        if(t.th32OwnerProcessID!=pid || t.th32ThreadID==self)continue;
        if(count>=512)goto cleanup;
        HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,t.th32ThreadID);
        if(!h)goto cleanup;threads[count++]=h;
    }while(Thread32Next(snapshot,&t));
    if(GetLastError()!=ERROR_NO_MORE_FILES || !count)goto cleanup;
    CloseHandle(snapshot);snapshot=INVALID_HANDLE_VALUE;
    for(int j=0;j<count;j++) {
        if(SuspendThread(threads[j])==(DWORD)-1)goto cleanup;suspended++;
        CONTEXT c={0};c.ContextFlags=CONTEXT_CONTROL;
        if(!GetThreadContext(threads[j],&c))goto cleanup;
        for(int k=0;k<TOTAL;k++)if(c.Rip>=car35_patch_plan[k].address && c.Rip<car35_patch_plan[k].address+car35_patch_plan[k].size)goto cleanup;
    }
    if(!verify_build())goto cleanup;
    for(int j=0;j<page_count;j++) {
        if(!VirtualProtect((void*)pages[j],info.dwPageSize,PAGE_EXECUTE_READWRITE,&protection[j]))goto restore_protection;
        writable++;
    }
    for(int j=0;j<TOTAL;j++) {
        volatile unsigned char *p=(volatile unsigned char*)car35_patch_plan[j].address;
        for(unsigned k=0;k<car35_patch_plan[j].size;k++)p[k]=car35_patch_plan[j].replacement[k];
    }
    written=1;ok=1;
    for(int j=0;j<TOTAL;j++)if(!FlushInstructionCache(GetCurrentProcess(),(void*)car35_patch_plan[j].address,car35_patch_plan[j].size))ok=0;
restore_protection:
    for(int j=0;j<writable;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,protection[j],&ignored))ok=0;
    if(written && !ok) {
        for(int j=0;j<page_count;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,PAGE_EXECUTE_READWRITE,&ignored))rollback_ok=0;
        if(rollback_ok)for(int j=0;j<TOTAL;j++) {
            volatile unsigned char *p=(volatile unsigned char*)car35_patch_plan[j].address;
            for(unsigned k=0;k<car35_patch_plan[j].size;k++)p[k]=car35_patch_plan[j].original[k];
            if(!FlushInstructionCache(GetCurrentProcess(),(void*)p,car35_patch_plan[j].size))rollback_ok=0;
        }
        for(int j=0;j<page_count;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,protection[j],&ignored))rollback_ok=0;
    }
cleanup:
    for(int j=suspended-1;j>=0;j--)if(ResumeThread(threads[j])==(DWORD)-1)ok=0;
    for(int j=0;j<count;j++)CloseHandle(threads[j]);
    if(snapshot!=INVALID_HANDLE_VALUE)CloseHandle(snapshot);
    /* Relays remain allocated: never free code if an OS rollback failed. */
    log_line("CAR35_INSTALL success=%d total_sites=%d retained_R4_sites=%d carousel_sites=%d rollback_ok=%d",ok,TOTAL,HOOK_COUNT,CAR35_CALL_COUNT,rollback_ok);
    return ok;
}
