/* Called only at a pre-iterator boundary after role, class, score, budget
   and offer authorization have been revalidated for this exact school. */
static int r2_hs_add(Rec team,Rec recruit,U eval,int score) {
 Rec target=t36_target(team,recruit);
 if(target){release(target);return 1;}
 Rec board=board_array(team);U before=board?array_count(board):51;release(board);
 if(before>50 || score<=0)return 0;
 unsigned tid=scalar(team,383,0x100);Rec old=0;U manager=q(eval+0xc8);
 if(!manager || tid>=138)return 0;
 int ask=0,paid=0,wallet=0;
 if(r2_hs_requested(recruit,team,eval) && (!r2_quote(eval,team,recruit,0,&ask,&paid,&wallet) || ask-paid>wallet))return 0;
 if(before==50){
  Hs28RecoveryTeam *cache=&hs28_recovery_teams[tid];
  if(!r2_hs_requested(recruit,team,eval) || !cache->valid || !r2_hs_spares(team,eval,cache->plan,cache->n,&old))return 0;
  U candidate[6]={(U)old,0,0,0,0,0};
  int rc=((int(*)(U,Rec*,void*))(game_base+0x081a7b10))(manager,&team,candidate);cache_note_removal();
  board=board_array(team);U size=board?array_count(board):51;release(board);
  if(rc || size!=49){if(size!=50)portal_fault("R2_HS_exchange_remove");release(old);return 0;}
 }
 struct {U rec;int32_t values[10];} candidate={(U)recruit,{0}};candidate.values[8]=score;
 int rc=((int(*)(U,Rec*,void*))(game_base+0x081496c0))(manager,&team,&candidate);cache_note_write();
 Rec added=t36_target(team,recruit);
 board=board_array(team);U after=board?array_count(board):51;release(board);
 int ok=!rc && added && after==(old?before:before+1);release(added);
 if(!ok && old && rc && after==49){
  candidate.rec=(U)old;candidate.values[8]=0;
  for(int i=0;i<hs28_recovery_teams[tid].n;i++)if(hs28_identity(&hs28_recovery_teams[tid].plan[i],old))
    candidate.values[8]=hs28_recovery_teams[tid].plan[i].native_score;
  int restored=((int(*)(U,Rec*,void*))(game_base+0x081496c0))(manager,&team,&candidate);cache_note_write();
  Rec back=t36_target(team,old);if(restored || !back)portal_fault("R2_HS_exchange_restore");release(back);
 }else if(!ok && (!rc || after>50))portal_fault("R2_HS_exchange_add");
 log_line("R2_HS_BOARD,%u,%u,%d,%u,%u,%d,%d,%d",tid,row(recruit),old?(int)row(old):-1,(unsigned)before,(unsigned)after,ask,wallet,ok);
 release(old);hs28_recovery_teams[tid].valid=0;test29_class_invalidate(team);return ok;
}
/* R14: the native offseason selector is portal-only. At its pre-iterator
   boundary, materialize verified HS reservations using the SAME native board
   admission routine as the regular HS selector. The following native funding
   pass owns scholarships, productive actions, budgets and earned influence. */
static void hs32_offseason_admit(Rec team,U eval) {
 if(mode!=1 || faulted || !hs28_market_scope(eval,0))return;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 int active=season && hs28_pair_window(eval);
 release(season);release(f);if(!active)return;
 Rec user=reference(team,407);if(user){release(user);return;}
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return;
 int count=hs28_market.recovery_count[tid];if(!count)return;
 Hs28MarketReservation pending[50];memcpy(pending,hs28_market.recovery[tid],(size_t)count*sizeof(*pending));
 f=context_ref(eval,6);Rec list=f?array_field(f,39):0;release(f);
 if(!list || array_count(list)>HS28_MARKET_LIMIT){release(list);return;}
 for(int k=0;k<count && !faulted;k++) {
  Rec recruit=0;
  for(U j=0;j<array_count(list);j++) {
   Rec r=array_at(list,j);
   if(r && q((U)r+0x10)==pending[k].table && row(r)==pending[k].rid){recruit=r;break;}release(r);
  }
  if(!recruit)continue;
  if(!hs15_is_recruit(recruit) || scalar(recruit,9,0x108)>=5 || hs32_admission_block(recruit,team,eval)){release(recruit);continue;}
  int owner=-1,published=portal_published_owner(recruit,eval,&owner);
  if(published){release(recruit);if(published<0)portal_fault("HS32_recovery_owner");continue;}
  Rec board=board_array(team);int exists=0;U before=board?array_count(board):MAX_BOARD+1;
  if(board && before<=50)for(U j=0;j<before;j++) {
   Rec target=array_at(board,j),r=target?reference(target,10):0;
   exists|=r && q((U)r+0x10)==pending[k].table && row(r)==pending[k].rid;release(r);release(target);
  }
  release(board);
  if(exists || before>50){release(recruit);continue;}
  Hs15Snapshot shot;int score=0,slot=0,role=0;
  hs28_recovery_teams[tid].valid=0;
  int valid=hs15_collect(&shot,team,eval)?hs28_recovery_destination(recruit,team,eval,&shot,&score,&slot,&role):-1;
  if(valid<0){release(recruit);portal_fault("HS32_recovery_revalidate");break;}
  if(!valid || slot!=pending[k].slot || role!=pending[k].role || locked_out(recruit,team,eval)) {
   log_line("HS32_OFFSEASON_ADMIT,%u,%u,REVALIDATION_DECLINED",tid,row(recruit));release(recruit);continue;
  }
  r2_hs_add(team,recruit,eval,score);
  hs28_recovery_teams[tid].valid=0;release(recruit);
 }
 release(list);
}
/* R15: native portal retention otherwise puts a newly admitted HS target
   outside its portal shortlist into the replacement vector before funding.
   Carry only a still-valid, selected recovery reservation through that pass.
   This returns permission to retain; it never writes a target, offer or hours. */
static int hs33_offseason_retain(Rec recruit,Rec target,Rec team,U eval) {
 if(mode!=1 || faulted || !recruit || !target || !hs15_is_recruit(recruit) ||
    scalar(recruit,9,0x108)>=5 || !hs28_market_scope(eval,0))return 0;
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 int active=season && scalar(season,5,0x108)==6 && scalar(season,4,0x100)>=2 && scalar(season,4,0x100)<=6;
 release(season);release(f);if(!active)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 unsigned tid=scalar(team,383,0x100);if(tid>=138)return 0;
 Rec linked=reference(target,10);int match=linked && portal_same_record(linked,recruit);release(linked);
 if(!match)return 0;
 Hs28MarketReservation *reservation=0;
 for(int i=0;i<hs28_market.recovery_count[tid];i++) {
  Hs28MarketReservation *pending=&hs28_market.recovery[tid][i];
  if(pending->table==q((U)recruit+0x10) && pending->rid==row(recruit)){reservation=pending;break;}
 }
 if(!reservation)return 0;
 int owner=-1,published=portal_published_owner(recruit,eval,&owner);
 if(published){if(published<0)portal_fault("HS33_retention_owner");return 0;}
 Hs15Snapshot shot;int score=0,slot=0,role=0;
 hs28_recovery_teams[tid].valid=0;
 int valid=hs15_collect(&shot,team,eval)?hs28_recovery_destination(recruit,team,eval,&shot,&score,&slot,&role):-1;
 if(valid<0){portal_fault("HS33_retention_revalidate");return 0;}
 if(!valid || slot!=reservation->slot || role!=reservation->role || locked_out(recruit,team,eval))return 0;
 Hs28Choice pool[MAX_BOARD];int n=0;
 if(!hs28_make_board_plan(team,eval,&shot,pool,&n,0)){portal_fault("HS33_retention_plan");return 0;}
 for(int i=0;i<n;i++)if(hs28_identity(&pool[i],recruit)) {
  if(!pool[i].selected || !pool[i].legal || !pool[i].viable || pool[i].slot!=slot || pool[i].choice!=role)return 0;
  log_line("HS33_RECOVERY_RETAIN,%u,%u,%d,%d,NATIVE_FUNDING_PENDING",tid,row(recruit),slot,role);
  return 1;
 }
 return 0;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestHs32Offseason(Rec t,U eval){hs32_offseason_admit(t,eval);}
#endif
