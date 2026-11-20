"""Execute the production end-of-week boundary with controlled callback leaves.
Verify HS cleanup is phase-independent while allocations keep their old calendar.
"""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
src=(S/'portal_lifecycle.inc').read_text();body=src[src.index('void DriAfterPortalWeekly('):src.index('void DriAfterPortalInstant(')]
c=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uintptr_t U;typedef uintptr_t Rec;typedef struct{U rbp;}Registers;
#define __try if(1)
#define __except(x) else
static int mode=1,faulted,typ,week,clean,recover,seed,refill,reopen,checkpoint,changed,portal_clean,fail_clean;
static struct{int valid,active;U table;unsigned rid,year;}maintenance_scope;
static void pt28_invalidate(void){}
static U q(U p){(void)p;return 123;}
static Rec portal_team_by_id(U e,unsigned t){assert(e==123);return t+100;}
static void test29_class_invalidate(Rec t){assert(t>=100&&t<238);changed++;}
static void release(Rec r){(void)r;}
static void portal29_observe_all(U e,const char *s){assert(e==123&&!strcmp(s,"POST_WEEK"));}
static void hs28_nsd_reset(void){}
static void hs28_coverage_end(U e){assert(e==123);}
static void hs28_after_week(U e){assert(e==123);}
static void hs28_cleanup_rivals(Rec t,U e){assert(!t&&e==123);clean++;if(fail_clean)faulted=1;}
static int hs28_pair_window(U e){assert(e==123);return typ<=1||(typ==6&&week>=2&&week<=6);}
static void hs29_reopen_uncommittable(U e,const char *s){assert(e==123&&!strcmp(s,"POST_WEEK"));reopen++;}
static void hs28_recover_market(U e,const char *s){assert(e==123&&!strcmp(s,"POST_WEEK"));recover++;}
static void hs28_coverage_seed(U e){assert(e==123);seed++;}
static void test34_class_refill(U e){assert(e==123);refill++;}
static Rec context_ref(U e,int f){assert(e==123&&f==6);return 1;}
static Rec reference(Rec r,int f){assert(r==1&&f==44);return 2;}
static unsigned scalar(Rec r,int f,int kind){assert(r==2);(void)kind;return f==5?(unsigned)typ:f==2?2026:(unsigned)week;}
static unsigned row(Rec r){assert(r==1);return 0;}
static void portal_recover_market(U e,int opening){assert(e==123&&!opening);}
static void portal_cleanup_all(U e){assert(e==123);portal_clean++;}
static void hs34r3_checkpoint(U e){assert(e==123);checkpoint++;}
'''+body+r'''
int main(void){Registers r={0};
 for(typ=0;typ<=6;typ++)for(week=0;week<=21;week++){
  clean=recover=seed=refill=reopen=checkpoint=changed=portal_clean=0;
  DriAfterPortalWeekly(&r);
  assert(!faulted&&changed==138&&clean==1&&checkpoint==1);
  int allocation=typ<=1||(typ==6&&week>=2&&week<=6);
  assert(recover==allocation&&seed==allocation&&refill==allocation&&reopen==allocation);
  assert(portal_clean==(typ==6));
 }
 fail_clean=1;clean=recover=checkpoint=0;DriAfterPortalWeekly(&r);assert(faulted&&clean==1&&!recover&&!checkpoint);
 for(mode=-1;mode<=0;mode++){faulted=0;clean=changed=0;DriAfterPortalWeekly(&r);assert(!clean&&!changed);}
 return 0;
}'''
(O/'boundary.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(O/'boundary.c'),'-o',str(O/'boundary')],check=True)
subprocess.run([str(O/'boundary')],check=True)
r={'passed':True,'production_boundary_executed':True,'callback_leaves_modeled':True,'calendar_cases':154,'checks':['Rival cleanup reaches every native weekly phase','Allocation/reopening/refill calendar unchanged','Checkpoint follows completed maintenance','Fault stops downstream work','Diagnostic and disabled modes do not mutate'],'live_game_verified':False}
(O/'test34_r3_boundary_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
