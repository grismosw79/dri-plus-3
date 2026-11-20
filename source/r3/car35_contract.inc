/* Selected offers remain owned by their existing JobOpening.ContractOfferList.
   The global outstanding list and native offer counters still follow native
   cleanup. All five call sites are version-locked; no sidecar or new field. */
typedef struct { void *team,*coach; int role,points,user,team_user; } car35_ContractJob;

static void car35_contract_job_release(car35_ContractJob *j) {
    car35_release_reference(j->team);car35_release_reference(j->coach);memset(j,0,sizeof(*j));
}
static int car35_contract_job(void *job,car35_ContractJob *j) {
    int filled=0;memset(j,0,sizeof(*j));
    if(!job || !car35_record_field(job,1,1,&filled) || !filled ||
       !car35_record_field(job,6,0,&j->role) || j->role<0 || j->role>2 ||
       !car35_record_field(job,2,2,&j->points))return 0;
    j->team=car35_retain_reference(job,10);j->coach=car35_retain_reference(job,9);
    if(!j->team || !j->coach || !car35_record_field(j->coach,0,1,&j->user) ||
       !car35_record_reference_present(j->team,407,&j->team_user)) {
        car35_contract_job_release(j);return 0;
    }
    return 1;
}
static int car35_contract_offer_matches(void *offer,const car35_ContractJob *j,int wanted_status) {
    int role=-1,points=-1,length=-1,status=-1;void *team=NULL,*coach=NULL;int match=0;
    if(!offer || !car35_record_field(offer,3,0,&role) || role!=j->role ||
       !car35_record_field(offer,7,2,&points) || points!=j->points ||
       !car35_record_field(offer,6,2,&length) || length<1 || length>20 ||
       !car35_record_field(offer,11,0,&status) ||
       (wanted_status>=0?status!=wanted_status:(status!=0 && status!=2)) ||
       /* A human must already have accepted. CPU selected jobs may still
          carry native Pending (2), as in the tested CAR3T2 lifecycle. */
       (j->user && status!=0))return 0;
    team=car35_retain_reference(offer,12);coach=car35_retain_reference(offer,9);
    match=team==j->team && coach==j->coach;
    car35_release_reference(team);car35_release_reference(coach);return match;
}
static void *car35_contract_array(void *record,unsigned field) {
    void *base=car35_retain_reference(record,field),*array=NULL;uintptr_t vt,cast;
    if(base && car35_local_read((uintptr_t)base,&vt,8) && car35_local_read(vt+0x28,&cast,8) && car35_code_pointer(cast))
        ((void*(*)(void*,void**,uint32_t))cast)(base,&array,0x41525259u);
    car35_release_reference(base);return array;
}
static int car35_contract_count(void *array,size_t *count) {
    if(!array)return 0;
    uint64_t n=((uint64_t(*)(void*))(game_base+0x037f4130u))(array);
    if(n>4096)return 0;*count=(size_t)n;return 1;
}
static void *car35_contract_at(void *array,size_t index) {
    uintptr_t data,vt,reader;uint32_t row;void *value=NULL;
    if(!array || !car35_local_read((uintptr_t)array+0x10,&data,8) || !data ||
       !car35_local_read((uintptr_t)array+0x18,&row,4) || !car35_local_read(data,&vt,8) ||
       !car35_local_read(vt+0x128,&reader,8) || !car35_code_pointer(reader))return NULL;
    ((void*(*)(void*,void**,uint32_t,size_t))reader)((void*)data,&value,row,index);
    return value;
}
static void car35_offer_trace(void *offer,void *job,const char *event) {
    if(!offer)return;
    void *coach=car35_retain_reference(offer,9),*team=car35_retain_reference(offer,12);
    log_line("CAR35_OFFER event=%s job_row=%lu offer_row=%lu coach_row=%lu team_row=%lu role=%d status=%d length=%d points=%d base_interest=%d adjusted_interest=%d team_interest=%d",
      event,(unsigned long)car35_row_of(job),(unsigned long)car35_row_of(offer),(unsigned long)car35_row_of(coach),(unsigned long)car35_row_of(team),
      car35_get(offer,3,0),car35_get(offer,11,0),car35_get(offer,6,2),car35_get(offer,7,2),car35_get(offer,1,2),car35_get(offer,0,2),car35_get(offer,13,2));
    car35_release_reference(coach);car35_release_reference(team);
}
/* Native finalizer 08346DA6..08346DF5 uses enum setter +148 and dirty
   notification 08055870 for Offer.Status. 0=Accepted, 2=Pending. */
static int car35_contract_accept(void *offer) {
    uintptr_t vt,getter,type,field,data,dv,writer;uint32_t row;int16_t count;
    if(!offer || !car35_local_read((uintptr_t)offer,&vt,8) || !car35_local_read(vt+0x38,&getter,8) || !car35_code_pointer(getter))return 0;
    type=((uintptr_t(*)(void*))getter)(offer);
    if(!type || !car35_local_read(type+0xd8,&count,2) || count<=11)return 0;
    field=((uintptr_t(*)(void*,unsigned))(game_base+0x08055d60u))((void*)type,11);
    if(field==0xffffffffu || !car35_local_read((uintptr_t)offer+0x10,&data,8) || !data ||
       !car35_local_read((uintptr_t)offer+0x18,&row,4) || !car35_local_read(data,&dv,8) ||
       !car35_local_read(dv+0x148,&writer,8) || !car35_code_pointer(writer))return 0;
    ((void(*)(void*,uint32_t,void*,int))writer)((void*)data,row,(void*)field,0);
    ((void(*)(void*))(game_base+0x080595e0u))((void*)data);
    int status=-1;return car35_record_field(offer,11,0,&status) && status==0;
}
/* Return exactly one matching selected offer, owning its normal native ref. */
static void *car35_contract_selected(void *array,const car35_ContractJob *j,int status,size_t *selected) {
    size_t n=0;void *found=NULL;int matches=0;
    if(!car35_contract_count(array,&n))return NULL;
    for(size_t i=0;i<n;i++) {
        void *offer=car35_contract_at(array,i);
        if(!offer) { car35_release_reference(found);return NULL; }
        if(car35_contract_offer_matches(offer,j,status)) {
            ++matches;
            if(matches==1) { found=offer;*selected=i;offer=NULL; }
        }
        car35_release_reference(offer);
    }
    if(matches!=1) { car35_release_reference(found);return NULL; }
    return found;
}

/* At 08352E3A, native RBP is job_ref and R14 is StaffHiringEval. */
__attribute__((naked,used)) static void car35_contract_remove_adapter(void) {
    __asm__ volatile("mov %rbp, %r8\n\tmov %r14, %r9\n\tjmp car35_contract_remove_hook\n\t");
}
__declspec(dllexport) size_t car35_contract_remove_hook(void *array,size_t index,void **job_ref,void *module) {
    typedef size_t(*Remove)(void*,size_t);(void)module;
    void *job=NULL,*offer=NULL;car35_ContractJob j;size_t selected=0;
    car35_local_read((U)job_ref,&job,8);
    size_t trace_count=0;
    if(car35_contract_count(array,&trace_count) && index<trace_count) {
        void *traced=car35_contract_at(array,index);car35_offer_trace(traced,job,"local_remove");car35_release_reference(traced);
    }
    if(mode==1 && car35_local_read((uintptr_t)job_ref,&job,8) && car35_contract_job(job,&j)) {
        offer=car35_contract_selected(array,&j,-1,&selected);
        if(offer && selected==index && car35_contract_accept(offer)) {
            int length=-1;car35_record_field(offer,6,2,&length);
            log_line("CAR35_ACCEPTED source=remove offer_row=%lu points=%d job_row=%lu coach_row=%lu team_row=%lu role=%d length=%d user=%d team_user=%d",
                (unsigned long)car35_row_of(offer),j.points,
                (unsigned long)car35_row_of(job),(unsigned long)car35_row_of(j.coach),(unsigned long)car35_row_of(j.team),j.role,length,j.user,j.team_user);
            car35_release_reference(offer);car35_contract_job_release(&j);return index;
        }
        if(!offer)log_line("CONTRACT_HOLD_MISS source=remove job_row=%lu reason=missing_ambiguous_or_unreadable_selected_offer",(unsigned long)car35_row_of(job));
        car35_release_reference(offer);car35_contract_job_release(&j);
    }
    return ((Remove)(game_base+0x08052780u))(array,index);
}

/* At 0832A3C8, native R14 is job, RSI is StaffHiringEval. Global clear
   already ran. Clear other local offers with the original array operation. */
__attribute__((naked,used)) static void car35_contract_clear_adapter(void) {
    __asm__ volatile("mov %r14, %rdx\n\tmov %rsi, %r8\n\tjmp car35_contract_clear_hook\n\t");
}
__declspec(dllexport) void car35_contract_clear_hook(void *array,void *job,void *module) {
    typedef void(*Clear)(void*);car35_ContractJob j;size_t selected=0,n=0;void *offer=NULL;
    if(mode==1 && !faulted)t36_pre_contract(module,1);
    if(car35_contract_count(array,&n))for(size_t i=0;i<n;i++) {
        void *traced=car35_contract_at(array,i);car35_offer_trace(traced,job,"local_round_clear");car35_release_reference(traced);
    }
    if(mode==1 && car35_contract_job(job,&j)) {
        offer=car35_contract_selected(array,&j,-1,&selected);
        if(offer && car35_contract_count(array,&n) && car35_contract_accept(offer)) {
            for(size_t i=n;i>0;i--)if(i-1!=selected)
                ((size_t(*)(void*,size_t))(game_base+0x08052780u))(array,i-1);
            int length=-1;car35_record_field(offer,6,2,&length);
            if(!faulted && j.role==0)r2_hc_hire(module,j.coach,j.team,j.role,length);
            log_line("CAR35_ACCEPTED source=round_clear offer_row=%lu points=%d job_row=%lu coach_row=%lu team_row=%lu role=%d length=%d removed=%llu user=%d team_user=%d",
                (unsigned long)car35_row_of(offer),j.points,
                (unsigned long)car35_row_of(job),(unsigned long)car35_row_of(j.coach),(unsigned long)car35_row_of(j.team),j.role,length,(unsigned long long)(n-1),j.user,j.team_user);
            car35_release_reference(offer);car35_contract_job_release(&j);return;
        }
        log_line("CONTRACT_HOLD_MISS source=round_clear job_row=%lu reason=missing_ambiguous_or_unreadable_selected_offer",(unsigned long)car35_row_of(job));
        car35_release_reference(offer);car35_contract_job_release(&j);
    }
    ((Clear)(game_base+0x08047250u))(array);
}

/* Only native budget recomputation 08324177 is redirected. Held Accepted
   offers have already left the global pending list and reserve no points. */
__declspec(dllexport) int car35_contract_max_hook(void *job_wrapper) {
    typedef int(*Max)(void*);void *job=NULL,*array=NULL;car35_ContractJob j;int result=0,ignored=0,valid=1;size_t n=0;
    if(mode!=1 || !car35_local_read((uintptr_t)job_wrapper+8,&job,8) || !car35_contract_job(job,&j))
        return ((Max)(game_base+0x0843e7b0u))(job_wrapper);
    array=car35_contract_array(job,0);
    if(!car35_contract_count(array,&n))valid=0;
    for(size_t i=0;valid && i<n;i++) {
        void *offer=car35_contract_at(array,i);int points=0;
        if(!offer || !car35_record_field(offer,7,2,&points))valid=0;
        else if(car35_contract_offer_matches(offer,&j,0))++ignored;
        else if(points>result)result=points;
        car35_release_reference(offer);
    }
    car35_release_reference(array);car35_contract_job_release(&j);
    if(!valid || ignored!=1)return ((Max)(game_base+0x0843e7b0u))(job_wrapper);
    return result;
}

/* Explicit UI withdrawal can visit a held offer via the local array even
   after global removal. Do not decrement NumContractOffers a second time.
   Native 08352E00 consumes one retained reference supplied in array_ref. */
__declspec(dllexport) unsigned char car35_contract_withdraw_hook(void *module,void **job_ref,void **offer_ref,void **array_ref,void **coach_ref,unsigned char flag) {
    typedef unsigned char(*RemoveOffer)(void*,void**,void**,void**,void**,unsigned char);
    void *job=NULL,*offer=NULL,*array=NULL;car35_ContractJob j;
    if(mode==1 && car35_local_read((uintptr_t)job_ref,&job,8) && car35_local_read((uintptr_t)offer_ref,&offer,8) &&
       car35_local_read((uintptr_t)array_ref,&array,8) && array && car35_contract_job(job,&j)) {
        /* Only an unambiguous, still-selected accepted agreement is a
           repeat cleanup. Pending, declined, withdrawn, unfilled and
           superseded selections must continue through native withdrawal. */
        size_t index=0;void *local=car35_contract_array(job,0);
        void *selected=local==array?car35_contract_selected(local,&j,0,&index):NULL;
        int held=selected && selected==offer;
        car35_release_reference(selected);car35_release_reference(local);car35_contract_job_release(&j);
        if(held) { car35_release_reference(array);log_line("CONTRACT_HOLD source=repeat_withdraw job_row=%lu reason=unique_selected_accepted_not_pending",(unsigned long)car35_row_of(job));return 1; }
    }
    return ((RemoveOffer)(game_base+0x08356c00u))(module,job_ref,offer_ref,array_ref,coach_ref,flag);
}

/* Redirect only finalizer call 08346CE4. Global native Pending lookup first;
   if absent, reuse the selected Accepted record from the serialized job. */
__declspec(dllexport) void *car35_contract_lookup_hook(void *module,void **out,void **team_ref,int role,void **coach_ref,int status,unsigned char flag) {
    typedef void*(*Lookup)(void*,void**,void**,int,void**,int,unsigned char);
    if(mode==1 && !faulted)t36_pre_contract(module,0);
    void *ret=((Lookup)(game_base+0x0833f0a0u))(module,out,team_ref,role,coach_ref,status,flag);
    void *existing=NULL,*team=NULL,*coach=NULL,*jobs=NULL,*found=NULL;size_t n=0;int matches=0,valid=1;
    if(mode!=1 || status!=2 || !car35_local_read((uintptr_t)out,&existing,8) || existing ||
       !car35_local_read((uintptr_t)team_ref,&team,8) || !team ||
       !car35_local_read((uintptr_t)coach_ref,&coach,8) || !coach)return ret;
    jobs=car35_contract_array(module,21);
    if(!car35_contract_count(jobs,&n))valid=0;
    for(size_t i=0;valid && i<n;i++) {
        void *job=car35_contract_at(jobs,i);car35_ContractJob j;
        if(!job) { valid=0;break; }
        if(car35_contract_job(job,&j)) {
            if(j.team==team && j.coach==coach && j.role==role) {
                void *array=car35_contract_array(job,0);size_t selected=0;
                void *offer=car35_contract_selected(array,&j,0,&selected);car35_release_reference(array);
                if(offer) { ++matches;if(matches==1) { found=offer;offer=NULL; }car35_release_reference(offer); }
            }
            car35_contract_job_release(&j);
        }
        car35_release_reference(job);
    }
    car35_release_reference(jobs);
    if(valid && matches==1 && found) {
        int length=-1;car35_record_field(found,6,2,&length);
        *out=found;
        log_line("CONTRACT_REUSE coach_row=%lu team_row=%lu role=%d length=%d source=saved_selected_offer offer_row=%lu points=%d user=%d",
            (unsigned long)car35_row_of(coach),(unsigned long)car35_row_of(team),role,length,
            (unsigned long)car35_row_of(found),car35_get(found,7,2),car35_get(coach,0,1));
        return out;
    }
    car35_release_reference(found);
    log_line("CONTRACT_REUSE_MISS coach_row=%lu team_row=%lu role=%d matches=%d valid=%d fallback=native_reconstruction",
        (unsigned long)car35_row_of(coach),(unsigned long)car35_row_of(team),role,matches,valid);
    return ret;
}
