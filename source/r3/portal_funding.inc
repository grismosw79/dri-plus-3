/* Test12: the native interaction evaluator owns a 72-byte candidate vector and
   an index permutation. Reweight only unsecured, legal CPU portal targets in
   each native stage budget. Preserve the EXACT original total in each bucket,
   all HS scores, the team's hours/NIL budgets, per-player caps and action gates.
   Native accounting still spends the money/hours and creates the actual offers. */
typedef struct {PortalNativeCandidate candidate;U target;int32_t extra[4];} PortalFundingCandidate;
_Static_assert(sizeof(PortalFundingCandidate)==72 && offsetof(PortalFundingCandidate,target)==48,"Funding ABI");
#include "test31_funding_scope.inc"
#include "test29_source_funding.inc"
#include "test36_nil.inc"
#include "portal29_observe.inc"
#include "portal29_execution.inc"
#include "portal29_admit.inc"
#include "test29r1_funding.inc"
#include "test32_hs_funding.inc"
/* Runs after both native funding-allocation passes, before their reset.
   Observe funded actions and recorded execution separately; never spend,
   offer, or change a board here. */
/* Cache stack: the two observers run inside one window (closed right after them, and again after an exception). */
static int fund_observe_window;
void DriAfterHsFunding(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode!=1 || faulted)return;
 __try {
  hs29_offer_invalidate(); /* Native funding may have created same-week offers. */
  U eval=r->rsi,holder=q(r->rbp-0x60);Rec team=holder?(Rec)q(holder):0;
  if(team)fund_observe_window=cache_callback_window("FUND_OBSERVE",eval);
  if(team && hs28_pair_window(eval))hs28_coverage_observe(team,eval,"POST_FUNDING");
  if(team && !faulted){portal29_observe(team,eval,"POST_FUNDING");
   cache_callback_window_end(fund_observe_window);fund_observe_window=0;
   portal29_funding_end(team,eval);hs33_release_school(team,eval);r2_funding_tick(team,eval);}
 } __except(fail_exception("HS_post_funding",GetExceptionCode())){}
 if(fund_observe_window){cache_callback_window_end(fund_observe_window);fund_observe_window=0;}
}
static void hs28_funding(Registers *r,Rec team,U eval,int after_offers) {
 if(!hs28_pair_window(eval))return;
 U begin=q(r->rbp-0x50),end=q(r->rbp-0x48),ib=q(r->rbp+0x58),ie=q(r->rbp+0x60);
 if(end<begin || (end-begin)%72 || (end-begin)/72>MAX_BOARD || ie<ib || ie-ib!=((end-begin)/72)*8){portal_fault("HS_pair_funding_bounds");return;}
 int n=(int)((end-begin)/72);if(!n)return;
 PortalFundingCandidate *a=(void*)begin;U *indices=(void*)ib;int seen[MAX_BOARD]={0};
 for(int j=0;j<n;j++)if(indices[j]>=(U)n || seen[indices[j]]++){portal_fault("HS_pair_funding_indices");return;}
 int has_hs=0;
 for(int j=0;j<n;j++)if(a[j].candidate.rec && hs15_is_recruit((Rec)a[j].candidate.rec) && scalar((Rec)a[j].candidate.rec,9,0x108)<5 && a[j].candidate.values[8]>0){has_hs=1;break;}
 if(!has_hs)return;
 Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int np=0;
 if(!hs15_collect(&shot,team,eval)){portal_fault("HS_pair_funding_snapshot");return;}
 /* The first native pass has finished all scholarship attempts. Release only
    current CPU HS proposals that failed to become real offers; then choose a
    fresh primary/backup before the second native pass computes allowances. */
 if(after_offers)hs33_refresh_school(team,eval);
 if(after_offers)for(int j=0;j<n;j++) {
  Rec rec=(Rec)a[j].candidate.rec,target=(Rec)a[j].target;
  if(!rec || !target || !hs15_is_recruit(rec) || scalar(rec,9,0x108)>=5 || a[j].candidate.values[8]<=0)continue;
  unsigned status=scalar(target,12,0x108);if(status>=2 && status<=4)continue;
  Rec player=reference(rec,5);int acquired=player && hs15_has(&shot,player);release(player);
  if(!acquired)hs33_release_offer(rec,team,eval);
 }
 int denied=0;
 for(int attempt=0;attempt<3;attempt++) {
  if(!hs28_make_board_plan(team,eval,&shot,pool,&np,0)){portal_fault("HS_pair_funding_plan");return;}
  denied=hs33_plan_published(pool,np,team,eval);if(!denied)break;
 }
 if(denied) {
  hs33_plan_strict(team,eval,1);
  int ok=hs28_make_board_plan(team,eval,&shot,pool,&np,0);
  if(ok)denied=hs33_plan_published(pool,np,team,eval);
  hs33_plan_strict(team,eval,0);
  if(!ok || denied){portal_fault("HS33_funding_plan_membership");return;}
 }
 int slot[MAX_BOARD],bucket[MAX_BOARD],weight[MAX_BOARD],value[MAX_BOARD],order[MAX_BOARD],m=0;
 int64_t total[2]={0},assigned[2]={0};int wt[2]={0},counts[2]={0};
 for(int j=0;j<n;j++) {
  Rec rec=(Rec)a[j].candidate.rec;
  if(!rec || !hs15_is_recruit(rec) || scalar(rec,9,0x108)>=5 || a[j].candidate.values[8]<=0)continue;
  /* Native vectors can still contain an acquired recruit with a soft-looking
     stage. The shared plan excludes that incoming player. Preserve its native
     candidate bytes and index position instead of treating it as a missing
     active pursuit or including its score in the HS funding redistribution. */
  Rec player=reference(rec,5);if(!player){portal_fault("HS_pair_funding_player");return;}
  int acquired=hs15_has(&shot,player);release(player);if(acquired)continue;
  int k=hs28_pool_index(pool,np,rec);if(k<0){portal_fault("HS_pair_funding_identity");return;}
  Rec target=(Rec)a[j].target,linked=target?reference(target,10):0;
  int match=hs28_identity(&pool[k],linked);release(linked);
  if(!match){portal_fault("HS_pair_funding_target");return;}
  int b=a[j].extra[2]>=1 && a[j].extra[2]<=5;
  unsigned status=scalar(target,12,0x108);int offered=status>=2 && status<=4;
  int eligible=pool[k].legal && pool[k].viable &&
   (offered || (!after_offers && pool[k].selected && !hs32_admission_block(rec,team,eval)));
  slot[m]=j;bucket[m]=b;weight[m]=eligible?(pool[k].selected?pool[k].weight:1):0;
  order[m]=eligible?(pool[k].selected?pool[k].order:1000+j):2000+j;
  total[b]+=a[j].candidate.values[8];wt[b]+=weight[m];counts[b]+=weight[m]>0;m++;
 }
 for(int b=0;b<2;b++)if(total[b]>0x7fffffff){portal_fault("HS_pair_funding_total");return;}
 for(int j=0;j<m;j++) {
  int b=bucket[j];
  /* A source/stage bucket with no executable peer remains numerically inert:
     conserve its native denominator, but both native action gates refuse its
     unoffered targets. Never move those hours to another source or stage. */
  value[j]=!wt[b]?a[slot[j]].candidate.values[8]:weight[j]?1+(int)((total[b]-counts[b])*weight[j]/wt[b]):0;
  assigned[b]+=value[j];
 }
 for(int j=0;j<m;j++)if(weight[j]>0 && assigned[bucket[j]]<total[bucket[j]]){value[j]++;assigned[bucket[j]]++;}
 for(int b=0;b<2;b++)if(assigned[b]!=total[b]){portal_fault("HS_pair_funding_conservation");return;}
 /* The captured first allocation loop uses the index permutation, but the
    second walks the physical72-byte vector. Put the same primary-first order
    in BOTH views. Entire candidate/target records travel together; unrelated
    portal, acquired and nonpositive entries retain their physical/index slots. */
 int priority[MAX_BOARD];for(int j=0;j<m;j++)priority[j]=j;
 for(int j=1;j<m;j++){int k=priority[j],i=j;while(i>0 && order[priority[i-1]]>order[k]){priority[i]=priority[i-1];i--;}priority[i]=k;}
 PortalFundingCandidate copy[MAX_BOARD];memcpy(copy,a,(size_t)n*sizeof(*a));
 for(int j=0;j<m;j++) {
  int k=priority[j];a[slot[j]]=copy[slot[k]];a[slot[j]].candidate.values[8]=value[k];
  log_line("HS28_FUNDING,%u,%u,%d,%d,%d",scalar(team,383,0x100),row((Rec)a[slot[j]].candidate.rec),weight[k],value[k],bucket[k]);
  if(!weight[k])log_line("TEST33_HS_FUNDING_HOLD,%u,%u,%s,%s,%d",scalar(team,383,0x100),row((Rec)a[slot[j]].candidate.rec),
   after_offers?"AFTER_OFFERS":"BEFORE_OFFERS",wt[bucket[k]]?"REALLOCATED_SAME_BUCKET":"NO_ELIGIBLE_PEER_INERT",value[k]);
 }
 int next=0;
 for(int j=0;j<n;j++)for(int k=0;k<m;k++)if(indices[j]==(U)slot[k]){indices[j]=(U)slot[next++];break;}
 test32_hs_minimum_share(team,eval,a,n,indices,pool,np);
}
void DriOnPortalFunding(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode!=1 || faulted)return;
 __try {
  U eval=r->rsi,tr=q(r->rbp-0x60);Rec team=tr?(Rec)q(tr):0;
  if(!team)return;
  Rec user=reference(team,407);if(user){release(user);return;}
  test31_offer_register(eval);
  U pass_begin=q(r->rbp-0x50),pass_end=q(r->rbp-0x48);
  if(pass_end<pass_begin || (pass_end-pass_begin)%72 || (pass_end-pass_begin)/72>MAX_BOARD){portal_fault("funding31_pass_bounds");return;}
  test31_funding_record(team,eval,(PortalFundingCandidate *)pass_begin,(int)((pass_end-pass_begin)/72));if(faulted)return;
  test29_class_invalidate(team); /* Native board/ownership may have changed since the preceding pass. */
  hs28_funding(r,team,eval,0);if(faulted || hs28_regular(eval))return;
  Rec f=context_ref(eval,6),season=f?reference(f,44):0;
  int active=season && scalar(season,5,0x108)==6 && scalar(season,4,0x100)>=2 && scalar(season,4,0x100)<=6;
  release(season);release(f);if(!active)return;
  unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
  portal29_funding_begin(team,eval);
  U begin=q(r->rbp-0x50),end=q(r->rbp-0x48),ib=q(r->rbp+0x58),ie=q(r->rbp+0x60);
  if(end<begin || (end-begin)%72 || (end-begin)/72>MAX_BOARD || ie<ib || ie-ib!=((end-begin)/72)*8) {portal_fault("funding_bounds");return;}
  int n=(int)((end-begin)/72);if(!n)return;
  PortalFundingCandidate *a=(void *)begin,copy[MAX_BOARD];PortalChoice pool[MAX_BOARD];
  PortalQuality rooms[POSITIONS];int prestige=0,np=0,slots[MAX_BOARD],bucket[MAX_BOARD],weights[MAX_BOARD],values[MAX_BOARD];
  int seen[MAX_BOARD]={0};U *indices=(void *)ib;
  for(int j=0;j<n;j++) {if(indices[j]>=(U)n || seen[indices[j]]++){portal_fault("funding_indices");return;}}
  test29_hs_funding_priority(team,eval,a,n,indices);if(faulted)return;
  if(!portal_quality_rooms(rooms,0,&prestige,team,eval)){portal_fault("funding_room");return;}
  int reserved[22]={0};test29_hs_reserved(team,eval,reserved);if(faulted)return;
  dri_portal_set_hs_reservations(rooms,reserved,prestige);
  for(int j=0;j<n;j++) {
   Rec rec=(Rec)a[j].candidate.rec;if(!rec || !transfer_recruit(rec) || (portal_academy(tid) || !portal_academy_allowed(rec,tid)) || scalar(rec,9,0x108)>=5 || a[j].candidate.values[8]<=0)continue;
   Rec target=(Rec)a[j].target,linked=target?reference(target,10):0;
   int match=portal_same_record(rec,linked);release(linked);
   if(!match){portal_fault("funding_identity");return;}
   Rec player=reference(rec,5);if(!player){portal_fault("funding_player");return;}
   int pos=(int)scalar(player,3,0x108),ovr=(int)scalar(player,191,0x100);release(player);
   if(pos<0 || pos>=POSITIONS || ovr<0 || ovr>99){portal_fault("funding_values");return;}
   int raw_ovr=ovr;ovr=portal_projected_ovr(ovr,portal_projection_step(eval));
   int role=dri_portal_role_candidate(&rooms[pos],prestige,ovr,raw_ovr);if(!role)continue;
   /* Never infer the native fit by subtracting a fresh-room bonus. The score
      hook retained the latest exact decomposition. If any participating
      entry lacks matching provenance, preserve the whole native vector and
      its index order for this callback, with a diagnostic instead of a fault. */
   pool[np]=(PortalChoice){.pos=pos,.role=role,.score=a[j].candidate.values[8],.existing=1,
    .index=j,.rid=row(rec),.rec=(U)rec,.ovr=ovr,
    .plan_score=a[j].candidate.values[8],.raw_ovr=raw_ovr};
   if(!portal_choice_race(&pool[np],target,team,eval)){portal_fault("funding_race");return;}
   slots[np++]=j;
  }
  if(!np){test29r1_recovery_funding(team,eval,a,n,indices);return;}
  int missing=portal_funding_bases(eval,(U)team,pool,np);
  if(missing){log_line("PORTAL_PLAN_SKIP,FUNDING,%u,%u,SCORE_PROVENANCE",tid,pool[missing-1].rid);return;}
  market29_portal_apply(pool,np,team,eval,rooms,prestige);if(faulted)return;
  rc4_compare_team=(int)tid;rc4_compare_site="FUNDING";
  dri_portal_order(pool,np,np,rooms,prestige);rc4_compare_site=0;
  portal_commit_plan_store(pool,np,team,eval,"FUNDING");
  /* Slot and round survive the physical reorder. Move the funded candidates for
     the first empty-position seat ahead of every optional native purchase. */
  for(int i=1;i<np;i++) {
   PortalChoice v=pool[i];int j=i;
   int rank=v.selected && v.slot==1 && rooms[v.pos].count==0 && v.round<=2?v.round:3;
   while(j>0) {
    PortalChoice *prev=&pool[j-1];
    int last=prev->selected && prev->slot==1 && rooms[prev->pos].count==0 && prev->round<=2?prev->round:3;
    if(last<=rank)break;
    pool[j]=*prev;j--;
   }
   pool[j]=v;
  }
  int64_t totals[2]={0},allocated[2]={0};int weight_total[2]={0},counts[2]={0};
  for(int j=0;j<np;j++) {
   int stage=a[pool[j].index].extra[2]; /* native +40 stage bucket; Top5..Hard vs Top10 */
   bucket[j]=stage>=1 && stage<=5;
   weights[j]=pool[j].round==1?3:pool[j].round==2?2:1;
   if(pool[j].selected && pool[j].slot==1 && rooms[pool[j].pos].count==0 && pool[j].round<=2)
    weights[j]=pool[j].round==1?9:6;
   counts[bucket[j]]++;totals[bucket[j]]+=pool[j].score;weight_total[bucket[j]]+=weights[j];
  }
  for(int b=0;b<2;b++)if(totals[b]>0x7fffffff){portal_fault("funding_total");return;}
  for(int j=0;j<np;j++) {
   int b=bucket[j];values[j]=1+(int)((totals[b]-counts[b])*weights[j]/weight_total[b]);
   allocated[b]+=values[j];
  }
  for(int j=0;j<np;j++)if(allocated[bucket[j]]<totals[bucket[j]]){values[j]++;allocated[bucket[j]]++;}
  for(int b=0;b<2;b++)if(allocated[b]!=totals[b]){portal_fault("funding_conservation");return;}
  /* All validation and apportionment precede the first native-memory write. */
  memcpy(copy,a,(size_t)n*sizeof(*a));
  for(int j=0;j<np;j++) {
   a[slots[j]]=copy[pool[j].index];a[slots[j]].candidate.values[8]=values[j];
   log_line("PORTAL_FUNDING,%u,%u,%d,%d,%d,%d,%d,%d,NATIVE_BUDGET",tid,pool[j].rid,pool[j].pos,pool[j].slot,pool[j].role,pool[j].round,pool[j].score,values[j]);
   if(log30_deep) {
    PortalChoice *p=&pool[j];
    log_line("PORTAL_PLAN,FUNDING,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",tid,p->rid,p->pos,p->score,p->native_base,p->plan_score,p->offered,p->influence,p->leader,p->commit_score,p->stage,p->race_ok,p->existing,p->selected);
   }
  }
  /* Preserve HS/secured entries' physical positions and native index slots.
     Put the portal primary coverage first in both native interaction passes. */
  int next=0;
  for(int j=0;j<n;j++) {
   int portal=0;for(int k=0;k<np;k++)if(indices[j]==(U)slots[k]){portal=1;break;}
   if(portal)indices[j]=(U)slots[next++];
  }
  test29r1_recovery_funding(team,eval,a,n,indices);
 } __except(fail_exception("portal_funding",GetExceptionCode())){}
}

/* Safe boundary after the indexed scholarship pass and before the physical
   productive-action pass. There is no active candidate iterator to mutate. */
void DriBetweenHsFunding(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(mode!=1 || faulted)return;
 __try {
  U eval=r->rsi,holder=q(r->rbp-0x60);Rec team=holder?(Rec)q(holder):0;if(!team)return;
  Rec user=reference(team,407);if(user){release(user);return;}
  hs29_offer_invalidate();
  hs28_funding(r,team,eval,1);
 } __except(fail_exception("HS_between_funding",GetExceptionCode())){}
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest33BetweenHsFunding(Registers *r){DriBetweenHsFunding(r);}
#endif
