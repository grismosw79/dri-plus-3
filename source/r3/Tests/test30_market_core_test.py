"""Compile production Test30 authorization; no live spending claim."""
from pathlib import Path
import subprocess, os, json
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR', S/'build/test30-market-core'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <assert.h>
#include <stdio.h>
#include "market29_core.h"
static int count(const uint64_t *p){int n=0;for(unsigned t=0;t<138;t++)n+=market29_has(p,t);return n;}
static void init(Market29Pressure *p,int reverse,int portal){
 memset(p,0,sizeof(*p));
 for(unsigned k=0;k<40;k++){unsigned t=reverse?39-k:k;
  market29_bit(p->planned,t,1);market29_bit(p->primary,t,t%2==0);
  market29_bit(p->exception,t,1);p->fit[t]=500+(int)t;p->interest[t]=100+(int)t;
 }
 assert(market30_hard_reconcile(p,portal,123,2026)==40-market30_limit(portal));
}
int main(void){
 for(int portal=0;portal<2;portal++){
  int limit=market30_limit(portal);Market29Pressure a,b;
  init(&a,0,portal);init(&b,1,portal);
  assert(!memcmp(a.hard_admitted,b.hard_admitted,24));
  assert(count(a.hard_admitted)==limit && count(a.hard_rejected)==40-limit);
  market30_hard_freeze(&a,portal,123,2026);
  assert(count(a.planned)==limit && count(a.hard_authorized)==limit);
  for(unsigned t=0;t<40;t++)assert(!market29_has(a.hard_rejected,t)||!market29_has(a.hard_authorized,t));
  /* Role relabeling cannot acquire an additional school slot. */
  memcpy(a.primary,a.planned,24);market30_hard_freeze(&a,portal,123,2026);
  assert(count(a.planned)==limit && count(a.hard_authorized)==limit);
  /* Existing active/conditional offered pursuits consume the same ceiling. */
  memset(&a,0,sizeof(a));for(unsigned t=0;t<40;t++){
   a.fit[t]=200;market29_bit(a.planned,t,1);market29_bit(a.primary,t,t>=4);
   if(t<4){market29_bit(a.protected_offer,t,1);market29_bit(a.active,t,1);}
  }
  market30_hard_freeze(&a,portal,77,2026);
  assert(count(a.hard_authorized)==limit);for(unsigned t=0;t<4;t++)assert(market29_has(a.hard_authorized,t));
  /* Grandfathered excess: no destruction, but no additional new school. */
  memset(&a,0,sizeof(a));for(unsigned t=0;t<40;t++){
   a.fit[t]=200;market29_bit(a.planned,t,1);market29_bit(a.primary,t,1);
   if(t<(unsigned)(limit+3))market29_bit(a.protected_offer,t,1);
  }
  market30_hard_freeze(&a,portal,77,2026);
  assert(count(a.hard_authorized)==limit+3 && count(a.planned)==limit+3);
  for(unsigned t=limit+3;t<40;t++)assert(!market29_has(a.hard_authorized,t));
  /* Frozen standby slots: late recovery cannot depend on school call order. */
  memset(&a,0,sizeof(a));for(unsigned t=0;t<40;t++){a.fit[t]=200;a.interest[t]=t;if(t<2)market29_bit(a.planned,t,1);}
  b=a;market30_hard_freeze(&a,portal,77,2026);market30_hard_freeze(&b,portal,77,2026);
  assert(count(a.planned)==2 && count(a.hard_authorized)==limit);
  assert(!memcmp(a.hard_authorized,b.hard_authorized,24));
  /* A formerly denied school can recover after winners lose eligibility. */
  init(&a,0,portal);unsigned former=0;
  while(!market29_has(a.hard_rejected,former))former++;
  memset(a.planned,0,24);memset(a.primary,0,24);memset(a.fit,0,sizeof(a.fit));a.fit[former]=500;
  market30_hard_freeze(&a,portal,123,2026);
  assert(count(a.hard_authorized)==1 && market29_has(a.hard_authorized,former));
  /* Absent/non-positive native fit cannot receive spare recovery permission. */
  memset(&a,0,sizeof(a));a.fit[32]=200;market30_hard_freeze(&a,portal,1,2026);
  assert(count(a.hard_authorized)==1 && market29_has(a.hard_authorized,32));
 }
 puts("PASS Test30 hard CPU HS16/portal10: all exceptions bounded; conditional+primary union; stable order; grandfather excess; frozen recovery standby");
}
'''
(O/'test30_market_core.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(S),str(O/'test30_market_core.c'),'-o',str(O/'test30_market_core')],check=True)
r=subprocess.run([str(O/'test30_market_core')],check=True,capture_output=True,text=True);print(r.stdout)
(O/'test30_market_core_results.json').write_text(json.dumps({'status':'PASS','scope':'compiled production authorization; not live execution','hard_cpu_limits':{'HS':16,'portal':10},'live_game_verified':False,'checks':['all-exception oversubscription','stable simultaneous allocation','primary and conditional combined','role relabeling cannot enlarge authorization','protected offers consume cap','grandfathered excess blocks all new entry','stable finite recovery standby','absent or nonpositive fit excluded']},indent=2)+'\n')
