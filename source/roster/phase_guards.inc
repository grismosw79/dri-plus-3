/* Always-on ENC sequencing; deep trace is optional and not a safety gate. */
static int rm_phase_team(U tp){
 Rec t=0;int ok=0;if(!local_read(tp,&t,8)||!t){rm_abort("PHASE_TEAM_UNREADABLE");return -1;}
 int id=pilot_get(t,383,2);if(id<0||id>=RM_TEAMS||!rm_state||!rm_teams[id]||!pilot_same(t,rm_teams[id])){rm_abort("PHASE_TEAM_RECORD_MISMATCH");return -1;}
 Rec user=reference(t,407,&ok);int isuser=user!=0;release(user);
 if(!ok||isuser!=rm_state->t[id].user){rm_abort("PHASE_USER_BOUNDARY_CHANGED");return -1;}return id;
}
static void rm_phase_enter(unsigned site,U module,U tp){
 if(!enabled||!rm_active)return;rm_failed_route();if(rm_fallback)return;
 if((DWORD)observer_thread!=GetCurrentThreadId()){rm_abort("ENC_CALLBACK_THREAD_CHANGED");rm_failed_route();return;}
 if((site==5||site==6||site==7||site==8||site==9||site==11)&&module!=rm_manager){rm_abort("ENC_CALLBACK_MANAGER_CHANGED");rm_failed_route();return;}
 if(site==5){
  log_line("RM18R22 ENROLLMENT_LIMIT_SUMMARY teams_checked=%u midcycle_repairs=%u target=%d",rm_enrollment_checks,rm_enrollment_repairs,rm_enrollment_target);
  if(rm_end_complete){rm_abort("CYCLE_END_DUPLICATE");rm_failed_route();return;}
  for(int t=0;t<RM_TEAMS;t++)if(rm_team_phase[t]!=5){log_line("RM18R22 CALLBACK_ORDER site=%u team=%d expected=5 actual=%u",site,t,rm_team_phase[t]);rm_abort("CYCLE_END_BEFORE_ALL_TEAMS_PREPARED");break;}
 }else if(site==1){if(!rm_end_complete||rm_encourage_started)rm_abort("ENCOURAGE_ALL_ORDER");else rm_encourage_started=1;}
 else{
  int id=rm_phase_team(tp);if(id<0){rm_failed_route();return;}
  if(site>=6&&site<=10){unsigned expected=site-6;
   if(rm_end_complete||rm_team_phase[id]!=expected){log_line("RM18R22 CALLBACK_ORDER site=%u team=%d expected=%u actual=%u",site,id,expected,rm_team_phase[id]);rm_abort("TEAM_CALLBACK_ORDER_OR_REENTRY");}
   else if(site!=6||rm_check_team_enrollment_limit(id))rm_team_phase[id]=(unsigned char)(0x80|expected);
  }else if(site==11){if(!rm_end_complete||!rm_encourage_complete||rm_validated[id])rm_abort("PRESEASON_VALIDATION_ORDER_OR_DUPLICATE");}
  else if(site==2){if(!rm_encourage_started||rm_encourage_complete||rm_encouraged[id])rm_abort("ENCOURAGE_TEAM_ORDER_OR_DUPLICATE");else rm_encouraged[id]=1;}
 }
 rm_failed_route();
}
static void rm_phase_leave(unsigned site,U tp){
 if(!enabled||!rm_active||rm_fallback)return;
 if(site==5){rm_end_complete=1;rm_program_checkpoint("AFTER_NATIVE_CYCLE_END");}
 else if(site==1){rm_encourage_complete=1;rm_program_checkpoint("AFTER_NATIVE_ENCOURAGE_ALL");}
 else if(site>=6&&site<=10){int id=rm_phase_team(tp);if(id>=0){if(rm_team_phase[id]!=(0x80|site-6))rm_abort("CALLBACK_COMPLETION_STATE_CHANGED");else rm_team_phase[id]=(unsigned char)(site-5);}}
 else if(site==2||site==3||site==11)(void)rm_phase_team(tp);
 rm_failed_route();
}
