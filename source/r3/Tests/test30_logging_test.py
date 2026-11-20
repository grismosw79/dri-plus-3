"""Compile actual logging policy; optionally replay the archived R5 CSV.
Timing is local Linux per-row write workload time, never a Windows/live-sim claim.
"""
from pathlib import Path
import os, subprocess, json, hashlib
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR', S/'build/test30-logging'))
O.mkdir(parents=True,exist_ok=True)
code=r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "log30_policy.h"
static Log30State state;
static int keep(const char *s,int deep){return log30_keep(&state,s,strlen(s),deep);}
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static void replay(const char *path,int deep){
 FILE *in=fopen(path,"rb"),*out=tmpfile();assert(in && out);
 char *line=0;size_t capacity=0;ssize_t length;unsigned long long total=0,written=0,skipped=0,bytes=0,summaries=0;
 memset(&state,0,sizeof(state));double start=now();
 while((length=getline(&line,&capacity,in))>=0){
  size_t n=(size_t)length;total++;
  if(log30_boundary(line)){
   if(state.suppressed[0] || state.suppressed[1] || state.suppressed[2]){
    char summary[180];int m=snprintf(summary,sizeof(summary),"TEST30_LOG_REPEAT_SUMMARY,%llu,%llu,%llu,unchanged_decision_plan_defer\r\n",
       (unsigned long long)state.suppressed[0],(unsigned long long)state.suppressed[1],(unsigned long long)state.suppressed[2]);
    assert(m>0 && m<(int)sizeof(summary));assert(write(fileno(out),summary,(size_t)m)==m);bytes+=m;summaries++;
   }
   log30_reset(&state);
  }
  if(log30_keep(&state,line,n,deep)){assert(write(fileno(out),line,n)==(ssize_t)n);written++;bytes+=n;}else skipped++;
 }
 assert(!ferror(in));fflush(out);
 printf("{\"mode\":\"%s\",\"input_rows\":%llu,\"kept_original_rows\":%llu,\"suppressed_rows\":%llu,\"summary_rows\":%llu,\"output_bytes\":%llu,\"local_per_row_write_seconds\":%.6f}\n",deep?"deep":"summary",total,written,skipped,summaries,bytes,now()-start);
 free(line);fclose(in);fclose(out);
}
int main(int argc,char **argv){
 const char *a="HS15_DECISION,accept,7,42,1,3,4,0,0,0,17,ACTIVE";
 const char *b="HS15_DECISION,accept,7,42,1,3,4,0,0,0,0,ACTIVE";
 assert(keep(a,0));assert(!keep(a,0));assert(keep(b,0));assert(keep(a,0));
 assert(keep(a,1));assert(keep(a,1));
 const char *p="HS28_PLAN,7,1,42,1,1,40,100";
 assert(keep(p,0));assert(!keep(p,0));
 assert(keep("HS28_PLAN,7,2,42,1,1,40,100",0));
 const char *d="HS28_PAIR_DEFER,accept,7,42,50,1,0";
 assert(keep(d,0));assert(!keep(d,0));
 assert(keep("HS28_PAIR_DEFER,accept,7,42,51,1,0",0));
 assert(keep("HS28_PAIR_DEFER,accept,8,42,51,1,0",0));
 const char *critical[]={"FAULT,test,disabled","HS28_FUNDING,7,42,40,40,1","PORTAL_SIGN_PENDING,x,7,42", "HS29_FIRM,7,42,0", "TEST30_CAP,7,42,16"};
 for(unsigned i=0;i<sizeof(critical)/sizeof(*critical);i++){assert(keep(critical[i],0));assert(keep(critical[i],0));}
 assert(log30_boundary("EVAL,123,7,2026,0,1"));
 assert(log30_boundary("TEST29_PHASE_MS,HS_PREPARE,7,2026,0,1,50,MEASURED"));
 assert(log30_boundary("FAULT,test"));
 log30_reset(&state);assert(keep(a,0));assert(!keep(a,0));
 state.epoch=0xffffffffu;log30_reset(&state);assert(state.epoch==1 && keep(a,0));
 char longrow[400];memset(longrow,'x',sizeof(longrow));memcpy(longrow,"HS15_DECISION,",14);longrow[399]=0;
 assert(keep(longrow,0));assert(keep(longrow,0));
 assert(keep("HS15_DECISION,malformed",0));assert(keep("HS15_DECISION,malformed",0));
 /* Force collisions by emitting > slot count distinct keys; first occurrence
    of every key must survive, including every changed payload afterwards. */
 for(int i=0;i<LOG30_SLOTS*3;i++){char row[96];snprintf(row,sizeof(row),"HS28_PLAN,7,1,%d,1,1,40,100",i);assert(keep(row,0));}
 for(int i=0;i<LOG30_SLOTS*3;i++){char row[96];snprintf(row,sizeof(row),"HS28_PLAN,7,1,%d,1,1,40,101",i);assert(keep(row,0));}
 puts("PASS: repeat-only suppression, changed states, week/team keys, resets, collisions, critical evidence and deep override");
 if(argc>1){replay(argv[1],1);replay(argv[1],0);}
}
'''
(O/'logging.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(S),str(O/'logging.c'),'-o',str(O/'logging')],check=True)
fixture=os.environ.get('DRI_TEST_LOG_CSV')
command=[str(O/'logging')]+([fixture] if fixture else [])
r=subprocess.run(command,check=True,text=True,capture_output=True);print(r.stdout)
result={'status':'PASS','method':'compiled production logging policy','checks':['only three diagnostic families eligible','first and changed states retained including A-B-A','all funding/signing/fault/cap records retained','team/week isolation','boundary resets and epoch wrap','malformed/long rows passed through','collisions emit safely','deep override retains repeats'],'replay':[json.loads(x) for x in r.stdout.splitlines() if x.startswith('{')],'live_game_verified':False,'windows_logging_time_measured':False}
if fixture:result['input']={'name':Path(fixture).name,'sha256':hashlib.sha256(Path(fixture).read_bytes()).hexdigest()}
(O/'test30_logging_results.json').write_text(json.dumps(result,indent=2)+'\n')
