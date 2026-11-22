/* Test29R1: make a verified recovery primary affordable to the native action
   allocator by moving score weight within its existing stage bucket. Never
   change native bucket totals, budgets, action costs or per-recruit ceilings.
   Ask the native limit helper itself; if even the available weight cannot
   produce a usable allowance, restore every score and report the failed path. */
static int test29r1_native_limit(U eval,Rec team,PortalFundingCandidate *a,int index) {
 Rec target=(Rec)a[index].target;U begin=(U)a;
 int rc=((int(*)(U,Rec*,Rec*,unsigned,U*))(game_base+0x0817e810))(eval,&team,&target,(unsigned)index,&begin);
 cache_note_write();return rc;
}
static void test29r1_recovery_funding(Rec team,U eval,PortalFundingCandidate *a,int n,U *indices) {
 if(mode!=1 || faulted || n<1 || n>MAX_BOARD || hs28_regular(eval))return;
 int recovery=0;
 for(int i=0;i<n;i++)if(a[i].candidate.rec && hs15_is_recruit((Rec)a[i].candidate.rec) &&
  scalar((Rec)a[i].candidate.rec,9,0x108)<5 && hs28_coverage_recovered((Rec)a[i].candidate.rec,team,eval))recovery++;
 Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int np=0;
 if(!hs15_collect(&shot,team,eval)){portal_fault("R1_recovery_funding_snapshot");return;}
 int secured=0;for(int p=0;p<22;p++)secured+=shot.hs[p];
 int completion=secured<(shot.annual_valid?shot.annual_target:test29_class_floor(scalar(team,383,0x100)));
 int portal_recovery=0;
 for(int i=0;i<n;i++){Rec rec=(Rec)a[i].candidate.rec;if(rec && transfer_recruit(rec)){
  Market29PortalRecruit *m=market29_portal_find(q((U)rec+0x10),row(rec));
  portal_recovery|=m && market29_has(m->recovery_primary,scalar(team,383,0x100));}}
 if(!recovery && !completion && !portal_recovery)return;
 if(!hs28_make_board_plan(team,eval,&shot,pool,&np,0)){portal_fault("R1_recovery_funding_plan");return;}
 int primary[MAX_BOARD]={0},rescue[MAX_BOARD]={0},bucket[MAX_BOARD],original[MAX_BOARD];
 unsigned tid=scalar(team,383,0x100);PortalCommitPlan plan,scope={0};
 AcquireSRWLockExclusive(&commitment_lock);plan=commitment_plans[tid];ReleaseSRWLockExclusive(&commitment_lock);
 int matched=portal_commit_scope(&scope,team,eval) && portal_commit_plan_matches(&plan,&scope);
 for(int i=0;i<n;i++) {
  Rec rec=(Rec)a[i].candidate.rec;original[i]=a[i].candidate.values[8];
  bucket[i]=a[i].extra[2]>=1 && a[i].extra[2]<=5;
  if(!rec || scalar(rec,9,0x108)>=5) {primary[i]=1;continue;}
  for(int k=0;k<np;k++)if(hs28_identity(&pool[k],rec) && pool[k].selected && pool[k].choice==1 && pool[k].legal && pool[k].viable) {
   primary[i]=1;rescue[i]=hs28_coverage_recovered(rec,team,eval) ||
    (completion && pool[k].slot%32!=31 && (shot.annual_valid?shot.hs[pool[k].pos]<shot.annual[pool[k].pos]:shot.count[pool[k].pos]<shot.ideal[pool[k].pos]));break;
  }
  int k=matched?portal_commit_index(&plan,rec):-1;
  if(k>=0 && plan.entries[k].round==1){
   primary[i]=1;
   if(transfer_recruit(rec)){
    Market29PortalRecruit *m=market29_portal_find(q((U)rec+0x10),row(rec));
    rescue[i]|=m && market29_has(m->recovery_primary,tid);
   }
  }
 }
 for(int order=0;order<n && !faulted;order++) {
  int i=(int)indices[order];if(i<0 || i>=n){portal_fault("R1_recovery_indices");return;}
  if(!rescue[i] || a[i].candidate.values[8]<=0)continue;
  Rec rec=(Rec)a[i].candidate.rec;int offered=portal_active_offer(rec,team);
  int cost=offered>=0?hs28_recovery_cost(eval,offered):0;
  if(cost<=0)continue;
  int before=test29r1_native_limit(eval,team,a,i);if(before>=cost)continue;
  int donor[MAX_BOARD],saved[MAX_BOARD],nd=0;int64_t available=0;
  for(int j=0;j<n;j++)if(j!=i && !primary[j] && bucket[j]==bucket[i] && a[j].candidate.values[8]>1) {
   donor[nd]=j;saved[nd++]=a[j].candidate.values[8];available+=a[j].candidate.values[8]-1;
  }
  int own=a[i].candidate.values[8];if(!available || available>0x7fffffff-own)continue;
  /* Bounded search for the least transfer that clears the native cap. */
  int lo=0,hi=(int)available,success=0;
  for(int step=0;step<33;step++) {
   int amount=step==0?hi:lo+(hi-lo)/2,left=amount;
   for(int k=0;k<nd;k++){int take=saved[k]-1;if(take>left)take=left;a[donor[k]].candidate.values[8]=saved[k]-take;left-=take;}
   a[i].candidate.values[8]=own+amount;
   int limit=test29r1_native_limit(eval,team,a,i);
   if(limit>=cost){success=1;hi=amount;}else lo=amount+1;
   if((step==0 && !success) || lo>=hi)break;
  }
  for(int k=0;k<nd;k++)a[donor[k]].candidate.values[8]=saved[k];
  a[i].candidate.values[8]=own;
  if(success) {
   int left=hi;
   for(int k=0;k<nd;k++){int take=saved[k]-1;if(take>left)take=left;a[donor[k]].candidate.values[8]-=take;left-=take;}
   a[i].candidate.values[8]+=hi;
  }
  int after=test29r1_native_limit(eval,team,a,i);
  log_line("TEST29R1_RECOVERY_FUNDING,%u,%u,%d,%d,%d,%d,%s",tid,row(rec),cost,before,after,success?hi:0,
   after>=cost?"NATIVE_ALLOCATION_PENDING":"NATIVE_LIMIT_NO_USABLE_SHARE");
 }
 int64_t sums[2]={0};for(int i=0;i<n;i++)sums[bucket[i]]+=(int64_t)a[i].candidate.values[8]-original[i];
 if(sums[0] || sums[1]){for(int i=0;i<n;i++)a[i].candidate.values[8]=original[i];portal_fault("R1_recovery_conservation");}
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest29R1Funding(Rec team,U eval,PortalFundingCandidate *a,int n,U *indices){test29r1_recovery_funding(team,eval,a,n,indices);}
#endif
