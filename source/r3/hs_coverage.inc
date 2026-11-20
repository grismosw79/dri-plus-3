/* R7: one primary and one backup per approved opening. All writes use the
   verified native candidate/replacement vector ABI. No offer or roster is
   synthesized. Unselected candidates cannot pass the admission hook. */
static int hs28_pool_index(Hs28Choice *pool,int n,Rec rec) {
 for(int j=0;j<n;j++)if(hs28_identity(&pool[j],rec))return j;
 return -1;
}
static int hs28_admit(Registers *r,Rec rec) {
 PortalCache *c=portal_cache(0);
 if(mode!=1 || faulted || !c || !c->hs_valid || !c->allocated || c->frame!=r->rbp)return 0;
 return hs32_admission_block(rec,(Rec)c->team,c->eval) || hs28_pool_index(c->hs_choices,c->hs_count,rec)<0;
}
/* Audit the reservation against the actual post-shortlist pool, before writes. */
static void hs29_trace_reserved(Hs28Choice *pool,int n,PortalNativeCandidate *a,U count,Rec team,U eval) {
 if(!hs28_market_scope(eval,0))return;
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *r=&hs28_market.recovery[tid][i];Rec rec=0;int score=0,k=-1;
  for(U j=0;j<count;j++)if(a[j].rec && q(a[j].rec+0x10)==r->table && row((Rec)a[j].rec)==r->rid){rec=(Rec)a[j].rec;score=a[j].values[8];break;}
  for(int j=0;j<n;j++)if(pool[j].table==r->table && pool[j].rid==r->rid){k=j;break;}
  Hs28Choice *p=k>=0?&pool[k]:0;
  int eligible=rec?native_recruit_eligible(eval,team,rec):-1;
  const char *why=p && p->selected?"SELECTED":eligible==0?"NATIVE_INELIGIBLE":!rec && !p?"ABSENT":!p?"NONPOSITIVE_NATIVE_SCORE":!p->legal?"NO_LEGAL_OPENING_OR_RACE":!p->viable?"RACE_NOT_VIABLE":"PLAN_CAPACITY_OR_OBLIGATION";
  log_line("HS29_RESERVED_PATH,%u,%u,%d,%d,%d,%d,%d,%d,%s",tid,r->rid,r->slot,r->role,score,eligible,p?p->slot:0,p?p->choice:0,why);
  if(faulted)return;
 }
}
static void hs25_allocate(Registers *r) {
 PortalCache *c=portal_cache(0);
 if(mode!=1 || faulted || !c || !c->hs_valid || c->allocated || c->frame!=r->rbp)return;
 U sp=(U)(r+1),vec=q(r->rbp-0x68);if(!vec)return;
 U begin=q(vec),end=q(vec+8),count=q(sp+0x68);
 U rb=q(sp+0x78),re=q(r->rbp-0x80),bc=q(sp+0x50);int capacity=i32(r->rbp+0x30);
 if(end<begin || (end-begin)%48 || count!=(end-begin)/48 || count>PORTAL_PLAN_LIMIT ||
    re<rb || (re-rb)%48 || (re-rb)/48>MAX_BOARD || bc>MAX_BOARD || (re-rb)/48>bc || capacity<0 || capacity>50) {
  portal_fault("HS_pair_vector_bounds");return;
 }
 int nr=(int)((re-rb)/48),fixed=0,n=0,vacancy[22],optional[22];
 int general=hs28_rooms(&c->hs_snapshot,vacancy,optional);
 Hs28Choice board[MAX_BOARD];int bn=0;
 if(general<0 || !hs28_board((Rec)c->team,c->eval,&c->hs_snapshot,board,&bn,&fixed,0)){portal_fault("HS_pair_board");return;}
 Hs28Choice *pool=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+bn+1)*sizeof(*pool));
 PortalNativeCandidate *copy=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(count+1)*sizeof(*copy));
 unsigned char *copied=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,count+1);
 if(!pool || !copy || !copied){portal_fault("HS_pair_memory");goto done;}
 PortalNativeCandidate *a=(void*)begin,*rem=(void*)rb;
 for(int i=0;i<bn;i++){pool[n]=board[i];pool[n++].index=-1;}
 for(U j=0;j<count;j++) {
  Rec rec=(Rec)a[j].rec;
  if(!rec || !hs15_is_recruit(rec) || scalar(rec,9,0x108)>=5 || a[j].values[8]<=0)continue;
  int k=hs28_pool_index(pool,n,rec);
  if(k>=0) {
   if(pool[k].index>=0){portal_fault("HS_pair_duplicate");goto done;}
   pool[k].index=(int)j;pool[k].native_score=a[j].values[8];continue;
  }
  if(!hs28_choice(&pool[n],rec,0,(Rec)c->team,c->eval,&c->hs_snapshot,0)){portal_fault("HS_pair_candidate");goto done;}
  pool[n].native_score=a[j].values[8];pool[n++].index=(int)j;
 }
 for(int j=0;j<nr;j++) {
  int k=hs28_pool_index(pool,n,(Rec)rem[j].rec);
  if(k>=0 && !pool[k].native_score)pool[k].native_score=rem[j].values[8];
 }
 if(faulted)goto done;
 hs33_prepare_native_pool(pool,n,(Rec)c->team,c->eval);
 int chosen=0,denied=0;
 /* Reconcile real shortlist membership before publishing native candidates.
    A final strict pass uses only current permits, preserving the shared
    primary/backup planner instead of removing individual selected rows. */
 for(int attempt=0;attempt<3;attempt++) {
  chosen=hs28_plan_team(pool,n,vacancy,optional,general,capacity-fixed,(Rec)c->team,c->eval,&c->hs_snapshot);
  hs28_coverage_repair(pool,n,(Rec)c->team,c->eval);
  if(faulted)goto done;
  denied=hs33_plan_published(pool,n,(Rec)c->team,c->eval);
  if(!denied)break;
 }
 if(denied) {
  hs33_plan_strict((Rec)c->team,c->eval,1);
  chosen=hs28_plan_team(pool,n,vacancy,optional,general,capacity-fixed,(Rec)c->team,c->eval,&c->hs_snapshot);
  hs28_coverage_repair(pool,n,(Rec)c->team,c->eval);
  denied=hs33_plan_published(pool,n,(Rec)c->team,c->eval);
  hs33_plan_strict((Rec)c->team,c->eval,0);
  if(denied){portal_fault("HS33_strict_plan_membership");goto done;}
 }
 hs29_trace_reserved(pool,n,a,count,(Rec)c->team,c->eval);
 if(faulted)goto done;
 if(chosen>50){portal_fault("HS_pair_capacity");goto done;}
 if(log30_deep) {
  for(int j=0;j<n;j++)if(pool[j].crowd_penalty)
   log_line("HS32_CROWDING,%u,%u,%d,%d,%d",scalar((Rec)c->team,383,0x100),pool[j].rid,pool[j].native_score,pool[j].crowd_penalty,pool[j].selected);
 }
 U written=0;int selected_existing_unscored=0;
 for(int order=1;order<=chosen;order++)for(int j=0;j<n;j++)if(pool[j].selected && pool[j].order==order) {
  int index=pool[j].index;
  if(index>=0) {
   if((U)index>=count || copied[index] || written>=count){portal_fault("HS_pair_index");goto done;}
   copy[written++]=a[index];copied[index]=1;
  } else if(pool[j].existing)selected_existing_unscored++;
 }
 /* Selection is by recruit identity; permutation is by physical vector slot.
    An existing selected recruit may have no positive-score main entry, or a
    nonpositive alias may share its identity. Preserve those entries exactly
    once in the tail. They must not disappear merely because the recruit won
    a place in the pair plan. Native scores and reference ownership stay intact. */
 for(U j=0;j<count;j++) {
  if(!copied[j])copy[written++]=a[j];
 }
 if(written!=count){portal_fault("HS_pair_permutation");goto done;}
 /* Publish only a completely validated plan. */
 c->hs_count=0;
 for(int order=1;order<=chosen;order++)for(int j=0;j<n;j++)if(pool[j].selected && pool[j].order==order)
  c->hs_choices[c->hs_count++]=pool[j];
 hs28_plan_store(c->hs_choices,c->hs_count,(Rec)c->team,c->eval);
 hs28_coverage_reserve_plan(c->hs_choices,c->hs_count,(Rec)c->team,c->eval);
 /* Validate before writing. Every protected replacement owns one native
    reference which is released when removed from the replacement vector. */
 memcpy(a,copy,count*sizeof(*a));int kept=0;
 for(int j=0;j<nr;j++) {
  int k=hs28_pool_index(pool,n,(Rec)rem[j].rec);
  if(k>=0 && pool[k].selected)release((Rec)rem[j].rec);
  else {if(kept!=j)rem[kept]=rem[j];kept++;}
 }
 if(nr>kept)memset(rem+kept,0,(nr-kept)*sizeof(*rem));
 *(U *)(r->rbp-0x80)=rb+(U)kept*48;c->allocated=1;
 log_line("HS28_ALLOC,%u,%u,%d,%d,%d,%d,%d",scalar((Rec)c->team,383,0x100),(unsigned)count,bn,chosen,selected_existing_unscored,nr,kept);
 for(int j=0;j<c->hs_count;j++) {
  Hs28Choice *p=&c->hs_choices[j];
  log_line("HS28_PAIR,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d",scalar((Rec)c->team,383,0x100),p->rid,p->pos,p->slot,p->choice,p->weight,p->existing,p->viable,p->rank,p->cut_points);
 }
done:
 if(pool)HeapFree(GetProcessHeap(),0,pool);
 if(copy)HeapFree(GetProcessHeap(),0,copy);
 if(copied)HeapFree(GetProcessHeap(),0,copied);
}
