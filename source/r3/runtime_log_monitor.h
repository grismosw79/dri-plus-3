#ifndef DRI_RUNTIME_LOG_MONITOR_H
#define DRI_RUNTIME_LOG_MONITOR_H
#include <stddef.h>
#include <string.h>
/* Streaming text only: no game access and no changes to runtime state. Keep
   partial rows between reads and preserve a bounded prefix of long rows. */
typedef struct {
 char line[512],reason[512];size_t used;int fault,stopped;
} DriLogMonitor;
static void dri_log_monitor_feed(DriLogMonitor *s,const char *bytes,size_t size) {
 for(size_t j=0;j<size;j++) {
  unsigned char c=(unsigned char)bytes[j];
  if(c=='\n') {
   while(s->used && s->line[s->used-1]=='\r')s->used--;
   s->line[s->used]=0;
   if(!s->fault && !strncmp(s->line,"FAULT,",6)) {
    s->fault=1;memcpy(s->reason,s->line,s->used+1);
   }
   if(!strncmp(s->line,"DISABLED,",9) || !strcmp(s->line,"MODE,0") ||
      !strncmp(s->line,"READY,DIAGNOSTIC,",17))s->stopped=1;
   if(!strcmp(s->line,"MODE,1") || !strncmp(s->line,"READY,ACTIVE,",13))s->stopped=0;
   s->used=0;
  } else if(s->used+1<sizeof(s->line))s->line[s->used++]=(char)c;
 }
}
#endif
