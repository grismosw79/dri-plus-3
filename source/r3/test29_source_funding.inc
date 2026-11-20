/* Included after PortalFundingCandidate's verified72-byte native ABI. */
static void test29_hs_funding_priority(Rec team,U eval,PortalFundingCandidate *a,int n,U *indices) {
 if(mode!=1 || faulted || n<1 || n>MAX_BOARD || !hs28_pair_window(eval))return;
 int has_hs=0;
 for(int j=0;j<n;j++)if(a[j].candidate.rec && hs15_is_recruit((Rec)a[j].candidate.rec) &&
   scalar((Rec)a[j].candidate.rec,9,0x108)<5 && a[j].candidate.values[8]>0){has_hs=1;break;}
 if(!has_hs)return;
 Hs15Snapshot shot;if(!hs15_collect(&shot,team,eval)){portal_fault("class_source_snapshot");return;}
 int secured=0;for(int p=0;p<22;p++)secured+=shot.hs[p];
 Hs28Choice pool[MAX_BOARD];int np=0;
 if(!hs28_make_board_plan(team,eval,&shot,pool,&np,0)){portal_fault("class_source_plan");return;}
 int bucket[MAX_BOARD],priority[MAX_BOARD]={0},primaries=0;
 for(int j=0;j<n;j++) {
  bucket[j]=a[j].extra[2]>=1 && a[j].extra[2]<=5;
  Rec rec=(Rec)a[j].candidate.rec;
  if(!rec || !hs15_is_recruit(rec) || scalar(rec,9,0x108)>=5 || a[j].candidate.values[8]<=0)continue;
  for(int k=0;k<np;k++)if(hs28_identity(&pool[k],rec) && pool[k].selected &&
    pool[k].choice==1 && pool[k].legal && pool[k].viable) {
   Rec target=(Rec)a[j].target,linked=target?reference(target,10):0;
   int match=hs28_identity(&pool[k],linked);release(linked);
   if(!match){portal_fault("class_source_target");return;}
   if(secured<(shot.annual_valid?shot.annual_target:test29_class_floor(scalar(team,383,0x100))) || hs28_coverage_recovered(rec,team,eval)){priority[j]=1;primaries++;}
   break;
  }
 }
 if(!primaries)return;
 int changed=test29_source_prioritize(n,(uint64_t *)indices,bucket,priority);
 if(changed<0){portal_fault("class_source_permutation");return;}
 log_line("TEST29_HS_FUNDING_PRIORITY,%u,%d,%d,%d,ORDER_ONLY_NATIVE_PAYMENT_PENDING",scalar(team,383,0x100),secured,primaries,changed);
 test29_class_invalidate(team);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest29HsFundingPriority(Rec team,U eval,PortalFundingCandidate *a,int n,U *indices) {
 test29_hs_funding_priority(team,eval,a,n,indices);
}
#endif
