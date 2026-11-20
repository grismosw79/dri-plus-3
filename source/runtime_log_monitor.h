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
/* Auxiliary JSONL status rows use fixed native-emitter keys. Match the
   status message prefix, never arbitrary player data or diagnostic text. */
typedef struct {char line[512],reason[512];size_t used;int fault;} DriPrepMonitor;
static void dri_prep_monitor_feed(DriPrepMonitor *s,const char *bytes,size_t size) {
 for(size_t j=0;j<size;j++) {
  unsigned char c=(unsigned char)bytes[j];
  if(c=='\n') {
   while(s->used && s->line[s->used-1]=='\r')s->used--;
   s->line[s->used]=0;
   const char *message=strstr(s->line,"\"message\":\"");
   if(!s->fault && strstr(s->line,"\"event\":\"STATUS\"") && message) {
    message+=11;
    const char *prefixes[]={"CAPTURE_FAILED ","DEV_ANNUAL_DEFER ",
     "RM17R3 FAILED_DISCARD_BRANCH ","RM17R3 PREFLIGHT_DECLINED_NATIVE_FALLTHROUGH;"};
    for(size_t k=0;k<sizeof(prefixes)/sizeof(prefixes[0]);k++)
     if(!strncmp(message,prefixes[k],strlen(prefixes[k]))) {
      s->fault=1;memcpy(s->reason,s->line,s->used+1);break;
     }
   }
   s->used=0;
  } else if(s->used+1<sizeof(s->line))s->line[s->used++]=(char)c;
 }
}
#endif
