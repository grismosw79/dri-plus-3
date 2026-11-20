/* Test29R1 acceptance continuity across sources and positions. A live,
   competitive, paid material portal upgrade may retain the capacity it needs
   against an inferior optional signing. Simulate the actual addition and
   reconstruct 2+2+1 before deciding; a forecast never reserves an allowance.
   Strict quality ordering prevents reciprocal deferrals. All records are read
   only; native callbacks still choose, pay for and publish each signing. */
static int test29r1_capacity_block_checked(Rec recruit,Rec team,U eval,const char *site,PortalRoomIssue *optional) {
 if(mode!=1 || faulted || !recruit || scalar(recruit,9,0x108)>=5)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 int hs=hs15_is_recruit(recruit),portal=transfer_recruit(recruit);
 if(!hs && !portal)return 0;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 int active=(season && scalar(season,5,0x108)==6 && scalar(season,4,0x100)>=2 && scalar(season,4,0x100)<=6) ||
  (hs && !hs28_pair_window(eval));
 release(season);release(f);if(!active)return 0;
 Rec player=reference(recruit,5);if(!player)return 0;
 int pos=(int)scalar(player,3,0x108),raw=(int)scalar(player,191,0x100);release(player);
 if(pos<0 || pos>=22 || raw<0 || raw>99)return 0;
 Hs15Snapshot shot;PortalQuality before[22],after[22];
 if(!hs15_collect(&shot,team,eval) || !hs15_portal_rooms(&shot,before,shot.prestige,shot.projection_step)) {
  if(optional){optional->checked_rid=row(recruit);return portal_room_issue(optional,"R1_CAPACITY_SNAPSHOT",-1);}
  portal_fault("R1_capacity_snapshot");return 0;
 }
 int reserved[22]={0};
 if(!test29_hs_reserved(team,eval,reserved) || faulted)return 0;
 dri_portal_set_hs_reservations(before,reserved,shot.prestige);
 if(before[pos].count>=192)return 0;
 int projected=portal_projected_ovr(raw,portal?shot.projection_step:0);
 int own_rank=0,own_value=portal?dri_quality_bonus_candidate(&before[pos],shot.prestige,projected,raw,&own_rank):0;
 memcpy(after,before,sizeof(after));
 dri_portal_project_add_candidate(&after[pos],projected,raw);
 if(hs) {
  /* Same insertion order as project_add: the newly inserted equal OVR goes
     first. Mark only this simulated player as HS and reconstruct the budget. */
  int k=0;while(k<after[pos].count && after[pos].rating[k]>projected)k++;
  after[pos].incoming[k]=0;after[pos].base_count++;after[pos].hs_count++;
  after[pos].incoming_best=before[pos].incoming_best;
  /* The actual HS addition fulfills one reserved source seat, rather than
     counting both that player and its old reservation in the simulation. */
  if(after[pos].hs_reserved>0)after[pos].hs_reserved--;
 }
 dri_portal_refresh_budget(after,shot.prestige);
 int deadline=strncmp(site,"SIGN_",5)==0 || strncmp(site,"SELECT_",7)==0;
 Rec board=board_array(team);U n=board?array_count(board):MAX_BOARD+1;
 if(!board || n>MAX_BOARD){release(board);portal_fault("R1_capacity_board");return 0;}
 Rec budget=reference(team,224);unsigned assigned=budget?scalar(budget,1,0x100):0,total=budget?scalar(budget,3,0x100):0;release(budget);
 uint64_t productive=0;
 for(U i=0;i<n;i++){Rec target=array_at(board,i);unsigned mask=0;if(target)productive+=hs28_productive_allocated(target,eval,&mask);release(target);}
 if(productive>assigned || assigned>total){release(board);return 0;}
 int blocked=0;unsigned blocker=0;
 for(U i=0;i<n && !faulted;i++) {
  Rec target=array_at(board,i),other=target?reference(target,10):0;
  if(!target || !other){release(other);release(target);portal_fault("R1_capacity_target");break;}
  if(portal_same_record(other,recruit) || !transfer_recruit(other) || scalar(other,9,0x108)>=5) {release(other);release(target);continue;}
  unsigned mask=0,rank=0;
  int viable=hs28_productive_allocated(target,eval,&mask)>0 &&
   portal_commit_viable(other,target,team,eval,deadline,&rank,optional)>0;
  if(faulted || (optional && optional->source)){release(other);release(target);break;}
  if(viable) {
   Rec p=reference(other,5);int op=p?(int)scalar(p,3,0x108):-1,ov=p?(int)scalar(p,191,0x100):-1;release(p);
   if(op>=0 && op<22 && ov>=0 && ov<=99) {
    int estimate=portal_projected_ovr(ov,shot.projection_step),role=0;
    int value=dri_quality_bonus_candidate(&before[op],shot.prestige,estimate,ov,&role);
    /* Same-position superiority is concrete. Across positions, require a
       useful-role upgrade with strictly greater improvement value. */
    int better=op==pos?estimate>=projected+4:role>0 && value>own_value;
    if(better && !dri_quality_block_candidate(&before[op],shot.prestige,estimate,ov) &&
       dri_quality_block_candidate(&after[op],shot.prestige,estimate,ov)) {
     blocked=19;blocker=row(other);
    }
   }
  }
  release(other);release(target);if(blocked)break;
 }
 release(board);
 if(blocked)log_line("TEST29R1_CAPACITY_DEFER,%s,%u,%u,%u,PAID_MATERIAL_UPGRADE",site,scalar(team,383,0x100),row(recruit),blocker);
 return blocked;
}
static int test29r1_capacity_block(Rec recruit,Rec team,U eval,const char *site) {
 return test29r1_capacity_block_checked(recruit,team,eval,site,0);
}
/* R2 acceptance-only predicate. Callers separately prove a displayed offer,
   productive native assignment and (where required) a current primary plan.
   All four acceptance gates are re-read; there is no week-scoped acceptance
   cache that could survive an intervening signing or withdrawn offer. The
   lower-level viability checks deliberately do not call this helper, avoiding
   recursive capacity/priority arbitration. This predicts school acceptance,
   not that native recruit interest/commitment timing will choose the school. */
static int test29r2_portal_priority_checked(Rec recruit,Rec team,U eval,const char *site,PortalRoomIssue *optional) {
 /* Internal continuation only: role and native eligibility were checked in
    this same read-only observation. Avoid rebuilding the same roster twice
    for every funded witness; never retain the result across callbacks. */
 if(faulted || portal_commitment_block_checked(recruit,team,eval,site,optional) || faulted || (optional && optional->source))return 0;
 if(test29r1_capacity_block_checked(recruit,team,eval,site,optional) || faulted || (optional && optional->source))return 0;
 return 1;
}
static int test29r2_portal_priority_clear(Rec recruit,Rec team,U eval,const char *site) {
 return test29r2_portal_priority_checked(recruit,team,eval,site,0);
}
static int test29r2_portal_committable_checked(Rec recruit,Rec team,U eval,const char *site,PortalRoomIssue *optional) {
 if(mode!=1 || faulted || !recruit || !team || !eval ||
    !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 if(!native_recruit_eligible(eval,team,recruit) || faulted)return 0;
 if(portal_decision_checked(recruit,team,eval,site,optional) || faulted || (optional && optional->source))return 0;
 return test29r2_portal_priority_checked(recruit,team,eval,site,optional);
}
static int test29r2_portal_committable(Rec recruit,Rec team,U eval,const char *site) {
 return test29r2_portal_committable_checked(recruit,team,eval,site,0);
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest29R1Capacity(Rec r,Rec t,U e,int deadline){return test29r1_capacity_block(r,t,e,deadline?"SELECT_NSD":"WEEKLY");}
__declspec(dllexport) int DriTest29R2Committable(Rec r,Rec t,U e,int deadline){return test29r2_portal_committable(r,t,e,deadline?"SELECT_NSD":"FIRM_CHECK");}
#endif
