/* RM18R22: only the two automatic FillStaff callers can open this scope.
   One invocation owns one CPU vacancy. Nothing survives the native call;
   there is no row registry, cached save pointer, grade write or budget write.
   The ordinary/UI builders and all reconstruction paths retain C+.
   Native FillStaff signs an emergency HC directly at 08361DAF. */
typedef struct {
    void *module,*team,*manager,*owner,*selected,*build;
    int enabled,building,floor,selected_floor,selected_grade,consumed;
} Car35Emergency;
static DWORD car35_emergency_tls=TLS_OUT_OF_INDEXES;
static Car35Emergency *car35_emergency_context(void) {
    if(car35_emergency_tls==TLS_OUT_OF_INDEXES)return NULL;
    return (Car35Emergency*)TlsGetValue(car35_emergency_tls);
}
static int car35_emergency_vacancy(void *team) {
    int user=-1,hc=-1;
    return team && car35_record_reference_present(team,407,&user) && user==0 &&
        car35_record_reference_present(team,111,&hc) && hc==0;
}
static int car35_emergency_owner(Car35Emergency *e) {
    void *manager=NULL,*owner=NULL;
    return e && e->enabled && e->module &&
        car35_local_read((U)e->module+0x178,&manager,8) && manager && manager==e->manager &&
        car35_local_read((U)e->module+0x28,&owner,8) && owner && owner==e->owner;
}
static int car35_emergency_candidate(void *coach) {
    int user=-1,status=-1;
    return coach && car35_record_field(coach,0,1,&user) && user==0 &&
        car35_record_field(coach,12,0,&status) && status==7;
}
static int car35_emergency_candidate_floor(void *module,void *team,void *coach,int destination,void *build) {
    Car35Emergency *e=car35_emergency_context();
    return mode==1 && e && e->building && build && e->build==build && e->module==module && e->team==team &&
        destination==0 && car35_emergency_owner(e) && car35_emergency_candidate(coach)?e->floor:6;
}
static int car35_emergency_vector(void *out,U *begin,U *end) {
    U cap=0;
    return out && car35_local_read((U)out,begin,8) &&
        car35_local_read((U)out+8,end,8) && car35_local_read((U)out+16,&cap,8) &&
        *end>=*begin && cap>=*end && (!*end || *begin) &&
        (*end-*begin)%48==0 && (cap-*begin)%48==0 && (cap-*begin)/48<=4096;
}
static int car35_emergency_retain_selected(Car35Emergency *e,void *out) {
    U begin=0,end=0,vt=0,addref=0;void *coach=NULL;int grade=-1,current=-1,previous=-1;
    if(!car35_emergency_vector(out,&begin,&end) || begin==end ||
       !car35_local_read(begin,&coach,8) || !car35_emergency_candidate(coach) ||
       !car35_record_field(coach,112,0,&grade) ||
       !car35_record_field(coach,16,0,&current) || !car35_record_field(coach,17,0,&previous) ||
       !car35_promotion_grade_at_floor(0,current,previous,grade,e->floor) ||
       !car35_local_read((U)coach,&vt,8) || !car35_local_read(vt,&addref,8) ||
       !car35_code_pointer(addref))return 0;
    ((void(*)(void*))addref)(coach);
    car35_release_reference(e->selected);
    e->selected=coach;e->selected_floor=e->floor;e->selected_grade=grade;
    return 1;
}
static int car35_emergency_sign_floor(void *manager,void *team,void *coach,void *offer,int role,U caller) {
    Car35Emergency *e=car35_emergency_context();int grade=-1,status=-1;
    if(mode!=1 || !e || !e->enabled || e->building || e->consumed ||
       caller!=0x08361e14 || role!=0 || e->manager!=manager || e->team!=team ||
       e->selected!=coach || !car35_emergency_owner(e) || !car35_emergency_vacancy(team) ||
       !car35_emergency_candidate(coach) || !car35_record_field(coach,112,0,&grade) ||
       grade!=e->selected_grade || !car35_record_field(offer,11,0,&status) || status!=0)return 6;
    void *offered_team=car35_retain_reference(offer,12);
    int match=offered_team==team;car35_release_reference(offered_team);
    return match?e->selected_floor:6;
}
__declspec(dllexport) unsigned char car35_fill_staff_hook(void *module,void **team_ref,unsigned char fill,unsigned char report) {
    typedef unsigned char(*Fill)(void*,void**,unsigned char,unsigned char);
    Fill original=(Fill)(game_base+0x08361b70);
    Car35Emergency *previous=car35_emergency_context(),*e=NULL;
    /* Fail closed if TLS cannot isolate this invocation from a nested one. */
    if(!TlsSetValue(car35_emergency_tls,NULL)) {
        if(previous)previous->enabled=0;
        return original(module,team_ref,fill,report);
    }
    void *team=NULL,*manager=NULL,*owner=NULL;
    if(mode==1 && fill && car35_local_read((U)team_ref,&team,8) && car35_emergency_vacancy(team) &&
       car35_local_read((U)module+0x178,&manager,8) && manager &&
       car35_local_read((U)module+0x28,&owner,8) && owner) {
        e=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*e));
        if(e) {
            e->module=module;e->team=team;e->manager=manager;e->owner=owner;
            e->enabled=1;e->floor=6;
            if(!TlsSetValue(car35_emergency_tls,e)) {HeapFree(GetProcessHeap(),0,e);e=NULL;}
        }
    }
    unsigned char result=original(module,team_ref,fill,report);
    if(e) {
        int hc=-1;car35_record_reference_present(team,111,&hc);
        log_line("CAR35_EMERGENCY_RESULT tier=%d selected=%d relaxed_sign_authorized=%d hc_present=%d native_result=%d floor_Cminus=8",
            e->selected_floor,e->selected!=NULL,e->consumed,hc,result);
        e->enabled=0;car35_release_reference(e->selected);
        e->module=e->team=e->manager=e->owner=e->selected=e->build=NULL;
    }
    if(!TlsSetValue(car35_emergency_tls,previous)) {
        /* Retain a disabled allocation, never leave TLS pointing at a stack
           object or freed context. A later invocation may clear it. */
        log_line("CAR35_EMERGENCY_DISABLED reason=TLS_restore_failed normal_Cplus_retained=1");
    } else if(e)HeapFree(GetProcessHeap(),0,e);
    return result;
}
