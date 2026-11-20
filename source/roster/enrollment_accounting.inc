/* ENC has two different balances: funded recruiting-board offers and incoming
   player contracts. Native enrollment refunds each contract from recruiting
   spend (clamped at zero), then charges NIL. Cleanup refunds unused offers.
   This module predicts that lifecycle; it never writes native money or offers. */
static int rm_enrollment_contracts[RM_TEAMS],rm_enrollment_holdback[RM_TEAMS];
static int rm_enrollment_reserve(int t,int *value){
 U finance=0;
 if(!local_read(rm_manager+0x1a0,&finance,8)||!finance)return 0;
 Rec team=rm_teams[t];
 *value=((int(*)(U,Rec*))(game_base+0x82ce2f0))(finance,&team);
 return *value>=0&&*value<=20000&&!capture_fault;
}
static int rm_enrollment_board(int t,int contracts,int hold_pool){
 int ok=0,errors=0,total=0,covered=0,committed_offers=0,refundable=0,held=0;
 Rec board=reference(rm_teams[t],224,&ok),a=0;
 if(ok&&board)a=array_field(board,0,&ok);
 U n=a?array_count(a):0,stores[RM_ROOM]={0};uint32_t rows[RM_ROOM]={0};
 if(!ok||!board||!a||n>RM_ROOM){release(a);release(board);return rm_audit_error(0,t,"RECRUIT_BOARD","readable",1,0);}
 for(U j=0;j<n;j++){
  Rec target=array_at(a,j,&ok),recruit=0,player=0;int offer=0;
  if(!ok||!target||!scalar(target,2,2,&offer)||offer<0||offer>1023){errors+=rm_audit_error(0,t,"RECRUIT_BOARD","valid_offer",1,0);release(target);continue;}
  /* Zero offers cannot change the financial transition. */
  if(!offer){release(target);continue;}
  recruit=reference(target,10,&ok);if(ok&&recruit)player=reference(recruit,5,&ok);
  if(!ok||!recruit||!player||!identity(player,&stores[j],&rows[j])){errors+=rm_audit_error(0,t,"RECRUIT_BOARD","funded_player_readable",1,0);goto next;}
  for(U k=0;k<j;k++)if(stores[k]&&stores[k]==stores[j]&&rows[k]==rows[j])errors+=rm_audit_error(0,t,"RECRUIT_BOARD","unique_funded_player",1,0);
  int id=pilot_get(player,228,2),i=rm_lookup(id),stage=pilot_get(recruit,9,3),first=-1;
  if(stage<0||stage>6){errors+=rm_audit_error(id,t,"RECRUIT_BOARD","valid_stage",1,0);goto next;}
  if(stage==6){
   Rec schools=array_field(recruit,13,&ok),school=0;U size=schools?array_count(schools):0;
   if(ok&&schools&&size&&size<=138)school=array_at(schools,0,&ok);
   if(!ok||!school||!scalar(school,0,2,&first)||first<0||first>=RM_TEAMS)errors+=rm_audit_error(id,t,"RECRUIT_BOARD","signed_leader_readable",1,0);
   release(school);release(schools);
  }
  int own=i>=0&&rm_live_identity(player,i)&&rm_state->p[i].arrival&&rm_state->p[i].original==t;
  total+=offer;
  if(own){
   covered+=rm_expected_nil[i];committed_offers+=offer;
  }
  /* This is the exact native cleanup predicate (862d910 / 862cf00).
     Interest-list rank is used ONLY to predict that native refund. Published
     roster/commit ownership above remains the source of player ownership. */
  if(stage!=6||first!=t){
   refundable+=offer;
   /* When cleanup must release cash, do not consume a funded unsigned record
      before its recruiting obligation clears. This is detached plan state. */
   if(hold_pool&&rm_expected_money[t].value[2]>contracts&&i>=0&&rm_live_identity(player,i)&&rm_state->p[i].pool&&!rm_state->p[i].held){rm_state->p[i].held=1;held++;rm_event("FUNDED_UNSIGNED_WAITING_FOR_NATIVE_CLEANUP",id,t,-1,offer);}
  }
 next:release(player);release(recruit);release(target);
 }
 release(a);release(board);
 if(total!=rm_expected_money[t].value[2])errors+=rm_audit_error(0,t,"RECRUIT_BOARD","funded_offers_equal_recruit_spend",rm_expected_money[t].value[2],total);
 if(covered>contracts)errors+=rm_audit_error(0,t,"RECRUIT_BOARD","covered_contracts_within_incoming_total",contracts,covered);
 if(refundable<rm_max(0,total-contracts))errors+=rm_audit_error(0,t,"RECRUIT_BOARD","native_cleanup_covers_remaining_recruit_spend",rm_max(0,total-contracts),refundable);
 char b[800];snprintf(b,sizeof(b),"\"event\":\"RM18R22_ENROLLMENT_BUDGET\",\"phase\":\"%s\",\"team\":%d,\"incoming_contracts\":%d,\"recruit_spend\":%d,\"covered_contracts\":%d,\"contracts_outside_board\":%d,\"funded_committed_offers\":%d,\"unused_funded_offers\":%d,\"native_cleanup_refundable\":%d,\"additional_cash_required\":%d,\"expected_cleanup_refund\":%d,\"temporary_pool_holds\":%d,\"errors\":%d",rm_audit_phase,t,contracts,total,covered,contracts-covered,committed_offers,total-committed_offers,refundable,rm_max(0,contracts-total),rm_max(0,total-contracts),held,errors);emit(b);
 return errors;
}
static int rm_enrollment_prepare(int t,int contracts){
 int reserve=-1,errors=0;
 /* Nonzero native recruiting reserves need a different transition model.
    Refuse before mutations instead of guessing or accepting lost money. */
 if(!rm_enrollment_reserve(t,&reserve)||reserve)errors+=rm_audit_error(0,t,"ENROLLMENT_RESERVE","native_reserve_at_enc",0,reserve);
 rm_enrollment_contracts[t]=contracts;
 int hold=rm_max(0,contracts-rm_expected_money[t].value[2]);rm_enrollment_holdback[t]=hold;
 errors+=rm_enrollment_board(t,contracts,1);
 if(rm_state->t[t].budget<hold)errors+=rm_audit_error(0,t,"ENROLLMENT_FUNDING","cash_for_existing_commits",hold,rm_state->t[t].budget);
 else rm_state->t[t].budget-=hold;
 return errors;
}
static int rm_enrollment_seal(void){
 int errors=0;
 for(int t=0;t<RM_TEAMS;t++){
  int reserve=-1,contracts=rm_enrollment_contracts[t],held=rm_enrollment_holdback[t];
  if(!rm_enrollment_reserve(t,&reserve)||reserve)errors+=rm_audit_error(0,t,"ENROLLMENT_RESERVE","native_reserve_at_enc",0,reserve);
  errors+=rm_enrollment_board(t,contracts,0);
  int required=held+rm_max(0,rm_opening_variance[t]);
  if(rm_expected_money[t].value[3]<required)errors+=rm_audit_error(0,t,"ENROLLMENT_FUNDING","cash_after_custom_transactions",required,rm_expected_money[t].value[3]);
 }
 if(errors)return 0;
 for(int t=0;t<RM_TEAMS;t++){
  PilotMoney *m=&rm_expected_money[t];int amount=rm_enrollment_contracts[t];
  m->value[0]+=amount;m->value[3]+=m->value[2]-amount;m->value[2]=0;
 }
 return !capture_fault;
}
