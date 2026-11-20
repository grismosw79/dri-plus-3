/* R9: shared recent-success and established-career qualification. Neither
   control mode nor dismissal status earns a bonus. All native post gates stay
   effective. A former HC is evaluated on verified career evidence, never by
   pretending his team rank was an OC/DC unit rank. */
static void *car35_contract_array(void *record,unsigned field);
static int car35_contract_count(void *array,size_t *count);
static void *car35_contract_at(void *array,size_t index);

static int car35_competition_population(void *module) {
    void *franchise=car35_retain_reference(module,20);
    void *teams=car35_contract_array(franchise,45);size_t n=0;
    unsigned char seen[255]={0};int valid=car35_contract_count(teams,&n) && n>=2 && n<=255;
    for(size_t i=0;valid && i<n;i++) {
        void *team=car35_contract_at(teams,i);int index=-1;
        valid=car35_record_field(team,383,2,&index) && index>=0 && index<255 && !seen[index];
        if(valid)seen[index]=1;
        car35_release_reference(team);
    }
    car35_release_reference(teams);car35_release_reference(franchise);
    return valid?(int)n:0;
}

/* -1 means unreadable, 0 clear, 1 blocked. A null native blacklist is clear.
   Compare SDF data plus row as well as pointers; wrappers may differ. */
static int car35_competition_blacklist(void *team,void *coach) {
    int present=-1;
    if(!car35_record_reference_present(team,284,&present))return -1;
    if(!present)return 0;
    void *array=car35_contract_array(team,284);size_t n=0;
    int result=car35_contract_count(array,&n)?0:-1;
    for(size_t i=0;result==0 && i<n;i++) {
        void *item=car35_contract_at(array,i);uintptr_t a=0,b=0;uint32_t ar=0,br=0;
        if(!item)result=-1;
        else if(item==coach)result=1;
        else if(!car35_local_read((U)item+0x10,&a,8) || !car35_local_read((U)coach+0x10,&b,8) ||
                !car35_local_read((U)item+0x18,&ar,4) || !car35_local_read((U)coach+0x18,&br,4))result=-1;
        else if(a && a==b && ar==br)result=1;
        car35_release_reference(item);
    }
    car35_release_reference(array);return result;
}

static const char *car35_competition_qualify(car35_BuildContext *c,car35_CandidateMeta *m) {
    int role=-1,status=-1,side=-1,years=-1,wins=-1,losses=-1,scheme=-1,option=-1,rank=-1;
    m->qualified=0;m->qualification_path=0;m->grade=-1;m->unit=-1;m->games=-1;m->blacklist=-1;
    if(c->bad || !c->coordinator_enabled)return "outside_shared_coordinator_scope";
    if(!m->broad_seen || !m->broad_pass)return "native_broad_not_passed";
    if(c->population<2)return "population_unverified";
    if(!car35_record_field(m->coach,16,0,&role) || (role!=c->destination && role!=0) ||
       !car35_record_field(m->coach,134,0,&side) || side!=c->destination-1 ||
       !car35_record_field(m->coach,12,0,&status) || (status!=0 && status!=2 && status!=4 && status!=7))return "role_identity_or_status";
    if(!car35_record_field(m->coach,112,0,&m->grade) || m->grade<0 || m->grade>6)return "below_Cplus_or_unknown_grade";
    if(!car35_record_field(m->coach,135,2,&years) || years<=0)return "no_verified_experience";
    void *career=car35_retain_reference(m->coach,29);
    int history=car35_record_field(career,24,2,&wins) && car35_record_field(career,7,2,&losses);
    car35_release_reference(career);
    if(!history || wins<0 || losses<0 || (int64_t)wins+losses<=0 || (int64_t)wins+losses>INT32_MAX)return "no_recorded_career_games";
    m->games=wins+losses;
    int recent=role==c->destination && car35_record_field(m->coach,120,2,&rank) &&
        rank>=0 && rank<(c->population+1)/2;
    /* B or better, at least two actual coaching years and a winning career.
       The grade retains the FTC prestige calculation/history weights. These
       are eligibility categories, not invented history or scoring weights. */
    int established=m->grade<=4 && years>=2 && wins>losses && m->games>=12;
    if(!recent && !established)return "neither_recent_unit_success_nor_established_winning_career";
    m->qualification_path=recent?1:2;
    m->unit=role==c->destination && rank>=0 && rank<c->population?rank:c->population;
    /* The native score requires an offensive scheme for both destinations.
       Keep its triple-option prohibition even when its final score is zero. */
    void *offense=car35_retain_reference(m->coach,51);
    int scheme_ok=car35_record_field(offense,6,0,&scheme);
    car35_release_reference(offense);
    if(!scheme_ok || scheme<0 || scheme>10 ||
       !car35_record_field(c->team,10,1,&option) || (scheme==10 && !option))return "native_scheme_gate";
    m->blacklist=car35_competition_blacklist(c->team,m->coach);
    if(m->blacklist!=0)return "blacklisted_or_unreadable";
    m->qualified=1;return recent?"qualified_recent_unit":"qualified_established_career";
}

static int car35_competition_post(car35_BuildContext *c,car35_CandidateMeta *m,U entry) {
    int expected=-1,offered=-1,interest=-1,cutoff=-1,comparison=-1;
    m->post_pass=0;
    if(!car35_local_read(entry+16,&expected,4) || !car35_local_read(entry+20,&offered,4) ||
       !car35_local_read(entry+32,&interest,4) || !car35_local_read(entry+40,&comparison,4) ||
       !car35_local_read((U)c->module+0x23c,&cutoff,4))return 0;
    m->blacklist=car35_competition_blacklist(c->team,m->coach);
    /* Native free agents are forced to 1 even after cost/interest rejection.
       That fallback remains available, but is never treated as qualification. */
    m->post_pass=m->qualified && m->native>0 && expected>=0 && offered>=expected &&
        cutoff>=0 && interest>=cutoff && comparison==2 && m->blacklist==0;
    return m->post_pass;
}

/* On an invalid complete vector, withdraw only R6's injected support from
   non-free-agents. Never turn an unverified list into new offer eligibility. */
static void car35_competition_withdraw(void *begin,void *end,car35_BuildContext *c) {
    U first=(U)begin,last=(U)end;
    if(mode!=1 || last<first || (last-first)%48 || (last-first)/48>1024)return;
    for(U p=first;p<last;p+=48) {
        void *coach=NULL;
        if(!car35_local_read(p,&coach,8))continue;
        car35_CandidateMeta *m=car35_meta_for(c,coach,0);int status=-1;
        if(m && m->support_applied && car35_record_field(coach,12,0,&status) && status!=7) {
            int zero=0;memcpy((void*)(p+24),&zero,4);
        }
    }
}
