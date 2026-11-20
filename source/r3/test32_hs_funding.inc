/* Test32: a positive priority weight is not a usable native allowance. The
   native helper truncates the weighted stage share, then applies remaining
   stage hours and the per-target cap. A selected regular-season primary can
   consequently receive less than the cheapest productive native action.

   Only move score from nonprimary HS targets in the SAME stage bucket, and
   only if the captured native helper proves that the minimum action becomes
   affordable. All primaries and non-HS candidates are protected donors. The
   subsequent native action/eligibility/payment path still decides and spends.
   No hours, action flags, influence, scholarship or budget is written here. */
static void test32_hs_minimum_share(Rec team,U eval,PortalFundingCandidate *a,int n,U *indices,
                                  const Hs28Choice *pool,int np) {
 if(mode!=1 || faulted || !hs28_regular(eval) || n<1 || n>MAX_BOARD || np<0 || np>MAX_BOARD)return;
 int primary[MAX_BOARD]={0},donatable[MAX_BOARD]={0},bucket[MAX_BOARD],original[MAX_BOARD];
 for(int i=0;i<n;i++) {
  Rec rec=(Rec)a[i].candidate.rec;original[i]=a[i].candidate.values[8];
  bucket[i]=a[i].extra[2]>=1 && a[i].extra[2]<=5;
  if(!rec || !hs15_is_recruit(rec) || scalar(rec,9,0x108)>=5 || original[i]<=0)continue;
  for(int k=0;k<np;k++)if(hs28_identity(&pool[k],rec)) {
   if(pool[k].final)break;
   primary[i]=pool[k].selected && pool[k].choice==1 && pool[k].legal && pool[k].viable;
   donatable[i]=!primary[i];break;
  }
 }
 unsigned tid=scalar(team,383,0x100);
 for(int at=0;at<n && !faulted;at++) {
  if(indices[at]>=(U)n){portal_fault("HS32_minimum_indices");return;}
  int i=(int)indices[at];if(!primary[i])continue;
  Rec rec=(Rec)a[i].candidate.rec;int offered=portal_active_offer(rec,team);
  int cost=offered>=0?hs28_recovery_cost(eval,offered):0;if(cost<=0)continue;
  int before=test29r1_native_limit(eval,team,a,i);if(before>=cost)continue;
  int donor[MAX_BOARD],saved[MAX_BOARD],nd=0;int64_t available=0;
  /* Least-priority alternatives supply the repair first; no other firm
     pursuit loses its score, and every retained alternative stays positive. */
  for(int step=n-1;step>=0;step--) {
   int j=(int)indices[step];if(j<0 || j>=n){portal_fault("HS32_minimum_indices");return;}
   if(j!=i && donatable[j] && bucket[j]==bucket[i] && a[j].candidate.values[8]>1) {
    donor[nd]=j;saved[nd++]=a[j].candidate.values[8];available+=a[j].candidate.values[8]-1;
   }
  }
  int own=a[i].candidate.values[8],success=0,amount=0;
  if(available>0 && available<=0x7fffffff-own) {
   int lo=0,hi=(int)available;
   for(int step=0;step<33;step++) {
    int probe=step?lo+(hi-lo)/2:hi,left=probe;
    for(int k=0;k<nd;k++){int take=saved[k]-1;if(take>left)take=left;a[donor[k]].candidate.values[8]=saved[k]-take;left-=take;}
    a[i].candidate.values[8]=own+probe;
    if(test29r1_native_limit(eval,team,a,i)>=cost){success=1;hi=probe;}else lo=probe+1;
    if((step==0 && !success) || lo>=hi)break;
   }
   for(int k=0;k<nd;k++)a[donor[k]].candidate.values[8]=saved[k];
   a[i].candidate.values[8]=own;
   if(success) {
    amount=hi;int left=amount;
    for(int k=0;k<nd;k++){int take=saved[k]-1;if(take>left)take=left;a[donor[k]].candidate.values[8]-=take;left-=take;}
    a[i].candidate.values[8]+=amount;
   }
  }
  int after=test29r1_native_limit(eval,team,a,i);
  log_line("HS32_MINIMUM_SHARE,%u,%u,%d,%d,%d,%d,%d,%s",tid,row(rec),bucket[i],cost,before,after,amount,
   after>=cost?"NATIVE_ACTION_PENDING":!available?"NO_NONPRIMARY_SAME_BUCKET_WEIGHT":"NATIVE_LIMIT_REFUSED");
 }
 int64_t sums[2]={0};
 for(int i=0;i<n;i++)sums[bucket[i]]+=(int64_t)a[i].candidate.values[8]-original[i];
 if(sums[0] || sums[1]) {
  for(int i=0;i<n;i++)a[i].candidate.values[8]=original[i];
  portal_fault("HS32_minimum_conservation");
 }
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest32HsMinimum(Rec team,U eval,PortalFundingCandidate *a,int n,U *indices) {
 Hs15Snapshot shot;Hs28Choice pool[MAX_BOARD];int np=0;
 if(hs15_collect(&shot,team,eval) && hs28_make_board_plan(team,eval,&shot,pool,&np,0))
  test32_hs_minimum_share(team,eval,a,n,indices,pool,np);
}
#endif
