#include <assert.h>
#include <stdio.h>
#include "runtime_log_monitor.h"
int main(void) {
 const char *actual="FAULT,portal_HS_projection,behavior_disabled_restart_game\r\n";
 for(size_t cut=0;cut<=strlen(actual);cut++) {
  DriLogMonitor s={0};
  dri_log_monitor_feed(&s,actual,cut);
  dri_log_monitor_feed(&s,actual+cut,strlen(actual)-cut);
  assert(s.fault && !strcmp(s.reason,"FAULT,portal_HS_projection,behavior_disabled_restart_game"));
 }
 DriLogMonitor s={0};
 const char *clean="READY,ACTIVE,game_base_0000000140000000\r\nPORTAL_SCORE,1,2,FAULT,4\r\n";
 dri_log_monitor_feed(&s,clean,strlen(clean));assert(!s.fault && !s.stopped);
 const char *disabled="DISABLED,behavior_passthrough_until_process_exit\r\n";
 dri_log_monitor_feed(&s,disabled,strlen(disabled));assert(s.stopped && !s.fault);
 dri_log_monitor_feed(&s,"MODE,1\r\n",8);assert(!s.stopped);
 dri_log_monitor_feed(&s,"MODE,0\r\n",8);assert(s.stopped);
 dri_log_monitor_feed(&s,"READY,ACTIVE,base\n",18);assert(!s.stopped);
 /* Oversized non-fault metadata cannot conceal the next fault or overflow. */
 memset(&s,0,sizeof(s));char large[10000];memset(large,'X',sizeof(large));
 dri_log_monitor_feed(&s,large,sizeof(large));dri_log_monitor_feed(&s,"\n",1);
 assert(!s.fault);dri_log_monitor_feed(&s,actual,strlen(actual));assert(s.fault);
 char saved[512];memcpy(saved,s.reason,sizeof(saved));
 dri_log_monitor_feed(&s,"MODE,1\nFAULT,second\n",20);assert(!strcmp(saved,s.reason));
 /* Fault itself may be longer than the displayed, bounded reason. */
 memset(&s,0,sizeof(s));dri_log_monitor_feed(&s,"FAULT,",6);
 dri_log_monitor_feed(&s,large,sizeof(large));dri_log_monitor_feed(&s,"\r\n",2);
 assert(s.fault && strlen(s.reason)==511 && !strncmp(s.reason,"FAULT,",6));
 puts("PASS: actual Test15 fault at every chunk boundary, normal rows, false positives, disable/re-enable, long rows, first fault retained");
 return 0;
}
