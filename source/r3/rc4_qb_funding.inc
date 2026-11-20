static int rc4_paid(Rec target,Rec team,U eval){
    unsigned mask=0,status=target?scalar(target,12,0x108):0;
    return status>=2 && status<=4 && test31_productive_allocated(target,team,eval,&mask)>0;
}
static Rec rc4_pool_member(Rec pool,unsigned rr){
    if(rr>=RC4_ROWS || !rc4.member[rr].index || rc4.member[rr].index>array_count(pool))return 0;
    Rec r=array_at(pool,rc4.member[rr].index-1);
    if(!r || row(r)!=rr || !rc4_member(r)){release(r);return 0;}return r;
}
/* Native teams have completed the ordinary funding batch. Race, actual paid
   target, original vacancy, hours, cap and the current scholarship flag must
   still agree before buying a second offer for the opening. */
static void rc4_work_openings(R2School *schools,Rec f,U eval,const R2Pass *scope,R2Offers *offers) {
    if(!rc4.rec_table || !rc4_live_window(eval))return;
    Rec list=array_field(f,46);if(!list)return;
    for(unsigned t=0;t<138 && !faulted;t++){
        R2School *s=schools+t;if(!s->team || s->user || portal_academy(t))continue;
        PortalCommitPlan plan,now={0};
        AcquireSRWLockExclusive(&commitment_lock);plan=commitment_plans[t];ReleaseSRWLockExclusive(&commitment_lock);
        if(!plan.valid || !portal_commit_scope(&now,s->team,eval) || !portal_commit_plan_matches(&plan,&now) || plan.week!=scope->week)continue;
        for(int j=0;j<plan.count && !faulted;j++){
            PortalCommitEntry *b=plan.entries+j;
            if(b->pos!=0 || b->round!=2 || b->slot<1 || b->slot>128)continue;
            int k=0;while(k<plan.count && (plan.entries[k].pos!=0 || plan.entries[k].round!=1 || plan.entries[k].slot!=b->slot))k++;
            if(k==plan.count)continue;
            unsigned first=plan.entries[k].rid;
            int recorded=rc4_same_pair(t,b->slot,first,b->rid,scope->week);
            if(!recorded && !rc4_pair_free(t,b->slot,first,b->rid,scope->week))continue;
            Rec primary=rc4_pool_member(list,first),backup=rc4_pool_member(list,b->rid);
            if(!primary || !backup || (!rc4_elite((U)primary) && !rc4_elite((U)backup)) ||
               scalar(primary,9,0x108)>=5 || scalar(backup,9,0x108)>=5){release(backup);release(primary);continue;}
            Rec pt=t36_target(s->team,primary),bt=t36_target(s->team,backup);
            int first_paid=rc4_paid(pt,s->team,eval),both_paid=first_paid && rc4_paid(bt,s->team,eval);
            release(bt);release(pt);
            Rec person=reference(backup,5);int raw=person?(int)scalar(person,191,0x100):-1;release(person);
            if(first_paid && raw>=0 && raw<=99 && r2_load_room(s,eval,1) &&
               (s->rooms[0].count+s->rooms[0].hs_reserved<s->rooms[0].ideal ||
                dri_portal_role_candidate(&s->rooms[0],s->shot->prestige,portal_projected_ovr(raw,s->shot->projection_step),raw)==1)){
                if(both_paid && !recorded){
                    rc4.disk.pair[rc4.disk.pairs++]=(Rc4Pair){t,(unsigned short)b->slot,(unsigned short)scope->week,(unsigned short)first,(unsigned short)b->rid};
                    rc4_save();log_line("RC4_QB_PAIR_EXISTING,%u,%u,%u,%u,%u,%u",scope->year,scope->week,t,b->slot,first,b->rid);
                }else if(!both_paid){
                    Market29PortalRecruit *m=market29_portal_find(q((U)backup+0x10),b->rid);
                    R2Offers *o=m?offers+(m-market29_portal.recruits):0;
                    if(o && !o->unverified && !locked_out(backup,s->team,eval) &&
                       (market29_has(o->offered,t) || market30_count(o->offered)<market30_limit(1))){
                        R2Pursuit v={.school=(int)t,.compare_qb=1};
                        r2_late_recruit=backup;r2_late_team=s->team;
                        if(raw>=0 && r2_option(s,backup,eval,1,0,raw,scope->week,&v) && v.pair_round && v.rival==first && v.slot==b->slot){
                            r2_offer_set(m,o->offered,t);
                            int success=r2_fund(backup,s->team,eval,1,0,&v);
                            if(!faulted && portal_active_offer(backup,s->team)>0)market29_bit(o->offered,t,1);
                            r2_offer_set(m,o->offered,-1);
                            if(success)r2_forget_room(s);
                        }
                        r2_late_recruit=0;r2_late_team=0;
                    }
                }
            }
            release(primary);release(backup);
        }
    }
    release(list);
}
