/* A funded pair is two real scholarships for the same school opening. Native
   offers, ownership and room eligibility are reread at each signing. */
static Rc4Pair *rc4_pair_for(unsigned team,unsigned rr,unsigned week) {
    for(unsigned i=0;i<rc4.disk.pairs;i++){
        Rc4Pair *p=rc4.disk.pair+i;
        if(p->team==team && p->week<=week && (p->primary==rr || p->backup==rr))return p;
    }
    return 0;
}
static int rc4_pair_free(unsigned team,unsigned slot,unsigned primary,unsigned backup,unsigned week) {
    if(primary==backup || primary>=RC4_ROWS || backup>=RC4_ROWS || rc4.disk.pairs>=RC4_PAIRS)return 0;
    if(!slot || slot>128)return 0;
    for(unsigned i=0;i<rc4.disk.pairs;i++){
        const Rc4Pair *p=rc4.disk.pair+i;
        if(p->team==team && p->slot==slot && p->week<=week)return 0;
    }
    return !rc4_pair_for(team,primary,week) && !rc4_pair_for(team,backup,week);
}
static int rc4_same_pair(unsigned t,unsigned slot,unsigned a,unsigned b,unsigned week){
    Rc4Pair *p=rc4_pair_for(t,b,week);return p && p->slot==slot && ((p->primary==a && p->backup==b) || (p->primary==b && p->backup==a));
}
static __declspec(thread) Rc4Pair rc4_pending;
static __declspec(thread) int rc4_pending_live;
static int rc4_pair_gate(Rec recruit,Rec team,U eval,const char *site,PortalRoomIssue *optional) {
    Rc4Member *member=rc4_member(recruit);
    if(!member || member->pos!=0)return 0;
    unsigned tid=scalar(team,383,0x100),rr=row(recruit);
    Rc4Pair *pair=rc4_pair_for(tid,rr,rc4.week);
    if(!pair && rc4_pending_live && rc4_pending.team==tid && (rc4_pending.primary==rr || rc4_pending.backup==rr))pair=&rc4_pending;
    if(!pair)return 0;
    Rec own=t36_target(team,recruit);unsigned own_status=own?scalar(own,12,0x108):0;release(own);
    if((own_status<2 || own_status>4) && !(rc4_pending_live && pair==&rc4_pending))return 0;
    Rec f=context_ref(eval,6),season=f?reference(f,44):0;
    int scope=f && (U)f==rc4.franchise && season && scalar(season,5,0x108)==6 && scalar(season,2,0x100)==rc4.year &&
              scalar(season,4,0x100)>=pair->week && scalar(season,4,0x100)<=6;
    Rec list=scope?array_field(f,46):0;release(season);release(f);
    if(!list)return 0;
    unsigned other=rr==pair->primary?pair->backup:pair->primary;
    Rc4Member *om=rc4.member+other;
    Rec rival=om->index && om->index<=array_count(list)?array_at(list,om->index-1):0;
    release(list);
    if(!rival || row(rival)!=other || rc4_member(rival)!=om){release(rival);return 0;}
    int owner=-1,stage=(int)scalar(rival,9,0x108),decision=0;
    if(stage>=5){
        int known=0;
        if(stage==5)known=portal_hard_owner(rival,eval,&owner);
        else{Rec p=reference(rival,5);known=p?portal_signed_destination(rival,p,&owner):0;release(p);}
        if(!known)known=portal_published_owner(rival,eval,&owner);
        if(known>0)decision=owner==(int)tid?10:-1;
        else decision=9;
    }else if(rr==pair->backup){
        Rec target=t36_target(team,rival);unsigned rank=0;
        int final=!strncmp(site,"SIGN_",5) || !strncmp(site,"SELECT_",7);
        int held=target && scalar(target,12,0x108)>=2 && scalar(target,12,0x108)<=4;
        int viable=held?portal_commit_viable(rival,target,team,eval,final,&rank,optional):0;
        /* Preserve a new paid primary while it earns its first weekly points.
           On signing day the normal winner/rank checks determine viability. */
        int fresh=held && !final && !rank && pair->week==rc4.week && !locked_out(rival,team,eval) && native_recruit_eligible(eval,team,rival);
        decision=held && (viable!=0 || fresh)?9:-1;
        release(target);
    }else decision=-1;
    release(rival);
    if(!optional && decision)log_line("RC4_QB_SETTLE,%u,%u,%u,%u,%u,%u,%d,%d,%d,%s",rc4.year,rc4.week,tid,pair->slot,rr,other,stage,owner,decision,site);
    return decision;
}
