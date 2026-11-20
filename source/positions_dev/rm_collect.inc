/* Live ownership census and per-team preservation boundary. Read-only collection. */
static int record_id(Rec p,RMIdentity *out) {
    out->store=0;out->row=UINT32_MAX;
    return !p || (identity(p,&out->store,&out->row) && out->store && out->row!=UINT32_MAX);
}
static int same_id(RMIdentity a,RMIdentity b){return a.store==b.store && a.row==b.row;}
static int season_value(Rec season,const char *name,int *v) {
    for(unsigned i=0;i<sizeof(fields_SeasonInfo)/sizeof(fields_SeasonInfo[0]);i++)
        if(!strcmp(fields_SeasonInfo[i].name,name))return scalar(season,fields_SeasonInfo[i].index,fields_SeasonInfo[i].kind,v);
    return 0;
}
static int census_owner(RMIdentity p,int team) {
    unsigned slot=(unsigned)(((p.store>>4)^(p.row*UINT64_C(11400714819323198485)))&(RM_OWNER_SLOTS-1));
    for(unsigned probe=0;probe<RM_OWNER_SLOTS;probe++,slot=(slot+1)&(RM_OWNER_SLOTS-1)) {
        RMOwner *v=owners+slot;
        if(!v->store){*v=(RMOwner){p.store,p.row,team};return 1;}
        if(v->store==p.store && v->row==p.row){membership_conflict[team]=1;membership_conflict[v->team]=1;return 1;}
    }
    return 0;
}
static int census_has_owner(RMIdentity p,int team) {
    unsigned slot=(unsigned)(((p.store>>4)^(p.row*UINT64_C(11400714819323198485)))&(RM_OWNER_SLOTS-1));
    for(unsigned probe=0;probe<RM_OWNER_SLOTS;probe++,slot=(slot+1)&(RM_OWNER_SLOTS-1)) {
        const RMOwner *v=owners+slot;
        if(!v->store)return 0;
        if(v->store==p.store && v->row==p.row)return v->team==team;
    }
    return 0;
}
static __declspec(noinline) int league_census(Rec franchise,Rec season) {
    RMIdentity fid;int year,sy,ok=0,users=0;Rec teams=0;
    if(!record_id(franchise,&fid) || !fid.store || !season_value(season,"CurrentYear",&year) || !season_value(season,"CurrentSeasonYear",&sy))goto fail;
    if(league_valid && same_id(fid,census_franchise) && year==census_year && sy==census_season)return 1;
    league_valid=0;expected_cpu=completed_cpu=trained_cpu=0;dev_reset();
    memset(owners,0,sizeof(owners));memset(census_membership,0,sizeof(census_membership));
    memset(membership_conflict,0,sizeof(membership_conflict));memset(prep_seen,0,sizeof(prep_seen));memset(training_seen,0,sizeof(training_seen));
    teams=array_field(franchise,45,&ok);U n=array_count(teams);
    if(!ok || !n || n>RM_TEAM_LIMIT)goto fail;
    for(U i=0;i<n;i++) {
        Rec t=array_at(teams,i,&ok);int id=-1;
        if(!ok || !t || !scalar(t,383,2,&id) || id<0 || id>=RM_TEAM_LIMIT || census_membership[id]){release(t);goto fail;}
        Rec user=reference(t,407,&ok);int owned=user!=0;release(user);
        if(!ok){release(t);goto fail;}
        census_membership[id]=owned?2:1;if(owned)users++;else expected_cpu++;
        U total=0;
        for(int source=0;source<2;source++) {
            Rec a=array_field(t,source?33:239,&ok);U count=array_count(a);total+=count;
            if(!ok || count>RM_MAX_PLAYERS || total>RM_MAX_PLAYERS){release(a);release(t);goto fail;}
            for(U j=0;j<count;j++) {
                Rec p=array_at(a,j,&ok);RMIdentity pid;
                int valid=ok && p && record_id(p,&pid) && census_owner(pid,id);release(p);
                if(!valid){release(a);release(t);goto fail;}
            }
            release(a);
        }
        release(t);
    }
    release(teams);census_franchise=fid;census_year=year;census_season=sy;league_valid=1;
    int conflicts=0;for(int i=0;i<RM_TEAM_LIMIT;i++)if(membership_conflict[i]){conflicts++;log_line("CENSUS_MEMBERSHIP_CONFLICT affected_team=%d no custom writes for affected team",i);}
    log_line("CPU_CENSUS teams=%llu cpu=%d user=%d ownership_conflicts=%d year=%d season=%d",(unsigned long long)n,expected_cpu,users,conflicts,year,sy);
    return 1;
fail:
    release(teams);capture_error("league ownership census unavailable or beyond supported shape");return 0;
}
static __declspec(noinline) int pilot_guard(Rec team,Rec season) {
    pilot_clear();int ok=0,id=-1;
    if(!pilot_stage_guard(season) || !target(team) || !scalar(team,383,2,&id) ||
       !league_valid || census_membership[id]!=1 || membership_conflict[id])return 0;
    const int user_slot=field_slot("IsUserControlled"),position_slot=field_slot("Position"),
              type_slot=field_slot("PlayerType"),pid_slot=field_slot("PresentationId");
    for(int source=0;source<2;source++) {
        Rec a=array_field(team,source?33:239,&ok);U n=array_count(a);
        if(!ok || n>RM_MAX_PLAYERS || pilot_count+n>RM_MAX_PLAYERS){release(a);return 0;}
        source_counts[source]=(unsigned)n;
        for(U j=0;j<n;j++) {
            Rec p=array_at(a,j,&ok);RMIdentity ident;
            if(!ok || !p || !record_id(p,&ident) || input_index(p)>=0 || !census_has_owner(ident,id)){release(p);release(a);return 0;}
            unsigned i=pilot_count;pilot_players[i]=p;pilot_count++;
            if(!read_all(p,rm_initial[i]) || rm_initial[i][user_slot]){release(a);return 0;}
            int pos=rm_initial[i][position_slot],type=rm_initial[i][type_slot];
            if(pos<0 || pos>21 || type<0){release(a);return 0;}
            rm_inputs[i]=(RMPilotInput){ident.store,ident.row,(unsigned)rm_initial[i][pid_slot],source,pos,type};
            rm_members[source][j]=ident;
        }
        release(a);
    }
    if(!pilot_count)return 0;
    /* Stable identity order keeps tied assignments independent of array order. */
    for(unsigned i=1;i<pilot_count;i++)for(unsigned j=i;j>0;j--) {
        RMPilotInput a=rm_inputs[j-1],b=rm_inputs[j];
        if(a.row<b.row || (a.row==b.row && a.store<=b.store))break;
        rm_inputs[j-1]=b;rm_inputs[j]=a;Rec p=pilot_players[j-1];pilot_players[j-1]=pilot_players[j];pilot_players[j]=p;
        int vals[RM_FIELD_COUNT];memcpy(vals,rm_initial[j-1],sizeof(vals));memcpy(rm_initial[j-1],rm_initial[j],sizeof(vals));memcpy(rm_initial[j],vals,sizeof(vals));
    }
    memcpy(rm_expected,rm_initial,pilot_count*sizeof(rm_initial[0]));
    Rec chart=reference(team,63,&ok);
    if(!ok || !chart){release(chart);return 0;}
    Rec locked=array_field(chart,35,&ok);U locked_count=array_count(locked);release(locked);
    if(!ok || locked_count){release(chart);log_line("TEAM_INPUT_REJECT locked_depth_entries=%llu cannot alter forced chart assignments",(unsigned long long)locked_count);return 0;}
    for(unsigned c=0;c<RM_CHART_COUNT;c++) {
        Rec a=array_field(chart,charts[c].index,&ok);U n=array_count(a);
        if(!ok || n>RM_MAX_PLAYERS){release(a);release(chart);return 0;}
        chart_counts[c]=(unsigned)n;
        for(U j=0;j<n;j++) {
            Rec p=array_at(a,j,&ok);int valid=ok && record_id(p,&rm_chart_expected[c][j]);release(p);
            if(!valid){release(a);release(chart);return 0;}
        }
        release(a);
    }
    release(chart);pilot_ready=1;
    log_line("TEAM_INPUT_ACCEPT players=%u rostered=%u committed=%u fields=%d charts=%d",pilot_count,source_counts[0],source_counts[1],RM_FIELD_COUNT,RM_CHART_COUNT);
    for(unsigned i=0;i<pilot_count && !capture_fault;i++)player_snapshot_values("PRE_YEAR",pilot_players[i],rm_inputs[i].source,rm_initial[i]);
    return !capture_fault;
}
static __declspec(noinline) int pilot_finish(Rec team) {
    int ok=0;
    for(unsigned i=0;i<pilot_count;i++) {
        int now[RM_FIELD_COUNT];
        if(!read_all(pilot_players[i],now)){capture_error("final player capture");return 0;}
        for(unsigned f=0;f<RM_FIELD_COUNT;f++)if(now[f]!=rm_expected[i][f]) {
            log_line("TEAM_PRESERVATION_FAIL row=%u field=%s expected=%d actual=%d",rm_inputs[i].row,fields_Player[f].name,rm_expected[i][f],now[f]);
            capture_error("unexpected player mutation");return 0;
        }
    }
    for(int source=0;source<2;source++) {
        Rec a=array_field(team,source?33:239,&ok);U n=array_count(a);
        if(!ok || n!=source_counts[source]){release(a);capture_error("membership count changed");return 0;}
        for(U j=0;j<n;j++) {
            Rec p=array_at(a,j,&ok);RMIdentity v;int valid=ok && record_id(p,&v) && same_id(v,rm_members[source][j]);release(p);
            if(!valid){release(a);capture_error("membership identity/order changed");return 0;}
        }
        release(a);
    }
    Rec chart=reference(team,63,&ok);
    if(!ok || !chart){release(chart);capture_error("final chart reference");return 0;}
    Rec locked=array_field(chart,35,&ok);U locked_count=array_count(locked);release(locked);
    if(!ok || locked_count){release(chart);capture_error("locked depth entries changed");return 0;}
    for(unsigned c=0;c<RM_CHART_COUNT;c++) {
        Rec a=array_field(chart,charts[c].index,&ok);U n=array_count(a);
        if(!ok || n!=chart_counts[c]){release(a);release(chart);capture_error("chart count changed");return 0;}
        for(U j=0;j<n;j++) {
            Rec p=array_at(a,j,&ok);RMIdentity v;int valid=ok && record_id(p,&v) && same_id(v,rm_chart_expected[c][j]);release(p);
            if(!valid){release(a);release(chart);capture_error("unexpected chart mutation");return 0;}
        }
        release(a);
    }
    release(chart);log_line("TEAM_PRESERVATION_PASS players=%u fields_per_player=%d membership=UNCHANGED charts=VERIFIED",pilot_count,RM_FIELD_COUNT);
    return 1;
}
