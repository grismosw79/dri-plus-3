/* Adapted installer pattern from accepted R3 into this independent overlay. */
/* Prepare relays before freezing threads; install all RM12 observer sites in
   one transaction. No allocation or logging while frozen. No R3 site is patched. */
typedef struct { const char *reason;DWORD code,subject;U address; } Car35InstallError;
static Car35InstallError car35_install_error;
static void car35_install_failure(const char *reason,DWORD code,DWORD subject,U address) {
 if(!car35_install_error.reason) {car35_install_error.reason=reason;car35_install_error.code=code;car35_install_error.subject=subject;car35_install_error.address=address;}
}
static void *car35_relay_pool;
static int rm_image_relays;
static const char *rm_padding_reason;
static DWORD rm_padding_subject;
enum { RM_PADDING_RVA=0xa3b4c00, RM_PADDING_BYTES=0x100,
       RM_CALL_PATCHES=HOOK_COUNT+CAR35_CALL_COUNT,
       RM_MAX_PATCHES=RM_CALL_PATCHES+CAR35_CALL_COUNT };
typedef struct { U address;unsigned size;unsigned char original[32],replacement[32]; } Car35Patch;
static Car35Patch car35_patch_plan[RM_MAX_PATCHES];
typedef struct { DWORD queries,free_regions,attempts,last_error;ULONGLONG start; } RelaySearch;
/* Loader alignment slack OUTSIDE declared virtual AND raw .text content.
   The old capture ends at a3b4c00; ctr begins a3b5000. Its intervening bytes
   were not captured. Require all 1024 bytes zero in the actual process and
   repeat this check under the thread freeze. Never scan for arbitrary caves,
   commit another allocation's reserved pages, or borrow the R3 relay pool. */
static int validate_image_padding(void) {
    rm_padding_reason="padding_header_read";rm_padding_subject=0;
    unsigned char header[0x600],bytes[RM_PADDING_BYTES];
    if(!local_read(game_base,header,sizeof(header)))return 0;
    rm_padding_reason="padding_header_layout";
    IMAGE_DOS_HEADER *dos=(void*)header;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew!=0x188)return 0;
    IMAGE_NT_HEADERS64 *nt=(void*)(header+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       nt->FileHeader.NumberOfSections!=17 || nt->FileHeader.SizeOfOptionalHeader!=240 ||
       nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       nt->OptionalHeader.SectionAlignment!=0x1000 || nt->OptionalHeader.SizeOfImage!=0x12003000 ||
       nt->OptionalHeader.SizeOfHeaders!=0x600 || nt->OptionalHeader.NumberOfRvaAndSizes!=16)return 0;
    IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);
    if((unsigned char*)(sections+17)>header+sizeof(header))return 0;
    rm_padding_reason="padding_text_bounds";
    if(memcmp(sections[0].Name,".text\0\0\0",8) || sections[0].VirtualAddress!=0x1000 ||
       sections[0].Misc.VirtualSize!=0xa3b3c00 || sections[0].SizeOfRawData!=0xa3b3c00 ||
       sections[0].Characteristics!=0x60000020 || sections[1].VirtualAddress!=0xa3b5000)return 0;
    for(unsigned i=0;i<17;i++) {
        rm_padding_reason="padding_section_overlap";rm_padding_subject=i;
        uint64_t start=sections[i].VirtualAddress;
        uint64_t size=sections[i].Misc.VirtualSize>sections[i].SizeOfRawData?sections[i].Misc.VirtualSize:sections[i].SizeOfRawData;
        if(start<RM_PADDING_RVA+RM_PADDING_BYTES && start+size>RM_PADDING_RVA)return 0;
    }
    for(unsigned i=0;i<16;i++) {
        rm_padding_reason="padding_directory_overlap";rm_padding_subject=i;
        if(i==IMAGE_DIRECTORY_ENTRY_SECURITY)continue; /* File offset, not RVA. */
        IMAGE_DATA_DIRECTORY d=nt->OptionalHeader.DataDirectory[i];
        if(d.Size && d.VirtualAddress<RM_PADDING_RVA+RM_PADDING_BYTES &&
           (uint64_t)d.VirtualAddress+d.Size>RM_PADDING_RVA)return 0;
    }
    rm_padding_reason="padding_mapping";rm_padding_subject=0;
    MEMORY_BASIC_INFORMATION m;U start=game_base+RM_PADDING_RVA;
    if(!VirtualQuery((void*)start,&m,sizeof(m)) || m.State!=MEM_COMMIT || m.Type!=MEM_IMAGE ||
       (U)m.AllocationBase!=game_base || (U)m.BaseAddress>start ||
       m.RegionSize<RM_PADDING_BYTES || (U)m.BaseAddress>UINTPTR_MAX-m.RegionSize ||
       (U)m.BaseAddress+m.RegionSize<start+RM_PADDING_BYTES ||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
       !(m.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return 0;
    rm_padding_reason="padding_bytes_unreadable";
    if(!local_read(start,bytes,sizeof(bytes)))return 0;
    rm_padding_reason="padding_not_zero";
    for(unsigned i=0;i<sizeof(bytes);i++)if(bytes[i]) {rm_padding_subject=i;return 0;}
    rm_padding_reason=NULL;
    return 1;
}
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
    log_line("INSTALL_STAGE relay_search_done pool=0x%llx queries=%lu free_regions=%lu attempts=%lu last_allocation_error=%lu elapsed_ms=%llu",
        (unsigned long long)(uintptr_t)p,s.queries,s.free_regions,s.attempts,s.last_error,(unsigned long long)(GetTickCount64()-s.start));
    if(p)return p;
    if(car35_install_error.reason)return NULL; /* Do not mask query failures. */
    U padding=game_base+RM_PADDING_RVA;
    if(padding<low || padding>high || CAR35_CALL_COUNT*16>RM_PADDING_BYTES || !validate_image_padding()) {
        car35_install_failure(rm_padding_reason?rm_padding_reason:"relay_padding_range",ERROR_INVALID_DATA,rm_padding_subject,padding);return NULL;
    }
    rm_image_relays=1;
    log_line("INSTALL_STAGE relay_fallback=image_alignment_padding rva=0x%x slots=%d bytes=%d full_zero_guard=%d; no writes yet",
        RM_PADDING_RVA,CAR35_CALL_COUNT,CAR35_CALL_COUNT*16,RM_PADDING_BYTES);
    return (void*)padding;
}
static __declspec(noinline) int car35_install_all(void) {
    int total=RM_CALL_PATCHES;
    U sites[CAR35_CALL_COUNT],pages[RM_MAX_PATCHES*2];DWORD protection[RM_MAX_PATCHES*2];
    HANDLE threads[512],snapshot=INVALID_HANDLE_VALUE;
    int page_count=0,count=0,suspended=0,writable=0,written=0,ok=0,rollback_ok=1;
    const char *phase="system_info";
    DWORD ignored=0;SYSTEM_INFO info;GetSystemInfo(&info);
    if(info.dwPageSize!=4096 || info.dwAllocationGranularity!=65536)goto cleanup;
    for(int j=0;j<CAR35_CALL_COUNT;j++)sites[j]=game_base+car35_calls[j].rva;
    phase="relay_prepare";car35_relay_pool=allocate_relay_pool(sites,&info);if(!car35_relay_pool)goto cleanup;
    if(rm_image_relays)total+=CAR35_CALL_COUNT;
    phase="patch_plan";
    for(int j=0;j<RM_CALL_PATCHES;j++) {
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
            unsigned char code[16]={0xff,0x25,0,0,0,0};memcpy(code+6,&c->entry,8);
            if(rm_image_relays) {
                Car35Patch *rp=&car35_patch_plan[RM_CALL_PATCHES+k];
                rp->address=relay;rp->size=16;memset(rp->original,0,16);memcpy(rp->replacement,code,16);
            } else memcpy((void*)relay,code,16);
        }
    }
    for(int j=0;j<total;j++) {
        Car35Patch *p=&car35_patch_plan[j];
        for(int edge=0;edge<2;edge++) {
            U page=(p->address+(edge?p->size-1:0))&~((U)info.dwPageSize-1);int found=0;
            for(int k=0;k<page_count;k++)if(pages[k]==page)found=1;
            if(!found)pages[page_count++]=page;
        }
    }
    phase="relay_protection";
    if(!rm_image_relays && (!VirtualProtect(car35_relay_pool,info.dwPageSize,PAGE_EXECUTE_READ,&ignored) ||
       !FlushInstructionCache(GetCurrentProcess(),car35_relay_pool,info.dwPageSize)))goto cleanup;
    phase="thread_enumeration";
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
    phase="thread_suspend";
    for(int j=0;j<count;j++) {
        if(SuspendThread(threads[j])==(DWORD)-1)goto cleanup;suspended++;
        CONTEXT c={0};c.ContextFlags=CONTEXT_CONTROL;
        if(!GetThreadContext(threads[j],&c))goto cleanup;
        for(int k=0;k<total;k++)if(c.Rip>=car35_patch_plan[k].address && c.Rip<car35_patch_plan[k].address+car35_patch_plan[k].size)goto cleanup;
    }
    phase="frozen_recheck";
    if(!verify_build())goto cleanup;
    if(rm_image_relays && !validate_image_padding()) {
        car35_install_failure(rm_padding_reason,ERROR_INVALID_DATA,rm_padding_subject,(U)car35_relay_pool);goto cleanup;
    }
    for(int j=0;j<total;j++)if(memcmp((void*)car35_patch_plan[j].address,car35_patch_plan[j].original,car35_patch_plan[j].size))goto cleanup;
    phase="page_writable";
    for(int j=0;j<page_count;j++) {
        if(!VirtualProtect((void*)pages[j],info.dwPageSize,PAGE_EXECUTE_READWRITE,&protection[j]))goto restore_protection;
        writable++;
    }
    phase="patch_write";
    /* Relays first, then call sites; other threads are still frozen. */
    for(int j=total-1;j>=0;j--) {
        volatile unsigned char *p=(volatile unsigned char*)car35_patch_plan[j].address;
        for(unsigned k=0;k<car35_patch_plan[j].size;k++)p[k]=car35_patch_plan[j].replacement[k];
    }
    written=1;ok=1;phase="cache_flush";
    for(int j=0;j<total;j++)if(!FlushInstructionCache(GetCurrentProcess(),(void*)car35_patch_plan[j].address,car35_patch_plan[j].size))ok=0;
restore_protection:
    for(int j=0;j<writable;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,protection[j],&ignored)) {
        ok=0;if(!written)rollback_ok=0;
    }
    if(!written && !rollback_ok) {
        /* No bytes changed, but restore any page protections left writable. */
        rollback_ok=1;
        for(int j=0;j<writable;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,protection[j],&ignored))rollback_ok=0;
    }
    if(written && !ok) {
        for(int j=0;j<page_count;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,PAGE_EXECUTE_READWRITE,&ignored))rollback_ok=0;
        if(rollback_ok)for(int j=0;j<total;j++) {
            volatile unsigned char *p=(volatile unsigned char*)car35_patch_plan[j].address;
            for(unsigned k=0;k<car35_patch_plan[j].size;k++)p[k]=car35_patch_plan[j].original[k];
            if(!FlushInstructionCache(GetCurrentProcess(),(void*)p,car35_patch_plan[j].size))rollback_ok=0;
        }
        for(int j=0;j<page_count;j++)if(!VirtualProtect((void*)pages[j],info.dwPageSize,protection[j],&ignored))rollback_ok=0;
    }
cleanup:
    if(!ok && !car35_install_error.reason)car35_install_failure(phase,GetLastError(),written,0);
    for(int j=suspended-1;j>=0;j--)if(ResumeThread(threads[j])==(DWORD)-1) {
        ok=0;car35_install_failure("thread_resume",GetLastError(),j,0);
    }
    for(int j=0;j<count;j++)CloseHandle(threads[j]);
    if(snapshot!=INVALID_HANDLE_VALUE)CloseHandle(snapshot);
    /* Relays remain allocated: never free code if an OS rollback failed. */
    if(!ok)log_line("INSTALL_FAILURE reason=%s windows_error=%lu subject=%lu address=0x%llx phase=%s written=%d rollback_ok=%d; restart game",
        car35_install_error.reason?car35_install_error.reason:"unknown",car35_install_error.code,car35_install_error.subject,
        (unsigned long long)car35_install_error.address,phase,written,rollback_ok);
    log_line("RM12_INSTALL success=%d total_sites=%d baseline_sites_patched=%d observer_sites=%d rollback_ok=%d relay_mode=%s padding_patches=%d",
        ok,RM_CALL_PATCHES,HOOK_COUNT,CAR35_CALL_COUNT,rollback_ok,rm_image_relays?"image_padding":"private_allocation",rm_image_relays?CAR35_CALL_COUNT:0);
    return ok;
}
