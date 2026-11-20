"""Compile the actual observer body against bounded read-only record services.

Checks callback-local reuse and ownership cleanup, including fault interruption.
Native eligibility/funding semantics remain covered by the portal native suites;
this controlled service test is not a native runtime or live timing measurement.
"""
from pathlib import Path
import hashlib,json,os,subprocess
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test31-observer-batch'))
O.mkdir(parents=True,exist_ok=True)
source=(S/'portal29_observe.inc').read_text()
body=source[source.index('static void portal29_observe(Rec '):source.index('static void portal29_observe_all(')]
prefix=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define DRI_EMULATION_TEST 1
#define MAX_BOARD 128
typedef uintptr_t U;
typedef struct Node Node;
typedef Node *Rec;
struct Node {unsigned id,portal,stage,status,allocated,mask,denied,eligible,firm,refs,excluded;Rec recruit;};
typedef struct {U franchise_table;struct {int slot,round;} entries[MAX_BOARD];} PortalCommitPlan;
static Node team_node,franchise_node,season_node,budget_node,board_node,target[3],recruit[3];
static unsigned board_size=3,assigned=40,total=800,year=2026,week=3,type=6;
static unsigned allocation_reads,array_reads,count_reads,decision_reads,eligibility_reads,firm_reads,stores;
static unsigned fault_on_rid=0,observed_productive,observed_assigned,observed_total;static int stamp_valid;
static unsigned provenance_raw,provenance_current,provenance_excluded;
static int mode=1,faulted=0,commitment_lock;
static PortalCommitPlan commitment_plans[138];
static struct {unsigned rid,flags;} portal29_test_witness[MAX_BOARD];
static int portal29_test_witness_count;
static void AcquireSRWLockExclusive(int *p){(void)p;}
static void ReleaseSRWLockExclusive(int *p){(void)p;}
static Rec retained(Rec r){if(r)r->refs++;return r;}
static void release(Rec r){if(r){assert(r->refs>1);r->refs--;}}
static Rec context_ref(U eval,int field){assert(eval==77 && field==6);return retained(&franchise_node);}
static Rec reference(Rec r,int field){
 if(r==&team_node && field==407)return 0;
 if(r==&team_node && field==224)return retained(&budget_node);
 if(r==&franchise_node && field==44)return retained(&season_node);
 if(field==10)return retained(r->recruit);
 assert(0);return 0;
}
static unsigned scalar(Rec r,int field,int schema){(void)schema;
 if(r==&team_node && field==383)return 7;
 if(r==&season_node){if(field==2)return year;if(field==4)return week;if(field==5)return type;}
 if(r==&budget_node){if(field==1)return assigned;if(field==3)return total;}
 if(field==9)return r->stage;
 if(field==12)return r->status;
 if(field==6)return r->allocated;
 if(field==7)return 0;
 assert(0);return 0;
}
static Rec board_array(Rec team){assert(team==&team_node);return retained(&board_node);}
static U array_count(Rec board){assert(board==&board_node);count_reads++;return board_size;}
static Rec array_at(Rec board,U i){assert(board==&board_node && i<3);array_reads++;return retained(&target[i]);}
static unsigned test31_productive_allocated_detail(Rec t,Rec team,U eval,unsigned *mask,unsigned *raw){
 assert(team==&team_node && eval==77);allocation_reads++;*raw=t->allocated;
 *mask=t->excluded?0:t->mask;return t->excluded?0:t->allocated;
}
static int transfer_recruit(Rec r){return r->portal;}
static unsigned row(Rec r){return r->id;}
static int portal_commit_scope(PortalCommitPlan *p,Rec t,U e){assert(t==&team_node && e==77);p->franchise_table=1;return 1;}
static int portal_commit_plan_matches(const PortalCommitPlan *a,const PortalCommitPlan *b){return a->franchise_table==b->franchise_table;}
static int portal_commit_index(const PortalCommitPlan *p,Rec r){(void)p;return (int)(r->id-100);}
static int portal_decision(Rec r,Rec t,U e,const char *site){(void)t;(void)e;(void)site;decision_reads++;return r->denied;}
static int native_recruit_eligible(U e,Rec t,Rec r){(void)e;(void)t;eligibility_reads++;if(r->id==fault_on_rid)faulted=1;return r->eligible;}
static int test29r2_portal_priority_clear(Rec r,Rec t,U e,const char *site){(void)t;(void)e;(void)site;firm_reads++;return r->firm;}
static void log_line(const char *format,...){
 if(!strncmp(format,"TEST31_FUNDING_PROVENANCE,",25)){
  va_list args;va_start(args,format);(void)va_arg(args,const char *);
  for(int i=0;i<4;i++)(void)va_arg(args,unsigned);
  provenance_raw=va_arg(args,unsigned);provenance_current=va_arg(args,unsigned);provenance_excluded=va_arg(args,unsigned);
  va_end(args);
 }
}
static void portal29_budget_store(unsigned tid,U eval,const PortalCommitPlan *now,unsigned a,unsigned t,unsigned p,int valid){
 assert(tid==7 && eval==77 && now->franchise_table==1);stores++;observed_assigned=a;observed_total=t;observed_productive=p;stamp_valid=valid;
}
'''
suffix=r'''
static void balanced(void){
 assert(team_node.refs==1 && franchise_node.refs==1 && season_node.refs==1 && budget_node.refs==1 && board_node.refs==1);
 for(unsigned i=0;i<3;i++)assert(target[i].refs==1 && recruit[i].refs==1);
}
static void counts_reset(void){allocation_reads=array_reads=count_reads=decision_reads=eligibility_reads=firm_reads=stores=0;}
static unsigned flags(unsigned rid){for(int i=0;i<portal29_test_witness_count;i++)if(portal29_test_witness[i].rid==rid)return portal29_test_witness[i].flags;return 0;}
static void observe(void){portal29_observe(&team_node,77,"POST_FUNDING");balanced();}
int main(void){
 team_node.refs=franchise_node.refs=season_node.refs=budget_node.refs=board_node.refs=1;
 commitment_plans[7].franchise_table=1;
 for(unsigned i=0;i<3;i++){
  recruit[i]=(Node){.id=100+i,.portal=i<2,.eligible=1,.firm=i==0,.refs=1};
  target[i]=(Node){.status=3,.allocated=10,.mask=1,.refs=1,.recruit=&recruit[i]};
  commitment_plans[7].entries[i].slot=1;commitment_plans[7].entries[i].round=i+1;
 }
 counts_reset();observe();
 assert(allocation_reads==3 && array_reads==3 && count_reads==1);
 assert(decision_reads==2 && eligibility_reads==2 && firm_reads==1);
 assert((flags(100)&15)==7 && (flags(101)&15)==11);
 assert(stores==1 && stamp_valid && observed_productive==30 && observed_assigned==40 && observed_total==800);
 assert(provenance_raw==30 && provenance_current==30 && provenance_excluded==0);
 /* Reuse ends with the callback. Changed action allocation is read immediately. */
 counts_reset();target[0].allocated=0;target[0].mask=0;observe();
 assert(allocation_reads==3 && (flags(100)&15)==1 && (flags(101)&15)==11 && observed_productive==20);
 /* Whole-board accounting includes the HS target: omitted HS spend would falsely certify. */
 target[0].allocated=10;target[2].allocated=30;assigned=40;observe();
 assert(!stamp_valid && observed_productive==50 && !(flags(100)&14) && !(flags(101)&14));
 /* Status and eligibility are fresh even when numeric productive allocation is unchanged. */
 target[2].allocated=10;target[0].status=0;observe();assert(!(flags(100)&15));
 target[0].status=3;recruit[0].eligible=0;observe();assert((flags(100)&15)==1);
 recruit[0].eligible=1;recruit[0].firm=0;observe();assert((flags(100)&15)==11);
 /* Current-pass filtering keeps retained flag cost visible without a second lookup. */
 target[2].excluded=1;assigned=20;counts_reset();observe();
 assert(allocation_reads==3 && stamp_valid && observed_productive==20);
 assert(provenance_raw==30 && provenance_current==20 && provenance_excluded==1);
 /* All retained board references are released even when a fault stops the second pass. */
 counts_reset();fault_on_rid=100;observe();assert(faulted && !stamp_valid && allocation_reads==3 && array_reads==3);assert(portal29_test_witness_count==1);
 faulted=0;fault_on_rid=0;board_size=MAX_BOARD+1;counts_reset();observe();assert(!array_reads && !allocation_reads && !stores);
 puts("PASS one allocation read per board target, fresh next callback, unchanged budget checks and balanced fault cleanup");
}
'''
(O/'observer_batch.c').write_text(prefix+body+suffix)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2',str(O/'observer_batch.c'),'-o',str(O/'observer_batch')],check=True)
subprocess.run([str(O/'observer_batch')],check=True)
result={'status':'PASS','method':'actual production observer body compiled against controlled read-only record services',
        'source_sha256':hashlib.sha256((S/'portal29_observe.inc').read_bytes()).hexdigest(),
        'mixed_board':{'board_targets':3,'portal_targets':2,'allocation_lookups_before':5,'allocation_lookups_after':3,
                       'board_array_lookups_before':6,'board_array_lookups_after':3},
        'checks':['one productive allocation lookup per board target','complete-board HS allocation remains in budget sum',
                  'fresh allocation/status/eligibility/firm state on later callbacks','assigned/total/productive certification unchanged',
                  'raw/current/excluded-target provenance summary uses the same single per-target observation',
                  'retained references balanced on normal, fault and board-bounds exits'],
        'native_runtime_verified':False,'live_game_verified':False,'timing_improvement_measured':False}
(O/'test31_observer_batch_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
