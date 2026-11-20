"""Actual compiled annual-class policy and source-budget reconstruction."""
from pathlib import Path
import os,subprocess,json
S=Path(__file__).resolve().parents[1];O=Path(os.environ['DRI_TEST_OUTPUT_DIR']);O.mkdir(parents=True,exist_ok=True)
c=r'''
#include <assert.h>
#include <string.h>
#include "test29_class.h"
#include "portal_policy.h"
int main(void) {
 int ideal[22]={4,6,0,10,6,3,3,3,3,3,4,4,6,1,4,4,8,4,4,2,2,0};
 int annual[22],again[22],target=class31_annual(ideal,10,2026,annual);
 assert(target>=15 && target<=25 && annual[0]>=1 && !annual[2] && !annual[21]);
 assert(class31_annual(ideal,10,2026,again)==target && !memcmp(annual,again,sizeof(annual)));
 Test29ClassInput in={0};Test29ClassPlan out;
 in.annual_valid=1;in.annual_target=target;in.general=2;
 for(int p=0;p<22;p++) {in.ideal[p]=ideal[p];in.returning[p]=ideal[p]+2;in.vacancy[p]=annual[p];in.candidates[p]=annual[p]+2;}
 test29_class_plan(&in,&out);
 assert(out.target==target && out.planned==target && !out.shortfall && !out.general);
 assert(!memcmp(out.vacancy,annual,sizeof(annual))); /* full rooms do not veto developmental class */
 for(int p=0;p<22;p++) {in.secured[p]=annual[p];in.vacancy[p]=0;}
 test29_class_plan(&in,&out);assert(out.secured==target && !out.remaining && !out.planned);
 in.secured[0]--;in.vacancy[0]=1;test29_class_plan(&in,&out);assert(out.planned==1 && out.vacancy[0]==1);
 in.candidates[0]=0;test29_class_plan(&in,&out);assert(!out.planned && out.shortfall==1);
 /* Four normal arrivals across already-full positions consume zero general
    allowances; a third genuinely discretionary overage cannot borrow QB/RB. */
 PortalQuality rooms[22]={0};
 for(int p=0;p<22;p++){rooms[p].pos=p;rooms[p].ideal=ideal[p];rooms[p].base_count=ideal[p]+annual[p];rooms[p].count=rooms[p].base_count;rooms[p].hs_count=annual[p];rooms[p].hs_normal_limit=annual[p];rooms[p].incoming_best=-1;}
 assert((dri_portal_set_exceptions(rooms,7)&~DRI_EXCEPTION_READY)==1u);
 rooms[5].base_count++;rooms[5].count++;rooms[5].hs_count++;
 uint32_t one=dri_portal_set_exceptions(rooms,7)&~DRI_EXCEPTION_READY;assert(one && one!=1u);
 rooms[6].base_count++;rooms[6].count++;rooms[6].hs_count++;
 assert(dri_portal_set_exceptions(rooms,7)&~DRI_EXCEPTION_READY);
 rooms[7].base_count++;rooms[7].count++;rooms[7].hs_count++;
 assert(!(dri_portal_set_exceptions(rooms,7)&~DRI_EXCEPTION_READY));
 /* Merely planned slots cannot hide a portal extra or fabricate a player. */
 PortalQuality q={.ideal=4,.base_count=4,.count=4,.hs_normal_limit=2};
 assert(dri_hs_normal_ceiling(&q)==4);q.hs_count=1;q.base_count=5;assert(dri_hs_normal_ceiling(&q)==5);
 int ks=0,ps=0;for(unsigned y=2026;y<2035;y++){class31_annual(ideal,10,y,annual);ks+=annual[19];ps+=annual[20];assert(annual[19]<=1 && annual[20]<=1);}
 assert(ks==4 && ps==4); /* 2-place specialist room /4.5 years, no annual specialist mandate */
 return 0;
}
'''
p=O/'class31_policy.c';p.write_text(c)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(S),str(p),'-o',str(O/'class31_policy')],check=True)
subprocess.run([str(O/'class31_policy')],check=True)
r={'passed':True,'checks':['stable annual 15–25 source map','ordinary full-room development has the complete target in primary permissions','secured commitments fulfill source slots','candidate scarcity remains explicit','normal annual HS does not consume shared 2+2+1','three discretionary HS overages cannot borrow portal allowances','forecasts create no exempt players','K/P four-per-nine-year cadence'],'live_game_verified':False}
(O/'test31_class_policy_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
