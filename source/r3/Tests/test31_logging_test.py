"""Exact-state diagnostic suppression, with optional archived Test30 replay.

This compiles the production header directly. It does not measure live game or
Windows disk time. Funding, ownership, execution and fault rows remain complete.
"""
from pathlib import Path
import hashlib, json, os, subprocess

S = Path(__file__).resolve().parents[1]
O = Path(os.environ.get('DRI_TEST_OUTPUT_DIR', S/'build/test31-logging'))
O.mkdir(parents=True, exist_ok=True)
code = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "log30_policy.h"
static Log30State state;
static int keep(const char *s, int deep) {return log30_keep(&state,s,strlen(s),deep);}
static void unit(void) {
 const char *role="PORTAL_ROLE,score,7,42,9,2,3,80,1,2";
 const char *changed="PORTAL_ROLE,score,7,42,9,2,3,81,1,2";
 assert(keep(role,0)); assert(!keep(role,0)); assert(keep(changed,0)); assert(keep(role,0));
 assert(keep("PORTAL_ROLE,accept,7,42,9,2,3,80,1,2",0));
 assert(keep("PORTAL_ROLE,score,8,42,9,2,3,80,1,2",0));
 assert(keep("PORTAL_ROLE,score,7,43,9,2,3,80,1,2",0));
 const char *q="HS29_QUALITY_PLAN,7,42,80,2,3,4,5";
 const char *qchanged="HS29_QUALITY_PLAN,7,42,81,2,3,4,5";
 assert(keep(q,0)); assert(!keep(q,0)); assert(keep(qchanged,0)); assert(keep(q,0));
 /* OVR is payload, never part of quality-plan identity: A-B-A is observable. */
 assert(keep("HS29_QUALITY_PLAN,8,42,80,2,3,4,5",0));
 assert(keep("HS29_QUALITY_PLAN,7,43,80,2,3,4,5",0));
 assert(keep(role,1)); assert(keep(role,1)); assert(keep(q,1)); assert(keep(q,1));
 const char *critical[]={"PORTAL29_FUNDING_WITNESS,x,7,42,1", "PORTAL29_FUNDING_SUMMARY,x,7,2",
  "PORTAL29_EXECUTION_SKIP,x,7,BOARD_BOUNDS", "PORTAL_SIGN_PENDING,x,7,42",
  "HS28_FUNDING,x,7,42,40", "HS29_FIRM,7,42,0", "HS30_BOARD_CLEANUP,7,42,1",
  "TEST30_CAP,7,42,16", "FAULT,x,disabled", "DISABLED,x", "MODE,x",
  "PORTAL30_CLEANUP,x,7,42", "HS30_OWNERSHIP,x,7,42", "TEST31_FUNDING_PROVENANCE,x,2026,6,3,7,80,40,1",
  "TEST31_INITIAL_COMMIT,42,3,4,7,0,HOLD_NO_CURRENT_FIRM_OFFER_INITIAL_POINTS_PRESERVED"};
 for(unsigned i=0;i<sizeof(critical)/sizeof(*critical);i++) {
  assert(log30_kind(critical[i])<0); assert(keep(critical[i],0)); assert(keep(critical[i],0));
 }
 assert(log30_boundary("EVAL,x")); assert(log30_boundary("TEST29_PHASE_MS,x"));
 assert(log30_boundary("FAULT,x")); log30_reset(&state); assert(keep(role,0)); assert(!keep(role,0));
 state.epoch=0xffffffffu;log30_reset(&state);assert(state.epoch==1 && keep(role,0));
 assert(keep("PORTAL_ROLE,malformed",0)); assert(keep("PORTAL_ROLE,malformed",0));
 char longrow[400];memset(longrow,'x',sizeof(longrow));memcpy(longrow,"PORTAL_ROLE,",12);longrow[399]=0;
 assert(keep(longrow,0)); assert(keep(longrow,0));
 /* Distinct colliding identities must only cause extra output, never lost changes. */
 for(int i=0;i<LOG30_SLOTS*3;i++){char row[96];snprintf(row,sizeof(row),"HS29_QUALITY_PLAN,7,%d,80,2,3,4",i);assert(keep(row,0));}
 for(int i=0;i<LOG30_SLOTS*3;i++){char row[96];snprintf(row,sizeof(row),"HS29_QUALITY_PLAN,7,%d,81,2,3,4",i);assert(keep(row,0));}
 puts("PASS first/changed state, A-B-A, bounded identity, critical evidence, scopes and deep logging");
}
static void replay(const char *path) {
 FILE *in=fopen(path,"rb");assert(in);char *line=0;size_t capacity=0;ssize_t length;
 unsigned long long rows=0,kept=0,skipped=0,input_bytes=0,output_bytes=0,summaries=0;
 unsigned long long skipped_group[3]={0};memset(&state,0,sizeof(state));
 while((length=getline(&line,&capacity,in))>=0){
  rows++;input_bytes+=(size_t)length;
  if(log30_boundary(line)) {
   if(state.suppressed[0] || state.suppressed[1] || state.suppressed[2]){
    char summary[180];int m=snprintf(summary,sizeof(summary),"TEST31_LOG_REPEAT_SUMMARY,%llu,%llu,%llu,unchanged_decision_plan_defer\r\n",
     (unsigned long long)state.suppressed[0],(unsigned long long)state.suppressed[1],(unsigned long long)state.suppressed[2]);
    assert(m>0 && m<(int)sizeof(summary));output_bytes+=m;summaries++;
   }
   log30_reset(&state);
  }
  if(log30_keep(&state,line,(size_t)length,0)){kept++;output_bytes+=(size_t)length;}
  else {skipped++;skipped_group[log30_kind(line)]++;}
 }
 assert(!ferror(in));free(line);fclose(in);
 printf("{\"input_rows\":%llu,\"kept_rows\":%llu,\"suppressed_rows\":%llu,\"summary_rows\":%llu,\"input_bytes\":%llu,\"output_bytes\":%llu,\"suppressed_by_group\":[%llu,%llu,%llu]}\n",
 rows,kept,skipped,summaries,input_bytes,output_bytes,skipped_group[0],skipped_group[1],skipped_group[2]);
}
int main(int argc,char **argv){if(argc<2)unit();else replay(argv[1]);return 0;}
'''
(O/'logging.c').write_text(code)

def compile_at(include, target):
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(include),
                    str(O/'logging.c'),'-o',str(target)],check=True)

compile_at(S,O/'logging')
subprocess.run([str(O/'logging')],check=True)
result={'status':'PASS','production_policy_sha256':hashlib.sha256((S/'log30_policy.h').read_bytes()).hexdigest(),
        'eligible_families':['HS15_DECISION','PORTAL_ROLE','HS28_PLAN','HS29_QUALITY_PLAN','HS28_PAIR_DEFER'],
        'checks':['new diagnostic families suppress exact repeats only','OVR changes including A-B-A retained',
                  'site/team/recruit isolation','all funding/signing/ownership/fault records passed through',
                  'scope resets, epoch wrap, collision safety, deep override and long/malformed passthrough'],
        'live_game_verified':False,'timing_improvement_measured':False}
fixture=os.environ.get('DRI_TEST_LOG_CSV')
baseline=os.environ.get('DRI_TEST30_BASELINE_DIR')
if fixture:
    fp=Path(fixture)
    result['input']={'name':fp.name,'sha256':hashlib.sha256(fp.read_bytes()).hexdigest()}
    result['test31_replay']=json.loads(subprocess.check_output([str(O/'logging'),str(fp)],text=True))
    if baseline:
        old=Path(baseline)/'Runtime/Portal/Source'
        compile_at(old,O/'logging_test30')
        result['test30_replay']=json.loads(subprocess.check_output([str(O/'logging_test30'),str(fp)],text=True))
        # Baseline is an explicit negative control for new-family deduplication.
        old_unit=subprocess.run([str(O/'logging_test30')],capture_output=True)
        assert old_unit.returncode!=0 and b'!keep(role,0)' in old_unit.stderr, 'Test30 negative control did not fail at the expected added-family assertion'
        result['negative_control']='Test30 fails PORTAL_ROLE exact-repeat assertion as expected'
        result['additional_rows_suppressed']=result['test31_replay']['suppressed_rows']-result['test30_replay']['suppressed_rows']
        assert result['additional_rows_suppressed']>=0
(O/'test31_logging_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
