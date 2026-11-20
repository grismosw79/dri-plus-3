"""Compile the vacancy boundary and shared HS/portal budget reconstruction."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
c=r'''
#include <assert.h>
#include <string.h>
#include "class34_policy.h"
#include "portal_policy.h"
static int extra(int pos,int target,int secured,int total,int ideal,int annual,int hs,int count) {
 return dri_hs_vacancy_extra(1,target,secured,total,pos,ideal,annual,hs,count);
}
int main(void) {
 assert(extra(1,18,14,70,5,1,1,4)); /* Navy: one HB signed, real room/class gap */
 assert(!extra(1,18,18,70,5,1,1,4));
 assert(!extra(1,18,14,85,5,1,1,4));
 assert(!extra(1,18,14,70,5,1,2,4)); /* no second additional annual seat */
 assert(!extra(1,18,14,70,5,1,1,5));
 assert(!extra(1,18,14,70,5,1,0,4)); /* ordinary quota, not an optional seat */
 for(int p=0;p<22;p++)if(p==0 || p>=19)assert(!extra(p,18,14,70,5,1,1,4));
 assert(!extra(2,18,14,70,0,0,0,0));
 assert(extra(6,18,14,70,3,0,0,1)); /* a real zero annual quota is supported */
 PortalQuality q[22]={0};
 for(int p=0;p<22;p++){q[p].pos=p;q[p].ideal=4;q[p].hs_annual_valid=1;q[p].incoming_best=-1;q[p].starters=1;}
 q[1].ideal=5;q[1].count=q[1].base_count=4;q[1].hs_count=q[1].hs_normal_limit=1;
 assert((dri_portal_set_exceptions(q,8)&~DRI_EXCEPTION_READY)==1u);
 int paid[22]={0};paid[1]=1;
 dri_portal_set_hs_reservations(q,paid,8);
 assert(q[1].hs_reserved==1 && dri_hs_reserved_extra_count(&q[1])==1);
 assert((q[1].exception_states&~DRI_EXCEPTION_READY)==(1u<<6));
 /* A paid optional HS source cannot lend its general place to a portal
    acquisition. Fulfilling that reservation preserves, rather than doubles,
    the actual reconstructed charge. */
 q[1].hs_reserved--;
 q[1].count++;q[1].base_count++;q[1].hs_count++;
 assert(dri_hs_extra_count(&q[1])==1);
 uint32_t state=dri_portal_set_exceptions(q,8)&~DRI_EXCEPTION_READY;
 assert(state==(1u<<6)); /* under-ideal extra charges general, not emergency/RB */
 PortalQuality restored[22];memcpy(restored,q,sizeof(q));
 assert((dri_portal_set_exceptions(restored,8)&~DRI_EXCEPTION_READY)==state);
 q[6].base_count=q[6].count=q[6].hs_count=1;q[6].hs_normal_limit=0;
 assert(dri_hs_extra_count(&q[6])==1);
 assert((dri_portal_set_exceptions(q,8)&~DRI_EXCEPTION_READY)==(1u<<12));
 q[7].base_count=q[7].count=q[7].hs_count=1;
 assert(!(dri_portal_set_exceptions(q,8)&~DRI_EXCEPTION_READY));
 /* A secured portal overage shares the same budget, and its presence cannot
    refund a previously charged below-ideal HS exception. */
 memset(&q[7],0,sizeof(q[7]));q[7].pos=7;q[7].ideal=3;q[7].starters=1;
 q[7].base_count=3;q[7].count=4;q[7].incoming_best=60;
 for(int i=0;i<4;i++)q[7].rating[i]=60;
 q[7].incoming[3]=1;
 assert(!(dri_portal_set_exceptions(q,8)&~DRI_EXCEPTION_READY));
 q[6].base_count=q[6].count=q[6].hs_count=0;
 assert((dri_portal_set_exceptions(q,8)&~DRI_EXCEPTION_READY)==(1u<<12));
 /* Normal annual arrivals remain free even in inherited full rooms. */
 q[1].hs_count=1;q[1].base_count=q[1].count=9;
 assert(dri_hs_extra_count(&q[1])==0);
 /* Legacy numeric fixtures that have no annual publication keep their old
    vacancy accounting; actual collectors always set hs_annual_valid. */
 q[1].hs_annual_valid=0;q[1].base_count=q[1].count=4;q[1].hs_count=2;
 assert(dri_hs_extra_count(&q[1])==0);
 return 0;
}
'''
(O/'vacancy34_policy.c').write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(S),str(O/'vacancy34_policy.c'),'-o',str(O/'vacancy34_policy')],check=True)
subprocess.run([str(O/'vacancy34_policy')],check=True)
result={'passed':True,'checks':['Navy-shaped actual position/class shortage','one additional annual place per position','class and 85-player ceilings','QB/K/P/LS exclusions','zero-quota positions','under-ideal extras charge shared general allowance','reload reconstructs charges','third general extra refused','portal overage and HS share allowance','normal annual and legacy controls'],'live_game_verified':False}
(O/'test34_vacancy_policy_results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
