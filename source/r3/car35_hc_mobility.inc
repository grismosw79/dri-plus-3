/* R10: restrict only a verified, still-employed HC's move to a non-P4
   coordinator vacancy. Read current conference membership, not school IDs,
   prestige, or a historical P5/Pac-12 flag. No new tunable or record writes. */
static int car35_same_record(void *a,void *b);
static int car35_hc_destination_conference(void *module,void *team) {
    void *franchise=car35_retain_reference(module,20);
    void *league=car35_retain_reference(franchise,27);
    void *conferences=car35_contract_array(league,3);
    size_t count=0;int valid=team && car35_contract_count(conferences,&count) && count>0 && count<=32;
    int matches=0,result=-1;
    for(size_t i=0;valid && i<count;i++) {
        void *conf=car35_contract_at(conferences,i);
        if(!conf)continue;
        void *teams=car35_contract_array(conf,40);size_t n=0;
        /* Empty/unused conference slots may have no team array. */
        if(teams && (!car35_contract_count(teams,&n) || n>256))valid=0;
        for(size_t k=0;valid && k<n;k++) {
            void *member=car35_contract_at(teams,k);
            if(car35_same_record(member,team)) {
                ++matches;result=car35_get(conf,22,0);
            }
            car35_release_reference(member);
        }
        car35_release_reference(teams);car35_release_reference(conf);
    }
    car35_release_reference(conferences);car35_release_reference(league);car35_release_reference(franchise);
    return valid && matches==1 && result>=0 && result<=10?result:-1;
}
static int car35_hc_source_bound(void *module,void *coach,void *old_team,int source) {
    if(old_team) {
        void *hc=car35_retain_reference(old_team,111);
        int bound=car35_get(old_team,383,2)==source && car35_same_record(hc,coach);
        car35_release_reference(hc);return bound;
    }
    /* Reconstruction has no old-team argument. Resolve its actual incumbent
       from the same franchise, before native cleanup mutates the coach. */
    void *franchise=car35_retain_reference(module,20);
    void *teams=car35_contract_array(franchise,45);size_t n=0;int matches=0,bound=0;
    int valid=car35_contract_count(teams,&n) && n<=256;
    for(size_t i=0;valid && i<n;i++) {
        void *team=car35_contract_at(teams,i);
        if(team && car35_get(team,383,2)==source) {
            ++matches;void *hc=car35_retain_reference(team,111);
            bound=car35_same_record(hc,coach);car35_release_reference(hc);
        }
        car35_release_reference(team);
    }
    car35_release_reference(teams);car35_release_reference(franchise);
    return valid && matches==1 && bound;
}
static int car35_hc_move_allowed(void *module,void *coach,void *old_team,void *team,int role,int conference,const char *path) {
    if(!coach || (role!=1 && role!=2) || car35_get(coach,16,0)!=0)return 1;
    int status=car35_get(coach,12,0),source=car35_get(coach,8,2),target=car35_get(team,383,2);
    int employed=(status==0 || status==4) && source>=0 && source<255;
    int bound=0,blocked=0;
    const char *reason="not_still_employed_native_retained";
    if(employed && team && source!=target) {
        bound=car35_hc_source_bound(module,coach,old_team,source);
        if(!bound)reason="unverified_incumbent_native_retained";
        else {
            if(conference==-2)conference=car35_hc_destination_conference(module,team);
            if(conference<0)reason="unverified_conference_native_retained";
            else {
                /* CollegeConferences values, verified in the current schema:
                   ACC=0, BigTen=1, Big12=2, SEC=9. Pac12=8 is non-P4. */
                blocked=conference!=0 && conference!=1 && conference!=2 && conference!=9;
                reason=blocked?"employed_HC_nonP4_coordinator":"P4_coordinator_native_retained";
            }
        }
    } else if(employed)reason="same_school_or_unresolved_destination_native_retained";
    log_line("CAR35_HC_COORD_GATE path=%s coach_row=%lu team_row=%lu source_team=%d target_team=%d destination=%d status=%d user=%d incumbent_verified=%d conference=%d blocked=%d returned=%d reason=%s mode=%ld",
        path,(unsigned long)car35_row(coach),(unsigned long)car35_row(team),source,target,role,status,
        car35_get(coach,0,1),bound,conference,blocked,mode!=1 || !blocked,reason,mode);
    return mode!=1 || !blocked;
}
