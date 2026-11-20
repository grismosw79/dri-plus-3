/* RM12: coverage against live ideals; never rewrite a depth-chart array. */
static __declspec(noinline) int position_targets(Rec team,int ideal[22]) {
    /* Same native path/indices as accepted R3 t36_config / hs15_collect. */
    static const int members[22]={5,2,1,9,8,4,3,0,6,7,3,6,1,4,5,7,0,2,8,1,6,4};
    int ok=0,total=0;Rec cfg=reference(team,64,&ok),off=0,def=0;
    if(!ok||!cfg)goto fail;
    off=reference(cfg,5,&ok);if(!ok||!off)goto fail;
    def=reference(cfg,0,&ok);if(!ok||!def)goto fail;
    for(int p=0;p<22;p++) {
        Rec entry=reference(p<10?off:p<19?def:cfg,members[p],&ok);
        int valid=ok&&entry&&scalar(entry,2,2,ideal+p);release(entry);
        if(!valid||ideal[p]<0||ideal[p]>30)goto fail;
        total+=ideal[p];
    }
    release(def);release(off);release(cfg);
    return total>=40&&total<=110&&ideal[0]>0;
fail:release(def);release(off);release(cfg);return 0;
}
static int listed_count(int position) {
    int n=0,slot=field_slot("Position");
    for(unsigned i=0;i<pilot_count;i++)n+=rm_expected[i][slot]==position;
    return n;
}
static int primary_elsewhere(unsigned i,int destination) {
    /* Inspect real charts. One primary duty is retained, not two listed players. */
    for(int pos=0;pos<=20;pos++) {
        if(pos==destination)continue;
        int c=chart_index(pos);
        if(c>=0&&chart_counts[c]&&rm_inputs[i].store==rm_chart_expected[c][0].store&&
           rm_inputs[i].row==rm_chart_expected[c][0].row)return 1;
    }
    return 0;
}
static int evaluated_grade(Rec grade,unsigned i,int destination,int *type,int *value) {
    int pos=rm_expected[i][field_slot("Position")],oldtype=rm_expected[i][field_slot("PlayerType")];
    *type=destination_type(pos,destination,oldtype);
    if(*type<0)return 0;
    int self=0,stored=rm_expected[i][field_slot("OverallRating")];
    int ok=role_grade(grade,pilot_players[i],oldtype,&self)&&self==stored&&
           role_grade(grade,pilot_players[i],*type,value);
    log_line("ROLE_FIT row=%u source=%s destination=%s type=%d stored=%d self=%d grade=%d verified=%d",
             rm_inputs[i].row,role_names[pos],role_names[destination],*type,stored,self,*value,ok);
    return ok;
}
static int destination_rank(unsigned i,int dest) {
    int c=chart_index(dest);if(c<0)return RM_MAX_PLAYERS+1;
    for(unsigned j=0;j<chart_counts[c];j++)if(rm_inputs[i].store==rm_chart_expected[c][j].store&&
        rm_inputs[i].row==rm_chart_expected[c][j].row)return (int)j;
    return RM_MAX_PLAYERS+1;
}
static int best_reserve(Rec grade,int source,int dest,int keep_strongest,int *newtype,int *newgrade) {
    int best=-1,best_rank=RM_MAX_PLAYERS+2;
    for(unsigned i=0;i<pilot_count&&!capture_fault;i++) {
        if(rm_expected[i][field_slot("Position")]!=source||primary_elsewhere(i,dest))continue;
        int type=-1,value=0;if(!evaluated_grade(grade,i,dest,&type,&value))continue;
        /* Retain the source's strongest listed player when choosing among
           reserves, without imposing a blanket top-two restriction. */
        int other_best=-1;
        for(unsigned j=0;j<pilot_count;j++)if(j!=i&&rm_expected[j][field_slot("Position")]==source&&
            rm_expected[j][field_slot("OverallRating")]>other_best)other_best=rm_expected[j][field_slot("OverallRating")];
        /* Empty K/P coverage keeps the actual source primary above, but must
           not also protect a different, higher-rated reserve. */
        if(keep_strongest&&other_best<rm_expected[i][field_slot("OverallRating")])continue;
        int rank=destination_rank(i,dest);
        if(best<0||rank<best_rank||(rank==best_rank&&value>*newgrade)) {
            best=(int)i;best_rank=rank;*newtype=type;*newgrade=value;
        }
    }
    /* The grade helper is read-only; validate all players before any write. */
    for(unsigned i=0;i<pilot_count;i++) {
        int values[RM_FIELD_COUNT];
        if(!read_all(pilot_players[i],values)||memcmp(values,rm_expected[i],sizeof(values))) {
            capture_error("read-only role evaluation changed player");return -1;
        }
    }
    return best;
}
static void position_pair(Rec manager,Rec team,Rec grade,int family,int a,int b,const int ideal[22],int emergency) {
    int ca=listed_count(a),cb=listed_count(b),source=-1,dest=-1,goal=0,floor=0;
    unit_status[family]=1;
    if(emergency) {
        if(!cb&&ca>=2){source=a;dest=b;goal=1;floor=1;}
        else if(!ca&&cb>=2){source=b;dest=a;goal=1;floor=1;}
    } else {
        if(ca>ideal[a]&&cb<ideal[b]){source=a;dest=b;goal=ideal[b];floor=ideal[a];}
        else if(cb>ideal[b]&&ca<ideal[a]){source=b;dest=a;goal=ideal[a];floor=ideal[b];}
    }
    log_line("COUNT_PLAN family=%s a=%s before_a=%d ideal_a=%d b=%s before_b=%d ideal_b=%d emergency=%d",
             unit_names[family],role_names[a],ca,ideal[a],role_names[b],cb,ideal[b],emergency);
    if(source<0) {
        log_line("COUNT_HOLD family=%s reason=%s",unit_names[family],emergency?"both_covered_or_no_spare":
                 (ca>=ideal[a]&&cb>=ideal[b])?"ideals_already_met":"no_source_surplus_defer_remaining_shortage");
    } else while(listed_count(dest)<goal&&listed_count(source)>floor&&!capture_fault) {
        int type=-1,value=0,i=best_reserve(grade,source,dest,!emergency,&type,&value);
        if(i<0){unit_status[family]=2;log_line("UNIT_DEFERRED family=%s reason=no_safe_reserve",unit_names[family]);break;}
        log_line("COUNT_MOVE row=%u source=%s destination=%s source_count=%d destination_count=%d destination_goal=%d",
                 rm_inputs[i].row,role_names[source],role_names[dest],listed_count(source),listed_count(dest),goal);
        if(!convert_preserving(manager,team,i,dest,type,value))capture_error("native conversion preservation");
    }
    log_line("COUNT_RESULT family=%s after_a=%d after_b=%d charts_written=0",unit_names[family],listed_count(a),listed_count(b));
}
static void center_exception(Rec manager,Rec team,Rec grade,const int ideal[22]) {
    /* No absolute quality cutoff has been approved. This build logs possible
       quality-only G-to-C deferrals for review, rather than silently inventing one.
       A genuinely EMPTY C room can receive one safe surplus guard. */
    int centers=listed_count(RM_C),best=-1,type=-1,value=0,src=-1;
    if(ideal[RM_C]<=0){log_line("CENTER_HOLD reason=zero_ideal");return;}
    if(centers){log_line("CENTER_HOLD count=%d reason=quality_exception_threshold_unfinalized no_C_to_G=1",centers);return;}
    for(int k=0;k<2;k++) {
        int s=k?RM_RG:RM_LG;if(listed_count(s)<=ideal[s])continue;
        int t=-1,v=0,i=best_reserve(grade,s,RM_C,1,&t,&v);
        if(i>=0&&(best<0||v>value)){best=i;type=t;value=v;src=s;}
    }
    if(best<0){log_line("CENTER_DEFERRED empty_room no_safe_surplus_guard");return;}
    log_line("CENTER_EMERGENCY row=%u source=%s destination=C",rm_inputs[best].row,role_names[src]);
    if(!convert_preserving(manager,team,best,RM_C,type,value))capture_error("center conversion preservation");
}
static void pilot_positions(Rec team_manager,Rec team,Rec franchise) {
    int ok=0,ideal[22];Rec ev=reference(franchise,9,&ok),grade_raw=0,grade=0,manager_raw=0,manager=0;
    if(ok&&ev)grade_raw=reference(ev,10,&ok);
    if(ok&&grade_raw)grade=cast_record(grade_raw,0x46455045);
    manager_raw=reference(team_manager,6,&ok);if(ok&&manager_raw)manager=cast_record(manager_raw,0x504c4d67);
    if(!ok||!grade||!manager||!position_targets(team,ideal)) {
        for(int f=0;f<6;f++){unit_status[f]=2;log_line("UNIT_DEFERRED family=%s native_module_or_ideal_unavailable",unit_names[f]);}
        goto done;
    }
    for(int p=0;p<22;p++)log_line("POSITION_COUNT position=%s count=%d ideal=%d",role_names[p],listed_count(p),ideal[p]);
    for(int f=0;f<6&&!capture_fault;f++)position_pair(manager,team,grade,f,unit_roles[f][0],unit_roles[f][1],ideal,f==5);
    if(!capture_fault)center_exception(manager,team,grade,ideal);
    team_snapshot("POSITION_TEAM_AFTER",team);
done:release(manager);release(manager_raw);release(grade);release(grade_raw);release(ev);
}
