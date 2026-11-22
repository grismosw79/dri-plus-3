/* Test29: materialize a globally selected, affordable recovery pursuit only
   at the coordinator's pre-iterator boundary. The supplied score must have
   native scoring provenance. Native 08145950 owns board creation; later native
   funding owns the offer and every hour. Admission is not a recovery success. */
static int portal29_admit(Rec team,Rec recruit,U eval,int verified_score) {
 if(mode!=1 || faulted || !team || !recruit || verified_score<=0 || verified_score>=1000000)return 0;
 unsigned tid=scalar(team,383,0x100);
 if(tid>=138 || (portal_academy(tid) || !portal_academy_allowed(recruit,tid)) || !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 int active=season && scalar(season,5,0x108)==6 && scalar(season,4,0x100)>=2 && scalar(season,4,0x100)<=6;
 release(season);release(f);if(!active)return 0;
 int owner=-1,published=portal_published_owner(recruit,eval,&owner);
 if(published){if(published<0)portal_fault("portal29_admit_owner");return published<0?-1:0;}
 int blocked=market29_portal_admission_block(recruit,team,eval) || portal_decision(recruit,team,eval,"RECOVERY_ADMIT");
 if(faulted)return -1;
 if(blocked || !native_recruit_eligible(eval,team,recruit) || locked_out(recruit,team,eval)) {
  log_line("PORTAL29_ADMIT,%u,%u,REVALIDATION_DECLINED,%d",tid,row(recruit),blocked);return 0;
 }
 Rec board=board_array(team);U before=board?array_count(board):MAX_BOARD+1;int matches=0;
 if(board && before<=50)for(U j=0;j<before;j++) {
  Rec target=array_at(board,j),linked=target?reference(target,10):0;
  matches+=linked && portal_same_record(linked,recruit);release(linked);release(target);
 }
 release(board);
 if(matches==1)return 1;
 if(matches>1 || before>50){portal_fault("portal29_admit_board");return -1;}
 if(before==50){log_line("PORTAL29_ADMIT,%u,%u,BOARD_FULL",tid,row(recruit));return 0;}
 struct {U rec;int32_t values[10];} candidate={(U)recruit,{0}};
 _Static_assert(sizeof(candidate)==48,"Test29 native board candidate ABI");
 candidate.values[8]=verified_score;
 U manager=q(eval+0xc8);if(!manager){portal_fault("portal29_admit_manager");return -1;}
 int result=((int(*)(U,Rec*,void*))(game_base+0x081496c0))(manager,&team,&candidate);cache_note_write();
 board=board_array(team);U after=board?array_count(board):MAX_BOARD+1;matches=0;
 if(board && after<=50)for(U j=0;j<after;j++) {
  Rec target=array_at(board,j),linked=target?reference(target,10):0;
  matches+=linked && portal_same_record(linked,recruit);release(linked);release(target);
 }
 release(board);
 log_line("PORTAL29_ADMIT,%u,%u,%d,%u,%u,%d,NATIVE_BOARD_ADD",tid,row(recruit),result,(unsigned)before,(unsigned)after,matches);
 if((!result && (matches!=1 || after!=before+1)) || after>50 || matches>1){portal_fault("portal29_admit_result");return -1;}
 if(!result && matches==1)test29_class_invalidate(team);
 return !result && matches==1;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTestPortal29Admit(Rec team,Rec recruit,U eval,int score){return portal29_admit(team,recruit,eval,score);}
#endif
