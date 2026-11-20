"""Production maintenance boundary: preload must precede every mutating leaf.
Callback leaves and SDF services are controlled; no native game is claimed.
"""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
src=(S/'hs28_maintenance.inc').read_text().split('#ifdef DRI_EMULATION_TEST')[0]
c=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uintptr_t U;typedef uintptr_t Rec;
static int mode=1,faulted,week=11,enabled=1,fail_at,step,preloads,mutations;
static int hs28_pair_window(U e){assert(e==123);return enabled;}
static Rec context_ref(U e,int f){assert(e==123&&f==6);return 1;}
static Rec reference(Rec r,int f){assert(r==1&&f==44);return 2;}
static Rec array_field(Rec r,int f){assert(r==1&&f==45);return 3;}
static U q(U a){return a+100;}
static unsigned row(Rec r){assert(r==1);return 0;}
static unsigned scalar(Rec r,int f,int k){assert(r==2);(void)k;return f==2?2026:f==4?(unsigned)week:1;}
static void release(Rec r){assert(r>=1&&r<=3);}
static void log_line(const char *f,...){(void)f;}
static void hs34r4_preload(U e){assert(e==123&&step==0&&!mutations);step++;preloads++;if(fail_at==1)faulted=1;}
static void hs28_nsd_reset(void){assert(step==1);step++;}
static void hs28_cleanup_rivals(Rec r,U e){assert(!r&&e==123&&step==2&&preloads);step++;mutations++;if(fail_at==2)faulted=1;}
static void hs29_reopen_uncommittable(U e,const char *s){assert(e==123&&!strcmp(s,"PRE_PLAN")&&step==3&&preloads);step++;mutations++;}
static void hs28_coverage_begin(U e){assert(e==123&&step==4);step++;}
static void hs28_recover_market(U e,const char *s){assert(e==123&&!strcmp(s,"PRE_PLAN")&&step==5);step++;}
static void hs32_crowding_begin(U e){assert(e==123&&step==6);step++;}
'''+src+r'''
static void reset(void){memset(&hs28_maintenance_scope,0,sizeof(hs28_maintenance_scope));mode=1;faulted=0;enabled=1;fail_at=step=preloads=mutations=0;}
int main(void){
 reset();hs28_maintenance(123);assert(step==7&&preloads==1&&mutations==2);
 hs28_maintenance(123);assert(step==7&&preloads==1&&mutations==2);
 week++;step=mutations=0;hs28_maintenance(123);assert(step==7&&preloads==2&&mutations==2);
 reset();fail_at=1;hs28_maintenance(123);assert(faulted&&step==1&&!mutations&&!hs28_maintenance_scope.valid);
 reset();fail_at=2;hs28_maintenance(123);assert(faulted&&step==3&&mutations==1&&!hs28_maintenance_scope.valid);
 for(int m=-1;m<=0;m++){reset();mode=m;hs28_maintenance(123);assert(!step&&!preloads&&!mutations);}
 reset();enabled=0;hs28_maintenance(123);assert(!step&&!preloads&&!mutations);
 puts("PASS: preload before cleanup/reopening/recovery/forecast; once per scope; fault/mode/calendar controls");return 0;
}
'''
(O/'preload_order.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S),str(O/'preload_order.c'),'-o',str(O/'preload_order')],check=True)
subprocess.run([str(O/'preload_order')],check=True)
(O/'test34_r4_preload_order_results.json').write_text(json.dumps({'passed':True,'production_boundary_executed':True,'callback_leaves_modeled':True,'checks':['Preload precedes every mutating maintenance callback','Repeated same scope does not reload or repeat maintenance','New week preloads before maintenance','Preload/cleanup faults stop subsequent mutations','Disabled and diagnostic modes do not mutate','Allocation calendar remains unchanged'],'live_game_verified':False},indent=2)+'\n')
