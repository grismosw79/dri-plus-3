/* Compiled only in the instrumented test DLL. These entry points call the
   production callbacks and are excluded from the Windows distribution DLL. */
__declspec(dllexport) int DriR2H1Instant(U manager, Rec r, Rec team, Rec target, int price) {
    t36_designated = r;
    t36_destination = team;
    int result = t36_accept_opening(manager, r, team, target, price);
    t36_designated = 0;
    t36_destination = 0;
    return result;
}
__declspec(dllexport) U DriR2H1Roll(void) { return (U)t36_instant_roll; }
__declspec(dllexport) int DriTest36Price(int previous, int current, int ask, int rival, int behind,
                                         int available) {
    return t36_nil_price(previous, current, ask, rival, behind, available);
}
__declspec(dllexport) int DriTest36Pay(U eval, Rec team, Rec target, int amount) {
    return t36_bid_debit(eval, team, target, amount, 1);
}
__declspec(dllexport) int DriTest36Funds(U e, Rec t) { return t36_transfer_budget(e, t); }
__declspec(dllexport) int DriTest36Leave(U e, Rec a, Rec p) { return t36_leave(e, a, p); }
__declspec(dllexport) void DriTest36Release(U e, Rec t, Rec requests, const int *status) {
    t36_position_release(e, t, requests, status, 2026, 0);
}
__declspec(dllexport) int DriTest36Room(int pos, int ideal, int starter, int n, const int *ratings,
                                        const int *incoming, int candidate, int excess) {
    PortalQuality q0 = {0};
    q0.pos = pos;
    q0.ideal = ideal;
    q0.starters = starter;
    q0.count = n;
    q0.incoming_best = -1;
    q0.team_excess = excess;
    q0.exception_states = DRI_EXCEPTION_READY;
    for (int i = 0; i < n; i++) {
        q0.rating[i] = ratings[i];
        q0.incoming[i] = incoming[i];
        if (incoming[i] && ratings[i] > q0.incoming_best)
            q0.incoming_best = ratings[i];
        else if (!incoming[i])
            q0.base_count++;
    }
    return dri_quality_block_candidate(&q0, 8, candidate, candidate - 2);
}
__declspec(dllexport) int DriTest36OfferCap(Rec r, Rec t, U e) { return test32_hs_offer_capacity(r, t, e); }
__declspec(dllexport) int DriTest36Promotion(Rec f, Rec source, Rec dest, int tierFrom, int tierTo,
                                             int prestigeFrom, int prestigeTo, int snapshot) {
    static T36School schools[138];
    memset(schools, 0, sizeof(schools));
    unsigned from = scalar(source, 383, 0x100), to = scalar(dest, 383, 0x100);
    if (from >= 138 || to >= 138)
        return -1;
    if (snapshot)
        t36_capture_hcs(f, 2026);
    schools[from].team = source;
    schools[from].tier = tierFrom;
    schools[from].prestige = prestigeFrom;
    schools[to].team = dest;
    schools[to].tier = tierTo;
    schools[to].prestige = prestigeTo;
    return t36_promotion(schools + to, schools, 2026);
}
__declspec(dllexport) void DriTest36MarkPaid(Rec t, int after) { t36_pay_by_team(t, after); }
__declspec(dllexport) int DriTest36HsCase(Rec r, Rec team, U eval, int star, int score, int offered,
                                          int completed) {
    Hs32Recruit *m = &hs32_market.recruits[0];
    memset(m, 0, sizeof(*m));
    memset(hs32_market.hash, 0, sizeof(hs32_market.hash));
    memset(hs32_market.completed, 0, sizeof(hs32_market.completed));
    unsigned tid = scalar(team, 383, 0x100);
    hs32_market.ready = 1;
    hs32_market.eval = eval;
    hs32_market.building = 0;
    hs28_scope(&hs32_market.scope, team, eval);
    m->table = q((U)r + 0x10);
    m->rid = row(r);
    m->admissions.stars = star;
    unsigned h = (unsigned)((m->table >> 4) ^ ((U)m->rid * 2654435761u)) & (HS32_HASH - 1);
    hs32_market.hash[h] = 1;
    m->authorized = 12;
    uint64_t live[3] = {0};
    for (int t = 0; t < 12; t++)
        market29_bit(m->pressure.protected_offer, (unsigned)t, 1);
    for (int t = 0; t < offered; t++)
        market29_bit(live, (unsigned)t, 1);
    market29_bit(hs32_market.completed, tid, completed);
    return test36_hs_grant(r, team, eval, score, live);
}
__declspec(dllexport) int DriTest36Leading(int place, int score, int leader, int runner) {
    return t36_in_leading_group(place, score, leader) && t36_decisive(score, runner);
}
__declspec(dllexport) int DriTest36Mutual(U eval, Rec r, Rec dest, Rec source, int rating, int incumbent,
                                          int source_incumbent, int follow, int reason) {
    static T36School d, o;
    memset(&d, 0, sizeof(d));
    memset(&o, 0, sizeof(o));
    d.team = dest;
    o.team = source;
    Rec p = reference(r, 5);
    int pos = (int)scalar(p, 3, 0x108);
    PortalQuality *q = &d.room[pos], *old = &o.room[pos];
    q->pos = old->pos = pos;
    q->ideal = old->ideal = 2;
    q->starters = old->starters = 1;
    if (incumbent >= 0) {
        q->rating[0] = incumbent;
        q->count = 1;
    }
    if (source_incumbent >= 0) {
        old->rating[0] = source_incumbent;
        old->count = 1;
    }
    T36Candidate c = {.pos = pos, .rating = rating, .follow = follow, .recruit = r};
    int result = t36_mutual(&c, &d, &o, p, reason, eval, eval, 0, 0);
    release(p);
    return result;
}

__declspec(dllexport) int DriTest36ArrivalPrice(int ask, int live, int from, int reason) {
    return t36_opening_price(ask, live, from, reason);
}

__declspec(dllexport) int DriR2Score(Rec r,Rec t,U e,U *c,int selection,int *out){(void)selection;return market29_portal_score_cached(r,t,e,c,out);}
__declspec(dllexport) int DriR2Grace(U e,Rec f,unsigned week,int post){return r2_seed_grace(e,q((U)f+0x10),row(f),2026,week,post);}
__declspec(dllexport) void DriR2Mark(U e,Rec t,unsigned week,unsigned paid){r2_ordinary_funding(e,t,2026,week,paid);}
__declspec(dllexport) int DriR2Cash(U e,Rec t,Rec bid,int value){return t36_bid_debit(e,t,bid,value,0);}
__declspec(dllexport) int DriR2Wallet(U e,Rec t){int a,b,c;return t36_funds(e,t,&a,&b,&c)?r2_wallet(a,b,c):-1;}
__declspec(dllexport) int DriR2SavedOwner(Rec r,U e,int *owner){return r2_saved_owner(r,e,owner);}
__declspec(dllexport) void DriR2Origin(Rec f){Rec s=reference(f,44);t36_capture_prior(f,s,1);release(s);}
__declspec(dllexport) void DriR2Hire(Rec module,Rec coach,Rec dest,int role,int length){r2_hc_hire(module,coach,dest,role,length);}
__declspec(dllexport) int DriR2Move(Rec f,Rec t,Rec old){
 static T36School schools[138];memset(schools,0,sizeof(schools));unsigned from=scalar(old,383,0x100),to=scalar(t,383,0x100);
 if(from>=138 || to>=138)return -1;
 schools[from].team=old;schools[to].team=t;r2_hire_league=scalar(f,28,0x100);return t36_promotion(schools+to,schools,2026);
}
__declspec(dllexport) void DriR2Cold(void){memset(r2_hires,0,sizeof(r2_hires));memset(t36_old_hc,0,sizeof(t36_old_hc));t36_captured_f=0;r2_hire_league=0;}
__declspec(dllexport) int DriR2BoardFixture(U e,Rec team,Rec r,Rec weak,int selected,int lead,int mode0){
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 hs28_market_scope(e,1);hs28_market.valid=1;hs28_market.ready=1;
 Hs28MarketRecruit *m=hs28_market.recruits;memset(m,0,sizeof(*m));m->table=q((U)r+0x10);m->rid=row(r);m->stars=3;
 unsigned h=(unsigned)((m->table>>4)^((U)m->rid*2654435761u))&(HS28_MARKET_HASH-1);hs28_market.hash[h]=1;hs28_market.count=1;
 Hs28RecoveryTeam *c=hs28_recovery_teams+tid;c->n=1;c->valid=1;
 c->plan[0]=(Hs28Choice){.table=q((U)weak+0x10),.rid=row(weak),.stars=1,.selected=selected,.rank=lead?1:4,.points=lead?27:0,.native_score=12};
 if(!mode0)return r2_hs_spares(team,e,c->plan,1,0);
 return r2_hs_add(team,r,e,338);
}
__declspec(dllexport) int DriR2Lone(Rec r,Rec t,U e){return r2_portal_live(r,t,e);}
__declspec(dllexport) void DriR2FundingTick(Rec t,U e){r2_funding_tick(t,e);}
__declspec(dllexport) U DriR2BeforeActions(U e,Rec t){return r2_before_actions(e,&t);}
__declspec(dllexport) int DriR2BatchClosed(void){return r2_funding_batch.closed;}
__declspec(dllexport) int DriR2PayOrdinary(Rec r,Rec t,U e,int cost){
 R2Pursuit v={.school=(int)scalar(t,383,0x100),.cost=cost,.slot=33,.role=1,.merit=338};
 return r2_fund(r,t,e,0,1,&v);
}
__declspec(dllexport) int DriR2CapRotate(unsigned live,unsigned chosen){
 Market29PortalRecruit *m=market29_portal.recruits;uint64_t actual[3]={0};
 for(unsigned i=0;i<live && i<138;i++)market29_bit(actual,i,1);
 for(unsigned i=0;i<6;i++)market29_bit(m->pressure.hard_authorized,20+i,1);
 r2_offer_set(m,actual,live<6?(int)chosen:-1);return market30_count(m->pressure.hard_authorized);
}

__declspec(dllexport) int DriH2ScoreWeek(Rec r,Rec t,U e,U *c,int week,int *out){return market29_portal_score_week(r,t,e,c,week,out);}
/* H3-only offline adapters; absent from the shipping DLL. */
__declspec(dllexport) int DriH3Pursuit(const int *v) {
 if(!v || v[0]<0 || v[0]>=22 || v[3]<0 || v[3]>11 || v[4]<0 || v[4]>=192)return -1;
 PortalQuality room={.pos=v[0],.ideal=v[2],.starters=v[3],.count=v[4]};
 for(int j=0;j<room.count;j++)room.rating[j]=v[10+j];
 PortalChoice p={.ovr=v[5],.raw_ovr=v[6],.offered=v[7],.market_protected=v[8],.coverage_priority=v[9]};
 return dri_portal_choice_pursuit_percent(&p,&room,v[1]);
}
__declspec(dllexport) int DriH3Scale(int value,int percent){return dri_portal_pursuit_scale(value,percent);}

/* R3 adapters use production collectors and mutation paths. */
__declspec(dllexport) int DriR3Rooms(Rec team,U eval,int projected,int raw,int hs) {
 if(hs){Hs15Snapshot s;PortalQuality q0[22];
  if(!hs15_collect(&s,team,eval) || !hs15_portal_rooms(&s,q0,s.prestige,s.projection_step))return -1;
  return dri_quality_block_candidate(q0,s.prestige,projected,raw);
 }
 PortalRoster r;if(!portal_roster(&r,team,eval,0))return -1;
 return dri_quality_block_candidate(&r.projected,r.prestige,projected,raw);
}
__declspec(dllexport) int DriR3QbPriority(Rec p){return r3_qb_priority(p);}
__declspec(dllexport) void DriR3PrePool(U eval){r3_qb_pre_pool(eval);}
__declspec(dllexport) int DriR3Supplement(U eval,Rec f,Rec requests,int *status) {
 unsigned before=(unsigned)array_count(requests);
 r3_qb_initial(eval,f,requests,status,2026,0);
 return (int)array_count(requests)-(int)before;
}
__declspec(dllexport) int DriR3RecoveryNeeded(int portal,int stars,int stage,int working,int owned){return r3_recovery_needed(portal,stars,stage,working,owned);}
__declspec(dllexport) void DriR3AfterActions(U eval,Rec team){r3_after_actions(eval,team);}
__declspec(dllexport) int DriR3PostClosed(void){return r2_funding_batch.post_closed;}
__declspec(dllexport) void DriR3CloseBatch(U eval){r2_funding_batch.scope=r2_pass(eval);r2_funding_batch.closed=1;r2_funding_batch.post_closed=0;memset(r2_funding_batch.acted,0,sizeof(r2_funding_batch.acted));}
__declspec(dllexport) int DriR3OpeningGuard(Rec recruit,Rec team,U eval){return r3_qb_admission(recruit,team,eval,"FIXTURE_OPENING");}
